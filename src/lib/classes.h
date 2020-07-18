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

#ifndef FOMUS_CLASSES_H
#define FOMUS_CLASSES_H

#ifndef BUILD_LIBFOMUS
#error "classes.h shouldn't be included"
#endif

#include "error.h" // errbase
#include "heads.h"
#include "instrs.h"
#include "marks.h"
#include "numbers.h"
#include "vars.h" // keysig stuff

namespace fomus {

  typedef boost::upgrade_lock<boost::shared_mutex> uplock;
  typedef boost::shared_lock<boost::shared_mutex> shlock;
  typedef boost::unique_lock<boost::shared_mutex> wrlock;

#ifndef NDEBUG
  extern boost::thread_specific_ptr<bool> lockcheck;
  struct lcheck {
    lcheck() {
      if (lockcheck.get()) {
        assert(!*lockcheck);
        *lockcheck = true;
      }
    }
    void unlock() {
      if (lockcheck.get())
        *lockcheck = false;
    }
    void relock() {
      if (lockcheck.get())
        *lockcheck = true;
    }
    ~lcheck() {
      if (lockcheck.get())
        *lockcheck = false;
    }
  };
#define READLOCK                                                               \
  lcheck zzz000;                                                               \
  boost::shared_lock<boost::shared_mutex> xxx((boost::shared_mutex&) mut)
#define UPREADLOCK                                                             \
  lcheck zzz000;                                                               \
  boost::upgrade_lock<boost::shared_mutex> xxx((boost::shared_mutex&) mut)
#define TOWRITELOCK boost::upgrade_to_unique_lock<boost::shared_mutex> yyy(xxx)
#define WRITELOCK                                                              \
  lcheck zzz000;                                                               \
  boost::unique_lock<boost::shared_mutex> yyy((boost::shared_mutex&) mut)
#define UNLOCK                                                                 \
  zzz000.unlock();                                                             \
  xxx.unlock()
#define TOWRITEUNLOCK yyy.unlock();
  template <typename T, typename L>
  inline T unlockandpass(const T x, L& xxx, lcheck& zzz000) {
    UNLOCK;
    return x;
  }
#define UNLOCKP(aaa) unlockandpass(aaa, xxx, zzz000)
#define UNLOCKWRITEP(aaa) unlockandpass(aaa, yyy, zzz000)
  struct unlockread {
    boost::shared_lock<boost::shared_mutex>& xxx;
    lcheck& zzz000;
    unlockread(boost::shared_lock<boost::shared_mutex>& xxx, lcheck& zzz000)
        : xxx(xxx), zzz000(zzz000) {
      UNLOCK;
    }
    ~unlockread() {
      zzz000.relock();
      xxx.lock();
    }
  };
  struct unlockupread {
    boost::upgrade_lock<boost::shared_mutex>& xxx;
    lcheck& zzz000;
    unlockupread(boost::upgrade_lock<boost::shared_mutex>& xxx, lcheck& zzz000)
        : xxx(xxx), zzz000(zzz000) {
      UNLOCK;
    }
    ~unlockupread() {
      zzz000.relock();
      xxx.lock();
    }
  };
  struct unlockwrite {
    boost::unique_lock<boost::shared_mutex>& xxx;
    lcheck& zzz000;
    unlockwrite(boost::unique_lock<boost::shared_mutex>& xxx, lcheck& zzz000)
        : xxx(xxx), zzz000(zzz000) {
      UNLOCK;
    }
    ~unlockwrite() {
      zzz000.relock();
      xxx.lock();
    }
  };
#define UNLOCKREAD unlockread yyy000(xxx, zzz000)
#define UNLOCKUPREAD unlockupread yyy000(xxx, zzz000)
#define UNLOCKWRITE unlockwrite yyy000(yyy, zzz000)
#define SHLOCKPAR shlock &xxx, lcheck &zzz000
#define UPLOCKPAR uplock &xxx, lcheck &zzz000
#define WRLOCKPAR wrlock &yyy, lcheck &zzz000
#define LOCKARG xxx, zzz000
#define WRLOCKARG yyy, zzz000
#define RELOCK                                                                 \
  zzz000.relock();                                                             \
  xxx.lock()
#else
#define READLOCK                                                               \
  boost::shared_lock<boost::shared_mutex> xxx((boost::shared_mutex&) mut)
#define UPREADLOCK                                                             \
  boost::upgrade_lock<boost::shared_mutex> xxx((boost::shared_mutex&) mut)
#define TOWRITELOCK boost::upgrade_to_unique_lock<boost::shared_mutex> yyy(xxx)
#define WRITELOCK                                                              \
  boost::unique_lock<boost::shared_mutex> yyy((boost::shared_mutex&) mut)
#define UNLOCK xxx.unlock()
#define TOWRITEUNLOCK yyy.unlock();
  template <typename T, typename L>
  inline T unlockandpass(const T x, L& xxx) {
    UNLOCK;
    return x;
  }
#define UNLOCKP(aaa) unlockandpass(aaa, xxx)
#define UNLOCKWRITEP(aaa) unlockandpass(aaa, yyy)
  struct unlockread {
    boost::shared_lock<boost::shared_mutex>& xxx;
    unlockread(boost::shared_lock<boost::shared_mutex>& xxx) : xxx(xxx) {
      UNLOCK;
    }
    ~unlockread() {
      xxx.lock();
    }
  };
  struct unlockupread {
    boost::upgrade_lock<boost::shared_mutex>& xxx;
    unlockupread(boost::upgrade_lock<boost::shared_mutex>& xxx) : xxx(xxx) {
      UNLOCK;
    }
    ~unlockupread() {
      xxx.lock();
    }
  };
  struct unlockwrite {
    boost::unique_lock<boost::shared_mutex>& xxx;
    unlockwrite(boost::unique_lock<boost::shared_mutex>& xxx) : xxx(xxx) {
      UNLOCK;
    }
    ~unlockwrite() {
      xxx.lock();
    }
  };
#define UNLOCKREAD unlockread yyy000(xxx)
#define UNLOCKUPREAD unlockupread yyy000(xxx)
#define UNLOCKWRITE unlockwrite yyy000(yyy)
#define SHLOCKPAR shlock& xxx
#define UPLOCKPAR uplock& xxx
#define WRLOCKPAR wrlock& yyy
#define LOCKARG xxx
#define WRLOCKARG yyy
#define RELOCK xxx.lock()
#endif

// wrapper for any value w/ mutex checks
#ifndef NDEBUG
  template <typename T>
  struct mutcheck _NONCOPYABLE {
    T val;
    boost::shared_mutex* mut;
    mutcheck(boost::shared_mutex* mut) : mut(mut) {}
    mutcheck(const mutcheck<T>& x, boost::shared_mutex* mut)
        : val(x.val), mut(mut) {}
    mutcheck(mutcheck<T>& x, boost::shared_mutex* mut) : val(x.val), mut(mut) {}
    template <typename V>
    mutcheck(V& val, boost::shared_mutex* mut) : val(val), mut(mut) {}
    template <typename V>
    mutcheck(const V& val, boost::shared_mutex* mut) : val(val), mut(mut) {}
    template <typename V1, typename V2>
    mutcheck(V1& val1, V2& val2, boost::shared_mutex* mut)
        : val(val1, val2), mut(mut) {}
    template <typename V1, typename V2>
    mutcheck(const V1& val1, V2& val2, boost::shared_mutex* mut)
        : val(val1, val2), mut(mut) {}
    template <typename V1, typename V2>
    mutcheck(V1& val1, const V2& val2, boost::shared_mutex* mut)
        : val(val1, val2), mut(mut) {}
    template <typename V1, typename V2>
    mutcheck(const V1& val1, const V2& val2, boost::shared_mutex* mut)
        : val(val1, val2), mut(mut) {}
    T& wget() {
      if (mut && lockcheck.get())
        assert(!mut->try_lock_shared());
      return val;
    } // NOT WRITE LOCKED!
    const T& wget() const {
      if (mut && lockcheck.get())
        assert(!mut->try_lock_shared());
      return val;
    }
    T& rget() {
      if (mut && lockcheck.get())
        assert(!mut->try_lock());
      return val;
    } // NOT LOCKED!
    const T& rget() const {
      if (mut && lockcheck.get())
        assert(!mut->try_lock());
      return val;
    }
    T& cget() {
      return val;
    }
    const T& cget() const {
      return val;
    }
    T& xget() {
      assert(!mut || !lockcheck.get());
      return val;
    } // NOT MUTEX-CHECK DISABLED!
    const T& xget() const {
      assert(!mut || !lockcheck.get());
      return val;
    }
  };
  struct disable_mutcheck {
    bool* bak;
    disable_mutcheck() : bak(lockcheck.release()) {}
    ~disable_mutcheck() {
      lockcheck.reset(bak);
    }
  };
#define MUTCHECK(xxx) mutcheck<xxx>
#define MUTCHECK2(xxx1, xxx2) mutcheck<xxx1, xxx2>
#define _MUT , &mut
#define _MUTP , mut
#define MUTINIT0(xxx) xxx(0)
#define MUTINIT0_(xxx) xxx(0),
#define _MUTINIT0(xxx) , xxx(0)
#define _MUTINIT(xxx) , xxx(&mut)
#define _MUTINITP(xxx) , xxx(mut)
#define MUTINITP_(xxx) xxx(mut),
#define RMUT(xxx) (xxx.rget())
#define WMUT(xxx) (xxx.wget())
#define XMUT(xxx) (xxx.xget())
#define CMUT(xxx) (xxx.cget())
#define MUTPARAM_ boost::shared_mutex *mut,
#define MUTPARAM boost::shared_mutex* mut
#define WITHMUT_ &mut,
#define WITHMUT &mut
#define WITHMUTP_ mut,
#define WITHOUTMUT_ (boost::shared_mutex*) 0,
#define STOREMUT boost::shared_mutex* mut111;
  struct mutdbg000 {};
#define MUTDBG mutdbg000()
#define _MUTDBG , mutdbg000()
#define MUTDBGPAR const mutdbg000& mmm111
#define DISABLEMUTCHECK disable_mutcheck mmm222
#else
#define MUTCHECK(xxx) xxx
#define MUTCHECK2(xxx1, xxx2) xxx1, xxx2
#define _MUT
#define _MUTP
#define MUTINIT0(xxx)
#define MUTINIT0_(xxx)
#define _MUTINIT0(xxx)
#define _MUTINIT(xxx)
#define _MUTINITP(xxx)
#define MUTINITP_(xxx)
#define RMUT(xxx) xxx
#define WMUT(xxx) xxx
#define XMUT(xxx) xxx
#define CMUT(xxx) xxx
#define MUTPARAM_
#define MUTPARAM
#define WITHMUT_
#define WITHMUT
#define WITHMUTP_
#define WITHOUTMUT_
#define STOREMUT
#define MUTDBG
#define _MUTDBG
#define MUTDBGPAR
#define DISABLEMUTCHECK
#endif

  class assmarkerr : public errbase {};
  struct badtuplvl : public errbase {};
  // struct badunsplit:public errbase {};

  inline void integerr(const char* str) {
    ferr << " found during " << str << " integrity check" << std::endl;
    assert(false);
    throw errbase();
  }

  typedef std::vector<fint> fintvect;
  typedef fintvect::iterator fintvect_it;
  typedef fintvect::const_iterator fintvect_constit;

  class offbase { // is an "input" object
protected:
    MUTCHECK(offgroff) off; // no mutex--updates are only done non-concurrently
public:
    offbase(MUTPARAM_ const numb& off, const numb& groff)
        : off(off, groff _MUTP) {}
    offbase(MUTPARAM_ const offgroff& off) : off(off _MUTP) {}
    virtual ~offbase() {}
    offbase(MUTPARAM_ offbase& x) : off(x.off _MUTP) {}
    offbase(MUTPARAM_ offbase& x, const numb& shift)
        : off((shift.isnull() ? RMUT(x.off).off : RMUT(x.off).off + shift),
              RMUT(x.off).groff _MUTP) {}
#ifndef NDEBUG
    offbase(MUTDBGPAR) : MUTINIT0(off) {
      assert(false);
    }

private:
    offbase(const offbase& x) : MUTINIT0(off) {
      assert(false);
    }

public:
#endif
    bool isgrace() const {
      return RMUT(off).isgrace();
    }
#ifndef NDEBUG
    bool isgrace(MUTDBGPAR) const {
      return CMUT(off).isgrace();
    }
#endif
    void dectime(const numb& tim) {
      WMUT(off).off = RMUT(off).off - tim;
    }
    bool operator<(const offbase& y) const {
      return RMUT(off) < RMUT(y.off);
    }
  };

  class measure;
  class event : public modobjbase_sets NONCOPYABLE {
protected:
    boost::shared_mutex mut; //, cachemut;
    measure* meas;           // events belong to a measure
    const filepos pos;
    setmap sets; // not a set, it's a map<const int, boost::shared_ptr<const
                 // varbase> >::type
    MUTCHECK(clef_str*)
    clf; // attributes: there is one of these for each staff/part
    MUTCHECK(staff_str*)
    stf; // attributes: there are separate staff instances in each part
    eventmap_it self;

public:
    event(const filepos& pos)
        : meas(0), pos(pos), clf((clef_str*) 0 _MUT), stf((staff_str*) 0 _MUT) {
      assert(isvalid());
    }
    event(const filepos& pos, const setmap& sets0)
        : meas(0), pos(pos), sets(sets0.begin(), sets0.end()),
          clf((clef_str*) 0 _MUT), stf((staff_str*) 0 _MUT) {}
    event(measure* meas, const filepos& pos)
        : meas(meas), pos(pos), clf((clef_str*) 0 _MUT),
          stf((staff_str*) 0 _MUT) {}
    event(event& x)
        : meas(0), pos(x.pos), sets(x.sets), clf(x.clf _MUT), stf(x.stf _MUT) {}
    event(const event& x, int)
        : meas(0), pos(x.pos), sets(x.sets), clf((clef_str*) 0 _MUT),
          stf((staff_str*) 0 _MUT) {}
    // boost::shared_mutex* getmut() {return &mut;}
    boost::shared_mutex& interngetmut() {
      return mut;
    }
    void setmeas(measure* m, const eventmap_it& it) {
      meas = m;
      self = it;
    }
    measure& getmeas() const {
      return *meas;
    }
    measure* getmeasptr() const {
      return meas;
    }
    const filepos& getfilepos() const {
      return pos;
    }
    const char* getfileposstr() const {
      std::ostringstream s;
      pos.printerr0(s);
      return make_charptr(s);
    }
    const varbase& get_varbase(const int id) const {
      return get_varbase0(id);
    }
    fint get_ival(const int id) const {
      return get_ival0(id);
    }
    rat get_rval(const int id) const {
      return get_rval0(id);
    }
    ffloat get_fval(const int id) const {
      return get_fval0(id);
    }
    const std::string& get_sval(const int id) const {
      return get_sval0(id);
    }
    const module_value& get_lval(const int id) const {
      return get_lval0(id);
    }
    const varbase& get_varbase_nomut(const int id) const {
      return get_varbase0(id, true);
    }
    fint get_ival_nomut(const int id) const {
      return get_ival0(id, true);
    }
    rat get_rval_nomut(const int id) const {
      return get_rval0(id, true);
    }
    ffloat get_fval_nomut(const int id) const {
      return get_fval0(id, true);
    }
    const std::string& get_sval_nomut(const int id) const {
      return get_sval0(id, true);
    }
    const module_value& get_lval_nomut(const int id) const {
      return get_lval0(id, true);
    }
    const varbase& get_varbase0(const int id, const bool nomut = false) const;
    fint get_ival0(const int id, const bool nomut = false) const;
    rat get_rval0(const int id, const bool nomut = false) const;
    ffloat get_fval0(const int id, const bool nomut = false) const;
    const std::string& get_sval0(const int id, const bool nomut = false) const;
    const module_value& get_lval0(const int id, const bool nomut = false) const;
    module_percinstobj getpercinst() const {
      return 0;
    } // perc instr returns something different
    virtual const percinstr_str* getpercinst0() const {
      return 0;
    } // perc instr returns something different
    const char* gettype() const {
      return "a note";
    }
    const eventmap_it& getnoteselfit() const {
      return self;
    }
    modobjbase* getmeasobj();
    modobjbase* getpartobj();
    modobjbase* getinstobj();
    fint partindex() const;
    eventmap_it& getself() {
      return self;
    }
    const eventmap_it& getself() const {
      return self;
    }
    info_setlist& getsettinginfo();
  };

  struct tupstruct {
    bool beg, end;
    fomus_rat
        tup; // the tuplet--***stored in fomus_rat so that it's unreduced***
    bool invisible;
    fomus_rat totdur; // not used internally, just store as fomus_rat
    tupstruct(const fomus_rat& tup, const bool beg, const bool end,
              const fomus_rat& totdur)
        : beg(beg), end(end), tup(tup), invisible(false), totdur(totdur) {}
    tupstruct(const divide_tuplet& x)
        : beg(x.begin), end(x.end), tup(x.tuplet), invisible(false),
          totdur(x.fulldur) {}
    rat getrat() const {
      return rat(tup.num, tup.den);
    }
    tupstruct getrtie() {
      tupstruct r(tup, false, end, totdur);
      end = false;
      return r;
    }
    bool operator!=(const tupstruct& x) const {
      return (beg != x.beg || end != x.end || tup != x.tup);
    }
    void setnotend() {
      end = false;
    }
    void setnotbeg() {
      beg = false;
    }
  };

  enum pointtype {
    point_none = 0x0,
    point_left = 0x1,
    point_right = 0x2,
    point_grleft = 0x4,
    point_grauto = 0x8,
    point_grright = 0x10,
    point_auto = 0x20,
  };

