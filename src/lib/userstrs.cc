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

#include "userstrs.h"
#include "module.h"
#include "schedr.h"
#include "vars.h"

namespace fomus {

  strtoclefmap strstoclefs;
  const std::string clefstostrs[] = {"subbass-8down",
                                     "bass-8down",
                                     "c-baritone-8down",
                                     "f-baritone-8down",
                                     "tenor-8down",
                                     "subbass",
                                     "alto-8down",
                                     "bass",
                                     "mezzosoprano-8down",
                                     "c-baritone",
                                     "f-baritone",
                                     "soprano-8down",
                                     "tenor",
                                     "subbass-8up",
                                     "treble-8down",
                                     "alto",
                                     "bass-8up",
                                     "mezzosoprano",
                                     "c-baritone-8up",
                                     "f-baritone-8up",
                                     "soprano",
                                     "tenor-8up",
                                     "treble",
                                     "alto-8up",
                                     "mezzosoprano-8up",
                                     "soprano-8up",
                                     "treble-8up",
                                     "percussion"};
  const int clefmidpitches[] = {35, 38, 41, 41, 45, 47, 48, 50, 52, 53,
                                53, 55, 57, 59, 59, 60, 62, 64, 65, 65,
                                67, 69, 71, 72, 76, 79, 83, 60};
  //#define NCLEFS ((int)(sizeof(clefstostrs) / sizeof(std::string)))
  std::string clefstypestr;

  // MODULE TYPES
  modtypemap strtomodtypes;
  const std::string modtypestostrs[] = {
      "(no type)",  // type{nomodtype}
      "(built-in)", // type{modinternal}

      "input",  // type{modinput}
      "output", // type{modoutput}

      "engine",    // type{modengine}
      "auxiliary", // type{modaux}

      "measure",       // type{modmeas}
      "timequantize",  // type{modtquant}
      "pitchquantize", // type{modpquant}
      "check",         // type{modcheck}
      "transpose",     // type{modtpose}

      "voices",     // type{modvoices}
      "markevents", // type{modmarkevs}
      //"markevents2", // ty*pe{modmarkevs2}
      "prune",           // type{modprune}
      "voicemarks",      // type{modvmarks}
      "accidentals",     // type{modaccs}
      "cautaccidentals", // type{modcautaccs}
      "staves",          // type{modstaves}
      "reststaves",      // type{modrstaves}
      "staffmarks",      // type{modsmarks}
      "octavesigns",     // type{modocts}
      "dynamics",        // type{moddynamics}

      "divide",      // type{moddivide}
      "mergevoices", // type{modmerge}
      "beams",       // type{modbeams}
      "marks",       // type{modmarks}
      "marklayouts", // type{modmarklayouts}

      "parts",     // type{modparts}
      "metaparts", // type{modmetaparts}
      "percnotes", // type{modpercnotes}
      "special"    // type{modspecial}
  };

#define NMODS ((int) (sizeof(modtypestostrs) / sizeof(const std::string)))

  // SETTING LOCATION TYPES
  setlocmap strtosetlocs;
  const std::string setlocstostrs[] = {
      "initfile", "score", "import",  "export", "inst", "percinst",
      "partmap",  "part",  "measdef", "staff",  "clef", "note"};

#define NSETLOCS ((int) (sizeof(setlocstostrs) / sizeof(const std::string)))

  void initstrenummaps() {
    for (int i = 0; i < clef_nclefs; ++i)
      strstoclefs.insert(strtoclefmap_val(clefstostrs[i], i));
    std::ostringstream out;
    int ie = clef_nclefs - 1;
    for (int i = 0; i < ie; ++i)
      out << clefstostrs[i] << '|';
    out << clefstostrs[ie];
    clefstypestr = out.str();
    assert(NMODS == module_modspecial + 1);
    for (int i = 0; i < NMODS; ++i)
      strtomodtypes.insert(
          modtypemap_val(modtypestostrs[i], (enum module_type) i));
    assert(NSETLOCS == module_locnote + 1);
    for (int i = 0; i < NSETLOCS; ++i)
      strtosetlocs.insert(
          setlocmap_val(setlocstostrs[i], (module_setting_loc) i));
  }

  std::string setloctostrex(const module_setting_loc en) {
    std::ostringstream ou;
    ou << setlocstostrs[en] << " (initfile";
    // bool fi = true;
    for (int i = 1; i < NSETLOCS; ++i) {
      if (localallowed((module_setting_loc) i, en)) {
        // if (fi) fi = false; else ou << ' ';
        ou << ' ' << setlocstostrs[i];
      }
    }
    ou << ')';
    return ou.str();
  }

} // namespace fomus

using namespace fomus;

int module_strtoclef(const char* str) {
  ENTER_API;
  assert(clef_nclefs == ((int) (sizeof(clefstostrs) / sizeof(std::string))));
  strtoclefmap_constit i(strstoclefs.find(str));
  if (i == strstoclefs.end())
    return -1;
  return i->second;
  EXIT_API_0;
}
const char* module_cleftostr(int en) {
  ENTER_API;
  assert(clef_nclefs == ((int) (sizeof(clefstostrs) / sizeof(std::string))));
  if (en < 0 || en >= clef_nclefs) {
    CERR << "invalid clef id ";
    modprinterr();
    return 0;
  }
  return cleftostr(en)
      .c_str(); // assuming this is in range, since this is an enum
  EXIT_API_0;
}
