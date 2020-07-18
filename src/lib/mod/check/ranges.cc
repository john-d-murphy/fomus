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

#include <boost/iostreams/concepts.hpp> // sink
#include <boost/iostreams/stream.hpp>
#include <cassert>
#include <limits>

#include "ifacedumb.h"
#include "module.h"

#include "foutaux.h"
using namespace foutaux;

namespace ranges {

  int minpid, maxpid;

  extern "C" {
  void dumb_run(FOMUS fom, void* moddata); // return true when done
  const char* dumb_err(void* moddata);
  }
  void dumb_run(FOMUS fom, void* moddata) {
    module_partobj pa = 0;
    fomus_int ca = 0, ci = 0;
    fomus_rat ma = {std::numeric_limits<int>::min() + 1, 1};
    fomus_rat mi = {std::numeric_limits<int>::max(), 1};
    module_noteobj n = module_nextnote();
    if (!n)
      return;
    // module_instobj in = module_inst(n);
    do {
      if (!pa)
        pa = module_part(n);
      fomus_rat p(module_pitch(n));
      if (p < module_setting_rval(n, minpid)) {
        ++ci;
        if (p < mi)
          mi = p;
      }
      if (p > module_setting_rval(n, maxpid)) {
        ++ca;
        if (p > ma)
          ma = p;
      }
      module_skipassign(n);
      n = module_nextnote();
    } while (n);
    if (ca > 0) {
      fout << "*** " << ca << " pitch";
      if (ca > 1)
        fout << "es";
      fout << " out of range (highest is "
           << module_pitchtostr(module_makeval(ma)) << ") in part `"
           << module_id(pa) << '\'' << std::endl;
    }
    if (ci > 0) {
      fout << "*** " << ci << " pitch";
      if (ci > 1)
        fout << "es";
      fout << " out of range (lowest is "
           << module_pitchtostr(module_makeval(mi)) << ") in part `"
           << module_id(pa) << '\'' << std::endl;
    }
  }
  const char* dumb_err(void* moddata) {
    return 0;
  }

  const char* minptype = "note | rational0..128";
  int valid_minp(const struct module_value val) {
    return module_valid_rat(val, module_makerat(0, 1), module_incl,
                            module_makerat(128, 1), module_incl, 0, minptype);
  } // also maxp

} // namespace ranges

using namespace ranges;

int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
void module_fill_iface(void* moddata, void* iface) { // module
  ((dumb_iface*) iface)->moddata = 0;
  ((dumb_iface*) iface)->run = dumb_run;
  ((dumb_iface*) iface)->err = dumb_err;
}
const char* module_longname() {
  return "Range Check";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Checks all notes and warns if any are out of range.";
}
void* module_newdata(FOMUS f) {
  return 0;
}
void module_freedata(void* dat) {}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modcheck;
}
const char* module_initerr() {
  return 0;
}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}
int module_itertype() {
  return module_bypart | module_norests | module_noperc;
}

// ------------------------------------------------------------------------------------------------------------------------

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "min-pitch"; // docscat{instsparts}
    set->type = module_notesym;
    set->descdoc = "Minimum pitch (untransposed) allowed for an instrument."
                   "  This is used to check if pitches fall within the correct "
                   "range for an instrument.  "
                   "Set it in an instrument or part definition.";
    set->typedoc = minptype;

    module_setval_int(&set->val, 21);

    set->loc = module_locnote;
    set->valid = valid_minp; // no range
    // set->validdeps = validdeps_minp;
    set->uselevel = 2;
    minpid = id;
    break;
  }
  case 1: {
    set->name = "max-pitch"; // docscat{instsparts}
    set->type = module_notesym;
    set->descdoc = "Maximum pitch (untransposed) allowed for an instrument."
                   "  This is used to check if pitches fall within the correct "
                   "range for an instrument.  "
                   "Set it in an instrument or part definition.";
    set->typedoc = minptype; // same valid fun

    module_setval_int(&set->val, 108);

    set->loc = module_locnote;
    set->valid = valid_minp; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2;
    maxpid = id;
    break;
  }
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
