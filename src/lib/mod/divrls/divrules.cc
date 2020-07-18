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
#include <functional> // binary_function
#include <limits>
#include <map>
#include <new>
#include <set>
#include <string>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "ifacedivrules.h"
#include "module.h"
#include "modutil.h"

#include "debugaux.h"
#include "ilessaux.h"
using namespace ilessaux;

// TODO:
// check for nbeats in the wrong place

namespace divrules {

  const char* ierr = 0;

#ifndef NDEBUG
#define rat_toint(xxx) (assert(xxx.den == 1), fomus_rat_toint(xxx))
#else
#define rat_toint(xxx) fomus_rat_toint(xxx)
#endif

  template <typename I1, typename I2, typename F>
  inline F for_each2(I1 first1, const I1& last1, I2 first2, F& fun) {
    while (first1 != last1)
      fun(*first1++, *first2++);
    return fun;
  }

  int nbeatsid, compid, maxtupsid, tuprattypeid, tupdivsid, deftupdivsid,
      setdefinitdivsid, setinitdivsid, mintupdurid, maxtupdurid, largetupsizeid;

  // enums
  enum divpar { div_top, div_sig, div_small };
  enum divdir { div_left, div_mid, div_right };
  enum divlvl {
    div_lvlnone = 0x00,
    div_lvlsig = 0x01,
    div_lvltop = 0x10,
    div_lvlall = 0x11
  };
  enum tuptype {
    tup_pow2,
    tup_closenums,
    tup_closenuml,
    tup_close1s,
    tup_close1l
  };

  std::map<std::string, divlvl, isiless> dvlvlmap;
  inline divlvl getdvlvl(const std::string& str) {
    std::map<std::string, divlvl>::const_iterator i(dvlvlmap.find(str));
    assert(i != dvlvlmap.end());
    return i->second;
  }

  std::map<std::string, tuptype, isiless> typmap;
  inline tuptype gettuprattype(const std::string& str) {
    std::map<std::string, tuptype>::const_iterator i(typmap.find(str));
    assert(i != typmap.end());
    return i->second;
  }

  // div class
  struct tupinfo {
    fomus_rat t;
    fomus_rat dur;
    bool beg, end;
    tupinfo(const fomus_rat& tup, const fomus_rat& dur, const bool beg,
            const bool end)
        : t(tup), dur(dur), beg(beg), end(end) {}
    tupinfo() {
      t.num = 0;
    }
    tupinfo(const tupinfo& x, const bool beg0, const bool end0)
        : t(x.t), dur(x.dur), beg(x.beg && beg0), end(x.end && end0) {}
#ifndef NDEBUGOUT
    void print() const {
      DBG("<tu:");
      if (t.num == 0)
        DBG('-');
      else
        DBG(t.num << '/' << t.den);
      DBG(" du:" << dur << " be:" << beg << " en:" << end << '>');
    }
#endif
  };
  typedef std::vector<tupinfo> tupletvect;
  typedef tupletvect::iterator tupletvect_it;
  typedef tupletvect::const_iterator tupletvect_constit;
  typedef boost::ptr_set<std::vector<fomus_rat>> initdurset;
  class rulesdata;
  struct basediv {
    fomus_rat tim, dur;
    tupletvect tuplet;
    bool alt, art;
    // fomus_rat adjdur;
#ifndef NDEBUG
    int valid;
#endif
    basediv(const fomus_rat& tim, const fomus_rat& dur, const tupletvect& tups,
            const tupinfo& tup, const bool alt, const bool art, const bool beg,
            const bool end)
        : // tim is time of parent div node
          tim(tim), dur(dur), alt(alt), art(art) /*, adjdur(dur)*/ {
      for (tupletvect_constit i(tups.begin()); i != tups.end(); ++i) {
        tuplet.push_back(tupinfo(*i, beg, end));
        // adjdur = adjdur * i->t;
      }
      if (tup.t.num > 0)
        tuplet.push_back(tup);
        // for (tupletvect::const_iterator i(tuplet.begin()); i != tuplet.end();
        // ++i) r = r * i->t;
#ifndef NDEBUG
      valid = 12345;
#endif
    }
#ifndef NDEBUG
    bool isvalid() const {
      return valid == 12345;
    }
#endif
    fomus_rat endtime() const {
      return tim + dur;
    }
    virtual divpar type() const = 0;
    virtual divrules_ornode expand(rulesdata& rd,
                                   const divrules_rangelist& excl) = 0;
    virtual bool tieleftallowed() const {
      return true;
    }
    virtual bool tierightallowed() const {
      return true;
    }
    virtual bool issmall() const {
      return false;
    }
    int ntupletlevels() const {
      return tuplet.size();
    }
    fomus_rat tupletdur(const int lvl) const {
      assert(lvl < ntupletlevels());
      return tuplet[lvl].dur;
    }
    int istupletbegin(const int lvl) const {
      assert(lvl < ntupletlevels());
      return tuplet[lvl].beg;
    }
    int istupletend(const int lvl) const {
      assert(lvl < ntupletlevels());
      return tuplet[lvl].end;
    }
    fomus_rat gettuplet(const int lvl) const {
      assert(lvl < ntupletlevels());
      return tuplet[lvl].t;
    }
    fomus_rat getdurmult() const {
      fomus_rat r = {1, 1};
      for (tupletvect::const_iterator i(tuplet.begin()); i != tuplet.end(); ++i)
        r = r * i->t;
      return r;
    }
    virtual bool isnoteonly() const {
      return false;
    }
    virtual ~basediv() {}
#ifndef NDEBUGOUT
    virtual void print() const {
      DBG("[base]");
      printaux();
      DBG('\n');
    }
    void printaux() const {
      DBG(" ti:" << tim << " du:" << dur << " al:" << alt << " ar:" << art
                 << " tu:");
      if (!tuplet.empty()) {
        DBG('(');
        for (tupletvect::const_iterator i(tuplet.begin()), e(tuplet.end());
             i != e; ++i)
          i->print();
        DBG(')');
      } else
        DBG('-');
    }
#endif
  };
  struct divsigortop : public basediv {
    boost::shared_ptr<initdurset> initdurs;
    divsigortop(const fomus_rat& tim, const fomus_rat& dur,
                const tupletvect& tups, const tupinfo& tup, const bool alt,
                const bool art, boost::shared_ptr<initdurset>& initdurs,
                const bool beg, const bool end)
        : basediv(tim, dur, tups, tup, alt, art, beg, end), initdurs(initdurs) {
    }
    virtual bool isdivtop() const = 0;
    virtual bool isdivsig() const = 0;
#ifndef NDEBUGOUT
    void print() const {
      DBG("[sigortop]");
      printaux();
      DBG('\n');
    }
    void printaux() const {
      basediv::printaux();
      DBG(" in:");
      if (initdurs.get()) {
        DBG('(');
        for (initdurset::const_iterator i(initdurs->begin()),
             e(initdurs->end());
             i != e; ++i) {
          DBG('<');
          for (std::vector<fomus_rat>::const_iterator j(i->begin()),
               je(i->end());
               j != je; ++j) {
            DBG(*j);
            if (j != boost::prior(je))
              DBG(' ');
          }
          DBG('>');
        }
        DBG(')');
      } else
        DBG('-');
    }
#endif
  };

  struct divtop : public divsigortop {
    divtop(boost::shared_ptr<initdurset>& initdurs, const fomus_rat& dur,
           const fomus_rat& tim, const tupletvect& tups, const tupinfo& tup)
        : divsigortop(tim, dur, tups, tup, true, true, initdurs, true, true) {}
    divpar type() const {
      return div_top;
    }
    bool isdivtop() const {
      return true;
    }
    bool isdivsig() const {
      return false;
    }
    divrules_ornode expand(rulesdata& rd, const divrules_rangelist& excl);
#ifndef NDEBUGOUT
    void print() const {
      DBG("[top]");
      printaux();
      DBG('\n');
    }
    void printaux() const {
      divsigortop::printaux();
    }
#endif
  };

  struct divsig : public divsigortop {
    bool irr; // is the PARENT "irregular"
    divsig(const bool alt, const bool art,
           boost::shared_ptr<initdurset>& initdurs, const fomus_rat& dur,
           const fomus_rat& tim, const bool irr, const tupletvect& tups,
           const tupinfo& tup, const bool beg, const bool end)
        : divsigortop(tim, dur, tups, tup, alt, art, initdurs, beg, end),
          irr(irr) {}
    divpar type() const {
      return div_sig;
    }
    bool isdivtop() const {
      return false;
    }
    bool isdivsig() const {
      return true;
    }
    divrules_ornode expand(rulesdata& rd, const divrules_rangelist& excl);
#ifndef NDEBUGOUT
    void print() const {
      DBG("[sig]");
      printaux();
      DBG('\n');
    }
    void printaux() const {
      divsigortop::printaux();
      DBG(" ir:" << irr);
    }
#endif
  };

  struct divsmallorundiv : public basediv {
    divsmallorundiv(const fomus_rat& tim, const fomus_rat& dur,
                    const tupletvect& tups, const tupinfo& tup, const bool alt,
                    const bool art, const bool beg, const bool end)
        : basediv(tim, dur, tups, tup, alt, art, beg, end) {}
    bool issmall() const {
      return true;
    }
#ifndef NDEBUGOUT
    void print() const {
      DBG("[smallorundiv]");
      printaux();
      DBG('\n');
    }
    void printaux() const {
      basediv::printaux();
    }
#endif
  };
  struct divsmall : public divsmallorundiv { // should be based on divsig???
    fomus_int div; // two vars, tiel and tier are always true
    divsmall(const bool alt, const bool art, const fomus_rat& dur,
             const fomus_rat& tim, const fomus_int div, const tupletvect& tups,
             const tupinfo& tup, const bool beg, const bool end)
        : divsmallorundiv(tim, dur, tups, tup, alt, art, beg, end), div(div) {}
    divpar type() const {
      return div_small;
    }
    divrules_ornode expand(rulesdata& rd, const divrules_rangelist& excl);
#ifndef NDEBUGOUT
    void print() const {
      DBG("[small]");
      printaux();
      DBG('\n');
    }
    void printaux() const {
      divsmallorundiv::printaux();
      DBG(" dv:" << div);
    }
#endif
  };

