// -*- c++ -*-

/*
    Copyright (C) 2009, 2010, 2011  David Psenicka
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

#include <boost/utility.hpp> // next & prior

#include "ifacedumb.h"
#include "module.h"

#include "debugaux.h"

#ifndef NDEBUG
#define NONCOPYABLE , boost::noncopyable
#define _NONCOPYABLE :boost::noncopyable
#else
#define NONCOPYABLE
#define _NONCOPYABLE
#endif

namespace special {

  int doitid;

  extern "C" {
  void run_fun(FOMUS fom, void* moddata); // return true when done
  const char* err_fun(void* moddata);
  }
  void doadds(module_obj a, const module_obj b, const bool bisgr) {
    assert(module_isgrace(a));
    // assert(module_isgrace(b));
    if (!bisgr || module_time(a) < module_time(b) ||
        module_gracetime(a) < module_gracetime(b)) {
      DBG("GRSLUR begin " << module_time(a) << std::endl);
      marks_assign_add(a, mark_graceslur_begin, 0, module_makenullval());
      marks_assign_done(a);
      while (true) {
        a = module_peeknextnote(a);
        if (a == b)
          break;
        module_skipassign(a);
      }
      DBG("GRSLUR end " << module_time(b) << std::endl);
      marks_assign_add(b, mark_graceslur_end, 0, module_makenullval());
      marks_assign_done(b);
    } else {
      while (true) {
        module_skipassign(a);
        if (a == b)
          break;
        a = module_peeknextnote(a);
      }
    }
  }
  void run_fun(FOMUS fom, void* moddata) {
    module_noteobj ingr = 0;
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n)
        break;
      module_noteobj nx = module_peeknextnote(n);
      if (module_isgrace(n) && module_gracetime(n) >= (fomus_int) 0 &&
          module_gracetime(n) <= (fomus_int) 1000) { // ***GRACE NOTE
        if (!ingr) { // grace note and not in grace slur yet...
          if (module_setting_ival(n, doitid))
            ingr = n;
        } else if (!nx) { // grace note and "in grace slur" and at the end
          doadds(ingr, n, true);
          return;
        } // else if (module_isrest(nx)) { // grace note and "in grace slur" and
          // grace rest coming up
        // 	  doadds(ingr, n);
        // 	  ingr = 0;
        // 	  continue;
        // 	}
      } else {      // ***REGULAR NOTE
        if (ingr) { // not a grace note, close the slur
          doadds(ingr, n, false);
          ingr = 0;
          continue;
        }
      }
      if (!ingr)
        module_skipassign(n);
    }
    while (ingr) {
      module_skipassign(ingr);
      ingr = module_peeknextnote(ingr);
    }
  }
  const char* err_fun(void* moddata) {
    return 0;
  }

} // namespace special

using namespace special;

int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
void module_fill_iface(void* moddata, void* iface) { // module
  ((dumb_iface*) iface)->moddata = moddata;
  ((dumb_iface*) iface)->run = run_fun;
  ((dumb_iface*) iface)->err = err_fun;
}
const char* module_err(void* data) {
  return 0;
}
const char* module_longname() {
  return "Grace Note Slurs";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Automatically attaches slurs to grace notes.";
}
void* module_newdata(FOMUS f) {
  return 0;
} // new accsdata;}
void module_freedata(void* dat) {} // delete (accsdata*)dat;}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modmarks;
}
int module_itertype() {
  return module_bymeas | module_byvoice;
}
const char* module_initerr() {
  return 0;
}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}
int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "grace-slurs"; // docscat{grnotes}
    set->type = module_bool;
    set->descdoc = "Whether or not to automatically slur grace notes.  "
                   "Set this to `yes' if you want all grace notes to have "
                   "slurs over them.";
    // set->typedoc = qtstype;

    module_setval_int(&set->val, 1);

    set->loc = module_locnote;
    // set->valid = valid_qts;
    set->uselevel = 2;
    doitid = id;
  } break;
  default:
    return 0;
  }
  return 1;
}
const char* module_engine(void*) {
  return "dumb";
}
void module_ready() {}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
