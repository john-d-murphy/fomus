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

#ifndef FOMUSMOD_IFACEDIVSEARCH_H
#define FOMUSMOD_IFACEDIVSEARCH_H

#include "modtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------------
// header file for special measure-divider search engine--works with and/or
// nodes from divrules module probably will only be 1 engine of this type
// --------------------------------------------------------------------------------

// notes:
// nodes shouldn't change after they're constructed and returned to the engine!

// program must return this value to fomus to uniquely identify the interface
// (not the engine)
#define ENGINE_INTERFACEID 3

typedef void* divsearch_node;
struct divsearch_andnode {
  int n;
  divsearch_node* nodes; // module/engine (sender) manages memory (more
                         // efficient if it isn't allocated/freed constantly)
};
struct divsearch_ornode {
  int n;
  struct divsearch_andnode*
      ands; // module/engine (sender) manages memory (more efficient if it isn't
            // allocated/freed constantly)
};
typedef void* divsearch_ornode_ptr;
typedef void* divsearch_andnode_ptr;

union divsearch_score {
  fomus_int i;
  fomus_float f; // rationals are too inefficient--don't use
  void* ptr;     // can store a pointer to location in data structure itself for
                 // more complex score
};

// engine automatically keeps track of and frees these structures (as long as 1
// ornode is created returned to engine when expand or root is called!)
// divsearch_ornode_ptr divsearch_newornode(const struct interface* iface);
typedef divsearch_andnode_ptr (*divsearch_new_andnode_fun)(
    divsearch_ornode_ptr ornode);
typedef void (*divsearch_push_back_fun)(divsearch_andnode_ptr andnode,
                                        divsearch_node node);
typedef struct divsearch_ornode (*divsearch_get_ornode_fun)(
    divsearch_ornode_ptr ornode); // get structure that program can read
typedef struct divsearch_andnode (*divsearch_get_andnode_fun)(
    divsearch_andnode_ptr andnode);

typedef union divsearch_score (*divsearch_score_fun)(void* moddata,
                                                     divsearch_node node);
typedef void (*divsearch_expand_fun)(
    void* moddata, divsearch_ornode_ptr ornode,
    divsearch_node node); // must expand node and operate on ornode
typedef divsearch_node (*divsearch_assemble_fun)(
    void* moddata, struct divsearch_andnode andnode);
typedef int (*divsearch_score_lt_fun)(void* moddata, union divsearch_score x,
                                      union divsearch_score y);
// typedef void (*divsearch_free_moddata_fun)(void* moddata);
typedef void (*divsearch_free_node_fun)(void* moddata, divsearch_node node);
typedef divsearch_node (*divsearch_get_root_fun)(void* moddata);
typedef int (*divsearch_is_leaf_fun)(void* moddata, divsearch_node node);
typedef void (*divsearch_solution_fun)(void* moddata, divsearch_node node);
typedef const char* (*divsearch_err_fun)(void* moddata);
// typedef int (*divsearch_done_fun)(void* moddata)
// #ifndef NDEBUG
//   typedef void (*divsearch_test)(divsearch_node node);
// #endif

struct divsearch_api {
  divsearch_new_andnode_fun new_andnode;
  divsearch_push_back_fun push_back;
  divsearch_get_ornode_fun get_ornode;
  divsearch_get_andnode_fun get_andnode;
};

// FOMUS asks engine for instance of this struct, then passes it to the module.
// module must fill it with info.  module consists only of callback functions &
// must report progress through the report_progress function, all
// modules/engines must be capable of running multiple instances in separate
// threads
struct divsearch_iface {
  // API
  struct divsearch_api api;

  // MODULE
  void* moddata; // module specific data storage

  union divsearch_score min_score;

  divsearch_score_fun score;
  divsearch_expand_fun expand;
  divsearch_assemble_fun assemble;
  divsearch_score_lt_fun score_lt;
  // divsearch_free_moddata_fun free_moddata; // free divsearch_node object
  divsearch_free_node_fun free_node;
  divsearch_get_root_fun get_root;
  divsearch_is_leaf_fun is_leaf;
  divsearch_solution_fun solution;
  divsearch_err_fun err;
  // divsearch_done_fun done;
  // #ifndef NDEBUG
  //     divsearch_test test;
  // #endif
};

#ifdef __cplusplus
}
#endif

#endif