  struct divundiv : public divsmallorundiv { // should be based on divsig???
    bool istiel, istier; // for calling module's information--is a tie allowed?
                         // (ie. on dotted notes)
    divundiv(const fomus_rat& dur, const fomus_rat& tim, const tupletvect& tups,
             const bool istiel, const bool istier, const bool beg,
             const bool end)
        : divsmallorundiv(tim, dur, tups, tupinfo(), /*true*/ false,
                          /*true*/ false, beg, end),
          istiel(istiel), istier(istier) {}
    divpar type() const {
      return div_small;
    }
    divrules_ornode expand(rulesdata& rd, const divrules_rangelist& excl);
    bool tieleftallowed() const {
      return istiel;
    }
    bool tierightallowed() const {
      return istier;
    }
#ifndef NDEBUGOUT
    void print() const {
      DBG("[undiv]");
      printaux();
      DBG('\n');
    }
    void printaux() const {
      divsmallorundiv::printaux();
      DBG(" tl:" << istiel << " tr:" << istier);
    }
#endif
    bool isnoteonly() const {
      return true;
    }
  };

  // functions for splitting at sig/top level
  inline bool atsigtop(const divsigortop& div, const divlvl lvl) {
    return (lvl & div_lvlsig) || (div.isdivtop() && (lvl & div_lvltop));
  }
  // inline bool attop(const divsigortop& div, const divlvl lvl) {return
  // div.isdivtop() && (lvl & div_lvltop);}

  // ornodeex & andnodeex
  typedef std::vector<divrules_div> andnodeexvect;
  typedef andnodeexvect::const_iterator andnodeexvect_constit;
  typedef andnodeexvect::iterator andnodeexvect_it;
  struct divrules_andnodeex_nodel {
    andnodeexvect arr;
    void setarr(divrules_andnode& str) {
      str.n = arr.size();
      str.divs = &arr[0];
    }
    divrules_andnode getarr() const {
      divrules_andnode x = {arr.size(), (divrules_div*) &arr[0]};
      return x;
    }
    void pushnew(const divrules_div d) {
      arr.push_back(d);
    }
    bool empty() const {
      return arr.empty();
    }
    void clear() {
      arr.clear();
    }
  };
  typedef boost::ptr_vector<divrules_andnodeex_nodel> ornodeexvect;
  typedef ornodeexvect::const_iterator ornodeexvect_constit;
  typedef ornodeexvect::iterator ornodeexvect_it;
  struct divrules_ornodeex_nodel {
    std::vector<divrules_andnode> arr; // array that module sees
    ornodeexvect aex;                  // extra info
    void clear() {
      aex.clear();
    }
    divrules_ornode getarr();
    divrules_andnodeex_nodel& getnewandnodeex() {
      divrules_andnodeex_nodel* x;
      aex.push_back(x = new divrules_andnodeex_nodel());
      return *x;
    }
    bool empty() const {
      return aex.empty();
    }
    void popback() {
      aex.pop_back();
    }
  };
  inline divrules_ornode divrules_ornodeex_nodel::getarr() {
    divrules_ornode x;
    x.n = aex.size();
    arr.resize(x.n);
    for_each2(aex.begin(), aex.end(), arr.begin(),
              bind(&divrules_andnodeex_nodel::setarr, boost::lambda::_1,
                   boost::lambda::_2));
    x.ands = &arr[0];
    return x;
  }

  // divs
  inline void pushnewsmalldiv(divrules_andnodeex_nodel& ands, divsmall& div,
                              const fomus_rat& durrat, const fomus_rat& pt1,
                              const divdir wh, const bool al, const bool ar,
                              const fomus_int dv) {
    ands.pushnew(new divsmall(
        /*alt*/ wh == div_left ? (al && div.alt) : (al && div.alt && div.art),
        /*art*/ wh == div_right ? (ar && div.art) : (ar && div.alt && div.art),
        /*dur*/ div.dur * durrat, /*tim*/ div.tim + div.dur * pt1,
        /*div*/ dv, // commented out region was `irr'
        /*tup*/ div.tuplet, tupinfo() /*module_makerat(0, 1)*/, wh == div_left,
        wh == div_right));
  }
  inline void pushnewdivundiv(divrules_andnodeex_nodel& ands,
                              const basediv& par, const fomus_rat& durrat,
                              const fomus_rat& pt1, const tupletvect& tups,
                              const bool istiel, const bool istier,
                              const divdir wh) {
    DBG("pushnewdivundiv " << par.dur * durrat << ", "
                           << par.tim + par.dur * pt1 << std::endl);
    ands.pushnew(new divundiv(par.dur * durrat, par.tim + par.dur * pt1, tups,
                              istiel, istier, wh == div_left, wh == div_right));
  }

  typedef std::vector<fomus_int> initdivvect;
  typedef boost::ptr_set<initdivvect> initdivsset;

  class rulesdata {
    friend struct basediv;
    std::map<const fomus_rat, boost::shared_ptr<initdurset>>
        divs; // entries from user timesig object or default list, looked up by
              // nbeats, so all of them should fit
    std::vector<std::pair<fomus_int, fomus_int>>
        tups;         // user-supplied list of tuplet ratios to choose from
    fomus_rat nbeats; // calculated from timenum / (timeden * beat)
    bool comp;
    divlvl dotnotelvl, dbldotnotelvl, slsnotelvl,
        syncnotelvl; // TODO: get these from settings!!!!
    fomus_rat mintupdur, maxtupdur;
    std::vector<fomus_int> maxtups;
    tuptype tuptyp; // how to get tuplet ratio/multiplier
    int tuptypwh;
    boost::ptr_map<const fomus_int, initdivsset>
        tupdivs; // map from tuplet len to list of divs
    divrules_ornodeex_nodel ornode;
    // boost::shared_ptr<initdurset> tmpdurs;
    struct module_list
        initdivslist; // return value cache, rulesdata takes care of freeing it
    fomus_rat mininitlookup, maxinitlookup;
#ifndef NDEBUG
    int valid;
#endif
public:
    rulesdata(const divrules_data& data)
        : nbeats(module_setting_rval(data.meas, nbeatsid)),
          comp(module_setting_ival(data.meas, compid)),
          dotnotelvl(data.dotnotelvl_setid < 0
                         ? div_lvlnone
                         : getdvlvl(module_setting_sval(
                               data.meas, data.dotnotelvl_setid))),
          dbldotnotelvl(data.dbldotnotelvl_setid < 0
                            ? div_lvlnone
                            : getdvlvl(module_setting_sval(
                                  data.meas, data.dbldotnotelvl_setid))),
          slsnotelvl(data.slsnotelvl_setid < 0
                         ? div_lvlnone
                         : getdvlvl(module_setting_sval(
                               data.meas, data.slsnotelvl_setid))),
          syncnotelvl(data.syncnotelvl_setid < 0
                          ? div_lvlnone
                          : getdvlvl(module_setting_sval(
                                data.meas, data.syncnotelvl_setid))),
          mintupdur(module_setting_rval(data.meas,
                                        /*data.mintupdur_setid*/ mintupdurid)),
          maxtupdur(module_setting_rval(data.meas,
                                        /*data.maxtupdur_setid*/ maxtupdurid)),
          tuptyp(gettuprattype(module_setting_sval(data.meas, tuprattypeid))),
          // tuptyp2(gettuprattype(module_setting_sval(data.meas,
          // tuprattype2id))),
          tuptypwh(module_setting_ival(data.meas, largetupsizeid)),
          mininitlookup(
              module_makerat(std::numeric_limits<fomus_int>::max(), 1)),
          maxinitlookup(
              module_makerat(std::numeric_limits<fomus_int>::min() + 1, 1)) {
      if (nbeats == (fomus_int) 0)
        nbeats = module_dur(data.meas);
      // maxtups & tupdivs
      module_value x(module_setting_val(data.meas, maxtupsid));
      for (module_value *i = x.val.l.vals, *ie = x.val.l.vals + x.val.l.n;
           i < ie; ++i) {
        assert(i->type == module_int);
        maxtups.push_back(i->val.i);
      }
      //#warning "*** why are all of these read in? should only need the
      //matching ones ***"
      fillupinitdivs(module_setting_val(data.meas, setdefinitdivsid), 0);
      std::set<fomus_rat> x1;
      fillupinitdivs(module_setting_val(data.meas, setinitdivsid), &x1);
      filluptupdivs(module_setting_val(data.meas, deftupdivsid), 0);
      std::set<fomus_int> x2;
      filluptupdivs(module_setting_val(data.meas, tupdivsid), &x2);
      initdivslist.n = -1;
      initdivslist.vals = 0;
#ifndef NDEBUG
      valid = 12345;
#endif
    }
    ~rulesdata() {
      module_free_list(initdivslist);
    }
#ifndef NDEBUG
    bool isvalid() const {
      return valid == 12345;
    }
#endif
    bool iscompound() const {
      return comp;
    }
    const struct module_list getinitdivs();
    void fillupinitdivs(const module_value& z, std::set<fomus_rat>* repl);
    void filluptupdivs(const module_value& z, std::set<fomus_int>* repl);
    // `in' flet in splitrules.lisp--create initial division
    void pushnewinitdiv(struct divrules_andnodeex_nodel& node, divtop& par,
                        const fomus_rat& dur, const bool al, const bool ar,
                        const fomus_rat& tim,
                        boost::shared_ptr<initdurset>& initdv,
                        const divdir wh) {
      // boost::shared_ptr<initdurset> dv(getdivs(dur)); // don't need to delete
      // this
      if (dur > (fomus_int) 1 && getdivs(dur).get()) {
        node.pushnew(new divsig(/*alt*/ al, /*art*/ ar, /*durrats*/ initdv,
                                /*dur*/ dur, /*tim*/ par.tim + tim,
                                /*irr*/ !isexpof2(nbeats),
                                /*istop*/ /*false,*/ /*tups*/ par.tuplet,
                                tupinfo(), wh == div_left, wh == div_right));
      } else {
        assert(dur > (fomus_int) 0);
        // fomus_rat adj(comp && dur <= (fomus_int)1 ? dur * module_makerat(3,
        // 2) : dur); if (isexpof2(adj.den)) {
        node.pushnew(new divsmall(
            /*alt*/ al, /*art*/ ar,
            /*dur*/ dur, /*tim*/ par.tim + tim,
            /*div*/
            std::max(
                /*adj*/ (
                    comp /*&& dur <= (fomus_int)1*/ ? dur * module_makerat(3, 2)
                                                    : dur)
                    .num,
                (fomus_int) 2),
            /*tups*/ par.tuplet, tupinfo(), wh == div_left, wh == div_right));
        //}
      }
    }
    // `si' flet in splitrules.lisp--create timesig-level division
    void pushnewsigdiv(struct divrules_andnodeex_nodel& node, divsigortop& par,
                       const fomus_rat& dur, const divdir wh, const bool al,
                       const bool ar, const fomus_rat& tim) {
      // if (dur > (fomus_int)1) {
      // boost::shared_ptr<initdurset> dv(getdivs(dur));
      if (dur > (fomus_int) 1 && getdivs(dur).get()) {
        node.pushnew(new divsig(
            /*alt*/ wh == div_left ? al && par.alt : al && par.alt && par.art,
            /*art*/ wh == div_right ? ar && par.art : ar && par.alt && par.art,
            /*durrats*/ par.initdurs, /*dur*/ dur, /*tim*/ par.tim + tim,
            /*irr*/ !isexpof2(par.dur),
            /*istop*/ /*false,*/ /*tups*/ par.tuplet, tupinfo(), wh == div_left,
            wh == div_right));
        // return;
      } else {
        // }
        assert(dur > (fomus_int) 0);
        // fomus_rat adj(comp && dur <= (fomus_int)1 ? dur * module_makerat(3,
        // 2) : dur); // sigortop is never in tuplet if (isexpof2(adj.den)) {
        node.pushnew(new divsmall(
            /*alt*/ /*true*/ al, /*art*/ /*true*/ ar,
            /*dur*/ dur, /*tim*/ par.tim + tim,
            std::max(
                /*adj*/ (
                    comp /*&& dur <= (fomus_int)1*/ ? dur * module_makerat(3, 2)
                                                    : dur)
                    .num,
                (fomus_int) 2),
            /*tups*/ par.tuplet, tupinfo(), wh == div_left, wh == div_right));
        //}
      }
    }
    inline void pushnewsigdivr(struct divrules_andnodeex_nodel& node,
                               divsigortop& par, const fomus_rat& durrat,
                               const divdir wh, const bool al, const bool ar,
                               const fomus_rat& pt1) {
      pushnewsigdiv(node, par, par.dur * durrat, wh, al, ar, par.dur * pt1);
    }

