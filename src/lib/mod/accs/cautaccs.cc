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
#include <cstring> // strcmp
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <boost/algorithm/string/predicate.hpp>

#include "ifacedist.h"
#include "ifacedumb.h"
#include "infoapi.h"
#include "module.h"
#include "modutil.h"

#include "ilessaux.h"
using namespace ilessaux;

namespace cautaccs {

  int octdiffid, octsignsid, rangeid, allowedid, enginemodid, octdistid,
      beatdistid, distmodid;

  enum octs_enum { octs_1, octs_all, octs_none };
  typedef std::map<const std::string, octs_enum, isiless> strtooctstype;
  typedef strtooctstype::value_type strtooctstype_val;
  typedef strtooctstype::const_iterator strtooctstype_constit;
  strtooctstype octsstrtoenum;

  inline bool octchangeeq(const module_noteobj p, const module_noteobj n,
                          const int pn, const int nt, const octs_enum dis) {
    int pno = pn - module_octsign(p) * 7;
    int nto = nt - module_octsign(n) * 7;
    return (pno == nto || (dis == octs_1 && diff(pn, nt) == 7) ||
            (dis == octs_all && (pn % 7 == nt % 7)) || pn == nto ||
            (dis == octs_1 && diff(pn, nt) == 7) ||
            (dis == octs_all && (pn % 7 == nt % 7)) || pno == nt ||
            (dis == octs_1 && diff(pn, nt) == 7) ||
            (dis == octs_all && (pn % 7 == nt % 7)));
  }

  extern "C" {
  void run_fun(FOMUS f, void* moddata); // return true when done
  const char* err_fun(void* moddata);
  }

  struct scoped_distiface : public dist_iface {
    ~scoped_distiface() {
      free_moddata(moddata);
    }
  };

  // by part
  void run_fun(FOMUS f, void* moddata) { // neither of these parameters used
    // fomus_float rng, range; // range is the MAXIMUM, sent to distance
    // function
    module_noteobj p = module_peeknextnote(0);
    scoped_distiface diface;
    diface.data.octdist_setid = octdistid;
    diface.data.beatdist_setid = beatdistid;
    diface.data.byendtime = true;
    diface.data.rangemax = module_setting_fval(p, rangeid);
    module_get_auxiface(module_setting_sval(p, distmodid), DIST_INTERFACEID,
                        &diface);
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n)
        break;
      if (!module_setting_ival(n, allowedid) ||
          (module_writtenacc1(n) !=
               module_makerat(std::numeric_limits<fomus_int>::max(), 1) &&
           module_writtenacc2(n) !=
               module_makerat(std::numeric_limits<fomus_int>::max(), 1))) {
        module_skipassign(n);
        continue; // accidental that appears in score!
      }
      while (p != n &&
             diface.is_outofrange(diface.moddata, p,
                                  n)) { // set p to first note in range
        // module_skipassign(p);
        p = module_peeknextnote(p);
      }
      int nt = module_writtennote(n);
      // fomus_float rng = module_setting_fval(n, rangeid);
      octs_enum dis = octsstrtoenum.find(module_setting_sval(n, octdiffid))
                          ->second; // should have checked for this already
      bool octsigns = module_setting_ival(n, octsignsid);
      fomus_rat nacc(module_fullacc(n));
      fomus_rat pacc(nacc);
      module_noteobj p0 = p;
      while (p0 != n) {
        if (diface.dist(diface.moddata, p0, n) <= diface.data.rangemax) {
          int pn = module_writtennote(p0); // white key
          if (pn == nt || (dis == octs_1 && diff(pn, nt) == 12) ||
              (dis == octs_all && (pn % 12 == nt % 12)) ||
              (octsigns && octchangeeq(p0, n, pn, nt, dis))) {
            pacc = module_fullacc(p0);
          }
        }
        p0 = module_peeknextnote(p0);
      }
      if (pacc != nacc)
        cautaccs_assign(n);
      else
        module_skipassign(n);
    }
  }

  const char* err_fun(void* moddata) {
    return 0;
  }

  const char* rangetype = "real>0";
  int valid_range(const struct module_value val) {
    //     struct module_value z;
    //     z.type = module_int;
    //     z.val.i = 0;
    return module_valid_num(val, module_makeval((fomus_int) 0), module_excl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            rangetype);
  } // also maxp
  const char* octstype = "none|one|all";
  int valid_octs_aux(const char* val) {
    return octsstrtoenum.find(val) != octsstrtoenum.end();
  }
  int valid_octs(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_octs_aux, octstype);
  }

  const char* scoretype = "real>=0";
  int valid_score(const struct module_value val) {
    //     module_value z;
    //     z.type = module_int;
    //     z.val.i = 0;
    return module_valid_num(val, module_makeval((fomus_int) 0), module_incl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            scoretype);
  }

  std::set<std::string> modstrset;
  std::string modstrtype;
  int valid_modstr_aux(const char* str) {
    return modstrset.find(str) != modstrset.end();
  }
  int valid_modstr(const struct module_value val) {
    return module_valid_string(val, 1, -1, valid_modstr_aux,
                               modstrtype.c_str());
  }

} // namespace cautaccs

