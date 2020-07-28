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

#include <cstring>
#include <limits>
#include <set>
#include <vector>
//#include <cstring> // strcmp
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
//#include <algorithm>
#include <stack>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "ifacedist.h"
#include "ifacedivrules.h"  // rules interface
#include "ifacedivsearch.h" // engine interface
#include "infoapi.h"
#include "module.h"
#include "modutil.h"

#include "debugaux.h"
#include "ilessaux.h"
using namespace ilessaux;

namespace split {

#define USERPEN 5
  const char* ierr = 0;

  template <class I, class C, class I2>
  void inplace_set_intersection(C& cont, I2 first2, const I2& last2) {
    I first1(cont.begin());
    while (first1 != cont.end()) {
      if (first2 == last2) {
        cont.erase(first1, cont.end());
        break;
      }
      if (*first1 < *first2)
        cont.erase(first1++);
      else if (*first2 < *first1)
        ++first2;
      else {
        ++first1;
        ++first2;
      }
    }
  }

  template <typename I, typename T>
  inline bool hasall(I first, const I& last, T& pred) {
    while (first != last)
      if (!pred(*first++))
        return false;
    return true;
  }

  typedef std::vector<fomus_rat> initdivsvect;
  typedef initdivsvect::const_iterator initdivsvect_constit;
  typedef initdivsvect::iterator initdivsvect_it;

  inline bool operator<(const initdivsvect& x, const initdivsvect& y) {
    for (initdivsvect_constit xi(x.begin()), yi(y.begin());; ++xi, ++yi) {
      if (xi == x.end())
        return yi != y.end();
      if (yi == y.end())
        return false;
      if (*xi < *yi)
        return true;
      if (*xi > *yi)
        return false;
    }
  }
  inline bool operator<(const initdivsvect* x, const initdivsvect& y) {
    return *x < y;
  }
  inline bool operator<(const initdivsvect& x, const initdivsvect* y) {
    return x < *y;
  }
  inline bool idvless0(const initdivsvect& x, const initdivsvect& y) {
    return x < y;
  }
  inline bool idvless0(const initdivsvect* x, const initdivsvect& y) {
    return *x < y;
  }
  inline bool idvless0(const initdivsvect& x, const initdivsvect* y) {
    return x < *y;
  }
  inline bool idvless0(const initdivsvect* x, const initdivsvect* y) {
    return *x < *y;
  }

  template <typename T>
  struct idvless : std::binary_function<T, T, bool> {
    bool operator()(const T& x, const T& y) const {
      return idvless0(x, y);
    }
  };

  typedef boost::ptr_set<initdivsvect, idvless<initdivsvect>> comdivsset;
  typedef comdivsset::const_iterator comdivsset_constit;
  typedef comdivsset::iterator comdivsset_it;

  typedef boost::ptr_set<initdivsvect, idvless<initdivsvect>,
                         boost::view_clone_allocator>
      comdivssetview;
  typedef comdivssetview::const_iterator comdivssetview_constit;
  typedef comdivssetview::iterator comdivssetview_it;

  inline bool operator!=(const initdivsvect& x, const initdivsvect& y) {
    initdivsvect_constit yi(y.begin());
    for (initdivsvect_constit xi(x.begin()); xi != x.end(); ++xi, ++yi) {
      if (yi == y.end())
        return true;
      if (*xi != *yi)
        return true;
    }
    return yi != y.end();
  }

