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
#include <set>
#include <vector>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/utility.hpp>

#include "ifacedumb.h"
#include "module.h"
#include "modutil.h"

#include "debugaux.h"

namespace beams {

  const char* ierr = 0;

  int beamlong8thsid, compid, join16thsid;

  inline bool aligned(const fomus_rat& r1, const fomus_rat& i) {
    return (r1 * i).den == 1;
  }

  struct notedata {
    module_noteobj note;
    int bl, br;
    fomus_rat o;
    notedata(const module_noteobj note)
        : note(note), bl(0), br(0), o(module_time(note)) {}
    notedata(const module_noteobj note, int) : note(note), bl(-1) {
#ifndef NDEBUG
      br = -1;
#endif
    } // member of a chord (not the first)
    void doassign(int& bl0, int& br0) const {
      DBG("note at " << module_time(note) << " = " << bl << '/' << br
                     << std::endl);
      if (bl >= 0) {
        assert(br >= 0);
        beams_assign_beams(note, bl0 = bl, br0 = br);
      } else {
        assert(bl0 >= 0);
        beams_assign_beams(note, bl0, br0); // module_skipassign(note);
      }
    }
    bool dontneedlt(/*const int bms*/) const {
      return bl <= br;
    }
    bool dontneedrt(/*const int bms*/) const {
      return br <= bl;
    }
    bool dontwant(const int bms) const {
      return bl == br && bl >= bms;
    }
    // bool dontwantrt(const int bms) const {return br == bl && br >= bms;}
    void getridlt(const int bms) {
      bl = bms;
      assert(bl >= 0 && bl <= 16 && br >= 0 && br <= 16);
    }
    void getridrt(const int bms) {
      br = bms;
      assert(bl >= 0 && bl <= 16 && br >= 0 && br <= 16);
    }
    void reducelt(const int bms) {
      if (bms < bl)
        bl = bms;
      assert(bl >= 0 && bl <= 16 && br >= 0 && br <= 16);
    }
    void reducert(const int bms) {
      if (bms < br)
        br = bms;
      assert(bl >= 0 && bl <= 16 && br >= 0 && br <= 16);
    }
    void zero() {
      bl = br = 0;
      assert(bl >= 0 && bl <= 16 && br >= 0 && br <= 16);
    }
    bool checknotenough(const int bms) {
      return ((bl > 0 || br > 0) && bl < bms && br < bms);
    }
    bool hasnone() const {
      return bl <= 0 && br <= 0;
    }
    void adjone(const int bms) {
      if (bl > br)
        bl = bms;
      else if (br > bl)
        bl = bms;
      else {
        bl = br = bms;
      }
      assert(bl >= 0 && bl <= 16 && br >= 0 && br <= 16);
    }
    // int totbeam() const {return std::max(lb, lr);}
  };
  inline bool operator<(const fomus_rat& x, const notedata& y) {
    return x < y.o;
  }
  inline bool operator<(const notedata& x, const fomus_rat& y) {
    return x.o < y;
  }

  fomus_rat getsmallest(boost::ptr_vector<notedata>::iterator i,
                        const boost::ptr_vector<notedata>::iterator& p2,
                        const int lvl, bool& rst) {
    fomus_rat r = {std::numeric_limits<fomus_int>::max(), 1};
    while (i < p2) {
      if (module_isrest(i->note))
        rst = true;
      fomus_rat wr(module_adjdur(i->note, lvl));
      if (wr < r)
        r = wr;
      ++i;
    }
    return r;
  }

  void inspts(std::set<fomus_rat>& pts,
              const boost::ptr_vector<notedata>::const_iterator i1,
              const boost::ptr_vector<notedata>::const_iterator i2) {
    assert(i1 < i2);
    fomus_rat b(i1->o);
    for (boost::ptr_vector<notedata>::const_iterator i(i1); i < i2; ++i) {
      pts.insert(i->o - b);
      pts.insert(module_endtime(i->note) - b);
    }
  }

