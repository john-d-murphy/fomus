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
#include <cassert>
#include <iterator>
#include <queue>

#include "ifacedumb.h"
#include "module.h"

#include "debugaux.h"

namespace rstaves {

  void blastem(std::deque<module_noteobj>& notes, const int cs, const int cl) {
    while (!notes.empty()) {
      module_noteobj n = notes.front();
      if (module_isrest(n)) {
        DBG("out: (t=" << module_time(n) << ") st=" << cs << " cl=" << cl
                       << std::endl);
        rstaves_assign(n, cs, cl);
      } else {
        module_skipassign(n);
      }
      notes.pop_front();
    }
  }

  void rstaves_run_fun(FOMUS fom, void* moddata) {
    std::deque<module_noteobj> notes;
    std::deque<fomus_rat> divs;
    int cs = 0, cs1 = 1, cl = -1; // "current staff" and clef
    module_measobj m = 0;
    module_ratslist dvs;
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n)
        break;
      notes.push_back(n);
      module_measobj m0 = module_meas(n);
      while (m != m0) {
        m = module_nextmeas();
        if (!m)
          break;
        dvs = module_divs(m);
        fomus_rat o(module_time(n));
        for (const fomus_rat *r = dvs.rats, *re = dvs.rats + dvs.n; r < re;
             ++r) {
          divs.push_back(o);
          o = o + *r;
        }
      }
      fomus_rat o(module_time(n));
      bool mb = false;
      while (!divs.empty() && divs.front() <= o) {
        mb = true;
        divs.pop_front();
      }
      if (!module_isrest(n)) {
        int c = module_staff(n);
        int l = module_clef(n);
        blastem(notes, cs ? cs : c, cs ? cl : l);
        cs = cs1 = c;
        cl = l;
      } else if (mb && (module_tuplet(n, 0) == (fomus_int) 0 ||
                        module_tupletbegin(n, 0))) {
        if (cs) {
          blastem(notes, cs, cl);
          cs = 0;
        }
      }
    }
    blastem(
        notes, cs1,
        cl < 0 ? module_strtoclef(module_staffclef(module_peeknextpart(0), cs1))
               : cl);
  }

  const char* rstaves_err_fun(void* moddata) {
    return 0;
  }

  void fill_iface(void* moddata, void* iface) {
    ((dumb_iface*) iface)->moddata = 0;
    ((dumb_iface*) iface)->run = rstaves_run_fun;
    ((dumb_iface*) iface)->err = rstaves_err_fun;
  };

} // namespace rstaves