    fomus_int gettupden(const fomus_int tup,
                        const fomus_rat& div) { // div is an adjusted duration
      if (!isexpof2(div.den) || isexpof2(div / tup))
        return 0; // can't make a tuplet out of this, or no point
      fomus_int ret =
          div2(div.num); // ret is the candidate denominator of the tuplet
      if (ret >= tup * 2)
        return 0; // ratio too large
      fomus_int sc = std::numeric_limits<fomus_int>::max();
      while (true) {
        fomus_int sc0;
        fomus_int d0(ret * 2);
        switch (div.num >= tuptypwh ? tuptyp : tup_pow2) {
        case tup_pow2:
          if (d0 >= tup)
            return ret;
          break;
        case tup_closenums: // smaller
          if ((sc0 = diff(d0, tup)) >= sc)
            return ret;
          break;
        case tup_closenuml: // larger
          if ((sc0 = diff(d0, tup)) > sc)
            return ret;
          break;
        case tup_close1s:
          if (diff(module_makerat_reduce(d0, tup), module_makerat(1, 1)) >= sc)
            return ret;
          break;
        case tup_close1l:
          if (diff(module_makerat_reduce(d0, tup), module_makerat(1, 1)) > sc)
            return ret;
          break;
        default:
          assert(false);
        }
        sc = sc0;
        ret = d0;
      }
    }

    //#warning "*** also divide by 2 and find again? *** YES"
    boost::shared_ptr<initdurset> getdivs(fomus_rat dur);
    boost::shared_ptr<initdurset> getdivsorcreate(const fomus_rat& dur);
    const initdivsset& gettupdivs(const fomus_int tup);
    void pushnewdivs(divrules_ornodeex_nodel& ors, basediv& par,
                     const fomus_int tup, const fomus_int tupden);
    void freeornode(divrules_ornode& ors) {
      delete (divrules_ornodeex_nodel*) ors.ands[ors.n].divs;
    }
    divrules_ornode top(const fomus_rat& tim,
                        const struct module_list& initdiv);
    divrules_ornode splitdivtop(divtop& div, const divrules_rangelist& excl);
    divrules_ornode splitdivsig(divsig& div, const divrules_rangelist& excl);
    void splitdivsigortop(divsigortop& div);
    divrules_ornode splitdivsmall(divsmall& div,
                                  const divrules_rangelist& excl);
    divrules_ornode splitdone(const basediv& b);
    bool tupsok(const basediv& div) const {
      return !div.tuplet.empty() ||
             ((mintupdur <= (fomus_int) 0 || mintupdur <= div.dur) &&
              (maxtupdur <= (fomus_int) 0 || div.dur <= maxtupdur));
    }
    fomus_int maxtup(const basediv& div) const {
      assert(div.tuplet.size() < maxtups.size());
      return *(maxtups.begin() + div.tuplet.size());
    }
    bool havemaxtup(const basediv& div) const {
      return div.tuplet.size() < maxtups.size();
    }
    divrules_ornode splitdivundiv(divundiv& div) {
      ornode.clear();
      return splitdone(div);
    }
  };

  boost::shared_ptr<initdurset> rulesdata::getdivs(fomus_rat dur) {
    assert(dur > (fomus_int) 0);
    fomus_int di;
    if (comp) {
      di = 1;
    } else {
      DBG("TRYING DIV=" << dur << ", dur = " << dur << std::endl);
      std::map<const fomus_rat, boost::shared_ptr<initdurset>>::const_iterator
          j(divs.find(dur));
      if (j != divs.end())
        return j->second;
      di = 2;
    }
    while (true) {
      fomus_rat dur1(dur / di);
      if (dur1 < mininitlookup)
        break;
      DBG("TRYING DIV=" << dur1 << ", dur = " << dur << std::endl);
      std::map<const fomus_rat, boost::shared_ptr<initdurset>>::const_iterator
          j(divs.find(dur1));
      if (j != divs.end()) {
        boost::shared_ptr<initdurset> r(new initdurset);
        for (initdurset::const_iterator k(j->second->begin()),
             ke(j->second->end());
             k != ke; ++k) {
          std::auto_ptr<std::vector<fomus_rat>> v(new std::vector<fomus_rat>);
          for (std::vector<fomus_rat>::const_iterator l(k->begin());
               l != k->end(); ++l) {
            fomus_rat x(*l * di);
            if (comp && x.den != 1 && (x.den / 3) * 3 != x.den)
              goto LOOP1; // no good for comp
            v->push_back(x);
          }
          r->insert(v.release());
        LOOP1:;
        }
        if (!r->empty())
          return r;
      }
      di *= 2;
    }
    di = 2;
    while (true) {
      fomus_rat dur1(dur * di);
      if (dur1 > maxinitlookup)
        return boost::shared_ptr<initdurset>();
      DBG("TRYING DIV=" << dur1 << ", dur = " << dur << std::endl);
      std::map<const fomus_rat, boost::shared_ptr<initdurset>>::const_iterator
          j(divs.find(dur1));
      if (j != divs.end()) {
        boost::shared_ptr<initdurset> r(new initdurset);
        for (initdurset::const_iterator k(j->second->begin()),
             ke(j->second->end());
             k != ke; ++k) {
          std::auto_ptr<std::vector<fomus_rat>> v(new std::vector<fomus_rat>);
          for (std::vector<fomus_rat>::const_iterator l(k->begin());
               l != k->end(); ++l) {
            fomus_rat x(*l / di);
            if (comp && x.den != 1 && (x.den / 3) * 3 != x.den)
              goto LOOP2; // no good for comp
            v->push_back(x);
          }
          r->insert(v.release());
        LOOP2:;
        }
        if (!r->empty())
          return r;
      }
      di *= 2;
    }
  }

  const struct module_list rulesdata::getinitdivs() {
    if (initdivslist.n < 0) {
      boost::shared_ptr<initdurset> i(getdivs(nbeats));
      if (i.get()) {
        DBG("Got DIVS, size=" << i->size() << std::endl);
        const initdurset& se = *i;
        initdivslist = module_new_list(se.size());
        struct module_value* v = initdivslist.vals;
        for (initdurset::const_iterator d(se.begin()); d != se.end();
             ++d, ++v) {
          v->type = module_list;
          v->val.l = module_new_list(d->size());
          struct module_value* v2 = v->val.l.vals;
          for (std::vector<fomus_rat>::const_iterator j(d->begin());
               j != d->end(); ++j, ++v2) {
            v2->type = module_rat;
            v2->val.r = *j;
          }
        }
      } else {
        DBG("Don't Got DIVS" << std::endl);
        // return 1 div = entire measure--TEST THIS
        initdivslist = module_new_list(1);
        initdivslist.vals->type = module_list;
        initdivslist.vals->val.l = module_new_list(1);
        initdivslist.vals->val.l.vals[0] = module_makeval(nbeats);
      }
    }
    return initdivslist; // caller must free the list!
  }
  void rulesdata::fillupinitdivs(const module_value& z,
                                 std::set<fomus_rat>* res) {
    for (module_value *i = z.val.l.vals, *ie = z.val.l.vals + z.val.l.n; i < ie;
         ++i) {
      assert(i->type == module_list);
      if (i->val.l.n <= 1)
        continue;
      std::vector<fomus_rat>* dd = new std::vector<fomus_rat>;
      fomus_rat s = {0, 1};
      for (module_value *j = i->val.l.vals, *je = i->val.l.vals + i->val.l.n;
           j < je; ++j) {
        fomus_rat xx(GET_R(*j));
        dd->push_back(xx);
        s = s + xx;
      }
      initdurset* dd0;
      if (res && res->insert(s).second) {
        divs.erase(s);
        divs.insert(std::map<const fomus_rat, boost::shared_ptr<initdurset>>::
                        value_type(s, boost::shared_ptr<initdurset>(
                                          dd0 = new initdurset)));
      } else {
        std::map<const fomus_rat, boost::shared_ptr<initdurset>>::iterator ii(
            divs.find(s));
        if (ii == divs.end()) {
          divs.insert(std::map<const fomus_rat, boost::shared_ptr<initdurset>>::
                          value_type(s, boost::shared_ptr<initdurset>(
                                            dd0 = new initdurset)));
        } else
          dd0 = ii->second.get();
      }
      dd0->insert(dd);
      if (s > maxinitlookup)
        maxinitlookup = s;
      if (s < mininitlookup)
        mininitlookup = s;
    }
  }
  void rulesdata::filluptupdivs(const module_value& y,
                                std::set<fomus_int>* res) {
    for (module_value *i = y.val.l.vals, *ie = y.val.l.vals + y.val.l.n; i < ie;
         ++i) {
      assert(i->type == module_list);
      if (i->val.l.n <= 1)
        continue;
      initdivvect* dd = new initdivvect;
      fomus_int s = 0;
      for (module_value *j = i->val.l.vals, *je = i->val.l.vals + i->val.l.n;
           j < je; ++j) {
        dd->push_back(j->val.i);
        s += j->val.i;
      }
      initdivsset* dd0;
      if (res && res->insert(s).second) {
        tupdivs.erase(s);
        tupdivs.insert(s, dd0 = new initdivsset);
      } else {
        boost::ptr_map<const fomus_int, initdivsset>::iterator ii(
            tupdivs.find(s));
        if (ii == tupdivs.end()) {
          tupdivs.insert(s, dd0 = new initdivsset);
        } else
          dd0 = ii->second;
      }
      dd0->insert(dd);
    }
  }

