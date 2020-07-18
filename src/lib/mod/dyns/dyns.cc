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
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/utility.hpp> // next & prior

#include "ifacedumb.h"
#include "module.h"
#include "modutil.h"

#include "ilessaux.h"
using namespace ilessaux;
#include "debugaux.h"

namespace dyns {

  extern "C" {
  void run_fun(FOMUS f, void* moddata); // return true when done
  const char* err_fun(void* moddata);
  }

  int dynrangeid, dynsymrangeid, stickid, wedgeid, dodynid, maxrestid;

  std::map<std::string, fomus_float, isiless> symvals;
  enum module_markids dynmarks[12] = {
      mark_pppppp, mark_ppppp, mark_pppp, mark_ppp, mark_pp,  mark_p,
      mark_mp,     mark_mf,    mark_f,    mark_ff,  mark_fff, mark_ffff};
  std::map<int, int> dynids; // numbers are midpoints of dyn bins
  std::set<int> moredynids;
  std::set<int> evenmoreids;

  struct ass {
    module_noteobj n;
    // bool doit;
    std::vector<module_markids> mks;
    fomus_float d;
    bool dodyn;      // enter dynamic mark for this note
    fomus_int point; // used to find wedges
    bool nowedge;
    ass(const module_noteobj n, const fomus_float d, const bool dodyn)
        : n(n), d(d), dodyn(dodyn), point(-1), nowedge(false) {}
    void assign() const {
      if (!dodyn || mks.empty()) {
        module_skipassign(n);
      } else {
        for (std::vector<module_markids>::const_iterator i(mks.begin());
             i != mks.end(); ++i)
          marks_assign_add(n, *i, 0, module_makenullval());
        marks_assign_done(n);
      }
    }
  };

  template <typename V, typename T1, typename T2>
  inline fomus_float scale(const V& val, const T1& a, const T1& b, const T2& x,
                           const T2& y) {
    return x + (((val - a) / (b - a)) * (y - x));
  }

  inline void getnthf(const module_noteobj no, const int id, fomus_float& v1,
                      fomus_float& v2) {
    const module_value* x = module_setting_val(no, id).val.l.vals;
    v1 = GET_F(x[0]);
    v2 = GET_F(x[1]);
  }
  inline void getnths(const module_noteobj no, const int id, const char*& v1,
                      const char*& v2) {
    const module_value* x = module_setting_val(no, id).val.l.vals;
    v1 = x[0].val.s;
    v2 = x[1].val.s;
  }

