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

#ifndef MODULE_FULLTIME_H
#define MODULE_FULLTIME_H

#include "config.h"

#include <limits>
#include "module.h"

namespace ftimeaux {

  struct ftime {
    fomus_rat time;
    fomus_rat grtime;
    ftime(const fomus_rat& time, const fomus_rat& grtime):time(time), grtime(grtime) {}
    ftime(const fomus_rat& time):time(time) {grtime.num = std::numeric_limits<fomus_int>::min();}
    ftime(const module_noteobj no):time(module_time(no)) {
      if (module_isgrace(no)) grtime = module_gracetime(no); else grtime.num = std::numeric_limits<fomus_int>::min();
    }
    ftime(const module_noteobj no, const int):time(module_endtime(no)) {
      if (module_isgrace(no)) grtime = module_graceendtime(no); else grtime.num = std::numeric_limits<fomus_int>::min();
    }
    ftime(const bool isgr, const module_noteobj no):time(module_time(no)) {
      if (isgr) grtime = module_gracetime(no); else grtime.num = std::numeric_limits<fomus_int>::min();
    }
    ftime(const bool isgr, const module_noteobj no, int):time(module_endtime(no)) {
      if (isgr) grtime = module_graceendtime(no); else grtime.num = std::numeric_limits<fomus_int>::min();
    }
    bool isgrace() const {return grtime.num == std::numeric_limits<fomus_int>::min();}
  };

  inline bool operator<(const ftime& x, const ftime& y) {
    if (x.time == y.time) {
      return x.isgrace() ? false : (y.isgrace() ? true : x.grtime < y.grtime);
    } else return x.time < y.time;
  }
  inline bool operator>(const ftime& x, const ftime& y) {return y < x;}
  inline bool operator==(const ftime& x, const ftime& y) { // grace offset might be `none'
    if (x.time == y.time) {
      if (x.isgrace() && y.isgrace()) return true;
      if (x.isgrace() || y.isgrace()) return false;
      assert(!x.isgrace());
      assert(!y.isgrace());
      return x.grtime == y.grtime;
    } else return false;
  }
  inline bool operator!=(const ftime& x, const ftime& y) {
    return !(x == y);
  }  
  inline bool operator<=(const ftime& x, const ftime& y) {
    if (x.time == y.time) {
      if (x.isgrace()) return (y.isgrace());
      if (y.isgrace()) return true;
      assert(!x.isgrace());
      assert(!y.isgrace());
      return x.grtime <= y.grtime;
    } else return x.time <= y.time;
  }
  inline bool operator>=(const ftime& x, const ftime& y) {return y <= x;}

  struct ftimewends {
    fomus_rat time;
    fomus_rat grtime;
    bool isend;
    ftimewends(const fomus_rat& time, const fomus_rat& grtime):time(time), grtime(grtime), isend(false) {}
    ftimewends(const fomus_rat& time):time(time), isend(false) {grtime.num = std::numeric_limits<fomus_int>::min();}
    ftimewends(const fomus_rat& time, const int):time(time), isend(true) {grtime.num = std::numeric_limits<fomus_int>::min();}
    ftimewends(const module_noteobj no):time(module_time(no)), isend(false) {
      if (module_isgrace(no)) grtime = module_gracetime(no); else grtime.num = std::numeric_limits<fomus_int>::min();
    }
    ftimewends(const module_noteobj no, const int):time(module_endtime(no)), isend(true) {
      if (module_isgrace(no)) grtime = module_graceendtime(no); else grtime.num = std::numeric_limits<fomus_int>::min();
    }
    ftimewends(const bool isgr, const module_noteobj no):time(module_time(no)), isend(false) {
      if (isgr) grtime = module_gracetime(no); else grtime.num = std::numeric_limits<fomus_int>::min();
    }
    ftimewends(const bool isgr, const module_noteobj no, const int):time(module_endtime(no)), isend(true) {
      if (isgr) grtime = module_graceendtime(no); else grtime.num = std::numeric_limits<fomus_int>::min();
    }
    bool isgrace() const {return grtime.num != std::numeric_limits<fomus_int>::min();}
  };

  inline bool operator<(const ftimewends& x, const ftimewends& y) {
    if (x.time == y.time) {
      if (x.isgrace()) {
	if (y.isgrace()) return x.grtime < y.grtime; // both are grace
	return !y.isend; // x is, y isn't
      } else {
	if (y.isgrace()) return x.isend; // x isn't, y is
	return false; // neither are
      }
    } else return x.time < y.time;
  }
  inline bool operator>(const ftimewends& x, const ftimewends& y) {return y < x;}
  inline bool operator==(const ftimewends& x, const ftimewends& y) { // grace offset might be `none'
    if (x.time == y.time) {
      if (x.isgrace()) {
	if (y.isgrace()) return x.grtime == y.grtime; // both are grace
      } else {
	if (!y.isgrace()) return true; // neither are
      }
      return false;
    } else return false;
  }
  inline bool operator!=(const ftimewends& x, const ftimewends& y) {
    return !(x == y);
  }  
  inline bool operator<=(const ftimewends& x, const ftimewends& y) {
    if (x.time == y.time) {
      if (x.isgrace()) {
	if (y.isgrace()) return x.grtime <= y.grtime; // both are grace
	return !y.isend; // x is, y isn't
      } else {
	if (y.isgrace()) return x.isend; // x isn't, y is
	return true; // neither are
      }
    } else return x.time <= y.time;
  }
  inline bool operator>=(const ftimewends& x, const ftimewends& y) {return y <= x;}
  
}

#endif
