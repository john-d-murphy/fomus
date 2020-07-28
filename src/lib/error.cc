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

#include "error.h"
#include "fomusapi.h"
#include "schedr.h"

namespace fomus {

  bool isinited = false;

  boost::mutex outlock;

  bool newline = true;

  const std::string intname("(internal)");

  // boost::mutex writemut;
  std::streamsize myout::write(const char* s, std::streamsize n) {
    // boost::lock_guard<boost::mutex> xxx(writemut);
    std::string::size_type p0 = 0, p1 = x.size();
    x.append(s, n);
    while (true) {
      std::string::size_type p = x.find('\n', p1);
      if (p == std::string::npos) {
        x.erase(0, p0);
        return n;
      }
      if (fun) {
        std::string x0(x.substr(p0, newline ? (p + 1) - p0 : p - p0));
        fun((iserr ? "fomus: " + x0 : x0).c_str());
      }
      p0 = p1 = ++p;
    }
  }

  // stdout & stderr are unbuffered
  void fomstdout(const char* out) {
    fputs(out, stdout);
  }
  void fomstderr(const char* out) {
    fputs(out, stderr);
  }

  boost::iostreams::stream<myout> fout(fomstdout, stdout, false),
      ferr(fomstderr, stderr, true);

  boost::thread_specific_ptr<int> fomerr(delvoidobj);

} // namespace fomus

using namespace fomus;

void fomus_set_outputs(fomus_output out, fomus_output err, int newline0) {
  fout->setfun(out);
  ferr->setfun(err);
  newline = newline0;
}

int fomus_err() {
  return (boost::int_t<sizeof(int*) * 8>::fast) fomerr.get();
}
