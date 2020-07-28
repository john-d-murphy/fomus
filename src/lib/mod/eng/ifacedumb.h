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

#ifndef FOMUSMOD_IFACEDUMB_H
#define FOMUSMOD_IFACEDUMB_H

#include "basetypes.h"
#include "modtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENGINE_INTERFACEID 1

typedef void* dumb_data;

typedef void (*dumb_run_fun)(FOMUS f, void* moddata); // return true when done
typedef const char* (*dumb_err_fun)(void* moddata);
// typedef void (*dumb_free_moddata_fun)(void* moddata); // free up mod_data
// structure and choices vector when finished

struct dumb_iface {
  void* moddata; // module specific data storage--should probably return the
                 // same pointer as module_newdata, but can be different

  dumb_run_fun run;
  dumb_err_fun err; // should probably be the same as module's module_err
                    // function, but can be different
  // dumb_free_moddata_fun free_moddata;
};

#ifdef __cplusplus
}
#endif

#endif
