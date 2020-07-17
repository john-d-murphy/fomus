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

#include <cassert>
#include <map>
#include <set>
#include <string>
#include <limits>

#include <boost/utility.hpp> // next & prior
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "module.h"
#include "ifacedumb.h"
#include "modutil.h"

#include "ilessaux.h"
using namespace ilessaux;
#include "debugaux.h"

namespace phrdyns {

  const char* ierr = 0;
  
  int maxrestdurid, beginid, endid, beginmarksid, endmarksid,
    dynrangeid, dynsymrangeid, dodynsid;
  
  extern "C" {
    void run_fun(FOMUS f, void* moddata); // return true when done
    const char* err_fun(void* moddata);
  }

  enum module_markids dynmarks[12] = {
    mark_pppppp,
    mark_ppppp,
    mark_pppp,
    mark_ppp,
    mark_pp,
    mark_p,
    mark_mp,
    mark_mf,
    mark_f,
    mark_ff,
    mark_fff,
    mark_ffff
  };
  std::set<int> thedyns;
  std::map<std::string, fomus_float, isiless> symvals;

  template<typename V, typename T1, typename T2>
  inline fomus_float scale(const V& val, const T1& a, const T1& b, const T2& x, const T2& y) {
    return x + (((val - a) / (b - a)) * (y - x));
  }

  inline void getnthf(const module_noteobj no, const int id, fomus_float& v1, fomus_float& v2) {
    const module_value* x = module_setting_val(no, id).val.l.vals;
    v1 = GET_F(x[0]);
    v2 = GET_F(x[1]);
  }
  inline void getnths(const module_noteobj no, const int id, const char* &v1, const char* &v2) {
    const module_value* x = module_setting_val(no, id).val.l.vals;
    v1 = x[0].val.s;
    v2 = x[1].val.s;
  }
  
  void dodyn(const module_noteobj n, const module_markslist& ml) {
    //module_markslist ml(module_singlemarks(n));
    if (!module_setting_ival(n, dodynsid)) return;
    for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) { 
      int i = module_markid(*m);
      if (thedyns.find(i) != thedyns.end()) return;
    }
    fomus_float mi, ma;
    getnthf(n, dynrangeid, mi, ma);
    getnthf(n, dynrangeid, mi, ma);
    const char *x1, *x2;
    getnths(n, dynsymrangeid, x1, x2);
    fomus_float mis = symvals.find(x1)->second;
    fomus_float mas = symvals.find(x2)->second + 1;
    assert(mis >= 0 && mis < 12);
    assert(mas >= 0 && mas < 12);
    int dy = (int)scale(GET_F(module_dyn(n)), mi, ma, mis, mas);
    if (dy < 0) dy = 0;
    if (dy >= 12) dy = 12;
    bool skp = false;
    module_markids dym = dynmarks[dy];
    for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) { 
      int i = module_markid(*m);
      if (i == dym) {skp = true; continue;}
      if (thedyns.find(i) != thedyns.end()) marks_assign_remove(n, i, 0, module_makenullval());
    }
    if (!skp) marks_assign_add(n, dym, 0, module_makenullval());
  }

  bool hasamark(const module_noteobj n, const module_markslist& ml, const int setid) {
    //module_markslist ml(module_singlemarks(n));
    module_value li(module_setting_val(n, setid));
    assert(li.type == module_list);
    for (const module_value *i(li.val.l.vals), *ie(li.val.l.vals + li.val.l.n); i < ie; ++i) {
      assert(i->type == module_string);
      std::string mn(i->val.s);
      boost::algorithm::trim(mn);
      for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) { 
	int i = module_markid(*m);
	if (boost::algorithm::ilexicographical_compare(mn, module_marktostr(i))) return true;
	int d = module_markbaseid(i);
	if (d != i && boost::algorithm::ilexicographical_compare(mn, module_marktostr(d))) return true;
      }
    }
    return false;
  }
  
  void run_fun(FOMUS f, void* moddata) {
    fomus_rat lastoff = {-1, 1};
    module_noteobj ln = 0, ln0 = 0;
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n) break;
      fomus_rat ti(module_time(n));
      //bool rep = module_setting_rval(n, repeatid);
      module_noteobj n0 = n;
      if (lastoff < (fomus_int)0 || ti - lastoff > module_setting_rval(n, maxrestdurid)) { // module_setting_rval(n, repeatid)
	if (ln) {
	  module_markslist lml(module_singlemarks(ln));
	  if (module_setting_ival(ln, endid) || hasamark(ln, lml, endmarksid)) dodyn(ln, lml);
	}
	module_markslist ml(module_singlemarks(n));
	if (module_setting_ival(n, beginid) || hasamark(n, ml, beginmarksid)) {dodyn(n, ml); n = 0;}
      }
      fomus_rat et(module_endtime(n0));
      if (et > lastoff) lastoff = et;
      if (ln0) marks_assign_done(ln0);
      ln = n;
      ln0 = n0;
    }
    if (ln) {
      module_markslist lml(module_singlemarks(ln));
      if (module_setting_ival(ln, endid) || hasamark(ln, lml, endmarksid)) dodyn(ln, lml);
    }
    if (ln0) marks_assign_done(ln0);
  }
  
  const char* err_fun(void* moddata) {return 0;}

  const char* maxrestdurtype = "rational>0";
  int valid_maxrestdurtype(const struct module_value val) {
    return module_valid_rat(val, module_makerat(0, 1), module_excl, module_makerat(0, 1), module_nobound, 0, maxrestdurtype);
  }

  const char* markslisttype = "(string_mark string_mark ...)";
  int valid_markslisttypeaux(int i, const char* str) {
    std::string x(str);
    boost::algorithm::trim(x);
    boost::algorithm::to_lower(x);
    return (module_strtomark(str) >= 0);
  }
  int valid_markslisttype(const struct module_value val) {
    return module_valid_listofstrings(val, -1, -1, -1, -1, valid_markslisttypeaux, markslisttype);
  }
}

