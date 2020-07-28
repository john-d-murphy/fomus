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

#include <algorithm> // transform, copy
#include <cassert>
#include <cmath>
#include <cstring>
#include <deque>
#include <iterator> // back_inserter
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/utility.hpp> // next & prior

#include "ifacedist.h"
#include "ifacesearch.h"
#include "infoapi.h"
#include "module.h"
#include "modutil.h"

#include "debugaux.h"

namespace staves {

  const char* ierr = 0;

  int upllsid, downllsid, distmodid, octdistid, beatdistid, properclefid,
      cleforderid, pitchorderid, clefchangeid, staffchangeid, voicemaxid,
      /*octupid, octdownid,*/ exponid, enginemodid, rangeid,
      heapsizeid /*, defclefid*/, preferredid, clefprefid, highlowclefid,
      highlowstaffid, staffid, clefid, balanceid /*, vertbalanceid*/;

#ifndef NDEBUG
  inline fomus_float testgtzero(const fomus_float sc) {
    assert(sc >= 0);
    return sc;
  }
#else
#define testgtzero(xxx) (xxx)
#endif

  struct stavesnode _NONCOPYABLE {
    module_noteobj note;
    int uplls, downlls; //, octup, octdown;
    int clef, staff, voice;
    int midpitch;
    fomus_float expon; // distance exp base (expon ^ -dist = multiplier)
    fomus_float properclefpenalty, cleforderpenalty, pitchorderpenalty,
        clefchangepenalty, staffchangepenalty, voicemaxpenalty, clefprefpenalty,
        highlowclefpenalty, highlowstaffpenalty,
        balancepenalty /*, vertbalancepenalty*/; //, defclefpenalty;
#ifndef NDEBUG
    int valid;
#endif
    int defcl;
    fomus_rat no, ti /*, teti*/;
    fomus_int wno;
    stavesnode(const module_noteobj note, const int staff, const int clef,
               const module_partobj prt, const fomus_float totclefpref)
        : note(note), clef(clef), staff(staff), voice(module_voice(note)),
          midpitch(todiatonic(module_clefmidpitch(clef))),
#ifndef NDEBUG
          valid(12345),
#endif
          defcl(module_strtoclef(module_staffclef(prt, staff))),
          no(module_pitch(note)), ti(module_time(note)),
          /*teti(module_tiedendtime(note)),*/ wno(module_writtennote(note)) {
      module_clefobj cl = staves_clef(prt, staff, clef);
      uplls = midpitch + module_setting_ival(cl, upllsid) * 2 +
              5; // add these to mid written pitch to get max/min writ pitch
      downlls = midpitch - (module_setting_ival(cl, downllsid) * 2 + 5);
      properclefpenalty = module_setting_fval(note, properclefid);
      cleforderpenalty = module_setting_fval(note, cleforderid);
      pitchorderpenalty = module_setting_fval(note, pitchorderid);
      clefchangepenalty = module_setting_fval(note, clefchangeid);
      staffchangepenalty = module_setting_fval(note, staffchangeid);
      voicemaxpenalty = module_setting_fval(note, voicemaxid);
      clefprefpenalty = testgtzero(module_setting_fval(note, preferredid)) *
                        (testgtzero(totclefpref) /
                         testgtzero(module_setting_fval(cl, clefprefid)));
      assert(clefprefpenalty >= 0);
      highlowclefpenalty = module_setting_fval(note, highlowclefid);
      highlowstaffpenalty = module_setting_fval(note, highlowstaffid);
      balancepenalty = module_setting_fval(note, balanceid);
      // vertbalancepenalty = module_setting_fval(note, vertbalanceid);
      expon = pow(2, -1 / module_setting_fval(note, exponid));
    }
#ifndef NDEBUG
    bool isvalid() const {
      return valid == 12345;
    }
#endif
  };

  struct scorenode {
    const stavesnode*
        node; // pointer to keep boost::pool_allocator from bitching
    fomus_float dist;
    bool staffch, clefch;
    scorenode(const stavesnode* node, const fomus_float dist, const int st,
              const int cl)
        : node(node), dist(dist), staffch(st), clefch(cl) {
      assert(node->isvalid());
    }
  };

  // is clef the default for that staff?
  inline fomus_float
  defaultclef(const stavesnode& n2) { // o1 and o2 = *written* notes
    assert(n2.isvalid());
    assert(iswhite(n2.wno));
    assert(n2.clefprefpenalty >= 0);
    return n2.clefprefpenalty; // precalculated
  }