  inline divrules_ornode divtop::expand(rulesdata& rd,
                                        const divrules_rangelist& excl) {
    return rd.splitdivtop(*this, excl);
  }
  inline divrules_ornode divsig::expand(rulesdata& rd,
                                        const divrules_rangelist& excl) {
    return rd.splitdivsig(*this, excl);
  }
  inline divrules_ornode divsmall::expand(rulesdata& rd,
                                          const divrules_rangelist& excl) {
    return rd.splitdivsmall(*this, excl);
  }
  inline divrules_ornode divundiv::expand(rulesdata& rd,
                                          const divrules_rangelist& excl) {
    return rd.splitdivundiv(*this);
  }

#warning "sorting isn't really necessary, get rid of it"
  inline fomus_float sortave(const divrules_andnodeex_nodel& tp) {
    assert(!tp.arr.empty());
    const basediv& b = *(basediv*) tp.arr.back();
    fomus_rat s(b.tim + b.dur);
    for (std::vector<divrules_div>::const_iterator i(tp.arr.begin());
         i != tp.arr.end(); ++i)
      s = s + ((basediv*) (*i))->tim;
    return s / (fomus_float)((fomus_int) tp.arr.size() + 1);
  }
  struct sortors
      : public std::binary_function<const divrules_andnodeex_nodel&,
                                    const divrules_andnodeex_nodel&, bool> {
    const fomus_float mid;
    sortors(const fomus_rat& mid) : mid(mid.num / (fomus_float) mid.den) {}
    bool operator()(const divrules_andnodeex_nodel& x,
                    const divrules_andnodeex_nodel& y) const {
      fomus_float ax(sortave(x));
      fomus_float ay(sortave(y));
      fomus_float ax2(diff(ax, mid));
      fomus_float ay2(diff(ay, mid));
      if (ax2 != ay2)
        return ax2 < ay2; // average closer to center
      if (ax != ay)
        return ax < ay; // larger divs first
      return false;
    }
  };

  divrules_ornode rulesdata::top(const fomus_rat& tim,
                                 const struct module_list& initdiv) { // root
    ornode.clear();
    boost::shared_ptr<initdurset> dv;
    if (initdiv.n > 0) { // module supplies initial divisions
      dv.reset(new initdurset);
      std::vector<fomus_rat>* v;
      dv->insert(v = new std::vector<fomus_rat>);
#ifndef NDEBUG
      fomus_rat xx = {0, 1};
#endif
      for (module_value *e = initdiv.vals, *ie = initdiv.vals + initdiv.n;
           e < ie; ++e) {
        v->push_back(GET_R(*e));
#ifndef NDEBUG
        xx = xx + GET_R(*e);
#endif
      }
      assert(xx == nbeats);
    }
    ornode.getnewandnodeex().pushnew(
        new divtop(dv, nbeats, tim, tupletvect(), tupinfo()));
    ornode.aex.sort(sortors(tim + nbeats / (fomus_int) 2));
    return ornode.getarr();
  }

  struct cmpsets
      : public std::binary_function<const std::vector<fomus_rat>&,
                                    const std::vector<fomus_rat>&, bool> {
    bool operator()(const std::vector<fomus_rat>& x,
                    const std::vector<fomus_rat>& y) const {
      return std::lexicographical_compare(x.begin(), x.end(), y.begin(),
                                          y.end());
    }
  };

  bool modinitdivs(const fomus_rat& tim, const divrules_rangelist& excl,
                   boost::ptr_set<std::vector<fomus_rat>, cmpsets>& sids,
                   const initdurset& id, const bool comp) {
#ifndef NDEBUG
    for (const divrules_range *i(excl.ranges), *ie(excl.ranges + excl.n);
         i + 1 < ie; ++i) {
      assert(i->time2 >= i->time1);
      assert((i + 1)->time1 >= i->time2);
    }
#endif
    bool ret = false;
    for (boost::ptr_set<std::vector<fomus_rat>, cmpsets>::const_iterator i(
             id.begin());
         i != id.end(); ++i) {
      fomus_rat ti(tim);
      std::set<fomus_rat> se;
      DBG("INITDIV was [");
      for (std::vector<fomus_rat>::const_iterator e(i->begin()); e != i->end();
           ++e) {
        ti = ti + *e;
        se.insert(ti - tim);
        DBG(ti - tim << " ");
      }
      DBG("]" << std::endl);
      fomus_rat ti0 = {0, 1};
      for (std::set<fomus_rat>::const_iterator i(se.begin()); i != se.end();) {
        fomus_rat d(*i - ti0);
        // if (comp) d = d * module_makerat(3, 2);
        if (!isexpof2(d.den)) {
          DBG("  d = " << d << std::endl);
          fomus_int n = div2(d.den);
          DBG("  n = " << n << std::endl);
          assert(module_makerat_reduce(d.num / n, d.den / n) < d);
          fomus_rat pl(module_makerat_reduce(d.num / n, d.den / n));
          if (pl <= (fomus_int) 0)
            goto NOPL;
          fomus_rat nt(ti0 + pl);
          ti0 = *i++;
          se.insert(nt);
        } else {
        NOPL:
          ti0 = *i++;
        }
      }
#ifndef NDEBUGOUT
      DBG(" it's [");
      for (std::set<fomus_rat>::const_iterator i(se.begin()); i != se.end();
           ++i) {
        DBG(*i << " ");
      }
      DBG("]" << std::endl);
#endif
      fomus_rat et(ti - tim);
      for (const divrules_range *ex = excl.ranges, *exe = excl.ranges + excl.n;
           ex < exe;
           ++ex) { // exclusions (e.g., division in the middle of a user tuplet)
        while (true) {
          if (ex->lvl <= 0)
            break; // pass up any exclusions not at top division level
          ++ex;
          if (ex >= exe)
            goto DONE;
        }
        DBG("  ex @ " << ex->time1 << " to " << ex->time2 << std::endl);
        if ((ex->time1 > tim && ex->time1 < ti) ||
            (ex->time2 > tim && ex->time2 < ti)) { // exclusion is in-bounds
          fomus_rat t1(ex->time1 - tim);
          fomus_rat t2(ex->time2 - tim);
          fomus_rat t20(std::min(t2, et));
          for (std::set<fomus_rat>::iterator lb(se.upper_bound(t1));
               lb != se.end() && *lb < t20;) { // erase division that lie in the
                                               // middle of exclusion range
            ret = true;
            se.erase(lb++);
          }
          if (t1 > (fomus_int) 0 && se.insert(t1).second)
            ret = true;
          if (t2 < et && se.insert(t2).second)
            ret = true;
        }
      }
    DONE:
      assert(!se.empty());
      assert(*boost::prior(se.end()) == ti - tim);
#ifndef NDEBUGOUT
      DBG("NOW it's [");
      for (std::set<fomus_rat>::const_iterator i(se.begin()); i != se.end();
           ++i) {
        DBG(*i << " ");
      }
      DBG("]" << std::endl);
#endif
      sids.insert(std::auto_ptr<std::vector<fomus_rat>>(
          new std::vector<fomus_rat>(se.begin(), se.end())));
    }
    return ret;
  }

  divrules_ornode rulesdata::splitdivtop(divtop& div,
                                         const divrules_rangelist& excl) {
    ornode.clear();
    boost::shared_ptr<initdurset> id;
    id = (div.initdurs.get() ? div.initdurs : getdivsorcreate(nbeats));
    boost::ptr_set<std::vector<fomus_rat>, cmpsets> sids;
    bool md = modinitdivs(div.tim, excl, sids, *id, comp);
    for (boost::ptr_set<std::vector<fomus_rat>, cmpsets>::const_iterator i(
             sids.begin());
         i != sids.end(); ++i) { // divs adds up to getnum (nbeats)
      assert(!i->empty());
      divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
      fomus_rat xx = {0, 1};
      bool la = true;
      divdir wh = div_left;
      assert(!i->empty());
      for (std::vector<fomus_rat>::const_iterator e(i->begin()),
           ee(boost::prior(i->end()));
           e != i->end();) {
        bool ra =
            (*e == nbeats) ||
            (isexpof2(*e) &&
             isexpof2(nbeats - *e)); // right "anchor", place where a tuplet can
                                     // start/end without being confusing
        pushnewinitdiv(/*node*/ ands, /*par*/ div, /*dur*/ *e - xx, /*al*/ la,
                       /*ar*/ ra, /*pt1*/ xx, /*dv*/ id, /*wh*/ wh);
        xx = *e;
        la = ra;
        wh = ((++e == ee) ? div_right : div_mid);
      }
      assert(xx == nbeats);
    }
    if (!md)
      splitdivsigortop(div);
    return splitdone(div); // div is for calculating the mid time point!--return
                           // value is in "ornode"
  }

