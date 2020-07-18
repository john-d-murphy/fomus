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

#ifndef FOMUS_NUMBERS_H
#define FOMUS_NUMBERS_H

#ifdef BUILD_LIBFOMUS
#include "heads.h"
#endif

#include "basetypes.h" // fomus_float, etc..
#include "error.h"     // class err
#include "modtypes.h"  // module_value struct
#include "module.h"    // module_value arithmetic
#include "modutil.h"   // module_value arithmetic

namespace FNAMESPACE {

  // class numberr:public errbase {};

#ifdef BUILD_LIBFOMUS
  inline void thrownumb() {
    CERR << "invalid value type (internal error)"
         << std::endl; // INTERNAL ERROR
  }
#else
  inline void thrownumb() {}
#endif

  // important typedefs
  typedef fomus_int fint;     // fomus int
  typedef fomus_float ffloat; // fomus float
  typedef fomus_bool fbool;

  // rational definition
  typedef boost::rational<fint> rat; // also change this in modules.h

#ifdef BUILD_LIBFOMUS
  inline void initvalue(module_value& val) {
    val.type = module_int;
    val.val.i = 0;
  }
  void freevalue(module_value& val);
#endif

  inline struct fomus_rat makerat(fomus_int n, fomus_int d) {
    struct fomus_rat r = {n, d};
    return r;
  }

  // number definition
  struct numb : public module_value {
    // struct module_value v;
    numb() {}
    numb(const enum module_value_type t) {
      module_value::type = module_none;
    }
    numb(const struct module_value& v) : module_value(v) {
      assert(type() == v.type);
    }
    numb(const fint n) {
      module_value::type = module_int;
      module_value::val.i = n;
    }
    numb(const rat& n) {
      if (n.denominator() <= 1) {
        module_value::type = module_int;
        module_value::val.i = n.numerator();
      } else {
        module_value::type = module_rat;
        module_value::val.r.num = n.numerator();
        module_value::val.r.den = n.denominator();
      }
    } // only allow w/ rat class (so it is always reduced)
    numb(const fomus_rat& n) {
      rat n0(n.num, n.den);
      if (n0.denominator() <= 1) {
        module_value::type = module_int;
        module_value::val.i = n0.numerator();
      } else {
        module_value::type = module_rat;
        module_value::val.r.num = n0.numerator();
        module_value::val.r.den = n0.denominator();
      }
    }
    numb(const ffloat n) {
      module_value::type = module_float;
      module_value::val.f = n;
    }
    numb(const numb& x) : module_value(x) {
      assert(type() == x.type());
    }
#ifdef BUILD_LIBFOMUS
    void operator=(const module_value_type t) {
      freevalue(*this);
      module_value::type = module_none;
    }
#endif
    void operator=(const fint n) {
      module_value::type = module_int;
      module_value::val.i = n;
    }
    void operator=(const rat& n) {
      if (n.denominator() <= 1) {
        module_value::type = module_int;
        module_value::val.i = n.numerator();
      } else {
        module_value::type = module_rat;
        module_value::val.r.num = n.numerator();
        module_value::val.r.den = n.denominator();
      }
    }
    void operator=(const fomus_rat& n) {
      if (n.den <= 1) {
        module_value::type = module_int;
        module_value::val.i = n.num;
      } else {
        module_value::type = module_rat;
        module_value::val.r = n;
      }
    }
    void operator=(const ffloat n) {
      module_value::type = module_float;
      module_value::val.f = n;
    }
    void operator=(const numb& n) {
      (module_value&) * this = (module_value&) n;
    }
    module_value_type type() const {
      return module_value::type;
    }
    void settype(const module_value_type ty) {
      module_value::type = ty;
    }
    void null() {
      module_value::type = module_none;
    }
    bool isnull() const {
      return module_value::type == module_none;
    }
    bool isntnull() const {
      return module_value::type != module_none;
    }
    fint geti() const {
      return module_value::val.i;
    } // NO CONVERSIONS
    ffloat getf() const {
      return module_value::val.f;
    }
    rat getr() const {
      return rat(module_value::val.r.num, module_value::val.r.den);
    }
    fint getnum() const {
      return module_value::val.r.num;
    }
    fint getden() const {
      return module_value::val.r.den;
    }
    const module_value& modval() const {
      return *this;
    }
    module_value& modval() {
      return *this;
    }
    bool getbool() const { // these do conversions!
      return module_value::val.i;
      //       switch (module_value::type) {
      //       case module_int: return module_value::val.i;
      //       case module_float: return module_value::val.f;
      //       case module_rat: return module_value::val.r.num;
      //       default:
      // 	assert(false);
      //       }
    }
    bool israt() const {
      return module_value::type == module_int ||
             module_value::type == module_rat;
    }
    bool isint() const {
      return module_value::type == module_int ||
             (module_value::type == module_rat && val.r.den == 1);
    }
    bool islist() const {
      return module_value::type == module_list;
    }
  };
  //   struct numbless {
  //     inline bool operator()(const numb& x, const numb& y) {return x.modval()
  //     < y.modval();}
  //   };

