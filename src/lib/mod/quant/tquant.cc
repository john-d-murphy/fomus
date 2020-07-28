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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <deque>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/casts.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "ifacedivrules.h"
#include "ifacedivsearch.h"
#include "infoapi.h"
#include "module.h" // include modnotes.h

#include "debugaux.h"
#include "ilessaux.h"
using namespace ilessaux;

namespace tquant {

  int rlmodid, maxdivid, delzeroid, howaveid,
      enginemodid /*, mintupdurid, maxtupdurid*/; //, smallestdurid;

  template <typename I, typename T>
  inline bool hassome(I first, const I& last, T& pred) {
    while (first != last)
      if (pred(*first++))
        return true;
    return false;
  }
#ifndef NDEBUG
  template <typename I, typename T>
  inline bool hasall(I first, const I& last, T& pred) {
    while (first != last)
      if (!pred(*first++))
        return false;
    return true;
  }
#endif

  struct setptr : public boost::shared_ptr<std::set<int>> {
    setptr() : boost::shared_ptr<std::set<int>>(new std::set<int>) {}
    setptr(std::set<int>* x) : boost::shared_ptr<std::set<int>>(x) {}
  };
  inline setptr getvs(const module_noteobj no) {
    module_intslist vs(module_voices(no));
    return setptr(new std::set<int>(vs.ints, vs.ints + vs.n));
  }
  struct setptrcmp
      : public std::binary_function<const setptr&, const setptr&, bool> {
    bool operator()(const setptr& a, const setptr& b) {
      return std::lexicographical_compare(a->begin(), a->end(), b->begin(),
                                          b->end());
    }
  };

  struct module_valueq : public module_value {
    bool q;
    module_valueq(const module_value& v, const bool q)
        : module_value(v), q(q) {}
    bool hasbeenquant() const {
      return q;
    }
    void setquant() {
      assert(!q);
      q = true;
    }
    module_valueq getqed() const {
      return module_valueq(*this, true);
    }
  };
  inline bool operator<(const module_valueq& x, const module_valueq& y) {
    return (const module_value&) x < (const module_value&) y;
  }
#ifndef NDEBUGOUT
  std::ostream& operator<<(std::ostream& o, const module_valueq& x) {
    switch (((const module_value&) x).type) {
    case module_rat: {
      o << module_rattostr(((const module_value&) x).val.r);
      return o;
    }
    case module_int:
      o << ((const module_value&) x).val.i;
      return o;
    case module_float:
      o << ((const module_value&) x).val.f;
      return o;
    default:
      assert(false);
    }
  }
#endif

  typedef std::set<module_valueq /*, std::less<module_valueq>*/> timepointssetq;
  typedef timepointssetq::iterator timepointssetq_it;
  typedef timepointssetq::const_iterator timepointssetq_constit;

  typedef std::set<module_value /*, std::less<module_value>*/> timepointsset;
  typedef timepointsset::iterator timepointsset_it;
  typedef timepointsset::const_iterator timepointsset_constit;

  // assumes ordered vectors--might increment a
  inline fomus_float closestdist(const module_value& v,
                                 timepointsset_constit& a,
                                 const timepointsset_constit& aend) {
    assert(a != aend);
    fomus_float x = std::fabs(GET_F(*a - v));
    while (true) {
      timepointsset_constit b(boost::next(a));
      if (b == aend)
        return x;
      fomus_float y = std::fabs(GET_F(*b - v));
      if (x <= y)
        return x;
      else {
        a = b;
        x = y;
      }
    }
  }

  // assumes ordered vectors--might increment a
  inline fomus_rat closest(const module_value& v, const timepointsset& a) {
    DBG("finding closest to " << v << " in: ");
#ifndef NDEBUGOUT
    for (timepointsset::const_iterator i(a.begin()); i != a.end(); ++i) {
      DBG(*i << ", ");
    }
    DBG(std::endl);
#endif
    timepointsset_constit a2(a.upper_bound(v));
    if (a2 == a.begin())
      return GET_R(*a2);
    timepointsset_constit a1(boost::prior(a2));
    if (a2 == a.end())
      return GET_R(*a1);
    return GET_R(
        *(std::fabs(GET_F(*a1 - v)) < std::fabs(GET_F(*a2 - v)) ? a1 : a2));
  }