  divrules_ornode rulesdata::splitdivsig(divsig& div,
                                         const divrules_rangelist& excl) {
    ornode.clear(); // split has already been decided because entry has been
                    // looked up
    boost::shared_ptr<initdurset> r(getdivsorcreate(div.dur));
    boost::ptr_set<std::vector<fomus_rat>, cmpsets> sids;
    bool md = modinitdivs(div.tim, excl, sids, *r, comp);
    for (boost::ptr_set<std::vector<fomus_rat>, cmpsets>::const_iterator i(
             sids.begin());
         i != sids.end(); ++i) { // divs always present in divsig object
      assert(!i->empty());
      divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
      fomus_rat xx = {0, 1};
      bool la = true;
      divdir di = div_left;
      for (std::vector<fomus_rat>::const_iterator e(i->begin()),
           pe(boost::prior(i->end()));
           e != i->end();) {
        bool ra =
            (*e == div.dur || isexpof2(*e) ||
             isexpof2(div.dur - *e)); // right "anchor", place where a tuplet
                                      // can start/end without being confusing
        pushnewsigdiv(/*node*/ ands, /*par*/ div, /*durrat*/ *e - xx, /*wh*/ di,
                      /*al*/ la, /*ar*/ ra, /*pt1*/ xx);
        xx = *e;
        di = (++e == pe ? div_right : div_mid);
        la = ra;
      }
      assert(xx == div.dur);
    }
    if (!md)
      splitdivsigortop(div);
    return splitdone(div);
  }

  // called before splitdone()
  void rulesdata::splitdivsigortop(divsigortop& div) {
    if (atsigtop(div, dotnotelvl) && !comp && isexpof2(div.dur)) {
      {
        divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
        pushnewdivundiv(ands, div, module_makerat(3, 4), module_makerat(0, 1),
                        div.tuplet, true, false,
                        div_left); // no tie to the right
        pushnewsigdivr(ands, div, module_makerat(1, 4), div_right,
                       /*true*/ false,
                       /*true*/ false, module_makerat(3, 4));
        if (atsigtop(div, dbldotnotelvl)) {
          divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
          pushnewdivundiv(ands, div, module_makerat(7, 8), module_makerat(0, 1),
                          div.tuplet, true, false, div_left); // no tie to right
          pushnewsigdivr(ands, div, module_makerat(1, 8), div_right,
                         /*true*/ false, /*true*/ false, module_makerat(7, 8));
        }
      }
      {
        divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
        pushnewsigdivr(ands, div, module_makerat(1, 4), div_left,
                       /*true*/ false,
                       /*true*/ false, module_makerat(0, 1));
        pushnewdivundiv(ands, div, module_makerat(3, 4), module_makerat(1, 4),
                        div.tuplet, false, true,
                        div_right); // no tie to the left
        if (atsigtop(div, dbldotnotelvl)) {
          divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
          pushnewsigdivr(ands, div, module_makerat(1, 8), div_left,
                         /*true*/ false, /*true*/ false, module_makerat(0, 1));
          pushnewdivundiv(ands, div, module_makerat(7, 8), module_makerat(1, 8),
                          div.tuplet, false, true, div_right); // no tie to left
        }
      }
    }
    if (atsigtop(div, slsnotelvl) &&
        /*div.alt && div.art &&*/ (!comp || div.dur >= (fomus_int) 4) &&
        isexpof2(div.dur)) {
      divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
      pushnewsigdivr(ands, div, module_makerat(1, 4), div_left, /*true*/ false,
                     /*true*/ false, module_makerat(0, 1));
      pushnewdivundiv(ands, div, module_makerat(1, 2), module_makerat(1, 4),
                      div.tuplet, false, false, div_mid);
      pushnewsigdivr(ands, div, module_makerat(1, 4), div_right, /*true*/ false,
                     /*true*/ false, module_makerat(3, 4));
    }
    if (atsigtop(div, syncnotelvl) /*&& div.alt && div.art*/) {
      if (comp ? div.dur.den == 1 && div.dur > (fomus_int) 4
               : div.dur.den <= 2 && div.dur > (fomus_int) 2) {
        for (fomus_rat d1(module_makerat((fomus_int) 1,
                                         comp ? (fomus_int) 1 : (fomus_int) 2)),
             dve(div.dur / (fomus_int) 4);
             d1 < dve && (div.dur / d1).den == 1; d1 = d1 * (fomus_int) 2) {
          fomus_rat d1dr(d1 / div.dur);
          fomus_rat d2dr(d1dr * (fomus_int) 2);
          switch (d2dr.num) {
          case 1: {
            divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
            pushnewdivundiv(ands, div, d1dr, module_makerat(0, 1), div.tuplet,
                            true, false, div_left);
            fomus_rat i(d1dr);
            for (fomus_rat e((fomus_int) 1 - d1dr); i < e; i = i + d2dr)
              pushnewdivundiv(ands, div, d2dr, i, div.tuplet, false, false,
                              div_mid);
            assert(i == (fomus_int) 1 - d1dr);
            pushnewdivundiv(ands, div, d1dr, i, div.tuplet, false, true,
                            div_right);
          } break;
          case 2: // num must be odd
          {
            divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
            pushnewdivundiv(ands, div, d2dr, module_makerat(0, 1), div.tuplet,
                            true, false, div_left);
            fomus_rat i(d2dr);
            for (fomus_rat e((fomus_int) 1 - d1dr); i < e; i = i + d2dr)
              pushnewdivundiv(ands, div, d2dr, i, div.tuplet, false, false,
                              div_mid);
            assert(i == (fomus_int) 1 - d1dr);
            pushnewdivundiv(ands, div, d1dr, i, div.tuplet, false, true,
                            div_right);
          }
            {
              divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
              pushnewdivundiv(ands, div, d1dr, module_makerat(0, 1), div.tuplet,
                              true, false, div_left);
              fomus_rat i(d1dr);
              for (fomus_rat e((fomus_int) 1 - d2dr); i < e; i = i + d2dr)
                pushnewdivundiv(ands, div, d2dr, i, div.tuplet, false, false,
                                div_mid);
              assert(i == (fomus_int) 1 - d2dr);
              pushnewdivundiv(ands, div, d2dr, i, div.tuplet, false, true,
                              div_right);
            }
            break;
          default:
            assert(false);
          }
        }
      }
    }
    if (tupsok(div) && havemaxtup(div) /*&& (div.alt || div.art)*/) {
      fomus_rat adj(comp ? div.dur * module_makerat(3, 2)
                         : div.dur); // doesn't matter if <= 1
      std::set<std::pair<fomus_int, fomus_int>> tps;
      for (fomus_int j = 2, mt = maxtup(div); j <= mt; ++j) {
        fomus_int d = gettupden(j, adj);
        if (d > 1 && tps.insert(std::pair<fomus_int, fomus_int>(j, d)).second) {
          DBG("sig inserting tuplet " << j << "/" << d << " @ " << div.tim
                                      << std::endl);
          pushnewdivs(ornode, (basediv&) div, j, d);
        }
      }
    }
  }
  divrules_ornode rulesdata::splitdivsmall(divsmall& div,
                                           const divrules_rangelist& excl) {
    ornode.clear();
    std::set<fomus_rat> exs;
    fomus_rat etim(div.tim + div.dur);
    for (const divrules_range *e(excl.ranges), *ee(excl.ranges + excl.n);
         e < ee; ++e) {
      if (e->lvl == div.ntupletlevels()) {
        assert(e->time1 <= e->time2);
        if (e->time1 > div.tim && e->time1 < etim)
          exs.insert((e->time1 - div.tim) / div.dur);
        if (e->time2 > div.tim && e->time2 < etim)
          exs.insert((e->time1 - div.tim) / div.dur);
      }
    }
    if (exs.empty()) {
      const initdivsset& dvs(gettupdivs(div.div));
      for (initdivsset::const_iterator v(dvs.begin()); v != dvs.end(); ++v) {
        divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
        assert(!v->empty());
        fomus_rat x1 = {0, 1};
        bool la = true;
        divdir di = div_left;
        for (std::vector<fomus_int>::const_iterator j(v->begin()),
             pe(boost::prior(v->end()));
             j != v->end();) {
          fomus_rat r(module_makerat_reduce(*j, div.div)); // durrat
          bool ra = j == pe || (isexpof2(r) && isexpof2((fomus_int) 1 - r));
          pushnewsmalldiv(/*node*/ ands, /*par*/ div, /*durrat*/ r, /*pt1*/ x1,
                          /*wh*/ di, /*al*/ la, /*ar*/ ra,
                          /*div*/ std::max(div2(*j), (fomus_int) 2));
          di = (++j == pe ? div_right : div_mid);
          x1 = x1 + r;
          la = ra;
        }
      }
      if (dotnotelvl == div_lvlall && isexpof2(div.div)) {
        {
          divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
          pushnewdivundiv(ands, div, /*durrat*/ module_makerat(3, 4),
                          /*pt1*/ module_makerat(0, 1), /*tups*/ div.tuplet,
                          /*tiel*/ true, /*tier*/ false,
                          div_left); // no tie to right
          pushnewsmalldiv(
              /*node*/ ands, /*par*/ div, /*durrat*/ module_makerat(1, 4),
              /*pt1*/ module_makerat(3, 4), /*wh*/ div_right,
              /*al*/ /*true*/ false, /*ar*/ /*true*/ false, /*div*/ 2);
          if (dbldotnotelvl == div_lvlall) {
            divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
            pushnewdivundiv(ands, div, /*durrat*/ module_makerat(7, 8),
                            /*pt1*/ module_makerat(0, 1), /*tups*/ div.tuplet,
                            /*tiel*/ true, /*tier*/ false,
                            div_left); // no tie to right
            pushnewsmalldiv(
                /*node*/ ands, /*par*/ div, /*durrat*/ module_makerat(1, 8),
                /*pt1*/ module_makerat(7, 8), /*wh*/ div_right,
                /*al*/ /*true*/ false, /*ar*/ /*true*/ false, /*div*/ 2);
          }
        }
        {
          divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
          pushnewsmalldiv(ands, div, module_makerat(1, 4), module_makerat(0, 1),
                          div_left, /*true*/ false, /*true*/ false, 2);
          pushnewdivundiv(ands, div, module_makerat(3, 4), module_makerat(1, 4),
                          div.tuplet, false, true,
                          div_right); // no tie to the left
          if (dbldotnotelvl == div_lvlall) {
            divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
            pushnewsmalldiv(ands, div, module_makerat(1, 8),
                            module_makerat(0, 1), div_left, /*true*/ false,
                            /*true*/ false, 2);
            pushnewdivundiv(ands, div, module_makerat(7, 8),
                            module_makerat(1, 8), div.tuplet, false, true,
                            div_right); // no tie to the left
          }
        }
      }
      if ((slsnotelvl == div_lvlall ||
           ((slsnotelvl & div_lvlsig) && div.dur >= (fomus_int) 1)) &&
          /*div.alt && div.art &&*/ isexpof2(div.div)) {
        divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
        pushnewsmalldiv(ands, div, module_makerat(1, 4), module_makerat(0, 1),
                        div_left, /*true*/ false, /*true*/ false, 2);
        pushnewdivundiv(ands, div, module_makerat(1, 2), module_makerat(1, 4),
                        div.tuplet, false, false, div_mid);
        pushnewsmalldiv(ands, div, module_makerat(1, 4), module_makerat(3, 4),
                        div_right, false, /*true*/ false, 2);
      }
      if (tupsok(div) && havemaxtup(div) /*&& (div.alt || div.art)*/) {
        std::set<std::pair<fomus_int, fomus_int>> tps;
        for (fomus_int j = 2, mt = maxtup(div); j <= mt; ++j) {
          fomus_int d = gettupden(j, module_makerat(div.div, 1));
          if (d > 1 &&
              tps.insert(std::pair<fomus_int, fomus_int>(j, d)).second) {
            DBG("sm" << div.div << " inserting tuplet " << j << "/" << d
                     << " @ " << div.tim << std::endl);
            pushnewdivs(ornode, (basediv&) div, j, d);
          }
        }
      }
    } else {
      divrules_andnodeex_nodel& ands = ornode.getnewandnodeex();
      fomus_rat x1 = {0, 1};
      bool la = true;
      divdir di = div_left;
      exs.insert(module_makerat(1, 1));
      for (std::set<fomus_rat>::const_iterator j(exs.begin()),
           pe(boost::prior(exs.end()));
           j != exs.end();) {
        fomus_rat r(*j - x1); // durrat
        bool ra = j == pe || (isexpof2(r) && isexpof2((fomus_int) 1 - r));
        pushnewsmalldiv(
            /*node*/ ands, /*par*/ div, /*durrat*/ r, /*pt1*/ x1,
            /*wh*/ di, /*al*/ la, /*ar*/ ra,
            /*div*/ std::max(div2((div.div * r).num), (fomus_int) 2));
        di = (++j == pe ? div_right : div_mid);
        x1 = x1 + r;
        la = ra;
      }
      assert(x1 == (fomus_int) 0);
    }
    return splitdone(div);
  }

