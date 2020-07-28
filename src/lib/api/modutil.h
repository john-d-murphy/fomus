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

/*
  ALL COMMENTS IN THIS FILE ARE OLD AND HAVEN'T BEEN UPDATED YET!
*/

#ifndef FOMUS_MODUTIL_H
#define FOMUS_MODUTIL_H

#include "module.h"

#ifdef __cplusplus
typedef bool modutil_bool; // only use in `inline' functions
#else
typedef int modutil_bool;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// integer, rational abs
#ifdef NO_INLINE
LIBFOMUS_EXPORT fomus_int abs_int(fomus_int x);
LIBFOMUS_EXPORT struct fomus_rat abs_rat(struct fomus_rat x);
#else
inline fomus_int abs_int(fomus_int x) {
  return x >= 0 ? x : -x;
}
inline struct fomus_rat abs_rat(struct fomus_rat x) {
  return x.num >= 0 ? x : module_ratneg(x);
}
#endif

LIBFOMUS_EXPORT fomus_int mod_int(fomus_int x, fomus_int y);
LIBFOMUS_EXPORT fomus_float mod_float(fomus_float x, fomus_float y);
LIBFOMUS_EXPORT struct fomus_rat mod_rat(struct fomus_rat x,
                                         struct fomus_rat y);

// divide by 2 until can't anymore
LIBFOMUS_EXPORT fomus_int div2_int(fomus_int x);
#ifdef NO_INLINE
LIBFOMUS_EXPORT struct fomus_rat div2_rat(struct fomus_rat x);
#else
inline struct fomus_rat div2_rat(struct fomus_rat x) {
  if (x.den == 1)
    x.num = div2_int(x.num);
  return x;
}
#endif

// get highest power of 2 <= x
LIBFOMUS_EXPORT fomus_int maxdiv2_int(fomus_int x);
LIBFOMUS_EXPORT struct fomus_rat maxdiv2_rat(struct fomus_rat x);

// is exponent of 2
#ifdef NO_INLINE
LIBFOMUS_EXPORT int isexpof2_int(fomus_int x);
LIBFOMUS_EXPORT int isexpof2_rat(struct fomus_rat x);
#else
inline modutil_bool isexpof2_int(fomus_int x) {
  return div2_int(x) == 1;
}
inline modutil_bool isexpof2_rat(struct fomus_rat x) {
  return x.den == 1 ? isexpof2_int(x.num)
                    : (abs_int(x.num) == 1 && isexpof2_int(x.den));
}
#endif

// floor function
LIBFOMUS_EXPORT fomus_int mfloor(struct module_value x);
LIBFOMUS_EXPORT fomus_int mfloorto_int(struct module_value x, fomus_int to);
LIBFOMUS_EXPORT struct fomus_rat mfloorto_rat(struct module_value x,
                                              struct fomus_rat to);

LIBFOMUS_EXPORT fomus_int roundto_int(struct module_value x, fomus_int to);
LIBFOMUS_EXPORT struct fomus_rat roundto_rat(struct module_value x,
                                             struct fomus_rat to);

struct modutil_lowmults {
  fomus_int n;
  fomus_int* vals;
};
LIBFOMUS_EXPORT struct modutil_lowmults lowmults(fomus_int x);

struct modutil_range {
  struct module_value x1, x2;
};
struct modutil_ranges {
  fomus_int n;
  struct modutil_range* ranges;
};
typedef void* modutil_rangesobj;
LIBFOMUS_EXPORT modutil_rangesobj ranges_init();
LIBFOMUS_EXPORT void ranges_insert(modutil_rangesobj rangeobj,
                                   struct modutil_range range);
LIBFOMUS_EXPORT void ranges_remove(modutil_rangesobj rangeobj,
                                   struct modutil_range range);
LIBFOMUS_EXPORT void ranges_free(modutil_rangesobj rangeobj);
LIBFOMUS_EXPORT struct modutil_ranges ranges_get(modutil_rangesobj rangeobj);
LIBFOMUS_EXPORT fomus_int ranges_size(modutil_rangesobj rangeobj);