  class durbase : public offbase {
protected:
    MUTCHECK(numb) dur;
    MUTCHECK(std::vector<tupstruct>)
    tups; // don't need shared_ptr--this will change for ea. tied note
    MUTCHECK(pointtype) point;

public:
    durbase(MUTPARAM_ const numb& off, const numb& groff, const numb& dur)
        : offbase(WITHMUTP_ off, groff), dur(dur _MUTP) _MUTINITP(tups),
          point(point_none _MUTP) {}
    durbase(MUTPARAM_ const numb& off, const numb& groff, const numb& dur,
            const pointtype point)
        : offbase(WITHMUTP_ off, groff), dur(dur _MUTP) _MUTINITP(tups),
          point(point _MUTP) {}
    durbase(MUTPARAM_ durbase& x, const numb& off1)
        : offbase(WITHMUTP_ x.isgrace() ? offgroff(RMUT(x.off).off, off1)
                                        : offgroff(off1, module_none)),
          dur(RMUT(x.off).off + RMUT(x.dur) - off1 _MUTP) _MUTINITP(tups),
          point(x.point _MUTP) { // for splitting, this is rt note
      assert(CMUT(dur) > (fint) 0);
      WMUT(x.dur) = off1 - RMUT(x.off).off;
      std::transform(
          WMUT(x.tups).begin(), WMUT(x.tups).end(),
          std::back_inserter(CMUT(tups)),
          boost::lambda::bind(&tupstruct::getrtie, boost::lambda::_1));
    } // shouldn't split grace notes
    durbase(MUTPARAM_ durbase& x, const numb& off1, int)
        : offbase(WITHMUTP_ x.isgrace() ? offgroff(RMUT(x.off).off, off1)
                                        : offgroff(off1, module_none)),
          dur((x.isgrace() ? RMUT(x.off).groff : RMUT(x.off).off) +
              RMUT(x.dur) - off1 _MUTP) _MUTINITP(tups),
          point(x.point _MUTP) { // for splitting, this is rt note
      assert(CMUT(dur) > (fint) 0);
      WMUT(x.dur) =
          off1 - (x.isgrace() ? RMUT(x.off).groff
                              : RMUT(x.off).off); // NO SPLITTING TUPLETS--this
                                                  // is for the postties routine
    } // shouldn't split grace notes
    durbase(MUTPARAM_ durbase& x)
        : offbase(WITHMUTP_ x), dur(x.dur _MUTP) _MUTINITP(tups),
          point(x.point _MUTP) {}
    durbase(MUTPARAM_ durbase& x, const int, const numb& shift)
        : offbase(WITHMUTP_ x, shift), dur(x.dur _MUTP) _MUTINITP(tups),
          point(x.point _MUTP) {}
#ifndef NDEBUG
private:
    durbase(const durbase& x)
        : offbase(MUTDBG) _MUTINIT0(dur) _MUTINIT0(tups) _MUTINIT0(point) {
      assert(false);
    }

public:
#endif
    void setdur(const numb& du) {
      WMUT(dur) = du;
    }
#ifndef NDEBUG
    void setdur(const numb& du, MUTDBGPAR) {
      CMUT(dur) = du;
    }
#endif
    void adjusttime(const fomus_rat& o) {
      WMUT(dur) = RMUT(off).off + RMUT(dur) - o;
      WMUT(off).off = o;
      assert(RMUT(off).groff.isnull());
    }
#ifndef NDEBUG
    void adjusttime(const fomus_rat& o, MUTDBGPAR) {
      CMUT(dur) = CMUT(off).off + CMUT(dur) - o;
      CMUT(off).off = o;
      assert(CMUT(off).groff.isnull());
    }
#endif
    void adjustdur(const fomus_rat& d) {
      WMUT(dur) = d;
      assert(d > (fomus_int) 0);
    } // also gracedur
#ifndef NDEBUG
    void adjustdur(const fomus_rat& d, MUTDBGPAR) {
      CMUT(dur) = d;
      assert(d > (fomus_int) 0);
    } // also gracedur
#endif
    void adjustdur(const numb& d) {
      WMUT(dur) = d;
      assert(d > (fomus_int) 0);
    }
#ifndef NDEBUG
    void adjustdur(const numb& d, MUTDBGPAR) {
      CMUT(dur) = d;
      assert(d > (fomus_int) 0);
    }
#endif
    void adjustgracetime(const fomus_rat& o) {
      assert(RMUT(off).groff.isntnull());
      WMUT(dur) = RMUT(off).groff + RMUT(dur) - o;
      WMUT(off).groff = o;
    }
#ifndef NDEBUG
    void adjustgracetime(const fomus_rat& o, MUTDBGPAR) {
      assert(CMUT(off).groff.isntnull());
      CMUT(dur) = CMUT(off).groff + CMUT(dur) - o;
      CMUT(off).groff = o;
    }
#endif
    void filltups(const std::vector<tupstruct>& tps) {
      WMUT(tups).assign(tps.begin(), tps.end());
    }
#ifndef NDEBUG
    void filltups(const std::vector<tupstruct>& tps, MUTDBGPAR) {
      CMUT(tups).assign(tps.begin(), tps.end());
    }
#endif
    rat gettupmult(const int level) const {
      int n = level < 0 ? RMUT(tups).size()
                        : std::min((int) RMUT(tups).size(), level);
      rat m(1, 1);
      for (int i = 0; i < n; ++i)
        m *= RMUT(tups)[i].getrat();
      return m;
    }
    const std::vector<tupstruct>& gettupsvect() const {
      return RMUT(tups);
    }
#ifndef NDEBUG
    const std::vector<tupstruct>& gettupsvect(MUTDBGPAR) const {
      return CMUT(tups);
    }
#endif
    void replacetups(const durbase& b) {
      assert(RMUT(tups).empty());
      WMUT(tups) = RMUT(b.tups);
    }
    bool gettupletbegin(const int level) const {
      if (level < 0)
        throw badtuplvl();
      return (level >= (int) RMUT(tups).size()) ? false : RMUT(tups)[level].beg;
    }
    bool gettupletend(const int level) const {
      if (level < 0)
        throw badtuplvl();
      return (level >= (int) RMUT(tups).size()) ? false : RMUT(tups)[level].end;
    }
    fomus_rat gettuplet(const int level) const {
      if (level < 0)
        throw badtuplvl();
      if (level >= (int) RMUT(tups).size()) {
        fomus_rat r = {0, 1};
        return r;
      }
      return RMUT(tups)[level].tup;
    }
    bool isinnerbeg() const {
      return !RMUT(tups).empty() && RMUT(tups).back().beg;
    }
    bool isinnerend() const {
      return !RMUT(tups).empty() && RMUT(tups).back().end;
    }
    void setinnerinvis() {
      assert(!RMUT(tups).empty());
      WMUT(tups).back().invisible = true;
    }
    bool tupseq(const std::vector<tupstruct>& x) const {
      std::vector<tupstruct>::const_iterator j(x.begin());
      for (std::vector<tupstruct>::const_iterator i(RMUT(tups).begin());; ++i) {
        if (i == RMUT(tups).end())
          return j == RMUT(tups).end();
        if (j == RMUT(tups).end() || *i != *j)
          return false;
      }
    }
    bool ispoint() const {
      return RMUT(point) != point_none;
    }
    bool ispoint_nomut() const {
      return CMUT(point) != point_none;
    }
    pointtype getpoint_nomut() const {
      return RMUT(point);
    }
    offgroff getfullendtime() const {
      offgroff r = RMUT(off);
      if (r.groff.isnull())
        r.off = r.off + RMUT(dur);
      else
        r.groff = r.groff + RMUT(dur);
      return r;
    }
    fomus_rat getfulltupdur(const int lvl) const {
      if (lvl < 0 || lvl >= (int) RMUT(tups).size())
        throw badtuplvl();
      return RMUT(tups)[lvl].totdur;
    }
    void trem_halfdur(const numb& at) {
      WMUT(dur) = at;
      std::for_each(
          WMUT(tups).begin(), WMUT(tups).end(),
          boost::lambda::bind(&tupstruct::setnotend, boost::lambda::_1));
    }
    void trem_incoff(const numb& at) {
      WMUT(dur) = at;
      if (RMUT(off).isgrace())
        WMUT(off).groff = RMUT(off).groff + RMUT(dur);
      else
        WMUT(off).off = RMUT(off).off + RMUT(dur);
      std::for_each(
          WMUT(tups).begin(), WMUT(tups).end(),
          boost::lambda::bind(&tupstruct::setnotbeg, boost::lambda::_1));
    }
    void trem_dur0() {
      WMUT(dur) = (fint) 0;
    }
    bool is0dur() {
      return RMUT(dur) <= (fint) 0;
    }
#ifndef NDEBUG
    bool is0dur(MUTDBGPAR) {
      return CMUT(dur) <= (fint) 0;
    }
#endif
    void unsplittups(const durbase& rt);
  };

  class voicesbase _NONCOPYABLE {
public:
    MUTCHECK(std::vector<int>) voices;
    MUTCHECK(noteevbase*) mergeto;

    voicesbase(MUTPARAM_ const std::set<int>& voices0)
        : voices(voices0.begin(), voices0.end() _MUTP),
          mergeto((noteevbase*) 0 _MUTP) {
      if (CMUT(voices).empty())
        CMUT(voices).push_back(1);
    }
    voicesbase(MUTPARAM_ const int v)
        : voices(1, v _MUTP), mergeto((noteevbase*) 0 _MUTP) {}
    voicesbase(MUTPARAM_ voicesbase& x)
        : voices(x.voices _MUTP), mergeto((noteevbase*) 0 _MUTP) {}
    int get1voice() const {
      return RMUT(voices).size() == 1 ? RMUT(voices)[0] : 0;
    } // return 0 if there isn't 1 voice
    struct module_intslist getvoices() const {
      module_intslist l;
      l.n = RMUT(voices).size();
      l.ints = &RMUT(voices)[0];
      return l;
    }
    bool hasvoice(const int v) const {
      return std::binary_search(RMUT(voices).begin(), RMUT(voices).end(), v);
    }
    void assign(const int voice) {
      WMUT(voices).assign(1, voice);
    }
    void checkvoices(const char* wh) const {
      assert(RMUT(voices).size() > 0);
      if (RMUT(voices).size() > 1) {
        CERR << "unassigned voice";
        integerr(wh);
      }
      if (RMUT(voices)[0] < 1) {
        CERR << "invalid voice number";
        integerr(wh);
      }
    }
    const std::vector<int>& getvoicevect() const {
      return RMUT(voices);
    }
  };

  class stavesbase _NONCOPYABLE {
public:
    MUTCHECK(std::vector<int>) staves;
    MUTCHECK(int) clef;

    stavesbase(MUTPARAM_ const int s) : staves(1, s _MUTP), clef(-1 _MUTP) {}
    stavesbase(MUTPARAM) : MUTINITP_(staves) clef(-1 _MUTP) {}
    stavesbase(MUTPARAM_ stavesbase& x)
        : staves(x.staves _MUTP), clef(x.clef _MUTP) {}
    int get1staff() const {
      return RMUT(staves).size() == 1 ? RMUT(staves)[0] : 0;
    } // return 0 if there isn't 1 staff
    struct module_intslist getstaves() const {
      module_intslist l;
      l.n = RMUT(staves).size();
      l.ints = &RMUT(staves)[0];
      return l;
    }
    int getclefid() const {
      return RMUT(clef);
    }
    bool hasstaff(const int v) const {
      return std::binary_search(RMUT(staves).begin(), RMUT(staves).end(), v);
    }
    void assign(const int staff, const int clf) {
      WMUT(staves).assign(1, staff);
      WMUT(clef) = clf;
    }
    void assign(const int staff) {
      WMUT(staves).assign(1, staff);
    }
    // void assign(stavesbase& st) {WMUT(staves) = RMUT(st.staves); WMUT(clef) =
    // RMUT(st.clef);}
    void assign(const std::vector<int>& st, const int cl) {
      WMUT(staves) = st;
      WMUT(clef) = cl;
    }
    void checkstaves() const {
      assert(RMUT(staves).size() > 0);
      if (RMUT(staves).size() > 1) {
        CERR << "unassigned staff";
        integerr("staves/clefs");
      }
      checkstaves2();
    }
    void checkstaves2() const { // for rests!
      assert(RMUT(staves).size() > 0);
      if (RMUT(staves)[0] < 1) {
        CERR << "invalid staff number";
        integerr("staves/clefs");
      }
    }
    const std::vector<int>& getstavesvect() const {
      return RMUT(staves);
    }
  };

  struct markobj : public modobjbase {
    const markbase* def; // the mark definition
    std::string str;
    numb val;
    int sort; // set at very very end of processing
    enum module_markpos pos;
    markobj() {
      assert(false);
    }
    markobj(const markbase& x)
        : def(&x), val(module_none), sort(1), pos(x.getpos()) {}
    markobj(const int type, const char* str0, const module_value& val)
        : def(&markdefs[type]),
          str(boost::trim_copy(std::string(str0 ? str0 : ""))), val(val),
          sort(1), pos(markdefs[type].getpos()) {}
    markobj(const int type, const std::string& str0, const module_value& val)
        : def(&markdefs[type]), str(str0), val(val), sort(1),
          pos(markdefs[type].getpos()) {}
    const char* gettype() const {
      return "a mark";
    }
    int getmarkid() const {
      return def->getid();
    }
    s_type getspantype() const {
      return def->getspantype();
    }
    marks_which getwhich() const {
      return def->getwhich();
    }
    mark_movetype getmove() const {
      switch (sort) {
      case 0:
        return move_left;
      case 1:
        return def->getmove();
      case 2:
        return move_right;
      default:
        assert(false);
      }
    }
    const char* getmarkstr() const {
      return str.empty() ? 0 : str.c_str();
    }
    const numb& getmarkval() const {
      return val;
    }
    const char* getcid() const {
      return def->getname();
    }
    bool objless(const modobjbase& y) const {
      return y.objgreat(*this);
    }
    bool objless(const markobj& y) const {
      return def < y.def;
    }
    bool objgreat(const markobj& y) const {
      return y.def < def;
    }
    bool setrem(const char* arg1, const module_value& arg2) {
      return ((val.isnull() ? numb(arg2).isnull() : (val == arg2)) &&
              str == boost::trim_copy(std::string(arg1 ? arg1 : "")));
    }
    bool iscont() const {
      return def->iscont();
    }
    bool getdontspread() const {
      return def->getdontspread();
    }
    bool setval(const numb& val0) {
      if (def->hasaval()) {
        val = val0;
        return false;
      } else
        return true;
    } // return true on error
    bool setstr(const std::string& str0) {
      if (def->hasastr()) {
        str = boost::trim_copy(str0);
        return false;
      } else
        return true;
    }
    int getmarkorder() const {
      return sort;
    }
    int getgroup() const {
      return def->getgroup();
    }
    enum module_markpos getmarkpos() const {
      return pos;
    }
    void setmarkpos(const enum module_markpos p) {
      pos = p;
    }
    void checkpos() const {
      if (pos < 0 || pos > markpos_below) {
        CERR << "bad mark position";
        integerr("marks");
      }
    }
    void insureddr() {
      if (!boost::algorithm::ends_with(str, "--"))
        str += "--";
    }
    void insureddl() {
      if (!boost::algorithm::starts_with(str, "--"))
        str = "--" + str;
    }
    void checkargs(const fomusdata* fom, const filepos& pos) const {
      def->checkargs(fom, str, val, pos);
    }
  };
  inline bool operator<(const markobj& x, const markobj& y) {
    if (x.def != y.def) {
      assert(x.getmarkid() != y.getmarkid());
      return x.getmarkid() < y.getmarkid();
    }
    assert(x.getmarkid() == y.getmarkid());
    if (x.val.isntnull() && y.val.isntnull() && x.val != y.val)
      return x.val < y.val;
    if (x.val.isntnull() != y.val.isntnull())
      return y.val.isnull();
    return x.str < y.str;
  }
  struct dontspreadless
      : public std::binary_function<const markobj&, const markobj&, bool> {
    bool operator()(const markobj& x, const markobj& y) const {
      return x.getdontspread() || !y.getdontspread();
    }
  };
  inline bool operator<(const markobj& x, const bool y) {
    return (!y && x.getdontspread());
  }
  inline bool operator<(const bool x, const markobj& y) {
    return (x && y.getdontspread());
  }
  inline bool mvalneq(const numb& x, const numb& y) {
    return (x.isntnull() && y.isntnull()) ? x != y : (x.isnull() != y.isnull());
  }
  inline bool operator!=(const markobj& x, const markobj& y) {
    return (x.def != y.def || x.str != y.str || mvalneq(x.val, y.val));
  }
  // special function to test for uniqueness
  inline bool mvaleq_un(const numb& x, const numb& y) {
    return (x.isntnull() && y.isntnull()) ? x == y : (x.isnull() == y.isnull());
  }
  struct markobjeq_un
      : public std::binary_function<const markobj&, const markobj&, bool> {
    bool operator()(const markobj& x, const markobj& y) const {
      return ((x.def == y.def ||
               (x.getgroup() != 0 && x.getgroup() == y.getgroup() &&
                x.getwhich() == y.getwhich())) &&
              x.str == y.str && mvaleq_un(x.val, y.val));
    }
  };

  // sort by span type first
  struct marksetlt
      : public std::binary_function<const markobj&, const markobj&, bool> {
    bool operator()(const markobj& x, const markobj& y) const {
      return (x.getspantype() != y.getspantype())
                 ? x.getspantype() < y.getspantype()
                 : (x < y);
    }
  };
  struct markslr
      : public std::binary_function<const markobj&, const markobj&, bool> {
    bool operator()(const markobj& x, const markobj& y) const {
      return x.getmove() > y.getmove();
    }
  };

  struct binsearchmark {
    bool operator()(const module_markids x, const module_markobj y) const {
      return x < ((markobj*) (modobjbase*) (y))->getmarkid();
    }
    bool operator()(const module_markobj x, const module_markids y) const {
      return ((markobj*) (modobjbase*) (x))->getmarkid() < y;
    }
  };

