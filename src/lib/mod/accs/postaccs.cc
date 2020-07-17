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

#include <limits>
#include <cassert>
#include <vector>
#include <set>

#include <boost/algorithm/string/predicate.hpp>

#include "module.h"
#include "ifacedumb.h"
#include "modutil.h"

#include <boost/ptr_container/ptr_vector.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

namespace postaccs {

  extern "C" {
    void run_fun(FOMUS f, void* moddata); // return true when done
    const char* err_fun(void* moddata);
  }

  inline fomus_int todiatonic2(const fomus_int x) {return (x >= 0 ? todiatonic(x) : -todiatonic(-x));}
  inline void getaccsaux(const fomus_rat& frnote, const fomus_rat& facc, const fomus_rat& tonote, fomus_rat& acc1, fomus_rat& acc2) {
    assert((frnote - facc).den == 1);
    assert((tonote - frnote).den == 1);
    fomus_rat ac(tonote - tochromatic(todiatonic((frnote - facc).num) + todiatonic2((tonote - frnote).num)));
    acc1 = module_makerat(ac.num / ac.den, 1);
    acc2 = ac - acc1;
  }
  
  struct noteholder {
    module_noteobj n;
    fomus_rat acc1, acc2;
    fomus_rat p;
    noteholder(const module_noteobj n):n(n), p(module_pitch(n)) {acc1.num = std::numeric_limits<fomus_int>::max();}
    void assign() const {if (acc1.num == std::numeric_limits<fomus_int>::max()) module_skipassign(n); else accs_assign(n, acc1, acc2);}
    void set(const fomus_rat& a) {acc1 = module_makerat(a.num / a.den, 1); acc2 = a - acc1;}
    void setcopy(const noteholder& x) {acc1 = x.acc1; acc2 = x.acc2;}
    void getaccs(const fomus_rat& frnote, const fomus_rat& facc) {getaccsaux(frnote, facc, p, acc1, acc2);}
  };
  inline bool operator<(const noteholder& x, const noteholder& y) {return x.p < y.p;}
  inline bool operator==(const noteholder& x, const noteholder& y) {return x.p == y.p;}

  // by part
  void run_fun(FOMUS f, void* moddata) { // neither of these parameters used
    boost::ptr_vector<noteholder> nos;
    std::vector<noteholder*> nos1, nos2;
    std::set<noteholder*> nos0;
    bool t = false, h = false, mo = false;
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n) break;
      noteholder* nh;
      nos.push_back(nh = new noteholder(n));
      module_markslist m(module_singlemarks(n));
      for (const module_markobj* i(m.marks), *ie(m.marks + m.n); i != ie; ++i) {
	switch (module_markid(*i)) {
	case mark_longtrill:
	  nos1.push_back(nh);
	  t = true;
	  break;
	case mark_longtrill2:
	  nos2.push_back(nh);
	  t = true;
	  break;
	case mark_artharm_base:
	case mark_natharm_string:
	case mark_natharm_sounding:
	case mark_artharm_sounding:
	case mark_natharm_touched:
	case mark_artharm_touched:
	  nos0.insert(nh);
	  h = true;
	  break;
	case mark_trem:
	  nos1.push_back(nh);
	  mo = true;
	  break;
	case mark_trem2:
	  nos2.push_back(nh);
	  mo = true;
	  break;
	default: ;
	}
      }
      if (module_isendchord(n)) {
	if (t && !nos1.empty()) {
	  fomus_int o = todiatonic(module_writtennote(nos1[0]->n)); // assuming everything valid at this point
	  for (std::vector<noteholder*>::iterator i(nos2.begin()); i != nos2.end(); ++i) {
	    fomus_rat d(nos1[0]->p - (*i)->p);
	    if (d > (fomus_int)0 && d < (fomus_int)3) { // valid trill = whole step or less, must be 1 diatonic step
	      (*i)->set((*i)->p - tochromatic(o + 1));
	    }
	  }
	}
	if (mo && !nos1.empty() && !nos2.empty() && nos1.size() == nos2.size()) { // if same notes in a tremolo
	  std::multiset<noteholder*> p1, p2;
	  for (std::vector<noteholder*>::iterator i(nos1.begin()); i != nos1.end(); ++i) p1.insert(*i);
	  for (std::vector<noteholder*>::iterator i(nos2.begin()); i != nos2.end(); ++i) p2.insert(*i);
	  if (std::equal(p1.begin(), p1.end(), p2.begin())) {
	    std::multiset<noteholder*>::iterator x2(p2.begin());
	    for (std::multiset<noteholder*>::const_iterator x1(p1.begin()); x1 != p1.end(); ++x1, ++x2) (*x2)->setcopy(**x1);
	  }
	}
	if (h && !nos0.empty()) { // written harm, in unison w/ base note
	  noteholder& b(**nos0.begin());
	  fomus_rat n = module_pitch(b.n);
	  fomus_rat wn = module_fullacc(b.n);
	  for (std::set<noteholder*>::iterator i(boost::next(nos0.begin())); i != nos0.end(); ++i) (*i)->getaccs(n, wn);
	}
	std::for_each(nos.begin(), nos.end(), boost::lambda::bind(&noteholder::assign, boost::lambda::_1));
	nos1.clear();
	nos2.clear();
	nos0.clear();
	nos.clear();
	t = h = mo = false;
      }
    }
    std::for_each(nos.begin(), nos.end(), boost::lambda::bind(&noteholder::assign, boost::lambda::_1));
  }

  const char* err_fun(void* moddata) {return 0;}

}

using namespace postaccs;

void module_fill_iface(void* moddata, void* iface) {
  ((dumb_iface*)iface)->moddata = moddata;
  ((dumb_iface*)iface)->run = run_fun;
  ((dumb_iface*)iface)->err = err_fun;
};
const char* module_longname() {return "Accidentals Post-processing";}
const char* module_author() {return "(fomus)";}
const char* module_doc() {return "Handles accidentals in special situations such as trills and artificial harmonics.";}
void* module_newdata(FOMUS f) {return 0;}
void module_freedata(void* dat) {}
int module_priority() {return 1000;}
enum module_type module_type() {return module_modaccs;}
const char* module_initerr() {return 0;}
void module_init() {}
void module_free() {/*assert(newcount == 0);*/}
int module_engine_iface() {return ENGINE_INTERFACEID;} // dumb
int module_itertype() {return module_bymeas | module_byvoice | module_firsttied | module_norests;} // notes aren't divided yet, so they reach across measures
//const char* module_engine(void* d) {return module_setting_sval(module_peeknextpart(0), enginemodid);}

// ------------------------------------------------------------------------------------------------------------------------

int module_get_setting(int n, module_setting* set, int id) {return 0;}
void module_ready() {}
const char* module_engine(void* f) {return "dumb";}
int module_sameinst(module_obj a, module_obj b) {return true;}
