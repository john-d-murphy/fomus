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

#ifndef MODULE_DEBUGAUX_H
#define MODULE_DEBUGAUX_H

#include "config.h"
#ifndef NDEBUG
#include "module.h"
#include <cassert>
#include <iostream>
#endif

#include "boost/utility.hpp"

#ifndef NDEBUG
#define NONCOPYABLE , boost::noncopyable
#define _NONCOPYABLE :boost::noncopyable
#else
#define NONCOPYABLE
#define _NONCOPYABLE
#endif

#ifndef NDEBUGOUT
#define DBG(xxx) std::cout << xxx
#else
#define DBG(xxx)
#endif

#ifndef NDEBUGOUT
inline std::ostream& operator<<(std::ostream& ou, const module_value& x) {
  switch (x.type) {
  case module_int:
  case module_rat:
  case module_float: {
    ou << module_valuetostr(x);
    break;
  } break;
  case module_string:
    ou << x.val.s;
    break;
  case module_none:
    ou << "NIL";
    break;
  default:
    assert(false);
  }
  return ou;
}
inline std::ostream& operator<<(std::ostream& o, const fomus_rat& x) {
  o << module_rattostr(x);
  return o;
}
#endif

#endif