  // in proper clef (fits inside the staff)
  inline fomus_float
  properclef(const stavesnode& n2) { // o1 and o2 = *written* notes
    assert(iswhite(n2.wno));
    fomus_int d = todiatonic(n2.wno);
    assert(n2.properclefpenalty >= 0);
    if (d < n2.downlls)
      return testgtzero(((n2.downlls - d) / 2) *
                        n2.properclefpenalty); // one penalty for each legerline
    if (d > n2.uplls)
      return testgtzero(((d - n2.uplls) / 2) * n2.properclefpenalty);
    return 0;
  }
  // PENALTIES:
  // clef change (+ all previous ones in range--so + penalty for them being
  // close together)
  inline fomus_float
  clefchange(const bool ch,
             const stavesnode& n2) { // n2 used for calculating distance
    assert(n2.isvalid());
    return ch ? n2.clefchangepenalty : 0;
  }
  // staff change (...ditto...)
  inline fomus_float
  staffchange(const bool ch,
              const stavesnode& n2) { // n2 used for calculating distance
    assert(n2.isvalid());
    return ch ? n2.staffchangepenalty : 0;
  }
  // are higher pitches in higher clefs?
  inline fomus_float highlowpitchclef(const stavesnode& n1,
                                      const stavesnode& n2) {
    return ((n1.no < n2.no && n1.midpitch > n2.midpitch) ||
            (n1.no > n2.no && n1.midpitch < n2.midpitch) ||
            (n1.no == n2.no && n1.midpitch != n2.midpitch &&
             n1.staff == n2.staff))
               ? n2.highlowclefpenalty
               : 0;
  }
  inline fomus_float highlowpitchstaff(const stavesnode& n1,
                                       const stavesnode& n2) {
    return ((n1.no < n2.no && n1.staff < n2.staff) ||
            (n1.no > n2.no && n1.staff > n2.staff) ||
            (n1.no == n2.no && n1.staff != n2.staff))
               ? n2.highlowstaffpenalty
               : 0;
  }
  inline fomus_float balance(const stavesnode& n1, const stavesnode& n2) {
    return (/*n1.ti == n2.ti &&*/ n1.voice != n2.voice && n1.staff == n2.staff
                ? n2.balancepenalty
                : 0);
  }
  // inline fomus_float vertbalance(const stavesnode& n1, const stavesnode& n2)
  // {
  //   return (n1.ti == n2.ti && n1.voice != n2.voice && n1.staff == n2.staff ?
  //   n2.vertbalancepenalty : 0);
  //   //return (n1.staff == n2.staff && ((n1.voice < n2.voice && n1.no < n2.no)
  //   || (n1.voice > n2.voice && n1.no > n2.no)) ? n2.vertbalancepenalty : 0);
  // }

  struct staff {
    int st;
    fomus_float tcp;
    staff(const int st) : st(st), tcp(0) {}
  };