  void evalgrps(std::vector<boost::ptr_vector<notedata>::iterator>& grps) {
    if (grps.size() >= 3) {
      bool fi = true, sdq = true, syq = true; // sdq = ea. group has all same
                                              // dur., syq = symmetricality test
      std::set<fomus_rat> pts;
      for (std::vector<boost::ptr_vector<notedata>::iterator>::const_iterator i(
               boost::next(grps.begin()));
           i < grps.end(); ++i) {
        boost::ptr_vector<notedata>::iterator i1(*boost::prior(i)), i2(*i);
        assert(i1 < i2);
        if (sdq) {
          fomus_rat du(module_dur(i1->note));
          for (boost::ptr_vector<notedata>::const_iterator j(boost::next(i1));
               j < i2; ++j) {
            if (module_dur(j->note) != du) {
              sdq = false;
              if (!syq)
                goto SKIPTOREM;
              else
                break;
            }
          }
        }
        if (fi) {
          inspts(pts, i1, i2);
          fi = false;
        } else {
          if (syq) {
            std::set<fomus_rat> pts2;
            inspts(pts2, i1, i2);
            if (pts.size() != pts2.size() ||
                !std::equal(pts.begin(), pts.end(), pts2.begin()))
              syq = false;
          }
        }
        if (!sdq &&
            !syq) { // failure, remove all beam connections between groups!
        SKIPTOREM:
          for (std::vector<
                   boost::ptr_vector<notedata>::iterator>::const_iterator
                   i(boost::next(grps.begin())),
               ie(boost::prior(grps.end()));
               i < ie; ++i) {
            boost::ptr_vector<notedata>::iterator i2(*i);
            boost::ptr_vector<notedata>::iterator i1(boost::prior(i2));
            if (i1->dontneedrt() && i2->dontneedlt() && i1->br <= 1 &&
                i2->bl <= 1) {
              i1->reducert(0);
              i2->reducelt(0);
            }
          }
          break;
        }
      }
      for (std::vector<boost::ptr_vector<notedata>::iterator>::const_iterator
               i(boost::next(grps.begin())),
           ie(boost::prior(grps.end()));
           i < ie; ++i) {
        boost::ptr_vector<notedata>::iterator i2(*i);
        boost::ptr_vector<notedata>::iterator i1(boost::prior(i2));
        assert(i2 - *boost::prior(i) > 0);
        if (i2 - *boost::prior(i) <= 1) { // group on left has 1 note
          i1->zero();
          if (i2->dontneedlt())
            i2->reducelt(0);
        }
        assert(*boost::next(i) - i2 > 0);
        if (*boost::next(i) - i2 <= 1) { // group on right has 1 note
          if (i1->dontneedrt())
            i1->reducert(0);
          i2->zero();
        }
      }
    } else if (grps.size() >= 2 && grps[1] - grps[0] <= 1) {
      assert(grps[1] - grps[0] > 0);
      grps[0]->zero();
    }
  }

