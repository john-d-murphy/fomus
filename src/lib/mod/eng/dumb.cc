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

#include "config.h"

#include <cassert>
#include <sstream>
#include <string>

#include "ifacedumb.h"
#include "module.h"

#include "dumb.h"

#include "debugaux.h"

namespace dumb {

  struct dumberr {
    const char* err;
    dumberr(const char* err) : err(err) {}
  };
  inline void checkerr(const dumb_iface& iface) {
    const char* err = iface.err(iface.moddata);
    if (err)
      throw dumberr(err);
  }

  class data _NONCOPYABLE {
public:
    dumb_iface iface;

private:
    bool cerr;
    std::stringstream CERR;
    std::string errstr;
    FOMUS fom;

public:
    dumb_iface* getiface() {
      return &iface;
    }
    data(FOMUS fom) : cerr(false), fom(fom) {}
    const char* module_err() {
      if (!cerr)
        return 0;
      std::getline(CERR, errstr);
      return errstr.c_str();
    }
    void run() {
      try {
        iface.run(fom, iface.moddata);
        checkerr(iface);
      } catch (const dumberr& e) {
        if (e.err) {
          CERR << e.err << std::endl;
        } else {
          CERR << "unknown" << std::endl;
        }
        cerr = true;
      }
    }
  };

  void* newdata(FOMUS f) {
    return new data(f);
  }
  void freedata(void* dat) {
    delete (data*) dat;
  }
  const char* err(void* dat) {
    return ((data*) dat)->module_err();
  }
  void* get_iface(void* dat) {
    return ((data*) dat)->getiface();
  }
  void run(void* dat) {
    ((data*) dat)->run();
  }

} // namespace dumb
