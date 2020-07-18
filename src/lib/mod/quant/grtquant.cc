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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <boost/utility.hpp>

#include "ifacedumb.h"
#include "module.h"
#include "modutil.h"

#include "debugaux.h"

namespace grtquant {

  int defgracedurid;

  extern "C" {
  void dumb_run(FOMUS fom, void* moddata); // return true when done
  const char* dumb_err(void* moddata);
  }

  struct holder {
    module_noteobj n;
    module_value o1, o2;
    fomus_rat dur;
    holder(const module_noteobj n)
        : n(n), o1(module_vgracetime(n)), o2(module_vgraceendtime(n)) {}
    void fixupdur(const fomus_rat& wrm);
    void setdur(const fomus_rat& d) {
      dur = d;
      o2 = o1 + dur;
    }
    void setoff(const fomus_rat& o) {
      o1 = module_makeval(o);
      o2 = module_makeval(o + dur);
    }
    void setendoff(const fomus_rat& o) {
      o2 = module_makeval(o);
      o1 = module_makeval(o - dur);
    }
    void assign() const {
      // assert(o1.type == module_rat);
      // assert(o2.type == module_rat);
      fomus_rat o10(GET_R(o1)), o20(GET_R(o2));
      DBG("GRTQUANT ASSIGN ti=" << module_time(n)
                                << " grti=" << module_makeval(o10) << " grdur="
                                << module_makeval(o20 - o10) << std::endl);
      tquant_assign_gracetime(n, module_time(n), module_makeval(o10),
                              module_makeval(o20 - o10));
    }
  };
  // get the closest power-of-2 duration
  void holder::fixupdur(const fomus_rat& wrm) {
    if (o1 < (fomus_int) -1000 || o1 > (fomus_int) 1000)
      return; // it's an autodur or a 0-dur
    if (o1 >= o2) {
      dur = module_makerat(0, 1);
      return;
    }
    fomus_float d = GET_F((o2 - o1) * wrm);
    fomus_rat st = {1, 1}; // the result, must be / by wrm
    bool di = (d >= st);   // direction = *2 or /2
    fomus_float df = std::numeric_limits<fomus_float>::max();
    fomus_rat st0;
    assert(fabs(d - st) < df);
    while (true) {
      fomus_float d0 = fabs(d - st);
#warning "put a min and max on this!"
      if (d0 >= df) {
        dur = st0 / wrm;
        return;
      }
      df = d0;
      st0 = st;
      if (di)
        st = st * (fomus_int) 2;
      else
        st = st / (fomus_int) 2;
    }
  }
  struct holderless
      : std::binary_function<const holder&, const fomus_int, bool> {
    bool operator()(const holder& x, const fomus_int y) const {
      return x.o1 < y;
    }
  };

  inline bool isolap(const holder& x, const module_value& yo1,
                     const module_value& yo2, const fomus_rat& ydur) {
    return (std::min(x.o2, yo2) - std::max(x.o1, yo1) >=
            (x.dur + ydur) / (fomus_int) 4);
  }

  void blastem(std::vector<holder>& vect, const fomus_rat& wrm) {
    std::vector<holder>::iterator be(vect.end()), mi(vect.end()),
        en(vect.end()); // start at first user grace
    for (std::vector<holder>::iterator i(vect.begin()); i != vect.end(); ++i) {
      if (i->o1 >= (fomus_int) -1000 && be == vect.end())
        be = i;
      if (i->o1 > (fomus_int) 1000 && en == vect.end())
        en = i;
      if (i->o1 >= (fomus_int) 0 && mi == vect.end())
        mi = i;
      i->fixupdur(wrm);
    }
    assert(!vect.empty());
    for (std::vector<holder>::iterator c(be), i(boost::next(be)); i < en;
         ++i) { // loop through all, group into chords & set to same durs
      if (c->o1 == i->o1) {    // same offset
        if (i->dur > c->dur) { // if i is greater dur than previous, rest durs
                               // from c to i
          for (std::vector<holder>::iterator j(c); j < i; ++j)
            j->setdur(i->dur);
        }
      } else
        c = i;
    }
    for (std::vector<holder>::iterator i(be); i < en; ++i) {
      if (i->dur <= (fomus_int) 0)
        i->setdur(module_setting_rval(i->n, defgracedurid));
    }
    if (mi != en) {
      fomus_rat cur = {0, 1};
      for (std::vector<holder>::iterator i(mi), l(boost::prior(en));;
           ++i) { // 0 and after
        module_value o1(i->o1), o2(i->o2);
        i->setoff(cur);
        if (i >= l)
          break;
        if (!isolap(*boost::next(i), o1, o2, i->dur)) {
          cur = cur + i->dur;
        }
      }
    }
    if (mi > be) {
      fomus_rat cur = {0, 1};
      for (std::vector<holder>::iterator i(boost::prior(mi));;
           --i) { // before 0
        module_value o1(i->o1), o2(i->o2);
        i->setendoff(cur);
        if (i == be)
          break;
        if (!isolap(*boost::prior(i), o1, o2, i->dur)) {
          cur = cur - i->dur;
        }
      }
    }
    for (std::vector<holder>::iterator i(vect.begin()); i != vect.end(); ++i)
      i->assign();
  }

  struct grquant {
    bool cerr;
    std::stringstream CERR;
    std::string errstr;
    grquant() : cerr(false) {}
    const char* module_err() {
      if (!cerr)
        return 0;
      std::getline(CERR, errstr);
      return errstr.c_str();
    }
    void run(FOMUS fom) { // return true when done
      fomus_rat wrm(module_writtenmult(module_nextmeas()));
      module_value ti;
      ti.type = module_int;
      ti.val.i = -1;
      std::vector<holder> vect;
      while (true) {
        module_noteobj n = module_nextnote();
        if (!n) {
          if (!vect.empty())
            blastem(vect, wrm);
          break;
        }
        module_value ti0(module_vtime(n));
        if (ti0.type == module_float) {
          cerr = true;
          CERR << "time must be quantized before grace time" << std::endl;
          return;
        }
        if (ti0 == ti) {
          vect.push_back(n);
        } else {
          if (!vect.empty()) {
            blastem(vect, wrm);
            vect.clear();
          }
          vect.push_back(n);
          ti = ti0;
        }
      }
    }
  };
  void dumb_run(FOMUS fom, void* moddata) {
    return ((grquant*) moddata)->run(fom);
  }
  const char* dumb_err(void* moddata) {
    return ((grquant*) moddata)->module_err();
  }
  void dumb_free_moddata(void* moddata) {
  } // free up mod_data structure and choices vector when finished

} // namespace grtquant

using namespace grtquant;

int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
void module_fill_iface(void* moddata, void* iface) { // module
  ((dumb_iface*) iface)->moddata = moddata;
  ((dumb_iface*) iface)->run = dumb_run;
  ((dumb_iface*) iface)->err = dumb_err;
}

const char* module_longname() {
  return "Quantize Grace Times/Durations";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Quantizes and aligns grace note times and durations.";
}
void* module_newdata(FOMUS f) {
  return new grquant;
}
void module_freedata(void* dat) {
  delete ((grquant*) dat);
}
int module_priority() {
  return 10;
}
enum module_type module_type() {
  return module_modtquant;
}
const char* module_initerr() {
  return 0;
}
int module_itertype() {
  return module_bypart | module_graceonly;
}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}

int module_get_setting(int n, module_setting* set, int id) {
  return 0;
}

const char* module_engine(void*) {
  return "dumb";
}
void module_ready() {
  defgracedurid = module_settingid("default-gracedur");
}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