  class marksbase _NONCOPYABLE {
protected:
    MUTCHECK(boost::ptr_vector<markobj>)
    marks; // shared_ptr not needed--marks will change w/ each tied note
    MUTCHECK2(boost::ptr_multiset<markobj, marksetlt>) nmarks;
    MUTCHECK(bool) newm;
    MUTCHECK(module_markslist)
    s, b, e; // single, begin, end marks, pointing into marks vect, ready to be
             // returned
public:
    marksbase(MUTPARAM)
        : MUTINITP_(marks) MUTINITP_(nmarks) newm(false _MUTP) _MUTINITP(s)
              _MUTINITP(b) _MUTINITP(e) {
#ifndef NDEBUG
      CMUT(s).marks = CMUT(b).marks = CMUT(e).marks = 0;
#endif
      CMUT(s).n = CMUT(b).n = CMUT(e).n = 0;
    }
    marksbase(MUTPARAM_ marksbase& x);
    marksbase(MUTPARAM_ const marksbase& x, int)
        : marks(RMUT(x.marks).begin(), RMUT(x.marks).end() _MUTP)
              _MUTINITP(nmarks),
          newm(false _MUTP) _MUTINITP(s) _MUTINITP(b) _MUTINITP(e) {
      assert(RMUT(x.marks).size() == CMUT(marks).size());
    }
    marksbase(MUTPARAM_ const boost::ptr_set<markobj>& x)
        : marks(x.begin(), x.end() _MUTP) _MUTINITP(nmarks),
          newm(false _MUTP) _MUTINITP(s) _MUTINITP(b) _MUTINITP(e) {
      CMUT(s).n = CMUT(b).n = CMUT(e).n = 0;
      assert(CMUT(marks).size() == x.size());
    }
    marksbase(MUTPARAM_ const boost::ptr_vector<markobj>& x)
        : marks(x.begin(), x.end() _MUTP) _MUTINITP(nmarks),
          newm(false _MUTP) _MUTINITP(s) _MUTINITP(b) _MUTINITP(e) {
      CMUT(s).n = CMUT(b).n = CMUT(e).n = 0;
      assert(CMUT(marks).size() == x.size());
    }
    marksbase(MUTPARAM_ const special_markslist& ml)
        : MUTINITP_(marks) MUTINITP_(nmarks) newm(false _MUTP) _MUTINITP(s)
              _MUTINITP(b) _MUTINITP(e) {
      for (const special_markspec *i(ml.marks), *ie(ml.marks + ml.n); i < ie;
           ++i)
        CMUT(marks).push_back(new markobj(i->id, i->str, i->val));
      CMUT(marks).sort(marksetlt());
      DISABLEMUTCHECK;
      cachessort();
    }
    void cachessort();
    boost::ptr_vector<markobj>& getmarkslst() {
      return RMUT(marks);
    }
    const boost::ptr_vector<markobj>& getmarkslst() const {
      return RMUT(marks);
    }
    void movetoold() {
      if (!RMUT(newm)) {
        WMUT(newm) = true;
        WMUT(nmarks).insert(RMUT(marks).begin(), RMUT(marks).end());
        assert(RMUT(marks).size() ==
               RMUT(nmarks).size()); // keeps markslist valid
      }
    }
    void spreadgetmarks(boost::ptr_set<markobj, marksetlt>& mrks);
    void assignmarkrem(const int type, const char* arg1,
                       const module_value& arg2);
    void recachemarksaux();
    void recachemarks() {
      if (RMUT(newm)) {
        recachemarksaux();
        WMUT(marks).sort(marksetlt());
        cachessort();
      }
    }
    void
    recachemarks0() { // called by dopostmarkevs, caching is done afterwords
      if (RMUT(newm))
        recachemarksaux();
    }
    void unsplitmarks(marksbase& rt);
    void spreadreplmarks(const boost::ptr_set<markobj, marksetlt>& mrks);
    void assignmarkins(const int type, const char* arg1,
                       const module_value& arg2) {
      if (type < 0 || type >= (int) markdefs.size())
        throw assmarkerr();
      movetoold();
      WMUT(nmarks).insert(new markobj(type, arg1, arg2));
    }
    void assignmarkins(const int type, const std::string& arg1,
                       const module_value& arg2) {
      if (type < 0 || type >= (int) markdefs.size())
        throw assmarkerr();
      movetoold();
      WMUT(nmarks).insert(new markobj(type, arg1, arg2));
    }
    bool noteq(const marksbase& m) const {
      for (boost::ptr_vector<markobj>::const_iterator i(RMUT(marks).begin()),
           j(RMUT(m.marks).begin());
           ; ++i, ++j) {
        if (i == RMUT(marks).end())
          return (j != RMUT(m.marks).end());
        if (j == RMUT(m.marks).end() || *i != *j)
          return true;
      }
      return false;
    }
    void remdupmarks() {
      boost::ptr_vector<markobj>::iterator e(
          unique(RMUT(marks).begin(), RMUT(marks).end(),
                 markobjeq_un())); // should be sorted
      WMUT(marks).erase(e, RMUT(marks).end());
      cachessort();
    }
    void remgraceslash() {
      movetoold(); // actually copies to new
      boost::ptr_set<markobj>::iterator i(
          RMUT(nmarks).find(markobj(mark_graceslash, 0, numb(module_none))));
      if (i != RMUT(nmarks).end())
        WMUT(nmarks).erase(i);
      recachemarks();
    }
    void xfermarks(marksbase& fr) { // FR ISN'T IN AN EVENTS LIST
      movetoold();
      WMUT(nmarks).transfer(CMUT(fr.marks).begin(), CMUT(fr.marks).end(),
                            CMUT(fr.marks));
      recachemarks();
    }
    void remtremmarks();
    void switchtrems(const module_markids from, const module_markids to);
  };

  class dynbase _NONCOPYABLE {
protected:
    MUTCHECK(numb) dyn;

public:
    dynbase(MUTPARAM_ const numb& dyn) : dyn(dyn _MUTP) {}
    dynbase(MUTPARAM_ const dynbase& x) : dyn(x.dyn _MUTP) {}
  };

  struct divsplitstr { // division instruction for divide.cc
    std::vector<tupstruct> tups;
    divsplitstr(const divide_split& s) {
      for (divide_tuplet *t = s.tups, *te = s.tups + s.n; t < te; ++t) {
        tups.push_back(tupstruct(*t));
      }
    }
  };

  struct clippair {
    fomus_rat o1, o2;
    clippair(const fomus_rat& o1, const fomus_rat& o2) : o1(o1), o2(o2) {}
  };
  inline bool operator<(const clippair& x, const clippair& y) {
    return x.o1 < y.o1;
  }
  struct syncs;
  class modbase;
  class stage;
  class noteevbase : public event,
                     public durbase,
                     public voicesbase,
                     public stavesbase,
                     public marksbase {
#ifndef NDEBUG
    const stage* stageptr0;
#endif
    fint stagechk;
    MUTCHECK(int) octsign; // bool octbegin;
public:
    bool assdel;

private:
    std::auto_ptr<std::set<clippair>> clips;
    boost::ptr_map<const rat, boost::nullable<divsplitstr>>
        splits;   // assigned splits
    fint sortind; // set by special sched.cc function
protected:
    MUTCHECK(bool) invisible;
    bool nend;

public:
    noteevbase(const numb& off, const numb& groff, const numb& dur,
               const std::set<int>& voices, const filepos& pos,
               const boost::ptr_set<markobj>& mm, const pointtype point,
               const setmap& sets0)
        : // called from noteev & restev
          event(pos, sets0), durbase(WITHMUT_ off, groff, dur, point),
          voicesbase(WITHMUT_ voices), stavesbase(WITHMUT),
          marksbase(WITHMUT_ mm), octsign(0 _MUT), assdel(false),
          invisible(false _MUT), nend(false) {
      assert(dur >= (fint) 0);
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    }
    noteevbase(const numb& off, const numb& groff, const numb& dur,
               const int voices, const int staves, const filepos& pos,
               const boost::ptr_set<markobj>& mm)
        : event(pos), durbase(WITHMUT_ off, groff, dur),
          voicesbase(WITHMUT_ voices), stavesbase(WITHMUT_ staves),
          marksbase(WITHMUT_ mm), octsign(0 _MUT), assdel(false),
          invisible(false _MUT), nend(false) {
      assert(dur > (fint) 0);
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    }
    noteevbase(const numb& off, const numb& groff, const numb& dur,
               const int voices, const int staves, const filepos& pos,
               const bool inv)
        : // NO MARKS // INV
          event(pos), durbase(WITHMUT_ off, groff, dur),
          voicesbase(WITHMUT_ voices), stavesbase(WITHMUT_ staves),
          marksbase(WITHMUT), octsign(0 _MUT), assdel(false),
          invisible(inv _MUT), nend(false) {
      assert(off == (fint) 0 || dur > (fint) 0);
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    }
    noteevbase(const numb& off, const numb& groff, const numb& dur,
               const int voices, const int staves, const filepos& pos,
               const boost::ptr_vector<markobj>& mm)
        : // INV
          event(pos), durbase(WITHMUT_ off, groff, dur),
          voicesbase(WITHMUT_ voices), stavesbase(WITHMUT_ staves),
          marksbase(WITHMUT_ mm), octsign(0 _MUT), assdel(false),
          invisible(true _MUT), nend(false) {
      assert(dur > (fint) 0);
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    }
    noteevbase(const numb& off, const numb& groff, const numb& dur,
               const int voices, const filepos& pos)
        : // called from `assigndetmark', inv = true, marks in right obj are
          // empty
          event(pos), durbase(WITHMUT_ off, groff, dur, point_auto),
          voicesbase(WITHMUT_ voices), stavesbase(WITHMUT), marksbase(WITHMUT),
          octsign(0 _MUT), assdel(false), invisible(true _MUT), nend(false) {
      assert(isvalid());
      // assert(dur = (fint)0);
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    }

    noteevbase(noteevbase& lft, const numb& off)
        : event(lft), durbase(WITHMUT_ lft, off), voicesbase(WITHMUT_ lft),
          stavesbase(WITHMUT_ lft), marksbase(WITHMUT_ lft),
          octsign(lft.octsign _MUT), //, octbegin(false),
          assdel(false), invisible(lft.invisible _MUT), nend(false) {
      assert(!isgrace(MUTDBG)); // grace notes should never be split!
      assert(CMUT(dur) > (fint) 0);
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    }
    // for splitting
    noteevbase(noteevbase& lft, const numb& off, int)
        : event(lft), durbase(WITHMUT_ lft, off, 0), voicesbase(WITHMUT_ lft),
          stavesbase(WITHMUT_ lft), marksbase(WITHMUT_ lft),
          octsign(lft.octsign _MUT), assdel(false),
          invisible(lft.invisible _MUT), nend(false) {
      assert(!isgrace(MUTDBG)); // grace notes should never be split!
      assert(CMUT(dur) > (fint) 0);
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    }
    noteevbase(noteevbase& lft, const int v)
        : event(lft), durbase(WITHMUT_ lft), voicesbase(WITHMUT_ v),
          stavesbase(WITHMUT_ lft), marksbase(WITHMUT_ lft),
          octsign(lft.octsign _MUT), assdel(false), invisible(false _MUT),
          nend(false) {
      assert(!isgrace(MUTDBG)); // grace notes should never be split!
      assert(CMUT(dur) > (fint) 0);
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    }
    noteevbase(noteevbase& x, const int v, int)
        : event(x), durbase(WITHMUT_ x), voicesbase(WITHMUT_ v),
          stavesbase(WITHMUT_ x), marksbase(WITHMUT_ x, 0), octsign(0 _MUT),
          assdel(false), invisible(false _MUT),
          nend(false) { // for metapart dist.
      assert(CMUT(dur) > (fint) 0);
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    }
    noteevbase(noteevbase& x)
        : event(x, 0), durbase(WITHMUT_ x), voicesbase(WITHMUT_ x),
          stavesbase(WITHMUT_ x), marksbase(WITHMUT_ x, 0), octsign(0 _MUT),
          assdel(false), invisible(false _MUT), nend(false) {
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    } // fomcloning
    noteevbase(noteevbase& x, const int, const numb& shift)
        : event(x, 0), durbase(WITHMUT_ x, 0, shift), voicesbase(WITHMUT_ x),
          stavesbase(WITHMUT_ x), marksbase(WITHMUT_ x, 0), octsign(0 _MUT),
          assdel(false), invisible(false _MUT), nend(false) {
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    } // fomcloning
    noteevbase(noteevbase& x, const special_markslist& marks)
        : event(x, 0), durbase(WITHMUT_ x), voicesbase(WITHMUT_ x),
          stavesbase(WITHMUT_ x), marksbase(WITHMUT_ marks), octsign(0 _MUT),
          assdel(false), invisible(false _MUT), nend(false) {
#ifndef NDEBUG
      stageptr0 = 0;
#endif
    }
#ifndef NDEBUG
    bool isrlocked() const {
      return lockcheck.get() ? !((boost::shared_mutex&) mut).try_lock() : true;
    }
    bool iswlocked() const {
      return lockcheck.get() ? !((boost::shared_mutex&) mut).try_lock_shared()
                             : true;
    }
    bool isntlocked() const;
#endif
    virtual noteevbase* fomclone(const numb& shift) {
      assert(false);
    };
    std::auto_ptr<std::set<clippair>>& getclips() {
      return clips;
    }
    virtual bool isbeambeg() const {
      return false;
    }
    virtual bool isbeamend() const {
      return false;
    }
    virtual bool isbeammid() const {
      return false;
    }
    virtual void domerge();
    void setasendofmeas() {
      assert(!nend);
      nend = true;
    }
    bool isendofmeas() {
      if (nend) {
        nend = false;
        return true;
      } else
        return false;
    }
    virtual bool getisnote() const {
      return false;
    }
    bool getisperc() const {
      return false;
    }
    bool getisperc_nomut() const {
      return false;
    }
    void setcheck(const fint c) {
      stagechk = c;
    }
    pointtype getpoint() const {
      READLOCK;
      return RMUT(point);
    }
#ifndef NDEBUG
    void setstageptr(const stage* n) {
      stageptr0 = n;
    }
    const stage* getstageptr() const {
      return stageptr0;
    }
#endif
    void post_apisetvalue();
    virtual bool isfirsttied() const {
      assert(false);
    }
    virtual bool ismidtied() const {
      assert(false);
    }
    virtual bool islasttied() const {
      assert(false);
    }
    const numb& gettime() const {
      READLOCK;
      return RMUT(off).off;
    }
    const numb& gettime_nomut() const {
      return RMUT(off).off;
    }
#ifndef NDEBUG
    const numb& gettime_nomut(MUTDBGPAR) const {
      return CMUT(off).off;
    }
#endif
    const numb& getgracetime() const {
      READLOCK;
      return RMUT(off).groff;
    }
    const numb& getgracetime_nomut() const {
      return RMUT(off).groff;
    }
#ifndef NDEBUG
    const numb& getgracetime_nomut(MUTDBGPAR) const {
      return CMUT(off).groff;
    }
#endif
    bool getisbeginchord() const {
      return true;
    }
    bool getisendchord() const {
      return true;
    }
    bool getischordlow() const {
      return true;
    }
    bool getischordhigh() const {
      return true;
    }
    numb getdur() const {
      READLOCK;
      return isgrace() ? numb((fint) 0) : RMUT(dur);
    }
    numb getdur_nomut() const {
      return isgrace() ? numb((fint) 0) : RMUT(dur);
    }
    numb getendtime() const {
      READLOCK;
      return isgrace() ? RMUT(off).off : RMUT(off).off + RMUT(dur);
    }
    numb getendtime_nomut() const {
      return isgrace() ? RMUT(off).off : RMUT(off).off + RMUT(dur);
    }
#ifndef NDEBUG
    numb getendtime_nomut(MUTDBGPAR) const {
      return isgrace(MUTDBG) ? CMUT(off).off : CMUT(off).off + CMUT(dur);
    }
#endif
    numb getgraceendtime() const {
      READLOCK;
      if (!isgrace()) {
        module_value x;
        x.type = module_none;
        return x;
      }
      return RMUT(off).groff + RMUT(dur);
    }
    module_value getgraceendtimenochk() const {
      return RMUT(off).groff + RMUT(dur);
    }
#ifndef NDEBUG
    module_value getgraceendtimenochk(MUTDBGPAR) const {
      return CMUT(off).groff + CMUT(dur);
    }
#endif
    numb getgracedur() const {
      READLOCK;
      if (!isgrace()) {
        module_value x;
        x.type = module_none;
        return x;
      }
      return RMUT(dur);
    }
    fint getsortind() const {
      return sortind;
    }
    boost::ptr_map<const rat, boost::nullable<divsplitstr>>& getsplits() {
      return splits;
    }
    int getvoice() const {
      assert(isvalid());
      assert(isntlocked());
      READLOCK;
      assert(isrlocked());
      return get1voice();
    }
    int getvoice_nomut() const {
      return get1voice();
    }
    int getstaff() const {
      READLOCK;
      return get1staff();
    }
    int getstaff_nomut() const {
      return get1staff();
    }
    struct module_intslist getvoices() {
      READLOCK;
      return voicesbase::getvoices();
    }
    struct module_intslist getstaves() {
      READLOCK;
      return stavesbase::getstaves();
    }
    bool getisgrace() const {
      READLOCK;
      return isgrace();
    }
    bool getisgrace_nomut() const {
      return isgrace();
    }
#ifndef NDEBUG
    bool getisgrace_nomut(MUTDBGPAR) const {
      return isgrace(MUTDBG);
    }
#endif
    bool gethasvoice(const int voice) const {
      READLOCK;
      return voicesbase::hasvoice(voice);
    }
    bool gethasvoice_nomut(const int voice) const {
      return voicesbase::hasvoice(voice);
    }
    bool gethasstaff(const int staff) const {
      READLOCK;
      return stavesbase::hasstaff(staff);
    }
    bool gethasstaff_nomut(const int staff) const {
      return stavesbase::hasstaff(staff);
    }
    int getoctsign() const {
      READLOCK;
      return RMUT(octsign);
    }
    int getoctsign_nomut() const {
      return RMUT(octsign);
    }
    // noteevbase* getprevnoteevbase() const;
    noteevbase* getnextnoteevbase() const;
    bool getoctavebegin();
    bool getoctaveend();
    // int octsignwithmut() {READLOCK; return RMUT(octsign);}
    fomus_rat getfullwrittenacc() const;
    fomus_rat getwrittenacc1() const;
    fomus_rat getwrittenacc2() const;

    const std::vector<int>& voicesref() const {
      return RMUT(voices);
    }
    const std::vector<int>& stavesref() const {
      return RMUT(staves);
    }

    boost::shared_ptr<std::vector<int>> clefscache;
    void getclefsinit();
    struct module_intslist getclefs();
    struct module_intslist getclefs(const int st);
    bool gethasclef(const int clef);
    bool gethasclef_nomut(const int clef);
    bool gethasclef(const int clef, const int staff);
    bool gethasclef_nomut(const int clef, const int staff);
    bool gettupletbegin(const int level) const {
      READLOCK;
      return durbase::gettupletbegin(level);
    }
    bool gettupletbegin_nomut(const int level) const {
      return durbase::gettupletbegin(level);
    }
    bool gettupletend(const int level) const {
      READLOCK;
      return durbase::gettupletend(level);
    }
    bool gettupletend_nomut(const int level) const {
      return durbase::gettupletend(level);
    }
    fomus_rat gettuplet(const int level) const {
      READLOCK;
      return durbase::gettuplet(level);
    }
    fomus_rat gettuplet_nomut(const int level) const {
      return durbase::gettuplet(level);
    }
    module_markslist getmarks() {
      READLOCK;
      module_markslist l = {
          RMUT(marksbase::marks).size(),
          (const module_markobj*) RMUT(marksbase::marks).c_array()};
      return l;
    }
    module_markslist getmarks_nomut() {
      module_markslist l = {
          RMUT(marksbase::marks).size(),
          (const module_markobj*) RMUT(marksbase::marks).c_array()};
      return l;
    }
    module_markslist getsinglemarks() {
      assert(isvalid());
      READLOCK;
      return RMUT(marksbase::s);
    }
    module_markslist getsinglemarks_nomut() {
      assert(isvalid());
      return RMUT(marksbase::s);
    }
    module_markslist getspannerbegins() {
      assert(isvalid());
      READLOCK;
      return RMUT(marksbase::b);
    }
    module_markslist getspannerbegins_nomut() {
      assert(isvalid());
      return RMUT(marksbase::b);
    }
    module_markslist getspannerends() {
      assert(isvalid());
      READLOCK;
      return RMUT(marksbase::e);
    }
    module_markslist getspannerends_nomut() {
      assert(isvalid());
      return RMUT(marksbase::e);
    }
    numb getwrittendur(const int level) const {
      READLOCK;
      return getdur_nomut() * tofrat(durbase::gettupmult(level));
    }
    numb getwrittengracedur(const int level) const {
      READLOCK;
      if (!isgrace()) {
        module_value x;
        x.type = module_none;
        return x;
      }
      return RMUT(dur) * tofrat(durbase::gettupmult(level));
    }
    fomus_rat getbeatstowrittendur(const fomus_rat& dur0,
                                   const int level) const {
      READLOCK;
      return tofrat(rat(dur0.num, dur0.den) * durbase::gettupmult(level));
    }
    void assigntquant(struct fomus_rat& ti,
                      struct fomus_rat& du) { // dur = 0  -->  gracenote
      {
        WRITELOCK;
        WMUT(off).off = ti;
        WMUT(dur) = du;
      }
      post_apisetvalue();
    }
    void assigntquant(struct fomus_rat& ti, struct module_value& grti,
                      struct module_value& du) { // dur = 0  -->  gracenote
      {
        WRITELOCK;
        WMUT(off).off = ti;
        WMUT(off).groff = grti;
        WMUT(dur) = du;
      }
      post_apisetvalue();
    }
    void assigntquantdel() {
      assdel = true;
      post_apisetvalue();
    }
    void fixtimequant(numb& mx);
    bool isassdel() const {
      return assdel;
    }
    virtual noteevbase* getsplitat(const numb& off) {
      assert(false);
    }
    virtual noteevbase* getsplitat0(const numb& off) {
      assert(false);
    }
    virtual noteevbase* getsplitatt(const numb& off) {
      assert(false);
    }
    void skipassign() {
      post_apisetvalue();
    }
    void recacheassign() {
      {
        WRITELOCK;
        recachemarks();
      }
      post_apisetvalue();
    }
    virtual void checkvoices() {
      post_apisetvalue();
    }
    virtual void checkrestvoices(boost::ptr_list<noteevbase>& tmp) {}
    virtual void checkstaves() {}
    virtual bool checkstaves0() {
      post_apisetvalue();
      return true;
    }
    void checkprune();
    void checkoct();
    void checkoct(const int sign) {
      WMUT(octsign) = sign;
    }
    void setinnerinvis() {
      WRITELOCK;
      durbase::setinnerinvis();
    }
    void assignprune(const fomus_rat& time1, const fomus_rat& time2) {
      if (!clips.get()) {
        clips.reset(new std::set<clippair>);
      }
      clips->insert(clippair(time1, time2));
    }
    void assignprunedone() {
      if (!clips.get()) {
        clips.reset(new std::set<clippair>);
      }
      post_apisetvalue();
    }
    void assignocts(const int octs) {
      {
        WRITELOCK;
        WMUT(octsign) = octs;
      }
      post_apisetvalue();
    }
    void collectallvoices(std::set<int>& v) {
      v.insert(RMUT(voices).begin(), RMUT(voices).end());
    }
    void collectallstaves(std::set<int>& s) {
      s.insert(RMUT(staves).begin(), RMUT(staves).end());
    }
    bool objless(const modobjbase& y) const {
      return y.objgreat(*this);
    }
    bool objless(const noteevbase& y) const {
      return *this < y;
    }
    bool objgreat(const noteevbase& y) const {
      return y < *this;
    }
    void assignmarkrem(const int type, const char* arg1,
                       const module_value& arg2) {
      WRITELOCK;
      marksbase::assignmarkrem(type, arg1, arg2);
    }
    void assignmarkins(const int type, const char* arg1,
                       const module_value& arg2) {
      WRITELOCK;
      marksbase::assignmarkins(type, arg1, arg2);
    }
    void assignmarkins(const int type, const std::string& arg1,
                       const module_value& arg2) {
      WRITELOCK;
      marksbase::assignmarkins(type, arg1, arg2);
    }
    void recachemarks() { // called from intern.cc, so needs to lock
      marksbase::recachemarks();
    }
    noteevbase& getnoteevbase() {
      return *this;
    }
    bool getistiedleft() const {
      return false;
    }
    virtual bool getistiedleft_nomut() const {
      return false;
    }
    bool getistiedright() const {
      return false;
    }
    virtual bool getistiedright_nomut() const {
      return false;
    }
    int getclef() const {
      READLOCK;
      return RMUT(clef);
    }
    int getclef_nomut() const {
      return RMUT(clef);
    }
    void sortord(fomus_int& i) {
      sortind = ++i;
    }
    int getvoiceinstaff() const;
    virtual void assignbeams2(const int bl, const int br, UPLOCKPAR) {
      assert(false);
    }
    virtual void checkbeam() {}
#ifndef NDEBUG
    virtual void dumpall() const {
      assert(false);
    }
#endif
    void checkmerge(const noteevbase& x) const {
      if (RMUT(off).off != RMUT(x.off) || RMUT(dur) != RMUT(x.dur) ||
          !tupseq(RMUT(x.tups)) || noteq(x)) {
        CERR << "bad merge";
        integerr("merge"); // found during merge integrity check" << std::endl;
      }
    }
    virtual void estabpercinst() {
      post_apisetvalue();
    }
    void dopointl(const offgroff& o1, std::list<noteevbase*>& tmp, UPLOCKPAR);
    void dopointr(const offgroff& o2, std::list<noteevbase*>& tmp, UPLOCKPAR);
    noteevbase* releaseme();
    // virtual bool maybereplaceclef(const int cl) {assert(false);}
    fomus_rat getfulltupdur(const int lvl) const {
      READLOCK;
      return durbase::getfulltupdur(lvl);
    }
    fomus_rat getfulltupdur_nomut(const int lvl) const {
      return durbase::getfulltupdur(lvl);
    }
    void assigninv() {
      {
        WRITELOCK;
        WMUT(invisible) = true;
      }
      post_apisetvalue();
    }
    bool isinvisible() const {
      READLOCK;
      return RMUT(invisible);
    }
    bool isinvisible_nomut() const {
      return RMUT(invisible);
    }
    bool getismarkrest() const {
      return false;
    }
    virtual bool getismarkrest_nomut() const {
      return false;
    }
    void remdupmarks() {
      {
        WRITELOCK;
        marksbase::remdupmarks();
      }
      post_apisetvalue();
    }
    // void trem_incoff() {
    //   {
    // 	WRITELOCK;
    // 	durbase::trem_incoff();
    //   }
    //   assert(!assdel);
    //   assdel = true;
    // }
    void resetassdel() {
      assdel = false;
    }
    // bool resetnassdelinv() {assdel = false; return getvoice_nomut() < 1000;}
    void assignfixedstaff(const int st) {
      stavesbase::assign(st);
    }
    bool getoctaux(bool& bl, const int st) const {
      READLOCK;
      if (get1staff() == st) {
        bl = !getoctsign_nomut();
        return true;
      }
      return false;
    }
    bool beginchordaux(bool& bl, const offgroff& ti, const int v) const {
      READLOCK;
      if (isinvisible_nomut())
        return false;
      if (getfulltime_nomut() < ti) {
        bl = true;
        return true;
      }
      if (get1voice() == v) {
        bl = false;
        return true;
      }
      return false;
    }
    bool endchordaux(bool& bl, const offgroff& ti, const int v) const {
      READLOCK;
      if (isinvisible_nomut())
        return false;
      if (getfulltime_nomut() > ti) {
        bl = true;
        return true;
      }
      if (get1voice() == v) {
        bl = false;
        return true;
      }
      return false;
    }
    virtual bool lowchordaux1(bool& bl, const offgroff& ti, const int v,
                              const rat& n) const {
      return false;
    }
    virtual bool lowchordaux2(bool& bl, const offgroff& ti, const int v,
                              const rat& n) const {
      return false;
    }
    virtual bool highchordaux1(bool& bl, const offgroff& ti, const int v,
                               const rat& n) const {
      return false;
    }
    virtual bool highchordaux2(bool& bl, const offgroff& ti, const int v,
                               const rat& n) const {
      return false;
    }
    virtual bool voiceinstaffaux1(bool& bl, const offgroff& ti, const int vo,
                                  const int st, std::set<int>& bf,
                                  bool& vgr) const {
      READLOCK;
      if (getfullendtime() < ti) {
        bl = true;
        return true;
      }
      if (isinvisible_nomut()) {
        bl = false;
        return true;
      }
      if (st == get1staff()) {
        int v = get1voice();
        if (v < vo)
          bf.insert(v);
        else if (v > vo)
          vgr = true;
      }
      return false;
    }
    bool voiceinstaffaux2(bool& bl, const offgroff& ti, const int vo,
                          const int st, std::set<int>& bf, bool& vgr) const {
      READLOCK;
      if (getfulltime_nomut() > ti) {
        bl = true;
        return true;
      }
      if (isinvisible_nomut()) {
        bl = false;
        return true;
      }
      if (st == get1staff()) {
        int v = get1voice();
        if (v < vo)
          bf.insert(v);
        else if (v > vo)
          vgr = true;
      }
      return false;
    }
    bool connbeamsleftaux(int& nb, const int vo) const {
      READLOCK;
      if (get1voice() == vo) {
        nb = getbeamsright_nomut();
        return true;
      }
      return false;
    }
    bool connbeamsrightaux(int& nb, const int vo) const {
      READLOCK;
      if (get1voice() == vo) {
        nb = getbeamsleft_nomut();
        return true;
      }
      return false;
    }
    virtual int getbeamsleft_nomut() const {
      assert(false);
    } // these belong here
    virtual int getbeamsright_nomut() const {
      assert(false);
    }
    offgroff& getfulltime() {
      READLOCK;
      return RMUT(off);
    }
    const offgroff& getfulltime() const {
      READLOCK;
      return RMUT(off);
    }
    offgroff& getfulltime_nomut() {
      return RMUT(off);
    }
#ifndef NDEBUG
    offgroff& getfulltime_nomut(MUTDBGPAR) {
      return CMUT(off);
    }
#endif
    const offgroff& getfulltime_nomut() const {
      return RMUT(off);
    }
#ifndef NDEBUG
    const offgroff& getfulltime_nomut(MUTDBGPAR) const {
      return CMUT(off);
    }
#endif
    void spreadgetmarks(boost::ptr_set<markobj, marksetlt>& mrks) {
      WRITELOCK;
      marksbase::spreadgetmarks(mrks);
    }
    void spreadreplmarks(const boost::ptr_set<markobj, marksetlt>& mrks) {
      WRITELOCK;
      marksbase::spreadreplmarks(mrks);
    }
    bool matchesvoiceetime(const int v, const offgroff& t, int& st,
                           int& cl) const {
      READLOCK;
      if (get1voice() == v && getfullendtime() > t) {
        st = get1staff();
        assert(st);
        cl = getclef_nomut();
        return true;
      }
      return false;
    }
    bool lastgrendoffaux(const offgroff& off, numb& ret, const int v) const;
    bool firstgrendoffaux(const offgroff& off, numb& ret, const int v) const;
    virtual void checkstaves2() {
      post_apisetvalue();
    }
    virtual bool isfill() const {
      return false;
    }
    void preprocess() { // after .fms file is written but before any processing
                        // happens
      if (get_ival_nomut(FILL_ID)) {
        WMUT(dur) = (fint) 0;
        if (getisgrace_nomut()) {
          WMUT(point) = point_grright;
        } else {
          WMUT(point) = point_right;
          WMUT(off).groff = std::numeric_limits<fint>::max() / 2;
        }
      }
    }
    // virtual bool unsplit0(noteevbase& lt) {CERR << "bad unsplit";
    // integerr("note division"); return false;}
    virtual bool unsplit(
        noteevbase& rt) { /*CERR << "bad unsplit"; integerr("note division");*/
      return false;
    }
    virtual bool unsplitaux(
        noteev& rt) { /*CERR << "bad unsplit"; integerr("note division");*/
      return false;
    }
  };
  inline bool operator<(const noteevbase& x, const noteevbase& y) {
    return x.getsortind() < y.getsortind();
  }