  struct stavesdata _NONCOPYABLE {
    search_api api;    // engine api
    dist_iface diface; // fill it up with data!
    std::vector<int> clefs;
    std::vector<staff> staves; // individual voices, size is the nchoices
    module_noteobj ass, getn;
    module_partobj prt;
    stavesdata() : ass(0), getn(0) {
      diface.moddata = 0;
      diface.data.octdist_setid = octdistid;
      diface.data.beatdist_setid = beatdistid;
      diface.data.byendtime = true;
      module_partobj p = module_peeknextpart(0);
      diface.data.rangemax = module_setting_fval(p, rangeid);
      module_get_auxiface(module_setting_sval(p, distmodid), DIST_INTERFACEID,
                          &diface);
    }
    ~stavesdata() {
      if (diface.moddata)
        diface.free_moddata(diface.moddata);
    }
    bool isoutofrange(const search_node n1, const search_node n2) const {
      return diface.is_outofrange(diface.moddata, ((stavesnode*) n1)->note,
                                  ((stavesnode*) n2)->note);
    }
    int getlistofclefs(const module_partobj p) {
      prt = p;
      module_intslist cl(module_clefs(p)); // gets them all
      clefs.assign(cl.ints, cl.ints + cl.n);
      int st = module_totalnstaves(p);
      for (int i = 1; i <= st; ++i)
        staves.push_back(i);
      for (std::vector<staff>::iterator i(staves.begin()); i != staves.end();
           ++i) {
        for (std::vector<int>::const_iterator j(clefs.begin());
             j != clefs.end(); ++j) {
          i->tcp +=
              module_setting_fval(staves_clef(prt, i->st, *j), clefprefid);
        }
        assert(!clefs.empty());
        i->tcp /= clefs.size(); // get the average
      }
      return cl.n * st;
    }
    // order (clefs are in order vertically)
    // order (notes are in order vertically like voices)
    // balance (counting # voices between staves, min && max)
    inline fomus_float orderbalance(
        const std::vector<scorenode>& arr,
        const stavesnode& n2) const { // only looks at overlapping notes!
      std::vector<scorenode>::const_iterator i(boost::prior(arr.begin()));
      fomus_float sc = 0;
      boost::ptr_map<const int, std::set<int>> cnt; // staff, count
      std::set<int>* x;
      cnt.insert(n2.staff, x = new std::set<int>);
      x->insert(n2.voice);
      for (std::vector<scorenode>::const_iterator i(arr.begin());
           i != arr.end(); ++i) {
        if (module_tiedendtime(i->node->note) > n2.ti) { // was >
          boost::ptr_map<const int, std::set<int>>::iterator c(
              cnt.find(i->node->staff));
          if (c == cnt.end()) {
            cnt.insert(i->node->staff, x = new std::set<int>);
            x->insert(i->node->voice);
          } else
            c->second->insert(i->node->voice);
          assert(n2.cleforderpenalty >= 0);
          assert(n2.pitchorderpenalty >= 0);
          if (i->node->staff < n2.staff) { // i is in higher staff
            if (i->node->midpitch <= n2.midpitch)
              sc += n2.cleforderpenalty;
            if (module_pitch(i->node->note) < n2.no)
              sc += n2.pitchorderpenalty;
          } else if (i->node->staff > n2.staff) { // i is in lower staff
            if (i->node->midpitch >= n2.midpitch)
              sc += n2.cleforderpenalty;
            if (module_pitch(i->node->note) > n2.no)
              sc += n2.pitchorderpenalty;
          }
        }
      }
      for (boost::ptr_map<int, std::set<int>>::const_iterator i(cnt.begin());
           i != cnt.end(); ++i) {
        int s = i->second->size();
        if (s > 2) {
          sc += n2.voicemaxpenalty * (s - 2);
        }
      }
      return sc;
    }
    //     inline fomus_float vertbalance(const std::vector<scorenode>& arr,
    //     const stavesnode& n2) const {
    //       std::multiset<std::pair<int, int> > x;
    //       for (std::vector<scorenode>::const_reverse_iterator
    //       i(arr.rbegin()); i != arr.rend() && i->node->ti >= n2.ti; ++i) {
    // #ifndef NDEBUG
    // 	assert(i->node->ti <= n2.ti);
    // 	// if (i != arr.rbegin()) assert(i->node->ti <=
    // boost::prior(i)->node->ti); #endif
    // 	//if (i->node->ti == n2.ti)
    // 	x.insert(std::set<std::pair<int, int> >::value_type(i->node->staff,
    // i->node->voice));
    //       }
    //       fomus_float r = 0;
    //       if (!x.empty()) {
    // 	bool mv = false;
    // 	int nmv = 0, nv = 0;
    // 	for (std::set<std::pair<int, int> >::const_iterator
    // i(boost::next(x.begin())); i != x.end(); ++i) { 	  const std::pair<int,
    // int>& p(*boost::prior(i)); 	  if (i->first == p.first) { // same staff 	    if
    // (i->second == p.second) mv = true; 	    else {
    // 	      ++nv; // nv is number of voices in staff - 1
    // 	      if (mv) {
    // 		++nmv; mv = false; // nmv is number of voices w/ >1 note
    // 	      }
    // 	    }
    // 	  } else {
    // 	    if (mv) {++nmv; mv = false;}
    // 	    if (nv > 0 && nmv > 0)
    // 	      r += nv * nmv * n2.vertbalancepenalty;
    // 	    nv = nmv = 0;
    // 	  }
    // 	}
    // 	if (mv) ++nmv;
    // 	if (nv > 0 && nmv > 0)
    // 	  r += nv * nmv * n2.vertbalancepenalty;
    //       }
    //       return r;
    //     }