using namespace phrdyns;

const char* module_engine(void* d) {return "dumb";}
int module_engine_iface() {return ENGINE_INTERFACEID;}
void module_fill_iface(void* moddata, void* iface) { // module
  ((dumb_iface*)iface)->moddata = moddata;
  ((dumb_iface*)iface)->run = run_fun;
  ((dumb_iface*)iface)->err = err_fun;  
}
const char* module_err(void* data) {return 0;}
const char* module_longname() {return "Phrase Dynamics";}
const char* module_author() {return "(fomus)";}
const char* module_doc() {return "Converts dynamic values to score symbols at the beginnings and ends of phrases.";}
void* module_newdata(FOMUS f) {return 0;}
void module_freedata(void* dat) {}
int module_priority() {return -100;} // should be first
enum module_type module_type() {return module_moddynamics;}
int module_itertype() {return module_bypart | module_byvoice | module_norests;}
const char* module_initerr() {return ierr;}
void module_init() {
  symvals.insert(std::map<std::string, fomus_float>::value_type("pppppp", 0));
  symvals.insert(std::map<std::string, fomus_float>::value_type("ppppp", 1));
  symvals.insert(std::map<std::string, fomus_float>::value_type("pppp", 2));
  symvals.insert(std::map<std::string, fomus_float>::value_type("ppp", 3));
  symvals.insert(std::map<std::string, fomus_float>::value_type("pp", 4));
  symvals.insert(std::map<std::string, fomus_float>::value_type("p", 5));
  symvals.insert(std::map<std::string, fomus_float>::value_type("mp", 6));
  symvals.insert(std::map<std::string, fomus_float>::value_type("ffff", 11));
  symvals.insert(std::map<std::string, fomus_float>::value_type("fff", 10));
  symvals.insert(std::map<std::string, fomus_float>::value_type("ff", 9));
  symvals.insert(std::map<std::string, fomus_float>::value_type("f", 8));
  symvals.insert(std::map<std::string, fomus_float>::value_type("mf", 7));
  thedyns.insert(std::set<int>::value_type(mark_pppppp));
  thedyns.insert(std::set<int>::value_type(mark_ppppp));
  thedyns.insert(std::set<int>::value_type(mark_pppp));
  thedyns.insert(std::set<int>::value_type(mark_ppp));
  thedyns.insert(std::set<int>::value_type(mark_pp));
  thedyns.insert(std::set<int>::value_type(mark_p));
  thedyns.insert(std::set<int>::value_type(mark_mp));
  thedyns.insert(std::set<int>::value_type(mark_ffff));
  thedyns.insert(std::set<int>::value_type(mark_fff));
  thedyns.insert(std::set<int>::value_type(mark_ff));
  thedyns.insert(std::set<int>::value_type(mark_f));
  thedyns.insert(std::set<int>::value_type(mark_mf));
}
void module_free() {}
int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {  
  case 0:
    {
      set->name = "dynphrase-maxrestdur"; // docscat{dyns}
      set->type = module_rat;
      set->descdoc = "The maximum rest duration allowed anywhere in a \"dynamics phrase,\" calculated for the purpose of attaching dynamic symbols to the beginnings and ends of them.  "
	"Adjust this setting to a value that makes sense for the type of music you are notating.  "
	"A rest duration larger than this value causes FOMUS to separate a musical passage into at least two phrases on either side of the rest.  "
	"These phrases may or may not correspond to actual phrase markings in the score (if there are any).";
      set->typedoc = maxrestdurtype; 

      module_setval_rat(&set->val, module_makerat(1, 2));
      
      set->loc = module_locnote;
      set->valid = valid_maxrestdurtype;
      set->uselevel = 2;
      maxrestdurid = id;
    }
    break;
  case 1:
    {
      set->name = "dynphrase-begin"; // docscat{dyns}
      set->type = module_bool;
      set->descdoc = "Whether or not a dynamic symbol should appear at the beginning of a \"dynamics phrase.\"  "
	"A dynamics phrase is calculated using `dynphrase-maxrestdur', `dynphrase-beginmarks' and `dynphrase-endmarks' "
	"for the purpose of placing dynamic marks at logical locations in the score.";
      //set->typedoc = phrasetype; 

      module_setval_int(&set->val, 1);
      
      set->loc = module_locnote;
      //set->valid = valid_maxrestdurtype;
      set->uselevel = 2;
      beginid = id;
    }
    break;
  case 2:
    {
      set->name = "dynphrase-end"; // docscat{dyns}
      set->type = module_bool;
      set->descdoc = "Whether or not a dynamic symbol should appear at the end of a \"dynamics phrase.\"  "
	"A dynamics phrase is calculated using `dynphrase-maxrestdur', `dynphrase-beginmarks' and `dynphrase-endmarks' "
	"for the purpose of placing dynamic marks at logical locations in the score.";
      //set->typedoc = phrasetype; 

      module_setval_int(&set->val, 1);
      
      set->loc = module_locnote;
      //set->valid = valid_maxrestdurtype;
      set->uselevel = 2;
      endid = id;
    }
    break;
  case 3:
    {
      set->name = "dynphrase-beginmarks"; // docscat{dyns}
      set->type = module_list_strings;
      set->descdoc = "Marks that denote the beginning of a phrase calculated for the purpose of attaching dynamic symbols to the beginnings and ends of them.  "
	"Set this to influence how dynamics phrases are calculated.  "
	"Any mark whose id appears in this list is considered to be the beginning of a phrase."
	;
      set->typedoc = markslisttype; 

      module_setval_list(&set->val, 0); // 0 list
      
      set->loc = module_locnote;
      set->valid = valid_markslisttype;
      set->uselevel = 2;
      beginmarksid = id;
    }
    break;
  case 4:
    {
      set->name = "dynphrase-endmarks"; // docscat{dyns}
      set->type = module_list_strings;
      set->descdoc = "Marks that denote the end of a phrase calculated for the purpose of attaching dynamic symbols to the beginnings and endings of them.  "
	"Set this to influence how dynamics phrases are calculated.  "
	"Any mark whose id appears in this list is considered to be the end of a phrase."
	;
      set->typedoc = markslisttype; 

      module_setval_list(&set->val, 0); // 0 list
      
      set->loc = module_locnote;
      set->valid = valid_markslisttype;
      set->uselevel = 2;
      endmarksid = id;
    }
    break;
  default:
    return 0;
  }
  return 1;
}
void module_ready() {
  dynrangeid = module_settingid("dyn-range");
  if (dynrangeid < 0) {ierr = "missing required setting `dyn-range'"; return;}
  dynsymrangeid = module_settingid("dynsym-range");
  if (dynsymrangeid < 0) {ierr = "missing required setting `dynsym-range'"; return;}
  
  dodynsid = module_settingid("dyns");
  if (dodynsid < 0) {ierr = "missing required setting `dyns'"; return;}
}
int module_sameinst(module_obj a, module_obj b) {return true;}