  inline bool operator==(const comdivsset& x, const comdivsset& y) {
    comdivsset_constit i2(y.begin());
    for (comdivsset_constit i1(x.begin()); i1 != x.end(); ++i1, ++i2) {
      if (i2 == y.end())
        return false;
      if (*i1 != *i2)
        return false;
    }
    return i2 == y.end();
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
  // inline bool operator<(const userpt& x, const userpt& y) {
  //   if (x.o != y.o) return x.o < y.o;
  //   if (x.lvl != y.lvl) return ((x.isend ? 1 : 0) <= (y.isend ? 1 : 0) ?
  //   x.lvl < y.lvl : x.lvl > y.lvl); return (x.isend ? 1 : 0) < (y.isend ? 1 :
  //   0);
  // }

  struct splitdata;
  struct noteobj;
  typedef boost::ptr_list<noteobj> noteobjlisttype;
  typedef noteobjlisttype::const_iterator noteobjlist_constit;
  typedef noteobjlisttype::iterator noteobjlist_it;

  struct usertup {
    int lvl;
    fomus_rat o1, o2;
    usertup(const int lvl, const fomus_rat& o1, const fomus_rat& o2)
        : lvl(lvl), o1(o1), o2(o2) {}
  };

  struct noteobjlist : noteobjlisttype {
    std::set<userpt> pts;
    std::vector<divrules_range> excl;
    std::map<int, fomus_rat> begs;
    std::vector<usertup> tups;
    int llvl;
    noteobjlist() : noteobjlisttype(), llvl(std::numeric_limits<int>::max()) {}
#ifndef NDEBUG
    void isempty() const {
      assert(pts.empty());
      assert(excl.empty());
      assert(begs.empty());
      assert(tups.empty());
    }
#endif
    void donotes();
    void insertbpt(const fomus_rat& o, const int lvl) {
      pts.insert(userpt(o, false, lvl));
      std::map<int, fomus_rat>::iterator i(begs.find(lvl));
      if (i != begs.end()) {
        tups.push_back(usertup(
            lvl, o,
            module_makerat(std::numeric_limits<fomus_int>::min() + 1, 1)));
        i->second = o;
      } else
        begs.insert(std::map<int, fomus_rat>::value_type(lvl, o));
    }
    void insertept(const fomus_rat& o, const int lvl) {
      pts.insert(userpt(o, true, lvl));
      std::map<int, fomus_rat>::iterator i(begs.find(lvl));
      if (i != begs.end()) {
        tups.push_back(usertup(lvl, i->second, o));
        begs.erase(i);
      } else
        tups.push_back(usertup(
            lvl, module_makerat(std::numeric_limits<fomus_int>::max(), 1), o));
      if (lvl < llvl)
        llvl = lvl;
    }
    void clearfrom() {
      for (std::map<int, fomus_rat>::iterator i(begs.lower_bound(llvl));
           i != begs.end();) {
        tups.push_back(usertup(
            i->first, i->second,
            module_makerat(std::numeric_limits<fomus_int>::min() + 1, 1)));
        begs.erase(i++);
      }
      llvl = std::numeric_limits<int>::max();
    }
  };

  typedef boost::ptr_vector<noteobj> noteobjvect;
  typedef noteobjvect::const_iterator noteobjvect_constit;
  typedef noteobjvect::iterator noteobjvect_it;

  typedef boost::ptr_map<int, noteobjlist> noteobjvectmap;
  typedef noteobjvectmap::const_iterator noteobjvectmap_constit;
  typedef noteobjvectmap::iterator noteobjvectmap_it;
  struct meascache _NONCOPYABLE {
    struct divrules_iface rliface;
    module_measobj m;
    comdivsset coms;
    noteobjvectmap nobjs; // put this in a pool!
    std::string divgrp;
    // std::vector<divrules_range> exclvect;
    meascache(const divrules_data& rlifacedata, const module_measobj m);
    module_noteobj sticknotes(module_noteobj n, splitdata& sd);
    ~meascache() {
      if (rliface.moddata)
        rliface.free_moddata(rliface.moddata);
    }
    bool samedivsas(const meascache& as) const {
      return coms == as.coms;
    }
  };
  // inline bool operator<(const meascache& x, const meascache& y) {
  //   return boost::algorithm::iequals(x.divgrp, y.divgrp) ? x.cnt < y.cnt :
  //   boost::algorithm::ilexicographical_compare(x.divgrp, y.divgrp);
  // }

  int setsimid, ddallowedid, dddallowedid, tiescoreid, dotscoreid,
      dotdotscoreid, dotdotdotscoreid,
      // tupletscoreid, //preferredtupsizeid,
      divrulesmodid, tupdistmodid, enginemodid, dotnotelevelid,
      doubledotnotelevelid, shortlongshortnotelevelid, syncopatednotelevelid,
      rlmodid, basescoreid, grouptogid, tupletbasescoreid, forcetupsizeid,
      forcetupratid, rightsizetupletscoreid, bigtupletnumscoreid,
      tupletrestscoreid, samedurtupletscoreid, avedurscoreid,
      smalltupletscoreid, simptupletscoreid, beatdivid, prefmeasdivscoreid,
      weirdtupdurscoreid; // ids for settings

  // SPLITDATA module data object
  struct splitdata _NONCOPYABLE {
    divsearch_api api;
    struct divrules_data rlifacedata; // template--root rules are in msrs
    boost::ptr_vector<meascache> msrs;
    comdivssetview comss;
    int count;
    boost::ptr_map<std::string,
                   boost::ptr_vector<meascache, boost::view_clone_allocator>>
        vects;
    splitdata() {
      rlifacedata.dotnotelvl_setid = dotnotelevelid;
      rlifacedata.dbldotnotelvl_setid = doubledotnotelevelid;
      rlifacedata.slsnotelvl_setid = shortlongshortnotelevelid;
      rlifacedata.syncnotelvl_setid = syncopatednotelevelid;
#ifndef NDEBUG
      count = 0;
#endif
    }
    divsearch_node getroot();
  };

  meascache::meascache(const divrules_data& rlifacedata, const module_measobj m)
      : m(m), divgrp(module_setting_sval(m, setsimid)) {
    rliface.data = rlifacedata;
    rliface.data.meas = m;
    const char* rlmod = module_setting_sval(m, divrulesmodid);
    assert(rlmod);
    module_get_auxiface(rlmod, DIVRULES_INTERFACEID, &rliface);
    struct module_list v(rliface.get_initdivs(rliface.moddata));
    for (module_value *j = v.vals, *je = v.vals + v.n; j != je; ++j) {
      assert(j->type == module_list);
      initdivsvect* divs;
      coms.insert(divs = new initdivsvect);
      for (module_value *k = j->val.l.vals, *ke = j->val.l.vals + j->val.l.n;
           k != ke; ++k) {
        assert(k->type != module_list);
        divs->push_back(GET_R(*k));
      }
    }
  }

  // ------------------------------------------------------------------------------------------------------------------------
  // NOTE NODE OBJECTS
  // ------------------------------------------------------------------------------------------------------------------------
  struct scoped_div {
    const divrules_iface& rliface;
    const divrules_div div;
    scoped_div(const divrules_iface& rliface, const divrules_div div)
        : rliface(rliface), div(div) {
      assert(div);
    }
    ~scoped_div() {
      rliface.free_node(rliface.moddata, div);
    }
    divrules_div get() const {
      return div;
    }
  };

  // struct noredcmp:std::binary_function<const fomus_rat&, const fomus_rat&,
  // bool> {
  //   bool operator()(const fomus_rat& x, const fomus_rat& y) const {
  //     if (x.num != y.num) return x.num < y.num;
  //     if (x.den != y.den) return x.den < y.den;
  //     return false;
  //   }
  // };

  // stores all tuplets that appear in the section, so we can penalize tuplets
  // that are different sizes
  struct tscntset {
    boost::ptr_map<const fomus_rat,
                   std::map<fomus_rat, tscntset> /*, noredcmp*/>*
        s; // tscntsets are indexed by rat then duration
    fomus_float pen, dpen;
    tscntset() : s(0) {} // pen in base is set afterwards
    tscntset(const fomus_float& pen, const fomus_float& dpen)
        : s(0), pen(pen), dpen(dpen) {}
    tscntset(const tscntset& x) : s(x.s), pen(x.pen) {}
    ~tscntset() {
      delete s;
    }
    tscntset* insert(const fomus_rat& rat, const fomus_rat& dur,
                     const fomus_float pen0, const fomus_float dpen0) {
      if (!s)
        s = new boost::ptr_map<const fomus_rat,
                               std::map<fomus_rat, tscntset> /*, noredcmp*/>;
      boost::ptr_map<const fomus_rat,
                     std::map<fomus_rat, tscntset> /*, noredcmp*/>::iterator
          i(s->find(rat));
      if (i == s->end())
        i = s->insert(rat, new std::map<fomus_rat, tscntset>).first;
      return &(i->second
                   ->insert(std::map<fomus_rat, tscntset>::value_type(
                       dur, tscntset(pen0, dpen0)))
                   .first->second);
    }
    void adds(fomus_float& sc, const fomus_rat& mdur) const {
      if (s) {
        for (boost::ptr_map<const fomus_rat,
                            std::map<fomus_rat, tscntset> /*, noredcmp*/>::
                 const_iterator i(s->begin());
             i != s->end(); ++i) {
          if (i->second->size() > 1) {
            sc += pen * (i->second->size() -
                         1); // penalty for ea. additional sized tuplet
            DBG("tscntset penalty = " << pen * (i->second->size() - 1)
                                      << std::endl);
          }
          for (std::map<fomus_rat, tscntset>::const_iterator
                   j(i->second->begin()),
               je(i->second->end());
               j != je; ++j) {
            assert(mdur > (fomus_int) 0);
            if (mdur > (fomus_int) 0) {
              fomus_int x((mdur / j->first).den);
              if (x > 1)
                sc += dpen * (x - 1);
            }
            DBG("tscnt next level" << std::endl);
            j->second.adds(sc, j->first);
            DBG("tscnt done" << std::endl);
          }
        }
      }
    }
  };

  struct tupcntset {
    std::set<fomus_rat> offs, boffs;
    fomus_float pen, spen;
    int num; // numerator of tuplet--number of non-grace time offsets should be
             // close to this
    fomus_rat etim;
    // fomus_rat avedur;
    tupcntset(const fomus_rat& etim, const fomus_float pen,
              const fomus_float spen,
              const int num /*, const fomus_rat& avedur*/)
        : pen(pen), spen(spen), num(num), etim(etim) /*, avedur(avedur)*/ {
      boffs.insert(etim);
    }
    void insert(const fomus_rat& noff, const bool notie) {
      if (noff < etim) {
        offs.insert(noff);
        if (notie)
          boffs.insert(noff);
      }
    } // insert note time offset
    void adds(fomus_float& user, fomus_float& sc, const bool iscomp) const {
      if (!offs.empty()) {
        std::set<fomus_rat> szs;
        for (std::set<fomus_rat>::const_iterator i(boost::next(offs.begin()));;
             ++i) {
          szs.insert((i == offs.end() ? etim : *i) - *boost::prior(i));
          if (i == offs.end())
            break;
        }
        if (user >= USERPEN)
          goto OK;
        for (std::set<fomus_rat>::const_iterator i(boost::next(boffs.begin()));
             i != boffs.end(); ++i) {
          if (!isexpof2((iscomp ? ((*i - *boost::prior(i)) * (fomus_int) 3)
                                : (*i - *boost::prior(i)))
                            .den)) {
            goto OK;
          }
        }
        user = USERPEN;
      OK:
        sc += pen
                  // * (std::max((offs.size() >= num
                  // 	      ? offs.size() - num * 3 / (fomus_float)2
                  // 	      : (num * 2 + 2) / (fomus_float)3 - offs.size()),
                  // (fomus_float)0)
                  //    / num)
                  // * (num / div2(num));
                  * ((fomus_int) offs.size() >= num ? offs.size() - num
                                                    : num - (offs.size() + 1)) *
                  num / div2(num) +
              szs.size() * spen;
      }
    }
    void removerange(const fomus_rat& t1, const fomus_rat& t2) {
      offs.erase(offs.upper_bound(t1), offs.upper_bound(t2));
    }
  };
  typedef boost::ptr_vector<boost::ptr_map<const fomus_rat, tupcntset>>
      tupcntmap; // indexed by lvl/time

  inline void addtupcnt(tupcntmap& tscnt, const int lvl, const fomus_rat& off,
                        const bool notie) {
    if (lvl < (int) tscnt.size()) {
      boost::ptr_map<const fomus_rat, tupcntset>& mp(tscnt[lvl]);
      boost::ptr_map<const fomus_rat, tupcntset>::iterator i(
          mp.upper_bound(off));
      if (i != mp.begin()) {
        assert(!mp.empty());
        assert(boost::prior(i)->first <= off);
        boost::prior(i)->second->insert(
            off, notie); // the insert function won't insert if >= etim
      }
    }
  }
  inline void addtupcnttup(tupcntmap& tscnt, const int lvl,
                           const fomus_rat& off, const fomus_rat& eoff,
                           const fomus_float pen, const fomus_float spen,
                           const int num) {
    if (lvl + 1 > (int) tscnt.size())
      tscnt.resize(lvl + 1);
    boost::ptr_map<const fomus_rat, tupcntset>& mp(tscnt[lvl]);
    boost::ptr_map<const fomus_rat, tupcntset>::iterator i(mp.find(off));
    if (i == mp.end())
      mp.insert(off, new tupcntset(eoff, pen, spen,
                                   num /*, (eoff - off) / (fomus_int)num*/));
  }
  inline void fixtupcnts(tupcntmap& tscnt) {
    assert(!tscnt.empty());
    for (tupcntmap::iterator j(boost::next(tscnt.begin())); j < tscnt.end();
         ++j) {
      tupcntmap::iterator i(boost::prior(j));
      for (boost::ptr_map<const fomus_rat, tupcntset>::iterator ii(i->begin());
           ii != i->end(); ++ii) {
        for (boost::ptr_map<const fomus_rat, tupcntset>::iterator jj(
                 j->begin());
             jj != j->end(); ++jj) {
          DBG("tuprange " << ii->first << ", " << ii->second->etim
                          << "  removing " << jj->first << ", "
                          << jj->second->etim << std::endl);
          ii->second->removerange(jj->first, jj->second->etim);
        }
      }
    }
  }

  // fomus_int dlim = 32;

  // local note object representation
  struct noteobj {
    const fomus_rat o1,
        o2; // begin and end offsets, subdivision of note in `no'
    const divrules_iface& rliface;
    module_noteobj no;
    boost::shared_ptr<scoped_div> rule;
    std::set<int> tupbegs, tupends,
        notups; // user specifies tuplet begin or end
    noteobj(const divrules_iface& rliface, boost::shared_ptr<scoped_div>& rule,
            const fomus_rat& o1, const fomus_rat& o2, const module_noteobj no)
        : o1(o1), o2(o2), rliface(rliface), no(no), rule(rule) {}
    noteobj(const noteobj& x, boost::shared_ptr<scoped_div>& rule,
            const fomus_rat& o1, const fomus_rat& o2)
        : o1(o1), o2(o2), rliface(x.rliface), no(x.no), rule(rule) {
      if (!x.tupbegs.empty()) {
        tupbegs.insert(x.tupbegs.begin(), x.tupbegs.end());
      } else {
        notups.insert(x.notups.begin(), x.notups.end());
      }
    }
    noteobj(const noteobj& x, boost::shared_ptr<scoped_div>& rule,
            const fomus_rat& o1, const fomus_rat& o2, const int)
        : o1(o1), o2(o2), rliface(x.rliface), no(x.no), rule(rule) {
      if (!x.tupends.empty()) {
        tupends.insert(x.tupends.begin(), x.tupends.end());
      } else {
        notups.insert(x.notups.begin(), x.notups.end());
      }
    }
    virtual ~noteobj() {}
    bool iscomp() const {
      return rliface.iscompound(rliface.moddata);
    }
    virtual std::pair<noteobj*, noteobj*>
    split(const fomus_rat& o, boost::shared_ptr<scoped_div>& rule0) {
      assert(false);
    }
    virtual fomus_float basescore(fomus_float& user, tupcntmap& tscnt,
                                  tscntset& tscnt2, const bool hasbeg,
                                  const bool hasend) const {
      assert(false);
    }
    virtual noteobj* clone() const {
      assert(false);
    }
    virtual bool isvaliddup() const {
      assert(false);
    }
    void setrule(boost::shared_ptr<scoped_div>& rule0) {
      rule = rule0;
    }
    void basescoretups(fomus_float& sc,
                       /*const fomus_rat& du,*/ fomus_float& user,
                       tupcntmap& tscnt, tscntset& tscnt2, const bool hasbeg,
                       const bool hasend) const;
    void inittscnt(tupcntmap& tscnt, const fomus_rat& end, bool& hasbeg,
                   bool& hasend) const;
    int getntuplvls() const {
      assert(rule.get());
      return rliface.ntupletlevels(rliface.moddata, rule->get());
    }
    fomus_rat gettuplet(const int l) const {
      assert(rule.get());
      return rliface.tuplet(rliface.moddata, rule->get(), l);
    }
    bool gettupb(const int l) const {
      assert(rule.get());
      return rliface.istupletbegin(rliface.moddata, rule->get(), l);
    }
    bool gettupe(const int l) const {
      assert(rule.get());
      return rliface.istupletend(rliface.moddata, rule->get(), l);
    }
    fomus_rat getfulldur(const int l) const {
      assert(rule.get());
      return rliface.leveldur(rliface.moddata, rule->get(), l);
    }
    bool maxo2(fomus_rat& off) const {
      if (o2 > off)
        off = o2;
      return (o2 - o1 <=
              module_makerat(1, module_setting_ival(no, beatdivid) * 2));
    }
    void dotups(const usertup& ut) {
      assert(ut.lvl >= 0);
      if (ut.o1 == o1) {
        tupbegs.insert(ut.lvl);
        DBG("TUPBEG at " << ut.o1 << "--" << ut.o2 << " lvl " << ut.lvl
                         << std::endl);
      }
      if (ut.o2 == o2) {
        tupends.insert(ut.lvl);
        DBG("TUPEND at " << ut.o1 << "--" << ut.o2 << " lvl " << ut.lvl
                         << std::endl);
      }
      if (o1 > ut.o1 && o2 < ut.o2) {
        notups.insert(ut.lvl);
        DBG("NOTUP at " << ut.o1 << "--" << ut.o2 << " lvl " << ut.lvl
                        << std::endl);
      }
    }
    virtual bool istiedl() const {
      return false;
    }
  };
  inline noteobj* new_clone(const noteobj& t) {
    return t.clone();
  }
  struct note : public noteobj {
    const bool tl, tr; // ties
    bool ddallowed, dddallowed;
    fomus_float tiescore;
    note(const divrules_iface& rliface, boost::shared_ptr<scoped_div>& rule,
         const fomus_rat& o1, const fomus_rat& o2, const bool tl, const bool tr,
         const module_noteobj no)
        : noteobj(rliface, rule, o1, o2, no), tl(tl), tr(tr) {
      ddallowed = module_setting_ival(no, ddallowedid);
      dddallowed = module_setting_ival(no, dddallowedid);
      tiescore = module_setting_fval(no, tiescoreid) / 2;
    }
    note(const note& x, boost::shared_ptr<scoped_div>& rule,
         const fomus_rat& o1, const fomus_rat& o2, const bool tl, const bool tr)
        : noteobj(x, rule, o1, o2), tl(tl), tr(tr), ddallowed(x.ddallowed),
          dddallowed(x.dddallowed), tiescore(x.tiescore) {
      // ddallowed = module_setting_ival(no, ddallowedid);
      // dddallowed = module_setting_ival(no, dddallowedid);
      // tiescore = module_setting_fval(no, tiescoreid) / 2;
    }
    note(const note& x, boost::shared_ptr<scoped_div>& rule,
         const fomus_rat& o1, const fomus_rat& o2, const bool tl, const bool tr,
         const int)
        : noteobj(x, rule, o1, o2, 0), tl(tl), tr(tr), ddallowed(x.ddallowed),
          dddallowed(x.dddallowed), tiescore(x.tiescore) {
      // ddallowed = module_setting_ival(no, ddallowedid);
      // dddallowed = module_setting_ival(no, dddallowedid);
      // tiescore = module_setting_fval(no, tiescoreid) / 2;
    }
    note(const divrules_iface& rliface, boost::shared_ptr<scoped_div>& rule,
         const fomus_rat& o1, const fomus_rat& o2, const bool tl, const bool tr,
         const bool ddallowed, const bool dddallowed, const module_noteobj no)
        : noteobj(rliface, rule, o1, o2, no), tl(tl), tr(tr),
          ddallowed(ddallowed), dddallowed(dddallowed) {}
    std::pair<noteobj*, noteobj*> split(const fomus_rat& o,
                                        boost::shared_ptr<scoped_div>& rule0) {
      boost::shared_ptr<scoped_div> r0;
      return std::pair<noteobj*, noteobj*>(
          new note(*this, rule0, o1, o, tl, true),
          new note(*this, r0, o, o2, true, tr,
                   0)); // rule belongs to first note only--this one should have
                        // rule given to it later
    }
    bool isvaliddup() const;
    fomus_float basescore(fomus_float& user, tupcntmap& tscnt, tscntset& tscnt2,
                          const bool hasbeg, const bool hasend) const;
    noteobj* clone() const {
      return new note(*this);
    }
    bool istiedl() const {
      return tl;
    }
  };
  bool note::isvaliddup() const {
    DBG("N");
#ifndef NDEBUGOUT
    if (!rule.get())
      DBG("norule ");
#endif
    if (!rule.get())
      return false;
    DBG('(' << o1 << ',' << o2 - o1 << ',' << tl << ',' << tr << ")ru("
            << rliface.time(rliface.moddata, rule->get()) << ','
            << rliface.dur(rliface.moddata, rule->get()) << ','
            << rliface.tieleftallowed(rliface.moddata, rule->get()) << ','
            << rliface.tierightallowed(rliface.moddata, rule->get()) << ") ");
    fomus_rat d1(o2 - o1);
    if (d1 != rliface.dur(rliface.moddata, rule->get()) // <-- note matches rule
        || (tl && !rliface.tieleftallowed(rliface.moddata, rule->get())) ||
        (tr && !rliface.tierightallowed(rliface.moddata, rule->get())))
      return false;
    fomus_rat d(d1 * rliface.durmult(rliface.moddata, rule->get()));
    DBG("effectivedur = " << d << " ");
    if (rliface.iscompound(rliface.moddata)) {
      if (d1 <
          (fomus_int) 1 /*rliface.issmall(rliface.moddata, rule->get())*/) {
        if (rliface.ntupletlevels(rliface.moddata, rule->get()) <= 0)
          d = d * module_makerat(3, 2);
        goto IVSKIP;
      }
      return isexpof2(d);
    } else {
    IVSKIP:
      return (isexpof2(d) || isexpof2(d * module_makerat(4, 3)) ||
              (ddallowed &&
               (isexpof2(d * module_makerat(8, 7)) ||
                (dddallowed && isexpof2(d * module_makerat(16, 15))))));
    }
  }
  fomus_float note::basescore(fomus_float& user, tupcntmap& tscnt,
                              tscntset& tscnt2, const bool hasbeg,
                              const bool hasend) const {
    fomus_float sc =
        module_setting_fval(no, basescoreid); // 1 for the note itself
    if (tr)
      sc += tiescore; // extra score for a tie
    if (tl)
      sc += tiescore;
    fomus_rat d1(o2 - o1);
    fomus_rat d(rule.get() ? d1 * rliface.durmult(rliface.moddata, rule->get())
                           : d1);
    if (rliface.iscompound(rliface.moddata) && d1 < (fomus_int) 1 &&
        rliface.ntupletlevels(rliface.moddata, rule->get()) <= 0)
      d = d * module_makerat(3, 2);
    DBG("d=" << d << ',');
    if (!isexpof2(d)) { // extra scores for dots *** why !ddallowed?????
      if (isexpof2(d * module_makerat(4, 3))) {
        DBG("DOTSCORE");
        sc += module_setting_fval(no, dotscoreid);
      } else if (isexpof2(d * module_makerat(8, 7))) {
        DBG("DOTDOTSCORE");
        sc += module_setting_fval(no, dotdotscoreid);
      } else if (isexpof2(d * module_makerat(16, 15))) {
        DBG("DOTDOTDOTSCORE");
        sc += module_setting_fval(no, dotdotdotscoreid);
      }
    }
    if (rule.get())
      basescoretups(sc, /*d1,*/ user, tscnt, tscnt2, hasbeg, hasend);
    return sc;
  }
  inline void noteobj::inittscnt(tupcntmap& tscnt, const fomus_rat& end,
                                 bool& hasbeg, bool& hasend) const {
    if (!hasbeg)
      hasbeg = !tupbegs.empty();
    if (!hasend)
      hasend = !tupends.empty();
    int n = rliface.ntupletlevels(rliface.moddata, rule->get());
    for (int i = 0; i < n; ++i) {
      if (rliface.istupletbegin(rliface.moddata, rule->get(), i)) {
        fomus_rat e(o1 + rliface.leveldur(rliface.moddata, rule->get(), i));
        if (e <= end) {
          addtupcnttup(
              tscnt, i, o1, e, module_setting_fval(no, rightsizetupletscoreid),
              module_setting_fval(no, simptupletscoreid),
              rliface.tuplet(rliface.moddata, rule->get(), i).num); // the ratio
        }
      }
    }
  }
  void noteobj::basescoretups(fomus_float& sc, fomus_float& user,
                              tupcntmap& tscnt, tscntset& tscnt2,
                              const bool hasbeg,
                              const bool hasend) const { // for a noteobj
    int n = rliface.ntupletlevels(
        rliface.moddata, rule->get()); // extra score for ea. tuplet, multiplied
                                       // by distance from "preferred" size
    if (n > 0) {
      fomus_float bs = module_setting_fval(no, tupletbasescoreid);
      module_value pr(module_setting_val(no, forcetupratid));
      module_value fp(module_setting_val(no, forcetupsizeid));
      fomus_float btn = module_setting_fval(no, bigtupletnumscoreid);
      module_value *xr = pr.val.l.vals,
                   *xre = pr.val.l.vals + pr.val.l.n; // force ratio
      module_value *xf = fp.val.l.vals,
                   *xfe = fp.val.l.vals + fp.val.l.n; // force dur
      bool nt = (o1 == module_time(no));
      tscntset* tc = &tscnt2;
      fomus_float tpn(module_setting_fval(no, samedurtupletscoreid));
      fomus_float dtpn(module_setting_fval(no, weirdtupdurscoreid));
      fomus_rat avd(o2 - o1);
      // if (!isexpof2((rliface.iscompound(rliface.moddata) ? (avd *
      // (fomus_int)3) : avd).den)) needtup = true;
      fomus_rat rdc = {1, 1};
      fomus_float rdcs = module_setting_fval(no, smalltupletscoreid);
      for (int i = 0; i < n; ++i, ++xr) { // i is the level
        fomus_rat u(
            rliface.leveldur(rliface.moddata, rule->get(),
                             i)); // duration of tuplet, according to rliface
        assert(u > (fomus_int) 0);
        fomus_rat rt(
            rliface.tuplet(rliface.moddata, rule->get(), i)); // the ratio
        DBG("TUPRAT<" << rt << ">");
        avd = avd * rt;
        tc = tc->insert(rt, u, tpn, dtpn);
        bool t1 = rliface.istupletbegin(rliface.moddata, rule->get(), i);
        if (t1 || nt)
          addtupcnt(tscnt, i, o1, t1 || !istiedl());
        bool t2 = rliface.istupletend(rliface.moddata, rule->get(), i);
        DBG("[tim=" << o1 << ": "
                    << "isuserbeg="
                    << (tupbegs.find(i) != tupbegs.end() ||
                        tupbegs.find(-1) != tupbegs.end())
                    << " isuserend="
                    << (tupends.find(i) != tupends.end() ||
                        tupends.find(-1) != tupends.end())
                    << " isusernotup="
                    << (notups.find(i) != notups.end() ||
                        notups.find(-1) != notups.end())
                    << " isbeg=" << t1 << " isend=" << t2);
        if (user < USERPEN && ((!t1 && (tupbegs.find(i) != tupbegs.end() ||
                                        tupbegs.find(-1) != tupbegs.end())) ||
                               (!t2 && (tupends.find(i) != tupends.end() ||
                                        tupends.find(-1) != tupends.end())) ||
                               (((t1 && !hasbeg) || (t2 && !hasend)) &&
                                (notups.find(i) != notups.end() ||
                                 notups.find(-1) != notups.end())))) {
          user = USERPEN; // sc += s * 16 /** di*/;
          DBG(" PENALTY");
        }
        DBG(']');
        sc += bs;
        if (u > (fomus_int) 1) {
          fomus_rat d(module_makerat_reduce(
              rt.den, module_makerat_reduce(rt.num, rt.den).den));
          if (d > (fomus_int) 1) {
            sc += (d - (fomus_int) 1) * btn;
          }
        }
        if (user < USERPEN && xr<xre&& * xr>(fomus_int) 0) {
          switch (xr->type) {
          case module_int:
            if (xr->val.i != rt.num)
              user = USERPEN;
            break;
          case module_rat:
            if (xr->val.r != rt)
              user = USERPEN;
            break;
#ifndef NDEBUG
          default:
            assert(false);
#endif
          }
        }
        if (user < USERPEN && xf < xfe) {
          fomus_rat r(GET_R(*xf)); // force duration
          if (r > (fomus_int) 0 && r != u)
            user = USERPEN;
        }
        if (u < rdc)
          sc += ((rdc / u) - (fomus_int) 1) * rdcs;
        rdc = rdc / rt;
      }
      fomus_int d = maxdiv2(avd.den);
      sc += module_makerat(d >= avd.den ? d : d * 2, maxdiv2(avd.num)) *
            module_setting_fval(no, avedurscoreid);
    }
  }

  inline void noteobjlist::donotes() {
    for (std::vector<usertup>::const_iterator i(tups.begin()); i != tups.end();
         ++i) {
      DBG("DOTUPS LOOP" << std::endl);
      for (noteobjlisttype::iterator n(begin()); n != end(); ++n)
        n->dotups(*i);
    }
    begs.clear();
    tups.clear();
  }

  struct rest : public noteobj {
    rest(const divrules_iface& rliface, boost::shared_ptr<scoped_div>& rule,
         const fomus_rat& o1, const fomus_rat& o2, const module_noteobj no)
        : noteobj(rliface, rule, o1, o2, no) {}
    rest(const rest& x, boost::shared_ptr<scoped_div>& rule,
         const fomus_rat& o1, const fomus_rat& o2)
        : noteobj(x, rule, o1, o2) {}
    rest(const rest& x, boost::shared_ptr<scoped_div>& rule,
         const fomus_rat& o1, const fomus_rat& o2, const int)
        : noteobj(x, rule, o1, o2, 0) {}
    std::pair<noteobj*, noteobj*> split(const fomus_rat& o,
                                        boost::shared_ptr<scoped_div>& rule0) {
      boost::shared_ptr<scoped_div> r0;
      return std::pair<noteobj*, noteobj*>(new rest(*this, rule0, o1, o),
                                           new rest(*this, r0, o, o2, 0));
    }
    bool isvaliddup() const;
    fomus_float basescore(fomus_float& user, tupcntmap& tscnt, tscntset& tscnt2,
                          const bool hasbeg, const bool hasend) const;
    noteobj* clone() const {
      return new rest(*this);
    }
  };
  bool rest::isvaliddup() const {
    DBG("R");
#ifndef NDEBUGOUT
    if (!rule.get())
      DBG("norule ");
#endif
    if (!rule.get())
      return false;
    if (rliface.isnoteonly(rliface.moddata, rule->get()))
      return false; // moved from basescore
    DBG('(' << o1 << ',' << o2 - o1 << ")ru("
            << rliface.time(rliface.moddata, rule->get()) << ','
            << rliface.dur(rliface.moddata, rule->get()) << ") ");
    fomus_rat d1(o2 - o1);
    if (d1 != rliface.dur(rliface.moddata, rule->get()))
      return false;
    fomus_rat d(d1 * rliface.durmult(rliface.moddata, rule->get()));
    DBG("effectivedur = " << d << " ");
    if (rliface.iscompound(rliface.moddata) && d1 < (fomus_int) 1 &&
        rliface.ntupletlevels(rliface.moddata, rule->get()) <= 0)
      d = d * module_makerat(3, 2);
    return isexpof2(d);
  }
  inline fomus_float rest::basescore(fomus_float& user, tupcntmap& tscnt,
                                     tscntset& tscnt2, const bool hasbeg,
                                     const bool hasend) const {
    fomus_float sc = module_setting_fval(no, basescoreid);
    if (rule.get()) {
      if (rliface.ntupletlevels(rliface.moddata, rule->get()) > 0)
        sc += module_setting_fval(no, tupletrestscoreid);
      basescoretups(sc, user, tscnt, tscnt2, hasbeg, hasend);
    }
    return sc;
  }

  struct rangest {
    fomus_rat time1, time2;
    rangest() : time1(module_makerat(-1, 1)), time2(module_makerat(-1, 1)) {}
    void getrange(const int l, std::vector<divrules_range>& ra) const {
      if (time1 >= (fomus_int) 0 && time2 >= (fomus_int) 0) {
        divrules_range r = {(time1 < (fomus_int) 0 ? time2 : time1),
                            (time2 < (fomus_int) 0 ? time1 : time2), l};
        ra.push_back(r);
      }
    }
  };

  module_noteobj meascache::sticknotes(module_noteobj n, splitdata& sd) {
#ifndef NDEBUG
    for (noteobjvectmap_constit i(nobjs.begin()); i != nobjs.end(); ++i)
      i->second->isempty(); // assertion
#endif
    if (n)
      goto STICKFIRST;
    while (true) {
      n = module_nextnote();
      if (!n)
        break;
#ifndef NDEBUG
      ++sd.count;
#endif
    STICKFIRST:
      if (module_meas(n) != m)
        break;
      boost::shared_ptr<scoped_div> r0; // null scoped_div
      int v = module_voice(n);
      noteobjvectmap_it i(nobjs.find(v));
      noteobjlist* l;
      if (i == nobjs.end())
        nobjs.insert(v, l = new noteobjlist);
      else
        l = i->second;
      fomus_rat o(module_time(n)), eo(module_endtime(n));
      l->push_back(module_isnote(n) // TODO: why is it a list and not a vector?
                       ? (noteobj*) new note(rliface, r0, o, eo,
                                             module_istiedleft(n),
                                             module_istiedright(n), n)
                       : (noteobj*) new rest(rliface, r0, o, eo, n));
      module_markslist ml(module_singlemarks(n));
      assert(ml.n >= 0);
      for (const module_markobj *m0(ml.marks), *me(ml.marks + ml.n); m0 < me;
           ++m0) {
        assert(module_markid(*m0) >= 0);
        switch (module_markid(*m0)) {
        case mark_tuplet_begin: {
          module_value n(module_marknum(*m0));
          l->insertbpt(o,
                       (n.type == module_none ? (fomus_int) 0 : GET_I(n) - 1));
          break;
        }
        case mark_tuplet_end: {
          module_value n(module_marknum(*m0));
          l->insertept(eo,
                       (n.type == module_none ? (fomus_int) 0 : GET_I(n) - 1));
          break;
        }
        }
      }
      l->clearfrom();
    }
    for (noteobjvectmap_it nol(nobjs.begin()); nol != nobjs.end(); ++nol) {
      nol->second->donotes();
      std::stack<rangest> st;
      std::set<userpt>& pts(nol->second->pts);
      std::vector<divrules_range>& exclvect(nol->second->excl);
      for (std::set<userpt>::const_iterator i(pts.begin()); i != pts.end();
           ++i) {
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
      while (!st.empty()) {
        st.top().getrange(st.size() - 1, exclvect);
        st.pop();
      }
    }
    return n;
  }

  // ------------------------------------------------------------------------------------------------------------------------
  // DIVIDE NODES
  // ------------------------------------------------------------------------------------------------------------------------
  struct ascore {
    fomus_float init; // 0, 1 or 2 based on how initially divided
    fomus_float
        user; // usually 0, 1 if user has specified some override (tuplets)
    fomus_float sm; // basic sum of elements score
  };
  inline bool operator<(const struct ascore& x, const struct ascore& y) {
    assert(x.init >= 0);
    assert(y.init >= 0);
    assert(x.user >= 1);
    assert(y.user >= 1);
    return (x.sm * std::max(x.init, (fomus_float) 1) * x.user) <
           (y.sm * std::max(y.init, (fomus_float) 1) * y.user);
  }

  // nodes
  struct divbase {
    const int depth;
    ascore score; // container for storing score struct
    noteobjlist notes;
    const initdivsvect*
        initdivs; // this is passed to divrule's GET_ROOT function--save it here
                  // so we know what div of final solution is
    fomus_rat measdur;
#ifndef NDEBUG
    int valid;
#endif
    divbase(const int depth, const initdivsvect* initdivs,
            const fomus_rat& measdur)
        : depth(depth), initdivs(initdivs), measdur(measdur) {
#ifndef NDEBUG
      valid = 12345;
#endif
    } // -1 = brand new top level
    divbase(struct divsearch_andnode& andnode, const int depth,
            const initdivsvect* initdivs, const fomus_rat& measdur)
        : depth(depth), initdivs(initdivs), measdur(measdur) { // for assembling
      for (divsearch_node *a = andnode.nodes, *ae = andnode.nodes + andnode.n;
           a < ae; ++a) {
        DBG("&&& FROM ");
#ifndef NDEBUGOUT
        ((divbase*) *a)->dumpnotes();
#endif
        notes.insert(notes.end(), ((divbase*) *a)->notes.begin(),
                     ((divbase*) *a)->notes.end());
      }
      score.init = ((divbase*) *andnode.nodes)->score.init;
#ifndef NDEBUG
      valid = 12345;
#endif
      DBG("&&& TO ");
#ifndef NDEBUGOUT
      dumpnotes();
#endif
    }
#ifndef NDEBUG
    bool isvalid() const {
      return valid == 12345;
    }
#endif
    virtual void expand(splitdata& data,
                        const divsearch_ornode_ptr ornode) const = 0;
    virtual bool isleaf(const splitdata& data) = 0;
    virtual ~divbase() {}
#ifndef NDEBUG
    virtual int whichone() const = 0;
    void dumpnotes() const {
      assert(isvalid());
#ifndef NDEBUGOUT
      for (noteobjlist_constit i(notes.begin()); i != notes.end(); ++i) {
        module_noteobj x = i->no;
        DBG("(p=" << module_part(x) << " v=" << module_voice(x)
                  << " t=" << module_time(x) << " n="
                  << (module_isnote(x) ? module_pitch(x) : module_makerat(0, 1))
                  << ") ");
      }
      DBG(std::endl);
#endif
    }
#endif
    divsearch_score getscore(const splitdata& data);
    void assignnotes(const splitdata& da) const;
    virtual void initdivscore(fomus_float& sc) const {}
  };

  struct divnotes : public divbase NONCOPYABLE {
    const struct divrules_iface& rliface;
    boost::shared_ptr<scoped_div> rule;
    std::vector<divrules_range> excl;
    divnotes(const struct divrules_iface& rliface,
             boost::shared_ptr<scoped_div>& rule, const fomus_rat& o1,
             const fomus_rat& o2, noteobjlist& n, const int depth,
             const fomus_float score1, const initdivsvect* initdivs,
             const fomus_rat& measdur,
             const std::vector<divrules_range>& vect); // EXPAND
    divnotes(const struct divrules_iface& rliface,
             struct divsearch_andnode& andnode, const int depth,
             const initdivsvect* initdivs, const fomus_rat& measdur)
        : divbase(andnode, depth, initdivs, measdur), rliface(rliface) {
    } // for assembling
    void expand(splitdata& data, const divsearch_ornode_ptr ornode) const;
    bool isleaf(const splitdata& data) {
      DBG("ISLEAF?  ");
      bool re =
          hasall(notes.begin(), notes.end(),
                 boost::lambda::bind(&noteobj::isvaliddup, boost::lambda::_1));
      DBG((re ? "*yes*" : "-no-") << std::endl); // yes = is leaf (valid)
      return re;
    }
#ifndef NDEBUG
    virtual int whichone() const {
      return 2;
    }
#endif
  };
  inline void divnotes::expand(splitdata& data,
                               const divsearch_ornode_ptr ornode) const {
    divrules_rangelist exc = {excl.size(), &excl[0]};
    divrules_ornode exp(rliface.expand(rliface.moddata, rule->get(), exc));
    for (divrules_andnode *i = exp.ands, *ie = exp.ands + exp.n; i != ie; ++i) {
      divsearch_andnode_ptr anode = data.api.new_andnode(ornode);
      noteobjlist nts(notes);
      fomus_rat t1(rliface.time(rliface.moddata, *i->divs));
      for (divrules_div *d = i->divs, *de = i->divs + i->n; d != de; ++d) {
        assert(t1 == rliface.time(rliface.moddata, *d));
        fomus_rat t2(rliface.endtime(rliface.moddata, *d));
        boost::shared_ptr<scoped_div> d0(new scoped_div(rliface, *d));
        data.api.push_back(anode,
                           new divnotes(rliface, d0, t1, t2, nts, depth + 1,
                                        score.init, initdivs, measdur,
                                        excl)); // get notes in range t1-t2,
                                                // splitting apart as necessary
        t1 = t2;
      }
      assert(nts.empty());
    }
  }
  divnotes::divnotes(const struct divrules_iface& rliface,
                     boost::shared_ptr<scoped_div>& rule, const fomus_rat& o1,
                     const fomus_rat& o2, noteobjlist& n, const int depth,
                     const fomus_float score1, const initdivsvect* initdivs,
                     const fomus_rat& measdur,
                     const std::vector<divrules_range>& vect)
      : divbase(depth, initdivs, measdur), rliface(rliface), rule(rule) {
    score.init = score1; // need this when things get assembled
    DBG("CREATING DIV " << o1 << "--" << o2 << "  ");
#ifndef NDEBUG
    {
      fomus_rat x = {-1, 1};
      for (noteobjlist_it i(n.begin()); i != n.end(); ++i) {
        assert(i->o1 >= x);
        x = i->o1;
      }
    }
#endif
    for (noteobjlist_it i(n.begin()); i != n.end() && i->o1 < o2;) {
      assert(i->o1 >= o1); // all should be >
      if (i->o2 <= o2) {
        DBG('(' << i->o1 << "," << i->o2 << ')');
        i->setrule(rule);
        notes.transfer(notes.end(), i++, n);
      } else {
        std::pair<noteobj*, noteobj*> spl(i->split(o2, rule));
        DBG('(' << spl.first->o1 << "," << spl.first->o2 << ')');
        notes.push_back(spl.first);
        i = n.erase(i);
        n.insert(i, spl.second);
      }
    }
    DBG(std::endl);
    DBG("  divnote exclusions are like this: [");
    for (std::vector<divrules_range>::const_iterator i(vect.begin());
         i != vect.end(); ++i) {
      if (i->time2 >= o1 && i->time1 <= o2) {
        DBG(i->time1 << "--" << i->time2 << " ");
        excl.push_back(*i);
      }
    }
    DBG("]" << std::endl);
  }
#warning                                                                       \
    "*** don't need to ask libfomus for this (don't need to create/free it) ***"
  struct scoped_modlist : public module_list { // just a generic list
    scoped_modlist(const module_list& l) : module_list(l) {}
    ~scoped_modlist() {
      module_free_list(*this);
    }
  };

  struct divmeas : public divbase NONCOPYABLE {
    const struct divrules_iface& rliface;
    const meascache* meas;
    divmeas(const struct divrules_iface& rliface, const meascache* meas,
            const initdivsvect* initdivs, const fomus_float score1)
        : divbase(0, initdivs, module_dur(meas->m)), rliface(rliface),
          meas(meas) {
      score.init = score1;
    }
    divmeas(const struct divrules_iface& rliface,
            struct divsearch_andnode& andnode, const initdivsvect* initdivs,
            const fomus_rat& measdur)
        : divbase(andnode, 0, initdivs, measdur), rliface(rliface) {
    } // for assembling
    void expand(splitdata& data, const divsearch_ornode_ptr onode) const;
    bool isleaf(const splitdata& data) {
      return false;
    }
#ifndef NDEBUG
    virtual int whichone() const {
      return 1;
    }
#endif
  };

  inline void divmeas::expand(splitdata& data,
                              const divsearch_ornode_ptr onode) const {
    scoped_modlist li(module_new_list(initdivs->size()));
    initdivsvect_constit ii = initdivs->begin();
    for (module_value *i = li.vals, *ie = li.vals + li.n; i != ie; ++i, ++ii) {
      i->type = module_rat;
      i->val.r = *ii;
    }
    assert(li.n > 0);
    divrules_ornode exp(rliface.get_root(
        rliface.moddata, module_time(meas->m),
        li)); // send get_root an initial div list--only need to free the nodes,
              // not the containers in ornode
    for (divrules_andnode *i = exp.ands, *ie = exp.ands + exp.n; i != ie;
         ++i) { // *i is an andnode of the rules
      std::vector<boost::shared_ptr<scoped_div>> ds;
      for (divrules_div *d = i->divs, *de = i->divs + i->n; d != de; ++d)
        ds.push_back(
            boost::shared_ptr<scoped_div>(new scoped_div(rliface, *d)));
      divsearch_andnode_ptr anode = data.api.new_andnode(onode);
      for (noteobjvectmap_constit v(meas->nobjs.begin()), ve(meas->nobjs.end());
           v != ve; ++v) {           // iterator over voices
        noteobjlist nts(*v->second); // nts = notes to divy up
        fomus_rat t1(rliface.time(rliface.moddata, *i->divs));
        for (std::vector<boost::shared_ptr<scoped_div>>::iterator d(ds.begin());
             d != ds.end(); ++d) { // this split level contains initial rules
                                   // divisions x voices
          assert(t1 == rliface.time(rliface.moddata, d->get()->div));
          fomus_rat t2(rliface.endtime(rliface.moddata, d->get()->div));
          data.api.push_back(
              anode,
              new divnotes(rliface, *d, t1, t2, nts, depth + 1, score.init,
                           initdivs, measdur,
                           v->second->excl)); // get notes in range t1-t2,
                                              // splitting apart as necessary
          t1 = t2;
        }
        assert(nts.empty());
      }
    }
  }
  struct divroot2 : public divbase NONCOPYABLE {
    comdivssetview_constit cb, ce;
    boost::ptr_vector<meascache>::const_iterator mb, me;
    divroot2(struct divsearch_andnode& andnode, const initdivsvect* initdivs,
             const fomus_rat& measdur)
        : divbase(andnode, -1, initdivs, measdur) {} // for assembling
    divroot2(const comdivssetview_constit& cb, const comdivssetview_constit& ce,
             const boost::ptr_vector<meascache>::const_iterator& mb,
             const boost::ptr_vector<meascache>::const_iterator& me,
             const fomus_float score1)
        : divbase(-1, 0, module_makerat(0, 1)), cb(cb), ce(ce), mb(mb), me(me) {
      score.init = score1;
    }
    void expand(splitdata& data, const divsearch_ornode_ptr ornode) const {
      for (comdivsset_constit c(cb); c != ce; ++c) {
        divsearch_andnode_ptr a = data.api.new_andnode(
            ornode); // measures must belong to an `and' group
        for (boost::ptr_vector<meascache>::const_iterator i(mb); i != me; ++i)
          data.api.push_back(a, new divmeas(i->rliface, &*i, &*c,
                                            score.init)); // smaller groups
      }
    }
    bool isleaf(const splitdata& data) {
      return false;
    }
#ifndef NDEBUG
    virtual int whichone() const {
      return 0;
    }
#endif
    void initdivscore(fomus_float& sc) const;
  };
  void divroot2::initdivscore(fomus_float& sc) const {
    assert(initdivs);
    assert(!initdivs->empty());
    if (!notes.empty()) {
      fomus_float dsc =
          module_setting_fval(notes.front().no, prefmeasdivscoreid);
      for (initdivsvect_constit i(boost::next(initdivs->begin()));
           i != initdivs->end(); ++i) {
        if (*i > *boost::prior(i))
          sc += dsc;
      }
    }
  }

  inline divsearch_score divbase::getscore(const splitdata& data) {
    DBG("SCORE: ");
    divsearch_score s;
    s.ptr = &score;
    fomus_float us = 0;
    score.sm = 0;
    score.user = 1;
    tscntset tscnt2;
    tupcntmap tscnt;
    fomus_rat o2 = {std::numeric_limits<fomus_int>::min() + 1, 1};
    for (noteobjlist_constit i(notes.begin()); i != notes.end(); ++i) {
      if (i->maxo2(o2))
        us += USERPEN;
    }
    bool hasbeg = false,
         hasend =
             false; // have a userspecified beg or end tuplet in these notes?
    for (noteobjlist_constit i(notes.begin()); i != notes.end(); ++i)
      i->inittscnt(tscnt, o2, hasbeg, hasend);
    for (noteobjlist_constit i(notes.begin()); i != notes.end(); ++i) {
      DBG("(" << i->o1 << "->" << i->o2 << ",");
      fomus_float x = i->basescore(score.user, tscnt, tscnt2, hasbeg, hasend);
      assert(x != std::numeric_limits<fomus_float>::max());
      DBG(x << ") ");
      score.sm += x;
    }
    if (!notes.empty()) {
      tscnt2.pen = module_setting_fval(notes.front().no, samedurtupletscoreid);
      tscnt2.dpen = module_setting_fval(notes.front().no, weirdtupdurscoreid);
      DBG("Before tscnt2 = " << score.sm << std::endl);
      fomus_float x = 0;
      tscnt2.adds(x, measdur);
      score.sm += x * notes.size();
      DBG("After tscnt2 = " << score.sm << std::endl);
    }
    if (!tscnt.empty()) {
      fomus_float x = 0;
      fixtupcnts(tscnt);
      bool cmp = (!notes.empty() && notes.front().iscomp());
      for (tupcntmap::const_iterator i(tscnt.begin()); i != tscnt.end(); ++i) {
        for (boost::ptr_map<const fomus_rat, tupcntset>::const_iterator j(
                 i->begin());
             j != i->end(); ++j) {
          DBG("TUP from " << j->first << " to " << j->second->etim << " has "
                          << j->second->offs.size() << " notes in it w/ num = "
                          << j->second->num << std::endl);
          j->second->adds(score.user, x, cmp);
        }
      }
      score.sm += x * notes.size();
    }
    initdivscore(score.sm);
    score.user += us;
    DBG("Final Score = " << score.sm << std::endl);
    return s;
  }

  struct divroot1 : public divbase NONCOPYABLE {
    boost::ptr_vector<meascache, boost::view_clone_allocator>::const_iterator
        msrsb,
        msrse;
    divroot1(struct divsearch_andnode& andnode, const initdivsvect* initdivs,
             const fomus_rat& measdur)
        : divbase(andnode, -2, initdivs, measdur) {}
    divroot1(const boost::ptr_vector<meascache>::const_iterator& msrsb,
             const boost::ptr_vector<meascache>::const_iterator& msrse)
        : divbase(-2, 0, module_makerat(0, 1)), msrsb(msrsb), msrse(msrse) {}
    void expand(splitdata& data, const divsearch_ornode_ptr ornode) const;
    bool isleaf(const splitdata& data) {
      return false;
    }
#ifndef NDEBUG
    virtual int whichone() const {
      return -1;
    }
#endif
  };

  inline void divroot1::expand(splitdata& data,
                               const divsearch_ornode_ptr ornode) const {
    assert(msrsb != msrse); // has to be at least 1
    fomus_float inimul = module_setting_fval(msrsb->m, grouptogid);
    if (boost::next(msrsb) !=
        msrse) { // if just one measure, don't bother with larger groups
      for (comdivsset_it c(msrsb->coms.begin()), ce(msrsb->coms.end()); c != ce;
           ++c)
        data.comss.insert(&*c); // don't want to transfer them!
      for (boost::ptr_vector<meascache,
                             boost::view_clone_allocator>::const_iterator
               i(boost::next(msrsb));
           i != msrse; ++i) { // en is at least mit + 1
        inplace_set_intersection<comdivssetview_it>(
            data.comss, i->coms.begin(), i->coms.end()); // coms is a set
      }
      data.api.push_back(
          data.api.new_andnode(ornode),
          new divroot2(data.comss.begin(), data.comss.end(), msrsb, msrse, 0));
      divsearch_andnode_ptr a = 0;
      for (boost::ptr_vector<meascache,
                             boost::view_clone_allocator>::const_iterator
               b(msrsb),
           e(msrsb);
           b != msrse;) {
        do
          ++e;
        while (e != msrse && b->samedivsas(*e));
        if (b == msrsb && e == msrse)
          break; // just repeating what's above
        if (!a)
          a = data.api.new_andnode(ornode);
        data.api.push_back(
            a, new divroot2(b->coms.begin(), b->coms.end(), b, e, inimul));
        b = e;
      }
    }
    divsearch_andnode_ptr a = data.api.new_andnode(ornode);
    for (boost::ptr_vector<
             meascache, boost::view_clone_allocator>::const_iterator mit(msrsb);
         mit != msrse; ++mit) {
      data.api.push_back(a, new divroot2(mit->coms.begin(), mit->coms.end(),
                                         mit, boost::next(mit), inimul * 2));
    }
  }

  struct divroot0 : public divbase NONCOPYABLE {
    divroot0(struct divsearch_andnode& andnode, const initdivsvect* initdivs,
             const fomus_rat& measdur)
        : divbase(andnode, -3, initdivs, measdur) {}
    divroot0() : divbase(-3, 0, module_makerat(0, 1)) {}
    void expand(splitdata& data, const divsearch_ornode_ptr ornode) const;
    bool isleaf(const splitdata& data) {
      return false;
    }
#ifndef NDEBUG
    virtual int whichone() const {
      return -2;
    }
#endif
  };

  inline void divroot0::expand(splitdata& data,
                               const divsearch_ornode_ptr ornode) const {
    assert(data.msrs.begin() != data.msrs.end()); // has to be at least 1
    assert(!data.vects.empty());
    divsearch_andnode_ptr a = data.api.new_andnode(ornode);
    for (boost::ptr_map<
             std::string,
             boost::ptr_vector<meascache, boost::view_clone_allocator>>::
             const_iterator i(data.vects.begin());
         i != data.vects.end(); ++i) {
      data.api.push_back(a, new divroot1(i->second->begin(), i->second->end()));
    }
  }

  divsearch_node splitdata::getroot() {
    if (!msrs.empty())
      return 0; // NOT called multiple times
    module_noteobj lnoteobj = 0;
    while (true) {
      module_measobj m = module_nextmeas();
      if (!m)
        break;
      meascache* c;
      msrs.push_back(c = new meascache(rlifacedata, m));
      lnoteobj = c->sticknotes(lnoteobj, *this);
    }
    assert(!msrs.empty());
    assert(!lnoteobj);
    assert(vects.empty());
    for (boost::ptr_vector<meascache>::iterator i(msrs.begin());
         i != msrs.end(); ++i)
      vects[i->divgrp].push_back(&*i);
    return new divroot0();
  }

  void divbase::assignnotes(const splitdata& da) const {
    assert(initdivs);
    assert(whichone() == -2);
    std::set<module_measobj> mss;
    for (noteobjlist::const_iterator i(notes.begin()); i != notes.end(); ++i) {
      module_measobj m = module_meas(i->no);
      if (mss.insert(m).second) {
        module_ratslist idvs = {initdivs->size(), &(*initdivs)[0]};
        divide_assign_initdivs(m, idvs);
        DBG("ASSIGNING INITDIVS: ");
#ifndef NDEBUGOUT
        for (std::vector<fomus_rat>::const_iterator i(initdivs->begin());
             i != initdivs->end(); ++i) {
          DBG(*i << ' ');
        }
#endif
        DBG(std::endl);
      }
      std::vector<divide_tuplet> tu;
      DBG("ASSIGNING SPLIT: note:(" << module_time(i->no) << ','
                                    << module_endtime(i->no) << ") split:("
                                    << i->o1 << ',' << i->o2 << ") tups:");
      for (int tl = 0, te = i->getntuplvls(); tl < te; ++tl) {
        DBG('(' << i->gettuplet(tl).num << '/' << i->gettuplet(tl).den << ','
                << i->gettupb(tl) << ',' << i->gettupe(tl) << ") ");
        struct divide_tuplet u = {i->gettuplet(tl), i->gettupb(tl),
                                  i->gettupe(tl), i->getfulldur(tl)};
        tu.push_back(u);
      }
      DBG(std::endl);
      divide_split sp = {i->no, i->o1, tu.size(), &tu[0]};
      divide_assign_split(m, sp);
    }
    for (boost::ptr_vector<meascache>::const_iterator i(da.msrs.begin());
         i != da.msrs.end(); ++i) {
      if (mss.insert(i->m).second) {
        fomus_rat x = module_dur(i->m);
        module_ratslist idvs = {1, &x};
        divide_assign_initdivs(i->m, idvs);
        DBG("ASSIGNING FULL-MEASURE REST DIV: " << x << std::endl);
      }
    }
    module_noteobj n = 0, ln;
#ifndef NDEBUG
    int count = 0;
#endif
    while (true) {
      ln = n;
      n = module_peeknextnote(n);
      if (ln)
        module_skipassign(ln);
      if (!n)
        break;
      DBG("-------- " << module_time(n) << ' '
                      << (module_isnote(n) ? module_pitch(n)
                                           : module_makerat(0, 1))
                      << std::endl);
#ifndef NDEBUG
      ++count;
#endif
    }
  }

  inline divsearch_node assemble(const splitdata& data,
                                 struct divsearch_andnode& andnode) {
#ifndef NDEBUG
    for (divsearch_node *i = andnode.nodes + 1, *ie = andnode.nodes + andnode.n;
         i < ie; ++i) {
      assert(((divbase*) *i)->depth == ((divbase*) *andnode.nodes)->depth);
    }
#endif
    const int dep = ((divbase*) *andnode.nodes)->depth;
    switch (dep) {
    case -2:
      DBG(std::endl << "&&& assembling to divgrp divroot" << std::endl);
      assert(((divbase*) *andnode.nodes)->whichone() == -1);
      return new divroot0(andnode, ((divbase*) *andnode.nodes)->initdivs,
                          ((divbase*) *andnode.nodes)->measdur);
    case -1: // andnode is full of measdivs
      DBG(std::endl << "&&& assembling to divroot" << std::endl);
      assert(((divbase*) *andnode.nodes)->whichone() == 0);
      return new divroot1(andnode, ((divbase*) *andnode.nodes)->initdivs,
                          ((divbase*) *andnode.nodes)->measdur);
    case 0: // andnode is full of measdivs
      DBG(std::endl << "&&& assembling to subdivroot" << std::endl);
      assert(((divbase*) *andnode.nodes)->whichone() == 1);
      return new divroot2(andnode, ((divbase*) *andnode.nodes)->initdivs,
                          ((divbase*) *andnode.nodes)->measdur);
    case 1: // andnode is full of lowest-level notedivs
      DBG(std::endl << "&&& assembling to divmeas" << std::endl);
      assert(((divbase*) *andnode.nodes)->whichone() == 2);
      return new divmeas(((divnotes*) *andnode.nodes)->rliface, andnode,
                         ((divbase*) *andnode.nodes)->initdivs,
                         ((divbase*) *andnode.nodes)->measdur);
    default: // full of higher-level notedivs
      DBG(std::endl << "&&& assembling to divnotes" << std::endl);
      assert(((divbase*) *andnode.nodes)->depth >= 2);
      assert(((divbase*) *andnode.nodes)->whichone() == 2);
      return new divnotes(((divnotes*) *andnode.nodes)->rliface, andnode,
                          dep - 1, ((divbase*) *andnode.nodes)->initdivs,
                          ((divbase*) *andnode.nodes)->measdur);
    }
  }

  extern "C" {
  union divsearch_score score(void* moddata, divsearch_node node);
  void expand(void* moddata, divsearch_ornode_ptr ornode,
              divsearch_node node); // must expand node and operate on ornode
  divsearch_node assemble(void* moddata, struct divsearch_andnode andnode);
  int score_lt(void* moddata, union divsearch_score x, union divsearch_score y);
  // void free_moddata(void* moddata);
  void free_node(void* moddata, divsearch_node node);
  divsearch_node get_root(void* moddata);
  int is_leaf(void* moddata, divsearch_node node);
  void solution(void* moddata, divsearch_node node);
  const char* diverr(void* moddata);
  }
  union divsearch_score score(void* moddata, divsearch_node node) {
    return ((divbase*) node)->getscore(*(splitdata*) moddata);
  }
  void expand(void* moddata, divsearch_ornode_ptr ornode, divsearch_node node) {
    ((divbase*) node)->expand(*(splitdata*) moddata, ornode);
  }
  divsearch_node
  assemble(void* moddata,
           struct divsearch_andnode
               andnode) { // also, collect timesig-level sub-divisions and merge
                          // with root divisions
    return assemble(*(splitdata*) moddata, andnode);
  }
  int score_lt(void* moddata, union divsearch_score x,
               union divsearch_score y) {
    return *(ascore*) y.ptr < *(ascore*) x.ptr;
  } // less is better
  void free_node(void* moddata, divsearch_node node) {
    delete (divbase*) node;
  }
  divsearch_node get_root(void* moddata) {
    return ((splitdata*) moddata)->getroot();
  }
  int is_leaf(void* moddata, divsearch_node node) {
    return ((divbase*) node)->isleaf(*(splitdata*) moddata);
  }
  void solution(void* moddata, divsearch_node node) {
    ((divbase*) node)->assignnotes(*((splitdata*) moddata));
  }
  const char* diverr(void* moddata) {
    return 0;
  }

  // ------------------------------------------------------------------------------------------------------------------------
  std::string divrulesmodstype;
  std::set<std::string> divrulesmodsset;
  int valid_div_aux(const char* str) {
    return divrulesmodsset.find(str) != divrulesmodsset.end();
  }
  int valid_div(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_div_aux,
                               divrulesmodstype.c_str());
  }
  const char* scoretype = "real>=0";
  int valid_score(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) 0), module_incl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            scoretype);
  }
  const char* preferredtuptype = "rational>=0 | (rational>=0 rational>=0 ...)";
  int valid_preferredtup(const struct module_value val) {
    return module_valid_listofrats(val, -1, -1, module_makerat(0, 1),
                                   module_incl, module_makerat(0, 1),
                                   module_nobound, 0, preferredtuptype);
  }
  std::string tupdistmodstype;
  std::set<std::string> tupdistmodsset;
  int valid_tupdistmod_aux(const char* str) {
    return tupdistmodsset.find(str) != tupdistmodsset.end();
  }
  int valid_tupdistmod(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_tupdistmod_aux,
                               tupdistmodstype.c_str());
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
  const char* dotnoteleveltype = "all|none|top|div";
  std::set<std::string, isiless> dvlvlset;
  int valid_dotnotelevel_aux(const char* str) {
    return dvlvlset.find(str) != dvlvlset.end();
  }
  int valid_dotnotelevel(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_dotnotelevel_aux,
                               dotnoteleveltype);
  }
} // namespace split