  inline divrules_ornode rulesdata::splitdone(const basediv& b) {
#ifndef NDEBUG
    for (ornodeexvect::const_iterator a(ornode.aex.begin());
         a != ornode.aex.end(); ++a) {
      fomus_rat ti(b.tim);
      DBG("  check splits: ");
      for (andnodeexvect::const_iterator i(a->arr.begin()), ie(a->arr.end());
           i != ie; ++i) {
        DBG("t=" << ((basediv*) (*i))->tim << " d=" << ((basediv*) (*i))->dur
                 << ' ');
        assert(((basediv*) (*i))->tim == ti);
        ti = ti + ((basediv*) (*i))->dur;
      }
      DBG(std::endl);
      assert(ti == b.tim + b.dur);
    }
#endif
    ornode.aex.sort(sortors(b.tim + b.dur / (fomus_int) 2));
    return ornode.getarr();
  }

  const initdivsset& rulesdata::gettupdivs(const fomus_int tup) {
    assert(tup > 1);
    boost::ptr_map<const fomus_int, initdivsset>::const_iterator j(
        tupdivs.find(tup));
    if (j != tupdivs.end())
      return *j->second;
    initdivsset* vct; // come up with a division
    tupdivs.insert(tup, vct = new initdivsset);
    fomus_int tu = maxdiv2(tup - 1);
    initdivvect* v = new initdivvect;
    v->push_back(tu);
    v->push_back(tup - tu);
    vct->insert(v);
    v = new initdivvect;
    v->push_back(tup - tu);
    v->push_back(tu);
    vct->insert(v);
    return *vct;
  }
  boost::shared_ptr<initdurset>
  rulesdata::getdivsorcreate(const fomus_rat& dur) {
    DBG("getdivsorcreate" << std::endl);
    boost::shared_ptr<initdurset> r(getdivs(dur));
    if (r.get()) {
      DBG("GOT divsorcreate, size=" << r->size() << std::endl);
      return r;
    }
    DBG("Don't GOT divsorcreate" << std::endl);
    boost::shared_ptr<initdurset> tmpdurs(new initdurset);
    DBG("maxdiv2(dur / (fomus_int)2) = " << maxdiv2(dur / (fomus_int) 2)
                                         << std::endl);
    for (fomus_rat dv = maxdiv2(dur / (fomus_int) 2); dv < dur;
         dv = dv * (fomus_int) 2) { // powers of 2 on the left
      std::vector<fomus_rat>* v = new std::vector<fomus_rat>;
      v->push_back(dv);
      v->push_back(dur - dv);
      tmpdurs->insert(v);
      v = new std::vector<fomus_rat>;
      v->push_back(dur - dv);
      v->push_back(dv);
      tmpdurs->insert(v);
    }
    return tmpdurs;
  }
  void rulesdata::pushnewdivs(divrules_ornodeex_nodel& ors, basediv& par,
                              const fomus_int tup, const fomus_int tupden) {
    const initdivsset& tupdivs(
        gettupdivs(tup)); // guaranteed to return something
    assert(!tupdivs.empty());
    for (initdivsset::const_iterator i(tupdivs.begin()); i != tupdivs.end();
         ++i) {
      assert(!i->empty());
      divrules_andnodeex_nodel& ands = ors.getnewandnodeex();
      bool a1 = true;
      bool t1 = true;
      fomus_rat e1 = {0, 1};
      fomus_int x0 = 0;
      fomus_rat tpt = {tup, tupden};
      for (std::vector<fomus_int>::const_iterator ii(i->begin());
           ii != i->end(); ++ii) {
        x0 += *ii;
        fomus_rat e2(module_makerat_reduce(x0, tup)); // e2 = point
        fomus_rat tt(e2 - e1);           // fraction of complete tuplet
        bool t2 = (e2 == (fomus_int) 1); // end of tuplet?
        bool a2 = (t2 || (isexpof2(e2) && isexpof2(tup - e2)));
        fomus_rat dr(par.dur * tt);
        ands.pushnew(new divsmall(/*alt*/ a1, /*art*/ a2,
                                  /*dur*/ dr, /*tim*/ par.tim + par.dur * e1,
                                  /*div*/ std::max(div2(*ii), (fomus_int) 2),
                                  par.tuplet,
                                  tupinfo(tpt, /*dr*/ par.dur, t1, t2),
                                  e1 == (fomus_int) 0, e2 == (fomus_int) 1));
        a1 = a2;
        t1 = t2;
        e1 = e2;
      }
      assert(e1 == (fomus_int) 1);
    }
  }

