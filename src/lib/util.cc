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

#include "heads.h"

#include "algext.h"
#include "classes.h"
#include "error.h"
#include "modtypes.h"
#include "module.h"
#include "modutil.h"
#include "numbers.h"
#include "schedr.h"
#include "vars.h"

using namespace fomus;

#define CASTMARKOBJ(xxx) ((markobj*) (modobjbase*) (xxx))
#define CASTMEASURE(xxx) ((measure*) (modobjbase*) (xxx))
#define CASTNOTEEVBASE(xxx) ((noteevbase*) (modobjbase*) (xxx))

struct modutil_lowmults lowmults(fomus_int x) {
  ENTER_API;
  std::vector<fomus_int> ret;
  for (fomus_int m = 2; m <= x; ++m) {
    if (!(x % m)) {
      x /= m;
      ret.push_back(m);
      while (!(x % m))
        x /= m;
    }
  }
  modutil_lowmults rr;
  rr.n = ret.size();
  rr.vals = &ret[0];
  return rr;
  EXIT_MODUTIL_LOWMULTS0;
}

fomus_int maxdiv2_int(fomus_int x) {
  ENTER_API;
  fomus_int r = 1;
  fomus_int r0 = 2;
  while (true) {
    if (r0 > x)
      return r;
    r = r0;
    r0 *= 2;
  }
  EXIT_API_0;
}
fomus_rat maxdiv2_rat(fomus_rat x) {
  ENTER_API;
  if (x.num >= x.den) { // >= 1
    fomus_rat r = {maxdiv2(x.num / x.den), 1};
    return r;
  }
  fomus_int x0 = x.den / x.num; // x0 > 1
  fomus_int r = 2;
  while (r < x0)
    r *= 2;
  fomus_rat rr = {1, r};
  return rr;
  EXIT_API_RAT0;
}

fomus_int div2_int(fomus_int x) {
  ENTER_API;
  if (x == 0)
    return 0;
  while (!(x & 1))
    x /= 2;
  return x;
  EXIT_API_0;
}

// the MOD function!
fomus_int mod_int(fomus_int x, fomus_int y) {
  ENTER_API;
  assert(y > 0);
  if (x >= 0)
    return x % y;
  else {
    fomus_int r = (-x) % y;
    return r > 0 ? y - r : 0;
  }
  EXIT_API_0;
}

// ffloats
fomus_float mod_float(
    fomus_float x,
    fomus_float y) { // fomus doesn't use any float type other than fomus_float
  ENTER_API;
  assert(y > 0);
  if (x >= 0)
    return std::fmod(x, y) * y;
  else {
    fomus_float r = std::fmod(-x, y) * y;
    return r > (fomus_float) 0 ? y - r : (fomus_float) 0;
  }
  EXIT_API_0;
}

fomus_rat mod_rat(fomus_rat x, fomus_rat y) {
  ENTER_API;
  fomus_int m = boost::integer::lcm(x.den, y.den);
  return fomus::tofrat(
      fomus::rat(mod(x.num * (m / x.den), y.num * (m / y.den)), m));
  EXIT_API_RAT0;
}

namespace fomus {
  inline modutil_bool isexpof2_fomrat(const fomus::rat& x) {
    return x.denominator() == 1
               ? isexpof2_int(x.numerator())
               : (abs_int(x.numerator()) == 1 ? isexpof2_int(x.denominator())
                                              : false);
  }
} // namespace fomus

struct modutil_rhythm rhythm(fomus_rat dur) {
  ENTER_API;
  struct modutil_rhythm rh;
  rh.dots = 0;
  fomus::rat rn(1, 2), r(1, 1);
  fomus::rat du(dur.num, dur.den);
  while (true) {
    fomus::rat d0(du / r);
    if (fomus::isexpof2_fomrat(du / r)) {
      rh.dur.num = d0.numerator();
      rh.dur.den = d0.denominator();
      return rh;
    }
    if (rh.dots >= 3) {
      rh.dur.num = 0;
      rh.dur.den = 0;
      rh.dots = 0;
      return rh;
    }
    r = r + rn;
    rn = rn / 2;
    ++rh.dots;
  }
  EXIT_MODUTIL_RHYTHM0;
}

