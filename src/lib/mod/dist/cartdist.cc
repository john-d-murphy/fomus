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
#include <cmath>
#include <limits>
#include <set>

#include "ifacedist.h"
#include "module.h"
#include "modutil.h"

namespace cartdist {

  struct etnode {
    const module_noteobj note;
    const fomus_rat et;
    etnode(const fomus_rat& et, const module_noteobj& note)
        : note(note), et(et) {}
  };
  inline bool operator<(const etnode& x, const etnode& y) {
    if (x.et != y.et)
      return x.et < y.et;
    return x.note < y.note;
  }

  typedef std::set<etnode> prevmap;
  struct distdata {
    int octdistid, beatdistid; // these belong here, shouldn't be global
    const bool byendtime;      // does note1's point = endtime instead of time?
    const fomus_float rng;
    prevmap prevs;
    distdata(const dist_iface& iface)
        : octdistid(iface.data.octdist_setid),
          beatdistid(iface.data.beatdist_setid),
          byendtime(iface.data.byendtime), rng(iface.data.rangemax) {}
    fomus_float dist(const module_noteobj note1, const module_noteobj note2) {
      fomus_float x =
          (module_time(note2) -
           (byendtime ? module_tiedendtime(note1) : module_time(note1))) *
          module_setting_fval(note2, beatdistid);
      fomus_float y = diff(module_pitch(note2), module_pitch(note1)) *
                      module_setting_fval(note2, octdistid) * ((double) 1 / 12);
      return sqrt(x * x + y * y);
    }
    bool isoutofmaxrange(
        const module_noteobj note1,
        const module_noteobj
            note2) { // return true if possible to add notes to beginning or end
                     // and be inside the max range (rng)
      fomus_rat et;
      if (byendtime) {
        et = module_tiedendtime(note1);
        for (prevmap::const_iterator i(
                 prevs.upper_bound(etnode(module_tiedendtime(note1), 0)));
             i != prevs.end(); ++i) {
          if (module_less(i->note, note1)) {
            et = i->et;
            break;
          }
        }
        prevs.insert(prevmap::value_type(module_tiedendtime(note1), note1));
      } else
        et = module_time(note1);
      return (module_time(note2) - et) *
                 module_setting_fval(note2, beatdistid) >
             rng;
    }
  };

  extern "C" {
  fomus_float dist_dist(void* moddata, module_noteobj note1,
                        module_noteobj note2);
  void free_moddata(void* moddata);
  int is_outofrange(void* moddata, module_noteobj note1, module_noteobj note2);
  }

  fomus_float dist_dist(void* moddata, module_noteobj note1,
                        module_noteobj note2) {
    return ((distdata*) moddata)->dist(note1, note2);
  }
  void free_moddata(void* moddata) {
    delete (distdata*) moddata;
  }
  int is_outofrange(void* moddata, module_noteobj note1, module_noteobj note2) {
    return ((distdata*) moddata)->isoutofmaxrange(note1, note2);
  }
}; // namespace cartdist

using namespace cartdist;

void aux_fill_iface(void* iface) {
  ((dist_iface*) iface)->moddata = new distdata(*(dist_iface*) iface);
  ((dist_iface*) iface)->dist = dist_dist;
  ((dist_iface*) iface)->is_outofrange = is_outofrange;
  ((dist_iface*) iface)->free_moddata = free_moddata;
}
int aux_ifaceid() {
  return DIST_INTERFACEID;
} // divrules.cc must provide a unique one in header divrules.h file

const char* module_longname() {
  return "Cartesian Distance";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Calculates distance between two objects by taking the square root of "
         "temporal difference squared plus difference in pitch squared (i.e. "
         "Cartesian distance if time and pitch are the axes on a "
         "two-dimensional grid).";
}
enum module_type module_type() {
  return module_modaux;
}
int module_get_setting(int n, module_setting* set, int id) {
  return 0;
}
void module_ready() {}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}
const char* module_initerr() {
  return 0;
}