  inline bool isatrem2(const module_noteobj n) {
    module_markslist mks(module_singlemarks(n));
    for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n); m < me;
         ++m) {
      switch (module_markid(*m)) {
      case mark_trem:
      case mark_trem2:
        assert(module_marknum(*m).type == module_int);
        return module_marknum(*m).val.i < 0;
      }
    }
    return false;
  }

  boost::ptr_vector<notedata>::iterator
  beam(const boost::ptr_vector<notedata>::iterator& p1,
       boost::ptr_vector<notedata>::iterator p2, const int bms, const int bms2,
       const fomus_int tuplvl, const fomus_rat& wrm, const bool comp,
       const fomus_rat& strt) {
    if (p1 == p2)
      return p2;
    assert(p1 < p2);
    fomus_rat td = {1,
                    bms2 * 4}; // notated dur must be LESS than this to qualify
    bool rst = false;
    fomus_rat sm(getsmallest(p1, p2, tuplvl, rst)); // smallest duration
    if (sm * wrm >= td)
      return p2;
    boost::ptr_vector<notedata>::const_iterator l(boost::prior(p2)); // last one
    DBG("beaming " << module_time(p1->note) << " to " << module_endtime(l->note)
                   << " at lvl " << bms << std::endl);
    fomus_rat dtinv, dt,
        dt0; // dtinv = inverse of duration of a group of 4 for this level (or
             // greater)--dt is the actual duration
    fomus_rat rawt(module_endtime(l->note) - /*module_time(p1->note)*/ strt);
    fomus_rat wt0(module_beatstoadjdur(p1->note, rawt, tuplvl) *
                  wrm); // written duration of section
    bool sht =
        (bms == 1 && tuplvl <= 0 && sm * wrm <= module_makerat(1, 16) &&
         !module_setting_ival(
             p1->note, join16thsid)); // sht = true is exception to 8th beaming
    if (comp && tuplvl <= 0 && rawt > (fomus_int) 1)
      dt0 = module_makerat(
          3, bms2 * 8 /*(sht ? 8 : 4)*/); // sig-level compound meter, beam up
                                          // to groups of 6
    else if (tuplvl >= 1 && module_makerat(1, bms2 * 8) >=
                                wt0 / module_tuplet(p1->note, tuplvl - 1).num)
      dt0 = module_makerat(1, bms2 * 8); // tuplet--small groups
    else if (tuplvl >= 1 && module_makerat(1, bms2 * 4) >=
                                wt0 / module_tuplet(p1->note, tuplvl - 1).num)
      dt0 = module_makerat(1, bms2 * 4); // tuplet--small groups
    else
      dt0 = module_makerat(1, bms2 * (sht ? 4 : 2)); // normal
    dt = dt0;
    if ((tuplvl >= 1 && module_makerat(1, bms2 * 8) >=
                            wt0 / module_tuplet(p1->note, tuplvl - 1).num) ||
        (tuplvl <= 0 && bms == 1 &&
         module_setting_ival(p1->note,
                             beamlong8thsid))) { // super long primary beam
      while (dt <= wt0)
        dt = dt * (fomus_int) 2;
    }
    dtinv.num = dt.den; // dt is inverse
    dtinv.den = dt.num;
    assert(dt.den != 0);
    boost::ptr_vector<notedata>::iterator i0(p1), li0(p1),
        i(p1); // i0 is beginning of unprocessed area--i is incremented, i0 to i
               // is unprocessed range, li0 is further back
    fomus_rat st0(
        strt); // st0 remains aligned to original group duration (dti0)
    while (i < p2) {
      assert(i == p1 ||
             module_endtime(boost::prior(i)->note) <= module_time(i->note));
      bool iat;
      if (module_tupletbegin(i->note,
                             tuplvl)) { // an inner tuplet begins at this level
                                        // (0 = top, 1 = inside 1 tuplet, etc..)
        assert(bms == 1);
        beam(i0, i, bms + 1, bms2 * 2, tuplvl, wrm, comp,
             st0); // take care of notes up to this point
        boost::ptr_vector<notedata>::iterator j(i);
        assert(module_tupletbegin(i->note, tuplvl));
        while (!module_tupletend(j->note, tuplvl))
          ++j;
        li0 = i0 = i = beam(i, boost::next(j), 1, 1, tuplvl + 1, wrm, comp,
                            module_time(i->note)); // j might go beyond p2
#warning "beam tuplets to main notes if it fits in a division"
        if (i0 < p2) {
          fomus_rat d(module_time(i0->note) - strt); // time difference
          fomus_rat y(module_beatstoadjdur(i0->note, d, tuplvl) *
                      wrm); // adjusted full notated difference
          fomus_rat x(floorto(module_makeval(y), dt0)); // floored diff
          assert(x <= y);
          st0 = strt + d * (x / y); // new start time
        } else {
          p2 = i0;
          goto ALLDONE;
        }
      } else if (module_beatstoadjdur(i0->note, module_time(i->note) - st0,
                                      tuplvl) *
                     wrm >=
                 dt) {
        i0 = beam(i0, i, bms + 1, bms2 * 2, tuplvl, wrm, comp, st0);
        assert(i0 == i);
        fomus_rat d(module_time(i0->note) - strt); // time difference
        fomus_rat y(module_beatstoadjdur(i0->note, d, tuplvl) *
                    wrm); // adjusted full notated difference
        fomus_rat x(floorto(module_makeval(y), dt0)); // floored diff
        assert(x <= y);
        st0 = strt + d * (x / y); // new start time
      } else if (module_isnote(i->note) &&
                 module_adjdur(i->note, tuplvl) * wrm *
                         ((iat = isatrem2(i->note)) ? (fomus_int) 2
                                                    : (fomus_int) 1) <
                     td) {
        DBG("---NOTE AT " << module_time(i->note) << std::endl);
        fomus_rat wt(
            module_beatstoadjdur(i0->note, module_time(i->note) - st0, tuplvl) *
            wrm); // tuplvl should be 0 when outside tuplets, 1 for lvl1, etc.
        bool bl = false, br = false, noa = false;
        bool resl =
            (i > li0 && !module_isrest(boost::prior(i)->note) &&
             module_endtime(boost::prior(i)->note) >= module_time(i->note));
        bool resr =
            (i < l && !module_tupletbegin(boost::next(i)->note, tuplvl) &&
             !module_isrest(boost::next(i)->note) &&
             module_endtime(i->note) >= module_time(boost::next(i)->note));
        // bms0 = bms; //int bms0 = iat ? bms + 1 : bms; // int bms0 = iat ?
        // bms0
        // + 1 : bms;    compare with rev. 696
      AGAIN:
        if (i > li0 && resl &&
            module_adjdur(boost::prior(i)->note, tuplvl) * wrm <
                module_makerat(1, 4) // neighbor is small enough and isn't rest
            && (noa || !aligned(wt, dtinv))) {
          bl = true;
        }
        if (i < l && resr &&
            module_adjdur(boost::next(i)->note, tuplvl) * wrm <
                module_makerat(1, 4) // neighbor is small enough and isn't rest
            && (noa ||
                !aligned(wt + module_adjdur(i->note, tuplvl) * wrm, dtinv))) {
          br = true;
        }
        if (bl && !br) {
          i->bl = bms;
          assert(i->bl >= 0 && i->bl <= 16 && i->br >= 0 && i->br <= 16);
        } // one or the other
        else if (br && !bl) {
          i->br = bms;
          assert(i->bl >= 0 && i->bl <= 16 && i->br >= 0 && i->br <= 16);
        } else if (bl) {
          assert(br);
          i->bl = bms;
          i->br = bms;
          assert(i->bl >= 0 && i->bl <= 16 && i->br >= 0 && i->br <= 16);
        } else if (i->checknotenough(bms)) {
          assert(!bl && !br);
          if (!noa) {
            noa = true;
            goto AGAIN;
          } else {
            i->adjone(bms);
          }
        }
        ++i;
      } else
        ++i;
    }
    beam(i0, i, bms + 1, bms2 * 2, tuplvl, wrm, comp, st0);
  ALLDONE:
    fomus_rat dt1 = {dt0.den, dt0.num};
#ifndef NDEBUGOUT
    for (boost::ptr_vector<notedata>::iterator i(p1); i < p2; ++i) {
      DBG(module_time(i->note) << " l=" << i->bl << " r=" << i->br << " / ");
    }
    DBG(std::endl);
#endif
    std::vector<boost::ptr_vector<notedata>::iterator> grps;
    assert(bms > 0);
    if (tuplvl <= 0)
      grps.push_back(p1);
    for (boost::ptr_vector<notedata>::iterator i(boost::next(p1)); i < p2;
         ++i) { // fixup!
      boost::ptr_vector<notedata>::iterator pi(boost::prior(i));
      if (module_tupletbegin(pi->note, tuplvl)) {
        assert(!module_tupletend(pi->note, tuplvl));
        if (tuplvl <= 0) {
          assert(!grps.empty());
          if (pi > grps.back())
            grps.push_back(pi);
          evalgrps(grps);
          grps.clear();
        }
        while (!module_tupletend(i->note, tuplvl))
          ++i;
        if (tuplvl <= 0)
          grps.push_back(boost::next(i));
        continue;
      }
      fomus_rat wt(
          module_beatstoadjdur(
              i->note, module_time(i->note) - module_time(p1->note), tuplvl) *
          wrm);
      bool al = aligned(wt, dt1);
      if (al) {
        bool x = pi->dontneedrt() && pi->br > i->bl;
        if (i->dontneedlt() && i->bl > pi->br) {
          i->getridlt(pi->br);
        }
        if (x) {
          pi->getridrt(i->bl);
        }
      }
      bool x = i->br > pi->br && i->br >= i->bl;
      if (pi->bl > i->bl && pi->bl >= pi->br) {
        pi->br = i->bl;
        assert(pi->bl >= 0 && pi->bl <= 16 && pi->br >= 0 && pi->br <= 16);
      }
      if (x) {
        i->bl = pi->br;
        assert(i->bl >= 0 && i->bl <= 16 && i->br >= 0 && i->br <= 16);
      }
      if (tuplvl <= 0) {
        if (i->bl <= 0 || pi->br <= 0) {
          assert(!grps.empty());
          if (i > grps.back())
            grps.push_back(i);
          evalgrps(grps);
          grps.clear();
          grps.push_back(i);
        } else if (al) {
          assert(!grps.empty());
          if (i > grps.back())
            grps.push_back(i);
        }
      }
    }
    if (tuplvl <= 0) {
      assert(!grps.empty());
      if (p2 > grps.back())
        grps.push_back(p2);
      evalgrps(grps);
    }
#ifndef NDEBUGOUT
    for (boost::ptr_vector<notedata>::iterator i(p1); i < p2; ++i) {
      DBG("l=" << i->bl << " r=" << i->br << " / ");
    }
    DBG(std::endl);
#endif
    return p2;
  }

  fomus_rat getgrsmallest(boost::ptr_vector<notedata>::iterator i,
                          const boost::ptr_vector<notedata>::iterator& p2,
                          const int lvl) {
    fomus_rat r = {std::numeric_limits<fomus_int>::max(), 1};
    while (i < p2) {
      fomus_rat wr(module_adjgracedur(i->note, lvl));
      if (wr < r)
        r = wr;
      ++i;
    }
    return r;
  }
  boost::ptr_vector<notedata>::iterator
  grbeam(const boost::ptr_vector<notedata>::iterator& p1,
         const boost::ptr_vector<notedata>::iterator& p2, const int bms,
         const int bms2, const fomus_int tuplvl,
         const fomus_rat&
             wrm /*, const fomus_rat& strt*/ /*, const fomus_rat& wrm*/) {
    if (p1 == p2)
      return p2;
    assert(p1 < p2);
    fomus_rat td = {1,
                    bms2 * 4}; // written dur must be less than this to qualify
    if (getgrsmallest(p1, p2, tuplvl) * wrm >= td)
      return p2;
    boost::ptr_vector<notedata>::const_iterator l(boost::prior(p2)); // last
    boost::ptr_vector<notedata>::iterator i0(p1), li0(p1), i(p1);
    while (i < p2) {
#ifndef NDEBUGOUT
      if (i == p1)
        DBG("graceendtime = XXX gracetime = XXX" << std::endl);
      else
        DBG("graceendtime = " << module_graceendtime(boost::prior(i)->note)
                              << " gracetime = " << module_gracetime(i->note)
                              << std::endl);
#endif
      assert(i == p1 || module_graceendtime(boost::prior(i)->note) ==
                            module_gracetime(i->note));
      if (module_tupletbegin(i->note,
                             tuplvl)) { // an inner tuplet begins at this level
                                        // (0 = top, 1 = inside 1 tuplet, etc..)
        assert(bms == 1);
        grbeam(i0, i, bms + 1, bms2 * 2, tuplvl, wrm /*, st0*/);
        boost::ptr_vector<notedata>::iterator j(i);
        assert(module_tupletbegin(i->note, tuplvl));
        while (!module_tupletend(j->note, tuplvl))
          ++j;
        li0 = i0 = i = grbeam(
            i, boost::next(j), 1, 1, tuplvl + 1,
            wrm /*, module_gracetime(i->note)*/); // grace tuplets have to be
                                                  // specified by the user
        if (i0 >= p2)
          goto ALLDONE;
        //} else if (i > li0 && module_graceendtime(boost::prior(i)->note) !=
        // module_gracetime(i->note)) { i0 = grbeam(i0, i, bms + 1, bms2 * 2,
        // tuplvl, wrm /*, st0*/); assert(i0 == i);
      } else if (module_isnote(i->note) &&
                 module_adjgracedur(i->note, tuplvl) * wrm < td) {
        bool bl = false, br = false;
        if (i > li0 && !module_isrest(boost::prior(i)->note) &&
            module_adjgracedur(boost::prior(i)->note, tuplvl) * wrm <
                module_makerat(1,
                               4)) { // neighbor is small enough and isn't rest
          bl = true;                 // i->bl = bms;
        }
        if ((i < l && !module_tupletbegin(boost::next(i)->note, tuplvl)) &&
            !module_isrest(boost::next(i)->note) &&
            module_adjgracedur(boost::next(i)->note, tuplvl) * wrm <
                module_makerat(1,
                               4)) { // neighbor is small enough and isn't rest
          br = true;                 // i->br = bms;
        }
        if (bl && !br) {
          i->bl = bms; /*more = true;*/ /*break;*/
        }                               // one or the other
        else if (br && !bl) {
          i->br = bms; /*more = true;*/ /*break;*/
        } else if (bl) {
          assert(br);
          i->bl = bms;
          i->br = bms; /*break;*/
        }
        ++i;
      } else
        ++i;
    }
    grbeam(i0, i, bms + 1, bms2 * 2, tuplvl, wrm /*, st0*/);
  ALLDONE:
    for (boost::ptr_vector<notedata>::iterator i(boost::next(p1)); i < p2;
         ++i) { // fixup!
      if (module_tupletbegin(i->note, tuplvl)) {
        while (!module_tupletend(i->note, tuplvl))
          ++i;
        continue;
      }
      boost::ptr_vector<notedata>::iterator pi(boost::prior(i));
      if (pi->bl > i->bl && i->bl > pi->br) {
        pi->br = i->bl;
      }
      if (i->br > pi->br && pi->br > i->bl) {
        i->bl = pi->br;
      }
    }
    return p2;
  }

  // beam groups (sixteenth beams within eighth beam):
  // must be: using one dur per group
  //          or same pattern each group