  inline fomus_float ave_mse(const timepointssetq& pts,
                             const timepointsset& qpts) {
    assert(!qpts.empty());
    fomus_float sm = 0;
    timepointsset_it q(qpts.begin()); //, q0(qpts.begin());
    for (timepointssetq_constit e(pts.begin()); e != pts.end(); ++e) {
      if (e->q) {
        DBG("q(" << *e << ") ");
        fomus_float x =
            closestdist(*e, q, qpts.end()); // q might be incremented
        sm += x * x;
      }
    }
    return sm; // return std::sqrt(sm); don't bother with extra calculation
  }
  inline fomus_float ave_sum(const timepointssetq& pts,
                             const timepointsset& qpts) {
    assert(!qpts.empty());
    fomus_float sm = 0;
    timepointsset_it q(qpts.begin()); //, q0(qpts.begin());
    for (timepointssetq_constit e(pts.begin()); e != pts.end(); ++e) {
      if (e->q) {
        sm += closestdist(*e, q, qpts.end());
      }
    }
    return sm;
  }
  enum quanthowave { how_mse, how_sum };

  inline fomus_float
  getscore(const quanthowave how, const timepointssetq& pts,
           const timepointsset& qpts) { // qpts are the search points
    DBG("SCORE:" << std::endl);
#ifndef NDEBUGOUT
    DBG("orig: ");
    for (timepointssetq_constit i(pts.begin()), ie(pts.end()); i != ie; ++i)
      DBG(*i << ' ');
    DBG(std::endl);
    DBG("quant: ");
    for (timepointsset_constit i(qpts.begin()), ie(qpts.end()); i != ie; ++i)
      DBG(*i << ' ');
    DBG(std::endl);
#endif
    switch (how) {
    case how_mse: {
      fomus_float x = ave_mse(pts, qpts);
      DBG('=' << x << std::endl);
      return x;
    }
    case how_sum:
      return ave_sum(pts, qpts);
    default:
      assert(false);
    }
  }

  struct solut {
    module_noteobj note;
    fomus_rat o1; // solution beginning, end (initially invalid)
    fomus_rat o2;
    int vs; // "voice-group" number
    std::set<int> begtup, endtup;
    // int tuplvl;
    // std::map<int, bool> utups; // bool is true if endtup
    solut(const module_noteobj note, const int vs);
    bool o1notset() const {
      return o1.den == -1;
    }
    bool o2notset() const {
      return o2.den == -1;
    }
    bool isready() const {
      return o1.den != -1 && o2.den != -1;
    }
    void assign(/*const int delzeroid*/);
    bool hasbegtup(const int lvl) const {
      return begtup.find(lvl) != begtup.end();
    }
    bool hasendtup(const int lvl) const {
      return endtup.find(lvl) != endtup.end();
    }
    bool hasanybegtup() const {
      return !begtup.empty();
    }
    bool hasanyendtup() const {
      return !endtup.empty();
    }
  };