    search_score getscore(const search_nodes& nodes)
        const { // nodes ordered by offset, last one is the node being scored
      assert(
          nodes.n >
          0); // nodes are guaranteed to be in range, though needs to be pruned
      std::vector<scorenode> arr;
      stavesnode** ie = (stavesnode**) nodes.nodes + nodes.n - 1;
      const stavesnode& n2 = **ie;
      std::map<int, int> lsts;  // staff, clef
      std::map<int, int> vlsts; // voice, staff
      for (stavesnode** i = (stavesnode**) nodes.nodes; i != ie; ++i) {
        fomus_float d = diface.dist(diface.moddata, (*i)->note, n2.note);
        std::map<int, int>::iterator l(lsts.find((*i)->staff));
        std::map<int, int>::iterator vl(vlsts.find((*i)->voice));
        if (d <= diface.data.rangemax)
          arr.push_back(
              scorenode(*i, pow(n2.expon, d),
                        vl != vlsts.end() && vl->second != (*i)->staff,
                        l != lsts.end() && l->second != (*i)->clef));
        if (l == lsts.end())
          lsts.insert(std::map<int, int>::value_type((*i)->staff, (*i)->clef));
        else
          l->second = (*i)->clef;
        if (vl == vlsts.end())
          vlsts.insert(
              std::map<int, int>::value_type((*i)->voice, (*i)->staff));
        else
          vl->second = (*i)->staff;
        assert(arr.empty() || arr.back().node->isvalid());
      }
      search_score sc;
      sc.f =
          testgtzero(properclef(n2)) + testgtzero(orderbalance(arr, n2)) +
          testgtzero(defaultclef(
              n2)) /*+ testgtzero(vertbalance(arr, n2))*/; // multiply by arr
                                                           // size so matches
                                                           // with accumulated
                                                           // calculations in
                                                           // loop
      if (!arr.empty()) {
        fomus_float mx = 0, ll = 0;
        for (std::vector<scorenode>::const_iterator i(arr.begin());
             i != arr.end(); ++i) { // arr contains all but n2
          assert(i->dist > 0);
          ll += i->dist;
          mx += testgtzero(clefchange(i->clefch, n2)) +
                testgtzero(staffchange(i->staffch, n2)) +
                (testgtzero(highlowpitchclef(*i->node, n2)) +
                 testgtzero(highlowpitchstaff(*i->node, n2)) +
                 balance(*i->node, n2) /*+ vertbalance(*i->node, n2)*/) *
                    i->dist;
        }
        sc.f += (mx / ll);
      }
      DBG("STAFF SCORE: ");
#ifndef NDEBUGOUT
      for (std::vector<scorenode>::const_iterator i(arr.begin());
           i != arr.end(); ++i) {
        DBG(module_pitch(i->node->note)
            << ',' << i->node->staff << ',' << i->node->clef << "  ");
      }
#endif
      DBG(module_pitch(n2.note) << ',' << n2.staff << ',' << n2.clef << "  ");
      DBG("=  " << sc.f << std::endl);
      return sc;
    }
    search_node newnode(const search_node prevnode, const int choice) {
      assert(prevnode);
      module_noteobj n =
          (prevnode == api.begin)
              ? (getn ? module_peeknextnote(0) : (getn = module_nextnote()))
              : ((((stavesnode*) prevnode)->note != getn)
                     ? module_peeknextnote(((stavesnode*) prevnode)->note)
                     : (getn = module_nextnote()));
      if (!n)
        return api.end;
      const staff& ss = staves[choice / clefs.size()];
      int st = ss.st, cl = clefs[choice % clefs.size()];
      if (!module_hasstaffclef(n, st, cl))
        return 0;
      int ust = module_staff(n);
      if (ust != 0 && ust != st)
        return 0;
      int ucl = module_clef(n);
      if (ucl != -1 && ucl != cl)
        return 0;
      const module_value z(module_setting_val(n, staffid));
      assert(z.type == module_list);
      if (z.val.l.n > 0) {
        for (const module_value *i = z.val.l.vals,
                                *ie = z.val.l.vals + z.val.l.n;
             i < ie; ++i) {
          if (*i == (fomus_int) st)
            goto ITSOK1;
        }
        return 0;
      }
    ITSOK1:
      const module_value zz(module_setting_val(n, clefid));
      assert(zz.type == module_list);
      if (zz.val.l.n > 0) {
        for (const module_value *i = zz.val.l.vals,
                                *ie = zz.val.l.vals + zz.val.l.n;
             i < ie; ++i) {
          assert(i->type == module_string);
          if (module_strtoclef(i->val.s) == cl)
            goto ITSOK2;
        }
        return 0;
      }
    ITSOK2:
      return new stavesnode(n, st, cl, prt, ss.tcp);
    }
    void assignnext(const int choice) {
      DBG("ASSIGNING STAFF = " << module_time(module_peeknextnote(ass))
                               << " --> " << staves[choice / clefs.size()].st
                               << ' ' << clefs[choice % clefs.size()]
                               << std::endl);
      staves_assign(ass = module_peeknextnote(ass),
                    staves[choice / clefs.size()].st,
                    clefs[choice % clefs.size()]);
      assert(ass);
    };
  };

