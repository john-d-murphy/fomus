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

#ifndef MODULE_ILESSAUX_H
#define MODULE_ILESSAUX_H

#include "config.h"

#include <boost/algorithm/string/predicate.hpp>
#include <string>

namespace ilessaux {

  struct isiless
      : std::binary_function<const std::string&, const std::string&, bool> {
    bool operator()(const std::string& x, const std::string& y) const {
      return boost::algorithm::ilexicographical_compare(x, y);
    }
  };
  struct charisiless : std::binary_function<const char*, const char*, bool> {
    bool operator()(const char* x, const char* y) const {
      return boost::algorithm::ilexicographical_compare(x, y);
    }
  };

} // namespace ilessaux

#endif