  solut::solut(const module_noteobj note, const int vs)
      : note(note), vs(vs) /*, begtup(false), endtup(false)*/ {
    o1.den = -1;
    o2.den = -1;
    module_markslist ml(module_singlemarks(note));
    for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) {
      switch (module_markid(*m)) {
      case mark_tuplet_begin: {
        module_value n(module_marknum(*m));
        begtup.insert(n.type == module_none ? (fomus_int) 0 : GET_I(n));
        break;
      }
      case mark_tuplet_end: {
        module_value n(module_marknum(*m));
        endtup.insert(n.type == module_none ? (fomus_int) 0 : GET_I(n));
        break;
      }
      }
    }
  }

  void solut::assign(/*const int delzeroid*/) {
    fomus_rat d(o2 - o1);
    bool isgr(module_isgrace(note));
    if (!isgr && d == (fomus_int) 0) { // maybe make it a grace note
      if (module_setting_ival(note, delzeroid)) {
        DBG("TQUANT ASSIGN a: ti:" << o1 << " du:" << d << std::endl);
        tquant_assign_time(note, o1, module_makerat(0, 1));
      } else {
        DBG("TQUANT ASSIGN b: delete" << std::endl);
        tquant_delete(note);
      }
    } else if (isgr) {
      DBG("TQUANT ASSIGN c: ti:" << o1 << " grti:" << module_vgracetime(note)
                                 << " grdu:" << module_vgracedur(note)
                                 << std::endl);
      tquant_assign_gracetime(note, o1, module_vgracetime(note),
                              module_vgracedur(note));
      // module_skipassign(note);
    } else {
      assert(d != (fomus_int) 0);
      DBG("TQUANT ASSIGN d: ti:" << o1 << " du:" << d << std::endl);
      tquant_assign_time(note, o1, d);
    }
  }

  typedef std::map<const std::string, quanthowave, isiless> strtohowavemap;
  typedef strtohowavemap::value_type strtohowavemap_val;
  typedef strtohowavemap::const_iterator strtohowavemap_constit;
  strtohowavemap strtohowave;

  struct quantnotes;
  struct quantdata {
    divsearch_api api;
    divrules_iface rliface;
    fomus_rat lowestlim;
    module_measobj lmeas;
    std::deque<solut> inout; // FILO queue--no pool
    quanthowave howave;
    std::map<setptr, int, setptrcmp> vslist;
    int lvsnum;
    bool pass2;
    std::vector<divrules_range> exclvect;
#ifndef NDEBUG
    bool nnn;
#endif
    quantdata() : lmeas(0), lvsnum(0), pass2(true) { // MSE or sum
      rliface.moddata = 0;
      rliface.data.dotnotelvl_setid = -1;
      rliface.data.dbldotnotelvl_setid = -1;
      rliface.data.slsnotelvl_setid = -1;
      rliface.data.syncnotelvl_setid = -1;
#ifndef NDEBUG
      nnn = false;
#endif
    }
    ~quantdata() {
      if (rliface.moddata)
        rliface.free_moddata(rliface.moddata);
    }
    int getvsnum(const module_noteobj n) {
      setptr sp(getvs(n));
      std::map<setptr, int, setptrcmp>::const_iterator i(vslist.find(sp));
      if (i != vslist.end())
        return i->second;
      int r = vslist.size();
      vslist.insert(std::map<setptr, int, setptrcmp>::value_type(sp, r));
      DBG("vsnum is now " << r << std::endl);
      return r;
    }
    int getnvsnums() const {
      return vslist.size();
    }
    void morenotes(const module_measobj meas, timepointssetq& vals);
    void assignnotes(const quantnotes& node);
    divsearch_node getroot();
  };

  void quantdata::morenotes(const module_measobj meas, timepointssetq& vals) {
  AGAIN:
    assert(vals.empty());
    fomus_rat mbeg(
        module_time(meas)); // measure off/endoffs expected to be rationals
    fomus_rat mend(module_endtime(meas));
    for (std::deque<solut>::const_iterator i(inout.begin()); i != inout.end();
         ++i) { //... previous notes that overlap into this measure
      if (i->vs != lvsnum)
        continue;
      module_value o(module_vtime(i->note));
      if (o >= mbeg && o <= mend && (pass2 || i->hasanybegtup())) {
        vals.insert(module_valueq(o, false)); // insert the offset...
      }
      module_value eo(module_vendtime(i->note));
      DBG("grabbing from prev meas: o="
          << o << " eo=" << eo
          << " o1=" << (i->o1notset() ? module_makerat(-1, 1) : i->o1) << " o2="
          << (i->o2notset() ? module_makerat(-1, 1) : i->o2) << std::endl);
      if (eo >= mbeg && eo <= mend && (pass2 || i->hasanyendtup())) {
        vals.insert(module_valueq(eo, false)); // insert the offset...
      }
    }
    if (meas != lmeas) {
      assert(!pass2);
      while (true) { //... read in notes and collect boundary points
#ifndef NDEBUG
        nnn = true;
#endif
        module_noteobj note = module_nextnote();
        if (!note)
          break;
        int v = getvsnum(note);
        inout.push_back(solut(note, v));
        if (v != lvsnum)
          continue;
        module_value o(module_vtime(note));
        if (o > mend)
          break;
        assert(o >= mbeg);
        if (inout.back().hasanybegtup()) {
          vals.insert(module_valueq(o, false));
        }
        module_value eo(module_vendtime(note));
        if (eo <= mend && inout.back().hasanyendtup()) {
          vals.insert(module_valueq(eo, false));
        }
      }
      lmeas = meas;
    }
    if (!pass2 && vals.empty()) {
      pass2 = true;
      goto AGAIN;
    }
    lowestlim = module_ratinv(
        module_setting_rval(meas, maxdivid)); // beatdivision returns rat
#ifndef NDEBUG
    strtohowavemap_constit hh(
        strtohowave.find(module_setting_sval(meas, howaveid)));
    assert(hh != strtohowave.end()); // should have been checked
    howave = hh->second;
#else
    howave = strtohowave.find(module_setting_sval(meas, howaveid))->second;
#endif
  }

  // NODES
  struct quantbase {
#ifndef NDEBUG
    int valid;
    quantbase() : valid(12345) {}
#endif
    virtual void expand(quantdata& data,
                        const divsearch_ornode_ptr ornode) const = 0;
    virtual bool isleaf(const quantdata& data) const = 0;
    virtual divsearch_score score(const quantdata& data) const = 0;
    virtual ~quantbase() {}
#ifndef NDEBUG
    bool isvalid() const {
      return valid == 12345;
    }
    virtual bool isnotes() const {
      return false;
    }
#endif
  };

  struct quantnotes : public quantbase {
    const divrules_iface& rliface;
    fomus_rat o1, o2;
    const divrules_div rule; // this should be valid for lifetime of struct
    timepointssetq vals;
    timepointsset qvals; // values to compare to
    bool qtzd;
    std::vector<divrules_range> excl;
    quantnotes(const quantdata& data, const timepointssetq_constit& n1,
               const timepointssetq_constit& n2, const divrules_div rule,
               const fomus_rat& o1, const fomus_rat& o2,
               const std::vector<divrules_range>& vect); // EXPAND
    quantnotes(const divrules_iface& rliface, const fomus_rat& o1,
               const fomus_rat& o2)
        : rliface(rliface), o1(o1), o2(o2), rule(0), qtzd(true) {} // ASSEMBLE
    quantnotes(const quantdata& data, struct divsearch_andnode& andnode);
    ~quantnotes() {
      if (rule)
        rliface.free_node(rliface.moddata, rule);
    }
#ifndef NDEBUG
    bool isnotes() const {
      return true;
    }
#endif
    void assem(const quantnotes& qn) { // add in the quantized stuff
      assert(isvalid());
      assert(qn.isvalid());
      vals.insert(qn.vals.begin(), qn.vals.end());
      qvals.insert(qn.qvals.begin(), qn.qvals.end());
      qtzd = qtzd && qn.qtzd;
    }
    void expand(quantdata& data, const divsearch_ornode_ptr onode) const;
    bool isleaf(const quantdata& data) const {
      DBG(" from " << o1 << " to " << o2 << " leaf= " << qtzd << std::endl);
      return qtzd;
    }
    divsearch_score score(const quantdata& data) const {
      divsearch_score r;
      r.f = getscore(data.howave, vals, qvals /*, data.lowestlim*/);
      return r;
    }
  };

  quantnotes::quantnotes(const quantdata& data,
                         const timepointssetq_constit& n1,
                         const timepointssetq_constit& n2,
                         const divrules_div rule, const fomus_rat& o1,
                         const fomus_rat& o2,
                         const std::vector<divrules_range>& vect)
      : rliface(data.rliface), o1(o1), o2(o2), rule(rule) {
    qvals.insert(module_makeval(o1));
    qvals.insert(module_makeval(o2));
    if (o2 - o1 > data.lowestlim &&
        hassome(n1, n2, boost::lambda::_1 > o1 && boost::lambda::_1 < o2)) {
      vals.insert(n1, n2);
      qtzd = false;
    } else {
      for (timepointssetq_constit i(n1); i != n2; ++i)
        vals.insert(i->getqed());
      qtzd = true;
    }
    for (std::vector<divrules_range>::const_iterator i(vect.begin());
         i != vect.end(); ++i) {
      if (i->time2 >= o1 && i->time1 <= o2)
        excl.push_back(*i);
    }
  }

  quantnotes::quantnotes(const quantdata& data,
                         struct divsearch_andnode& andnode)
      : rliface(data.rliface),
        o1(module_makerat(std::numeric_limits<fomus_int>::max(), 1)),
        o2(module_makerat(std::numeric_limits<fomus_int>::min() + 1, 1)),
        rule(0) {
    assert(andnode.n > 0);
    for (divsearch_node *d = andnode.nodes, *de = andnode.nodes + andnode.n;
         d != de; ++d) {
      const quantnotes& dd = *((quantnotes*) *d);
      if (dd.o1 < o1)
        o1 = dd.o1;
      if (dd.o2 > o2)
        o2 = dd.o2;
      vals.insert(dd.vals.begin(), dd.vals.end());
      qvals.insert(dd.qvals.begin(), dd.qvals.end());
    }
  }

  inline void quantnotes::expand(quantdata& data,
                                 const divsearch_ornode_ptr onode) const {
    assert(rule);
    divrules_rangelist exc = {excl.size(), &excl[0]};
    divrules_ornode exp(rliface.expand(rliface.moddata, rule,
                                       exc)); // only need to free the nodes
    assert(exp.n > 0);
    for (divrules_andnode *i = exp.ands, *ie = exp.ands + exp.n; i != ie; ++i) {
      divsearch_andnode_ptr anode = data.api.new_andnode(onode);
      fomus_rat t1(o1);
      timepointssetq_constit pts1 = vals.begin();
      for (divrules_div *d = i->divs, *de = i->divs + i->n; d != de; ++d) {
        assert(t1 == rliface.time(rliface.moddata, *d));
        fomus_rat t2(rliface.endtime(rliface.moddata, *d));
        timepointssetq_constit pts2(upper_bound(pts1, vals.end(), t2));
        data.api.push_back(anode,
                           new quantnotes(data, pts1, pts2, *d, t1, t2, excl));
        t1 = t2;
        pts1 = pts2;
      }
    }
  }

  struct quantmeas
      : public quantbase {     // measure node--should be "expanded" in order
    const module_measobj meas; // dfsearch should open these in order
    quantmeas(const module_measobj meas) : meas(meas) {}
    void expand(quantdata& data, const divsearch_ornode_ptr onode) const;
    bool isleaf(const quantdata& data) const {
      return false;
    }
    divsearch_score score(const quantdata& data) const {
      divsearch_score r;
      r.f = 0;
      return r;
    }
  };

  inline void quantmeas::expand(quantdata& data,
                                const divsearch_ornode_ptr onode) const {
    timepointssetq vals; // ea. contains boolean q "hasbeenquantized" flag
    data.morenotes(meas, vals);
    struct module_list x;
    x.n = 0;
    fomus_rat mt(module_time(meas)), met(module_endtime(meas));
    divrules_ornode rule(data.rliface.get_root(data.rliface.moddata, mt, x));
    assert(meas);
    for (divrules_andnode *a(rule.ands), *ae(rule.ands + rule.n); a < ae; ++a) {
      divsearch_andnode_ptr andn(data.api.new_andnode(onode));
      for (divrules_div *d(a->divs), de(a->divs + a->n); d < de; ++d) {
        data.api.push_back(andn, new quantnotes(data, vals.begin(), vals.end(),
                                                *d, mt, met, data.exclvect));
      }
    }
  }

  struct userpt {
    fomus_rat o;
    bool isend; // false = beg
    int lvl;
    userpt(const fomus_rat& o, const bool isend, const int lvl)
        : o(o), isend(isend), lvl(lvl) {}
  };
  inline bool operator<(const userpt& x, const userpt& y) {
    if (x.o != y.o)
      return x.o < y.o;
    if (x.isend != y.isend)
      return (x.isend ? 1 : 0) > (y.isend ? 1 : 0);
    return (x.isend ? x.lvl > y.lvl : x.lvl < y.lvl);
  }

  struct rangest {
    fomus_rat time1, time2;
    rangest() : time1(module_makerat(-1, 1)), time2(module_makerat(-1, 1)) {}
    void getrange(const int l, std::vector<divrules_range>& ra) const {
      if (time1 >= (fomus_int) 0 && time2 >= (fomus_int) 0) {
        // divrules_range r = {(time1 < (fomus_int)0 ? time2 : time1), (time2 <
        // (fomus_int)0 ? time1 : time2), l};
        divrules_range r = {time1, time2, l};
        ra.push_back(r);
      }
    }
  };

  inline void
  quantdata::assignnotes(const quantnotes& node) { // node is a solution node
    assert(lmeas);
#ifndef NDEBUG
    assert(node.qtzd);
    for (timepointssetq_constit i(node.vals.begin()); i != node.vals.end();
         ++i) {
      assert(i->q);
    }
#endif
    std::set<userpt> pts;
    fomus_rat mbeg(
        module_time(lmeas)); // measure off/endoffs expected to be rationals
    fomus_rat mend(module_endtime(lmeas));
    assert(pass2 || exclvect.empty());
    for (std::deque<solut>::iterator i(inout.begin()); i != inout.end();
         ++i) { // assign timepoints to notes
      if (i->vs != lvsnum)
        continue;
      DBG("Note TIMES = " << module_vtime(i->note) << " -- "
                          << module_vendtime(i->note) << std::endl);
      if (i->o1notset() && (pass2 || i->hasanybegtup())) {
        module_value o(module_vtime(i->note));
        if (o >= mbeg && o <= mend) {
          i->o1 = closest(o, node.qvals);
          DBG("found o1: " << i->o1 << std::endl);
          if (!pass2) {
            for (std::set<int>::const_iterator j(i->begtup.begin()),
                 je(i->begtup.end());
                 j != je; ++j) {
              pts.insert(userpt(i->o1, false, *j));
            }
          }
        }
      }
      if (i->o2notset() && (pass2 || i->hasanyendtup())) {
        module_value eo(module_vendtime(i->note));
        if (eo >= mbeg && eo <= mend) {
          i->o2 = closest(eo, node.qvals);
          DBG("found o2: " << i->o2 << std::endl);
          if (!pass2) {
            for (std::set<int>::const_iterator j(i->endtup.begin()),
                 je(i->endtup.end());
                 j != je; ++j) {
              pts.insert(userpt(i->o2, true, *j));
            }
          }
        }
      }
    }
    if (pass2) {
      while (!inout.empty() && inout.front().isready()) {
        inout.front().assign();
        inout.pop_front();
      }
    } else { // get exclusions
      std::stack<rangest> st;
      DBG("  the pts are like thus: [");
      for (std::set<userpt>::const_iterator i(pts.begin()); i != pts.end();
           ++i) {
        DBG((i->isend ? "--" : "") << i->o << (i->isend ? " " : "-- "));
        int sz = i->lvl + 1;
        while ((int) st.size() > sz) {
          st.top().getrange(i->lvl, exclvect);
          st.pop();
        }
        while ((int) st.size() < sz)
          st.push(rangest());
        if (i->isend) {
          st.top().time2 = i->o;
          st.top().getrange(i->lvl, exclvect);
          st.pop();
        } else {
          st.top().time1 = i->o;
        }
      }
      DBG("]" << std::endl);
      while (!st.empty()) {
        st.top().getrange(st.size() - 1, exclvect);
        st.pop();
      }
    }
  }

  // notes & rests haven't been divided up yet, even across measures--still the
  // user's raw input
  inline divsearch_node quantdata::getroot() {
    pass2 = !pass2;
    module_measobj m;
    if (pass2) {
      m = lmeas;
    } else {
      exclvect.clear();
      if (lvsnum >= getnvsnums()) {
        m = module_nextmeas();
        lvsnum = 0;
      } else {
        m = lmeas;
        ++lvsnum;
      }
    }
    if (m) {
      const char* rlmod = module_setting_sval(m, rlmodid); // rules module
      assert(rlmod);
      if (rliface.moddata)
        rliface.free_moddata(rliface.moddata);
      rliface.data.meas = m;
      module_get_auxiface(rlmod, DIVRULES_INTERFACEID, &rliface);
      return new quantmeas(m);
    } else {
      assert(nnn);
      assert(!module_nextnote());
      return 0;
    }
  }

  // ------------------------------------------------------------------------------------------------------------------------

  extern "C" {
  union divsearch_score score(void* moddata, divsearch_node node);
  void expand(void* moddata, divsearch_ornode_ptr ornode,
              divsearch_node node); // must expand node and operate on ornode
  divsearch_node assemble(void* moddata, struct divsearch_andnode andnode);
  int score_lt(void* moddata, union divsearch_score x, union divsearch_score y);
  void free_node(void* moddata, divsearch_node node);
  divsearch_node get_root(void* moddata);
  int is_leaf(void* moddata, divsearch_node node);
  void solution(void* moddata, divsearch_node node);
  const char* qerr(void* moddata);
  }
  union divsearch_score score(void* moddata, divsearch_node node) {
    return ((quantbase*) node)->score(*(quantdata*) moddata);
  }
  void expand(void* moddata, divsearch_ornode_ptr ornode, divsearch_node node) {
    ((quantbase*) node)->expand(*(quantdata*) moddata, ornode);
  }
  divsearch_node assemble(void* moddata, struct divsearch_andnode andnode) {
    assert(andnode.n > 0);
    assert((*(quantbase**) andnode.nodes)->isnotes());
    quantnotes* q = new quantnotes(
        ((quantdata*) moddata)->rliface,
        (*(quantnotes**) (quantbase**) andnode.nodes)->o1,
        (*(((quantnotes**) (quantbase**) andnode.nodes) + andnode.n - 1))->o2);
    std::for_each(((quantnotes**) (quantbase**) andnode.nodes),
                  ((quantnotes**) (quantbase**) andnode.nodes) + andnode.n,
                  boost::lambda::bind(&quantnotes::assem,
                                      boost::lambda::var(*q),
                                      *boost::lambda::_1));
    return q;
  } // TODO!
  int score_lt(void* moddata, union divsearch_score x,
               union divsearch_score y) {
    return x.f > y.f;
  }
  void free_node(void* moddata, divsearch_node node) {
    delete (quantbase*) node;
  }
  divsearch_node get_root(void* moddata) {
    return ((quantdata*) moddata)->getroot();
  }
  int is_leaf(void* moddata, divsearch_node node) {
    return ((quantbase*) node)->isleaf(*(quantdata*) moddata);
  }
  void solution(void* moddata, divsearch_node node) {
    ((quantdata*) moddata)->assignnotes(*(quantnotes*) node);
  }
  const char* qerr(void* moddata) {
    return 0;
  }

  // stuff for SETTINGS
  std::string divrulesmodstype;
  std::set<std::string> divrulesmodsset;
  int valid_div_aux(const char* str) {
    return divrulesmodsset.find(str) != divrulesmodsset.end();
  }
  int valid_div(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_div_aux,
                               divrulesmodstype.c_str());
  }
  const char* quanterrtype = "ave|mse";
  int valid_quanterr_aux(const char* val) {
    return strtohowave.find(val) != strtohowave.end();
  }
  int valid_quanterr(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_quanterr_aux, quanterrtype);
  }
  const char* beatdivtype = "rational>=1";
  int valid_beatdiv(const struct module_value val) {
    return module_valid_rat(val, module_makerat(1, 1), module_incl,
                            module_makerat(0, 1), module_nobound, 0,
                            beatdivtype);
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
  // const char* mintuptype = "rational_>=0";
  // int valid_mintupdur(const struct module_value val) {return
  // module_valid_rat(val, module_makerat(0, 1), module_incl, module_makerat(0,
  // 1), module_nobound, 0, mintuptype);} // also maxp int
  // validdeps_mintupdur(FOMUS f, const struct module_value val) {return
  // GET_R(val) <= module_setting_rval(f, maxtupdurid);} int
  // validdeps_maxtupdur(FOMUS f, const struct module_value val) {return
  // module_setting_rval(f, mintupdurid) <= GET_R(val);}
  const char* smallestdurtype = "rational>0";
  int valid_smallestdur(const struct module_value val) {
    return module_valid_rat(val, module_makerat(0, 1), module_excl,
                            module_makerat(0, 1), module_nobound, 0,
                            smallestdurtype);
  }

} // namespace tquant

