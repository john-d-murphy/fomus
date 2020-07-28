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

#ifndef MODULE_FERRAUX_H
#define MODULE_FERRAUX_H

#include "config.h"

#include <boost/iostreams/concepts.hpp> // sink
#include <boost/iostreams/stream.hpp>

#include "module.h"

namespace ferraux {

#define CERR ferraux::ferr
  //<< "fomus: "

  struct mymodout : public boost::iostreams::sink {
    mymodout(int) {}
    std::streamsize write(const char* s, std::streamsize n) {
      module_stderr(s, n);
      return n;
    }
  };
  boost::iostreams::stream<mymodout> ferr(mymodout(0));

} // namespace ferraux

#endif
