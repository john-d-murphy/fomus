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

#include <boost/algorithm/string/predicate.hpp>

#include "ifacedumb.h"
#include "module.h"

#include "debugaux.h"
#include "ferraux.h"
using namespace ferraux;

namespace percchs {

  const char* ierr = 0;

  int pchangeid;

  extern "C" {
  void run_fun(FOMUS fom, void* moddata); // return true when done
  const char* err_fun(void* moddata);
  }

  struct percsdata {
    bool cerr;
    std::stringstream MERR;
    std::string errstr;
    int cnt;

    percsdata() : cerr(false), cnt(0) {}

    void doerr(const module_noteobj n, const char* str);
    void run() {
      std::string cur;
      while (true) {
        module_noteobj n = module_nextnote();
        if (!n)
          break;
        const char* c0 = module_setting_sval(n, pchangeid);
        std::string c(c0 ? c0 : "");
        DBG("mark@" << module_time(n) << std::endl);
        if (c != cur) {
          if (c.empty()) {
            doerr(n, "no `perc-change' setting value");
            module_skipassign(n);
            continue;
          }
          cur = c;
          struct module_markslist l(module_singlemarks(n));
          for (const module_markobj *m = l.marks, *me = l.marks + l.n; m < me;
               ++m) {
            if (module_markid(*m) == mark_stafftext &&
                boost::algorithm::iequals(module_markstring(*m), cur)) {
              module_skipassign(n);
              continue;
            }
          }
          marks_assign_add(n, mark_stafftext, cur.c_str(),
                           module_makenullval());
          marks_assign_done(n);
        } else
          module_skipassign(n);
      }
      if (cnt) {
        MERR << "invalid percussion changes" << std::endl;
        cerr = true;
      }
    }
    const char* err() {
      if (!cerr)
        return 0;
      std::getline(MERR, errstr);
      return errstr.c_str();
    }
  };

  void percsdata::doerr(const module_noteobj n, const char* str) {
    if (cnt < 8) {
      CERR << str;
      ferr << module_getposstring(n) << std::endl;
    } else if (cnt == 8) {
      CERR << "more errors..." << std::endl;
    }
    ++cnt;
  }

  void run_fun(FOMUS fom, void* moddata) {
    ((percsdata*) moddata)->run();
  }
  const char* err_fun(void* moddata) {
    return ((percsdata*) moddata)->err();
  }

} // namespace percchs

using namespace percchs;

int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
void module_fill_iface(void* moddata, void* iface) { // module
  ((dumb_iface*) iface)->moddata = moddata;
  ((dumb_iface*) iface)->run = run_fun;
  ((dumb_iface*) iface)->err = err_fun;
}
const char* module_err(void* data) {
  return 0;
}
const char* module_longname() {
  return "Percussion Changes";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Adds percussion change markings.";
}
void* module_newdata(FOMUS f) {
  return new percsdata;
} // new accsdata;}
void module_freedata(void* dat) {
  delete (percsdata*) dat;
} // delete (accsdata*)dat;}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modspecial;
}
int module_itertype() {
  return module_bypart | module_byvoice | module_perconly | module_norests;
}
const char* module_initerr() {
  return ierr;
}
void module_init() {}
void module_free() {}
int module_get_setting(int n, struct module_setting* set, int id) {
  return 0;
  // switch (n) {
  // case 0:
  //   {
  //     set->name = "perc-change"; // docs*cat{instsparts}
  //     set->type = module_string;
  //     set->descdoc = "A text string that appears above the staff in a
  //     percussion part that indicates a change in instrumentation.  "
  // 	"Set this in a percussion instrument definition in addition to the ID
  // and `name' setting to create automatic instrument change text markings.  "
  // 	"Percussion instruments with the same `perc-change' string are
  // effectively grouped together for the purpose of inserting percussion change
  // markings "
  // 	"(e.g., a set of differently-pitched woodblocks with the same
  // `perc-change' value would cause a single instrumentation change text
  // marking " 	"to appear in the score over the first note belonging to this
  // set).";
  //     //set->typedoc = "string_"; // same valid fun

  //     module_setval_string(&set->val, "");

  //     set->loc = module_locpercinst;
  //     //set->valid = valid_ashowtypes; // no range
  //     //set->validdeps = validdeps_maxp;
  //     set->uselevel = 2; // technical
  //     pchangeid = id;
  //     break;
  //   }
  // default: ;
  //   return 0;
  // }
  // return 1;
}
const char* module_engine(void*) {
  return "dumb";
}
void module_ready() {
  pchangeid = module_settingid("perc-name");
  if (pchangeid < 0) {
    ierr = "missing required setting `perc-name'";
    return;
  }
}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
