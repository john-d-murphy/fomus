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

#ifndef FOMUS_MOREMATH_H
#define FOMUS_MOREMATH_H

#ifdef BUILD_LIBFOMUS
#include "heads.h"
#endif

#include "modutil.h"
#include "numbers.h"

namespace fomus {

  //   inline ffloat mod(const fint x, const ffloat y) {return mod((ffloat)x,
  //   y);} inline ffloat mod(const ffloat x, const fint y) {return mod(x,
  //   (ffloat)y);}

  // rational
  inline rat ratmod(const rat& x, const rat& y) {
    fint m = boost::integer::lcm(x.denominator(), y.denominator());
    return rat(mod(x.numerator() * (m / x.denominator()),
                   y.numerator() * (m / y.denominator())),
               m);
  }
  inline ffloat ratmod(const rat& x, const ffloat y) {
    return mod(rattofloat(x), y);
  }
  inline rat ratmod(const rat& x, const fint y) {
    return ratmod(x, rat(y, 1));
  }

  // special...
  //   template<typename T>
  //   inline T mod12(const T& x) {return mod(x, (fint)12);}

  //   inline rat torational(const fomus_rat& x) {return rat(x.num, x.den);}
  // inline fomus_rat tofomusrat(const rat& x) {fomus_rat r = {x.numerator(),
  // x.denominator()}; return r;}

  // ------------------------------------------------------------------------------------------------------------------------
  // operations on integers and rats

  inline fint floor_wbase(const fint x, const fint y) {
    return x - mod(x, y);
  }
  inline fint floor_wbase(const boost::rational<fint>& x, const fint y) {
    assert((x - ratmod(x, y)).denominator() == 1);
    return (x - ratmod(x, y)).numerator();
  }

  template <typename T>
  inline fint floor_base12(const T& x) {
    return floor_wbase(x, (fint) 12);
  }
  template <typename T>
  inline fint floor_base1(const T& x) {
    return floor_wbase(x, (fint) 1);
  }

  inline fint round_upwbase(const fint x, const fint y) {
    fint yy = mod(x, y);
    return yy < boost::rational<fint>(y, 2) ? x - yy : y + x - yy;
  }
  inline fint round_downwbase(const fint x, const fint y) {
    fint yy = mod(x, y);
    return yy <= boost::rational<fint>(y, 2) ? x - yy : y + x - yy;
  }
  inline fint round_upwbase(const boost::rational<fint>& x, const fint y) {
    boost::rational<fint> yy(ratmod(x, y));
    assert((x - yy).denominator() == 1);
    return yy < boost::rational<fint>(y, 2) ? (x - yy).numerator()
                                            : y + (x - yy).numerator();
  }
  inline fint round_downwbase(const boost::rational<fint>& x, const fint y) {
    boost::rational<fint> yy(ratmod(x, y));
    assert((x - yy).denominator() == 1);
    return yy <= boost::rational<fint>(y, 2) ? (x - yy).numerator()
                                             : y + (x - yy).numerator();
  }

  template <typename T>
  inline fint round_upbase12(const T& x) {
    return round_upwbase(x, (fint) 12);
  }
  template <typename T>
  inline fint round_upbase1(const T& x) {
    return round_upwbase(x, (fint) 1);
  }
  template <typename T>
  inline fint round_downbase12(const T& x) {
    return round_downwbase(x, (fint) 12);
  }
  template <typename T>
  inline fint round_downbase1(const T& x) {
    return round_downwbase(x, (fint) 1);
  }

} // namespace fomus
#endif
