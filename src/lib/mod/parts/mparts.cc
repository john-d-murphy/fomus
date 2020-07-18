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
#include <set>

#include <boost/ptr_container/ptr_deque.hpp>

#include "ifacedumb.h"
#include "module.h"

#include "debugaux.h"

namespace mparts {

  int fromvoiceid, tovoiceid, tposeid;

  extern "C" {
  void mparts_run_fun(FOMUS f, void* moddata); // return true when done
  const char* mparts_err_fun(void* moddata);
  }

  struct cirscope {
    std::set<module_partobj>& cir;
    std::set<module_partobj>::iterator i;
    cirscope(const module_partobj p, std::set<module_partobj>& cir)
        : cir(cir), i(cir.insert(p).first) {}
    ~cirscope() {
      cir.erase(i);
    }
  };
  void dumpstuff(const module_partobj p, const module_noteobj n,
                 std::set<module_partobj>& cir, const int v0,
                 const fomus_rat& no) {
    static module_value frto0 = module_makeval((fomus_int) 0);
    static struct module_list fr0 = {1, &frto0};
    static struct module_list to0 = {1, &frto0};
    if (cir.find(p) != cir.end())
      return;
    cirscope xxx(p, cir);
    metaparts_partmaps m(metaparts_getpartmaps(p));
    for (const metaparts_partmapobj *i(m.partmaps), *ie(m.partmaps + m.n);
         i < ie; ++i) {
      module_partobj np = metaparts_partmappart(*i);
      bool ism = module_ismetapart(np);
      fomus_rat tpos = module_setting_rval(*i, tposeid);
      struct module_list fr(module_setting_val(*i, fromvoiceid).val.l);
      if (!fr.n)
        fr = fr0;
      struct module_list to(module_setting_val(*i, tovoiceid).val.l);
      if (!to.n)
        to = to0;
      int mx = std::max(fr.n, to.n);
      for (int i = 0; i < mx; ++i) {
        int f = GET_I(fr.vals[i % fr.n]);
        int t = GET_I(to.vals[i % to.n]);
        if (!t)
          t = v0;
        if (!f || f == v0) {
          if (ism) {
            dumpstuff(
                np, n, cir, t,
                (no >= (fomus_int) 0 ? no + tpos : module_makerat(-1, 1)));
          } else { // got to a real part
            metaparts_assign(
                n, np, t,
                (no >= (fomus_int) 0 ? no + tpos : module_makerat(-1, 1)));
          }
        }
      }
    }
  }

  void mparts_run_fun(FOMUS fom, void* moddata) {
    module_noteobj n = module_nextnote();
    while (true) {
      module_partobj p = module_nextpart();
      if (!p)
        break;
      if (module_ismetapart(p)) {
        module_noteobj n0 = n;
        while (n0 && module_part(n0) == p) {
          std::set<module_partobj> cir;
          dumpstuff(
              p, n0, cir, module_voice(n0),
              (module_isrest(n0) ? module_makerat(-1, 1) : module_pitch(n0)));
          // metaparts_assign_done(n);
          n0 = module_peeknextnote(n0);
        }
        module_objlist mm0(module_getmarkevlist(p));
        for (const module_markobj *i(mm0.objs), *ie(mm0.objs + mm0.n); i != ie;
             ++i) {
          struct module_intslist vs(module_voices(*i));
          assert(vs.n > 0);
          for (const int *v(vs.ints), *ve = (vs.ints + vs.n); v < ve; ++v) {
            std::set<module_partobj> cir;
            dumpstuff(p, *i, cir, *v, module_makerat(60, 1));
          }
        }
      }
      while (n && module_part(n) == p) {
        module_skipassign(n);
        n = module_nextnote();
      }
    }
  }

  const char* mparts_err_fun(void* moddata) {
    return 0;
  }

  void fill_iface(void* moddata, void* iface) {
    ((dumb_iface*) iface)->moddata = 0;
    ((dumb_iface*) iface)->run = mparts_run_fun;
    ((dumb_iface*) iface)->err = mparts_err_fun;
  };

  const char* tofromvoicetype =
      "integer1..128 | (integer1..128 integer1..128 ...)";
  int valid_tofromvoice(struct module_value val) {
    return module_valid_listofints(val, -1, -1, 1, module_incl, 128,
                                   module_incl, 0, tofromvoicetype);
  }
  const char* tposetype = "rational-128..128";
  int valid_tpose(const struct module_value val) {
    return module_valid_rat(val, module_makerat(-128, 1), module_incl,
                            module_makerat(128, 1), module_incl, 0, tposetype);
  } // also maxp

  // ------------------------------------------------------------------------------------------------------------------------

  int get_setting(int n, module_setting* set, int id) {
    switch (n) {
    case 0: {
      set->name = "from-voice"; // docscat{instsparts}
      set->type = module_list_nums;
      set->descdoc =
          "Specifies the voice or voices that note events are copied from in a "
          "metapart.  Use in conjunction with `to-voice' to specify "
          "a complete distribution rule (i.e., note events must be moved from "
          "one voice or group of voices to another).  Can be a single voice or "
          "a "
          "list of voices."
          "  If both `from-voice' and `to-voice' are lists, then the two lists "
          "must be the same size so that the "
          "\"from\" and \"to\" voices match.";
      set->typedoc = tofromvoicetype;

      module_setval_list(&set->val, 0);

      set->loc = module_locpartmap;
      set->valid = valid_tofromvoice; // no range
      set->uselevel = 2;
      fromvoiceid = id;
      break;
    }
    case 1: {
      set->name = "to-voice"; // docscat{instsparts}
      set->type = module_list_nums;
      set->descdoc =
          "Specifies the voice or voices that note events are copied to in a "
          "metapart.  Use in conjunction with `from-voice' to specify "
          "a complete distribution rule (i.e., note events must be moved from "
          "one voice or group of voices to another).  Can be a single voice or "
          "a "
          "list of voices."
          "  If both `from-voice' and `to-voice' are lists, then the two lists "
          "must be the same size so that the "
          "\"from\" and \"to\" voices match.";
      set->typedoc = tofromvoicetype;

      module_setval_list(&set->val, 0);

      set->loc = module_locpartmap;
      set->valid = valid_tofromvoice; // no range
      set->uselevel = 2;
      tovoiceid = id;
      break;
    }
    case 2: {
      set->name = "partmap-tpose"; // docscat{instsparts}
      set->type = module_rat;
      set->descdoc = "Specifies a transposition in semitones to apply when "
                     "distributing note events from a metapart.";
      set->typedoc = tposetype;

      module_setval_int(&set->val, 0);

      set->loc = module_locpartmap;
      set->valid = valid_tpose; // no range
      set->uselevel = 2;
      tposeid = id;
      break;
    }
    default:
      return 0;
    }
    return 1;
  }

} // namespace mparts
