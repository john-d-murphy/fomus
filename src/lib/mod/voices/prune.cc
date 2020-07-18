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
#include <limits>
#include <list>
#include <map>
#include <queue>
#include <set>

#include <boost/ptr_container/ptr_deque.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include "ifacedumb.h"
#include "module.h"
#include "modutil.h"
#include "prune.h"

#include "debugaux.h"
#include "ilessaux.h"
using namespace ilessaux;

namespace prune {

  int prunetypeid, monoid;

  struct node {
    module_noteobj n;
    fomus_rat end;
    bool ok;
    modutil_rangesobj holes;
    bool isgr;
    fomus_rat pi;
    node(const module_noteobj n)
        : n(n), end(), ok(false), holes(ranges_init()), isgr(module_isgrace(n)),
          pi(module_isrest(n) ? module_makerat(-1, 1) : module_pitch(n)) {
      modutil_range x;
      if (isgr) {
        x.x1 = module_vgracetime(n);
        x.x2 = module_makeval(end = module_graceendtime(n));
      } else {
        x.x1 = module_vtime(n);
        x.x2 = module_makeval(end = module_endtime(n));
      }
      ranges_insert(holes, x);
    }
    ~node() {
      ranges_free(holes);
    }
    void remove(const modutil_range& r) {
      assert(r.x2 > r.x1);
      ranges_remove(holes, r);
      struct modutil_ranges x(ranges_get(holes));
      if (x.n > 0)
        end = GET_R((x.ranges + x.n - 1)->x2);
      else
        end = module_makerat(std::numeric_limits<fomus_int>::min() + 1, 1);
    }
    void insert(const modutil_range& r) {
      assert(r.x2 > r.x1);
      ranges_insert(holes, r);
      struct modutil_ranges x(ranges_get(holes));
      end = GET_R((x.ranges + x.n - 1)->x2);
    }
    bool isdirt(struct modutil_ranges& x) const {
      return (x.n != 1 || (isgr ? (x.ranges->x1 != module_gracetime(n) ||
                                   x.ranges->x2 != module_graceendtime(n))
                                : (x.ranges->x1 != module_time(n) ||
                                   x.ranges->x2 != module_endtime(n))));
    }
  };

  bool isspecial(const node& x, const node& y) {
    struct module_markslist m1(module_singlemarks(x.n));
    struct module_markslist m2(module_singlemarks(y.n));
    for (const module_markobj *i = m1.marks, *ie = m1.marks + m1.n; i < ie;
         ++i) {
      for (const module_markobj *j = m2.marks, *je = m2.marks + m2.n; j < je;
           ++j) {
        if (module_markisspecialpair(module_markid(*i), module_markid(*j)))
          return true;
      }
    }
    return false;
  }

  extern "C" {
  void prune_run_fun(FOMUS f, void* moddata); // return true when done
  const char* prune_err_fun(void* moddata);
  }

  enum prunetype { prune_cutoff, prune_breakup, prune_steal, prune_xfer };
  typedef std::map<std::string, prunetype, isiless> ptypemap;
  ptypemap ptypes;

  void blastassigns(boost::ptr_deque<node>& notes, const bool nlst) {
    while (!notes.empty()) {
      const node& no(notes.front());
      if (nlst && !no.ok)
        break;
      struct modutil_ranges x(ranges_get(no.holes));
      if (no.isdirt(x)) {
        for (modutil_range *i = x.ranges, *ie = x.ranges + x.n; i < ie; ++i) {
          DBG("PRUNE: " << module_time(no.n) << " - " << module_endtime(no.n)
                        << " to " << GET_R(i->x1) << " - " << GET_R(i->x2)
                        << std::endl);
          prune_assign(
              no.n, GET_R(i->x1),
              GET_R(i->x2)); // clips represent begins/ends of NEW notes
        }
        prune_assign_done(no.n);
      } else
        module_skipassign(no.n);
      notes.pop_front();
    }
  }

