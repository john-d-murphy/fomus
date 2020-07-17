// -*- c++ -*-

/*
    Copyright (C) 2009, 2010, 2011, 2012, 2013  David Psenicka
    This file is part of FOMUS.

    FOMUS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FOMUS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#include <algorithm>
#include <set>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/compare.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include "module.h"
#include "ifacedumb.h"

#include "debugaux.h"
#include "marksaux.h"
using namespace marksaux;
#include "ilessaux.h"
using namespace ilessaux;

namespace markgrs {

  int pairsid, listid;
  
  extern "C" {
    void markgrs_run_fun(FOMUS f, void* moddata); // return true when done
    const char* markgrs_err_fun(void* moddata);
  }

  int icompare(const std::string& x, const std::string& y) {
    std::string::const_iterator i(x.begin()), j(y.begin());
    while (true) {
      if (i == x.end()) return (j == y.end() ? 0 : -1);
      if (j == y.end()) return 1;
      if (!boost::algorithm::is_iequal()(*i, *j)) return (boost::algorithm::is_iless()(*i, *j) ? -1 : 1);
      ++i; ++j;
    }
  }
  
  struct markopts {
    std::set<std::string, isiless> mks;
    std::string abs; // mark that means absence of other marks
    std::string st; // state
    bool fo; // found a match
    int id;
    markopts():id(0) {}
    //bool operator==(const markopts& x) {return abs == x.abs && std::equal(mks, x.mks);}
    bool operator<(const markopts& x) const;
    void activate() {st = abs; id = module_strtomark(st.c_str()); if (id < 0) id = mark_text;}
    void resfo() {fo = false;}
    bool check(const std::string& a, const module_markobj m, boost::ptr_multiset<mark, markless>& orig) {
      DBG("a=\"" << a << "\", st=\"" << st << "\", mid=" << module_markid(m) << std::endl);
      if (boost::algorithm::iequals(a, st)) { // don't need this mark, throw it away
	orig.insert(new mark(m));
	fo = true; return true;
      }
      if ((!abs.empty() && boost::algorithm::iequals(abs, a)) || mks.find(a) != mks.end()) { // user change! leave it and change state
	st = a;
	fo = true; return true;	  
      }
      return false;
    }
    // bool remove(const std::string& a, const module_markobj m, boost::ptr_multiset<mark, markless>& orig) {
    //   DBG("MARKGRPS +++" << std::endl);
    //   if ((!abs.empty() && boost::algorithm::iequals(abs, a)) || mks.find(a) != mks.end()) {
    // 	orig.insert(new mark(m));
    // 	DBG("MARKGRPS ---" << std::endl);
    // 	return true;
    //   }
    //   return false;
    // }
    void found(boost::ptr_multiset<mark, markless>& ins) {
      if (!fo) { // means change to absence
	if (!abs.empty() && !boost::algorithm::iequals(st, abs)) {
	  ins.insert(id == mark_text ? new mark(id, abs) : new mark(id));
	  st = abs;
	}
      }
    }
  };

  bool markopts::operator<(const markopts& x) const {
    if (mks.size() < x.mks.size()) return true; // smaller groups first (also faster sort)
    if (mks.size() > x.mks.size()) return false;
    int c = icompare(abs, x.abs);
    if (c < 0) return true;
    if (c > 0) return false;
    for (std::set<std::string>::iterator i1(mks.begin()), i2(x.mks.begin()); i1 != mks.end(); ++i1) {
      int d = icompare(*i1, *i2);
      if (d < 0) return true;
      if (d > 0) return false;	
    }
    return false;
  }
  
  template<typename I, typename T>
  inline void for_each_until(I first, const I& last, T& pred) {
    while (first != last) if (pred(*first++)) return;
  }

  void markgrs_run_fun(FOMUS f, void* moddata) {
    std::auto_ptr<boost::ptr_set<markopts> > opts;
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n) break;
      DBG("MARKGRPS note " << module_time(n) << std::endl);
      module_value li(module_setting_val(n, listid));
      assert(li.type == module_list);
      std::set<std::string, isiless> inc;
      for (module_value *i(li.val.l.vals), *ie(li.val.l.vals + li.val.l.n); i != ie; ++i) inc.insert(i->val.s);
      std::auto_ptr<boost::ptr_set<markopts> > opts0(new boost::ptr_set<markopts>);
      module_value v(module_setting_val(module_nextpart(), pairsid));
      assert(v.type == module_list);
      for (module_value *i(v.val.l.vals), *ie(v.val.l.vals + v.val.l.n); i != ie; ++i) {
	bool skip = true;
	assert(i->type == module_list);
	std::auto_ptr<markopts> mo(new markopts);
	for (module_value *j(i->val.l.vals), *je(i->val.l.vals + i->val.l.n); j != je; ++j) {
	  assert(j->type == module_string);
	  std::string x(j->val.s);
	  if (inc.find(x) != inc.end()) skip = false;
	  if (!x.empty()) {
	    if (x[0] == '-') { // "absence" mark
	      mo->abs = x.substr(1);
	      if (inc.find(mo->abs) != inc.end()) skip = false;
	    } else {
	      mo->mks.insert(x);
	    }
	  }
	}
	if (!skip) {
	  if (opts.get()) {
	    boost::ptr_set<markopts>::iterator x(opts->find(*mo));
	    if (x != opts->end()) {opts0->transfer(x, *opts); continue;}
	  }
	  mo->activate();
	  opts0->insert(mo);
	}
      }
      opts = opts0;
      std::for_each(opts->begin(), opts->end(), boost::lambda::bind(&markopts::resfo, boost::lambda::_1));
      boost::ptr_multiset<mark, markless> orig; // remove!--(by type)
      boost::ptr_multiset<mark, markless> ins;
      // bool istl = module_istiedleft(n);
      //DBG("MARKGRPS istl = " << istl << std::endl);
      module_markslist l(module_singlemarks(n));
      for (const module_markobj* i(l.marks), *ie(l.marks + l.n); i != ie; ++i) {
	int id = module_markid(*i);
	const char* s;
	switch (id) {
	case mark_text:
	case mark_italictextabove:
	case mark_italictextbelow:
	case mark_stafftext:
	  s = module_markstring(*i);
	  if (!s) goto SKIPIT;
	  break;
	default:
	  s = module_marktostr(id);
	}
	if (s[0]) {
	  std::string a(s);
	  assert(!a.empty());
	  // if (istl) {
	  //   DBG("MARKGRPS ***" << std::endl);
	  //   for_each_until(opts->begin(), opts->end(), boost::lambda::bind(&markopts::remove, boost::lambda::_1, boost::lambda::constant_ref(a), *i, boost::lambda::var(orig)));
	  // } else {
	  for_each_until(opts->begin(), opts->end(), boost::lambda::bind(&markopts::check, boost::lambda::_1, boost::lambda::constant_ref(a), *i, boost::lambda::var(orig)));
	  // }
	}
      SKIPIT: ;
      }
      if (!module_istiedleft(n)) std::for_each(opts->begin(), opts->end(), boost::lambda::bind(&markopts::found, boost::lambda::_1, boost::lambda::var(ins))); // ones that weren't found are set to "absent"
      inplace_set_difference<boost::ptr_multiset<mark, markless>::iterator, boost::ptr_multiset<mark, markless>::iterator, markless>(orig, ins);
      std::for_each(orig.begin(), orig.end(), boost::lambda::bind(&mark::assrem, boost::lambda::_1, n));
      std::for_each(ins.begin(), ins.end(), boost::lambda::bind(&mark::assass, boost::lambda::_1, n));
      marks_assign_done(n);
    }
  }
  const char* markgrs_err_fun(void* moddata) {return 0;}

  const char* pairstype = "((string_mark string_mark ...) (string_mark string_mark ...) ...)";
  int valid_groups_aux(int n, const struct module_value val) {return module_valid_listofstrings(val, -1, -1, 1, -1, 0, 0);}
  int valid_groups(const struct module_value val) {return module_valid_listofvals(val, -1, -1, valid_groups_aux, pairstype);}

  const char* listtype = "(string_mark string_mark ...)";
  int valid_list(const struct module_value val) {return module_valid_listofstrings(val, -1, -1, 1, -1, 0, listtype);}

}

using namespace markgrs;

void module_fill_iface(void* moddata, void* iface) {
  ((dumb_iface*)iface)->moddata = 0;
  ((dumb_iface*)iface)->run = markgrs_run_fun;
  ((dumb_iface*)iface)->err = markgrs_err_fun;
};

// ------------------------------------------------------------------------------------------------------------------------

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {  
  case 0:
    {
      set->name = "mark-group-defs"; // docscat{marks}
      set->type = module_list_stringlists;
      set->descdoc = "Groups of marks that cancel each other when they appear in one voice.  Examples are dynamic markings and \"on/off\" mark pairs such as "
	"`pizz' and `arco'.  The strings in each inner list should match either a mark id (a single mark, not a spanner) or the string of a text mark.  "
	"Strings prefixed with a `-' indicate the mark that should appear in the absence of the other marks (i.e., the mark that indicates a return to a \"normal\" state)."
	"  (pizz -arco) would make sense for a string instrument, for example.  You would then use `pizz' marks on all pizzicato notes and "
	"never `arco' since FOMUS would add these automatically.";
      set->typedoc = pairstype; 
      
      module_setval_list(&set->val, 6);
      {
	module_value *x = set->val.val.l.vals + 0;
	module_setval_list(x, 2);
	module_setval_string(x->val.l.vals + 0, "pizz");
	module_setval_string(x->val.l.vals + 1, "-arco");
      } {
	module_value *x = set->val.val.l.vals + 1;
	module_setval_list(x, 2);
	module_setval_string(x->val.l.vals + 0, "mute");
	module_setval_string(x->val.l.vals + 1, "-unmute");
      } {
	module_value *x = set->val.val.l.vals + 2;
	module_setval_list(x, 3);
	module_setval_string(x->val.l.vals + 0, "vib");
	module_setval_string(x->val.l.vals + 1, "moltovib");
	module_setval_string(x->val.l.vals + 2, "-nonvib");
      } {
	module_value *x = set->val.val.l.vals + 3;
	module_setval_list(x, 3);
	module_setval_string(x->val.l.vals + 0, "leg");
	module_setval_string(x->val.l.vals + 1, "moltoleg");
	module_setval_string(x->val.l.vals + 2, "-nonleg");
      } {
	module_value *x = set->val.val.l.vals + 4;
	module_setval_list(x, 12);
	module_setval_string(x->val.l.vals + 0, "spic");
	module_setval_string(x->val.l.vals + 1, "tall");
	module_setval_string(x->val.l.vals + 2, "punta");
	module_setval_string(x->val.l.vals + 3, "pont");
	module_setval_string(x->val.l.vals + 4, "tasto");
	module_setval_string(x->val.l.vals + 5, "legno");
	module_setval_string(x->val.l.vals + 6, "flaut");
	module_setval_string(x->val.l.vals + 7, "etouf");
	module_setval_string(x->val.l.vals + 8, "table");
	module_setval_string(x->val.l.vals + 9, "cuivre");
	module_setval_string(x->val.l.vals + 10, "bellsup");
	module_setval_string(x->val.l.vals + 11, "-ord");
      } {
	module_value *x = set->val.val.l.vals + 5; 
	module_setval_list(x, 12);
	module_setval_string(x->val.l.vals + 0, "pppppp");
	module_setval_string(x->val.l.vals + 1, "ppppp");
	module_setval_string(x->val.l.vals + 2, "pppp");
	module_setval_string(x->val.l.vals + 3, "ppp");
	module_setval_string(x->val.l.vals + 4, "pp");
	module_setval_string(x->val.l.vals + 5, "p");
	module_setval_string(x->val.l.vals + 6, "mp");
	module_setval_string(x->val.l.vals + 7, "ffff");
	module_setval_string(x->val.l.vals + 8, "fff");
	module_setval_string(x->val.l.vals + 9, "ff");
	module_setval_string(x->val.l.vals + 10, "f");
	module_setval_string(x->val.l.vals + 11, "mf");
      }
      set->loc = module_locpart;
      set->valid = valid_groups; // no range
      set->uselevel = 2;
      pairsid = id;
      break;
    }
  case 1:
    {
      set->name = "mark-groups"; // docscat{marks}
      set->type = module_list_strings;
      set->descdoc = "A list of IDs that indicate which mark groups defined in `mark-group-defs' are to take effect.  "
	"An ID in this case is simply one of the marks or strings that are in a group (e.g., \"mp\" specifies the group containing all of the dynamic symbols including \"mp\").  "
	"Use this setting to switch mark groups on and off at any place in the score.";
      set->typedoc = listtype; 
      
      module_setval_list(&set->val, 6);
      module_setval_string(set->val.val.l.vals + 0, "mf");
      module_setval_string(set->val.val.l.vals + 1, "pizz");
      module_setval_string(set->val.val.l.vals + 2, "mute");
      module_setval_string(set->val.val.l.vals + 3, "vib");
      module_setval_string(set->val.val.l.vals + 4, "leg");
      module_setval_string(set->val.val.l.vals + 5, "pont");
      
      set->loc = module_locnote;
      set->valid = valid_list; // no range
      set->uselevel = 2;
      listid = id;
      break;
    }
  default:
    return 0;
  }
  return 1;
}

const char* module_longname() {return "Mark Groups";}
const char* module_author() {return "(built-in)";}
const char* module_doc() {return "Processes marks that belong in groups (e.g., dynamic markings).";}
void* module_newdata(FOMUS f) {return 0;}
void module_freedata(void* dat) {}
const char* module_err(void* dat) {return 0;}
int module_priority() {return 0;}
enum module_type module_type() {return module_modmarks;}
const char* module_initerr() {return 0;}
void module_init() {}
void module_free() {}
int module_itertype() {return module_bypart | module_byvoice;}
int module_engine_iface() {return ENGINE_INTERFACEID;}
const char* module_engine(void*) {return "dumb";}
void module_ready() {}
int module_sameinst(module_obj a, module_obj b) {return true;}