modutil_rangesobj ranges_init() {
  ENTER_API;
  return new fomus::ranges;
  EXIT_API_0;
}
void ranges_insert(modutil_rangesobj ranges0, modutil_range range0) {
  ENTER_API;
  ((fomus::ranges*) ranges0)->insert_range(range0);
  EXIT_API_VOID;
}
void ranges_remove(modutil_rangesobj ranges0, modutil_range range0) {
  ENTER_API;
  ((fomus::ranges*) ranges0)->remove_range(range0);
  EXIT_API_VOID;
}
void ranges_free(modutil_rangesobj ranges0) {
  ENTER_API;
  delete (fomus::ranges*) ranges0;
  EXIT_API_VOID;
}
modutil_ranges ranges_get(modutil_rangesobj ranges0) {
  ENTER_API;
  return ((fomus::ranges*) ranges0)->getranges();
  EXIT_MODUTIL_RANGES0;
}
fomus_int ranges_size(modutil_rangesobj rangeobj) {
  ENTER_API;
  return ((fomus::ranges*) rangeobj)->size();
  EXIT_API_0;
}

fomus_int roundto_int(struct module_value x, fomus_int to) {
  ENTER_API;
  return mfloor(((fomus_int) 2 * x + to) / ((fomus_int) 2 * to)) * to;
  EXIT_API_0;
}
struct fomus_rat roundto_rat(struct module_value x, struct fomus_rat to) {
  ENTER_API;
  return mfloor(((fomus_int) 2 * x + to) / ((fomus_int) 2 * to)) * to;
  EXIT_API_RAT0;
}