#warning "grace beams should be separate module?"
  void dobeams() {
    boost::ptr_vector<notedata, boost::view_clone_allocator> notes, grnotes;
    boost::ptr_vector<notedata> allnotes;
    module_measobj m = module_nextmeas();
    bool comp = module_setting_ival(m, compid);
    while (true) {
      module_noteobj o = module_nextnote();
      if (!o)
        break;
      if (module_isbeginchord(o)) {
        notedata* x;
        allnotes.push_back(x = new notedata(o));
        if (module_isgrace(o))
          grnotes.push_back(x);
        else {
          DBG("pushing back " << (module_isrest(o) ? "rest " : "note ") << "at "
                              << module_time(o) << " - " << module_endtime(o)
                              << std::endl);
          notes.push_back(x);
        }
      } else
        allnotes.push_back(new notedata(o, 0));
    }
    module_ratslist dvs(module_divs(m));
    fomus_rat wrm(module_writtenmult(m));
    assert(dvs.n > 0);
    if (!notes.empty()) {
      boost::ptr_vector<notedata>::iterator np(notes.begin());
      fomus_rat d = module_time(m);
      const fomus_rat* i = dvs.rats;
      for (const fomus_rat* ie = dvs.rats + dvs.n; i < ie; ++i) {
        d = d + *i;
        np = beam(np, std::lower_bound(np, notes.end(), d), 1, 1, 0, wrm, comp,
                  module_time(np->note));
        if (np == notes.end())
          break;
      }
    }
    if (!grnotes.empty()) {
      boost::ptr_vector<notedata>::iterator np(grnotes.begin());
      while (np != grnotes.end()) {
        boost::ptr_vector<notedata>::iterator npe(
            std::upper_bound(np, grnotes.end(), np->o));
        grbeam(np, npe, 1, 1, 0, wrm);
        np = npe;
      }
    }
#ifndef NDEBUG
    int bl0 = -1, br0;
#else
    int bl0, br0;
#endif
    std::for_each(allnotes.begin(), allnotes.end(),
                  boost::lambda::bind(&notedata::doassign, boost::lambda::_1,
                                      boost::lambda::var(bl0),
                                      boost::lambda::var(br0)));
  }

  extern "C" {
  void run_fun(FOMUS fom, void* moddata); // return true when done
  const char* err_fun(void* moddata);
  }
  void run_fun(FOMUS fom, void* moddata) {
    dobeams();
  }
  const char* err_fun(void* moddata) {
    return 0;
  }

} // namespace beams

