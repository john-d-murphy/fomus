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

#include "numbers.h"
#include "moremath.h" // mod
#include "vars.h"     // deletemodvals

namespace fomus {

  // highest long number that is divisible by all integers up to 28
#define RAT_CONVERT_NUM 223092870

  // const number NUMBER_MAX(DBL_MAX);

  rat floattorat(const ffloat x) {
    if (x >= 1) {
      ffloat df = RAT_CONVERT_NUM / x;
      fint d = (fint) df;
      if (d == df) {
        return rat(RAT_CONVERT_NUM, d);
      } else {
        rat r1(RAT_CONVERT_NUM, d);
        rat r2(RAT_CONVERT_NUM, d + 1);
        return r1.denominator() <= r2.denominator() ? r1 : r2;
      }
    } else if (x <= -1) {
      ffloat df = RAT_CONVERT_NUM / x;
      fint d = (fint) df;
      if (d == df) {
        return rat(RAT_CONVERT_NUM, d);
      } else {
        rat r1(RAT_CONVERT_NUM, d);
        rat r2(RAT_CONVERT_NUM, d - 1);
        return r1.denominator() <= r2.denominator() ? r1 : r2;
      }
    } else if (x >= 0) {
      ffloat df = RAT_CONVERT_NUM * x;
      fint d = (fint) df;
      if (d == df) {
        return rat(RAT_CONVERT_NUM, d);
      } else {
        rat r1(d, RAT_CONVERT_NUM);
        rat r2(d + 1, RAT_CONVERT_NUM);
        return r1.denominator() <= r2.denominator() ? r1 : r2;
      }
    } else {
      ffloat df = RAT_CONVERT_NUM * x;
      fint d = (fint) df;
      if (d == df) {
        return rat(RAT_CONVERT_NUM, d);
      } else {
        rat r1(d, RAT_CONVERT_NUM);
        rat r2(d - 1, RAT_CONVERT_NUM);
        return r1.denominator() <= r2.denominator() ? r1 : r2;
      }
    }
  }

#ifdef BUILD_LIBFOMUS
  void freevalue(module_value& val) {
    if (val.type >= module_list) {
      struct module_list& arr(val.val.l);
      if (arr.vals) {
        std::for_each(arr.vals, arr.vals + arr.n, freevalue);
        deletemodvals(arr.vals);
        arr.vals = 0;
      }
    }
  }
#endif

} // namespace fomus