  class noteev;
  class tiedbase _NONCOPYABLE {
public:
    MUTCHECK(noteev*) tiedl;
    MUTCHECK(noteev*) tiedr;
    bool willbetiedl, willbetiedr;

public:
    tiedbase(MUTPARAM_ noteev& lft);
    tiedbase(MUTPARAM_ int, noteev& lft);
    tiedbase(MUTPARAM_ const noteev& x, int);
    ~tiedbase();
    tiedbase(MUTPARAM)
        : tiedl((noteev*) 0 _MUTP), tiedr((noteev*) 0 _MUTP),
          willbetiedl(false), willbetiedr(false) {}
#ifndef NDEBUG
    void isvalidptrs() const;
#endif
    bool isfirsttied() const {
#ifndef NDEBUG
      isvalidptrs();
#endif
      return !RMUT(tiedl);
    }
    bool ismidtied() const {
#ifndef NDEBUG
      isvalidptrs();
#endif
      return RMUT(tiedl) && RMUT(tiedr);
    }
    bool islasttied() const {
#ifndef NDEBUG
      isvalidptrs();
#endif
      return RMUT(tiedl) && !RMUT(tiedr);
    }
    void killtiedr() {
      WMUT(tiedr) = 0;
    }
    void killtiedl() {
      WMUT(tiedl) = 0;
    }
    void settiedl(noteev* x) {
      WMUT(tiedl) = x;
    }
    void settiedr(noteev* x) {
      WMUT(tiedr) = x;
    }
  };