using namespace tquant;

void module_fill_iface(void* moddata, void* iface) {
  ((quantdata*) moddata)->api = ((divsearch_iface*) iface)->api;
  ((divsearch_iface*) iface)->moddata = moddata;
  ((divsearch_iface*) iface)->score = score;
  ((divsearch_iface*) iface)->expand = expand;
  ((divsearch_iface*) iface)->assemble = assemble;
  ((divsearch_iface*) iface)->score_lt = score_lt;
  ((divsearch_iface*) iface)->free_node = free_node;
  ((divsearch_iface*) iface)->get_root = get_root;
  ((divsearch_iface*) iface)->is_leaf = is_leaf;
  ((divsearch_iface*) iface)->solution = solution;
  ((divsearch_iface*) iface)->err = tquant::qerr;
};

const char* module_longname() {
  return "Quantize Times/Durations";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Quantizes times and durations to metrical and tuplet divisions "
         "providing the closest fit.";
}
void* module_newdata(FOMUS f) {
  return new quantdata;
}
void module_freedata(void* dat) {
  delete (quantdata*) dat;
}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modtquant;
}
const char* module_initerr() {
  return 0;
}
void module_init() {
  strtohowave.insert(strtohowavemap_val("mse", how_mse));
  strtohowave.insert(strtohowavemap_val("ave", how_sum));
}
void module_free() { /*assert(newcount == 0);*/
}
int module_itertype() {
  return module_bypart;
} // notes aren't divided yet, so they reach across measures

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "mod-quant-divrules"; // docscat{quant}
    set->type = module_string;
    set->descdoc = "The module used by FOMUS's quantizing algorithm to search "
                   "for valid metrical divisions and tuplets."
                   "  Different modules provide different rules affecting how "
                   "rhythmic spellings and note divisions/ties are determined."
                   "  Set this to choose a different set of rules.";
    struct info_modfilter fi0 = {0, 0, 0, module_modaux, DIVRULES_INTERFACEID};
    struct info_modfilterlist fi = {1, &fi0};
    const struct info_modlist ml(info_list_modules(&fi, 0, 0, -1));
    std::ostringstream s;
    for (const info_module *i = ml.mods, *ie = ml.mods + ml.n; i != ie; ++i) {
      divrulesmodsset.insert(i->name);
      if (i != ml.mods)
        s << '|';
      s << i->name;
    }
    divrulesmodstype = s.str();
    set->typedoc = divrulesmodstype.c_str();
    module_setval_string(&set->val, "divrules");

    set->loc = module_locmeasdef;
    set->valid = valid_div;
    set->uselevel = 3;
    rlmodid = id;
    break;
  }
  case 1: {
    set->name = "beatdiv"; // docscat{quant} // division of beat
    set->type = module_rat;
    set->descdoc =
        "The maximum number of divisions allowed in one beat.  "
        "Set this to the smallest division that times and durations should be "
        "quantized to.  "
        "If a beat is equivalent to a quarter note, a `beatdiv' of 4 allows "
        "notes to be quantized down to the size of a sixteenth note.  "
        "A `quant-max-beatdiv' of 8 would allow thirty-second notes, etc..";
    set->typedoc = beatdivtype;

    module_setval_int(&set->val, 16);

    set->loc = module_locmeasdef; // module_locpercinst;
    set->valid = valid_beatdiv;
    set->uselevel = 2;
    maxdivid = id;
    break;
  }
  case 2: {
    set->name = "quant-zerodur-gracenotes"; // docscat{quant}
    set->type = module_bool;
    set->descdoc =
        "Determines whether or not events quantized to a duration of zero "
        "should become grace notes.  "
        "Set this to `yes' if you want notes of very small duration to turn "
        "into grace notes or `no' if you want them to disappear.";
    // set->typedoc = beatdivtype;

    module_setval_int(&set->val, 1);

    set->loc = module_locnote; // module_locpercinst;
    // set->valid = beatdivvalid;
    set->uselevel = 2;
    delzeroid = id;
    break;
  }
  case 3: {
    set->name = "quant-error"; // docscat{quant}
    set->type = module_string;
    set->descdoc = "How quantization error is calculated.  FOMUS's quantize "
                   "module works by minimizing the amount of error and "
                   "searches for the combinations of "
                   "divisions and tuplets that provide the \"best fit.\"  "
                   "Choices are `ave' for average error and `mse' for "
                   "mean-squared error (larger errors are less likely at the "
                   "expense of a slightly higher average error).  "
                   "Choose the one that represents best how you want "
                   "quantization error to be calculated.";
    set->typedoc = quanterrtype; // same valid fun

    module_setval_string(&set->val, "mse");

    set->loc = module_locnote;
    set->valid = valid_quanterr; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2;
    howaveid = id;
    break;
  }
  case 4: {
    set->name = "tquant-engine"; // docscat{quant}
    set->type = module_string;
    set->descdoc = "Engines provide different types of search functionality to "
                   "the rest of FOMUS's modules and are interchangeable."
                   "  For example, two of FOMUS's default engines `dynprog' "
                   "and `bfsearch' execute two different search algorithms, "
                   "each with different benefits."
                   "  Set this to the name of an engine module to change the "
                   "search algorithm used for time quantization.";

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

    module_setval_string(&set->val, "divsearch"); // true

    set->loc = module_locmeasdef;
    set->valid = valid_enginemod;
    set->uselevel = 3;
    enginemodid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}
int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
const char* module_engine(void* f) {
  return module_setting_sval(module_peeknextpart(0), enginemodid);
}
void module_ready() {}
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
  return scmp(a, b, enginemodid);
}