  extern "C" {
  void
  search_assign(void* moddata,
                int choice); // makes a solution assignment & reports it so that
                             // other phases of the program can continue
  union search_score search_get_score(void* moddata, struct search_nodes nodes);
  search_node search_new_node(
      void* moddata, search_node prevnode,
      int choice); // return NULL if no valid next node or END if at end of
                   // search--`prevnode' might be BEGIN (get the first node)
  void search_free_node(void* moddata, search_node node);
  const char* search_err(void* moddata);
  int search_score_lt(void* moddata, union search_score x,
                      union search_score y); // less is better
  union search_score search_score_add(void* moddata, union search_score x,
                                      union search_score y);
  int search_is_outofrange(void* moddata, search_node node1, search_node node2);
  }

  void search_assign(void* moddata, int choice) {
    ((stavesdata*) moddata)->assignnext(choice);
  } // makes a solution assignment & reports it so that other phases of the
    // program can continue
  union search_score search_get_score(void* moddata,
                                      struct search_nodes nodes) {
    return ((stavesdata*) moddata)->getscore(nodes);
  }
  search_node search_new_node(void* moddata, search_node prevnode, int choice) {
    return ((stavesdata*) moddata)->newnode(prevnode, choice);
  } // return NULL if no valid next node or END if at end of search--`prevnode'
    // might be BEGIN (get the first node)
  void search_free_node(void* moddata, search_node node) {
    delete (stavesnode*) node;
  }
  const char* search_err(void* moddata) {
    return 0;
  }
  int search_score_lt(void* moddata, union search_score x,
                      union search_score y) {
    return x.f > y.f;
  }
  union search_score search_score_add(void* moddata, union search_score x,
                                      union search_score y) {
    search_score r;
    r.f = x.f + y.f;
    return r;
  }
  int search_is_outofrange(void* moddata, search_node node1,
                           search_node node2) {
    return ((stavesdata*) moddata)->isoutofrange(node1, node2);
  } // node2 has the proper diface

  const char* staffupllstype = "integer>=0";
  int valid_staffuplls(const struct module_value val) {
    return module_valid_int(val, 0, module_incl, 0, module_nobound, 0,
                            staffupllstype);
  }
  const char* staffdistmultandclefpreftype = "real>0";
  int valid_staffdistmultandclefpref(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) 0), module_excl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            staffdistmultandclefpreftype);
  }
  const char* scoretype = "real>=0";
  int valid_score(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) 0), module_incl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            scoretype);
  }
  std::string enginemodstype;
  std::set<std::string> enginemodsset;
  int valid_enginemod_aux(const char* str) {
    return enginemodsset.find(str) != enginemodsset.end();
  }
  int valid_enginemod(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_enginemod_aux,
                               enginemodstype.c_str());
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
  const char* heapsizetype = "integer>=10";
  int valid_heapsize(const struct module_value val) {
    return module_valid_int(val, 10, module_incl, 0, module_nobound, 0,
                            heapsizetype);
  }

} // namespace staves

using namespace staves;