  class noteev : public noteevbase,
                 public dynbase,
                 public tiedbase { // exists in a measure
public:
    MUTCHECK(numb) note;
    MUTCHECK(rat) acc1, acc2;
    MUTCHECK(int) beaml, beamr; // number of beams to left and to right
    MUTCHECK(bool) cautacc;
    MUTCHECK(const char*) percname;
    MUTCHECK(percinstr_str*) perc;
    noteev(const numb& off, const numb& groff, const numb& dur,
           const numb& note, const numb& dyn, const std::set<int>& voices,
           const filepos& pos, const boost::ptr_set<markobj>& mm,
           const char* percname, const pointtype point, const setmap& sets0)
        : noteevbase(off, groff, dur, voices, pos, mm, point, sets0),
          dynbase(WITHMUT_ dyn), tiedbase(WITHMUT), note(note _MUT),
          acc1(std::numeric_limits<fint>::max(), 1 _MUT),
          acc2(std::numeric_limits<fint>::max(), 1 _MUT), beaml(0 _MUT),
          beamr(0 _MUT), cautacc(false _MUT), percname(percname _MUT),
          perc((percinstr_str*) 0 _MUT) {
#ifndef NDEBUGOUT
      DBG("***** off=" << off << "  groff=" << groff << "  dur=" << dur
                       << "  note=");
      if (percname)
        DBG(percname);
      else
        DBG(note);
      DBG("  dyn=" << dyn << "  voices=(");
      for (std::set<int>::const_iterator i(voices.begin()); i != voices.end();
           ++i)
        DBG(*i << " ");
      DBG(")  ");
      for (boost::ptr_vector<markobj>::const_iterator i(CMUT(marks).begin());
           i != CMUT(marks).end(); ++i) {
        DBG('[' << i->def->getname() << " ...]");
      }
      if (CMUT(beaml) > 0)
        DBG(" bl:" << CMUT(beaml));
      if (CMUT(beamr) > 0)
        DBG(" br:" << CMUT(beamr));
      if (!sets.empty())
        DBG(sets.size() << " settings");
      DBG(std::endl);
#endif
    }
    noteev(noteev& x, const numb& off)
        : noteevbase(x, off), dynbase(WITHMUT_ x), tiedbase(WITHMUT_ x),
          note(x.note _MUT), acc1(x.acc1 _MUT), acc2(x.acc2 _MUT),
          beaml(0 _MUT), beamr(0 _MUT), cautacc(false _MUT),
          percname(x.percname _MUT), perc(x.perc _MUT) {}
    noteev(int, noteev& x, const numb& off)
        : noteevbase(x, off), dynbase(WITHMUT_ x), tiedbase(WITHMUT_ 0, x),
          note(x.note _MUT), acc1(x.acc1 _MUT), acc2(x.acc2 _MUT),
          beaml(0 _MUT), beamr(0 _MUT), cautacc(false _MUT),
          percname(x.percname _MUT), perc(x.perc _MUT) {}
    noteev(noteev& x, const numb& off, int)
        : noteevbase(x, off, 0), dynbase(WITHMUT_ x), tiedbase(WITHMUT_ x),
          note(x.note _MUT), acc1(x.acc1 _MUT), acc2(x.acc2 _MUT),
          beaml(0 _MUT), beamr(0 _MUT), cautacc(false _MUT),
          percname(x.percname _MUT), perc(x.perc _MUT) {}
    // noteev(int, noteev& x, const numb& off):noteevbase(x, off, 0),
    // dynbase(WITHMUT_ x), tiedbase(WITHMUT_ 0, x), note(x.note _MUT),
    // acc1(x.acc1 _MUT), acc2(x.acc2 _MUT), beaml(0 _MUT), beamr(0 _MUT),
    // cautacc(false _MUT), 					    percname(x.percname _MUT), perc(x.perc
    // _MUT) {
    // }
    noteev(noteev& x)
        : noteevbase(x), dynbase(WITHMUT_ x), tiedbase(WITHMUT),
          note(x.note _MUT), acc1(std::numeric_limits<fint>::max(), 1 _MUT),
          acc2(std::numeric_limits<fint>::max(), 1 _MUT), beaml(0 _MUT),
          beamr(0 _MUT), cautacc(false _MUT), percname(x.percname _MUT),
          perc(x.perc _MUT) {
      DBG("COPYING NOTEEV 1" << std::endl);
    } // for fomcloning
    noteev(noteev& x, const int, const numb& shift)
        : noteevbase(x, 0, shift), dynbase(WITHMUT_ x), tiedbase(WITHMUT),
          note(x.note _MUT), acc1(std::numeric_limits<fint>::max(), 1 _MUT),
          acc2(std::numeric_limits<fint>::max(), 1 _MUT), beaml(0 _MUT),
          beamr(0 _MUT), cautacc(false _MUT), percname(x.percname _MUT),
          perc(x.perc _MUT) {
      DBG("COPYING NOTEEV 1" << std::endl);
    } // for fomcloning
    noteev(noteev& x, const int v, const fomus_rat& p)
        : noteevbase(x, v, 0), dynbase(WITHMUT_ x), tiedbase(WITHMUT_ x, 0),
          note(p _MUT), acc1(x.acc1 _MUT), acc2(x.acc2 _MUT), beaml(0 _MUT),
          beamr(0 _MUT), cautacc(false _MUT), percname(x.percname _MUT),
          perc(x.perc _MUT) {
      DBG("COPYING NOTEEV 2" << std::endl);
    } // for metapart dist.
    noteev(noteev& x, const fomus_rat& note, const fomus_rat& acc1,
           const fomus_rat& acc2, const special_markslist& marks)
        : noteevbase(x, marks), dynbase(WITHMUT_ x), tiedbase(WITHMUT_ x, 0),
          note(note _MUT), acc1(rat(acc1.num, acc1.den) _MUT),
          acc2(rat(acc2.num, acc2.den) _MUT), beaml(0 _MUT), beamr(0 _MUT),
          cautacc(false _MUT), percname(x.percname _MUT), perc(x.perc _MUT) {
      DBG("COPYING NOTEEV 3" << std::endl);
    } // for new special note (harmonics, trills, etc.)
    void newinvnote(const fomus_rat& note, const fomus_rat& acc1,
                    const fomus_rat& acc2, const special_markslist& marks);
    noteevbase* fomclone(const numb& shift) {
      assert(isvalid());
      return new noteev(*this, 0, shift);
    }
    bool isfirsttied() const {
      READLOCK;
      return tiedbase::isfirsttied();
    }
    bool ismidtied() const {
      READLOCK;
      return tiedbase::ismidtied();
    }
    bool islasttied() const {
      READLOCK;
      return tiedbase::islasttied();
    }
    const numb& getdyn() const {
      READLOCK;
      return RMUT(dyn);
    }
    const numb& getdyn_nomut() const {
      return RMUT(dyn);
    }
    const numb& getnote() const {
      READLOCK;
      return RMUT(note);
    }
    const numb& getnote_nomut() const {
      return RMUT(note);
    }
    bool getisnote() const {
      return true;
    }
    numb gettiedendtime(measure* m) const;
    numb gettiedendtime() const;
    numb getwrittennote() const {
      READLOCK;
      return RMUT(note) - tofrat(RMUT(acc1) + RMUT(acc2));
    }
    numb getwrittennote_nomut() const {
      return RMUT(note) - tofrat(RMUT(acc1) + RMUT(acc2));
    }
    fomus_rat getfullacc() const {
      READLOCK;
      return tofrat(RMUT(acc1) + RMUT(acc2));
    }
    fomus_rat getfullacc_nomut() const {
      return tofrat(RMUT(acc1) + RMUT(acc2));
    }
    fomus_rat getacc1() const {
      READLOCK;
      fomus_rat r = {RMUT(acc1).numerator(), RMUT(acc1).denominator()};
      return r;
    }
    fomus_rat getacc1_nomut() const {
      fomus_rat r = {RMUT(acc1).numerator(), RMUT(acc1).denominator()};
      return r;
    }
    fomus_rat getacc2() const {
      READLOCK;
      fomus_rat r = {RMUT(acc2).numerator(), RMUT(acc2).denominator()};
      return r;
    }
    fomus_rat getacc2_nomut() const {
      fomus_rat r = {RMUT(acc2).numerator(), RMUT(acc2).denominator()};
      return r;
    }
    bool getisperc() const {
      READLOCK;
      return RMUT(percname);
    }
    bool getisperc_nomut() const {
      return RMUT(percname);
    }
    bool isbeambeg() const {
      return !RMUT(beaml) && RMUT(beamr);
    }
    bool isbeamend() const {
      return RMUT(beaml) && !RMUT(beamr);
    }
    bool isbeammid() const {
      return RMUT(beaml) && RMUT(beamr);
    }
    std::pair<rat, rat> getbothaccs() const {
      READLOCK;
      return std::pair<rat, rat>(RMUT(acc1), RMUT(acc2));
    }
    bool getistiedleft() const {
      READLOCK;
      return RMUT(tiedl);
    }
    bool getistiedleft_nomut() const {
      return RMUT(tiedl);
    }
    bool getistiedright() const {
      READLOCK;
      return RMUT(tiedr);
    }
    bool getistiedright_nomut() const {
      return RMUT(tiedr);
    }
    bool getisbeginchord() const;
    bool getisendchord() const;
    bool getischordlow() const;
    bool getischordhigh() const;
    noteevbase* getsplitat(const numb& off) {
      return new noteev(*this, off);
    }
    noteevbase* getsplitat0(const numb& off) {
      return new noteev(*this, off, 0);
    }
    noteevbase* getsplitatt(const numb& off) {
      return new noteev(0, *this, off);
    }
    void assignpquant(const fomus_rat& p) {
      {
        WRITELOCK;
        WMUT(note) = p;
      }
      post_apisetvalue();
    }
    void postassignpquant(const numb& p) {
      UPREADLOCK;
      {
        TOWRITELOCK;
        WMUT(note) = p;
      }
      if (RMUT(tiedr)) {
        (UNLOCKP((noteev*) RMUT(tiedr)))->postassignpquant(p);
      }
    }
    void checkpitch(const char* wh) {
      if (!RMUT(note).israt()) {
        CERR << "non-rational pitch value";
        integerr(wh);
      }
      if (RMUT(note) < (fint) 0 || RMUT(note) > (fint) 128) {
        CERR << "out of range pitch value";
        integerr(wh);
      }
    }
    void checkpquant() {
      {
        READLOCK;
        checkpitch("pitch quantization");
        if (RMUT(tiedr)) {
          const numb& p(RMUT(note));
          (UNLOCKP((noteev*) RMUT(tiedr)))->postassignpquant(p);
        }
      }
      post_apisetvalue();
    }
    void assignvoices(const int voice) {
      {
        WRITELOCK;
        voicesbase::assign(voice);
      }
      post_apisetvalue();
    }
    int postassignvoices();
    void checkvoices();
    void assignstaves(const int staff, const int clef) {
      {
        WRITELOCK;
        stavesbase::assign(staff, clef);
      }
      post_apisetvalue();
    }
    std::pair<int, int> postassignstaves();
    void checkstaves();
    bool checkstaves0() {
      bool ft;
      {
        READLOCK;
        ft = tiedbase::isfirsttied();
      }
      post_apisetvalue();
      return !ft;
    }
    void assignaccs(const fomus_rat& acc, const fomus_rat& macc) {
      {
        WRITELOCK;
        WMUT(acc1) = rat(acc.num, acc.den);
        WMUT(acc2) = rat(macc.num, macc.den);
      }
      post_apisetvalue();
    }
    void postassignacc(const rat& a1, const rat& a2) {
      UPREADLOCK;
      {
        TOWRITELOCK;
        WMUT(acc1) = a1;
        WMUT(acc2) = a2;
      }
      if (RMUT(tiedr)) {
        (UNLOCKP((noteev*) RMUT(tiedr)))->postassignacc(a1, a2);
      }
    }
    void checkacc1(const fomusdata* fom);
    void assigncautacc() {
      {
        WRITELOCK;
        WMUT(cautacc) = true;
      }
      post_apisetvalue();
    }
    void assignbeams(const int bl, const int br) {
      assert(bl >= 0 && bl <= 16 && br >= 0 && br <= 16);
      {
        WRITELOCK;
        WMUT(beaml) = bl;
        WMUT(beamr) = br;
      }
      post_apisetvalue();
    }
    void assignbeams2(const int bl, const int br, UPLOCKPAR) {
      assert(bl >= 0 && bl <= 16 && br >= 0 && br <= 16);
      TOWRITELOCK;
      WMUT(beaml) = bl;
      WMUT(beamr) = br;
    }
    bool objless(const modobjbase& y) const {
      return y.objgreat(*this);
    }
    bool objless(const noteev& y) const {
      return *this < y;
    }
    bool objgreat(const noteev& y) const {
      return y < *this;
    }
    void getblbr(int& bl, int& br) {
      bl = RMUT(beaml);
      br = RMUT(beamr);
    }
    void checkbeam() {
      if (RMUT(beaml) < 0 || RMUT(beamr) < 0 || RMUT(beaml) > 16 ||
          RMUT(beamr) > 16) {
        CERR << "invalid beam";
        integerr("beams");
      }
    }
#ifndef NDEBUGOUT
    void dumpall() const;
#endif
    void assignmpart(const module_partobj part, const int voice,
                     const fomus_rat& pitch) {
      WRITELOCK;
      std::auto_ptr<noteevbase> xx(new noteev(*this, voice, pitch));
      UNLOCKWRITEP((modobjbase*) part)->assignmpart(xx);
    }
    module_percinstobj getpercinst() const;
    const percinstr_str* getpercinst0() const {
      assert(isvalid());
      return RMUT(perc);
    } // perc instr returns something different
    void assignpercinst(const int voice, struct fomus_rat& pitch) {
      {
        WRITELOCK;
        voicesbase::assign(voice);
        WMUT(note) = pitch;
      }
      post_apisetvalue();
    }
    void estabpercinst();
    int getbeamsleft() const {
      READLOCK;
      return RMUT(beaml);
    }
    int getbeamsleft_nomut() const {
      return RMUT(beaml);
    }
    int getbeamsright() const {
      READLOCK;
      return RMUT(beamr);
    }
    int getbeamsright_nomut() const {
      return RMUT(beamr);
    }
    void killtiedr() {
      assert(isvalid());
      tiedbase::killtiedr();
    }
    void killtiedl() {
      assert(isvalid());
      tiedbase::killtiedl();
    }
    const char* getpercinststr() const {
      READLOCK;
      return RMUT(percname);
    }
    void tiewillbetieds(const numb& no, const bool will, noteev* ev,
                        UPLOCKPAR) {
      if (RMUT(note).isntnull() && no.isntnull() && RMUT(note) == no &&
          willbetiedr && will) {
        {
          TOWRITELOCK;
          WMUT(tiedr) = ev;
        }
        UNLOCK;
        ev->tiewillbetiedsaux(this);
      }
    }
    void tiewillbetiedsaux(noteev* x) {
      WRITELOCK;
      WMUT(tiedl) = x;
    }
    void replaceclef(const int cl, UPLOCKPAR);
    void replaceclef0(const int cl);
    // bool maybereplaceclef(const int cl);
    int getconnbeamsleft() const;
    int getconnbeamsright() const;
    struct module_keysigref getkeysigacc() const;
    modobjbase* leftmosttied() {
      READLOCK;
      if (RMUT(tiedl))
        return UNLOCKP(RMUT(tiedl))->leftmosttied();
      else
        return this;
    }
    modobjbase* rightmosttied() {
      READLOCK;
      if (RMUT(tiedr))
        return UNLOCKP(RMUT(tiedr))->rightmosttied();
      else
        return this;
    }
    bool getwillbetiedl() const {
      return willbetiedl;
    }
    bool getwillbetiedr() const {
      return willbetiedr;
    }
    bool lowchordaux1(bool& bl, const offgroff& ti, const int v,
                      const rat& n) const {
      READLOCK;
      if (isinvisible_nomut())
        return false;
      if (getfulltime_nomut() < ti) {
        bl = true;
        return true;
      }
      if (get1voice() == v && numtorat(getnote_nomut()) < n) {
        bl = false;
        return true;
      }
      return false;
    }
    bool lowchordaux2(bool& bl, const offgroff& ti, const int v,
                      const rat& n) const {
      READLOCK;
      if (isinvisible_nomut())
        return false;
      if (getfulltime_nomut() > ti) {
        bl = true;
        return true;
      }
      if (get1voice() == v && numtorat(getnote_nomut()) < n) {
        bl = false;
        return true;
      }
      return false;
    }
    bool highchordaux1(bool& bl, const offgroff& ti, const int v,
                       const rat& n) const {
      READLOCK;
      if (isinvisible_nomut())
        return false;
      if (getfulltime_nomut() < ti) {
        bl = true;
        return true;
      }
      if (get1voice() == v && numtorat(getnote_nomut()) > n) {
        bl = false;
        return true;
      }
      return false;
    }
    bool highchordaux2(bool& bl, const offgroff& ti, const int v,
                       const rat& n) const {
      READLOCK;
      if (isinvisible_nomut())
        return false;
      if (getfulltime_nomut() > ti) {
        bl = true;
        return true;
      }
      if (get1voice() == v && numtorat(getnote_nomut()) > n) {
        bl = false;
        return true;
      }
      return false;
    }
    void maybekilltiedr(const tiedbase* th) {
      assert(isvalid());
      UPREADLOCK;
      if (RMUT(tiedr) == th) {
        TOWRITELOCK;
        killtiedr();
      }
    }
    void maybekilltiedl(const tiedbase* th) {
      assert(isvalid());
      UPREADLOCK;
      if (RMUT(tiedl) == th) {
        TOWRITELOCK;
        killtiedl();
      }
    }
    void trem_fix(const numb& chop, WRLOCKPAR);
    void trem_halfdur(const numb& chop) {
      WRITELOCK;
      trem_fix(chop, WRLOCKARG);
      durbase::trem_halfdur((chop.isnull() ? RMUT(dur) : chop) / (fint) 2);
    }
    void trem_incoff(const numb& chop) {
      WRITELOCK;
      if (chop.isnull()) {
        durbase::trem_incoff(RMUT(dur) / (fint) 2);
      } else {
        durbase::trem_halfdur(chop);
        durbase::trem_incoff(chop / (fint) 2);
      }
      assdel = true;
    }
    void trem_halfdur0(const numb& chop) {
      WRITELOCK;
      durbase::trem_halfdur(chop);
    }
    void trem_incoff0(const numb& chop) {
      WRITELOCK;
      durbase::trem_incoff(chop);
      assdel = true;
    }
    void trem_dur0(const numb& chop) {
      {
        WRITELOCK;
        trem_fix(chop, WRLOCKARG);
        durbase::trem_dur0();
      }
      assdel = true;
    }
    void destroytiedl() {
      UPREADLOCK;
      noteev* x = RMUT(tiedl);
      if (x) {
        {
          TOWRITELOCK;
          WMUT(tiedl) = 0;
        }
        UNLOCK;
        x->maybekilltiedr(this);
      }
    }
    void destroytiedr() {
      UPREADLOCK;
      noteev* x = RMUT(tiedr);
      if (x) {
        {
          TOWRITELOCK;
          WMUT(tiedr) = 0;
        }
        UNLOCK;
        x->maybekilltiedl(this);
      }
    }
    void switchtrems(const module_markids from, const module_markids to) {
      WRITELOCK;
      marksbase::switchtrems(from, to);
    }
    void settiedl(noteev* x) {
      WRITELOCK;
      tiedbase::settiedl(x);
    }
    void settiedr(noteev* x) {
      WRITELOCK;
      tiedbase::settiedr(x);
    }
    void assignunsplit() {
      assdel = true;
      post_apisetvalue();
    }
    bool unsplit(noteevbase& lt) {
      return assdel ? lt.unsplitaux(*this) : false;
    }
    bool unsplitaux(noteev& rt) {
      if (RMUT(tiedr) == &rt) {
        WMUT(dur) = RMUT(dur) + RMUT(rt.dur);
        marksbase::unsplitmarks(rt);
        durbase::unsplittups(rt);
        return true;
      } else
        return false;
    }
  };
  inline tiedbase::tiedbase(MUTPARAM_ const noteev& x, int)
      : tiedl((noteev*) 0 _MUTP), tiedr((noteev*) 0 _MUTP),
        willbetiedl((bool) RMUT(x.tiedl)), willbetiedr((bool) RMUT(x.tiedr)) {
  } // copying noteev w/o split--tiedl and tiedr are invalid and must be fixed!
  inline tiedbase::~tiedbase() {
    if (CMUT(tiedl)) {
      CMUT(tiedl)->maybekilltiedr(this);
    }
    if (CMUT(tiedr)) {
      CMUT(tiedr)->maybekilltiedl(this);
    }
  }
#ifndef NDEBUG
  inline void tiedbase::isvalidptrs() const {
    assert(!CMUT(tiedl) || CMUT(tiedl)->isvalid());
    assert(!CMUT(tiedr) || CMUT(tiedr)->isvalid());
  }
#endif