using namespace beams;

const char* module_engine(void* d) {
  return "dumb";
}
void module_fill_iface(void* moddata, void* iface) {
  ((dumb_iface*) iface)->moddata = moddata;
  ((dumb_iface*) iface)->run = run_fun;
  ((dumb_iface*) iface)->err = err_fun;
};
const char* module_longname() {
  return "Beams";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Beams notes according to metrical divisions.";
}
void* module_newdata(FOMUS f) {
  return 0;
}
void module_freedata(void* dat) {}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modbeams;
}
const char* module_initerr() {
  return ierr;
}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}
int module_itertype() {
  return module_bymeas | module_byvoice;
} // notes aren't divided yet, so they reach across measures
int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "extralong-8thbeams"; // docscat{beams}
    set->type = module_bool;
    set->descdoc = "Whether or not to beam together groups of more than four "
                   "eighth notes.  "
                   "Set this to `yes' in measures or parts where you want "
                   "extra-long eighth note beams.";
    // set->typedoc = beam8thtype;

    module_setval_int(&set->val, 0);

    set->loc = module_locnote;
    // set->valid = valid_quantdiv;
    set->uselevel = 2;
    beamlong8thsid = id;
  } break;
  case 1: {
    set->name = "long-8thbeams"; // docscat{beams}
    set->type = module_bool;
    set->descdoc =
        "Whether or not to group together four or more eighth notes when 16th "
        "(or smaller) notes are beamed with them.  "
        "Set this to `yes', for example, if you want a secondary eighth-note "
        "beam to connect neighboring groups of sixteenth notes.";
    // set->typedoc = beam8thtype;

    module_setval_int(&set->val, 0);

    set->loc = module_locnote;
    // set->valid = valid_quantdiv;
    set->uselevel = 2;
    join16thsid = id;
  } break;
  default:
    return 0;
  }
  return 1;
}
int module_engine_iface() {
  return ENGINE_INTERFACEID;
} // dumb
void module_ready() {
  compid = module_settingid("comp");
  if (compid < 0) {
    ierr = "missing required setting `comp'";
    return;
  }
}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
