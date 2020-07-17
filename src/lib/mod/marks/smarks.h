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

#ifndef FOMUSMOD_SMARKS_H
#define FOMUSMOD_SMARKS_H

namespace smarks {
  
  inline const char* name() {return "smarks";};
  inline const char* prettyname() {return "Staff Marks";}
  inline const char* author() {return "(built-in)";}
  inline const char* doc() {return "Sorts and matches spanner begin/end marks in staves.";}
  inline void* newdata(FOMUS f) {return 0;}
  inline void freedata(void* dat) {}
  inline const char* err(void* dat) {return 0;}
  inline int priority() {return 0;}
  inline enum module_type modtype() {return module_modsmarks;}
  inline const char* initerr() {return 0;}
  inline void init() {}
  inline void free() {}

  int get_setting(int n, module_setting* set, int id);

  inline int itertype() {return module_bypart | module_bystaff /*| module_norests*/;}
//   inline int engine_iface() {return ENGINE_INTERFACEID;}
//   inline const char* engine() {return "dumb";}
  void fill_iface(void* moddata, void* iface);
  inline bool sameinst() {return true;}
  
}

#endif
