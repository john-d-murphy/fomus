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

#ifndef FOMUS_ALGEXT_H
#define FOMUS_ALGEXT_H

#ifdef BUILD_LIBFOMUS
#include "heads.h"
#endif

#include "modutil.h"
#include "numbers.h"

namespace FNAMESPACE {

  template <typename I, typename T>
  inline bool hassome(I first, const I& last, const T& pred) {
    while (first != last)
      if (pred(*first++))
        return true;
    return false;
  }
#ifdef BUILD_LIBFOMUS
  struct ll_hassome {
    typedef bool result_type;
    template <typename I, typename T>
    bool operator()(const I& a, const I& b, T& c) const {
      return hassome(a, b, c);
    }
  };
#endif

  template <typename I, typename T>
  inline bool hasnone(I first, const I& last, const T& pred) {
    while (first != last)
      if (pred(*first++))
        return false;
    return true;
  }
#ifdef BUILD_LIBFOMUS
  struct ll_hasnone {
    typedef bool result_type;
    template <typename I, typename T>
    bool operator()(const I& a, const I& b, T& c) const {
      return hasnone(a, b, c);
    }
  };
#endif

  template <typename I, typename T>
  inline bool hasall(I first, const I& last, const T& pred) {
    while (first != last)
      if (!pred(*first++))
        return false;
    return true;
  }
#ifdef BUILD_LIBFOMUS
  struct ll_hasall {
    typedef bool result_type;
    template <typename I, typename T>
    bool operator()(const I& a, const I& b, T& c) const {
      return hasall(a, b, c);
    }
  };
#endif

  template <typename I, typename F>
  inline F for_each_it(I first, const I& last, F& fun) {
    while (first != last)
      fun(first++);
    return fun;
  }
#ifdef BUILD_LIBFOMUS
  struct ll_for_each_it {
    template <typename Args>
    struct sig {
      typedef typename boost::remove_cv<
          typename boost::tuples::element<3, Args>::type>::type type;
    };
    template <typename I, typename F>
    F operator()(const I& a, const I& b, F& c) const {
      return for_each_it(a, b, c);
    }
  };
#endif

  template <typename I1, typename I2, typename F>
  inline F for_each2(I1 first1, const I1& last1, I2 first2, F& fun) {
    while (first1 != last1)
      fun(*first1++, *first2++);
    return fun;
  }
#ifdef BUILD_LIBFOMUS
  struct ll_for_each2 {
    template <typename Args>
    struct sig {
      typedef typename boost::remove_cv<
          typename boost::tuples::element<4, Args>::type>::type type;
    };
    template <typename I1, typename I2, typename F>
    F operator()(const I1& a, const I1& b, const I2& c, F& d) const {
      return for_each2(a, b, c, d);
    }
  };
#endif

  template <typename I, typename F>
  inline F for_each_reverse(const I& first, I last, F& fun) {
    while (first != last)
      fun(*--last);
    return fun;
  }
#ifdef BUILD_LIBFOMUS
  struct ll_for_each_reverse {
    template <typename Args>
    struct sig {
      typedef typename boost::remove_cv<
          typename boost::tuples::element<3, Args>::type>::type type;
    };
    template <typename I, typename F>
    F operator()(const I& a, const I& b, F& c) const {
      return for_each_reverse(a, b, c);
    }
  };
#endif

  template <typename I, typename F>
  inline F for_each_it_reverse(const I& first, I last, F& fun) {
    while (first != last)
      fun(--last);
    return fun;
  }
#ifdef BUILD_LIBFOMUS
  struct ll_for_each_it_reverse {
    template <typename Args>
    struct sig {
      typedef typename boost::remove_cv<
          typename boost::tuples::element<3, Args>::type>::type type;
    };
    template <typename I, typename F>
    F operator()(const I& a, const I& b, F& c) const {
      return for_each_it_reverse(a, b, c);
    }
  };
#endif

  template <typename I>
  inline typename std::iterator_traits<I>::difference_type
  match_count(I first1, const I& last1, I first2, const I& last2) {
    typename std::iterator_traits<I>::difference_type c(0), m(0);
    while (first1 != last1 && first2 != last2) {
      if (*(first1++) == *(first2++)) {
        ++c;
        if (c > m)
          m = c;
      } else {
        c = 0;
      }
    }
    return m;
  }