  extern "C" {
  divrules_ornode expand(void* moddata, divrules_div node,
                         divrules_rangelist excl);
  void free_moddata(void* moddata);
  divrules_ornode get_root(void* moddata, struct fomus_rat time,
                           struct module_list initdiv);
  void free_node(void* moddata, divrules_div node);
  struct fomus_rat rltime(void* moddata, divrules_div node);
  struct fomus_rat rlendtime(void* moddata, divrules_div node);
  struct fomus_rat rldur(void* moddata, divrules_div node);
  int tieleftallowed(void* moddata, divrules_div node);
  int tierightallowed(void* moddata, divrules_div node);
  int issmall(void* moddata, divrules_div node);
  int ntupletlevels(void* moddata, divrules_div node);
  fomus_rat tupletdur(void* moddata, divrules_div node, int lvl);
  int istupletbegin(void* moddata, divrules_div node, int lvl);
  int istupletend(void* moddata, divrules_div node, int lvl);
  const struct module_list getinitdivs(void* moddata);
  int iscompound(void* moddata);
  fomus_rat durmult(void* moddata, divrules_div node);
  int isnoteonly(void* moddata, divrules_div node);
  fomus_rat tuplet(void* moddata, divrules_div node, int lvl);
  }
  inline divrules_ornode expand(void* moddata, divrules_div node,
                                divrules_rangelist excl) {
#ifndef NDEBUGOUT
    ((basediv*) node)->print();
    divrules_ornode ret(((basediv*) node)->expand(*(rulesdata*) moddata, excl));
    for (divrules_andnode *a = ret.ands, *ae = ret.ands + ret.n; a < ae; ++a) {
      for (divrules_div *i = a->divs, *ie = a->divs + a->n; i < ie; ++i) {
        DBG("    ");
        ((basediv*) *i)->print();
      }
      DBG(std::endl);
    }
    return ret;
#else
    return ((basediv*) node)->expand(*(rulesdata*) moddata, excl);
#endif
  }
  inline void free_moddata(void* moddata) {
    delete (rulesdata*) moddata;
  }
  inline divrules_ornode get_root(void* moddata, struct fomus_rat time,
                                  struct module_list initdiv) {
    assert(((rulesdata*) moddata)->isvalid());
    return ((rulesdata*) moddata)->top(time, initdiv);
  }
  inline void free_node(void* moddata, divrules_div node) {
    assert(((basediv*) node)->isvalid());
    delete (basediv*) node;
  }
  inline struct fomus_rat rltime(void* moddata, divrules_div node) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->tim;
  }
  inline struct fomus_rat rlendtime(void* moddata, divrules_div node) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->endtime();
  }
  inline struct fomus_rat rldur(void* moddata, divrules_div node) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->dur;
  }
  inline int tieleftallowed(void* moddata, divrules_div node) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->tieleftallowed();
  }
  inline int tierightallowed(void* moddata, divrules_div node) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->tierightallowed();
  }
  inline int issmall(void* moddata, divrules_div node) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->issmall();
  }
  inline int ntupletlevels(void* moddata, divrules_div node) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->ntupletlevels();
  }
  inline fomus_rat tupletdur(void* moddata, divrules_div node, int lvl) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->tupletdur(lvl);
  }
  inline int istupletbegin(void* moddata, divrules_div node, int lvl) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->istupletbegin(lvl);
  }
  inline int istupletend(void* moddata, divrules_div node, int lvl) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->istupletend(lvl);
  }
  inline const struct module_list getinitdivs(void* moddata) {
    assert(((rulesdata*) moddata)->isvalid());
    return ((rulesdata*) moddata)->getinitdivs();
  }
  inline int iscompound(void* moddata) {
    assert(((rulesdata*) moddata)->isvalid());
    return ((rulesdata*) moddata)->iscompound();
  }
  inline fomus_rat durmult(void* moddata, divrules_div node) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->getdurmult();
  }
  inline int isnoteonly(void* moddata, divrules_div node) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->isnoteonly();
  }
  inline fomus_rat tuplet(void* moddata, divrules_div node, int lvl) {
    assert(((basediv*) node)->isvalid());
    return ((basediv*) node)->gettuplet(lvl);
  }

  const char* maxtupstype = "(integer>=1 integer>=1 ...)";
  int valid_maxtups(const struct module_value val) {
    return module_valid_listofints(val, -1, -1, 1, module_incl, 0,
                                   module_nobound, 0, maxtupstype);
  }
  const char* tuprattypetype = "pow2|diffsmall|difflarge|rat1small|rat1large";
  int valid_tuprattype_aux(const char* str) {
    return typmap.find(str) != typmap.end();
  }
  int valid_tuprattype(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_tuprattype_aux,
                               tuprattypetype);
  }
  const char* tupdivstype =
      "((rational>0 rational>0 ...) (rational>0 rational>0 ...) ...)";
  int valid_tupdivs_aux(int x, const struct module_value val) {
    return module_valid_listofrats(val, -1, -1, module_makerat(0, 1),
                                   module_excl, module_makerat(0, 1),
                                   module_nobound, 0, 0);
  }
  int valid_tupdivs(const struct module_value val) {
    return module_valid_listofvals(val, -1, -1, valid_tupdivs_aux, tupdivstype);
  }
  const char* measdivstype =
      "((rational>0 rational>0 ...) (rational>0 rational>0 ...) ...)";
  int valid_measdivs_aux(int x, const struct module_value val) {
    return module_valid_listofrats(val, -1, -1, module_makerat(0, 1),
                                   module_excl, module_makerat(0, 1),
                                   module_nobound, 0, 0);
  }
  int valid_measdivs(const struct module_value val) {
    return module_valid_listofvals(val, -1, -1, valid_measdivs_aux,
                                   measdivstype);
  }

  const char* mintuptype = "rational>=0";
  int valid_mintupdur(const struct module_value val) {
    return module_valid_rat(val, module_makerat(0, 1), module_incl,
                            module_makerat(0, 1), module_nobound, 0,
                            mintuptype);
  } // also maxp
  // int validdeps_mintupdur(FOMUS f, const struct module_value val) {return
  // GET_R(val) <= module_setting_rval(f, maxtupdurid);} int
  // validdeps_maxtupdur(FOMUS f, const struct module_value val) {return
  // module_setting_rval(f, mintupdurid) <= GET_R(val);}

  const char* largetupsizetype = "integer>=0";
  int valid_largetupsize(const struct module_value val) {
    return module_valid_int(val, 0, module_incl, 0, module_nobound, 0,
                            largetupsizetype);
  }
} // namespace divrules

using namespace divrules;