  // by voice
  // spanners are just begin + end--module removes multiple begins and ends,
  // adds new ones where necessary
  void prune_run_fun(FOMUS fom, void* moddata) {
    boost::ptr_deque<node> notes;
    std::list<node*> eoffs;
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n)
        break;
      node* nn;
      notes.push_back(nn = new node(n));
      modutil_range xx;
      if (nn->isgr) {
        xx.x1 = module_vgracetime(n);
        xx.x2 = module_vgraceendtime(n);
      } else {
        xx.x1 = module_vtime(n);
        xx.x2 = module_vendtime(n);
      }
      assert(ptypes.find(module_setting_sval(n, prunetypeid)) != ptypes.end());
      prunetype typ = ptypes.find(module_setting_sval(n, prunetypeid))->second;
      bool ismono = module_setting_ival(n, monoid);
      for (std::list<node*>::iterator i(eoffs.begin()); i != eoffs.end();) {
        if ((nn->isgr != (*i)->isgr) || (*i)->end <= xx.x1) {
          (*i)->ok = true; // ok to assign
          i = eoffs.erase(i);
        } else { // both are grace or notgrace
          if (ismono || (*i)->pi == nn->pi) {
            if (!isspecial(**i,
                           *nn)) { // if it's not a special overlapping pair...
              switch (typ) {
              case prune_cutoff: { // cutoff earlier note--only one that could
                                   // reduce the total duration
              CUTOFF:
                modutil_range yy = {xx.x1, module_makeval((*i)->end)};
                (*i)->remove(yy);
                break;
              }
              case prune_breakup: { // breakup earlier note
              BREAKUP:
                (*i)->remove(xx);
                break;
              }
              case prune_steal: { // steal from later note
              STEAL:
                modutil_range yy = {xx.x1, module_makeval((*i)->end)};
                nn->remove(yy);
                break;
              }
              case prune_xfer: {
                modutil_range yy = {xx.x1, module_makeval((*i)->end)};
                modutil_range yy2 = {module_makeval(nn->end),
                                     module_makeval((*i)->end)};
                (*i)->remove(yy);
                if (yy2.x2 > yy2.x1)
                  nn->insert(yy2);
                break;
              }
              }
            }
          } else if ((*i)->pi < (fomus_int) 0)
            goto STEAL; // second is a rest
          else if (nn->pi < (fomus_int) 0) {
            if (typ != prune_breakup)
              goto BREAKUP;
            else
              goto CUTOFF;
          } // first is a rest
          ++i;
        }
      }
      eoffs.push_back(nn);
      blastassigns(notes, true);
    }
    blastassigns(notes, false);
  }

  const char* prune_err_fun(void* moddata) {
    return 0;
  }

  void fill_iface(void* moddata, void* iface) {
    ((dumb_iface*) iface)->moddata = 0;
    ((dumb_iface*) iface)->run = prune_run_fun;
    ((dumb_iface*) iface)->err = prune_err_fun;
  };
  void init() {
    ptypes.insert(ptypemap::value_type("cutoff", prune_cutoff));
    ptypes.insert(ptypemap::value_type("split", prune_breakup));
    ptypes.insert(ptypemap::value_type("steal", prune_steal));
    ptypes.insert(ptypemap::value_type("transfer", prune_xfer));
  }

  // ------------------------------------------------------------------------------------------------------------------------

  const char* prunetype = "cutoff|transfer|split|steal";
  int valid_prune_aux(const char* val) {
    return ptypes.find(val) != ptypes.end();
  }
  int valid_prune(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_prune_aux, prunetype);
  }

  int get_setting(int n, module_setting* set, int id) {
    switch (n) {
    case 0: {
      set->name = "prune-type"; // docscat{voices}
      set->type = module_string;
      set->descdoc =
          "Notes of the same pitch that overlap in the same voice must be "
          "adjusted or \"pruned\" somehow.  This can be done in several "
          "different ways."
          "  `cutoff' specifies that the durations of earlier notes be \"cut\" "
          "to avoid conflicting with later notes."
          "  `transfer' preserves the total duration of two conflicting notes "
          "by "
          "cutting the earlier note and extending the duration of the later "
          "note "
          "if necessary."
          "  `split' splits notes into two parts if another conflicting note "
          "falls somewhere in the middle (i.e., an extra note might be "
          "created)."
          "  `steal' causes earlier notes to steal durations from any "
          "conflicting notes (i.e., the later notes are \"cut\").";
      set->typedoc = prunetype; // same valid fun

      module_setval_string(&set->val, "transfer");

      set->loc = module_locnote;
      set->valid = valid_prune; // no range
      // set->validdeps = validdeps_maxp;
      set->uselevel = 2;
      prunetypeid = id;
      break;
    }
    case 1: {
      set->name = "mono"; // docscat{voices}
      set->type = module_bool;
      set->descdoc =
          "Specifies whether or not notes are clipped when necessary "
          "to produce a monophonic line.  "
          "Set this to `yes' if you don't want any notes to overlap "
          "in the same voice.";
      // set->typedoc = prunetyp; // same valid fun

      module_setval_int(&set->val, 0);

      set->loc = module_locnote;
      // set->valid = valid_prune; // no range
      // set->validdeps = validdeps_maxp;
      set->uselevel = 2;
      monoid = id;
      break;
    }
    default:
      return 0;
    }
    return 1;
  }

} // namespace prune
