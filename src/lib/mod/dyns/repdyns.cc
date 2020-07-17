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
#include <set>
#include <limits>

#include <boost/utility.hpp> // next & prior

#include "module.h"
#include "ifacedumb.h"

#include "ilessaux.h"
using namespace ilessaux;
#include "debugaux.h"

namespace repdyns {

  extern "C" {
    void run_fun(FOMUS f, void* moddata); // return true when done
    const char* err_fun(void* moddata);
  }

  int maxrestdurid, repeatid;
  
  std::set<int> thedyns;

  //#error "removes some dyns!"
  void run_fun(FOMUS f, void* moddata) {
    fomus_rat lastoff = {-std::numeric_limits<fomus_int>::max() / 2, 1};
    int dy = -1, insdy = -1;
    fomus_rat stopinsdy;
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n) break;
      module_markslist ml(module_singlemarks(n));
      fomus_rat ti(module_time(n));
      bool rep = module_setting_ival(n, repeatid);
      module_noteobj n0 = n;
      module_markslist ml0 = ml;
      while (true) {
	for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) {
	  int i = module_markid(*m);
	  if (thedyns.find(i) != thedyns.end()) {
	    dy = i;
	    if (n0 == n) goto GOTIT2; else goto GOTIT1;
	  }
	}
	if (dy >= 0) break;
	n0 = module_peeknextnote(n0);
	if (!n0) {
	  do {
	    marks_assign_done(n);
	    n = module_nextnote();
	  } while (n);
	  return;
	}
	ml0 = module_singlemarks(n0);
      }
    GOTIT1: // dy is updated--no symbol here
      if (ti - lastoff > module_setting_rval(n, maxrestdurid)) { // module_setting_rval(n, repeatid) 
	// bool skp = false;
	// for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) { // delete anything else
	//   int i = module_markid(*m);
	//   if (i == dy) {skp = true; continue;}
	//   if (rep && thedyns.find(i) != thedyns.end()) marks_assign_remove(n, i, 0, fomus_make_nullval());
	// }
	// if (!skp) {
	if (rep) {
	  marks_assign_add(n, dy, 0, module_makenullval());
	  // }
	  insdy = dy;
	  stopinsdy = module_makerat(-1, 1);
	}
      } else if (insdy >= 0) {
	if (stopinsdy >= (fomus_int)0 && ti > stopinsdy) insdy = -1; // outside of chord now
	else {
	  for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) { 
	    int i = module_markid(*m);
	    if (i == insdy) { // get rid of insdy! (and in rest of chord)
	      marks_assign_remove(n, i, 0, module_makenullval());
	      stopinsdy = ti;
	    } else if (thedyns.find(i) != thedyns.end()) {
	      insdy = -1;
	    }
	  }
	}
      }
    GOTIT2:
      fomus_rat et(module_endtime(n));
      if (et > lastoff) lastoff = et;
      marks_assign_done(n);
    }
  }
  
  const char* err_fun(void* moddata) {return 0;}

  const char* maxrestdurtype = "rational>0";
  int valid_maxrestdurtype(const struct module_value val) {
    return module_valid_rat(val, module_makerat(0, 1), module_excl, module_makerat(0, 1), module_nobound, 0, maxrestdurtype);
  }

}

using namespace repdyns;

const char* module_engine(void* d) {return "dumb";}
int module_engine_iface() {return ENGINE_INTERFACEID;}
void module_fill_iface(void* moddata, void* iface) { // module
  ((dumb_iface*)iface)->moddata = moddata;
  ((dumb_iface*)iface)->run = run_fun;
  ((dumb_iface*)iface)->err = err_fun;  
}
const char* module_err(void* data) {return 0;}
const char* module_longname() {return "Repeated Dynamics";}
const char* module_author() {return "(fomus)";}
const char* module_doc() {return "Repeats dynamics after a certain rest interval for improved readability.";}
void* module_newdata(FOMUS f) {return 0;}
void module_freedata(void* dat) {}
int module_priority() {return 100;} // should be AFTER markgroups
enum module_type module_type() {return module_modmarks;}
int module_itertype() {return module_bypart | module_byvoice | module_norests;}
const char* module_initerr() {return 0;}
void module_init() {
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
      set->name = "dyn-repeat-maxrestdur"; // docscat{dyns}
      set->type = module_rat;
      set->descdoc = "The maximum rest duration allowed before a dynamic symbol is redundantly repeated.  "
	"Use this to adjust how FOMUS repeats dynamic symbols for improved readability.";
      set->typedoc = maxrestdurtype; 

      module_setval_int(&set->val, 12);
      
      set->loc = module_locnote;
      set->valid = valid_maxrestdurtype;
      set->uselevel = 2;
      maxrestdurid = id;
    }
    break;
  case 1:
    {
      set->name = "dyn-repeat"; // docscat{dyns}
      set->type = module_bool;
      set->descdoc = "Whether or not to repeat dynamic symbols after a certain rest interval.  "
	"Set this to `yes' to help improve score readability.";
      //set->typedoc = maxrestdurtype; 

      module_setval_int(&set->val, 1);
      
      set->loc = module_locnote;
      //set->valid = valid_maxrestdurtype;
      set->uselevel = 2;
      repeatid = id;
    }
    break;
  default:
    return 0;
  }
  return 1;
}
void module_ready() {}
int module_sameinst(module_obj a, module_obj b) {return true;}
