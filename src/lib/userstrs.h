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

#ifndef FOMUS_USERSTRS_H
#define FOMUS_USERSTRS_H

#ifndef BUILD_LIBFOMUS
#error "userstrs.h shouldn't be included"
#endif

#include "heads.h"
#include "error.h"
#include "algext.h"
#include "modtypes.h"
#include "fomusapi.h"

namespace fomus {

  class badstr:public errbase {}; // ***** SAVE THIS

  typedef std::map<const std::string, int, isiless> strtoclefmap;
  typedef strtoclefmap::value_type strtoclefmap_val;
  typedef strtoclefmap::const_iterator strtoclefmap_constit;
  extern strtoclefmap strstoclefs;
  extern const std::string clefstostrs[];
  extern const int clefmidpitches[];

  inline int strtoclef(const std::string& str) {
    strtoclefmap_constit i(strstoclefs.find(str));
    if (i == strstoclefs.end()) throw badstr(); // need badstr()
    return i->second;
  };
  inline bool isvalidclef(const std::string& str) {return strstoclefs.find(str) != strstoclefs.end();}
  inline const std::string& cleftostr(const int en) {return clefstostrs[en];}
  inline int clefmidpitch(const int en) {return clefmidpitches[en];}

  extern std::string clefstypestr;

  typedef std::map<const std::string, enum module_type, isiless> modtypemap;
  typedef modtypemap::const_iterator modtypemap_constit;
  typedef modtypemap::iterator modtypemap_it;
  typedef modtypemap::value_type modtypemap_val;

  extern modtypemap strtomodtypes;
  extern const std::string modtypestostrs[];

  inline bool isvalidmodtype(const std::string& str) {return strtomodtypes.find(str) != strtomodtypes.end();}
  inline const std::string& modtypetostr(const enum module_type en) {return modtypestostrs[en];}
  inline enum module_type strtomodtype(const std::string& str) {
    modtypemap_constit i(strtomodtypes.find(str));
    if (i == strtomodtypes.end()) throw badstr();
    return i->second;
  }

  typedef std::map<const std::string, module_setting_loc, isiless> setlocmap;
  typedef setlocmap::const_iterator setlocmap_constit;
  typedef setlocmap::iterator setlocmap_it;
  typedef setlocmap::value_type setlocmap_val;

  extern setlocmap strtosetlocs;
  extern const std::string setlocstostrs[];

  inline bool isvalidsetloc(const std::string& str) {return strtosetlocs.find(str) != strtosetlocs.end();}
  inline const std::string& setloctostr(const module_setting_loc en) {return setlocstostrs[en];}
  inline module_setting_loc strtosetloc(const std::string& str) {
    setlocmap_constit i(strtosetlocs.find(str));
    if (i == strtosetlocs.end()) throw badstr();
    return i->second;
  }
  std::string setloctostrex(const module_setting_loc en);

}

#endif