void aux_fill_iface(void* iface) {
  ((divrules_iface*) iface)->moddata =
      new rulesdata(((divrules_iface*) iface)->data);
  ((divrules_iface*) iface)->expand = divrules::expand;
  ((divrules_iface*) iface)->get_root = divrules::get_root;
  ((divrules_iface*) iface)->free_moddata = divrules::free_moddata;
  ((divrules_iface*) iface)->free_node = divrules::free_node;
  ((divrules_iface*) iface)->time = divrules::rltime;
  ((divrules_iface*) iface)->endtime = divrules::rlendtime;
  ((divrules_iface*) iface)->dur = divrules::rldur;
  ((divrules_iface*) iface)->tieleftallowed = divrules::tieleftallowed;
  ((divrules_iface*) iface)->tierightallowed = divrules::tierightallowed;
  ((divrules_iface*) iface)->issmall = divrules::issmall;
  ((divrules_iface*) iface)->ntupletlevels = divrules::ntupletlevels;
  ((divrules_iface*) iface)->leveldur = divrules::tupletdur;
  ((divrules_iface*) iface)->istupletbegin = divrules::istupletbegin;
  ((divrules_iface*) iface)->istupletend = divrules::istupletend;
  ((divrules_iface*) iface)->get_initdivs = divrules::getinitdivs;
  ((divrules_iface*) iface)->iscompound = divrules::iscompound;
  ((divrules_iface*) iface)->durmult = divrules::durmult;
  ((divrules_iface*) iface)->isnoteonly = divrules::isnoteonly;
  ((divrules_iface*) iface)->tuplet = divrules::tuplet;
}
const char* module_longname() {
  return "Divide Rules";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Implements rules for dividing and subdividing measures.";
}
enum module_type module_type() {
  return module_modaux;
}
int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "tuplets"; // docscat{tuplets}
    set->type = module_list_nums;
    set->descdoc =
        "List specifying the maximum tuplet division at each level (the first "
        "number in the list corresponds to the outermost tuplet level, etc.).  "
        "Set this to represent the highest tuplet division or divisions you "
        "want to appear in your score.  "
        "Increasing this increases computation time since FOMUS much search "
        "more for valid combinations of tuplet divisions.  "
        "The number of items in the list also specifies the number of nested "
        "tuplets allowed.  "
        "An empty list specifies that no tuplets are allowed at all.";
    set->typedoc = maxtupstype;

    module_setval_list(&set->val, 1);
    module_value* vals = set->val.val.l.vals;
    module_setval_int(vals + 0, 7);

    // if the location is changed, also change in lilyout.cc (and probably all
    // other backends)
    set->loc = module_locmeasdef;
    set->valid = valid_maxtups; // no range
    // set->validdeps = validdeps_minp;
    set->uselevel = 2;
    maxtupsid = id;
    break;
  }
  case 1: {
    set->name = "large-tuplet-ratiotype"; // docscat{tuplets}
    set->type = module_string;
    set->descdoc =
        "The rule used to calculate tuplet ratios when they are \"large.\"  "
        "What a large tuplet is is determined by `large-tuplet-size'.  "
        "Use this setting to break the \"power of two\" rule and avoid "
        "unintuitive tuplet ratios (like 31:16 instead of 31:32).  "
        "`pow2' indicates that the proper \"power of two\" rule be used where "
        "the denominator of the tuplet equals the highest power of two less "
        "than the numerator.  "
        "`diffsmall' indicates using a rule that the numerator be as close as "
        "possible to the denominator (using a smaller tuplet value if there "
        "happens to be a tie) while "
        "`difflarge' does the same, using the larger tuplet value if there is "
        "a tie.  "
        "`rat1small' indicates that the ratio be as close as possible to 1 "
        "while `rat1large' does the same, choosing the larger tuplet value if "
        "there happens to be a tie.";
    set->typedoc = tuprattypetype; // same valid fun

    module_setval_string(&set->val, "pow2");

    set->loc = module_locmeasdef;
    set->valid = valid_tuprattype; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2; // technical
    tuprattypeid = id;
    break;
  }
  case 2: {
    set->name = "default-tupletdivs"; // docscat{libs}
    set->type = module_list_numlists;
    set->descdoc =
        "A list of ways tuplets may be divided into smaller units, used to "
        "split and tie notes and rests."
        "  An entry of (3 2), for example, indicates that a tuplet with 5 in "
        "its numerator may be subdivided into 3 + 2."
        "  If you want to override these values to change the tuplet-dividing "
        "behavior of a single measure, use `tuplet-divs' instead.";
    set->typedoc = tupdivstype; // same valid fun

    int c = 0;
#define NTUPLETDIVS 38
    module_setval_list(&set->val, NTUPLETDIVS);
    struct module_list& li = set->val.val.l;
    { // 1 1
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 1);
      module_setval_int(li0.vals + 1, 1);
    }
    { // 2 1
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 1);
    }
    { // 1 2
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 1);
      module_setval_int(li0.vals + 1, 2);
    }
    { // 1 1 1
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 1);
      module_setval_int(li0.vals + 1, 1);
      module_setval_int(li0.vals + 2, 1);
    }
    { // 2 2
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 2);
    }
    { // 3 2
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 2);
    }
    { // 2 3
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 3);
    }
    { // 4 2
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 2);
    }
    { // 2 4
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 4);
    }
    { // 4 3
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 3);
    }
    { // 3 4
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 4);
    }
    { // 4 4
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 4);
    }
    { // 3 3 2
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 2);
    }
    { // 3 2 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 2 3 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 4 3 2
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 2);
    }
    { // 4 2 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 3 2 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 2 3 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 8 2
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 2);
    }
    { // 2 8
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 8);
    }
    { // 8 3
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 3);
    }
    { // 3 8
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 8);
    }
    { // 8 4
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 4);
    }
    { // 4 8
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 8);
    }
    { // 8 3 2
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 2);
    }
    { // 8 2 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 3 2 8
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 8);
    }
    { // 2 3 8
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 8);
    }
    { // 8 4 2
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 2);
    }
    { // 8 2 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 4 2 8
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 8);
    }
    { // 2 4 8
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 8);
    }
    { // 8 4 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 8 3 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 4 3 8
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 8);
    }
    { // 3 4 8
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 8);
    }
    { // 8 8
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 8);
    }
    assert(c == NTUPLETDIVS);
    set->loc = module_locmeasdef;
    set->valid = valid_tupdivs;
    set->uselevel = 2;
    tupdivsid = id;
    break;
  }
  case 3: {
    set->name = "default-measdivs"; // docscat{libs}
    set->type = module_list_numlists;
    set->descdoc = "A list of possible metrical divisions, used to split and "
                   "tie notes and rests."
                   "  An entry of (3 2), for example, indicates that a measure "
                   "with 5 beats can be subdivided into 3 + 2."
                   "  When you want to override these values to change how "
                   "individual measures are divided, use `meas-divs' instead.";
    set->typedoc = measdivstype;

    int c = 0;
#define NMEASDIVS 47
    module_setval_list(&set->val, NMEASDIVS);
    struct module_list& li = set->val.val.l;
    { // 1 1
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 1);
      module_setval_int(li0.vals + 1, 1);
    }
    { // 2 1
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 1);
    }
    { // 1 2
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 1);
      module_setval_int(li0.vals + 1, 2);
    }
    { // 1 1 1
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 1);
      module_setval_int(li0.vals + 1, 1);
      module_setval_int(li0.vals + 2, 1);
    }
    { // 2 2
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 2);
    }
    { // 3 2
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 2);
    }
    { // 2 3
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 3);
    }
    { // 2 2 2
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 2);
    }
    { // 4 3
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 3);
    }
    { // 3 4
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 4);
    }
    { // 3 2 2
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 2);
    }
    { // 2 3 2
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 2);
    }
    { // 2 2 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 4 4
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 4);
    }
    { // 3 3 2
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 2);
    }
    { // 3 2 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 2 3 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 4 3 2
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 2);
    }
    { // 4 2 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 3 2 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 2 3 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 3 2 2 2
      module_setval_list(li.vals + c, 4);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 2);
      module_setval_int(li0.vals + 3, 2);
    }
    { // 2 3 2 2
      module_setval_list(li.vals + c, 4);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 2);
      module_setval_int(li0.vals + 3, 2);
    }
    { // 2 2 3 2
      module_setval_list(li.vals + c, 4);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 3);
      module_setval_int(li0.vals + 3, 2);
    }
    { // 2 2 2 3
      module_setval_list(li.vals + c, 4);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 2);
      module_setval_int(li0.vals + 1, 2);
      module_setval_int(li0.vals + 2, 2);
      module_setval_int(li0.vals + 3, 3);
    }
    { // 4 3 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 3 4 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 3 3 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 5 5
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 5);
      module_setval_int(li0.vals + 1, 5);
    }
    { // 4 3 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 3 4 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 3 3 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 4 4 3
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 3);
    }
    { // 4 3 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 3);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 3 4 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 3);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 4 4 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 8 5
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 5);
    }
    { // 5 8
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 5);
      module_setval_int(li0.vals + 1, 8);
    }
    { // 5 4 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 5);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 4 5 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 5);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 4 4 5
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 7 7
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 7);
      module_setval_int(li0.vals + 1, 7);
    }
    { // 5 5 4
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 5);
      module_setval_int(li0.vals + 1, 5);
      module_setval_int(li0.vals + 2, 4);
    }
    { // 5 4 5
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 5);
      module_setval_int(li0.vals + 1, 4);
      module_setval_int(li0.vals + 2, 5);
    }
    { // 4 5 5
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 4);
      module_setval_int(li0.vals + 1, 5);
      module_setval_int(li0.vals + 2, 5);
    }
    { // 5 5 5
      module_setval_list(li.vals + c, 3);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 5);
      module_setval_int(li0.vals + 1, 5);
      module_setval_int(li0.vals + 2, 5);
    }
    { // 8 8
      module_setval_list(li.vals + c, 2);
      struct module_list& li0 = li.vals[c++].val.l;
      module_setval_int(li0.vals, 8);
      module_setval_int(li0.vals + 1, 8);
    }
    assert(c == NMEASDIVS);
    set->loc = module_locmeasdef;
    set->valid = valid_measdivs;
    set->uselevel = 2;
    setdefinitdivsid = id;
    break;
  }
  case 4: {
    set->name = "meas-divs"; // docscat{meas}
    set->type = module_list_numlists;
    set->descdoc = "A list of possible metrical divisions, used to split and "
                   "tie notes and rests.  "
                   "An entry of (3 2), for example, indicates that a measure "
                   "with 5 beats can be subdivided into 3 + 2.  "
                   "This overrides divisions in `default-measdivs' and should "
                   "be used to specify irregular divisions for a particular "
                   "section or group of measures.";
    set->typedoc = measdivstype;

    module_setval_list(&set->val, 0);

    set->loc = module_locmeasdef;
    set->valid = valid_measdivs;
    set->uselevel = 2;
    setinitdivsid = id;
    break;
  }
  case 5: {
    set->name = "tuplet-divs"; // docscat{tuplets}
    set->type = module_list_numlists;
    set->descdoc =
        "A list of ways tuplets may be divided into smaller units, used to "
        "split and tie notes and rests."
        "  An entry of (3 2), for example, indicates that a tuplet with 5 in "
        "its numerator may be subdivided into 3 + 2."
        "  This overrides divisions in `default-tupletdivs' and should be used "
        "to specify alternate behavior for a prticular section or group of "
        "measures."; // overrides default-tuplet-divs
    set->typedoc = measdivstype;

    module_setval_list(&set->val, 0);

    set->loc = module_locmeasdef;
    set->valid = valid_measdivs;
    set->uselevel = 2;
    deftupdivsid = id;
    break;
  }
  case 6: {
    set->name = "min-tupletdur"; // docscat{tuplets}
    set->type = module_rat;
    set->descdoc =
        "The minimum duration an outer-level tuplet is allowed to span when "
        "quantizing note times and durations.  "
        "Set this to the smallest tuplet duration you want to appear in your "
        "score.  "
        "A value of zero means there is no limit on minimum duration.";
    //	"Decreasing this value (or setting it to zero) increases computation
    // time and vice versa.";
    // 	"The level at which notes are allowed to be dotted.  This is only used
    // for quantizing.  "
    // 	"`top' specifies that dotted notes are allowed only when they take up a
    // full measure, "
    // 	"`div' specifies that they are allowed when they are more than a beat in
    // duration (i.e. \"metrical division\" level), "
    // 	"`all' specifies that they are always allowed, and `none' specifies that
    // they are never allowed.";
    set->typedoc = mintuptype; // same valid fun

    // module_setval_rat(&set->val, module_makerat(3, 2));
    module_setval_int(&set->val, 0); // 1

    set->loc = module_locmeasdef;
    set->valid = valid_mintupdur; // no range
    // set->validdeps = validdeps_mintupdur;
    set->uselevel = 2;
    mintupdurid = id;
    break;
  }
  case 7: {
    set->name = "max-tupletdur"; // docscat{tuplets}
    set->type = module_rat;
    set->descdoc =
        "The maximum duration an outer-level tuplet is allowed to span when "
        "quantizing note times and durations.  "
        "Set this to the largest tuplet duration you want to appear in your "
        "score.  "
        "A value of zero means there is no limit on maximum duration.";
    //	"Increasing this value (or setting it to zero) increases computation
    // time and vice versa.";
    // 	"The level at which notes are allowed to be dotted.  This is only used
    // for quantizing.  "
    // 	"`top' specifies that dotted notes are allowed only when they take up a
    // full measure, "
    // 	"`div' specifies that they are allowed when they are more than a beat in
    // duration (i.e. \"metrical division\" level), "
    // 	"`all' specifies that they are always allowed, and `none' specifies that
    // they are never allowed.";
    set->typedoc = mintuptype; // same valid fun

    module_setval_int(&set->val, 0); // 3

    set->loc = module_locmeasdef;
    set->valid = valid_mintupdur; // no range
    // set->validdeps = validdeps_maxtupdur;
    set->uselevel = 2;
    maxtupdurid = id;
    break;
  }
  case 8: {
    set->name = "large-tuplet-size"; // docscat{tuplets}
    set->type = module_rat;
    set->descdoc = "When the numerator of a tuplet is at least this value, the "
                   "tuplet is considered to be \"large.\""
                   "  You can specifiy that large tuplets be treated specially "
                   "when calculating the tuplet ratio."
                   "  Specify the type of special treatment you want with the "
                   "`large-tuplet-ratiotype' setting.";
    // 	"The level at which notes are allowed to be dotted.  This is only used
    // for quantizing.  "
    // 	"`top' specifies that dotted notes are allowed only when they take up a
    // full measure, "
    // 	"`div' specifies that they are allowed when they are more than a beat in
    // duration (i.e. \"metrical division\" level), "
    // 	"`all' specifies that they are always allowed, and `none' specifies that
    // they are never allowed.";
    set->typedoc = largetupsizetype; // same valid fun

    module_setval_int(&set->val, 8);

    set->loc = module_locmeasdef;
    set->valid = valid_largetupsize; // no range
    // set->validdeps = validdeps_maxtupdur;
    set->uselevel = 2;
    largetupsizeid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}
void module_ready() {
  nbeatsid = module_settingid("measdur");
  if (nbeatsid < 0) {
    ierr = "missing required setting `measdur'";
    return;
  }
  compid = module_settingid("comp");
  if (compid < 0) {
    ierr = "missing required setting `comp'";
    return;
  }
}
void module_init() {
  dvlvlmap.insert(
      std::map<std::string, divlvl>::value_type("none", div_lvlnone));
  dvlvlmap.insert(std::map<std::string, divlvl>::value_type("div", div_lvlsig));
  dvlvlmap.insert(std::map<std::string, divlvl>::value_type("top", div_lvltop));
  dvlvlmap.insert(std::map<std::string, divlvl>::value_type("all", div_lvlall));
  typmap.insert(std::map<std::string, tuptype>::value_type("pow2", tup_pow2));
  typmap.insert(
      std::map<std::string, tuptype>::value_type("diffsmall", tup_closenums));
  typmap.insert(
      std::map<std::string, tuptype>::value_type("difflarge", tup_closenuml));
  typmap.insert(
      std::map<std::string, tuptype>::value_type("rat1small", tup_close1s));
  typmap.insert(
      std::map<std::string, tuptype>::value_type("rat1large", tup_close1l));
}
void module_free() { /*assert(newcount == 0);*/
}
const char* module_initerr() {
  return ierr;
}
int aux_ifaceid() {
  return DIVRULES_INTERFACEID;
}