  // #ifdef BUILD_LIBFOMUS
  //   extern const numb fomnumb_min;
  // #endif
  inline fomus_rat tofrat(const rat& r) {
    fomus_rat x = {r.numerator(), r.denominator()};
    return x;
  }

  // rational utility funs
  inline ffloat rattofloat(const rat& x) {
    return boost::rational_cast<ffloat>(x);
  }
  rat floattorat(const ffloat x);
  inline fint rattoint(const rat& x) {
    return x.numerator() / x.denominator();
  }
  inline fint rattoint(const fint n, const fint d) {
    return n / d;
  }
  inline rat inttorat(const fint x) {
    return x;
  }
  inline fomus_rat rattofrat(const rat& x) {
    return makerat(x.numerator(), x.denominator());
  }

  // number conversions
  inline ffloat numtofloat(const numb& x) {
    switch (x.type()) {
    case module_int:
      return x.geti();
    case module_float:
      return x.getf();
    case module_rat:
      return rattofloat(x.getr());
    default:;
    }
    thrownumb();
    throw errbase(); // throw numberr();
  }
  inline rat numtorat(const numb& x) {
    switch (x.type()) {
    case module_int:
      return x.geti();
    case module_float:
      return floattorat(x.getf());
    case module_rat:
      return x.getr();
    default:;
    }
    thrownumb();
    throw errbase(); // throw numberr();
  }
  inline fomus_rat numtofrat(const numb& x) {
    switch (x.type()) {
    case module_int:
      return makerat(x.geti(), 1);
    case module_float:
      return rattofrat(floattorat(x.getf()));
    case module_rat:
      return rattofrat(x.getr());
    default:;
    }
    thrownumb();
    throw errbase(); // throw numberr();
  }
  inline fint numtoint(const numb& x) {
    switch (x.type()) {
    case module_int:
      return x.geti();
    case module_float:
      return lround(x.getf());
    case module_rat:
      return rattoint(x.getr());
    default:;
    }
    thrownumb();
    throw errbase(); // throw numberr();
  }

#ifdef BUILD_LIBFOMUS
  inline std::ostream& operator<<(std::ostream& os, const rat& x) {
    if (x.denominator() <= 1)
      os << x.numerator();
    else {
      fint v = x.numerator() / x.denominator();
      if (v > -1 && x < 1)
        boost::operator<<(os, x);
      else {
        os << v;
        rat r(x - v);
        if (r.numerator() >= 0)
          os << '+';
        boost::operator<<(os, r);
      }
    }
    return os;
  }
  inline std::ostream& operator<<(std::ostream& os, const numb& x) {
    switch (x.type()) {
    case module_none:
      os << "(null)";
      break;
    case module_int:
      os << x.geti();
      break;
    case module_float: {
      {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(5) << std::showpoint << x.val.f;
        std::string s(
            boost::trim_right_copy_if(ss.str(), boost::lambda::_1 == '0'));
        os << s;
        if (!s.empty() && *boost::prior(s.end()) == '.')
          os << '0';
      }
      break;
    }
    case module_rat:
      os << x.getr();
      break;
    default:
      DBG(x.type() << std::endl);
      thrownumb();
      throw errbase(); // throw numberr();
    }
    return os;
  }
#endif

#ifdef BUILD_LIBFOMUS

  inline fomus_rat operator-(const struct fomus_rat& x) {
    return module_ratneg(x);
  }

  inline bool operator==(const struct fomus_rat& a, const struct fomus_rat& b) {
    return rat(a.num, a.den) == rat(b.num, b.den);
  }
  inline bool operator!=(const struct fomus_rat& a, const struct fomus_rat& b) {
    return rat(a.num, a.den) != rat(b.num, b.den);
  }
  inline bool operator<(const struct fomus_rat& a, const struct fomus_rat& b) {
    return rat(a.num, a.den) < rat(b.num, b.den);
  }
  inline bool operator<=(const struct fomus_rat& a, const struct fomus_rat& b) {
    return rat(a.num, a.den) <= rat(b.num, b.den);
  }
  inline bool operator>(const struct fomus_rat& a, const struct fomus_rat& b) {
    return rat(a.num, a.den) > rat(b.num, b.den);
  }
  inline bool operator>=(const struct fomus_rat& a, const struct fomus_rat& b) {
    return rat(a.num, a.den) >= rat(b.num, b.den);
  }
  inline struct fomus_rat operator+(const struct fomus_rat& a,
                                    const struct fomus_rat& b) {
    return rattofrat(rat(a.num, a.den) + rat(b.num, b.den));
  }
  inline struct fomus_rat operator-(const struct fomus_rat& a,
                                    const struct fomus_rat& b) {
    return rattofrat(rat(a.num, a.den) - rat(b.num, b.den));
  }
  inline struct fomus_rat operator*(const struct fomus_rat& a,
                                    const struct fomus_rat& b) {
    return rattofrat(rat(a.num, a.den) * rat(b.num, b.den));
  }
  inline struct fomus_rat operator/(const struct fomus_rat& a,
                                    const struct fomus_rat& b) {
    return rattofrat(rat(a.num, a.den) / rat(b.num, b.den));
  }

