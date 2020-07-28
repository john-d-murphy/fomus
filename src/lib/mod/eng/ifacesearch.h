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

#ifndef FOMUSMOD_IFACESEARCH_H
#define FOMUSMOD_IFACESEARCH_H

#include "modtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------------
// at present, header file for dynamic programming and best first search engines
// modules can use them interchangeably
// --------------------------------------------------------------------------------

// program must return this value to fomus to uniquely identify the interface
// (not the engine)
#define ENGINE_INTERFACEID 2

typedef void* search_node;
struct search_nodes {
  int n;
  search_node* nodes;
};

union search_score {
  fomus_int i;
  fomus_float f; // rationals are too inefficient--don't use
  void* ptr;     // can store a pointer to location in data structure itself for
                 // more complex score
};

typedef void (*search_assign_fun)(
    void* moddata, int choice); // makes a solution assignment & reports it so
                                // that other phases of the program can continue
typedef union search_score (*search_get_score_fun)(void* moddata,
                                                   struct search_nodes nodes);
typedef search_node (*search_new_node_fun)(
    void* moddata, search_node prevnode,
    int choice); // return NULL if no valid next node or END if at end of
                 // search--`prevnode' might be BEGIN (get the first node)
typedef void (*search_free_node_fun)(void* moddata, search_node node);
// typedef void (*search_free_moddata_fun)(void* moddata); // free up moddata
// structure and choices vector when finished
typedef const char* (*search_err_fun)(void* moddata);
typedef int (*search_score_lt_fun)(void* moddata, union search_score x,
                                   union search_score y);
typedef union search_score (*search_score_add_fun)(void* moddata,
                                                   union search_score x,
                                                   union search_score y);
typedef int (*search_is_outofrange_fun)(void* moddata, search_node node1,
                                        search_node node2);

struct search_api {
  search_node begin;
  search_node end;
};

// FOMUS asks engine for instance of this struct, then passes it to the module.
// module must fill it with info.  module consists only of callback functions &
// must report progress through the report_progress function, all
// modules/engines must be capable of running multiple instances in separate
// threads
struct search_iface {
  // API
  struct search_api api; // module should store this

  // MODULE
  void* moddata; // module specific data storage--should probably return the
                 // same pointer as module_newdata, but can be different

  int nchoices;
  union search_score min_score;
  fomus_int heapsize;

  search_assign_fun assign;
  search_get_score_fun get_score; // scores will accumlate as search progresses
  search_new_node_fun new_node;
  search_free_node_fun free_node;
  // search_free_moddata_fun free_moddata;
  search_err_fun err; // should probably be the same as module's module_err
                      // function, but can be different
  search_score_lt_fun score_lt;
  search_score_add_fun score_add;
  search_is_outofrange_fun is_outofrange; // range is the MAXIMUM possible
};

#ifdef __cplusplus
}
#endif

#endif
