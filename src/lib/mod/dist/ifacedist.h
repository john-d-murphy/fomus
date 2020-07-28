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

#ifndef FOMUSMOD_IFACEDIST_H
#define FOMUSMOD_IFACEDIST_H

#include "modtypes.h"
//#include "moddistiface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DIST_INTERFACEID 1

//   struct dist_notes {
//     fomus_int n;
//     module_noteobj* notes;
//   };

typedef fomus_float (*dist_dist_fun)(void* moddata, module_noteobj note1,
                                     module_noteobj note2);
typedef void (*dist_free_moddata_fun)(void* moddata);
typedef int (*dist_is_outofrange_fun)(void* moddata, module_noteobj note1,
                                      module_noteobj note2);

// using module provides:
struct dist_data {
  int octdist_setid, beatdist_setid; // for cart dist--setting ids so dist.cc
                                     // can look them up by note!
  int byendtime;                     // boolean value
  fomus_float rangemax; // the maximum range, for is_inmaxrange function
};

struct dist_iface {
  void* moddata;

  // api:
  dist_dist_fun dist;
  dist_is_outofrange_fun is_outofrange;
  dist_free_moddata_fun free_moddata; // free mod_data

  struct dist_data data;
};

#ifdef __cplusplus
}
#endif

#endif