  inline bool operator==(const rat& a, const struct fomus_rat& b) {
    return a == rat(b.num, b.den);
  }
  inline bool operator!=(const rat& a, const struct fomus_rat& b) {
    return a != rat(b.num, b.den);
  }
  inline bool operator<(const rat& a, const struct fomus_rat& b) {
    return a < rat(b.num, b.den);
  }
  inline bool operator<=(const rat& a, const struct fomus_rat& b) {
    return a <= rat(b.num, b.den);
  }
  inline bool operator>(const rat& a, const struct fomus_rat& b) {
    return a > rat(b.num, b.den);
  }
  inline bool operator>=(const rat& a, const struct fomus_rat& b) {
    return a >= rat(b.num, b.den);
  }
  inline rat operator+(const rat& a, const struct fomus_rat& b) {
    return a + rat(b.num, b.den);
  }
  inline rat operator-(const rat& a, const struct fomus_rat& b) {
    return a - rat(b.num, b.den);
  }
  inline rat operator*(const rat& a, const struct fomus_rat& b) {
    return a * rat(b.num, b.den);
  }
  inline rat operator/(const rat& a, const struct fomus_rat& b) {
    return a / rat(b.num, b.den);
  }

  inline bool operator==(const struct fomus_rat& a, const rat& b) {
    return rat(a.num, a.den) == b;
  }
  inline bool operator!=(const struct fomus_rat& a, const rat& b) {
    return rat(a.num, a.den) != b;
  }
  inline bool operator<(const struct fomus_rat& a, const rat& b) {
    return rat(a.num, a.den) < b;
  }
  inline bool operator<=(const struct fomus_rat& a, const rat& b) {
    return rat(a.num, a.den) <= b;
  }
  inline bool operator>(const struct fomus_rat& a, const rat& b) {
    return rat(a.num, a.den) > b;
  }
  inline bool operator>=(const struct fomus_rat& a, const rat& b) {
    return rat(a.num, a.den) >= b;
  }
  inline rat operator+(const struct fomus_rat& a, const rat& b) {
    return rat(a.num, a.den) + b;
  }
  inline rat operator-(const struct fomus_rat& a, const rat& b) {
    return rat(a.num, a.den) - b;
  }
  inline rat operator*(const struct fomus_rat& a, const rat& b) {
    return rat(a.num, a.den) * b;
  }
  inline rat operator/(const struct fomus_rat& a, const rat& b) {
    return rat(a.num, a.den) / b;
  }

  inline bool operator==(const struct fomus_rat& a, const fomus_int b) {
    return rat(a.num, a.den) == b;
  }
  inline bool operator!=(const struct fomus_rat& a, const fomus_int b) {
    return rat(a.num, a.den) != b;
  }
  inline bool operator<(const struct fomus_rat& a, const fomus_int b) {
    return rat(a.num, a.den) < b;
  }
  inline bool operator<=(const struct fomus_rat& a, const fomus_int b) {
    return rat(a.num, a.den) <= b;
  }
  inline bool operator>(const struct fomus_rat& a, const fomus_int b) {
    return rat(a.num, a.den) > b;
  }
  inline bool operator>=(const struct fomus_rat& a, const fomus_int b) {
    return rat(a.num, a.den) >= b;
  }
  inline struct fomus_rat operator+(const struct fomus_rat& a,
                                    const fomus_int b) {
    return rattofrat(rat(a.num, a.den) + b);
  }
  inline struct fomus_rat operator-(const struct fomus_rat& a,
                                    const fomus_int b) {
    return rattofrat(rat(a.num, a.den) - b);
  }
  inline struct fomus_rat operator*(const struct fomus_rat& a,
                                    const fomus_int b) {
    return rattofrat(rat(a.num, a.den) * b);
  }
  inline struct fomus_rat operator/(const struct fomus_rat& a,
                                    const fomus_int b) {
    return rattofrat(rat(a.num, a.den) / b);
  }

  inline bool operator==(const fomus_int a, const struct fomus_rat& b) {
    return a == rat(b.num, b.den);
  }
  inline bool operator!=(const fomus_int a, const struct fomus_rat& b) {
    return a != rat(b.num, b.den);
  }
  inline bool operator<(const fomus_int a, const struct fomus_rat& b) {
    return a < rat(b.num, b.den);
  }
  inline bool operator<=(const fomus_int a, const struct fomus_rat& b) {
    return a <= rat(b.num, b.den);
  }
  inline bool operator>(const fomus_int a, const struct fomus_rat& b) {
    return a > rat(b.num, b.den);
  }
  inline bool operator>=(const fomus_int a, const struct fomus_rat& b) {
    return a >= rat(b.num, b.den);
  }
  inline struct fomus_rat operator+(const fomus_int a,
                                    const struct fomus_rat& b) {
    return rattofrat(a + rat(b.num, b.den));
  }
  inline struct fomus_rat operator-(const fomus_int a,
                                    const struct fomus_rat& b) {
    return rattofrat(a - rat(b.num, b.den));
  }
  inline struct fomus_rat operator*(const fomus_int a,
                                    const struct fomus_rat& b) {
    return rattofrat(a * rat(b.num, b.den));
  }
  inline struct fomus_rat operator/(const fomus_int a,
                                    const struct fomus_rat& b) {
    return rattofrat(a / rat(b.num, b.den));
  }