  class markev : public event,
                 public durbase,
                 public voicesbase,
                 public marksbase {
public:
    markev(const numb& off, const numb& groff, const numb& dur,
           const std::set<int>& voices, const filepos& pos,
           const boost::ptr_set<markobj>& mm, const pointtype point,
           const setmap& sets0)
        : event(pos, sets0), durbase(WITHMUT_ off, groff, dur, point),
          voicesbase(WITHMUT_ voices), marksbase(WITHMUT_ mm) {
      DISABLEMUTCHECK;
      recachemarks();
    }
    markev(markev& x)
        : event(x, 0), durbase(WITHMUT_ x), voicesbase(WITHMUT_ x),
          marksbase(WITHMUT_ x, 0) {
      DISABLEMUTCHECK;
      recachemarks();
    }
    markev(markev& x, const numb& shift)
        : event(x, 0), durbase(WITHMUT_ x, 0, shift), voicesbase(WITHMUT_ x),
          marksbase(WITHMUT_ x, 0) {
      DISABLEMUTCHECK;
      recachemarks();
    }
    markev(markev& x, const int voice)
        : event(x, 0), durbase(WITHMUT_ x), voicesbase(WITHMUT_ voice),
          marksbase(WITHMUT_ x, 0) {
      assert(!CMUT(clf));
      assert(!CMUT(stf));
      DISABLEMUTCHECK;
      recachemarks();
    }
    markev* fomclone(const numb& shift) {
      assert(isvalid());
      return new markev(*this, shift);
    }
    const numb& gettime() const {
      READLOCK;
      return RMUT(off).off;
    }
    const numb& gettime_nomut() const {
      return RMUT(off).off;
    }
#ifndef NDEBUG
    const numb& gettime_nomut(MUTDBGPAR) const {
      return CMUT(off).off;
    }
#endif
    const numb& getgracetime() const {
      READLOCK;
      return RMUT(off).groff;
    }
    const numb& getgracetime_nomut() const {
      return RMUT(off).groff;
    }
#ifndef NDEBUG
    const numb& getgracetime_nomut(MUTDBGPAR) const {
      return CMUT(off).groff;
    }
#endif
    numb getdur() const {
      READLOCK;
      return isgrace() ? numb((fint) 0) : RMUT(dur);
    }
    numb getdur_nomut() const {
      return isgrace() ? numb((fint) 0) : RMUT(dur);
    }
    numb getendtime() const {
      READLOCK;
      return isgrace() ? RMUT(off).off : RMUT(off).off + RMUT(dur);
    }
    numb getendtime_nomut() const {
      return isgrace() ? RMUT(off).off : RMUT(off).off + RMUT(dur);
    }
#ifndef NDEBUG
    numb getendtime_nomut(MUTDBGPAR) const {
      return isgrace() ? CMUT(off).off : CMUT(off).off + CMUT(dur);
    }
#endif
    numb getgraceendtime() const {
      READLOCK;
      if (!isgrace()) {
        module_value x;
        x.type = module_none;
        return x;
      }
      return RMUT(off).groff + RMUT(dur);
    }
    numb getgracedur() const {
      READLOCK;
      if (!isgrace()) {
        module_value x;
        x.type = module_none;
        return x;
      }
      return RMUT(dur);
    }
    module_value getgraceendtimenochk() const {
      return RMUT(off).groff + RMUT(dur);
    }
#ifndef NDEBUG
    module_value getgraceendtimenochk(MUTDBGPAR) const {
      return CMUT(off).groff + CMUT(dur);
    }
#endif
    int getvoice() const {
      READLOCK;
      return get1voice();
    }
    int getvoice_nomut() const {
      return get1voice();
    }
    struct module_intslist getvoices() {
      READLOCK;
      return voicesbase::getvoices();
    }
    bool getisgrace() const {
      READLOCK;
      return isgrace();
    }
    bool getisgrace_nomut() const {
      return isgrace();
    }
#ifndef NDEBUG
    bool getisgrace_nomut(MUTDBGPAR) const {
      return isgrace(MUTDBG);
    }
#endif
    bool gethasvoice(const int voice) const {
      READLOCK;
      return voicesbase::hasvoice(voice);
    }
    bool gethasvoice_nomut(const int voice) const {
      return voicesbase::hasvoice(voice);
    }
    module_markslist getmarks() {
      READLOCK;
      module_markslist l = {
          RMUT(marksbase::marks).size(),
          (const module_markobj*) RMUT(marksbase::marks).c_array()};
      return l;
    }
    module_markslist getmarks_nomut() {
      module_markslist l = {
          RMUT(marksbase::marks).size(),
          (const module_markobj*) RMUT(marksbase::marks).c_array()};
      return l;
    }
    module_markslist getsinglemarks() {
      assert(isvalid());
      READLOCK;
      return RMUT(marksbase::s);
    }
    module_markslist getsinglemarks_nomut() {
      assert(isvalid());
      return RMUT(marksbase::s);
    }
    module_markslist getspannerbegins() {
      assert(isvalid());
      READLOCK;
      return RMUT(marksbase::b);
    }
    module_markslist getspannerbegins_nomut() {
      assert(isvalid());
      return RMUT(marksbase::b);
    }
    module_markslist getspannerends() {
      assert(isvalid());
      READLOCK;
      return RMUT(marksbase::e);
    }
    module_markslist getspannerends_nomut() {
      assert(isvalid());
      return RMUT(marksbase::e);
    }
    void recachemarks() { // called from intern.cc, so needs to lock
      WMUT(marksbase::marks).sort(marksetlt());
      marksbase::cachessort();
    }
    void assignmpart(const module_partobj part, const int voice,
                     const fomus_rat& pitch) {
      WRITELOCK;
      std::auto_ptr<markev> xx(new markev(*this, voice));
      assert(((modobjbase*) part)->isvalid());
      UNLOCKWRITEP((modobjbase*) part)->assignmpartmarkev(xx);
    }
  };

  class restev : public noteevbase { // exists in a measure
    bool fill;

public:
    restev(const numb& off, const numb& groff, const numb& dur,
           const std::set<int>& voices, const filepos& pos,
           const boost::ptr_set<markobj>& mm, const pointtype point,
           const setmap& sets0)
        : noteevbase(off, groff, dur, voices, pos, mm, point, sets0),
          fill(false) {}
    restev(const numb& off, const numb& groff, const numb& dur,
           const int voices, const int staves, const filepos& pos,
           const boost::ptr_vector<markobj>& mm)
        : noteevbase(off, groff, dur, voices, staves, pos, mm), fill(false) {}
    restev(const numb& off, const numb& groff, const numb& dur,
           const int voices, const int staves, const filepos& pos)
        : noteevbase(off, groff, dur, voices, staves, pos, voices >= 1000),
          fill(false) {}
    restev(const numb& off, const numb& groff, const numb& dur,
           const int voices, const int staves, const filepos& pos, const int)
        : // called from insertfiller
          noteevbase(off, groff, dur, voices, staves, pos, voices >= 1000),
          fill(true) {}
    restev(const numb& off, const numb& groff, const numb& dur,
           const int voices, const filepos& pos)
        : noteevbase(off, groff, dur, voices, pos), fill(false) {
      assert(isvalid());
    } // called from 'assigndetmark', invisible = true;

    restev(restev& x, const numb& off)
        : noteevbase(x, off), fill(x.fill) {
    } // called when splitting...  what about invisible?
    restev(restev& x, const numb& off, int)
        : noteevbase(x, off, 0), fill(x.fill) {
    } // called when splitting...  what about invisible?
    restev(restev& x, const int v) : noteevbase(x, v), fill(x.fill) {
      assert(!RMUT(x.invisible));
    } // what about invisible?
    restev(restev& x) : noteevbase(x), fill(x.fill) {
      assert(true);
    } // for fomcloning
    restev(restev& x, const int, const numb& shift)
        : noteevbase(x, 0, shift), fill(x.fill) {
      assert(true);
    } // for fomcloning
    restev(restev& x, const int v, int) : noteevbase(x, v, 0), fill(x.fill) {
      assert(true);
    }

    noteevbase* fomclone(const numb& shift) {
      assert(isvalid());
      return new restev(*this, 0, shift);
    }
    bool isfill() const {
      return fill;
    }
    bool isfirsttied() const {
      return true;
    }
    bool ismidtied() const {
      return false;
    }
    bool islasttied() const {
      return false;
    }
    bool getisrest() const {
      return true;
    }
    numb gettiedendtime() const {
      READLOCK;
      return getendtime_nomut();
    }
    noteevbase* getsplitat(const numb& off) {
      return new restev(*this, off);
    }
    noteevbase* getsplitat0(const numb& off) {
      return new restev(*this, off, 0);
    }
    void checkrestvoices(boost::ptr_list<noteevbase>& tmp); // no checkstaves()
    void domerge() {
      {
        UPREADLOCK;
        if (RMUT(mergeto)) {
          checkmerge(*RMUT(mergeto));
          TOWRITELOCK;
          WMUT(invisible) = true;
        }
      }
      post_apisetvalue();
    }
    void assignrstaff(const int staff, const int clef) {
      {
        WRITELOCK;
        stavesbase::assign(staff, clef);
      }
      post_apisetvalue();
    }
    void assignbeams(const int bl, const int br) {
      post_apisetvalue();
    }
    void checkstaves2();
#ifndef NDEBUGOUT
    void dumpall() const;
#endif
    bool fmrnotsameas(const restev& e) const {
      return marksbase::noteq(e);
    } // basically check if marks are the same
    void assignmpart(const module_partobj part, const int voice,
                     const fomus_rat& pitch) {
      WRITELOCK;
      std::auto_ptr<noteevbase> xx(new restev(*this, voice));
      UNLOCKWRITEP((modobjbase*) part)->assignmpart(xx);
    }
    int getbeamsleft() const {
      return 0;
    }
    int getbeamsleft_nomut() const {
      return 0;
    }
    int getbeamsright() const {
      return 0;
    }
    int getbeamsright_nomut() const {
      return 0;
    }
    int getconnbeamsleft() const {
      return 0;
    }
    int getconnbeamsright() const {
      return 0;
    }
    bool getismarkrest() const {
      READLOCK;
      return get1voice() >= 1000;
    }
    bool getismarkrest_nomut() const {
      return get1voice() >= 1000;
    }
  };
  inline tiedbase::tiedbase(MUTPARAM_ noteev& lft)
      : tiedl(&lft _MUTP), tiedr(lft.tiedr _MUTP) {
    WMUT(lft.tiedr) = (noteev*) this;
    if (CMUT(tiedr))
      CMUT(tiedr)->settiedl((noteev*) this);
    willbetiedl = CMUT(tiedl);
    willbetiedr = CMUT(tiedr);
  }
  inline tiedbase::tiedbase(MUTPARAM_ int, noteev& lft)
      : tiedl((noteev*) 0 _MUTP), tiedr(lft.tiedr _MUTP) {
    WMUT(lft.tiedr) = 0;
    if (CMUT(tiedr))
      CMUT(tiedr)->settiedl((noteev*) this);
    willbetiedl = false;
    willbetiedr = CMUT(tiedr);
  }

  class part;
  class stage;
  struct stageless { // x->getid() < y->getid();
    bool operator()(const stage* x, const stage* y) const;
  };
  struct noteisbad : public errbase {};
  class measure : public modobjbase_sets, public durbase NONCOPYABLE {
    boost::shared_mutex mut; //, cachemut;
protected:
    boost::shared_ptr<measdef_str> attrs; // this won't ever change
    MUTCHECK(eventmap) events;
    std::set<stage*, stageless> stages;
    boost::shared_mutex wakeupsmut;
    std::multimap<stage*, boost::condition_variable_any*> wakeups;
    partormpart_str* prt;
    measmap_it self, tmpself;
    MUTCHECK(std::vector<fomus_rat>) divs;
    const bool rmable;
    MUTCHECK(bool) isfmr;
    MUTCHECK(std::string) newkey; // don't need to copy this
    struct modout_keysig keysigcache;
    std::vector<modout_keysig_indiv> kscache;
    MUTCHECK(module_barlines) barlin;
    const void* orig; // <-- not actually used as pointer, used to distinguish
                      // user-defined blocks
    MUTCHECK(int) icp;

public:
    measure(const numb& off, const numb& dur,
            boost::shared_ptr<measdef_str>& attrs, partormpart_str& prt)
        : durbase(WITHOUTMUT_ off, (fint) 0, dur),
          attrs(attrs) _MUTINIT(events), prt(&prt) _MUTINIT(divs),
          rmable(false), isfmr(false _MUT) _MUTINIT(newkey),
          barlin(barline_normal _MUT),
#ifndef NDEBUG
          orig(0),
#endif
          icp(0 _MUT) _MUTINIT(newevents) {
    }
    measure(const numb& off, const numb& dur,
            boost::shared_ptr<measdef_str>& attrs, partormpart_str* prt,
            const bool rmable, const measure* orig)
        : durbase(WITHOUTMUT_ off, (fint) 0, dur),
          attrs(attrs) _MUTINIT(events), prt(prt) _MUTINIT(divs),
          rmable(rmable), isfmr(false _MUT) _MUTINIT(newkey),
          barlin(barline_normal _MUT), orig(orig),
          icp(0 _MUT) _MUTINIT(newevents) {}
    measure(boost::shared_ptr<measdef_str>& ms, partormpart_str& prt)
        : durbase(WITHOUTMUT_(fint) 0, (fint) 0, (fint) 4),
          attrs(ms) _MUTINIT(events), prt(&prt) _MUTINIT(divs), rmable(false),
          isfmr(false _MUT) _MUTINIT(newkey), barlin(barline_normal _MUT),
#ifndef NDEBUG
          orig(0),
#endif
          icp(0 _MUT) _MUTINIT(newevents) {
    } // the initial full-score measure
    measure(measure& x, partormpart_str* prt)
        : durbase(WITHOUTMUT_ x), attrs(x.attrs) _MUTINIT(events),
          prt(prt) _MUTINIT(divs), rmable(false),
          isfmr(false _MUT) _MUTINIT(newkey), barlin(CMUT(x.barlin) _MUT),
          orig(x.orig), icp(0 _MUT) _MUTINIT(newevents) {}
    measure(measure& x, partormpart_str* prt, const int, const numb& shift)
        : durbase(WITHOUTMUT_ x, 0, shift), attrs(x.attrs) _MUTINIT(events),
          prt(prt) _MUTINIT(divs), rmable(false),
          isfmr(false _MUT) _MUTINIT(newkey), barlin(CMUT(x.barlin) _MUT),
          orig(x.orig), icp(0 _MUT) _MUTINIT(newevents) {}
    measure(measure& x, partormpart_str* prt, const numb& off)
        : durbase(WITHOUTMUT_ off, (fint) 0, RMUT(x.dur)),
          attrs(x.attrs) _MUTINIT(events), prt(prt) _MUTINIT(divs),
          rmable(false), isfmr(false _MUT) _MUTINIT(newkey),
          barlin(CMUT(x.barlin) _MUT), orig(x.orig),
          icp(0 _MUT) _MUTINIT(newevents) {
      assert(!CMUT(x.isfmr));
      getkeysig_init();
    }
    measure(measure& x, partormpart_str* prt, const numb& off, const numb& dur,
            const int i)
        : durbase(WITHOUTMUT_ off, (fint) 0, dur),
          attrs(x.attrs) _MUTINIT(events), prt(prt) _MUTINIT(divs),
          rmable(true), // pickup measure constructor
          isfmr(false _MUT) _MUTINIT(newkey), barlin(CMUT(x.barlin) _MUT),
          orig(x.orig), icp(i _MUT) _MUTINIT(newevents) {
      // assert(!CMUT(x.isfmr));
      // getkeysig_init();
      assert(false); // need to know if getkeysig_init has been called for other
                     // measures yet
    }
    void insertnew(noteevbase* ev) {
      ev->setmeas(this, XMUT(events).insert(ev->getfulltime_nomut(MUTDBG), ev));
    }
    eventmap_it insertnewret(noteevbase* ev) {
      eventmap_it r = XMUT(events).insert(ev->getfulltime_nomut(MUTDBG), ev);
      ev->setmeas(this, r);
      return r;
    }
    measure* fomclone(partormpart_str* prt, const numb& shift) {
      assert(isvalid());
      return new measure(*this, prt, 0, shift);
    }
    const eventmap& getevents() const {
      return CMUT(events);
    }
    eventmap& getevents() {
      return CMUT(events);
    }
    const measdef_str& getattrs() const {
      return *attrs.get();
    }
    void measisready();
    std::set<stage*, stageless>& getstages() {
      return stages;
    }
    bool issameblock(const measure& x) const {
      return x.orig == orig;
    }
    bool isntsameblock(const measure& x) const {
      return x.orig != orig;
    }
    void stagedone();
    void stagedone_nomut();
    void post_apisetvalue();
    void removerest(noteevbase& n);
    void setself(const measmap_it& s) {
      self = s;
    }
    void settmpself(const measmap_it& x) {
      tmpself = x;
    }
    const varbase& get_varbase(const int id) const;
    fint get_ival(const int id) const;
    rat get_rval(const int id) const;
    ffloat get_fval(const int id) const;
    const std::string& get_sval(const int id) const;
    const module_value& get_lval(const int id) const;
    const char* getcid() const {
      return attrs->getcid();
    }
    bool get_varbase0(const int id, const varbase*& ret) const {
      return attrs->get_varbase0(id, ret);
    }
    bool get_ival0(const int id, fint& ret) const {
      return attrs->get_ival0(id, ret);
    }
    bool get_rval0(const int id, rat& ret) const {
      return attrs->get_rval0(id, ret);
    }
    bool get_fval0(const int id, ffloat& ret) const {
      return attrs->get_fval0(id, ret);
    }
    bool get_sval0(const int id, const std::string*& ret) const {
      return attrs->get_sval0(id, ret);
    }
    bool get_lval0(const int id, const module_value*& ret) const {
      return attrs->get_lval0(id, ret);
    }
    const varbase& get_varbase_up(const int id, const event& ev) const;
    fint get_ival_up(const int id, const event& ev) const;
    rat get_rval_up(const int id, const event& ev) const;
    ffloat get_fval_up(const int id, const event& ev) const;
    const std::string& get_sval_up(const int id, const event& ev) const;
    const module_value& get_lval_up(const int id, const event& ev) const;
    const measmap_it& getmeasselfit() const {
      return self;
    }
    const measmap_it& getmeasselfit(const bool istmp) const {
      return istmp ? tmpself : self;
    }
    partormpart_str& getpart() {
      return *prt;
    }
    const partormpart_str& getpart() const {
      return *prt;
    }
    const char* gettype() const {
      return "a measure";
    }
    modobjbase* getmeasobj() {
      return this;
    }
    modobjbase* getpartobj() {
      return prt;
    }
    modobjbase* getinstobj() {
      return prt->getinstobj();
    }
    const numb& gettime() const {
      return RMUT(off).off;
    } // measure
#ifndef NDEBUG
    const numb& gettime_nomut(MUTDBGPAR) const {
      return CMUT(off).off;
    } // measure
#endif
    numb getdur() const {
      return RMUT(dur);
    } // measure
    numb getendtime() const {
      return RMUT(off).off + RMUT(dur);
    } // measure
    struct module_intslist getclefs() {
      return prt->getclefs();
    } // measure
    struct module_intslist getclefs(const int st) {
      return prt->getclefs(st);
    }
    bool gethasclef(const int clef) {
      return prt->gethasclef(clef);
    }
    bool gethasclef(const int clef, const int staff) {
      return prt->gethasclef(clef, staff);
    }
    module_ratslist getdivs() const {
      module_ratslist r;
      READLOCK;
      r.n = RMUT(divs).size();
      r.rats = &RMUT(divs)[0];
      return r;
    }
    bool getwritaccaux(const noteevbase& ev, SHLOCKPAR) const;
    fomus_rat getfullwritaccaux(const noteevbase& ev, SHLOCKPAR) const {
      assert(ev.isrlocked());
      return getwritaccaux(ev, LOCKARG)
                 ? ((const noteev&) ev).getfullacc_nomut()
                 : makerat(std::numeric_limits<fint>::max(), 1);
    }
    fomus_rat getwritacc1aux(const noteevbase& ev, SHLOCKPAR) const {
      assert(ev.isrlocked());
      return getwritaccaux(ev, LOCKARG)
                 ? ((const noteev&) ev).getacc1_nomut()
                 : makerat(std::numeric_limits<fint>::max(), 1);
    }
    fomus_rat getwritacc2aux(const noteevbase& ev, SHLOCKPAR) const {
      assert(ev.isrlocked());
      return getwritaccaux(ev, LOCKARG)
                 ? ((const noteev&) ev).getacc2_nomut()
                 : makerat(std::numeric_limits<fint>::max(), 1);
    }
    void assignmeas(const fomus_rat& time, const fomus_rat& dur,
                    const bool rmable);
    void fixtimequant(numb& mx, const bool inv);
    bool canremove() const {
      return rmable && CMUT(events).empty();
    }
    numb getlastgrendoff(const numb& off, const int v) const;
    numb getfirstgroff(const numb& off, const int v) const;
    void checkprune();
    void fillhole(const numb& o1, const numb& o2, const int v) {
      WRITELOCK;
      assert(o1.israt());
      assert(o2.israt());
      assert(o1 < o2);
      WMUT(newevents).insert(
          numb((fint) 0),
          new restev(o1, module_none, o2 - o1, v, 1, filepos("(internal)")));
    }
    void fillgrhole(const numb& o1, const numb& go1, const numb& o2,
                    const int v) {
      WRITELOCK;
      assert(o1.israt());
      assert(o2.israt());
      assert(go1.israt());
      assert(go1 < o2);
      WMUT(newevents).insert(numb((fint) 0), new restev(o1, go1, o2 - go1, v, 1,
                                                        filepos("(internal)")));
    }
    void fillholes2() {
      if (CMUT(events).empty()) {
        WMUT(isfmr) = true;
        insertnew(new restev(RMUT(off).off, module_none, getdur(), 1, 1,
                             filepos("(internal)")));
      }
    }
    void fillholes3() {
      if (RMUT(isfmr)) {
        assert(!CMUT(events).empty());
        XMUT(events).erase(boost::next(CMUT(events).begin()),
                           CMUT(events).end());
        CMUT(events).begin()->second->setdur(getdur());
      }
      if (CMUT(events).empty()) {
        WMUT(isfmr) = true;
        insertnew(new restev(RMUT(off).off, module_none, getdur(), 1, 1,
                             filepos("(internal)")));
      }
    }
    void preprocess() {
      for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end(); ++i)
        i->second->preprocess();
    }
    fint partindex() const {
      return prt->index();
    }
#ifndef NDEBUG
    void resetstage() {
      assert(stages.empty());
      assert(wakeups.empty());
      DBG("stages and wakeups OK" << std::endl);
    }
#endif
#ifndef NDEBUGOUT
    void showstage();
#endif
    staff_str* getstaffptr(const int staff) const {
      return prt->getstaffptr(staff);
    }
    clef_str* getclefptr(const int staff, const int clef) const {
      return prt->getclefptr(staff, clef);
    }
    void assigndivs(const module_ratslist& divs0);
    void assignsplit(const divide_split& split0) {
      assert(((modobjbase*) split0.note)->isvalid());
      ((modobjbase*) split0.note)
          ->getnoteevbase()
          .getsplits()
          .insert(rat(split0.time.num, split0.time.den),
                  new divsplitstr(split0));
    }
    void assigngracesplit(const divide_gracesplit& split0) {
      assert(((modobjbase*) split0.note)->isvalid());
      ((modobjbase*) split0.note)
          ->getnoteevbase()
          .getsplits()
          .insert(rat(split0.grtime.num, split0.grtime.den), 0);
    }
    void fixties();
    void doprune();
    void collectallvoices(std::set<int>& vv);
    void collectallstaves(std::set<int>& ss);
    modobjbase* getmeasdef() {
      return (modobjbase*) attrs.get();
    }
    boost::shared_ptr<measdef_str>& measdefptr() {
      return attrs;
    }
    const char* getprintstr() const {
      return attrs->getmeasprintstr();
    }
    std::vector<int>
        voicescache; // volatile bool voicescached; // parts/measures don't get
                     // copied/split--can put vector right inside the class
    const std::vector<int>& getvoicescache() const {
      return voicescache;
    }
    struct module_intslist getvoices() { // remove the const!  measure
      module_intslist r = {voicescache.size(), &voicescache[0]};
      return r;
    }
    std::vector<int> stavescache; // volatile bool stavescached;
    const std::vector<int>& getstavescache() const {
      return stavescache;
    }
    struct module_intslist getstaves() { // remove the const! measure
      module_intslist r = {stavescache.size(), &stavescache[0]};
      return r;
    }
    bool objless(const modobjbase& y) const {
      return y.objgreat(*this);
    }
    bool objless(const measure& y) const {
      return *this < y;
    }
    bool objgreat(const measure& y) const {
      return y < *this;
    }
    void initclefscaches() {
      for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end(); ++i)
        i->second->getclefsinit();
    }
    struct module_intslist getpartclefs() const;
    int getpartnstaves() const;
    const std::string& getpartid() const;
    fomus_rat timesig() const;
    rat writtenmultrat() const { // return "beat" modified for compound meters
      rat d(get_rval(BEAT_ID));
      if (get_ival(COMP_ID))
        d = rat(d.numerator() * 3, d.denominator() * 2);
      return d;
    }
    fomus_rat
    writtenmult() const { // return "beat" modified for compound meters
      rat d(get_rval(BEAT_ID));
      if (get_ival(COMP_ID))
        d = rat(d.numerator() * 3, d.denominator() * 2);
      fomus_rat re = {d.numerator(), d.denominator()};
      return re;
    }
    void sortord(fomus_int& i0) {
      for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end(); ++i)
        i->second->sortord(i0);
    }
    bool getisbeginchord(eventmap_it i, const offgroff& ti, const int v) const;
    bool getisendchord(eventmap_it i, const offgroff& ti, const int v) const;
    bool getischordlow(const eventmap_it& i, const offgroff& ti, const int v,
                       const rat& n) const;
    bool getischordhigh(const eventmap_it& i, const offgroff& ti, const int v,
                        const rat& n) const;
    void checkoct(eventmap_it e, const offgroff& ti, const int st,
                  const int os);
    int getvoiceinstaff(const noteevbase& no, const offgroff& ti, const int vo,
                        const int st) const;
