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
#include <limits>
#include <map>
#include <set>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "ifacedumb.h"
#include "module.h"
#include "modutil.h"

namespace tpose {

  const char* ierr = 0;

  int transpose, doit, keysigid, majmodesid, minmodesid, allkeysid, tposekeysit;

  extern "C" {
  void dumb_run(FOMUS fom, void* moddata); // return true when done
  const char* dumb_err(void* moddata);
  }

  struct akeysig {
    std::vector<fomus_rat> arr; // pitch classes of accidentals
    std::set<fomus_rat> srt;
    fomus_rat acccnt;
    akeysig(const std::vector<fomus_rat>& arr0) : acccnt(module_makerat(0, 1)) {
      assert(arr0.size() == 7);
      // for (std::vector<fomus_rat>::const_iterator i(arr0.begin()); i !=
      // arr0.end(); ++i) {
      for (int i = 0; i < 7; ++i) {
        fomus_rat x(mod(arr0[i], module_makerat(12, 1)));
        acccnt = acccnt + (arr0[i] - tochromatic(i));
        arr.push_back(x);
        srt.insert(x);
      }
    }
    akeysig(const std::vector<fomus_rat>& arr0, const fomus_rat& tp)
        : acccnt(module_makerat(0, 1)) {
      assert(arr0.size() == 7);
      // for (std::vector<fomus_rat>::const_iterator i(arr0.begin()); i !=
      // arr0.end(); ++i) {
      for (int i = 0; i < 7; ++i) {
        fomus_rat x(mod(arr0[i] - tp, module_makerat(12, 1)));
        acccnt = acccnt + (arr0[i] - tochromatic(i));
        arr.push_back(x);
        srt.insert(x);
      }
    }
  };
  struct akeysigref {
    const akeysig& a;
    akeysigref(const akeysig& a) : a(a) {}
  };
  const bool operator==(const akeysig& x, const akeysig& y) {
    return std::equal(x.srt.begin(), x.srt.end(), y.srt.begin());
  }
  const bool operator<(const akeysig& x, const akeysig& y) {
    return std::lexicographical_compare(x.srt.begin(), x.srt.end(),
                                        y.srt.begin(), y.srt.end());
  }
  const bool operator==(const akeysigref& x, const akeysigref& y) {
    return std::equal(x.a.srt.begin(), x.a.srt.end(), y.a.srt.begin());
  }
  const bool operator<(const akeysigref& x, const akeysigref& y) {
    return std::lexicographical_compare(x.a.srt.begin(), x.a.srt.end(),
                                        y.a.srt.begin(), y.a.srt.end());
  }

  bool isthemode(const char* str, const module_value& m) {
    assert(m.type == module_list);
    for (module_value *i = m.val.l.vals, *ie = m.val.l.vals + m.val.l.n; i < ie;
         ++i) {
      assert(i->type == module_string);
      if (boost::algorithm::ends_with(str, i->val.s))
        return true;
    }
    return false;
  }

  void dumb_run(FOMUS fom, void* moddata) {
    while (true) {
      module_measobj m = module_nextmeas();
      if (!m)
        break;
      bool tpk =
          module_setting_ival(m, tposekeysit) && module_setting_ival(m, doit);
      if (tpk) {
        module_value majmodes(module_setting_val(fom, majmodesid)),
            minmodes(module_setting_val(fom, minmodesid));
        boost::ptr_map<const std::string, akeysig> varsigsmaj, varsigsmin;
        // get all std keysigs...
        const module_value comm(module_setting_val(fom, allkeysid));
        assert(comm.type == module_list);
        bool wh = true;
        const char* nam;
        for (const module_value *i = comm.val.l.vals,
                                *ie = comm.val.l.vals + comm.val.l.n;
             i < ie; ++i, wh = !wh) {
          if (wh) {
            assert(i->type == module_string);
            nam = i->val.s;
          } else {
            assert(i->type == module_list);
            bool ismaj = isthemode(nam, majmodes);
            if (!ismaj && !isthemode(nam, minmodes))
              continue;
            std::vector<fomus_rat> nts;
            for (int i0 = 0; i0 < 7; ++i0)
              nts.push_back(module_makerat(tochromatic(i0), 1));
            for (const module_value *j = i->val.l.vals,
                                    *je = i->val.l.vals + i->val.l.n;
                 j < je; ++j) {
              assert(j->type == module_string);
              module_noteparts x;
              x.note = x.acc1 = x.acc2 =
                  module_makerat(std::numeric_limits<fomus_int>::max(), 1);
              fomus_rat pp(module_strtonote(j->val.s, &x));
              if (pp.num != std::numeric_limits<fomus_int>::max() &&
                  iswhite(x.note)) {
                fomus_rat a;
                if (x.acc1.num != std::numeric_limits<fomus_int>::max())
                  a = x.acc1;
                else
                  a = module_makerat(0, 1);
                if (x.acc2.num != std::numeric_limits<fomus_int>::max())
                  a = a + x.acc2;
                nts[mod(todiatonic(x.note.num), (fomus_int) 7)] = x.note + a;
              }
            }
            if (ismaj) {
              varsigsmaj.insert(nam, new akeysig(nts));
            } else {
              varsigsmin.insert(nam, new akeysig(nts));
            }
          }
        }
        fomus_rat tp(module_setting_rval(m, transpose));
        if (tp != (fomus_int) 0) {
          const char* ks = module_setting_sval(m, keysigid);
          if (ks[0]) { // not 0 len, have a commno keysig
            bool ismaj = isthemode(ks, majmodes);
            if (ismaj || isthemode(ks, minmodes)) {
              const boost::ptr_map<const std::string, akeysig>& varsigs =
                  (ismaj ? varsigsmaj : varsigsmin);
              std::multimap<const akeysigref, std::string>
                  kysgs; // vectors of pitch classes, for matching
              for (boost::ptr_map<const std::string, akeysig>::const_iterator z(
                       varsigs.begin());
                   z != varsigs.end(); ++z)
                kysgs.insert(
                    std::multimap<const akeysigref, std::string>::value_type(
                        *z->second, z->first));
              boost::ptr_map<const std::string, akeysig>::const_iterator x(
                  varsigs.find(ks));
              if (x != varsigs.end()) {
                akeysig xx(x->second->arr, tp);
                std::multimap<akeysigref, std::string>::const_iterator m0 =
                    kysgs.end();
                fomus_rat sc = {std::numeric_limits<fomus_int>::max(), 1};
                for (std::multimap<akeysigref, std::string>::const_iterator i(
                         kysgs.lower_bound(akeysig(xx)));
                     i != kysgs.end() && i->first == akeysigref(xx);
                     ++i) { // candidates
                  if (m0 != kysgs.end()) {
                    fomus_rat d(diff(i->first.a.acccnt, m0->first.a.acccnt));
                    if (d < sc) {
                      sc = d;
                      m0 = i;
                    }
                  } else
                    m0 = i;
                }
                if (m0 != kysgs.end())
                  tpose_assign_keysig(
                      m,
                      m0->second
                          .c_str()); // assign new key signature for measure
              }
            }
          }
        }
      }
    }
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n)
        break;
      if (module_setting_ival(n, doit)) {
        tpose_assign(n, module_pitch(n) - module_setting_rval(n, transpose));
      } else
        module_skipassign(n);
    }
  }
  const char* dumb_err(void* moddata) {
    return 0;
  }

  const char* tposetype = "rational-128..128";
  int valid_tpose(const struct module_value val) {
    return module_valid_rat(val, module_makerat(-128, 1), module_incl,
                            module_makerat(128, 1), module_incl, 0, tposetype);
  } // also maxp
  // int valid_vertmax (const struct module_value* val) {return
  // module_valid_rat(val, 0, module_incl, 1, module_nobound, 0, vertmaxtype);}
  // const char* staffoctuptype = "yes|no";
  // int valid_staffoctup (const struct module_value* val) {return
  // module_valid_int(val, 0, module_incl, 128, module_excl, 0,
  // midiintracktype);}

} // namespace tpose