  void run_fun(FOMUS f, void* moddata) {
    fomus_float dynstate;
    bool first = true;
    boost::ptr_deque<ass> deq;
    while (true) {
      module_noteobj n = module_nextnote();
      fomus_float mi, ma, mis, mas;
      bool fidr = true;
      if (n) {
        fomus_float lvl = GET_F(module_dyn(n));
        deq.push_back(new ass(n, lvl, module_setting_ival(n, dodynid)));
        getnthf(n, dynrangeid, mi, ma);
        const char *x1, *x2;
        getnths(n, dynsymrangeid, x1, x2);
        mis = symvals.find(x1)->second;
        mas = symvals.find(x2)->second + 1;
        assert(mis >= 0 && mis < 12);
        assert(mas >= 0 && mas < 12);
        struct module_markslist mks(module_singlemarks(n));
        for (const module_markobj *i(mks.marks), *ie(mks.marks + mks.n); i < ie;
             ++i) {
          int id = module_markbaseid(module_markid(*i));
          std::map<int, int>::const_iterator dy(dynids.find(id));
          if (dy != dynids.end()) {
            lvl = deq.back().d = scale(dy->second + 0.5, mis, mas, mi, ma);
            deq.back().point = dy->second;
            dynstate = lvl; // 76
            goto SKIP;
          } else if (moredynids.find(id) !=
                     moredynids
                         .end()) { // ids that don't signify permanent change
            deq.back().nowedge = true;
            fidr = false; // dont' reset "first" yet! dynstate cuodl be invalid
            goto SKIP;
          } else if (evenmoreids.find(id) !=
                     evenmoreids.end()) { // something that would cancel a wedge
            deq.back().nowedge = true;
            dynstate = lvl;
            goto SKIP;
          }
        }
        if (first) {
          int i = (int) scale(lvl, mi, ma, mis, mas);
          if (i < 0)
            i = 0;
          if (i >= 12)
            i = 11;
          deq.back().mks.push_back(dynmarks[i]); // set the mark
          deq.back().point = i;
        } else {
          int i = scale(dynstate, mi, ma, mis, mas);
          if (i < 0)
            i = 0;
          if (i >= 12)
            i = 11;
          fomus_float j0 = scale(lvl, mi, ma, mis, mas);
          if (j0 < 0)
            j0 = 0;
          if (j0 > 12)
            j0 = 12;
          fomus_float sti = module_setting_fval(n, stickid);
          if (j0 > i - sti && j0 < (i + 1) + sti)
            goto SKIP; // don't change the dyn bin#
          // scan forwards for another dyn mark
          int j = (int) j0;
          if (j < 0)
            j = 0;
          if (j >= 12)
            j = 11;
          deq.back().mks.push_back(dynmarks[j]); // set the mark
          deq.back().point = j;
        }
        dynstate = lvl;
      } else if (first)
        break;
    SKIP:
      std::deque<boost::ptr_deque<ass>::iterator> pts;
      int dir = 0;
      for (boost::ptr_deque<ass>::iterator i(deq.begin()); i != deq.end();
           ++i) {
        if (i->point >= 0) {
          if (pts.empty()) {
            pts.push_back(i);
            continue;
          }
          if ((dir > 0 && i->point > pts.back()->point) ||
              (dir < 0 && i->point < pts.back()->point) ||
              (dir == 0 &&
               i->point == pts.back()->point)) { // continuing in same
                                                 // direction, point is an int
            pts.back() = i;
          } else {
            dir = (i->point > pts.back()->point
                       ? 1
                       : (i->point < pts.back()->point ? -1 : 0));
            pts.push_back(i);
          }
        }
      }
      while (
          pts.size() >=
          (n ? 3
             : 2)) { // need three points (except at end) -- use only first two
        boost::ptr_deque<ass>::iterator be(pts[0]);
        boost::ptr_deque<ass>::iterator l(pts[1]);
        fomus_float w = module_setting_fval(be->n, wedgeid);
        if (w > 0) { // 1/9/12 was w >= 0
          fomus_int sz = 0;
          fomus_float dif = 0;
          fomus_rat t1(module_time(be->n)), t2(module_time(l->n));
          fomus_float v1 = scale(be->d, mi, ma, mis, mas),
                      v2 = scale(l->d, mi, ma, mis, mas);
          DBG(" looking for wedge at " << t1 << " - " << t2 << std::endl);
          fomus_rat et(module_endtime(be->n));
          for (boost::ptr_deque<ass>::iterator i(be); i <= l; ++i) {
            fomus_rat t(module_time(i->n));
            if (i->nowedge || t - et > module_setting_rval(i->n, maxrestid))
              goto NOWEDGE;
            et = module_endtime(i->n);
            if (t > t1 && t < t2) {
              ++sz;
              dif += fabs(scale(i->d, mi, ma, mis, mas) -
                          scale(t, t1, t2, v1, v2));
            }
          }
          if (sz > 0 && dif / sz <= w) {
            assert(be < l);
            for (boost::ptr_deque<ass>::iterator i(boost::next(be)); i != l;
                 ++i)
              i->mks.clear();
            if (be->d < l->d) {
              be->mks.push_back(mark_cresc_begin);
              l->mks.push_back(mark_cresc_end);
            } else if (be->d > l->d) {
              be->mks.push_back(mark_dim_begin);
              l->mks.push_back(mark_dim_end);
            }
          }
        }
      NOWEDGE:
        while (deq.begin() != l) {
          deq.front().assign();
          deq.pop_front();
        }
        pts.pop_front();
      }
      if (!n)
        break;
      ;
      if (fidr)
        first = false;
    }
    while (!deq.empty()) {
      deq.front().assign();
      deq.pop_front();
    }
  }

  const char* err_fun(void* moddata) {
    return 0;
  }

  const char* dynsymrangetype =
      "(string_dynsym string_dynsym), string_dynsym = "
      "pppppp|ppppp|pppp|ppp|pp|p|mp|mf|f|ff|fff|ffff";
  int valid_dynsymrange_aux(int n, const char* str) {
    return symvals.find(str) != symvals.end();
  }
  int valid_dynsymrange(const struct module_value val) {
    return module_valid_listofstrings(val, 2, 2, -1, -1, valid_dynsymrange_aux,
                                      dynsymrangetype);
  }