#ifndef NDEBUGOUT
    void dumpall() const;
#endif
    bool getisfullmeasrest() const {
      READLOCK;
      return RMUT(isfmr);
    }
    int getpartialmeas() const {
      READLOCK;
      return RMUT(icp) % 2;
    }
    int getpartialbarline() const {
      READLOCK;
      return RMUT(icp) > 2 ? RMUT(icp) - 2 : 0;
    }
    percinstr_str* findpercinst(const char* name) const {
      return prt->findpercinst(name);
    }
    const std::vector<std::pair<rat, rat>>& getkeysigvect() const;
    void checkrestvoices();
    numb getendoff() const {
      return RMUT(off).off + RMUT(dur);
    }
    void insertfiller() {
      /*return insertnewret(new restev(RMUT(off).off, module_none, getdur(), 1,
       * 1, filepos("(internal)")));*/
      assert(isvalid());
      insertnew(new restev(RMUT(off).off, module_none, getdur(), 1, 1,
                           filepos("(internal)"), 0));
      DBG("inserting filler in measure " << this << std::endl);
    }
    void deletefiller();
    struct modout_keysig getkeysig() const {
      return keysigcache;
    }
    const boost::ptr_vector<userkeysigent>*
    getkeysig_aux(const std::vector<std::pair<rat, rat>>*& vect,
                  std::string& ksname) const;
    bool isthemode(const std::string& mode, const int id) const;
    int getconnbeamsleft(eventmap_constit i, const offgroff& ti, const int vo,
                         const int bl) const;
    int getconnbeamsright(eventmap_constit i, const offgroff& ti, const int vo,
                          const int br) const;
    void assignnewkeysig(const char* str) {
      UPREADLOCK;
      if (!RMUT(newkey).empty())
        return;
      TOWRITELOCK;
      WMUT(newkey) = str;
      getkeysig_init();
    }
    std::vector<struct module_keysigref> keysigref;
    void getkeysig_init();
    const std::vector<std::pair<rat, rat>>& getkeysig_init_aux();
    const struct module_keysigref* getkeysigref() const {
      assert(keysigref.size() == 75);
      return &keysigref[0];
    }
    struct module_keysigref getkeysigacc(const int note) const {
      return keysigref[todiatonic(note)];
    }
    void postspecial();
    MUTCHECK2(boost::ptr_multimap<const numb, noteevbase>) newevents;
    void inserttmpevs() {
      for (boost::ptr_multimap<const numb, noteevbase>::iterator i(
               RMUT(newevents).begin());
           i != RMUT(newevents).end();)
        insertnew(WMUT(newevents).release(i++).release());
    }
    void assigndetmark0(numb off0, const int voice, const int type,
                        const char* arg1, const struct module_value& arg2);
    void dopostmarkevs();
    void getrestmarkstaffclef(restev& ev, UPLOCKPAR);
    void setbarline(const module_barlines bl) {
      CMUT(barlin) = bl;
    }
    module_barlines getbarline() const {
      return CMUT(barlin);
    }
    void reinsert();
    void inserttmp(noteevbase* nt) {
      prt->insertnew(nt);
    } // fresh new note events
    bool getoctavebegin(eventmap_it ev, const offgroff& ti, const int st) const;
    bool getoctaveend(eventmap_it ev, const offgroff& ti, const int st) const;
    // void callpost_apisetvalue(const noteevbase* x);
    bool fixmeascheck() const {
      return isexpof2(
          numtorat(get_ival(COMP_ID) ? RMUT(dur) * (fint) 3 : RMUT(dur))
              .denominator());
    }
    void dosplit(part& prt, rat p);
  };
  inline bool
  noteevbase::getoctavebegin() { // octaves are unique within ea. chord + staffn
    READLOCK;
    if (!RMUT(octsign))
      return false;
    offgroff ti(getfulltime_nomut());
    int st = get1staff();
    return UNLOCKP(meas)->getoctavebegin(self, ti, st);
  }
  inline bool noteevbase::getoctaveend() {
    READLOCK;
    if (!RMUT(octsign))
      return false;
    offgroff ti(getfulltime_nomut());
    int st = get1staff();
    return UNLOCKP(meas)->getoctaveend(self, ti, st);
  }
  inline struct module_keysigref noteev::getkeysigacc() const {
    return meas->getkeysigacc(numtoint(getwrittennote()));
  }
  inline int noteev::getconnbeamsleft() const {
    READLOCK;
    offgroff ti(getfulltime_nomut());
    int vo = get1voice();
    int bl(RMUT(beaml));
    return UNLOCKP(meas)->getconnbeamsleft(self, ti, vo, bl);
  }
  inline int noteev::getconnbeamsright() const {
    READLOCK;
    offgroff ti(getfulltime_nomut());
    int vo = get1voice();
    int br(RMUT(beamr));
    return UNLOCKP(meas)->getconnbeamsright(self, ti, vo, br);
  }
  inline void noteev::newinvnote(const fomus_rat& note, const fomus_rat& acc1,
                                 const fomus_rat& acc2,
                                 const special_markslist& marks) {
    noteev* x;
    {
      READLOCK;
      x = new noteev(*this, note, acc1, acc2, marks);
    }
    meas->inserttmp(x);
  }
  inline modobjbase* event::getmeasobj() {
    return meas;
  }
  inline modobjbase* event::getpartobj() {
    return meas->getpartobj();
  }
  inline modobjbase* event::getinstobj() {
    return meas->getinstobj();
  }
  inline bool operator<(const measure& x, const measure& y) {
    return (const offbase&) x < (const offbase&) y;
  }
  inline struct module_intslist noteevbase::getclefs() {
    READLOCK;
    if (!clefscache.get())
      return UNLOCKP(meas)->getclefs();
    module_intslist l = {clefscache->size(), &(*clefscache.get())[0]};
    return l;
  }
  inline struct module_intslist noteevbase::getclefs(const int st) {
    READLOCK;
    if (!clefscache.get())
      return UNLOCKP(meas)->getclefs(st);
    module_intslist l = {clefscache->size(), &(*clefscache.get())[0]};
    return l;
  }
  inline bool noteevbase::gethasclef(const int clef) {
    READLOCK;
    return clefscache.get() ? std::binary_search(clefscache->begin(),
                                                 clefscache->end(), clef)
                            : UNLOCKP(meas)->gethasclef(clef);
  }
  inline bool noteevbase::gethasclef(const int clef, const int staff) {
    READLOCK;
    return clefscache.get() ? std::binary_search(clefscache->begin(),
                                                 clefscache->end(), clef)
                            : UNLOCKP(meas)->gethasclef(clef, staff);
  }
  inline noteevbase* noteevbase::releaseme() {
    assert(meas->isvalid());
#ifndef NDEBUGOUT
    bool xx = false;
    for (eventmap_constit i(meas->getevents().begin());
         i != meas->getevents().end(); ++i) {
      if (i == self) {
        xx = true;
        break;
      }
    }
    DBG("It's " << xx << std::endl);
#endif
    return meas->getevents().release(self).release();
  }
  inline fomus_rat noteevbase::getfullwrittenacc() const {
    READLOCK;
    return meas->getfullwritaccaux(*this, LOCKARG);
  }
  inline fomus_rat noteevbase::getwrittenacc1() const {
    READLOCK;
    return meas->getwritacc1aux(*this, LOCKARG);
  }
  inline fomus_rat noteevbase::getwrittenacc2() const {
    READLOCK;
    return meas->getwritacc2aux(*this, LOCKARG);
  }
  inline int noteevbase::getvoiceinstaff() const {
    READLOCK;
    if (isinvisible_nomut())
      return 0;
    offgroff ti(getfulltime_nomut());
    int vo(get1voice());
    int st(get1staff());
    return UNLOCKP(meas)->getvoiceinstaff(*this, ti, vo, st);
  }
  inline bool noteev::getisbeginchord() const {
    READLOCK;
    offgroff ti(getfulltime_nomut());
    int v(get1voice());
    return UNLOCKP(meas)->getisbeginchord(self, ti, v);
  }
  inline bool noteev::getisendchord() const {
    READLOCK;
    offgroff ti(getfulltime_nomut());
    int v(get1voice());
    return UNLOCKP(meas)->getisendchord(self, ti, v);
  }
  inline bool noteev::getischordlow() const {
    READLOCK;
    offgroff ti(getfulltime_nomut());
    int v(get1voice());
    rat n(numtorat(getnote_nomut()));
    return UNLOCKP(meas)->getischordlow(self, ti, v, n);
  }
  inline bool noteev::getischordhigh() const {
    READLOCK;
    offgroff ti(getfulltime_nomut());
    int v(get1voice());
    rat n(numtorat(getnote_nomut()));
    return UNLOCKP(meas)->getischordhigh(self, ti, v, n);
  }
  inline void noteev::checkstaves() {
    bool x;
    {
      READLOCK;
      x = tiedbase::islasttied();
    }
    if (x)
      postassignstaves();
  }
  inline void noteev::checkvoices() {
    bool x;
    {
      READLOCK;
      x = (!getisperc_nomut() && tiedbase::islasttied());
    }
    if (x)
      postassignvoices();
    post_apisetvalue();
  }
  inline void noteev::replaceclef(const int cl, UPLOCKPAR) {
    {
      TOWRITELOCK;
      WMUT(clef) = cl;
      WMUT(clf) = meas->getclefptr(get1staff(), RMUT(clef));
    }
    if (RMUT(tiedr))
      UNLOCKP((noteev*) RMUT(tiedr))->replaceclef0(RMUT(clef));
  }
  inline void noteev::replaceclef0(const int cl) {
    UPREADLOCK;
#ifndef NDEBUG
    replaceclef(cl, xxx, zzz000);
#else
    replaceclef(cl, xxx);
#endif
  }
  // inline bool noteev::maybereplaceclef(const int cl) {return (cl !=
  // RMUT(clef));}
  inline module_percinstobj noteev::getpercinst() const { // find it!
    READLOCK;
    if (RMUT(perc))
      return (modobjbase*) RMUT(perc);
    if (RMUT(percname)) {
      const char* na = RMUT(percname);
      return (modobjbase*) UNLOCKP(meas)->findpercinst(na);
    }
    return 0;
  }
  inline void noteevbase::checkprune() {
    if (gettime() < meas->gettime() || getendtime() > meas->getendtime()) {
      CERR << "invalid offset/duration";
      integerr("prune");
    }
  }
  inline fint event::partindex() const {
    return meas->partindex();
  }
  inline void noteevbase::checkoct() {
    {
      READLOCK;
      if (RMUT(octsign) < -1 || RMUT(octsign) > 2) {
        CERR << "invalid octave sign";
        integerr("octave sign"); // found during octave sign integrity check" <<
                                 // std::endl;
      }
      offgroff ti(getfulltime_nomut());
      int st = get1staff();
      int os = RMUT(octsign);
      UNLOCKP(meas)->checkoct(
          self, ti, st,
          os); // make sure all notes in chord/staff are same
    }
    post_apisetvalue();
  }

  // holds the notes & objects
  class part _NONCOPYABLE {
    boost::shared_mutex mut; //, cachemut;
protected:
    partormpart_str* def; // part, mpart or partsref (partsref is temporary,
                          // resolved after output is written) -- should always
                          // be valid and in scorepartslist
    MUTCHECK(measmap) meass, newmeass;
    MUTCHECK2(std::multimap<int, parts_grouptype>) bgroups;
    MUTCHECK(std::set<int>) egroups;
    MUTCHECK(boost::ptr_list<noteevbase>) tmpevs;
    MUTCHECK(boost::ptr_vector<modobjbase>) markevs;
    MUTCHECK(boost::ptr_vector<modobjbase>) tmpmarkevs;
    MUTCHECK(bool) tmark;
#ifndef NDEBUG
    int valid;
#endif
public:
    part(partormpart_str* def, boost::shared_ptr<measdef_str>& ms)
        : def(def) _MUTINIT(meass) _MUTINIT(newmeass) _MUTINIT(bgroups)
              _MUTINIT(egroups) _MUTINIT(tmpevs) _MUTINIT(markevs)
                  _MUTINIT(tmpmarkevs),
          tmark(false _MUT) {
#ifndef NDEBUG
      valid = 12345;
#endif
      assert(false);
      measure* m = new measure(ms, *def);
      m->setself(CMUT(meass).insert(offgroff((fint) 0), m));
    }
    part(partormpart_str* def)
        : def(def) _MUTINIT(meass) _MUTINIT(newmeass) _MUTINIT(bgroups)
              _MUTINIT(egroups) _MUTINIT(tmpevs) _MUTINIT(markevs)
                  _MUTINIT(tmpmarkevs),
          tmark(false _MUT) { // for tmppart only
#ifndef NDEBUG
      valid = 12345;
#endif
    }
    part(part& x, partormpart_str* def, const numb& shift)
        : def(def) _MUTINIT(meass) _MUTINIT(newmeass) _MUTINIT(bgroups)
              _MUTINIT(egroups) _MUTINIT(tmpevs) _MUTINIT(markevs)
                  _MUTINIT(tmpmarkevs),
          tmark(false _MUT) { // cloning
#ifndef NDEBUG
      valid = 12345;
#endif
      mergefrom(x, shift);
    }
    void insdefmeas(boost::shared_ptr<measdef_str>& ms) {
      measure* m = new measure(ms, *def);
      m->setself(XMUT(meass).insert(offgroff((fint) 0), m));
    }
    void insertnew(noteevbase* nt) {
      WRITELOCK;
      assert(!CMUT(meass).empty());
      WMUT(tmpevs).push_back(nt);
    } // fresh new note events
    part* fomclone(partormpart_str* def0, const numb& shift) {
      assert(isvalid());
      return new part(*this, def0, shift);
    }
#ifndef NDEBUG
    bool isvalid() const {
      return valid == 12345;
    }
#endif
    bool tmpevsempty() const {
      READLOCK;
      return RMUT(tmpevs).empty();
    }
    bool alltmpevsempty() const { /*READLOCK;*/
      return CMUT(tmpevs).empty() && CMUT(markevs).empty();
    } // called before running
    int maxbgrouplvl() const {
      return RMUT(bgroups).empty() ? -1
                                   : boost::prior(RMUT(bgroups).end())->first;
    }
    int maxegrouplvl() const {
      return RMUT(egroups).empty() ? -1 : *boost::prior(RMUT(egroups).end());
    }
    void insbgroups(const int lvl, const parts_grouptype typ) {
      WMUT(bgroups).insert(
          std::multimap<int, parts_grouptype>::value_type(lvl, typ));
    }
    void insegroups(const int lvl) {
      WMUT(egroups).insert(lvl);
    }
    void postinput(std::vector<makemeas>& makemeass, const numb& trun1,
                   const numb& trun2);
    void postinput2(const numb& trun1 = (fint) -1,
                    const numb& trun2 = (fint) -1);
    void postinput3();
    const measmap& getmeass() const {
      return CMUT(meass);
    }
    measmap& getmeass() {
      return CMUT(meass);
    }
    measmap& getnewmeass() {
      return RMUT(newmeass);
    }
    void insertnewmeas(const offgroff& o, measure* m) {
      WRITELOCK;
      WMUT(newmeass).insert(o, m);
    }
    void insertnewmeas_nomut(const offgroff& o, measure* m) {
      WMUT(newmeass).insert(o, m);
    }
    void fixmeasures();
    void fixtimequantinv() {
      numb mx((fint) -1);
      for (measmap_it m(CMUT(meass).begin()); m != CMUT(meass).end(); ++m)
        m->second->fixtimequant(mx, true);
    }
    numb fixtimequant();
    void reinsert(std::auto_ptr<noteevbase>& e, const char* what);
    void trimmeasures(const fomus_rat& n);
    void assigngroupbegin(const int grpcnt, const parts_grouptype type) {
      WRITELOCK;
      WMUT(bgroups).insert(
          std::map<int, parts_grouptype>::value_type(grpcnt, type));
    }
    void settmark() {
      WMUT(tmark) = true;
    }
    void settmark(const bool m) {
      if (m) {
        WRITELOCK;
        WMUT(tmark) = true;
      }
    }
    void assigngroupend(const fint grpcnt, const parts_grouptype type) {
      WRITELOCK;
      WMUT(bgroups).insert(std::map<int, parts_grouptype>::value_type(
          grpcnt, type)); // yes, it's supposed to be bgroups
    }
#ifndef NDEBUG
    void resetstage();
#endif
#ifndef NDEBUGOUT
    void showstage();
#endif
    std::multimap<int, parts_grouptype>& getbgroups() {
      return RMUT(bgroups);
    }
    std::set<int>& getegroups() {
      return RMUT(egroups);
    }
    void collectallvoices(std::set<int>& vv);
    void collectallstaves(std::set<int>& ss);
    std::vector<int> voicescache; // parts/measures don't get copied/split--can
                                  // put vector right inside the class
    const std::vector<int>& getvoicescache() const {
      return voicescache;
    }
    struct module_intslist getvoices() { // remove the const! part
      module_intslist r = {voicescache.size(), &voicescache[0]};
      return r;
    }
    std::vector<int> stavescache;
    const std::vector<int>& getstavescache() const {
      return stavescache;
    }
    struct module_intslist getstaves() { // remove the const! part
      module_intslist r = {stavescache.size(), &stavescache[0]};
      return r;
    }
    const char* getcid() const {
      return def->getcid();
    }
    std::string getid() const {
      return def->getid();
    }
    const char* getprintstr() const {
      return def->getprintstr();
    }
    void filltmppart(measmapview& m);
    enum parts_grouptype partgroupbegin(const int lvl) const {
      READLOCK;
#ifndef NDEBUGOUT
      for (std::multimap<int, parts_grouptype>::const_iterator i(
               RMUT(bgroups).begin());
           i != RMUT(bgroups).end(); ++i) {
        DBG("BGROUP LVL " << i->first << std::endl);
      }
#endif
      std::multimap<int, parts_grouptype>::const_iterator i(
          RMUT(bgroups).find(lvl));
      return i == RMUT(bgroups).end() ? parts_nogroup : i->second;
    }
    bool partgroupend(const int lvl) const {
      READLOCK;
      return RMUT(egroups).find(lvl) != RMUT(egroups).end();
    }
    void sortord(fomus_int& i0) {
      for (measmap_it i(CMUT(meass).begin()); i != CMUT(meass).end(); ++i)
        i->second->sortord(i0);
    }
    void fillholes1() {
      for (measmap_it i(CMUT(meass).begin()); i != CMUT(meass).end(); ++i)
        i->second->inserttmpevs();
    }
    void fillholes2() {
      for (measmap_it i(CMUT(meass).begin()); i != CMUT(meass).end(); ++i)
        i->second->fillholes2();
    }
    void fillholes3() {
      for (measmap_it i(CMUT(meass).begin()); i != CMUT(meass).end(); ++i)
        i->second->fillholes3();
    }
    void preprocess(const numb& te, const std::string& ts);
#ifndef NDEBUGOUT
    void dumpall() const;
#endif
    void assignmpart(std::auto_ptr<noteevbase>& ev) {
      assert(isvalid());
      DBG("INSERTING NOTE INTO ... " << this << std::endl);
      WRITELOCK;
      WMUT(tmpevs).push_back(ev.release());
      DBG("size of tmpevs = " << CMUT(tmpevs).size() << std::endl);
    }
    void assignmpartmarkev(std::auto_ptr<markev>& ev) {
      assert(isvalid());
      DBG("INSERTING MARKEV INTO ... " << this << std::endl);
      WRITELOCK;
      WMUT(tmpmarkevs).push_back(ev.release());
      DBG("size of markevs = " << CMUT(tmpevs).size() << std::endl);
    }
    numb getlastmeasendoff() const {
      return boost::prior(CMUT(meass).end())->second->getendoff();
    }
    void insertnewmarkev(markev* m) {
      assert(isvalid());
      XMUT(markevs).push_back(m);
    }
    module_objlist getmarkevslist() {
      module_objlist r = {CMUT(markevs).size(),
                          (void**) CMUT(markevs).c_array()};
      return r;
    }
    void /*eventmap_it*/ insertfiller() {
      assert(!CMUT(meass).empty()); /*return*/
      (*CMUT(meass).begin())->second->insertfiller();
    }
    void deletefiller(/*eventmap_it& it*/) {
      assert(!CMUT(meass).empty());
      (*CMUT(meass).begin())->second->deletefiller(/*it*/);
    }
    void clearallnotes() {
      WRITELOCK;
      WMUT(tmpevs).clear();
      XMUT(markevs).clear();
    }
    void assigndetmark(const numb& off, const int voice, const int type,
                       const char* arg1, const struct module_value& arg2) {
      measmap::iterator i(CMUT(meass).upper_bound(offgroff(off)));
      (i == CMUT(meass).begin() ? i : boost::prior(i))
          ->second->assigndetmark0(off, voice, type, arg1, arg2);
    }
    void inserttmps();
    bool getoctavebegin(measmap_it me, const offgroff& ti, const int st) const;
    bool getoctaveend(measmap_it me, const offgroff& ti, const int st) const;
    bool gettmark() const {
      READLOCK;
      return RMUT(tmark);
    }
    void reinserttmps();
    void mergefrom(part& x, const numb& shift);
    // void postmeas();
    partormpart_str* getdef() const {
      return def;
    }
  };
  inline void partormpart_str::mergefrom(part& x, const numb& shift) {
    prt->mergefrom(x, shift);
  }
  inline void partormpart_str::reinserttmps() {
    prt->reinserttmps();
  }
  // inline void partormpart_str::postmeas() {prt->postmeas();}
  inline void partormpart_str::insertnewmeas(const offgroff& o, measure* m) {
    prt->insertnewmeas(o, m);
  }
  inline void partormpart_str::assignmpartmarkev(std::auto_ptr<markev>& ev) {
    prt->assignmpartmarkev(ev);
  }
  inline bool partormpart_str::gettmark() const {
    return prt->gettmark();
  }
  inline void partormpart_str::settmark(const bool tmark) {
    prt->settmark(tmark);
  }
  inline void partormpart_str::settmark() {
    prt->settmark();
  }
  inline bool partormpart_str::getoctavebegin(const measmap_it& me,
                                              const offgroff& ti,
                                              const int st) const {
    return prt->getoctavebegin(me, ti, st);
  }
  inline bool partormpart_str::getoctaveend(const measmap_it& me,
                                            const offgroff& ti,
                                            const int st) const {
    return prt->getoctaveend(me, ti, st);
  }
  inline void partormpart_str::inserttmps() {
    prt->inserttmps();
  }
  inline void partormpart_str::clearallnotes() {
    prt->clearallnotes();
  }
  inline void partormpart_str::insertnew(noteevbase* ev) {
    prt->insertnew(ev);
  }
  inline void partormpart_str::insertnewmarkev(markev* ev) {
    prt->insertnewmarkev(ev);
  }
  inline void /*eventmap_it*/ partormpart_str::insertfiller() { /*return*/
    prt->insertfiller();
  }
  inline void /*eventmap_it*/
  partormpart_str::preprocess(const numb& te,
                              const std::string& ts) { /*return*/
    prt->preprocess(te, ts);
  }
  inline void partormpart_str::deletefiller(/*eventmap_it& it*/) {
    prt->deletefiller(/*it*/);
  }
  inline module_objlist partormpart_str::getmarkevslist() {
    return prt->getmarkevslist();
  }
  inline numb partormpart_str::getlastmeasendoff() const {
    return prt->getlastmeasendoff();
  }
  inline enum parts_grouptype
  partormpart_str::partgroupbegin(const int lvl) const {
    return prt->partgroupbegin(lvl);
  }
  inline bool partormpart_str::partgroupend(const int lvl) const {
    return prt->partgroupend(lvl);
  }
  inline void partormpart_str::filltmppart(measmapview& m) const {
    prt->filltmppart(m);
  }
  inline struct module_intslist measure::getpartclefs() const {
    return prt->getclefs();
  }
  inline int measure::getpartnstaves() const {
    return prt->getnstaves();
  }
  inline const std::string& measure::getpartid() const {
    return prt->getid();
  }
  inline void measure::assignmeas(const fomus_rat& time, const fomus_rat& dur,
                                  const bool rmable) {
    assert(prt);
    prt->insertnewmeas(
        offgroff(time),
        new measure(numb(time), numb(dur), measdefptr(), prt, rmable, this));
  }
  inline void partormpart_str::fixmeasures() {
    prt->fixmeasures();
  }
  inline void partormpart_str::fixtimequantinv() {
    return prt->fixtimequantinv();
  }
  inline numb partormpart_str::fixtimequant() {
    return prt->fixtimequant();
  }
  inline void partormpart_str::reinsert(std::auto_ptr<noteevbase>& e,
                                        const char* what) {
    prt->reinsert(e, what);
  }
  inline void partormpart_str::trimmeasures(const fomus_rat& n) {
    prt->trimmeasures(n);
  }
  inline void partormpart_str::collectallvoices(std::set<int>& v) {
    prt->collectallvoices(v);
  }
  inline void partormpart_str::collectallstaves(std::set<int>& s) {
    prt->collectallstaves(s);
  }