const char* module_engine(void* f) {
  return module_setting_sval(module_peeknextpart(0), enginemodid);
}
int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
void module_fill_iface(void* moddata, void* iface) { // module
  ((stavesdata*) moddata)->api = ((search_iface*) iface)->api;
  ((search_iface*) iface)->moddata = moddata;
  ((search_iface*) iface)->assign = search_assign;
  ((search_iface*) iface)->get_score = search_get_score;
  ((search_iface*) iface)->new_node = search_new_node;
  ((search_iface*) iface)->free_node = search_free_node;
  //((search_iface*)iface)->free_moddata = search_free_moddata;
  ((search_iface*) iface)->err = search_err;
  ((search_iface*) iface)->score_lt = search_score_lt;
  ((search_iface*) iface)->score_add = search_score_add;
  ((search_iface*) iface)->is_outofrange = search_is_outofrange;
  module_partobj p = module_peeknextpart(0);
  ((search_iface*) iface)->nchoices =
      ((stavesdata*) moddata)->getlistofclefs(p);
  union search_score ms;
  ms.f = std::numeric_limits<fomus_float>::max();
  ((search_iface*) iface)->min_score = ms;
  ((search_iface*) iface)->heapsize = module_setting_ival(p, heapsizeid);
}

const char* module_longname() {
  return "Staves";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Divides notes among staves and creates clef changes where "
         "appropriate.";
}
void* module_newdata(FOMUS f) {
  return new stavesdata;
}
void module_freedata(void* dat) {
  delete (stavesdata*) dat;
}
const char* module_err(void* dat) {
  return 0;
}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modstaves;
}
const char* module_initerr() {
  return ierr;
}
int module_itertype() {
  return module_bypart | module_firsttied | module_norests;
}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}

