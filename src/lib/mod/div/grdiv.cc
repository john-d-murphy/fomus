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

#include "config.h"

#include <memory>
#include <set>

#include <boost/ptr_container/ptr_deque.hpp>

#include "ifacedumb.h"
#include "module.h"

#include "debugaux.h"

namespace grdiv {

  extern "C" {
  void grdiv_run_fun(FOMUS f, void* moddata); // return true when done
  const char* grdiv_err_fun(void* moddata);
  }

  struct node {
    module_noteobj n;
    fomus_rat o, eo;
    std::set<fomus_rat> pts;
    node(const module_noteobj n, const fomus_rat& o, const fomus_rat& eo)
        : n(n), o(o), eo(eo) {}
    void inspt(const fomus_rat& t) {
      if (t > o && t < eo)
        pts.insert(t);
    }
    void assign(const module_measobj m);
  };
  inline void doass(boost::ptr_deque<node>& nds, const module_measobj m) {
    while (!nds.empty()) {
      nds.front().assign(m);
      nds.pop_front();
    }
  }
  void node::assign(const module_measobj m) {
    if (!pts.empty()) {
      pts.insert(o);
      for (std::set<fomus_rat>::const_iterator i(pts.begin()); i != pts.end();
           ++i) {
        divide_gracesplit x = {n, *i};
        divide_assign_gracesplit(m, x);
        DBG("assigning grace split at off=" << *i << ", o=" << o
                                            << ", eo=" << eo << std::endl);
      }
    }
    module_skipassign(n);
  }

  void grdiv_run_fun(FOMUS fom, void* moddata) {
    module_measobj m = module_nextmeas();
    boost::ptr_deque<node> nds;
    fomus_rat lt = {0, 1};
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n)
        break;
      fomus_rat t(module_time(n));
      if (t > lt) {
        doass(nds, m);
        assert(nds.empty());
        lt = t;
      }
      fomus_rat o(module_gracetime(n));
      fomus_rat eo(module_graceendtime(n));
      std::auto_ptr<node> nno(new node(n, o, eo));
      for (boost::ptr_deque<node>::iterator i(nds.begin()); i != nds.end();
           ++i) {
        i->inspt(o);
        i->inspt(eo);
        nno->inspt(i->o);
        nno->inspt(i->eo);
      }
      while (!nds.empty() && nds.front().eo <= o) {
        nds.front().assign(m);
        nds.pop_front();
      }
      nds.push_back(nno);
    }
    doass(nds, m);
  }

  const char* grdiv_err_fun(void* moddata) {
    return 0;
  }

  void fill_iface(void* moddata, void* iface) {
    ((dumb_iface*) iface)->moddata = 0;
    ((dumb_iface*) iface)->run = grdiv_run_fun;
    ((dumb_iface*) iface)->err = grdiv_err_fun;
  };

} // namespace grdiv