// difference
#ifdef NO_INLINE
LIBFOMUS_EXPORT fomus_int diff_int(fomus_int x, fomus_int y);
LIBFOMUS_EXPORT struct fomus_rat diff_rat(struct fomus_rat x,
                                          struct fomus_rat y);
#else
inline fomus_int diff_int(fomus_int x, fomus_int y) {
  return abs_int(x - y);
}
inline struct fomus_rat diff_rat(struct fomus_rat x, struct fomus_rat y) {
  return abs_rat(module_ratminus(x, y));
}
#endif

struct modutil_rhythm {
  struct fomus_rat dur;
  int dots;
};
LIBFOMUS_EXPORT struct modutil_rhythm rhythm(struct fomus_rat dur);

// ------------------------------------------------------------------------------------------------------------------------
// NOTES STUFF

LIBFOMUS_EXPORT extern const int
    whitenotes[]; // array of 12 true/false (0/1) values (1 = white note)
LIBFOMUS_EXPORT extern const int blacknotes[]; // 1 = black note
#ifdef NO_INLINE
LIBFOMUS_EXPORT int iswhite_int(fomus_int x);
LIBFOMUS_EXPORT int iswhite_rat(struct fomus_rat x);
LIBFOMUS_EXPORT int isblack_int(fomus_int x);
LIBFOMUS_EXPORT int isblack_rat(struct fomus_rat x);
#else
inline int iswhite_int(fomus_int x) {
  return whitenotes[x % 12];
}
inline int iswhite_rat(struct fomus_rat x) {
  return x.den == 1 && whitenotes[x.num % 12];
}
inline int isblack_int(fomus_int x) {
  return blacknotes[x % 12];
}
inline int isblack_rat(struct fomus_rat x) {
  return x.den == 1 && blacknotes[x.num % 12];
}
#endif

LIBFOMUS_EXPORT extern const int
    diatonicnotes[]; // array of 12 w/ mappings to 0-6
LIBFOMUS_EXPORT extern const int
    chromaticnotes[]; // array of 7 w/ mappings to 0-11
#ifdef NO_INLINE
LIBFOMUS_EXPORT fomus_int todiatonic(fomus_int x);
LIBFOMUS_EXPORT fomus_int tochromatic(fomus_int x);
#else
inline fomus_int todiatonic(fomus_int x) {
  return ((x / 12) * 7) + diatonicnotes[x % 12];
}
inline fomus_int tochromatic(fomus_int x) {
  return ((x / 7) * 12) + chromaticnotes[x % 7];
}
#endif

LIBFOMUS_EXPORT const char* toroman(int n);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus) && !defined(BUILD_LIBFOMUS)

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
                    : (abs_int(x.num) == 1 && isexpof2(x.den));
}

inline fomus_int diff(fomus_int x, fomus_int y) {
  return abs_int(x - y);
}
inline fomus_rat diff(const fomus_rat& x, const fomus_rat& y) {
  return abs_rat(x - y);
}

inline fomus_int roundto(const module_value& x, fomus_int to) {
  return mfloor(((fomus_int) 2 * x + to) / ((fomus_int) 2 * to)) * to;
}
inline fomus_rat roundto(const module_value& x, const fomus_rat& to) {
  return mfloor(((fomus_int) 2 * x + to) / ((fomus_int) 2 * to)) * to;
}

inline fomus_int floorto(const module_value& x, fomus_int to) {
  return mfloor(x / to) * to;
}
inline fomus_rat floorto(const module_value& x, const fomus_rat& to) {
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
inline module_value mod(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return module_makeval(mod(a.val.i, b));
  case module_rat:
    return module_makeval(mod(a.val.r, b));
  case module_float:
    return module_makeval(mod(a.val.f, b));
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline module_value mod(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return mod(a, module_makeval(b.val.i));
  case module_rat:
    return mod(a, module_makeval(b.val.r));
  case module_float:
    return mod(a, module_makeval(b.val.f));
  default:
    throw fomus_exception();
  }
}
inline module_value mod(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return mod(a, b.val.i);
  case module_rat:
    return mod(a, b.val.r);
  case module_float:
    return mod(a, b.val.f);
  default:
    throw fomus_exception();
  }
}

#endif

#endif