// ------------------------------------------------------------------------------------------------------------------------

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "ledgers-up"; // docscat{instsparts}
    set->type = module_int;
    set->descdoc =
        "Number of ledger lines generally allowed above the staff before a "
        "clef, staff or octave sign occurs.  "
        "Increasing this increases the number of ledger lines generally used "
        "and decreases the number of octave signs above the staff.";
    set->typedoc = staffupllstype;

    module_setval_int(&set->val, 3);

    set->loc = module_locclef;
    set->valid = valid_staffuplls;
    set->uselevel = 2;
    upllsid = id;
  } break;
  case 1: {
    set->name = "ledgers-down"; // docscat{instsparts}
    set->type = module_int;
    set->descdoc = "Number of ledger lines generally allowed below the staff "
                   "before a clef, staff or octave sign occurs.  "
                   "Increasing this increases the number of ledger lines used "
                   "and decreases the number of octave signs below the staff.";
    set->typedoc = staffupllstype; // uses the same string/valid fun

    module_setval_int(&set->val, 3);

    set->loc = module_locclef;
    set->valid = valid_staffuplls; // uses the same string/valid fun
    set->uselevel = 2;
    downllsid = id;
  } break;
  case 2: {
    set->name = "staves-distmult"; // docscat{staves}
    set->type = module_number;
    set->descdoc = "FOMUS calculates the distance between note events so that "
                   "it can weigh scores and penalties according to how close "
                   "they are to each other."
                   "  An exponential curve is usually applied before actually "
                   "calculating the weight."
                   "  This value is the distance at which a score calculated "
                   "between two notes is decreased to 1/2 it's full value"
                   " (at twice this distance the score is decreased to 1/4 "
                   "it's full value, etc.).";
    set->typedoc =
        staffdistmultandclefpreftype; // uses the same string/valid fun

    module_setval_int(&set->val, 1); // true
    // module_setval_rat(&set->val, module_makerat(2, 3)); // true

    set->loc = module_locnote;
    set->valid = valid_staffdistmultandclefpref;
    set->uselevel = 3;
    exponid = id;
  } break;
  case 3: {
    set->name = "staves-engine"; // docscat{staves}
    set->type = module_string;
    set->descdoc = "Engines provide different types of search functionality to "
                   "the rest of FOMUS's modules and are interchangeable."
                   "  For example, two of FOMUS's default engines `dynprog' "
                   "and `bfsearch' execute two different search algorithms, "
                   "each with different benefits."
                   "  Set this to the name of an engine module to change the "
                   "search algorithm used for finding staff and clef changes.";

    struct info_modfilter fi0 = {0, 0, 0, module_modengine, ENGINE_INTERFACEID};
    struct info_modfilterlist fi = {1, &fi0};
    const struct info_modlist ml(info_list_modules(&fi, 0, 0, -1));
    std::ostringstream s;
    for (const info_module *i = ml.mods, *ie = ml.mods + ml.n; i != ie; ++i) {
      enginemodsset.insert(i->name);
      if (i != ml.mods)
        s << '|';
      s << i->name;
    }
    enginemodstype = s.str();
    set->typedoc = enginemodstype.c_str();

    module_setval_string(&set->val, "bfsearch"); // true

    set->loc = module_locpart;
    set->valid = valid_enginemod;
    set->uselevel = 3;
    enginemodid = id;
    break;
  }
  case 4: {
    set->name = "staves-distmod"; // docscat{staves}
    set->type = module_string;
    set->descdoc =
        "Module that is used to calculate the \"distance\" between notes."
        "  The closer two notes are to each other, the more important their "
        "relationship is in terms of note spelling, staff choice, voice "
        "assignments, etc.."
        //"  Since distance can be calculate in many different ways, there are
        //several interchangeable modules for this."
        "  Set this to change the algorithm used for calculating distance when "
        "making decisions regarding staff and clef changes.";

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
  case 5: {
    set->name = "staves-octavedist"; // docscat{staves}
    set->type = module_number;
    set->descdoc = "If the `cartdist' distance or similar module is selected, "
                   "this is the distance of one octave"
                   " (i.e., two notes an octave apart have a distance "
                   "equivalent to this value)."
                   "  A larger distance translates to a smaller weight in "
                   "decisions regarding staves/clefs and vice versa.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 2);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    octdistid = id;
  } break;
  case 6: {
    set->name = "staves-beatdist"; // docscat{staves}
    set->type = module_number;
    set->descdoc = "If the `cartdist' distance or similar module is selected, "
                   "this is the distance of one beat in a measure."
                   "  A larger distance translates to a smaller weight in "
                   "decisions regarding staves/clefs and vice versa.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    beatdistid = id;
  } break;
  case 7: {
    set->name = "staves-instaff-score"; // docscat{staves} // fits in the staff
                                        // w/ given clef
    set->type = module_number;
    set->descdoc = "Score applied when notes fit inside the staff.  "
                   "A high score causes notes to appear inside their staves "
                   "(i.e., without the use of copious ledger lines) and also "
                   "affects the selection of proper clef signatures.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 13);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    properclefid = id;
  } break;
  case 8: {
    set->name = "staves-clef-vertorder-score"; // docscat{staves} // for
                                               // simultaneous notes
    set->type = module_number;
    set->descdoc =
        "Score applied when clefs appear in the correct vertical order on more "
        "than one staff.  "
        "This persuades FOMUS to avoid situations where an upper staff is "
        "actually lower in register than a lower staff (and vice versa).";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 8);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    cleforderid = id;
  } break;
  case 9: {
    set->name = "staves-pitch-vertorder-score"; // docscat{staves} // for
                                                // simultaneous notes
    set->type = module_number;
    set->descdoc =
        "Score for putting simultaneous pitches in the correct vertical order "
        "(i.e., higher notes go on higher staves and vice versa).  "
        "Increasing this value makes it more likely for pitches to appear in "
        "the correct vertical order across staves.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 8); // 13

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    pitchorderid = id;
  } break;
  case 10: {
    set->name = "staves-clefchange-score"; // docscat{staves}
    set->type = module_number;
    set->descdoc = "Score for avoiding rapid clef changes."
                   "  Increasing this makes it less likely for clef changes to "
                   "occur rapidly in succession.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 8);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    clefchangeid = id;
  } break;
  case 11: {
    set->name = "staves-staffchange-score"; // docscat{staves}
    set->type = module_number;
    set->descdoc = "Score for avoiding rapid staff changes."
                   "  Increasing this makes it less likely for staff changes "
                   "to occur rapidly in succession.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 8); // 3

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    staffchangeid = id;
  } break;
  case 12: {
    set->name = "staves-voicemax-score"; // docscat{staves} // more than 2
                                         // voices in a staff penalty
    set->type = module_number;
    set->descdoc = "Score for placing no more than two voices in a staff.  "
                   "Three or more voices could become crowded since note stems "
                   "are forced to run into each other."
                   "  Increasing this value helps to avoid this situation.";
    set->typedoc = scoretype;

    module_setval_int(&set->val, 13);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    voicemaxid = id;
  } break;
  case 13: {
    set->name = "staves-maxdist"; // docscat{staves}
    set->type = module_number;
    set->descdoc = "The maximum distance allowed for staff/clef calculations."
                   "  Notes that are more than this distance apart from each "
                   "other are considered unrelated to each other and are not "
                   "included in FOMUS's calculations.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 5);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    rangeid = id;
  } break;
  case 14: {
    set->name = "staves-searchdepth"; // docscat{staves}
    set->type = module_int;
    set->descdoc =
        "The search depth used when an appropriate engine module (such as "
        "`bfsearch') is selected."
        "  A larger search depth increases both computation time and quality."
        //"  FOMUS looks ahead and considers this many events before making a
        //decision."
        ;
    set->typedoc = heapsizetype;

    module_setval_int(&set->val, 25);

    set->loc = module_locpart;
    set->valid = valid_heapsize;
    set->uselevel = 3;
    heapsizeid = id;
  } break;
  case 15: {
    set->name = "staves-preferred-clef-score"; // docscat{staves}
    set->type = module_number;
    set->descdoc = "Score for using a staff's \"preferred clef.\""
                   "  Preferred clefs are specified in staff objects with the "
                   "`clef-preference' setting.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 5);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    preferredid = id;
  } break;
  case 16: {
    set->name = "clef-preference"; // docscat{instsparts}
    set->type = module_number;
    set->descdoc = "This number affects how often a clef is used in a staff."
                   "  1 is the default value for all clefs, which gives them "
                   "equal probability of being chosen.  "
                   "Anything less than or greater than 1 decreases or "
                   "increases the probability of choosing that clef."
                   "  Set this in an instrument definition to affect how clefs "
                   "are chosen for that instrument.";
    set->typedoc = staffdistmultandclefpreftype;

    module_setval_int(&set->val, 1);

    set->loc = module_locclef;
    set->valid = valid_staffdistmultandclefpref;
    set->uselevel = 2;
    clefprefid = id;
  } break;
  case 17: {
    set->name =
        "staves-appropriate-clef-score"; // docscat{staves} // higher pitches
                                         // get highre clefs, vice versa
    set->type = module_number;
    set->descdoc =
        "Score applied when higher pitches are in \"higher\" clefs and vice "
        "versa."
        "  This helps insure that awkward clef changes don't occur when a "
        "pitch interval is leaping in the opposite direction.";
    set->typedoc = scoretype;

    module_setval_int(&set->val, 2);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    highlowclefid = id;
  } break;
  case 18: {
    set->name =
        "staves-appropriate-staff-score"; // docscat{staves} // higher pitches
                                          // get highre clefs, vice versa
    set->type = module_number;
    set->descdoc =
        "Score applied when relatively higher pitches in a melodic line are in "
        "higher staves and vice versa."
        "  This helps insure that awkward staff changes don't occur when a "
        "pitch interval is leaping in the opposite direction.";
    set->typedoc = scoretype;

    module_setval_int(&set->val, 3);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    highlowstaffid = id;
  } break;
  case 19: {
    set->name = "staves-balance-score"; // docscat{staves} // more than 2 voices
                                        // in a staff penalty
    set->type = module_number;
    set->descdoc = "Score for balancing voices across staves.  "
                   "Increasing this helps spread prevent crowding of separate "
                   "voices onto the same staff.";
    set->typedoc = scoretype;

    module_setval_int(&set->val, 5);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    balanceid = id;
  } break;
  // case 20:
  //   {
  //     set->name = "staves-vertbalance-score"; // docscat{staves} // more than
  //     2 voices in a staff penalty set->type = module_number; set->descdoc =
  //     "Score for balancing simultaneous notes across staves.  "
  // 	"Increasing this helps spread prevent crowding of separate chords onto
  // the same staff.";
  //     set->typedoc = scoretype;

  //     module_setval_int(&set->val, 0);

  //     set->loc = module_locnote;
  //     set->valid = valid_score;
  //     set->uselevel = 3;
  //     vertbalanceid = id;
  //   }
  //   break;
  default:
    return 0;
  }
  return 1;
}
void module_ready() {
  staffid = module_settingid("staff");
  if (staffid < 0) {
    ierr = "missing required setting `staff'";
    return;
  }
  clefid = module_settingid("clef");
  if (clefid < 0) {
    ierr = "missing required setting `clef'";
    return;
  }
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
  return scmp(a, b, enginemodid) && icmp(a, b, heapsizeid) &&
         scmp(a, b, distmodid) && vcmp(a, b, octdistid) &&
         vcmp(a, b, beatdistid) && vcmp(a, b, rangeid);
}
