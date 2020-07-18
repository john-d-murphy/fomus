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

#ifndef MODULE_MARKSAUX_H
#define MODULE_MARKSAUX_H

#include "config.h"

#include "debugaux.h"
#include "module.h"

namespace marksaux {

  template <class I1, class I2, class L, class C1, class C2>
  inline void
  inplace_set_difference(C1& cont1,
                         C2& cont2) { // doesn't modify the first one
    I1 first1(cont1.begin());
    I2 first2(cont2.begin());
    while (first1 != cont1.end() && first2 != cont2.end()) {
      if (L()(*first1, *first2))
        ++first1;
      else if (L()(*first2, *first1))
        ++first2;
      else {
        cont1.erase(first1++);
        cont2.erase(first2++);
      }
    }
  }

  struct mark {
    int ty;
    std::string str;
    module_value val;
    mark(const module_markobj m)
        : ty(module_markid(m)), val(module_marknum(m)) {
      const char* s = module_markstring(m);
      if (s)
        str = s;
    }
    mark(const int m, const std::string& str)
        : ty(m), str(str), val(module_makenullval()) {}
    mark(const int m) : ty(m), val(module_makenullval()) {}
    mark(const int m, const module_value& val) : ty(m), val(val) {}
    mark(const int m, const fomus_rat& r) : ty(m), val(module_makeval(r)) {}
    mark(const int m, const fomus_int r) : ty(m), val(module_makeval(r)) {}
    mark(const int m, const std::string& str, const module_value& val)
        : ty(m), str(str), val(val) {}
    mark(const int m, const std::string& str, const fomus_rat& r)
        : ty(m), str(str), val(module_makeval(r)) {}
    mark(const int m, const std::string& str, const fomus_int r)
        : ty(m), str(str), val(module_makeval(r)) {}
    void assass(const module_noteobj n) const {
      DBG("assigning mark " << ty << ',' << (!str.empty() ? str : "") << ','
                            << val << " at " << module_time(n) << std::endl);
      marks_assign_add(n, ty, str.c_str(), val);
    }
    void assrem(const module_noteobj n) const {
      DBG("removing mark " << ty << ',' << (!str.empty() ? str : "") << ','
                           << val << " at " << module_time(n) << std::endl);
      marks_assign_remove(n, ty, str.c_str(), val);
    }
  };
  struct markless : std::binary_function<const mark&, const mark&, bool> {
    inline bool operator()(const mark& x, const mark& y) const {
      if (x.ty != y.ty)
        return x.ty < y.ty;
      if (x.val.type != module_none && y.val.type != module_none &&
          x.val != y.val)
        return x.val < y.val;
      if ((x.val.type == module_none) != (y.val.type == module_none))
        return y.val.type == module_none;
      if (!(x.str.empty() || y.str.empty()))
        return x.str < y.str; // not case insensitive here
      if (x.str.empty() != y.str.empty())
        return y.str.empty();
      return false;
    }
  };

} // namespace marksaux

#endif
