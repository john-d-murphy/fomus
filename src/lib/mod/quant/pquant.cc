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

#include "ifacedumb.h"
#include "module.h"
#include "modutil.h"

namespace pquant {

  int qtsid;

  extern "C" {
  void dumb_run(FOMUS fom, void* moddata); // return true when done
  const char* dumb_err(void* moddata);
  }

  void dumb_run(FOMUS fom, void* moddata) { // return true when done
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n)
        break;
      pquant_assign(
          n, roundto(module_vpitch(n),
                     module_makerat(1, module_setting_ival(n, qtsid) ? 2 : 1)));
    }
  }
  const char* dumb_err(void* moddata) {
    return 0;
  }
  void dumb_free_moddata(void* moddata) {
  } // free up mod_data structure and choices vector when finished

} // namespace pquant

using namespace pquant;

int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
void module_fill_iface(void* moddata, void* iface) { // module
  ((dumb_iface*) iface)->moddata = 0;
  ((dumb_iface*) iface)->run = dumb_run;
  ((dumb_iface*) iface)->err = dumb_err;
}

const char* module_longname() {
  return "Quantize Pitches";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Quantizes pitches to semitones or quartertones.";
}
void* module_newdata(FOMUS f) {
  return 0;
}
void module_freedata(void* dat) {}
int module_priority() {
  return 10;
}
enum module_type module_type() {
  return module_modpquant;
}
const char* module_initerr() {
  return 0;
}
int module_itertype() {
  return module_bymeas | module_norests | module_noperc;
}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "quartertones"; // docscat{quant}
    set->type = module_bool;
    set->descdoc =
        "Whether or not pitches are quantized to quartertone values.  "
        "Set this to `yes' if you want quartertones.";
    // set->typedoc = 0 /*qtstype*/;

    module_setval_int(&set->val, 0);

    set->loc = module_locnote;
    // set->valid = 0;
    set->uselevel = 1;
    qtsid = id;
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