#ifndef NDEBUG
  inline void partormpart_str::resetstage() {
    prt->resetstage();
  }
#endif
#ifndef NDEBUGOUT
  inline void partormpart_str::showstage() {
    prt->showstage();
  }
#endif
  inline struct module_intslist partormpart_str::getvoices() {
    return prt->getvoices();
  }
  inline struct module_intslist partormpart_str::getstaves() {
    return prt->getstaves();
  }
  inline partormpart_str::partormpart_str(const partormpart_str& x,
                                          const numb& shift)
      : str_base(x), prt(x.prt->fomclone(this, shift)), ind(x.ind),
        insord(std::numeric_limits<int>::max()) {} // fomcloning
  inline void partormpart_str::sortord(fomus_int& i) {
    prt->sortord(i);
  }
  inline void partormpart_str::fillholes1() {
    prt->fillholes1();
  }
  inline void partormpart_str::fillholes2() {
    prt->fillholes2();
  }
  inline void partormpart_str::fillholes3() {
    prt->fillholes3();
  }
  inline partormpart_str::partormpart_str()
      : str_base(), prt(new part(this)),
        insord(std::numeric_limits<int>::max()) {}
  inline partormpart_str::partormpart_str(const partormpart_str& x)
      : str_base(x), prt(new part(this)),
        insord(std::numeric_limits<int>::max()) {}
  inline void partormpart_str::assigndetmark(const numb& off, const int voice,
                                             const int type, const char* arg1,
                                             const struct module_value& arg2) {
    prt->assigndetmark(off, voice, type, arg1, arg2);
  }
#ifndef NDEBUGOUT
  inline void partormpart_str::dumpall() const {
    prt->dumpall();
  }
#endif
  inline const std::vector<int>& partormpart_str::getvoicescache() const {
    return prt->getvoicescache();
  }
  inline const std::vector<int>& partormpart_str::getstavescache() const {
    return prt->getstavescache();
  }
  inline void partormpart_str::assignmpart(std::auto_ptr<noteevbase>& pa) {
    prt->assignmpart(pa);
  }
  inline void partormpart_str::postinput3() {
    prt->postinput3();
  }
  inline void partormpart_str::insdefmeas(boost::shared_ptr<measdef_str>& ms) {
    prt->insdefmeas(ms);
  }
  inline void partormpart_str::insbgroups(const int lvl,
                                          const parts_grouptype typ) {
    prt->insbgroups(lvl, typ);
  }
  inline void partormpart_str::insegroups(const int lvl) {
    prt->insegroups(lvl);
  }
  inline void partormpart_str::postinput(std::vector<makemeas>& makemeass,
                                         const int ord, const numb& trun1,
                                         const numb& trun2) {
    insord = ord;
    DBG("SETTING ORD = " << ord << " in " << this << std::endl);
    DBG("POSTINPUT: " << getid() << std::endl);
    prt->postinput(makemeass, trun1, trun2);
  }

  inline const varbase& measure::get_varbase(const int id) const {
    setmap_constit i(attrs->sets.find(id));
    return (i != attrs->sets.end()) ? *i->second : prt->get_varbase(id);
  }
  inline fint measure::get_ival(const int id) const {
    setmap_constit i(attrs->sets.find(id));
    return (i != attrs->sets.end()) ? i->second->getival() : prt->get_ival(id);
  }
  inline rat measure::get_rval(const int id) const {
    setmap_constit i(attrs->sets.find(id));
    return (i != attrs->sets.end()) ? i->second->getrval() : prt->get_rval(id);
  }
  inline ffloat measure::get_fval(const int id) const {
    setmap_constit i(attrs->sets.find(id));
    return (i != attrs->sets.end()) ? i->second->getfval() : prt->get_fval(id);
  }
  inline const std::string& measure::get_sval(const int id) const {
    setmap_constit i(attrs->sets.find(id));
    return (i != attrs->sets.end()) ? i->second->getsval() : prt->get_sval(id);
  }
  inline const module_value& measure::get_lval(const int id) const {
    setmap_constit i(attrs->sets.find(id));
    return (i != attrs->sets.end()) ? i->second->getmodval()
                                    : prt->get_lval(id);
  }

  inline const varbase& measure::get_varbase_up(const int id,
                                                const event& ev) const {
    return prt->get_varbase(id, ev);
  }
  inline fint measure::get_ival_up(const int id, const event& ev) const {
    return prt->get_ival(id, ev);
  }
  inline rat measure::get_rval_up(const int id, const event& ev) const {
    return prt->get_rval(id, ev);
  }
  inline ffloat measure::get_fval_up(const int id, const event& ev) const {
    return prt->get_fval(id, ev);
  }
  inline const std::string& measure::get_sval_up(const int id,
                                                 const event& ev) const {
    return prt->get_sval(id, ev);
  }
  inline const module_value& measure::get_lval_up(const int id,
                                                  const event& ev) const {
    return prt->get_lval(id, ev);
  }

  inline bool spanmark::getcantouch(const noteevbase& ev) const {
    if (props & spr_cantouch)
      return true;
    if (props & spr_cannottouch)
      return false;
    assert(cantouchdef);
    return ev.get_ival(cantouchdef);
  }
  inline bool spanmark::getcanspanone(const noteevbase& ev) const {
    if (props & spr_canspanone)
      return true;
    if (props & spr_cannotspanone)
      return false;
    assert(canspanonedef);
    return ev.get_ival(canspanonedef);
  }
  inline bool spanmark::getcanspanrests(const noteevbase& ev) const {
    if (props & spr_canspanrests)
      return true;
    if (props & spr_cannotspanrests)
      return false;
    assert(canspanrestsdef);
    return ev.get_ival(canspanrestsdef);
  }
  inline bool spanmark::getcantouch_nomut(const noteevbase& ev) const {
    if (props & spr_cantouch)
      return true;
    if (props & spr_cannottouch)
      return false;
    assert(cantouchdef);
    return ev.get_ival_nomut(cantouchdef);
  }
  inline bool spanmark::getcanspanone_nomut(const noteevbase& ev) const {
    if (props & spr_canspanone)
      return true;
    if (props & spr_cannotspanone)
      return false;
    assert(canspanonedef);
    return ev.get_ival_nomut(canspanonedef);
  }
  inline bool spanmark::getcanspanrests_nomut(const noteevbase& ev) const {
    if (props & spr_canspanrests)
      return true;
    if (props & spr_cannotspanrests)
      return false;
    assert(canspanrestsdef);
    return ev.get_ival_nomut(canspanrestsdef);
  }

} // namespace fomus

#endif