  const char* dynsticktype = "number0..1/2";
  int valid_dynstick(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) 0), module_incl,
                            module_makeval(module_makerat(1, 2)), module_incl,
                            0, dynsticktype);
  }

  const char* restmaxtype = "rational>0";
  int valid_restmax(const struct module_value val) {
    return module_valid_rat(val, module_makerat(0, 1), module_excl,
                            module_makerat(0, 1), module_nobound, 0,
                            restmaxtype);
  }

  const char* dynrangetype = "(number number)";
  int valid_dynrange(const struct module_value val) {
    return module_valid_listofnums(
        val, 2, 2, module_makeval((fomus_int) 0), module_nobound,
        module_makeval((fomus_int) 0), module_nobound, 0, dynrangetype);
  }

} // namespace dyns

using namespace dyns;

const char* module_engine(void* d) {
  return "dumb";
}
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
  return "Dynamics";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Converts dynamic values to appropriate score symbols.";
}
void* module_newdata(FOMUS f) {
  return 0;
} // new accsdata;}
void module_freedata(void* dat) {} // delete (accsdata*)dat;}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_moddynamics;
}
int module_itertype() {
  return module_bypart | module_byvoice | module_norests;
}
const char* module_initerr() {
  return 0;
}
void module_init() {
  moredynids.insert(mark_sf);
  moredynids.insert(mark_sff);
  moredynids.insert(mark_sfff);
  moredynids.insert(mark_sfz);
  moredynids.insert(mark_sffz);
  moredynids.insert(mark_sfffz);
  moredynids.insert(mark_fz);
  moredynids.insert(mark_ffz);
  moredynids.insert(mark_fffz);
  moredynids.insert(mark_rfz);
  moredynids.insert(mark_rf);
  dynids.insert(std::map<int, fomus_float>::value_type(mark_pppppp, 0));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_ppppp, 1));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_pppp, 2));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_ppp, 3));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_pp, 4));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_p, 5));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_mp, 6));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_ffff, 11));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_fff, 10));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_ff, 9));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_f, 8));
  dynids.insert(std::map<int, fomus_float>::value_type(mark_mf, 7));
  symvals.insert(std::map<std::string, fomus_float>::value_type("pppppp", 0));
  symvals.insert(std::map<std::string, fomus_float>::value_type("ppppp", 1));
  symvals.insert(std::map<std::string, fomus_float>::value_type("pppp", 2));
  symvals.insert(std::map<std::string, fomus_float>::value_type("ppp", 3));
  symvals.insert(std::map<std::string, fomus_float>::value_type("pp", 4));
  symvals.insert(std::map<std::string, fomus_float>::value_type("p", 5));
  symvals.insert(std::map<std::string, fomus_float>::value_type("mp", 6));
  symvals.insert(std::map<std::string, fomus_float>::value_type("ffff", 11));
  symvals.insert(std::map<std::string, fomus_float>::value_type("fff", 10));
  symvals.insert(std::map<std::string, fomus_float>::value_type("ff", 9));
  symvals.insert(std::map<std::string, fomus_float>::value_type("f", 8));
  symvals.insert(std::map<std::string, fomus_float>::value_type("mf", 7));
  evenmoreids.insert(mark_cresc_begin);
  evenmoreids.insert(mark_cresc_end);
  evenmoreids.insert(mark_dim_begin);
  evenmoreids.insert(mark_dim_end);
}
void module_free() { /*assert(newcount == 0);*/
}
int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "dyn-range"; // docscat{dyns}
    set->type = module_list_nums;
    set->descdoc = "The range of dynamic input values for a note event.  "
                   "Set this to a list of two numbers representing the minimum "
                   "and maximum dynamic values expected in note events."
                   "  Dynamic values are scaled/translated to dynamic text "
                   "markings by means of the setting `dynsym-range'.";
    set->typedoc = dynrangetype;

    module_setval_list(&set->val, 2);
    module_setval_int(set->val.val.l.vals, 0);
    module_setval_int(set->val.val.l.vals + 1, 1);

    set->loc = module_locnote;
    set->valid = valid_dynrange;
    set->uselevel = 1;
    dynrangeid = id;
  } break;
  case 1: {
    set->name = "dynsym-range"; // docscat{dyns}
    set->type = module_list_strings;
    set->descdoc =
        "The range of dynamic symbols that a dynamic input value translates "
        "to.  "
        "Set this to a list of two dynamic text symbols representing the "
        "minimum and maximum dynamic levels allowed in the score."
        //"  Use this together with `maxdynsym'."
        "  Options range from `pppppp' to `ffff'.";
    set->typedoc = dynsymrangetype;

    module_setval_list(&set->val, 2);
    module_setval_string(set->val.val.l.vals, "ppp");
    module_setval_string(set->val.val.l.vals + 1, "ff");

    set->loc = module_locnote;
    set->valid = valid_dynsymrange;
    set->uselevel = 2;
    dynsymrangeid = id;
  } break;
  case 2: {
    set->name = "dyn-sticky"; // docscat{dyns}
    set->type = module_number;
    set->descdoc =
        "Affects how dynamic input values are translated into dynamic text "
        "symbols.  "
        "If `dyn-sticky' is set to 0, dynamic values in the range `mindyn' to "
        "`maxdyn' are scaled and translated directly to "
        "dynamic marking symbols (the input values are effectively quantized "
        "to the given range of symbols).  "
        "A potential problem is that if values often change close to the "
        "borders between neighboring dynamic symbols, then the score could "
        "end up containing dynamic symbols that fluctuate constantly.  "
        "Increasing `dyn-stick' to a value like 1/3 or 1/2 effectively "
        "increases the amount the dynamic level must change before "
        "FOMUS switches to another dynamic symbol.  "
        "For example, 1/3 means the dynamic level must move at least one-third "
        "of the way into a neighboring dynamic symbol's range before that "
        "symbol is displayed.";
    set->typedoc = dynsticktype;

    module_setval_rat(&set->val, module_makerat(1, 3));

    set->loc = module_locnote;
    set->valid = valid_dynstick;
    set->uselevel = 2;
    stickid = id;
  } break;
  case 3: {
    set->name = "dyn-wedge"; // docscat{dyns}
    set->type = module_number;
    set->descdoc =
        "The likelihood of a crescendo/diminuendo wedge being created between "
        "two dynamic markings when translating dynamic input values into text "
        "symbols.  "
        "Increase this value to increase the likelihood of inserting a "
        "crescendo/diminuendo wedge between dynamic markings."
        "  For a span of notes to qualify for a wedge, the dynamic levels "
        "between them must gradually increase or decrease."
        "  This setting designates the amount of error allowed when comparing "
        "the levels to a straight line between two beginning and ending values."
        "  A value of 0 means no wedges are created.";
    set->typedoc = dynsticktype;

    module_setval_rat(&set->val, module_makerat(1, 2));

    set->loc = module_locnote;
    set->valid = valid_dynstick;
    set->uselevel = 2;
    wedgeid = id;
  } break;
  case 4: {
    set->name = "dyns"; // docscat{dyns}
    set->type = module_bool;
    set->descdoc = "Whether or not to translate dynamic input values into "
                   "dynamic text markings.  "
                   "Set this to `yes' if you want dynamic values that are "
                   "entered along with other note event parameters to be "
                   "converted to dynamic text symbols.";
    // set->typedoc = dynnumtype;

    module_setval_int(&set->val, 0);

    set->loc = module_locnote;
    // set->valid = valid_dynnum;
    set->uselevel = 1;
    dodynid = id;
  } break;
  case 5: {
    set->name = "dyn-wedge-maxrestdur"; // docscat{dyns}
    set->type = module_number;
    set->descdoc =
        "This is the maximum total rest duration that a wedge is allowed to "
        "pass through when creating crescendo/diminuendo wedges from dynamic "
        "input values.  "
        "Higher values decrease the possibility of wedges being inserted in "
        "passages containing rests.  "
        "Setting it to 0 prevents wedges from passing through rests at all.";
    set->typedoc = restmaxtype;

    module_setval_rat(&set->val, module_makerat(1, 2));

    set->loc = module_locnote;
    set->valid = valid_restmax;
    set->uselevel = 2;
    maxrestid = id;
  } break;
  default:
    return 0;
  }
  return 1;
}
void module_ready() {}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