  inline bool operator==(const struct fomus_rat& a, const fomus_float b) {
    return (a.num / (fomus_float) a.den) == b;
  }
  inline bool operator!=(const struct fomus_rat& a, const fomus_float b) {
    return (a.num / (fomus_float) a.den) != b;
  }
  inline bool operator<(const struct fomus_rat& a, const fomus_float b) {
    return (a.num / (fomus_float) a.den) < b;
  }
  inline bool operator<=(const struct fomus_rat& a, const fomus_float b) {
    return (a.num / (fomus_float) a.den) <= b;
  }
  inline bool operator>(const struct fomus_rat& a, const fomus_float b) {
    return (a.num / (fomus_float) a.den) > b;
  }
  inline bool operator>=(const struct fomus_rat& a, const fomus_float b) {
    return (a.num / (fomus_float) a.den) >= b;
  }
  inline fomus_float operator+(const struct fomus_rat& a, const fomus_float b) {
    return (a.num / (fomus_float) a.den) + b;
  }
  inline fomus_float operator-(const struct fomus_rat& a, const fomus_float b) {
    return (a.num / (fomus_float) a.den) - b;
  }
  inline fomus_float operator*(const struct fomus_rat& a, const fomus_float b) {
    return (a.num / (fomus_float) a.den) * b;
  }
  inline fomus_float operator/(const struct fomus_rat& a, const fomus_float b) {
    return (a.num / (fomus_float) a.den) / b;
  }

  inline bool operator==(const fomus_float a, const struct fomus_rat& b) {
    return a == (b.num / (fomus_float) b.den);
  }
  inline bool operator!=(const fomus_float a, const struct fomus_rat& b) {
    return a != (b.num / (fomus_float) b.den);
  }
  inline bool operator<(const fomus_float a, const struct fomus_rat& b) {
    return a < (b.num / (fomus_float) b.den);
  }
  inline bool operator<=(const fomus_float a, const struct fomus_rat& b) {
    return a <= (b.num / (fomus_float) b.den);
  }
  inline bool operator>(const fomus_float a, const struct fomus_rat& b) {
    return a > (b.num / (fomus_float) b.den);
  }
  inline bool operator>=(const fomus_float a, const struct fomus_rat& b) {
    return a >= (b.num / (fomus_float) b.den);
  }
  inline fomus_float operator+(const fomus_float a, const struct fomus_rat& b) {
    return a + (b.num / (fomus_float) b.den);
  }
  inline fomus_float operator-(const fomus_float a, const struct fomus_rat& b) {
    return a - (b.num / (fomus_float) b.den);
  }
  inline fomus_float operator*(const fomus_float a, const struct fomus_rat& b) {
    return a * (b.num / (fomus_float) b.den);
  }
  inline fomus_float operator/(const fomus_float a, const struct fomus_rat& b) {
    return a / (b.num / (fomus_float) b.den);
  }

  inline const module_value& fomus_make_val(const module_value& r) {
    return r;
  }
  inline module_value fomus_make_val(const fomus_int x) {
    module_value r;
    r.type = module_int;
    r.val.i = x;
    return r;
  }
  inline module_value fomus_make_val(const fomus_rat& x) {
    module_value r;
    r.type = module_rat;
    r.val.r = x;
    return r;
  }
  inline module_value fomus_make_val(const fomus_float x) {
    module_value r;
    r.type = module_float;
    r.val.f = x;
    return r;
  }
  inline module_value fomus_make_nullval() {
    module_value r;
    r.type = module_none;
    return r;
  }

  // struct numbx:public numb {
  //   numbx(const numb& x):numb(x) {}
  //   numbx(const numbx& x):numb(x) {}
  // };

  inline numb operator-(const numb& a) {
    switch (a.type()) {
    case module_int:
      return numb(-a.val.i);
    case module_rat:
      return numb(-a.val.r);
    case module_float:
      return numb(-a.val.f);
    default:
      assert(false);
    }
  }

