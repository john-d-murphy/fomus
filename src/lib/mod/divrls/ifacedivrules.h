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

#ifndef FOMUSMOD_IFACEDIVRULES_H
#define FOMUSMOD_IFACEDIVRULES_H

// --------------------------------------------------------------------------------
// C API for MEASURE DIVISION RULES

// Like an engine module, rules modules are used by other modules
// FOMUS takes care of function calls
// --------------------------------------------------------------------------------

#include "modtypes.h"
//#include "moddivrulesiface.h"

#ifdef __cplusplus
extern "C" {
#endif

// use this to make sure calling the right module functions
#define DIVRULES_INTERFACEID 2

typedef void* divrules_div; // actual rules structure is internal to divrules.cc
struct divrules_andnode {
  int n;
  divrules_div* divs;
};
struct divrules_ornode {
  int n;
  divrules_andnode* ands;
};

struct divrules_range {
  fomus_rat time1, time2;
  int lvl;
};
struct divrules_rangelist {
  int n;
  const divrules_range* ranges;
};

typedef divrules_ornode (*divrules_expand_fun)(void* moddata, divrules_div node,
                                               divrules_rangelist excl);
typedef void (*divrules_free_moddata_fun)(void* moddata);
// typedef void (*divrules_free_ornodeonly_fun)(void* moddata, divrules_ornode*
// ornode);
typedef divrules_ornode (*divrules_get_root_fun)(
    void* moddata, struct fomus_rat time,
    struct module_list initdiv); // pass it an initial div list
typedef void (*divrules_free_node_fun)(void* moddata, divrules_div node);
typedef struct fomus_rat (*divrules_time_fun)(void* moddata, divrules_div node);
typedef struct fomus_rat (*divrules_endtime_fun)(void* moddata,
                                                 divrules_div node);
typedef struct fomus_rat (*divrules_dur_fun)(void* moddata, divrules_div node);

typedef int (*divrules_tieleftallowed_fun)(void* moddata, divrules_div node);
typedef int (*divrules_tierightallowed_fun)(void* moddata, divrules_div node);
// typedef int (*divrules_istupletlevel_fun)(void* moddata, divrules_div node);
typedef int (*divrules_issmall_fun)(void* moddata, divrules_div node);
typedef int (*divrules_ntupletlevels_fun)(void* moddata, divrules_div node);
typedef fomus_rat (*divrules_leveldur_fun)(void* moddata, divrules_div node,
                                           int lvl);
typedef int (*divrules_istupletbegin_fun)(void* moddata, divrules_div node,
                                          int lvl);
typedef int (*divrules_istupletend_fun)(void* moddata, divrules_div node,
                                        int lvl);
typedef int (*divrules_iscompound)(void* moddata);
typedef fomus_rat (*divrules_durmult)(void* moddata, divrules_div node);
typedef int (*divrules_isnoteonly)(void* moddata, divrules_div node);
typedef fomus_rat (*divrules_tuplet)(void* moddata, divrules_div node, int lvl);

typedef const struct module_list (*divrules_getinitdivs_fun)(void* moddata);
//   typedef module_list (*divrules_gettupdivs_fun)();

struct divrules_data { // must all be the right types, or fomus will barf
  module_measobj meas; // measure object to get settings from
  // int nbeats_setid, comp_setid; these two divrules should look up
  int dotnotelvl_setid, dbldotnotelvl_setid, slsnotelvl_setid,
      syncnotelvl_setid; // set to <0 for none
  // int mintupdur_setid, maxtupdur_setid; //, /*maxtups_setid,*/
  // tuprattype_setid; //, tupdivs_setid; these divrules should look up
};

// modules that want to divide measures get entry point with this structure
struct divrules_iface {
  void* moddata;

  // settings
  divrules_expand_fun expand; // the function that does the work
  divrules_get_root_fun get_root;
  divrules_free_moddata_fun free_moddata; // free mod_data
  // divrules_free_ornodeonly_fun free_ornodeonly; // freeds or-node and nested
  // and-nodes, but not nodes
  divrules_free_node_fun free_node; // frees a node
  divrules_time_fun time;
  divrules_endtime_fun endtime;
  divrules_dur_fun dur;

  divrules_tieleftallowed_fun tieleftallowed;
  divrules_tierightallowed_fun tierightallowed;
  divrules_issmall_fun issmall;
  // divrules_istupletlevel_fun istupletlevel;
  divrules_ntupletlevels_fun ntupletlevels;
  divrules_leveldur_fun leveldur;
  divrules_istupletbegin_fun istupletbegin;
  divrules_istupletend_fun istupletend;
  divrules_durmult durmult;
  divrules_isnoteonly isnoteonly;

  divrules_getinitdivs_fun get_initdivs; // all of them
  divrules_iscompound iscompound;
  divrules_tuplet tuplet; // get tuplet at some level
                          //     divrules_gettupdivs_fun get_tupdivs;

  struct divrules_data data; // module fills this and sends to divrules.cc
};

#ifdef __cplusplus
}
#endif

#endif
