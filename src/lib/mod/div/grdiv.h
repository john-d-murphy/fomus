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

#ifndef FOMUSMOD_GRDIV_H
#define FOMUSMOD_GRDIV_H

namespace grdiv {

  inline const char* name() {return "grdiv";};
  inline const char* prettyname() {return "Grace Note Divider";}
  inline const char* author() {return "(built-in)";}
  inline const char* doc() {return "Splits and ties grace notes if necessary.";}
  inline void* newdata(FOMUS f) {return 0;}
  inline void freedata(void* dat) {}
  inline int priority() {return 0;}
  inline enum module_type modtype() {return module_moddivide;}
  inline const char* initerr() {return 0;}
  inline void init() {}
  inline void free() {}
  inline int get_setting(int n, module_setting* set, int id) {return 0;}
  inline int itertype() {return module_bymeas | module_byvoice | module_graceonly;}
  void fill_iface(void* moddata, void* iface);
  inline const char* engine() {return "dumb";}
  inline bool sameinst() {return true;}
}

#endif