using namespace tpose;

void module_fill_iface(void* moddata, void* iface) { // module
  ((dumb_iface*) iface)->moddata = 0;
  ((dumb_iface*) iface)->run = dumb_run;
  ((dumb_iface*) iface)->err = dumb_err;
}
int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
const char* module_longname() {
  return "Transpose Parts";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Automatically transposes parts according to instrument settings.";
}

void* module_newdata(FOMUS f) {
  return 0 /*new tposedata*/;
}
void module_freedata(void* dat) { /*delete (tposedata*)dat*/
  ;
}
// const char* module_err(void* dat) {return 0;}

int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modtpose;
}
const char* module_initerr() {
  return ierr;
}
int module_itertype() {
  return module_bypart | module_norests | module_noperc;
}

void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}

// ------------------------------------------------------------------------------------------------------------------------

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "transpose"; // docscat{instsparts}
    set->type = module_rat;
    set->descdoc =
        "An instrument's transposition in semitones, used to automatically "
        "transpose parts.  "
        "Set this in an instrument definition to an appropriate value (e.g., a "
        "B-flat clarinet would require a value of -2).";
    set->typedoc = tposetype;

    module_setval_int(&set->val, 0);

    set->loc = module_locnote;
    set->valid = valid_tpose;
    set->uselevel = 2;
    transpose = id;
    break;
  }
  case 1: {
    set->name = "transpose-part"; // docscat{instsparts}
    set->type = module_bool;
    set->descdoc = "Whether or not to transpose a part.  "
                   "Set this to `yes' at the score level if you want a "
                   "transposed score or `no' if you want a \"C\" score.";
    // set->typedoc = tposetype;

    module_setval_int(&set->val, 1);

    set->loc = module_locnote;
    // set->valid = v; // no range
    set->uselevel = 1;
    doit = id;
    break;
  }
  case 2: {
    set->name = "transpose-keysigs"; // docscat{instsparts}
    set->type = module_bool;
    set->descdoc =
        "Whether or not to transpose key signatures along with notes.  "
        "Set this to `yes' if key signatures must change to reflect "
        "transpositions or `no' if key signatures in the score must be exactly "
        "the ones specified on input.";
    // set->typedoc = tposetype;

    module_setval_int(&set->val, 1);

    set->loc = module_locmeasdef;
    // set->valid = v; // no range
    set->uselevel = 2;
    tposekeysit = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}

const char* module_engine(void*) {
  return "dumb";
}
void module_ready() {
  majmodesid = module_settingid("keysig-major-symbol");
  if (majmodesid < 0) {
    ierr = "missing required setting `keysig-major-symbol'";
    return;
  }
  minmodesid = module_settingid("keysig-minor-symbol");
  if (minmodesid < 0) {
    ierr = "missing required setting `keysig-minor-symbol'";
    return;
  }
  allkeysid = module_settingid("keysig-defs");
  if (allkeysid < 0) {
    ierr = "missing required setting `keysig-defs'";
    return;
  }
  keysigid = module_settingid("keysig");
  if (keysigid < 0) {
    ierr = "missing required setting `keysig'";
    return;
  }
}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