  template <typename R, typename I, typename F>
  inline R maximize(I first, const I& last, F& fun, const R& def = 0) {
    if (first == last)
      return def;
    R sc(fun(*first++));
    while (first != last) {
      R c(fun(*first++));
      if (sc < c)
        sc = c;
    }
    return sc;
  }
#ifdef BUILD_LIBFOMUS
  struct ll_maximize {
    template <typename Args>
    struct sig {
      typedef typename boost::remove_cv<
          typename boost::tuples::element<4, Args>::type>::type type;
    };
    template <typename R, typename I, typename F>
    R operator()(const I& a, const I& b, F& c, const R& d = 0) const {
      return maximize(a, b, c, d);
    }
  };
#endif

  template <typename R, typename I, typename F>
  inline R minimize(I first, const I& last, F& fun, const R& def = 0) {
    if (first == last)
      return def;
    R sc(fun(*first++));
    while (first != last) {
      R c(fun(*first++));
      if (c < sc)
        sc = c;
    }
    return sc;
  }
#ifdef BUILD_LIBFOMUS
  struct ll_minimize {
    template <typename Args>
    struct sig {
      typedef typename boost::remove_cv<
          typename boost::tuples::element<4, Args>::type>::type type;
    };
    template <typename R, typename I, typename F>
    R operator()(const I& a, const I& b, F& c, const R& d = 0) const {
      return minimize(a, b, c, d);
    }
  };
#endif

  template <typename R, typename I, typename F>
  inline R accumulate_results(I first, const I& last, F& fun) {
    R s((R) 0);
    while (first != last)
      s += fun(*first++);
    return s;
  }

  template <typename I, typename T>
  inline void copy_ptrs(I first1, const I& last, T first2) {
    while (first1 != last)
      *(first2++) = &(*first1++);
  }

  template <class T>
  inline void renew(T& obj) {
    obj.~T();
    ::new (&obj) T;
  }

  struct isiless
      : std::binary_function<const std::string&, const std::string&, bool> {
    bool operator()(const std::string& x, const std::string& y) const {
      return boost::algorithm::ilexicographical_compare(x, y);
    }
  };

#ifdef BUILD_LIBFOMUS
  struct range : public modutil_range {
    // numb x1, x2;
    range(const numb& xx1, const numb& xx2) {
      assert(xx1.type() != module_none);
      assert(xx2.type() != module_none);
      assert(xx1 <= xx2);
      x1 = xx1;
      x2 = xx2;
    }
    range(const module_value& xx1, const module_value& xx2) {
      assert(xx1.type != module_none);
      assert(xx2.type != module_none);
      assert(numb(xx1) <= numb(xx2));
      x1 = xx1;
      x2 = xx2;
    }
    bool operator<(const range& x) const {
      return x2 < x.x1;
    }
    range(const modutil_range& x) : modutil_range(x) {}
  }; // can be equal if touching

  class ranges : public std::multiset<range> {
    std::vector<modutil_range> arr;

public:
    ranges() : std::multiset<range>() {}
    ranges(const range& fill) : std::multiset<range>() {
      insert(fill);
    }
    void insert_range(range x) {
      iterator i1(std::multiset<range>::lower_bound(x));
      iterator i2(std::multiset<range>::upper_bound(x));
      if (i1 != i2) { // i1 must be valid, i2 must be >begin
        if (x.x1 > i1->x1)
          x.x1 = i1->x1;
        iterator i0(boost::prior(i2));
        if (x.x2 < i0->x2)
          x.x2 = i0->x2;
        erase(i1, i2);
      }
      insert(x);
    }
    void remove_range(const range& x) {
      iterator i1(std::multiset<range>::lower_bound(x));
      iterator i2(std::multiset<range>::upper_bound(x));
      if (i1 != i2) { // i1 must be valid, i2 must be >begin
        module_value t1(i1->x1);
        module_value t2(boost::prior(i2)->x2);
        erase(i1, i2);
        if (t1 < x.x1)
          insert(range(t1, x.x1));
        if (x.x2 < t2)
          insert(range(x.x2, t2));
      }
    }
    modutil_ranges getranges() {
      arr.assign(begin(), end());
      modutil_ranges ret;
      ret.n = arr.size();
      ret.ranges = &arr[0];
      return ret;
    }
  };
#endif

} // namespace FNAMESPACE
#endif