using namespace cautaccs;

void module_fill_iface(void* moddata, void* iface) {
  ((dumb_iface*) iface)->moddata = moddata;
  ((dumb_iface*) iface)->run = run_fun;
  ((dumb_iface*) iface)->err = err_fun;
};
const char* module_longname() {
  return "Cautionary Accidentals";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Adds cautionary accidentals.";
}
void* module_newdata(FOMUS f) {
  return 0;
}
void module_freedata(void* dat) {}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modcautaccs;
}
const char* module_initerr() {
  return 0;
}
void module_init() {
  octsstrtoenum.insert(strtooctstype_val("one", octs_1));
  octsstrtoenum.insert(strtooctstype_val("all", octs_all));
  octsstrtoenum.insert(strtooctstype_val("none", octs_none));
}
void module_free() { /*assert(newcount == 0);*/
}
int module_engine_iface() {
  return ENGINE_INTERFACEID;
} // dumb
int module_itertype() {
  return module_bypart | module_byvoice | module_firsttied | module_norests;
} // notes aren't divided yet, so they reach across measures
// const char* module_engine(void* d) {return
// module_setting_sval(module_peeknextpart(0), enginemodid);}

// ------------------------------------------------------------------------------------------------------------------------

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "cautaccs-range"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "The maximum distance two events are allowed to be from "
                   "each other to create a cautionary accidental"
                   " (how distance is calculated depends on the the value of "
                   "`cautaccs-distmod')."
                   "  Increase this value to create more cautionary "
                   "accidentals that are farther apart."
        //"  Changing `cautaccs-distmod' also affects where cautionary
        //accidentals occur."
        ;
    set->typedoc = rangetype;

    module_setval_int(&set->val, 2);

    set->loc = module_locnote;
    set->valid = valid_range; // no range
    // set->validdeps = validdeps_minp;
    set->uselevel = 2;
    rangeid = id;
    break;
  }
  case 1: {
    set->name = "cautaccs-octs"; // docscat{accs}
    set->type = module_string;
    set->descdoc =
        "Whether or not cautionary accidentals are affected by notes in other "
        "octaves.  "
        "Set this in the score, a part, a measure definition, or in note "
        "events to affect which notes receive cautionary accidentals.  "
        "`none' means only notes in the same octave can create a cautionary "
        "accidental.  "
        "`one' means notes one octave apart can create one while `all' means "
        "notes in any octave can cause one.";
    set->typedoc = octstype; // same valid fun

    module_setval_string(&set->val, "none");

    set->loc = module_locnote;
    set->valid = valid_octs; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2;
    octdiffid = id;
    break;
  }
  case 2: {
    set->name = "cautaccs-octsigns"; // docscat{accs}
    set->type = module_bool;
    set->descdoc =
        "Whether or not cautionary accidentals are affected by octave signs."
        "Set this in the score, a part, a measure definition, or in note "
        "events to affect the impact of octave signs on cautionary "
        "accidentals.  "
        "  `yes' means cautionary accidentals can occur in places where notes "
        "appear on the same staff line but are actually different pitches due "
        "to octave signs."
        "  `no' means cautionary accidentals occur only when the notes are "
        "actually the same pitch (or whatever is specified in "
        "`cautaccs-octs').";
    // set->typedoc = minptype; // same valid fun

    module_setval_int(&set->val, 1);

    set->loc = module_locnote;
    // set->valid = valid_; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2;
    octsignsid = id;
    break;
  }
  case 3: {
    set->name = "cautaccs"; // docscat{accs}
    set->type = module_bool;
    set->descdoc =
        "Whether or not cautionary accidentals should appear in the score.  "
        "Set this to `yes' if you want cautionary accidentals.";
    // set->typedoc = minptype; // same valid fun

    module_setval_int(&set->val, 1);

    set->loc = module_locnote;
    // set->valid = valid_; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 1;
    allowedid = id;
    break;
  }
  case 4: {
    set->name = "cautaccs-octavedist"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "If the `cartdist' distance or similar module is selected, "
                   "this is the distance of one octave"
                   " (i.e., two notes an octave apart have a distance "
                   "equivalent to this value)."
                   "  A larger distance translates to a smaller weight in "
                   "decisions regarding cautionary accidentals and vice versa.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1); // TODO: reasonable default

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    octdistid = id;
  } break;
  case 5: {
    set->name = "cautaccs-beatdist"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "If the `cartdist' distance or similar module is selected, "
                   "this is the distance of one beat in a measure."
                   "  A larger distance translates to a smaller weight in "
                   "decisions regarding cautionary accidentals and vice versa.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1); // TODO: reasonable default

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    beatdistid = id;
  } break;
  case 6: {
    set->name = "cautaccs-distmod"; // docscat{accs}
    set->type = module_string;
    set->descdoc =
        "Module that is used to calculate the \"distance\" between notes."
        "  The closer two notes are to each other, the more important their "
        "relationship is in terms of note spelling, staff choice, voice "
        "assignments, etc.."
        //"  Since distance can be calculate in many different ways, there are
        //several interchangeable modules for this."
        "  Set this to change the algorithm used for calculating distance when "
        "making decisions regarding cautionary accidentals.";

    struct info_modfilter fi0 = {0, 0, 0, module_modaux, DIST_INTERFACEID};
    struct info_modfilterlist fi = {1, &fi0};
    const struct info_modlist ml(info_list_modules(&fi, 0, 0, -1));
    std::ostringstream s;
    for (const info_module *i = ml.mods, *ie = ml.mods + ml.n; i != ie; ++i) {
      modstrset.insert(i->name);
      if (i != ml.mods)
        s << '|';
      s << i->name;
    }
    modstrtype = s.str();
    set->typedoc = modstrtype.c_str();

    module_setval_string(&set->val, "notedist");

    set->loc = module_locnote; // !!! --create dist and select on the fly?
    set->valid = valid_modstr;
    set->uselevel = 3;
    distmodid = id;
  } break;
  default:
    return 0;
  }
  return 1;
}
void module_ready() {}
const char* module_engine(void* f) {
  return "dumb";
}
inline bool icmp(const module_obj a, const module_obj b, const int id) {
  return module_setting_ival(a, id) == module_setting_ival(a, id);
}
inline bool vcmp(const module_obj a, const module_obj b, const int id) {
  return module_setting_val(a, id) == module_setting_val(a, id);
}
inline bool scmp(const module_obj a, const module_obj b, const int id) {
  return strcmp(module_setting_sval(a, id), module_setting_sval(a, id)) == 0;
}
int module_sameinst(module_obj a, module_obj b) {
  return scmp(a, b, distmodid) && vcmp(a, b, octdistid) &&
         vcmp(a, b, beatdistid) && vcmp(a, b, rangeid);
}