  inline bool operator==(const numb& a, const fomus_int& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i == b;
    case module_rat:
      return a.val.r == b;
    case module_float:
      return a.val.f == b;
    default:
      assert(false);
    }
  }
  inline bool operator==(const numb& a, const fomus_rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i == b;
    case module_rat:
      return a.val.r == b;
    case module_float:
      return a.val.f == b;
    default:
      assert(false);
    }
  }
  inline bool operator==(const numb& a, const rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i == b;
    case module_rat:
      return a.val.r == b;
    case module_float:
      return a.val.f == rattofloat(b);
    default:
      assert(false);
    }
  }
  inline bool operator==(const numb& a, const fomus_float& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i == b;
    case module_rat:
      return a.val.r == b;
    case module_float:
      return a.val.f == b;
    default:
      assert(false);
    }
  }
  inline bool operator!=(const numb& a, const fomus_int& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i != b;
    case module_rat:
      return a.val.r != b;
    case module_float:
      return a.val.f != b;
    default:
      assert(false);
    }
  }
  inline bool operator!=(const numb& a, const fomus_rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i != b;
    case module_rat:
      return a.val.r != b;
    case module_float:
      return a.val.f != b;
    default:
      assert(false);
    }
  }
  inline bool operator!=(const numb& a, const rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i != b;
    case module_rat:
      return a.val.r != b;
    case module_float:
      return a.val.f != rattofloat(b);
    default:
      assert(false);
    }
  }
  inline bool operator!=(const numb& a, const fomus_float& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i != b;
    case module_rat:
      return a.val.r != b;
    case module_float:
      return a.val.f != b;
    default:
      assert(false);
    }
  }
  inline bool operator<(const numb& a, const fomus_int& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i < b;
    case module_rat:
      return a.val.r < b;
    case module_float:
      return a.val.f < b;
    default:
      assert(false);
    }
  }
  inline bool operator<(const numb& a, const fomus_rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i < b;
    case module_rat:
      return a.val.r < b;
    case module_float:
      return a.val.f < b;
    default:
      assert(false);
    }
  }
  inline bool operator<(const numb& a, const rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i < b;
    case module_rat:
      return a.val.r < b;
    case module_float:
      return a.val.f < rattofloat(b);
    default:
      assert(false);
    }
  }
  inline bool operator<(const numb& a, const fomus_float& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i < b;
    case module_rat:
      return a.val.r < b;
    case module_float:
      return a.val.f < b;
    default:
      assert(false);
    }
  }
  inline bool operator<=(const numb& a, const fomus_int& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i <= b;
    case module_rat:
      return a.val.r <= b;
    case module_float:
      return a.val.f <= b;
    default:
      assert(false);
    }
  }
  inline bool operator<=(const numb& a, const fomus_rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i <= b;
    case module_rat:
      return a.val.r <= b;
    case module_float:
      return a.val.f <= b;
    default:
      assert(false);
    }
  }
  inline bool operator<=(const numb& a, const rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i <= b;
    case module_rat:
      return a.val.r <= b;
    case module_float:
      return a.val.f <= rattofloat(b);
    default:
      assert(false);
    }
  }
  inline bool operator<=(const numb& a, const fomus_float& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i <= b;
    case module_rat:
      return a.val.r <= b;
    case module_float:
      return a.val.f <= b;
    default:
      assert(false);
    }
  }
  inline bool operator>(const numb& a, const fomus_int& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i > b;
    case module_rat:
      return a.val.r > b;
    case module_float:
      return a.val.f > b;
    default:
      assert(false);
    }
  }
  inline bool operator>(const numb& a, const fomus_rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i > b;
    case module_rat:
      return a.val.r > b;
    case module_float:
      return a.val.f > b;
    default:
      assert(false);
    }
  }
  inline bool operator>(const numb& a, const rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i > b;
    case module_rat:
      return a.val.r > b;
    case module_float:
      return a.val.f > rattofloat(b);
    default:
      assert(false);
    }
  }
  inline bool operator>(const numb& a, const fomus_float& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i > b;
    case module_rat:
      return a.val.r > b;
    case module_float:
      return a.val.f > b;
    default:
      assert(false);
    }
  }
  inline bool operator>=(const numb& a, const fomus_int& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i >= b;
    case module_rat:
      return a.val.r >= b;
    case module_float:
      return a.val.f >= b;
    default:
      assert(false);
    }
  }
  inline bool operator>=(const numb& a, const fomus_rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i >= b;
    case module_rat:
      return a.val.r >= b;
    case module_float:
      return a.val.f >= b;
    default:
      assert(false);
    }
  }
  inline bool operator>=(const numb& a, const rat& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i >= b;
    case module_rat:
      return a.val.r >= b;
    case module_float:
      return a.val.f >= rattofloat(b);
    default:
      assert(false);
    }
  }
  inline bool operator>=(const numb& a, const fomus_float& b) {
    switch (a.type()) {
    case module_int:
      return a.val.i >= b;
    case module_rat:
      return a.val.r >= b;
    case module_float:
      return a.val.f >= b;
    default:
      assert(false);
    }
  }
  inline numb operator+(const numb& a, const fomus_int& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i + b);
    case module_rat:
      return numb(a.val.r + b);
    case module_float:
      return numb(a.val.f + b);
    default:
      assert(false);
    }
  }
  inline numb operator+(const numb& a, const fomus_rat& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i + b);
    case module_rat:
      return numb(a.val.r + b);
    case module_float:
      return numb(a.val.f + b);
    default:
      assert(false);
    }
  }
  inline numb operator+(const numb& a, const rat& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i + b);
    case module_rat:
      return numb(a.val.r + b);
    case module_float:
      return numb(a.val.f + rattofloat(b));
    default:
      assert(false);
    }
  }
  inline numb operator+(const numb& a, const fomus_float& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i + b);
    case module_rat:
      return numb(a.val.r + b);
    case module_float:
      return numb(a.val.f + b);
    default:
      assert(false);
    }
  }
  inline numb operator-(const numb& a, const fomus_int& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i - b);
    case module_rat:
      return numb(a.val.r - b);
    case module_float:
      return numb(a.val.f - b);
    default:
      assert(false);
    }
  }
  inline numb operator-(const numb& a, const fomus_rat& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i - b);
    case module_rat:
      return numb(a.val.r - b);
    case module_float:
      return numb(a.val.f - b);
    default:
      assert(false);
    }
  }
  inline numb operator-(const numb& a, const rat& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i - b);
    case module_rat:
      return numb(a.val.r - b);
    case module_float:
      return numb(a.val.f - rattofloat(b));
    default:
      assert(false);
    }
  }
  inline numb operator-(const numb& a, const fomus_float& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i - b);
    case module_rat:
      return numb(a.val.r - b);
    case module_float:
      return numb(a.val.f - b);
    default:
      assert(false);
    }
  }
  inline numb operator*(const numb& a, const fomus_int& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i * b);
    case module_rat:
      return numb(a.val.r * b);
    case module_float:
      return numb(a.val.f * b);
    default:
      assert(false);
    }
  }
  inline numb operator*(const numb& a, const fomus_rat& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i * b);
    case module_rat:
      return numb(a.val.r * b);
    case module_float:
      return numb(a.val.f * b);
    default:
      assert(false);
    }
  }
  inline numb operator*(const numb& a, const rat& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i * b);
    case module_rat:
      return numb(a.val.r * b);
    case module_float:
      return numb(a.val.f * rattofloat(b));
    default:
      assert(false);
    }
  }
  inline numb operator*(const numb& a, const fomus_float& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i * b);
    case module_rat:
      return numb(a.val.r * b);
    case module_float:
      return numb(a.val.f * b);
    default:
      assert(false);
    }
  }
  inline numb operator/(const numb& a, const fomus_int& b) {
    switch (a.type()) {
    case module_int:
      return (a.val.i % b) ? numb(makerat(a.val.i, b)) : numb(a.val.i / b);
    case module_rat:
      return numb(a.val.r / b);
    case module_float:
      return numb(a.val.f / b);
    default:
      assert(false);
    }
  }
  inline numb operator/(const numb& a, const fomus_rat& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i / b);
    case module_rat:
      return numb(a.val.r / b);
    case module_float:
      return numb(a.val.f / b);
    default:
      assert(false);
    }
  }
  inline numb operator/(const numb& a, const rat& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i / b);
    case module_rat:
      return numb(a.val.r / b);
    case module_float:
      return numb(a.val.f / rattofloat(b));
    default:
      assert(false);
    }
  }
  inline numb operator/(const numb& a, const fomus_float& b) {
    switch (a.type()) {
    case module_int:
      return numb(a.val.i / b);
    case module_rat:
      return numb(a.val.r / b);
    case module_float:
      return numb(a.val.f / b);
    default:
      assert(false);
    }
  }

  inline bool operator==(const fomus_int& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a == b.val.i;
    case module_rat:
      return a == b.val.r;
    case module_float:
      return a == b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator==(const fomus_rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a == b.val.i;
    case module_rat:
      return a == b.val.r;
    case module_float:
      return a == b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator==(const rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a == b.val.i;
    case module_rat:
      return a == b.val.r;
    case module_float:
      return rattofloat(a) == b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator==(const fomus_float& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a == b.val.i;
    case module_rat:
      return a == b.val.r;
    case module_float:
      return a == b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator!=(const fomus_int& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a != b.val.i;
    case module_rat:
      return a != b.val.r;
    case module_float:
      return a != b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator!=(const fomus_rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a != b.val.i;
    case module_rat:
      return a != b.val.r;
    case module_float:
      return a != b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator!=(const rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a != b.val.i;
    case module_rat:
      return a != b.val.r;
    case module_float:
      return rattofloat(a) != b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator!=(const fomus_float& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a != b.val.i;
    case module_rat:
      return a != b.val.r;
    case module_float:
      return a != b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator<(const fomus_int& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a < b.val.i;
    case module_rat:
      return a < b.val.r;
    case module_float:
      return a < b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator<(const fomus_rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a < b.val.i;
    case module_rat:
      return a < b.val.r;
    case module_float:
      return a < b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator<(const rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a < b.val.i;
    case module_rat:
      return a < b.val.r;
    case module_float:
      return rattofloat(a) < b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator<(const fomus_float& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a < b.val.i;
    case module_rat:
      return a < b.val.r;
    case module_float:
      return a < b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator<=(const fomus_int& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a <= b.val.i;
    case module_rat:
      return a <= b.val.r;
    case module_float:
      return a <= b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator<=(const fomus_rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a <= b.val.i;
    case module_rat:
      return a <= b.val.r;
    case module_float:
      return a <= b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator<=(const rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a <= b.val.i;
    case module_rat:
      return a <= b.val.r;
    case module_float:
      return rattofloat(a) <= b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator<=(const fomus_float& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a <= b.val.i;
    case module_rat:
      return a <= b.val.r;
    case module_float:
      return a <= b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator>(const fomus_int& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a > b.val.i;
    case module_rat:
      return a > b.val.r;
    case module_float:
      return a > b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator>(const fomus_rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a > b.val.i;
    case module_rat:
      return a > b.val.r;
    case module_float:
      return a > b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator>(const rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a > b.val.i;
    case module_rat:
      return a > b.val.r;
    case module_float:
      return rattofloat(a) > b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator>(const fomus_float& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a > b.val.i;
    case module_rat:
      return a > b.val.r;
    case module_float:
      return a > b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator>=(const fomus_int& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a >= b.val.i;
    case module_rat:
      return a >= b.val.r;
    case module_float:
      return a >= b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator>=(const fomus_rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a >= b.val.i;
    case module_rat:
      return a >= b.val.r;
    case module_float:
      return a >= b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator>=(const rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a >= b.val.i;
    case module_rat:
      return a >= b.val.r;
    case module_float:
      return rattofloat(a) >= b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator>=(const fomus_float& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a >= b.val.i;
    case module_rat:
      return a >= b.val.r;
    case module_float:
      return a >= b.val.f;
    default:
      assert(false);
    }
  }
  inline numb operator+(const fomus_int& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a + b.val.i);
    case module_rat:
      return numb(a + b.val.r);
    case module_float:
      return numb(a + b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator+(const fomus_rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a + b.val.i);
    case module_rat:
      return numb(a + b.val.r);
    case module_float:
      return numb(a + b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator+(const rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a + b.val.i);
    case module_rat:
      return numb(a + b.val.r);
    case module_float:
      return numb(rattofloat(a) + b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator+(const fomus_float& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a + b.val.i);
    case module_rat:
      return numb(a + b.val.r);
    case module_float:
      return numb(a + b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator-(const fomus_int& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a - b.val.i);
    case module_rat:
      return numb(a - b.val.r);
    case module_float:
      return numb(a - b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator-(const fomus_rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a - b.val.i);
    case module_rat:
      return numb(a - b.val.r);
    case module_float:
      return numb(a - b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator-(const rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a - b.val.i);
    case module_rat:
      return numb(a - b.val.r);
    case module_float:
      return numb(rattofloat(a) - b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator-(const fomus_float& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a - b.val.i);
    case module_rat:
      return numb(a - b.val.r);
    case module_float:
      return numb(a - b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator*(const fomus_int& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a * b.val.i);
    case module_rat:
      return numb(a * b.val.r);
    case module_float:
      return numb(a * b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator*(const fomus_rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a * b.val.i);
    case module_rat:
      return numb(a * b.val.r);
    case module_float:
      return numb(a * b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator*(const rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a * b.val.i);
    case module_rat:
      return numb(a * b.val.r);
    case module_float:
      return numb(rattofloat(a) * b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator*(const fomus_float& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a * b.val.i);
    case module_rat:
      return numb(a * b.val.r);
    case module_float:
      return numb(a * b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator/(const fomus_int& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return (a % b.val.i) ? numb(makerat(a, b.val.i)) : numb(a / b.val.i);
    case module_rat:
      return numb(a / b.val.r);
    case module_float:
      return numb(a / b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator/(const fomus_rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a / b.val.i);
    case module_rat:
      return numb(a / b.val.r);
    case module_float:
      return numb(a / b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator/(const rat& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a / b.val.i);
    case module_rat:
      return numb(a / b.val.r);
    case module_float:
      return numb(rattofloat(a) / b.val.f);
    default:
      assert(false);
    }
  }
  inline numb operator/(const fomus_float& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return numb(a / b.val.i);
    case module_rat:
      return numb(a / b.val.r);
    case module_float:
      return numb(a / b.val.f);
    default:
      assert(false);
    }
  }

  inline bool operator==(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a == b.val.i;
    case module_rat:
      return a == b.val.r;
    case module_float:
      return a == b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator!=(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a != b.val.i;
    case module_rat:
      return a != b.val.r;
    case module_float:
      return a != b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator<(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a < b.val.i;
    case module_rat:
      return a < b.val.r;
    case module_float:
      return a < b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator<=(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a <= b.val.i;
    case module_rat:
      return a <= b.val.r;
    case module_float:
      return a <= b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator>(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a > b.val.i;
    case module_rat:
      return a > b.val.r;
    case module_float:
      return a > b.val.f;
    default:
      assert(false);
    }
  }
  inline bool operator>=(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a >= b.val.i;
    case module_rat:
      return a >= b.val.r;
    case module_float:
      return a >= b.val.f;
    default:
      assert(false);
    }
  }
  inline numb operator+(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a + b.val.i;
    case module_rat:
      return a + b.val.r;
    case module_float:
      return a + b.val.f;
    default:
      assert(false);
    }
  }
  inline numb operator-(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a - b.val.i;
    case module_rat:
      return a - b.val.r;
    case module_float:
      return a - b.val.f;
    default:
      assert(false);
    }
  }
  inline numb operator*(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a * b.val.i;
    case module_rat:
      return a * b.val.r;
    case module_float:
      return a * b.val.f;
    default:
      assert(false);
    }
  }
  inline numb operator/(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return a / b.val.i;
    case module_rat:
      return a / b.val.r;
    case module_float:
      return a / b.val.f;
    default:
      assert(false);
    }
  }

  inline fomus_int div2(fomus_int x) {
    return div2_int(x);
  }
  inline fomus_rat div2(fomus_rat x) {
    if (x.den == 1)
      x.num = div2_int(x.num);
    return x;
  }

  inline fomus_int maxdiv2(fomus_int x) {
    return maxdiv2_int(x);
  }
  inline fomus_rat maxdiv2(fomus_rat x) {
    return maxdiv2_rat(x);
  }

  inline modutil_bool isexpof2(fomus_int x) {
    return div2_int(x) == 1;
  }
  inline modutil_bool isexpof2(const fomus_rat& x) {
    return x.den == 1 ? isexpof2(x.num)
                      : (abs_int(x.num) == 1 ? isexpof2(x.den) : false);
  }

  inline fomus_int diff(fomus_int x, fomus_int y) {
    return abs_int(x - y);
  }
  inline fomus_rat diff(const fomus_rat& x, const fomus_rat& y) {
    return abs_rat(x - y);
  }

  inline fomus_int roundto(const numb& x, fomus_int to) {
    return mfloor(((fomus_int) 2 * x + to) / ((fomus_int) 2 * to)) * to;
  }
  inline fomus_rat roundto(const numb& x, const fomus_rat& to) {
    return mfloor(((fomus_int) 2 * x + to) / ((fomus_int) 2 * to)) * to;
  }

  inline fomus_int floorto(const numb& x, fomus_int to) {
    return mfloor(x / to) * to;
  }
  inline fomus_rat floorto(const numb& x, const fomus_rat& to) {
    return mfloor(x / to) * to;
  }

  inline bool iswhite(fomus_int x) {
    return whitenotes[x % 12];
  }
  inline bool iswhite(const fomus_rat& x) {
    return x.den == 1 && whitenotes[x.num % 12];
  }
  inline bool isblack(fomus_int x) {
    return blacknotes[x % 12];
  }
  inline bool isblack(const fomus_rat& x) {
    return x.den == 1 && blacknotes[x.num % 12];
  }

  inline fomus_int mod(fomus_int x, fomus_int y) {
    return mod_int(x, y);
  }
  inline fomus_float mod(fomus_float x, fomus_float y) {
    return mod_float(x, y);
  }
  inline fomus_float mod(fomus_float x, fomus_int y) {
    return mod_float(x, y);
  }
  inline fomus_float mod(fomus_int x, fomus_float y) {
    return mod_float(x, y);
  }
  inline struct fomus_rat mod(const struct fomus_rat& a,
                              const struct fomus_rat& b) {
    return mod_rat(a, b);
  }
  inline struct fomus_rat mod(const struct fomus_rat& a, fomus_int b) {
    return mod_rat(a, module_inttorat(b));
  }
  inline struct fomus_rat mod(fomus_int a, const struct fomus_rat& b) {
    return mod_rat(module_inttorat(a), b);
  }
  inline fomus_float mod(const struct fomus_rat& a, fomus_float b) {
    return mod_float(module_rattofloat(a), b);
  }
  inline fomus_float mod(fomus_float a, const struct fomus_rat& b) {
    return mod_float(a, module_rattofloat(b));
  }
  template <typename T>
  inline numb mod(const numb& a, const T& b) {
    switch (a.type()) {
    case module_int:
      return numb(mod(a.val.i, b));
    case module_rat:
      return numb(mod(a.val.r, b));
    case module_float:
      return numb(mod(a.val.f, b));
    default:
      assert(false);
    }
  }
  template <typename T>
  inline numb mod(const T& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return mod(a, numb(b.val.i));
    case module_rat:
      return mod(a, numb(b.val.r));
    case module_float:
      return mod(a, numb(b.val.f));
    default:
      assert(false);
    }
  }
  inline numb mod(const numb& a, const numb& b) {
    switch (b.type()) {
    case module_int:
      return mod(a, b.val.i);
    case module_rat:
      return mod(a, b.val.r);
    case module_float:
      return mod(a, b.val.f);
    default:
      assert(false);
    }
  }

#endif

} // namespace FNAMESPACE

#endif
