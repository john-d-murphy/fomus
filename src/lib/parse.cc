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

#include "parse.h"
#include "modutil.h" // mod
#include "error.h" // CERR

namespace fomus {

  boostspirit::symbols<fint> boolsyms;
  void initboolsyms() {
    boolsyms.add
      ("yes", 1) ("y", 1) ("true", 1) ("t", 1) ("1", 1) ("on", 1)
      ("no", 0) ("n", 0) ("false", 0) ("f", 0) ("0", 0) ("off", 0);
  }

  void nearestpitch::operator()(const parse_it &s1, const parse_it &s2) const {
    //assert(val >= (fint)0 && val < (fint)12);
    val = val + roundto_int(prval.modval() - val, (fint)12);
    numb d(val - prval.modval());
    if (gup) {
      if (d == (fint)-6) val = val + (fint)12;
    } else {
      if (d == (fint)6) val = val - (fint)12;
    }
    gup = (val > prval.modval());
    prval = val;
  }

  fomsymbols<numb> note_parse;
  fomsymbols<numb> acc_parse;
  fomsymbols<numb> mic_parse;
  fomsymbols<numb> oct_parse;
  //fomsymbols<int> ksig_parse;

  fomsymbols<numb> durdot_parse;
  fomsymbols<numb> dursyms_parse;
  fomsymbols<numb> durtie_parse;
  fomsymbols<numb> tupsyms_parse;
  
  bool boolfalse = false;
  
  void filepos::printerr0(std::ostream& ou) const {
    if (line >= 0) { 
      ou << " in " << linestr << ' ' << line;
      if (col >= 0) ou << ", " << colstr << ' ' << col;
      if (!file.empty()) ou << " of `" << file << '\'';
    } else if (!file.empty()) ou << " in `" << file << '\'';
    //ou << std::endl;
  }
}
