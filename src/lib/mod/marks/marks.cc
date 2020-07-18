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

#include "config.h"

#include <algorithm>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <cassert>
#include <set>
#include <vector>

#include "ifacedumb.h"
#include "module.h"

#ifndef NDEBUG
#define NONCOPYABLE , boost::noncopyable
#define _NONCOPYABLE :boost::noncopyable
#else
#define NONCOPYABLE
#define _NONCOPYABLE
#endif

namespace marks {

  extern "C" {
  void marks_run_fun(FOMUS f, void* moddata); // return true when done
  const char* marks_err_fun(void* moddata);
  }

  typedef boost::ptr_map<int, std::set<int>> vsettype;

  // if only one voice in staff, return def
  enum module_markpos where(const vsettype& vsets, const int s, const int v,
                            const enum module_markpos def) {
    vsettype::const_iterator i(vsets.find(s));
    assert(i != vsets.end());
    if (i->second->size() == 1)
      return def;
    int u = 0; // u is number of voices in staff above v
    for (std::set<int>::const_iterator j(i->second->begin());
         j != i->second->end() && *j < v; ++j)
      ++u;
    return u < (int) i->second->size() - u ? markpos_above : markpos_below;
  }

  void remdups(const std::vector<module_noteobj>& nos) {
    std::set<int> ids;
    for (std::vector<module_noteobj>::const_iterator ii(nos.begin());
         ii != nos.end(); ++ii) {
      struct module_markslist ml(module_marks(*ii));
      for (const module_markobj *m = ml.marks, *me = ml.marks + ml.n; m < me;
           ++m) {
        if (module_markpos(*m) == markpos_above) {
          int i = module_markid(*m);
          std::set<int>::const_iterator a(ids.find(i));
          if (a == ids.end())
            ids.insert(i);
          else
            marks_assign_remove(*ii, i, module_markstring(*m),
                                module_marknum(*m));
        }
      }
    }
    ids.clear();
    for (std::vector<module_noteobj>::const_reverse_iterator ii(nos.rbegin());
         ii != nos.rend(); ++ii) {
      struct module_markslist ml(module_marks(*ii));
      for (const module_markobj *m = ml.marks, *me = ml.marks + ml.n; m < me;
           ++m) {
        if (module_markpos(*m) == markpos_below) {
          int i = module_markid(*m);
          std::set<int>::const_iterator a(ids.find(i));
          if (a == ids.end())
            ids.insert(i);
          else
            marks_assign_remove(*ii, i, module_markstring(*m),
                                module_marknum(*m));
        }
      }
    }
    std::for_each(nos.begin(), nos.end(),
                  boost::lambda::bind(marks_assign_done, boost::lambda::_1));
  }

  void marks_run_fun(FOMUS f, void* moddata) {
    vsettype vsets;
    module_noteobj n = 0;
    while (true) {
      n = module_peeknextnote(n);
      if (!n)
        break;
      vsets[module_staff(n)].insert(module_voice(n) % 1000);
    } // vsets has all staff/voice combos
    n = 0;
    assert(!vsets.empty());
    assert(!vsets.begin()->second->empty());
    while (true) {
      n = module_peeknextnote(n);
      if (!n)
        break;
      struct module_markslist ml(module_marks(n));
      for (const module_markobj *m = ml.marks, *me = ml.marks + ml.n; m < me;
           ++m) {
        switch (module_markpos(*m)) {
        case markpos_prefabove:
          markpos_assign(*m, where(vsets, module_staff(n),
                                   module_voice(n) % 1000, markpos_above));
          break;
        case markpos_prefbelow:
          markpos_assign(*m, where(vsets, module_staff(n),
                                   module_voice(n) % 1000, markpos_below));
          break;
        case markpos_prefmiddleorabove: {
          enum module_markpos p =
              where(vsets, module_staff(n), module_voice(n) % 1000,
                    markpos_prefmiddleorabove);
          if (p != markpos_prefmiddleorabove)
            markpos_assign(
                *m,
                p); // not default, so more than one voice, do what "where" says
          else
            markpos_assign(
                *m, (vsets.size() > 1 && vsets.begin()->first == module_staff(n)
                         ? markpos_below
                         : markpos_above));
          break;
        }
        case markpos_prefmiddleorbelow: {
          enum module_markpos p =
              where(vsets, module_staff(n), module_voice(n) % 1000,
                    markpos_prefmiddleorbelow);
          if (p != markpos_prefmiddleorbelow)
            markpos_assign(
                *m,
                p); // not default, so more than one voice, do what "where" says
          else
            markpos_assign(
                *m, (vsets.size() > 1 && vsets.begin()->first < module_staff(n)
                         ? markpos_above
                         : markpos_below));
          break;
        }
        default:;
        }
      } // markpos_done(n);
    }
    fomus_rat cu = {-1, 1};
    std::vector<module_noteobj> nos;
    while (true) {
      n = module_nextnote();
      if (!n)
        break;
      fomus_rat o(module_time(n));
      if (o > cu) {
        remdups(nos);
        nos.clear();
        cu = o;
      }
      nos.push_back(n);
    }
    remdups(nos);
  }

  const char* marks_err_fun(void* moddata) {
    return 0;
  }

  void fill_iface(void* moddata, void* iface) {
    ((dumb_iface*) iface)->moddata = 0;
    ((dumb_iface*) iface)->run = marks_run_fun;
    ((dumb_iface*) iface)->err = marks_err_fun;
  };

  // ------------------------------------------------------------------------------------------------------------------------

  int get_setting(int n, module_setting* set, int id) {
    return 0;
  }

} // namespace marks