namespace fomus {
  std::string toromanstr(const int n) {
    const static char* arr[][10] = {
        {"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"},
        {"", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC"},
        {"", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM"},
        {"", "M", "MM", "MMM"}};
    std::ostringstream ss;
    ss << n;
    const std::string& s(ss.str());
    std::ostringstream ou;
    int sz1 = s.size() - 1;
    for (int i = std::max(0, sz1 - 3); i <= sz1; ++i) {
      const char* x = arr[sz1 - i][s[i] - '0'];
      if (x == 0)
        break;
      ou << x;
    }
    return ou.str();
  }
} // namespace fomus

const char* toroman(int n) {
  ENTER_API;
  return fomus::make_charptr(toromanstr(n));
  EXIT_API_0;
}

fomus_int mfloor(struct module_value x) {
  ENTER_API;
  switch (x.type) {
  case module_int:
    return x.val.i;
  case module_rat:
    return (x.val.r.num < 0 ? (x.val.r.den == 1 ? x.val.r.num
                                                : x.val.r.num / x.val.r.den - 1)
                            : x.val.r.num / x.val.r.den);
  case module_float:
    return (x.val.f < 0
                ? (x.val.f == (fomus_int) x.val.f ? x.val.f
                                                  : (fomus_int) x.val.f - 1)
                : (fomus_int) x.val.f);
  default:
    return module_none;
  }
  EXIT_API_0;
}
fomus_int mfloorto_int(struct module_value x, fomus_int to) {
  ENTER_API;
  return mfloor(x / to) * to;
  EXIT_API_0;
}
fomus_rat mfloorto_rat(struct module_value x, struct fomus_rat to) {
  ENTER_API;
  return mfloor(x / to) * to;
  EXIT_API_RAT0;
}

int modout_keysigequal(struct modout_keysig x, struct modout_keysig y) {
  ENTER_API;
  if (x.n != y.n)
    return false;
  for (const modout_keysig_indiv *i(x.indiv), *j(y.indiv), *ie(x.indiv + x.n);
       i < ie; ++i, ++j) {
    if (i->dianote != j->dianote || i->acc1 != j->acc1 || i->acc2 != j->acc2)
      return false;
  }
  return true;
  EXIT_API_0;
}

struct modout_tempostr modout_tempostr(module_noteobj note,
                                       module_markobj mark) {
  ENTER_API;
  assert(CASTMARKOBJ(mark)->isvalid());
  assert(CASTNOTEEVBASE(note)->isvalid());
  struct modout_tempostr str;
  std::string t(CASTMARKOBJ(mark)->str.empty() ? std::string("* = #")
                                               : CASTMARKOBJ(mark)->str);
  size_t i = t.rfind('#');
  if (i != std::string::npos) {
    if (CASTMARKOBJ(mark)->val.isntnull()) {
      std::ostringstream o;
      o << CASTMARKOBJ(mark)->val;
      t.replace(i, 1, o.str());
    } else {
      t.replace(i, 1, "?");
    }
  }
  i = t.rfind('*');
  if (i != std::string::npos && i >= 0) {
    size_t i0 = (i <= 0 ? std::string::npos : t.rfind('*', i - 1));
    if (i0 != std::string::npos) {
      std::string d(t.substr(i0 + 1, i - i0 - 1));
      numb u(module_none);
      std::ostringstream gar;
      filepos pos(0);
      fint pt1, pt2, pt3;
      parserule rl(numbermatch(u, pt1, pt2, pt3, pos, gar));
      const char* d0 = d.c_str();
      parse_it p(d0, d0 + d.length());
      try {
        parse(p, parse_it(), rl);
      } catch (const boostspirit::parser_error<filepos*, parse_it>& e) {}
      if (u.isnull()) {
        numb x(doparsedur(threadfd.get(), d.c_str(), true));
        if (x.isnull()) {
          str.beat = rattofrat(CASTNOTEEVBASE(note)->get_rval(BEAT_ID));
          if (CASTNOTEEVBASE(note)->get_ival(COMP_ID))
            str.beat = str.beat * makerat(3, 2);
          goto SKIPCHECK;
        } else
          str.beat = numtofrat(x);
      } else
        str.beat = numtofrat(u);
      if ((str.beat.num != 1 && str.beat.num != 3) || !isexpof2(str.beat.den)) {
        t.insert(i0, "?");
        ++i0;
        ++i;
        str.beat.num = 0;
        str.beat.den = 1;
      }
    SKIPCHECK:
      t[i0] = 0;
    } else { // single beat
      str.beat = rattofrat(CASTNOTEEVBASE(note)->get_rval(BEAT_ID));
      if (CASTNOTEEVBASE(note)->get_ival(COMP_ID))
        str.beat = str.beat * makerat(3, 2);
      t[i] = 0;
    } // second string is at i + 1
    str.str1 = make_charptr(t);
    str.str2 = str.str1 + i + 1;
    if (!str.str2[0])
      str.str2 = 0;
  } else {
    str.beat.num = 0;
    str.beat.den = 1;
    str.str1 = make_charptr(t);
    str.str2 = 0;
  }
  if (!str.str1[0])
    str.str1 = 0;
  return str;
  EXIT_API_TEMPOSTR;
}

fomus_int todiatonic(fomus_int x) {
  ENTER_API;
  return ((x / 12) * 7) + diatonicnotes[x % 12];
  EXIT_API_0;
}
fomus_int tochromatic(fomus_int x) {
  ENTER_API;
  return ((x / 7) * 12) + chromaticnotes[x % 7];
  EXIT_API_0;
}
int iswhite_int(fomus_int x) {
  ENTER_API;
  return whitenotes[x % 12];
  EXIT_API_0;
}
int iswhite_rat(struct fomus_rat x) {
  ENTER_API;
  return x.den == 1 && whitenotes[x.num % 12];
  EXIT_API_0;
}
int isblack_int(fomus_int x) {
  ENTER_API;
  return blacknotes[x % 12];
  EXIT_API_0;
}
int isblack_rat(struct fomus_rat x) {
  ENTER_API;
  return x.den == 1 && blacknotes[x.num % 12];
  EXIT_API_0;
}
fomus_int diff_int(fomus_int x, fomus_int y) {
  ENTER_API;
  return abs_int(x - y);
  EXIT_API_0;
}
struct fomus_rat diff_rat(struct fomus_rat x, struct fomus_rat y) {
  ENTER_API;
  return abs_rat(module_ratminus(x, y));
  EXIT_API_RAT0;
}
fomus_int abs_int(fomus_int x) {
  ENTER_API;
  return x >= 0 ? x : -x;
  EXIT_API_0;
}
struct fomus_rat abs_rat(struct fomus_rat x) {
  ENTER_API;
  return x.num >= 0 ? x : module_ratneg(x);
  EXIT_API_RAT0;
}
struct fomus_rat div2_rat(struct fomus_rat x) {
  ENTER_API;
  if (x.den == 1)
    x.num = div2_int(x.num);
  return x;
  EXIT_API_RAT0;
}
int isexpof2_int(fomus_int x) {
  ENTER_API;
  return div2_int(x) == 1;
  EXIT_API_0;
}
int isexpof2_rat(struct fomus_rat x) {
  ENTER_API;
  return x.den == 1 ? isexpof2_int(x.num)
                    : (abs_int(x.num) == 1 && isexpof2_int(x.den));
  EXIT_API_0;
}