using namespace split;

void module_fill_iface(void* moddata, void* iface) {
  ((splitdata*) moddata)->api = ((divsearch_iface*) iface)->api;
  ((divsearch_iface*) iface)->moddata = moddata;
  ((divsearch_iface*) iface)->score = score;
  ((divsearch_iface*) iface)->expand = expand;
  ((divsearch_iface*) iface)->assemble = assemble;
  ((divsearch_iface*) iface)->score_lt = score_lt;
  ((divsearch_iface*) iface)->free_node = free_node;
  ((divsearch_iface*) iface)->get_root = get_root;
  ((divsearch_iface*) iface)->is_leaf = is_leaf;
  ((divsearch_iface*) iface)->solution = solution;
  ((divsearch_iface*) iface)->err = diverr;
};
const char* module_longname() {
  return "Note Divider";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Splits and ties notes according to allowed divisions in each "
         "measure.";
}
enum module_type module_type() {
  return module_moddivide;
}
int module_priority() {
  return 0;
}
int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "mod-div-divrules"; // docscat{rhythmic}
    set->type = module_string;
    set->descdoc =
        "The module used by FOMUS's measure dividing algorithm to search for "
        "valid combinations of metrical divisions and tuplets."
        "  Different modules provide different rules affecting how rhythmic "
        "spellings and note divisions/ties are determined."
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
    divrulesmodid = id;
    break;
  }
  case 1: {
    set->name = "divgroup"; // docscat{instsparts}
    set->type = module_string;
    set->descdoc =
        "An ID used to group together parts or measures that should be "
        "divided, beamed or laid out in a similar manner."
        "  Set this in orchestral part definitions or measures/measure "
        "definitions to define groups of instruments with similar or identical "
        "musical material."
        "  If part `A', for example, has id `grp1' and part `B' also has id "
        "`grp1' and both have a 5/8 meter, then both will be"
        " divided together into the same 3 + 2 or 2 + 3 subdivisions."
        "  By default, all parts belong to the same group and are thus divided "
        "similarly.";
    // set->typedoc = layoutidtype;

    module_setval_string(&set->val, "");

    set->loc = module_locmeasdef;
    // set->valid = valid_layoutid;
    set->uselevel = 2;
    setsimid = id;
    break;
  }
  case 2: {
    set->name = "doubledots"; // docscat{rhythmic}
    set->type = module_bool;
    set->descdoc = "Whether or not to allow notes with double dots.  "
                   "Setting this to `yes' allows double-dotted notes to appear "
                   "in the score while setting this to `no' disallows them.";
    // set->typedoc = divtype;

    module_setval_int(&set->val, 1);

    set->loc = module_locnote;
    // set->valid = valid_div;
    set->uselevel = 2;
    ddallowedid = id;
    break;
  }
  case 3: {
    set->name = "tripledots"; // docscat{rhythmic}
    set->type = module_bool;
    set->descdoc =
        "Whether or not to allow notes with triple dots (adding 7/8 of the "
        "duration)."
        "  Setting this to `yes' allows double-dotted notes to appear in the "
        "score while setting this to `no' disallows them.";
    // set->typedoc = divtype;

    module_setval_int(&set->val, 0);

    set->loc = module_locnote;
    // set->valid = valid_div;
    set->uselevel = 2;
    dddallowedid = id;
    break;
  }
  case 4: {
    set->name = "div-tie-score"; // docscat{rhythmic}
    set->type = module_number;
    set->descdoc = "The score for avoiding excessive note ties (increasing "
                   "this value encourages FOMUS not to use ties).";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    tiescoreid = id;
    break;
  }
  case 5: {
    set->name = "div-dot-score"; // docscat{rhythmic}
    set->type = module_number;
    set->descdoc =
        "The penalty for avoiding augmentation dots on notes (increasing this "
        "value encourages FOMUS not to use augmentation dots).";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 0);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern usedotscoreid
    dotscoreid = id;
    break;
  }
  case 6: {
    set->name = "div-doubledot-score"; // docscat{rhythmic}
    set->type = module_number;
    set->descdoc =
        "The penalty for creating a double dot on a note (increasing this "
        "value encourages FOMUS not to use double augmentation dots).";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 2);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    dotdotscoreid = id;
    break;
  }
  case 7: {
    set->name = "div-tripledot-score"; // docscat{rhythmic}
    set->type = module_number;
    set->descdoc =
        "The penalty for creating a triple dot on a note (increasing this "
        "value encourages FOMUS not to use triple augmentation dots).";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 5);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    dotdotdotscoreid = id;
    break;
  }
  // case 8:
  //   {
  //     set->name = "div-tupletdur-score"; // docs*cat{tuplets}
  //     set->type = module_number;
  //     set->descdoc = "The score for creating a tuplet close to the preferred
  //     span duration given in `tuplet-prefdur'."
  // 	"  Increasing this helps create uniformity in tuplet span durations but
  // also reduces flexibility.";
  //     set->typedoc = scoretype;

  //     module_setval_float(&set->val, 0.5); // was 3

  //     set->loc = module_locnote;
  //     set->valid = valid_score;
  //     set->uselevel = 3; // probably doesn't concern user
  //     tupletscoreid = id;
  //     break;
  //   }
  // case 9:
  //   {
  //     set->name = "tuplet-prefdur"; // docs*cat{tuplets}
  //     set->type = module_list_nums;
  //     set->descdoc =
  // 	"Specifies the preferred duration of a tuplet at each tuplet level (the
  // first list item represents the largest/outermost tuplet level)." 	"  Set
  // each item in the list to a value representing the tuplet duration that
  // should be used most often." 	"  A single value (equivalent to a list of size
  // one) represents the outermost tuplet duration and is probably all that is
  // necessary." 	"  An empty list or zero in any list position specifies no
  // preferrence in tuplet size."
  // 	//"  Size is measured in whatever unit is specified by
  // `mod-div-tupletdur-dist' (TODO: this isn't implemented yet--duration is
  // currently measured in beats)."
  // 	;
  //     set->typedoc = preferredtuptype;

  //     module_setval_list(&set->val, 1);
  //     module_setval_int(set->val.val.l.vals, 2);

  //     set->loc = module_locnote;
  //     set->valid = valid_preferredtup;
  //     set->uselevel = 2;
  //     preferredtupsizeid = id;
  //     break;
  //   }
  case 8: {
    set->name = "div-engine"; // docscat{rhythmic}
    set->type = module_string;
    set->descdoc =
        "Engines provide different types of search functionality to the rest "
        "of FOMUS's modules and are interchangeable."
        "  For example, two of FOMUS's default engines `dynprog' and "
        "`bfsearch' execute two different search algorithms, each with "
        "different benefits."
        "  Set this to the name of an engine module to change the search "
        "algorithm used for dividing and tying notes and rests.";

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
  case 9: {
    set->name = "dot-notelevel"; // docscat{rhythmic}
    set->type = module_string;
    set->descdoc =
        "The level at which notes are allowed to be dotted.  "
        "Set this to a string indicating how dotted notes may appear in the "
        "score.  "
        "`top' specifies that dotted notes are allowed only when they take up "
        "a full measure, "
        "`div' specifies that they are allowed when they are more than a beat "
        "in duration (i.e., the \"metrical division\" level), "
        "`all' specifies that they are always allowed, and `none' specifies "
        "that they are never allowed.";
    set->typedoc = dotnoteleveltype; // same valid fun

    module_setval_string(&set->val, "all");

    set->loc = module_locmeasdef;
    set->valid = valid_dotnotelevel; // no range
    // set->validdeps = validdeps_maxtupdur;
    set->uselevel = 2;
    dotnotelevelid = id;
    break;
  }
  case 10: {
    set->name = "doubledot-notelevel"; // docscat{rhythmic}
    set->type = module_string;
    set->descdoc =
        "The level at which notes are allowed to be double-dotted.  "
        "Set this to a string indicating how double-dotted notes may appear in "
        "the score.  "
        "`top' specifies that double-dotted notes are allowed only when they "
        "take up a full measure, "
        "`div' specifies that they are allowed when they are more than a beat "
        "in duration (i.e. \"metrical division\" level), "
        "`all' specifies that they are always allowed, and `none' specifies "
        "that they are never allowed.";
    set->typedoc = dotnoteleveltype; // same valid fun

    module_setval_string(&set->val, "all");

    set->loc = module_locmeasdef;
    set->valid = valid_dotnotelevel; // no range
    // set->validdeps = validdeps_maxtupdur;
    set->uselevel = 2;
    doubledotnotelevelid = id;
    break;
  }
  case 11: {
    set->name = "shortlongshort-notelevel"; // docscat{rhythmic}
    set->type = module_string;
    set->descdoc =
        "The level at which a short-long-short sequence of notes is allowed to "
        "occur (e.g. eighth note, quarter note, eighth note).  "
        "Set this to a string indicating how short-long-short sequences may "
        "appear in the score.  "
        "`top' specifies that such a sequence is allowed only when they take "
        "up a full measure, "
        "`div' specifies that they are allowed when the notes are more than a "
        "beat in duration (i.e. \"metrical division\" level), "
        "`all' specifies that they are always allowed, and `none' specifies "
        "that they are never allowed.";
    set->typedoc = dotnoteleveltype; // same valid fun

    module_setval_string(&set->val, "div");

    set->loc = module_locmeasdef;
    set->valid = valid_dotnotelevel; // no range
    // set->validdeps = validdeps_maxtupdur;
    set->uselevel = 2;
    shortlongshortnotelevelid = id;
    break;
  }
  case 12: {
    set->name = "syncopated-notelevel"; // docscat{rhythmic}
    set->type = module_string;
    set->descdoc =
        "The level at which a short-multiple-long-short syncopated sequence of "
        "notes is allowed to occur (e.g. eighth-quarter-quarter-eighth).  "
        "Set this to a string indicating how short-multiple-long-short "
        "sequences may appear in the score.  "
        "`top' specifies that such a sequence is allowed only when it takes up "
        "a full measure, "
        "`div' specifies that it is allowed when the notes are more than a "
        "beat in duration (i.e. \"metrical division\" level), "
        "`all' specifies that it is always allowed, and `none' specifies that "
        "it is never allowed.";
    set->typedoc = dotnoteleveltype; // same valid fun

    module_setval_string(&set->val, "div");

    set->loc = module_locmeasdef;
    set->valid = valid_dotnotelevel; // no range
    // set->validdeps = validdeps_maxtupdur;
    set->uselevel = 2;
    syncopatednotelevelid = id;
    break;
  }
  case 13: {
    set->name = "div-note-score"; // docscat{rhythmic}
    set->type = module_number;
    set->descdoc =
        "The base penalty for a single note appearing in the score."
        "  Increasing this value discourages notes from being split and tied "
        "or too many rests from being created."
        "  Increasing it also encourages the appearance of dotted notes.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 2);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    basescoreid = id;
    break;
  }
  case 14: {
    set->name = "div-group-score"; // docscat{rhythmic}
    set->type = module_number;
    set->descdoc =
        "The score for dividing simultaneous measures with similar "
        "subdivisions when they are grouped together using `divgroup'.  "
        //"  Parts can be grouped together using the `divgroup' setting.  "
        "Increasing this value increases the effort FOMUS makes trying to "
        "divide grouped parts similarly.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 3);

    set->loc = module_locmeasdef;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    grouptogid = id;
    break;
  }
  case 15: {
    set->name = "div-tuplet-score"; // docscat{tuplets}
    set->type = module_number;
    set->descdoc = "The base penalty for creating a tuplet.  Increasing this "
                   "makes it less likely for tuplets to appear.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 2);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    tupletbasescoreid = id;
    break;
  }
  case 16: {
    set->name = "tupletdur"; // docscat{tuplets}
    set->type = module_list_nums;
    set->descdoc =
        "A tuplet duration or list of durations.  Setting this forces FOMUS to "
        "use tuplets or nested tuplets that spans only a specified duration."
        "  A list value represents tuplet durations at multiple tuplet levels "
        "(the first list item represents the largest/outermost level)."
        "  A single numeric value (equivalent to a list of size one) specifies "
        "the duration only at the outermost tuplet level."
        "  An empty list or zero in any list position indicates no restriction "
        "on tuplet size."
        "  It's important to note that only valid tuplets allowed by FOMUS's "
        "rules (or specified with `tup..' and `..tup' marks) can appear"
        " (i.e., this setting can't be used to force FOMUS to create a tuplet "
        "at a specific location).";
    set->typedoc = preferredtuptype;

    module_setval_list(&set->val, 1);
    module_setval_int(set->val.val.l.vals, 0);

    set->loc = module_locnote;
    set->valid = valid_preferredtup;
    set->uselevel = 2;
    forcetupsizeid = id;
    break;
  }
  case 17: {
    set->name = "tupletrat"; // docscat{tuplets}
    set->type = module_list_nums;
    set->descdoc =
        "A tuplet division ratio.  Setting this forces FOMUS to use tuplets or "
        "nested tuplets with the given ratio.  "
        "A list value specifies the ratio at each tuplet level (the first list "
        "item represents the largest/outermost level).  "
        "A single numeric value (equivalent to a list of size one) specifies "
        "the ratio at the outermost tuplet level.  "
        "An integer in any list position specifies the number of divisions (or "
        "numerator of the tuplet ratio) "
        "while a rational number specifies the ratio itself.  "
        "An empty list or zero in any list position specifies no restriction "
        "on the tuplet ratio."
        "  It's important to note that only valid tuplets allowed by FOMUS's "
        "rules (or specified with `tup..' and `..tup' marks) can appear"
        " (i.e., this setting can't be used to force FOMUS to create a tuplet "
        "at a specific location).";
    set->typedoc = preferredtuptype;

    module_setval_list(&set->val, 1);
    module_setval_int(set->val.val.l.vals, 0);

    set->loc = module_locnote;
    set->valid = valid_preferredtup;
    set->uselevel = 2;
    forcetupratid = id;
    break;
  }
  case 18: {
    set->name = "div-tupletsize-score"; // docscat{tuplets}
    set->type = module_number;
    set->descdoc =
        "The score for producing tuplets that have approximately the same "
        "number of notes under them as the tuplet ratio numerator.  "
        "Increasing this prevents creating, for example, a 3:2 triplet over a "
        "group of six or more notes (a 6:4 sextuplet would be used instead).  "
        "Increasing this also prevents FOMUS from creating two consecutive, "
        "unequal triplets in many cases (e.g., one spanning two beats and the "
        "other spanning one beat).";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 8);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    rightsizetupletscoreid = id;
    break;
  }
  case 19: {
    set->name = "div-bigtupletnum-score"; // docscat{tuplets}
    set->type = module_number;
    set->descdoc =
        "The penalty for producing tuplets at the top or \"metrical division\" "
        "level of a measure (i.e., greater than a beat in duration)"
        " that have reducible ratios (e.g., 6:4, 10:8).  "
        "Increasing this prevents creating, for example, large 6:4 sextuplets "
        "that would look better if they were a pair triplets instead.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 0);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    bigtupletnumscoreid = id;
    break;
  }
  case 20: {
    set->name = "div-tupletrest-score"; // docscat{tuplets}
    set->type = module_number;
    set->descdoc = "The penalty for placing a rest underneath a tuplet.  "
                   "Increasing this encourages tuplets with notes underneath "
                   "rather than rests.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 0);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    tupletrestscoreid = id;
    break;
  }
  case 21: {
    set->name = "div-samedurtuplet-score"; // docscat{tuplets}
    set->type = module_number;
    set->descdoc = "The score for producing tuplets of the same duration when "
                   "they have the same ratio.  "
                   "For example, increasing this might prevent FOMUS from "
                   "creating two consecutive, unequally sized triplets, one "
                   "spanning two beats and the other spanning one beat.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 3 /*89*/);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    samedurtupletscoreid = id;
    break;
  }
  case 22: {
    set->name = "div-avedur-score"; // docscat{tuplets}
    set->type = module_number;
    set->descdoc = "TODO";
    // set->descdoc = "The score for using larger rhythmic values on average
    // (e.g., a quarter notes instead of eighths or dotted eighths).  "
    // 	"Increasing this descreases the tendency to split/tie notes unnevenly or
    // place some tuplets in awkward positions.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 0);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    avedurscoreid = id;
    break;
  }
  case 23: {
    set->name = "div-smalltuplet-score"; // docscat{tuplets}
    set->type = module_number;
    set->descdoc = "TODO";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 5);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    smalltupletscoreid = id;
    break;
  }
  case 24: {
    set->name = "div-simpletuplet-score"; // docscat{tuplets}
    set->type = module_number;
    set->descdoc = "TODO";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 5);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    simptupletscoreid = id;
    break;
  }
  case 25: {
    set->name = "div-measdivpref-score"; // docscat{tuplets}
    set->type = module_number;
    set->descdoc = "TODO";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 0.5);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    prefmeasdivscoreid = id;
    break;
  }
  case 26: {
    set->name = "div-meastupletfit-score"; // docscat{tuplets}
    set->type = module_number;
    set->descdoc = "TODO";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 13);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3; // probably doesn't concern user
    weirdtupdurscoreid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}

void module_init() {
  dvlvlset.insert("none");
  dvlvlset.insert("div");
  dvlvlset.insert("top");
  dvlvlset.insert("all");
  // dlim = ((fomus_int)2 << ((std::numeric_limits<fomus_int>::digits + 1) /
  // 2));
}
void module_free() { /*assert(newcount == 0);*/
}
void* module_newdata(FOMUS f) {
#ifndef NDEBUGOUT
  splitdata* x = new splitdata;
  DBG("new splitdata = " << x << std::endl);
  return x;
#else
  return new splitdata;
#endif
}
void module_freedata(void* dat) {
  DBG("delete splitdata = " << dat << std::endl);
  delete (splitdata*) dat;
}
const char* module_initerr() {
  return ierr;
}
int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
int module_itertype() {
  return module_bymeasgroups | module_nograce;
}
const char* module_engine(void* f) {
  return module_setting_sval(f, enginemodid);
}
void module_ready() {
  beatdivid = module_settingid("beatdiv");
  if (beatdivid < 0) {
    ierr = "missing required setting `beatdiv'";
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
  return scmp(a, b, enginemodid);
}
