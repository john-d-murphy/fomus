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

#include "config.h"

#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <boost/ptr_container/ptr_deque.hpp>

#include "ifacedumb.h"
#include "module.h"

#include "debugaux.h"
#include "ferraux.h"
using namespace ferraux;
#include "ilessaux.h"
using namespace ilessaux;

namespace pnotes {

  int pvoiceid, pnoteid;

  extern "C" {
  void pnotes_run_fun(FOMUS f, void* moddata); // return true when done
  const char* pnotes_err_fun(void* moddata);
  }

  struct pnotesdata {
    bool cerr;
    std::stringstream MERR;
    std::string errstr;
    int cnt;

    pnotesdata() : cerr(false), cnt(0) {}

    const char* err() {
      if (!cerr)
        return 0;
      std::getline(MERR, errstr);
      return errstr.c_str();
    }

    void doerr(const module_noteobj n, const std::string& str);

    void run() {
      bool ee = false;
      std::set<const char*, charisiless> badstrs;
      while (true) {
        module_noteobj n = module_nextnote();
        if (!n)
          break;
        module_percinstobj s = module_percinst(n); // does a lookup
        if (s) {
          assert(module_isperc(n));
          int v = module_setting_ival(s, pvoiceid);
          if (!v)
            v = module_voice(n);
          assert(v);
          percnotes_assign(n, v, module_setting_rval(s, pnoteid));
        } else {
          const char* str = module_percinststr(n);
          if (badstrs.insert(str).second) {
            std::ostringstream x;
            x << "invalid percussion instrument `" << str << '\'';
            ee = true;
            doerr(n, x.str());
          }
          module_skipassign(n);
        }
      }
      if (ee) {
        MERR << "invalid percussion instruments" << std::endl;
        cerr = true;
      }
    }
  };

  void pnotes_run_fun(FOMUS fom, void* moddata) {
    ((pnotesdata*) moddata)->run();
  }

  void pnotesdata::doerr(const module_noteobj n, const std::string& str) {
    if (cnt < 8) {
      CERR << str;
      ferr << module_getposstring(n) << std::endl;
      ++cnt;
    } else if (cnt == 8) {
      CERR << "more errors..." << std::endl;
      ++cnt;
    }
  }

  const char* pnotes_err_fun(void* moddata) {
    return ((pnotesdata*) moddata)->err();
  }

  void* newdata(FOMUS f) {
    return new pnotesdata;
  }
  void freedata(void* dat) {
    delete (pnotesdata*) dat;
  }

  void fill_iface(void* moddata, void* iface) {
    ((dumb_iface*) iface)->moddata = moddata;
    ((dumb_iface*) iface)->run = pnotes_run_fun;
    ((dumb_iface*) iface)->err = pnotes_err_fun;
  };

  const char* voicetype = "integer0..128";
  int valid_voice(struct module_value val) {
    return module_valid_int(val, 0, module_incl, 128, module_incl, 0,
                            voicetype);
  }
  // const char* notetype = "integer0..128";
  // int valid_note(struct module_value val) {return module_valid_int(val, 0,
  // module_incl, 128, module_excl, 0, notetype);}

  int get_setting(int n, module_setting* set, int id) {
    switch (n) {
    case 0: {
      set->name = "perc-voice"; // docscat{instsparts}
      set->type = module_int;
      set->descdoc = "The voice a note is inserted into when a percussion "
                     "instrument id is given as the pitch.  "
                     "If any voices are assigned to a note, they are "
                     "overridden by this value.  "
                     "This is used only when defining a percussion instrument.";
      set->typedoc = voicetype;

      module_setval_int(&set->val, 0);

      set->loc = module_locpercinst;
      set->valid = valid_voice; // no range
      set->uselevel = 2;
      pvoiceid = id;
      break;
    }
    case 1: {
      set->name = "perc-note"; // docscat{instsparts}
      set->type = module_notesym;
      set->descdoc = "The note that appears on the score when a percussion "
                     "instrument id is given as the pitch (unpitched clefs "
                     "behave like alto clefs as far as note placement goes).  "
                     "The name of the percussion instrument may then be used "
                     "in place of a note or pitch value.  "
                     "This is used only when defining a percussion instrument.";
      set->typedoc = voicetype;

      module_setval_int(&set->val, 60);

      set->loc = module_locpercinst;
      set->valid = valid_voice; // no range
      set->uselevel = 2;
      pnoteid = id;
      break;
    }
    default:
      return 0;
    }
    return 1;
  }

} // namespace pnotes
