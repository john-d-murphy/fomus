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

#ifndef FOMUS_DATA_H
#define FOMUS_DATA_H

#ifndef BUILD_LIBFOMUS
#error "data.h shouldn't be included"
#endif

#include "heads.h"

#include "algext.h" // renew
#include "algext.h"
#include "classes.h" // fintvect, modobjbase
#include "fomusapi.h"
#include "instrs.h"
#include "marks.h"   // markbase
#include "module.h"  // setid_list
#include "numbers.h" // class rat
#include "parse.h"   // nestedlistelstack
#include "vars.h"    // class varbase

namespace fomus {

  extern bool dumping;
  struct scopedumping {
    bool s;
    scopedumping() {
      s = dumping;
      dumping = false;
    }
    ~scopedumping() {
      dumping = s;
    }
  };

  // exceptions
  inline void throwinvalid(const filepos& pos) {
    CERR << "invalid parameter/action";
    pos.printerr();
    throw errbase();
  }
  inline void thrownoid(const char* str, const filepos& pos) {
    CERR << str << " id missing";
    pos.printerr();
    throw errbase();
  }
  inline void throwdupid(const char* str, const filepos& pos) {
    CERR << "duplicate " << str << " id";
    pos.printerr();
    throw errbase();
  }

  inline const markbase& getthemarkdef(const std::string& str,
                                       const filepos& pos) {
    std::map<std::string, markbase*, isiless>::const_iterator i(
        marksbyname.find(str));
    if (i != marksbyname.end())
      return *i->second;
    CERR << "invalid mark `" << str << '\'';
    pos.printerr();
    throw errbase();
  }

  // types
  typedef std::stack<listelvect*> listelstack;

  typedef std::map<const std::string, boost::shared_ptr<instr_str>, isiless>
      definstsmap;
  typedef definstsmap::iterator definstsmap_it;
  typedef definstsmap::const_iterator definstsmap_constit;
  typedef definstsmap::value_type definstsmap_val;

  typedef std::map<const std::string, boost::shared_ptr<percinstr_str>, isiless>
      defpercsmap;
  typedef defpercsmap::iterator defpercsmap_it;
  typedef defpercsmap::const_iterator defpercsmap_constit;
  typedef defpercsmap::value_type defpercsmap_val;

  typedef std::map<const std::string, boost::shared_ptr<part_str>, isiless>
      defpartsmap;
  typedef defpartsmap::iterator defpartsmap_it;
  typedef defpartsmap::const_iterator defpartsmap_constit;
  typedef defpartsmap::value_type defpartsmap_val;

  typedef std::map<const std::string, boost::shared_ptr<mpart_str>, isiless>
      defmpartsmap;
  typedef defmpartsmap::iterator defmpartsmap_it;
  typedef defmpartsmap::const_iterator defmpartsmap_constit;
  typedef defmpartsmap::value_type defmpartsmap_val;

  typedef std::map<const std::string, boost::shared_ptr<measdef_str>, isiless>
      defmeasdefmap;
  typedef defmeasdefmap::iterator defmeasdefmap_it;
  typedef defmeasdefmap::const_iterator defmeasdefmap_constit;
  typedef defmeasdefmap::value_type defmeasdefmap_val;

  typedef std::vector<boost::shared_ptr<varbase>> varcopiesvect;
  typedef varcopiesvect::iterator varcopiesvect_it;
  typedef varcopiesvect::const_iterator varcopiesvect_constit;

  struct dataholderreg;
  typedef boost::ptr_list<dataholderreg> datastack;
  typedef datastack::iterator datastack_it;
  typedef datastack::const_iterator datastack_constit;
  typedef datastack::reverse_iterator datastack_rit;
  typedef datastack::const_reverse_iterator datastack_constrit;

  struct dataholder {
    virtual ~dataholder() {}

    virtual void stickin(fomusdata& fd) = 0;
    numb off;
    virtual void setoff(const numb& val, const datastack& st) = 0;
    virtual void setendtime(const numb& val, const datastack& st,
                            const filepos& pos) = 0;
    virtual void incoff(const numb& val, const datastack& st) = 0;
    virtual void decoff(const numb& val, const datastack& st) = 0;
    virtual void multoff(const numb& val, const datastack& st,
                         const filepos& pos) = 0;
    virtual void divoff(const numb& val, const datastack& st,
                        const filepos& pos) = 0;
    virtual void incoffplus(const datastack& st, const filepos& pos) = 0;
    virtual void incgroffplus(const datastack& st, const filepos& pos) = 0;
    numb groff, groffbak;
    virtual void setgroff(const numb& val, const datastack& st) = 0;
    virtual void incgroff(const numb& val, const datastack& st) = 0;
    virtual void decgroff(const numb& val, const datastack& st) = 0;
    virtual void multgroff(const numb& val, const datastack& st,
                           const filepos& pos) = 0;
    virtual void divgroff(const numb& val, const datastack& st,
                          const filepos& pos) = 0;
    numb dur;
    virtual void setdur(const numb& val, const datastack& st) = 0;
    virtual void incdur(const numb& val, const datastack& st) = 0;
    virtual void decdur(const numb& val, const datastack& st) = 0;
    virtual void multdur(const numb& val, const datastack& st) = 0;
    virtual void divdur(const numb& val, const datastack& st) = 0;
    virtual void setpoint(const pointtype, const filepos& pos) = 0;
    numb pitch;
    virtual void setpitch(const numb& val, const datastack& st) = 0;
    virtual void setpitch(const char* val, const filepos& pos) = 0;
    virtual void incpitch(const numb& val, const datastack& st) = 0;
    virtual void decpitch(const numb& val, const datastack& st) = 0;
    virtual void multpitch(const numb& val, const datastack& st,
                           const filepos& pos) = 0;
    virtual void divpitch(const numb& val, const datastack& st,
                          const filepos& pos) = 0;
    numb dyn;
    virtual void setdyn(const numb& val, const datastack& st) = 0;
    virtual void incdyn(const numb& val, const datastack& st) = 0;
    virtual void decdyn(const numb& val, const datastack& st) = 0;
    virtual void multdyn(const numb& val) = 0;
    virtual void divdyn(const numb& val) = 0;
    std::set<int> voices;
    virtual void setvoices(const numb& val) = 0;
    virtual void addvoices(const numb& val) = 0;
    virtual void decvoices(const numb& val, const filepos& pos) = 0;
    virtual void clearvoices() = 0;
    virtual void setvoiceslist(const listelvect& lst) = 0;
    virtual void addvoiceslist(const listelvect& lst) = 0;
    virtual void decvoiceslist(const listelvect& lst, const filepos& pos) = 0;
    virtual void setvoiceslisttype(const fomus_action act,
                                   const filepos& pos) = 0;
    virtual bool isnorm() const = 0;
    boost::ptr_set<markobj> marks;
    virtual void addmarks(std::auto_ptr<markobj>& obj) = 0;
    virtual void decmarks(std::auto_ptr<markobj>& obj) = 0;
    setmap sets;
    void addset(const varbase* vb) {
      sets.erase(vb->getid());
      sets.insert(setmap::value_type(vb->getid(),
                                     boost::shared_ptr<const varbase>(vb)));
    }

    dataholder()
        : off(module_none), groff(module_none), groffbak(module_none),
          dur(module_none), pitch(module_none), dyn((fint) 0) {}
    dataholder(int)
        : off((fomus_int) 0), groff(module_none), groffbak(module_none),
          dur(module_none), pitch(module_none), dyn((fint) 0) {}
    dataholder(const datastack& stack);
    void checktimenumbs(const filepos& pos) const;
    void checkdurnumb(const filepos& pos) const;
    numb checkmdurnumb(const filepos& pos) const;
  };
  inline void checkvoice(const int x, const filepos& pos) {
    if (x <= 0 || x > 128) {
      CERR << "expected voice value of type `integer1..128'";
      pos.printerr();
      throw errbase();
    }
  }
  inline void checkstaff(const int x, const filepos& pos) {
    if (x <= 0 || x > 128) {
      CERR << "expected staff value of type `integer1..128'";
      pos.printerr();
      throw errbase();
    }
  }
  struct dataholderreg : public dataholder { // "region" holder (+, -, etc.)
    enum fomus_action offact;
    void setoff(const numb& val, const datastack& st) {
      off = val;
      offact = fomus_act_inc;
    } // base offset
    void setendtime(const numb& val, const datastack& st, const filepos& pos) {
      CERR << "cannot create a `duration' region with an endtime";
      pos.printerr();
      throw errbase();
    }
    void incoff(const numb& val, const datastack& st) {
      off = val;
      offact = fomus_act_inc;
    }
    void decoff(const numb& val, const datastack& st) {
      off = val;
      offact = fomus_act_dec;
    }
    void multoff(const numb& val, const datastack& st, const filepos& pos) {
      off = val;
      offact = fomus_act_mult;
    }
    void divoff(const numb& val, const datastack& st, const filepos& pos) {
      assert(val != (fint) 0);
      off = val;
      offact = fomus_act_div;
    }
    void modoff(numb& val) const {
      switch (offact) {
      case fomus_act_inc:
        val = val + off.modval();
        break;
      case fomus_act_dec:
        val = val - off.modval();
        break;
      case fomus_act_mult:
        val = val * off.modval();
        break;
      case fomus_act_div:
        val = val / off.modval();
        break;
      default:;
      }
    }
    void modoffmult(numb& val)
        const { // filters out + and - for incrementing in a scaled region
      switch (offact) {
      case fomus_act_mult:
        val = val * off.modval();
        break;
      case fomus_act_div:
        val = val / off.modval();
        break;
      default:;
      }
    }
    void incoffplus(const datastack& st, const filepos& pos) {
      CERR << "cannot create an `increment by last duration' region";
      pos.printerr();
      throw errbase();
    } // set increment to arg, which is norm's previous off0
    enum fomus_action groffact;
    void cleargr() {
      groffact = fomus_act_clear;
      groffbak = groff;
      groff.null();
    }
    void setgroff(const numb& val, const datastack& st) {
      groff = val;
      groffact = fomus_act_inc;
    }
    void incgroff(const numb& val, const datastack& st) {
      groff = val;
      groffact = fomus_act_inc;
    }
    void decgroff(const numb& val, const datastack& st) {
      groff = val;
      groffact = fomus_act_dec;
    }
    void multgroff(const numb& val, const datastack& st, const filepos& pos) {
      groff = val;
      groffact = fomus_act_mult;
    }
    void divgroff(const numb& val, const datastack& st, const filepos& pos) {
      assert(val != (fint) 0);
      groff = val;
      groffact = fomus_act_div;
    }
    void modgroff(numb& val) const {
      switch (groffact) {
      case fomus_act_inc:
        val = val + groff.modval();
        break;
      case fomus_act_dec:
        val = val - groff.modval();
        break;
      case fomus_act_mult:
        val = val * groff.modval();
        break;
      case fomus_act_div:
        val = val / groff.modval();
        break;
      default:;
      }
    }
    void modgroffmult(numb& val) const {
      switch (groffact) {
      case fomus_act_mult:
        val = val * groff.modval();
        break;
      case fomus_act_div:
        val = val / groff.modval();
        break;
      default:;
      }
    }
    void incgroffplus(const datastack& st, const filepos& pos) {
      CERR << "cannot create an `increment by last grace duration' region";
      pos.printerr();
      throw errbase();
    }
    enum fomus_action duract;
    void setdur(const numb& val, const datastack& st) {
      dur = val;
      duract = fomus_act_mult;
    } // base dur
    void incdur(const numb& val, const datastack& st) {
      dur = val;
      duract = fomus_act_inc;
    }
    void decdur(const numb& val, const datastack& st) {
      dur = val;
      duract = fomus_act_dec;
    }
    void multdur(const numb& val, const datastack& st) {
      dur = val;
      duract = fomus_act_mult;
    }
    void divdur(const numb& val, const datastack& st) {
      assert(val != (fint) 0);
      dur = val;
      duract = fomus_act_div;
    }
    void moddur(numb& val) const {
      switch (duract) {
      case fomus_act_inc:
        val = val + dur.modval();
        break;
      case fomus_act_dec:
        val = val - dur.modval();
        break;
      case fomus_act_mult:
        val = val * dur.modval();
        break;
      case fomus_act_div:
        val = val / dur.modval();
        break;
      default:;
      }
    }
    void moddurmult(numb& val) const {
      switch (duract) {
      case fomus_act_mult:
        val = val * dur.modval();
        break;
      case fomus_act_div:
        val = val / dur.modval();
        break;
      default:;
      }
    }
    void setpoint(const pointtype, const filepos& pos) {
      CERR << "cannot create a `duration' region with an automatic duration";
      pos.printerr();
      throw errbase();
    }
    enum fomus_action pitchact;
    void setpitch(const numb& val, const datastack& st) {
      pitch = val;
      pitchact = fomus_act_inc;
    }
    void incpitch(const numb& val, const datastack& st) {
      pitch = val;
      pitchact = fomus_act_inc;
    }
    void decpitch(const numb& val, const datastack& st) {
      pitch = val;
      pitchact = fomus_act_dec;
    }
    void multpitch(const numb& val, const datastack& st, const filepos& pos) {
      CERR << "cannot create a `multiply pitch' region";
      pos.printerr();
      throw errbase();
    }
    void divpitch(const numb& val, const datastack& st, const filepos& pos) {
      CERR << "cannot create a `divide pitch' region";
      pos.printerr();
      throw errbase();
    }
    void setpitch(const char* val, const filepos& pos) {
      CERR << "cannot create a `percussion pitch' region";
      pos.printerr();
      throw errbase();
    }
    void modpitch(numb& val) const {
      switch (pitchact) {
      case fomus_act_inc:
        val = val + pitch.modval();
        break;
      case fomus_act_dec:
        val = val - pitch.modval();
        break;
      case fomus_act_mult:
        val = val * pitch.modval();
        break;
      case fomus_act_div:
        val = val / pitch.modval();
        break;
      default:;
      }
    }
    enum fomus_action dynact;
    void setdyn(const numb& val, const datastack& st) {
      dyn = val;
      dynact = fomus_act_inc;
    }
    void incdyn(const numb& val, const datastack& st) {
      dyn = val;
      dynact = fomus_act_inc;
    }
    void decdyn(const numb& val, const datastack& st) {
      dyn = val;
      dynact = fomus_act_dec;
    }
    void multdyn(const numb& val) {
      dyn = val;
      dynact = fomus_act_mult;
    }
    void divdyn(const numb& val) {
      assert(val != (fint) 0);
      dyn = val;
      dynact = fomus_act_div;
    }
    void moddyn(numb& val) const {
      switch (dynact) {
      case fomus_act_inc:
        val = val + dyn.modval();
        break;
      case fomus_act_dec:
        val = val - dyn.modval();
        break;
      case fomus_act_mult:
        val = val * dyn.modval();
        break;
      case fomus_act_div:
        val = val / dyn.modval();
        break;
      default:;
      }
    }
    void moddynmult(numb& val) const {
      switch (dynact) {
      case fomus_act_mult:
        val = val * dyn.modval();
        break;
      case fomus_act_div:
        val = val / dyn.modval();
        break;
      default:;
      }
    }
    enum fomus_action voiact;
    void setvoices(const numb& val) {
      voices.clear();
      voices.insert(numtoint(val));
    }
    void setvoiceslist(const listelvect& lst) {
      voices.clear();
      for (listelvect_constit i(lst.begin()); i != lst.end(); ++i)
        voices.insert(listel_getint(*i));
    }

    void addvoices(const numb& val) {
      voices.insert(numtoint(val));
    } // add to list!
    void addvoiceslist(const listelvect& lst) {
      for (listelvect_constit i(lst.begin()); i != lst.end(); ++i)
        voices.insert(listel_getint(*i));
    }

    void decvoices(const numb& val, const filepos& pos) {
      voices.insert(numtoint(val));
    }
    void decvoiceslist(const listelvect& lst, const filepos& pos) {
      for (listelvect_constit i(lst.begin()); i != lst.end(); ++i)
        voices.erase(listel_getint(*i));
    }
    void setvoiceslisttype(const fomus_action act, const filepos& pos) {
      switch (act) { // was voiact
      case fomus_act_set:
      case fomus_act_inc:
      case fomus_act_remove:
        voiact = act;
        break;
      default:
        CERR << "invalid voice list action";
        pos.printerr();
        throw errbase();
      }
    }

    void modvoices(std::set<int>& val) const {
      switch (voiact) {
      case fomus_act_set:
        val.clear(); // goto next
      case fomus_act_inc:
        val.insert(voices.begin(), voices.end());
        break;
      case fomus_act_remove:
        for (std::set<int>::iterator i(voices.begin()); i != voices.end(); ++i)
          val.erase(*i);
        break;
      default:;
      }
    }
    void clearvoices() {
      voices.clear();
    }
    //
    fint id;
    dataholderreg(const datastack& stack)
        : dataholder(stack), offact(fomus_act_clear), groffact(fomus_act_clear),
          duract(fomus_act_clear), pitchact(fomus_act_clear),
          dynact(fomus_act_clear), voiact(fomus_act_clear) {}
    void setid(const fint id0) {
      id = id0;
    }
    bool hasid(const fint id0) const {
      return id == id0;
    }
    void stickin(fomusdata& fd);
    bool isnorm() const {
      return false;
    }
    boost::ptr_vector<markobj> marksubs; // marks to subtract
    void addmarks(std::auto_ptr<markobj>& obj) {
      marks.insert(obj);
    }
    void decmarks(std::auto_ptr<markobj>& obj) {
      marksubs.push_back(obj);
    }
    void modmarks(boost::ptr_set<markobj>& m) {
      m.insert(marks.begin(), marks.end());
      for (boost::ptr_vector<markobj>::const_iterator i(marksubs.begin());
           i != marksubs.end(); ++i)
        m.erase(*i);
    }
  };
  inline dataholder::dataholder(const datastack& stack)
      : off(module_none), groff(module_none), dur(module_none),
        pitch(module_none), dyn((fint) 0) {
    if (!stack.empty())
      sets.insert(stack.back().sets.begin(), stack.back().sets.end());
  }
  struct dataholdernorm
      : public dataholder {  // "normal" holder--i.e., not a region
    std::string perc;        // for perc notes
    numb off0, groff0, dur0; // store latest off/groff/dur entries
    dataholdernorm()
        : dataholder(0), off0((fomus_int) 0), groff0(module_none),
          dur0((fomus_int) 0), endtime(module_none), point(point_none) {}
    void setoff(const numb& val, const datastack& st) {
      off = val;
      std::for_each(st.rbegin(), st.rend(),
                    boost::lambda::bind(&dataholderreg::modoff,
                                        boost::lambda::_1,
                                        boost::lambda::var(off)));
    }
    numb modoffmult(numb val, const datastack& st) {
      std::for_each(st.rbegin(), st.rend(),
                    boost::lambda::bind(&dataholderreg::modoffmult,
                                        boost::lambda::_1,
                                        boost::lambda::var(val)));
      return val;
    }
    void incoff(const numb& val, const datastack& st) {
      off = off + modoffmult(val, st).modval();
    }
    void incoffplus(const datastack& st, const filepos& pos) {
      if (groff0.isnull()) {
        off = off + dur0;
        DBG("incoffplus norm off=" << off << std::endl);
      }
    } // if last dur was gr, no inc!
    void decoff(const numb& val, const datastack& st) {
      off = off - modoffmult(val, st).modval();
    }
    void multoff(const numb& val, const datastack& st, const filepos& pos) {
      CERR << "cannot multiply a time value";
      pos.printerr();
      throw errbase();
    }
    void divoff(const numb& val, const datastack& st, const filepos& pos) {
      CERR << "cannot divide a time value";
      pos.printerr();
      throw errbase();
    }
    // grace off
    void cleargr() {
      groffbak = groff;
      groff.null();
    }
    void setgroff(const numb& val, const datastack& st) {
      groff = val;
      std::for_each(st.rbegin(), st.rend(),
                    bind(&dataholderreg::modgroff, boost::lambda::_1,
                         boost::lambda::var(off)));
    }
    numb modgroffmult(numb val, const datastack& st) {
      for_each(st.rbegin(), st.rend(),
               boost::lambda::bind(&dataholderreg::modgroffmult,
                                   boost::lambda::_1, boost::lambda::var(val)));
      return val;
    }
    void incgroff(const numb& val, const datastack& st) {
      groff =
          (groff.isnull() ? (groffbak.isnull() ? numb((fomus_int) 0) : groffbak)
                          : groff) +
          modgroffmult(val, st).modval();
    }
    void incgroffplus(const datastack& st, const filepos& pos) {
      assert(dur0.isntnull());
      groff =
          (groff.isnull() ? (groffbak.isnull() ? numb((fomus_int) 0) : groffbak)
                          : groff) +
          dur0;
    }
    void decgroff(const numb& val, const datastack& st) {
      groff =
          (groff.isnull() ? (groffbak.isnull() ? numb((fomus_int) 0) : groffbak)
                          : groff) -
          modgroffmult(val, st).modval();
    }
    void multgroff(const numb& val, const datastack& st, const filepos& pos) {
      CERR << "cannot multiply a grace time value";
      pos.printerr();
      throw errbase();
    }
    void divgroff(const numb& val, const datastack& st, const filepos& pos) {
      CERR << "cannot multiply a grace time value";
      pos.printerr();
      throw errbase();
    }
    // dur
    void setdur(const numb& val, const datastack& st) {
      dur = val;
      std::for_each(st.rbegin(), st.rend(),
                    bind(&dataholderreg::moddur, boost::lambda::_1,
                         boost::lambda::var(dur)));
      endtime.null();
    }
    numb endtime, grendtime, endtime00; // endtime
    void setendtime(const numb& val, const datastack& st, const filepos& pos) {
      endtime = val;
      grendtime = val;
      std::for_each(st.rbegin(), st.rend(),
                    bind(&dataholderreg::modoff, boost::lambda::_1,
                         boost::lambda::var(endtime)));
      std::for_each(st.rbegin(), st.rend(),
                    bind(&dataholderreg::modgroff, boost::lambda::_1,
                         boost::lambda::var(grendtime)));
      endtime00 = endtime;
      dur.null();
    }
    numb moddurmult(numb val, const datastack& st) {
      std::for_each(st.rbegin(), st.rend(),
                    boost::lambda::bind(&dataholderreg::moddurmult,
                                        boost::lambda::_1,
                                        boost::lambda::var(val)));
      return val;
    }
    void incdur(const numb& val, const datastack& st) {
      dur = dur + moddurmult(val, st).modval();
      endtime.null();
    } // the increment must be multiplied/divided
    void decdur(const numb& val, const datastack& st) {
      dur = dur - moddurmult(val, st).modval();
      endtime.null();
    }
    void multdur(const numb& val, const datastack& st) {
      dur = dur * val.modval();
      endtime.null();
    }
    void divdur(const numb& val, const datastack& st) {
      dur = dur / val.modval();
      endtime.null();
    }
    pointtype point;
    void setpoint(const pointtype pt, const filepos& pos) {
      dur = (fint) 0;
      point = pt;
      endtime.null();
    }
    // pitch
    void setpitch(const numb& val, const datastack& st) {
      pitch = val;
      std::for_each(st.rbegin(), st.rend(),
                    boost::lambda::bind(&dataholderreg::modpitch,
                                        boost::lambda::_1,
                                        boost::lambda::var(pitch)));
      perc.clear();
    }
    void incpitch(const numb& val, const datastack& st) {
      pitch = pitch + val.modval();
      perc.clear();
    }
    void decpitch(const numb& val, const datastack& st) {
      pitch = pitch + val.modval();
      perc.clear();
    }
    void multpitch(const numb& val, const datastack& st, const filepos& pos) {
      CERR << "cannot multiply a pitch";
      pos.printerr();
      throw errbase();
    }
    void divpitch(const numb& val, const datastack& st, const filepos& pos) {
      CERR << "cannot divide a pitch";
      pos.printerr();
      throw errbase();
    }
    void setpitch(const char* prc, const filepos& pos) {
      perc = prc;
    }
    // dyn
    void setdyn(const numb& val, const datastack& st) {
      dyn = val;
      std::for_each(st.rbegin(), st.rend(),
                    boost::lambda::bind(&dataholderreg::moddyn,
                                        boost::lambda::_1,
                                        boost::lambda::var(dyn)));
    }
    numb moddynmult(numb val, const datastack& st) {
      for_each(st.rbegin(), st.rend(),
               boost::lambda::bind(&dataholderreg::moddynmult,
                                   boost::lambda::_1, boost::lambda::var(val)));
      return val;
    }
    void incdyn(const numb& val, const datastack& st) {
      dyn = dyn + moddynmult(val, st).modval();
    } // the increment must be multiplied/divided
    void decdyn(const numb& val, const datastack& st) {
      dyn = dyn - moddynmult(val, st).modval();
    }
    void multdyn(const numb& val) {
      dyn = dyn * val.modval();
    }
    void divdyn(const numb& val) {
      dyn = dyn / val.modval();
    }
    // voices
    void setvoices(const numb& val) {
      voices.clear();
      voices.insert(numtoint(val));
    }
    void setvoiceslist(const listelvect& lst) {
      voices.clear();
      for (listelvect_constit i(lst.begin()); i != lst.end(); ++i)
        voices.insert(listel_getint(*i));
    }

    void addvoices(const numb& val) {
      voices.insert(numtoint(val));
    }
    void addvoiceslist(const listelvect& lst) {
      for (listelvect_constit i(lst.begin()); i != lst.end(); ++i)
        voices.insert(listel_getint(*i));
    }

    void decvoices(const numb& val, const filepos& pos) {
      voices.erase(numtoint(val));
    }
    void decvoiceslist(const listelvect& lst, const filepos& pos) {
      for (listelvect_constit i(lst.begin()); i != lst.end(); ++i)
        voices.erase(listel_getint(*i));
    }
    void setvoiceslisttype(const fomus_action act, const filepos& pos) {
      CERR << "cannot set action for voice list";
      pos.printerr();
      throw errbase();
    }

    void clearvoices() {
      voices.clear();
    }
    void stickin(fomusdata& fd);
    bool isnorm() const {
      return true;
    }
    void addmarks(std::auto_ptr<markobj>& obj) {
      marks.insert(obj);
    }
    void decmarks(std::auto_ptr<markobj>& obj) {
      marks.erase(*obj);
    }
    void checknumbs(const filepos& pos) const;
  };

  void fomus_ivalaux(FOMUS f, int par, int act, fomus_int val);
  void fomus_rvalaux(FOMUS f, int par, int act, fomus_int num, fomus_int den);
  void fomus_mvalaux(FOMUS f, int par, int act, fomus_int val, fomus_int num,
                     fomus_int den);
  void fomus_fvalaux(FOMUS f, int par, int act, fomus_float val);
  void fomus_svalaux(FOMUS f, int par, int act, const char* str);
  void fomus_actaux(
      FOMUS f, int par,
      int act); // use fomus_append action to append (or fomus_set to replace)

  struct apiqueuebase {
    const int par;
    const int act;
    apiqueuebase(const int par, const int act) : par(par), act(act) {}
    virtual void enter(FOMUS f) const {
      fomus_actaux(f, par, act);
    }
    virtual ~apiqueuebase() {}
  };
  struct apiqueue_i : public apiqueuebase {
    const fomus_int val;
    apiqueue_i(const int par, const int act, const fomus_int val)
        : apiqueuebase(par, act), val(val) {}
    void enter(FOMUS f) const {
      fomus_ivalaux(f, par, act, val);
    }
  };
  struct apiqueue_r : public apiqueuebase {
    const fomus_int num, den;
    apiqueue_r(const int par, const int act, const fomus_int num,
               const fomus_int den)
        : apiqueuebase(par, act), num(num), den(den) {}
    void enter(FOMUS f) const {
      fomus_rvalaux(f, par, act, num, den);
    }
  };
  struct apiqueue_m : public apiqueuebase {
    const fomus_int val, num, den;
    apiqueue_m(const int par, const int act, const fomus_int val,
               const fomus_int num, const fomus_int den)
        : apiqueuebase(par, act), val(val), num(num), den(den) {}
    void enter(FOMUS f) const {
      fomus_mvalaux(f, par, act, val, num, den);
    }
  };
  struct apiqueue_f : public apiqueuebase {
    const fomus_float val;
    apiqueue_f(const int par, const int act, const fomus_float val)
        : apiqueuebase(par, act), val(val) {}
    void enter(FOMUS f) const {
      fomus_fvalaux(f, par, act, val);
    }
  };
  struct apiqueue_s : public apiqueuebase {
    const std::string val;
    apiqueue_s(const int par, const int act, const std::string val)
        : apiqueuebase(par, act), val(val) {}
    apiqueue_s(const int par, const int act, const char* val)
        : apiqueuebase(par, act), val(val) {}
    void enter(FOMUS f) const {
      fomus_svalaux(f, par, act, val.c_str());
    }
  };

  struct scoped_module_obj_list : public module_objlist {
    std::vector<module_obj> arr;
    scoped_module_obj_list() {
      n = 0;
      objs = 0;
    }
    void resize(const int sz) {
      if (sz != n) {
        arr.resize(n = sz);
        objs = &arr[0];
      }
      assert(sz == 0 || objs);
    }
  };
  struct scoped_info_marklist : public info_marklist {
    std::vector<info_mark> arr;
    scoped_info_marklist() {
      n = 0;
      marks = 0;
    }
    void resize(const int sz) {
      if (sz != n) {
        arr.resize(n = sz);
        marks = &arr[0];
      }
      assert(sz == 0 || marks);
    }
  };
  extern scoped_info_marklist marklist;
  extern scoped_info_setlist globsetlist;
  struct scoped_info_objinfo_list : public info_objinfo_list {
    std::vector<info_objinfo> arr;
    scoped_info_objinfo_list() {
      n = 0;
      objs = 0;
    }
    void resize(const int sz) {
      destroyvals();
      if (sz != n) {
        arr.resize(n = sz);
        objs = &arr[0];
      }
      assert(sz == 0 || objs);
    }
    ~scoped_info_objinfo_list() {
      destroyvals();
    }
    void destroyvals();
  };

  void get_markinfo(info_mark& m, const markbase& b);

  class stage;
  typedef boost::ptr_vector<stage> stagesvect; // container of all stages

  struct syncs;

  // *************************************************************************************************
  class fomusdata : public modobjbase_sets {
#ifndef NDEBUG
    int fomusdebug;

public:
    bool isvalid() const {
      return fomusdebug == 12345;
    }
#endif

private:
    varcopiesvect invars; // input file variables (temporary), by id
    varsptrmap strinvars; // lookup by name

    listelvect inlist;
    listelstack instack;          // current list
    globpercsvarvect inlistpercs; // special lists
    globinstsvarvect inlistinsts;

public:
    void startlist();
    // insert into the list
    void insertf(const ffloat val);
    void inserti(const fint val);
    void insertr(const fint num, const fint den);
    void insertm(const fint val, const fint num, const fint den);
    void inserts(const char* val);
    void endlist(const bool istop);

private:
    void clearlist() {
      while (!instack.empty())
        instack.pop();
      inlist.clear();
    } // for internal use

    // PARAMETERS
    // ---------------------------------------------------------------------
    int curvar;

public:
    const varcopiesvect& getinvars() {
      return invars;
    }
    void param_settingid(const std::string& str) {
      curvar = getvarid(str);
    }
    void param_settingid(const int id);

private:
    filepos pos;

public:
    filepos& getpos() {
      return pos;
    }
    const filepos& getpos() const {
      return pos;
    }
    inline void param_file(const std::string& str) {
      pos.file = str;
    }
    inline void param_line(const fint val) {
      pos.line = val;
    }
    inline void param_col(const fint val) {
      pos.col = val;
    }

private:
    bool queuestate;
    boost::ptr_vector<apiqueuebase> actqueue;

public:
    void startqueue() {
      queuestate = true;
    };
    void cancelqueue() {
      queuestate = false;
      actqueue.clear();
    }
    void resumequeue() {
      scopedumping xxx();
      queuestate = false;
      std::for_each(
          actqueue.begin(), actqueue.end(),
          boost::lambda::bind(&apiqueuebase::enter, boost::lambda::_1, this));
      actqueue.clear();
    }
    void store(apiqueuebase* x) {
      actqueue.push_back(x);
    }
    bool queueing() const {
      return queuestate;
    }

private:
    dataholder* data;
    datastack stack; // outermost level to most recent--for brackets
public:
    datastack& getstack() {
      return stack;
    }

    void
    setregion() { // user calls this right before a begin bracket expression
      if (data->isnorm()) {
        dataholderreg* d;
        stack.push_back(d = new dataholderreg(stack));
        data = d;
      }
    }
    void endsetregion(
        const fint val) { // this sets the bracket beginning (end of expression)
      setregion();        // insure that data is a holderreg
      ((dataholderreg*) data)->setid(val);
      data = &datanorm;
    }
    void endregion(const fint val);

public:
    bool redunent, soff;
    // `data' is the regionholder ONLY when setting it (before the bracket
    // begins)
    void setoff(const numb& val) {
      redunent = false;
      data->setoff(val, stack);
    }
    void incoff(const numb& val) {
      redunent = false;
      data->incoff(val, stack);
    }
    void decoff(const numb& val) {
      redunent = false;
      data->decoff(val, stack);
    }
    void multoff(const numb& val) {
      redunent = false;
      data->multoff(val, stack, pos);
    }
    void divoff(const numb& val) {
      redunent = false;
      data->divoff(val, stack, pos);
    }

    void setoffs(const numb& val) {
      redunent = false;
      data->setoff(val, stack);
      DBG("soff is on" << std::endl);
      soff = true;
    }
    void incoffs(const numb& val) {
      redunent = false;
      data->incoff(val, stack);
      DBG("soff is on" << std::endl);
      soff = true;
    }
    void decoffs(const numb& val) {
      redunent = false;
      data->decoff(val, stack);
      DBG("soff is on" << std::endl);
      soff = true;
    }
    void multoffs(const numb& val) {
      redunent = false;
      data->multoff(val, stack, pos);
      DBG("soff is on" << std::endl);
      soff = true;
    }
    void divoffs(const numb& val) {
      redunent = false;
      data->divoff(val, stack, pos);
      DBG("soff is on" << std::endl);
      soff = true;
    }

    void incoffbyldur() {
      redunent = false;
      data->incoffplus(stack, pos);
    } // set increment to off0 if reg, set off to off+dur0 if norm (off0,
      // dur0)=prev off/dur
    void incgroffbyldur() {
      redunent = false;
      data->incgroffplus(stack, pos);
    }

    void setgroff(const numb& val) {
      redunent = false;
      data->setgroff(val, stack);
    }
    void incgroff(const numb& val) {
      redunent = false;
      data->incgroff(val, stack);
    }
    void decgroff(const numb& val) {
      redunent = false;
      data->decgroff(val, stack);
    }
    void multgroff(const numb& val) {
      redunent = false;
      data->multgroff(val, stack, pos);
    }
    void divgroff(const numb& val) {
      redunent = false;
      data->divgroff(val, stack, pos);
    }

    void setdur(const numb& val) {
      redunent = false;
      data->setdur(val, stack);
    }
    void setendtime(const numb& val) {
      redunent = false;
      data->setendtime(val, stack, pos);
    }
    void incdur(const numb& val) {
      redunent = false;
      data->incdur(val, stack);
    }
    void decdur(const numb& val) {
      redunent = false;
      data->decdur(val, stack);
    }
    void multdur(const numb& val) {
      redunent = false;
      data->multdur(val, stack);
    }
    void divdur(const numb& val) {
      redunent = false;
      data->divdur(val, stack);
    }
    void setdur(const char* val) {
      redunent = false;
      data->setdur(doparsedur(this, val), stack);
    }

    void setpointlt() {
      redunent = false;
      data->setpoint(point_left, pos);
    }
    void setpointrt() {
      redunent = false;
      data->setpoint(point_right, pos);
    }

    void setpitch(const numb& val) {
      redunent = false;
      data->setpitch(val, stack);
    }
    void setpitch(const char* val) {
      redunent = false;
      numb x(doparsenote(this, val, true));
      if (x.type() == module_none) {
        getdefpercinstr(val);     // throws an error if it isn't a perc instr
        data->setpitch(val, pos); // it's supposedly a percussion instrument
      } else {                    // it's a pitch
        data->setpitch(x, stack);
      }
    }
    void incpitch(const numb& val) {
      redunent = false;
      data->incpitch(val, stack);
    }
    void decpitch(const numb& val) {
      redunent = false;
      data->decpitch(val, stack);
    }
    void multpitch(const numb& val) {
      redunent = false;
      data->multpitch(val, stack, pos);
    }
    void divpitch(const numb& val) {
      redunent = false;
      data->divpitch(val, stack, pos);
    }

    void setdyn(const numb& val) {
      redunent = false;
      data->setdyn(val, stack);
    }
    void incdyn(const numb& val) {
      redunent = false;
      data->incdyn(val, stack);
    }
    void decdyn(const numb& val) {
      redunent = false;
      data->decdyn(val, stack);
    }
    void multdyn(const numb& val) {
      redunent = false;
      data->multdyn(val);
    }
    void divdyn(const numb& val) {
      redunent = false;
      data->divdyn(val);
    }

    void setvoices(const numb& val) {
      redunent = false;
      data->setvoices(val);
    }
    void addvoices(const numb& val) {
      redunent = false;
      data->addvoices(val);
    }
    void decvoices(const numb& val) {
      redunent = false;
      data->decvoices(val, pos);
    }
    void setvoiceslist() {
      redunent = false;
      data->setvoiceslist(inlist);
      inlist.clear();
    }
    void addvoiceslist() {
      redunent = false;
      data->addvoiceslist(inlist);
      inlist.clear();
    }
    void decvoiceslist() {
      redunent = false;
      data->decvoiceslist(inlist, pos);
      inlist.clear();
    }
    void setvoiceslisttype(const fomus_action act) {
      redunent = false;
      data->setvoiceslisttype(act, pos);
    }

    void clearvoiceslist() {
      redunent = false;
      data->clearvoices();
    }

    std::auto_ptr<markobj> amark;
    void setmarkval(const numb& val) {
      redunent = false;
      if (amark.get()) {
        if (amark->setval(val)) {
          CERR << "cannot add number value to mark";
          pos.printerr();
          throw errbase();
        }
      } else {
        CERR << "no mark to add value to";
        pos.printerr();
        throw errbase();
      }
    }
    void setmarkval(const std::string& val) {
      redunent = false;
      if (amark.get()) {
        if (amark->setstr(val)) {
          CERR << "cannot add string value to mark";
          pos.printerr();
          throw errbase();
        }
      } else {
        CERR << "no mark to add value to";
        pos.printerr();
        throw errbase();
      }
    }
    void addmark(const std::string& val) {
      redunent = false;
      amark.reset(
          new markobj(getthemarkdef(val, pos))); /*markact = fomus_act_add;*/
    }

    void
    setmarklist() { // insert one INTO the list (or remove from current list)
      redunent = false;
      if (amark.get()) {
        data->addmarks(amark);
      } else {
        CERR << "no mark to add";
        pos.printerr();
        throw errbase();
      }
    }
    void
    decmarklist() { // insert one INTO the list (or remove from current list)
      redunent = false;
      if (amark.get()) {
        data->decmarks(amark);
      } else {
        CERR << "no mark to remove";
        pos.printerr();
        throw errbase();
      }
    }

    boost::shared_ptr<partormpart_str>
        curseldpart; // currently selected part (for input)
    void setpart(const std::string& str);
    fomus_param curblast;
    void setblastnote(const fomus_param par) {
      redunent = false;
      curblast = par;
    } // set type of note (note, rest, mark)
    void blastnote() {
      if (!redunent)
        data->stickin(*this);
    }
    std::vector<makemeas> makemeass;
    void blastmeasaux(
        const numb& du) { // store them, wait for ALL parts to be created
      DBG("makemeass.push_back du=" << du << std::endl);
      makemeass.push_back(
          makemeas(pos, data->off, du, curmeasdef)); // data->dur
      curmeasdef.reset();
    }

private:
    dataholdernorm datanorm;

    std::string infile;

public:
    void setfilename(const std::string& str) {
      if (infile.empty())
        infile = str;
    }
    const std::string& getfilename() {
      return infile;
    }

private:
    boost::shared_ptr<import_str> imp;
    boost::shared_ptr<export_str> exp;
    boost::shared_ptr<clef_str> clef;
    boost::shared_ptr<staff_str> staff;
    boost::shared_ptr<percinstr_str> perc;
    boost::shared_ptr<instr_str> instr;
    boost::shared_ptr<part_str> apart;
    boost::shared_ptr<partmap_str> partmap;
    boost::shared_ptr<mpart_str> mpart;
    std::vector<boost::shared_ptr<mpart_str>> mpartstack;
    boost::shared_ptr<measdef_str> measdef;

    defpartsmap default_parts;
    defmpartsmap default_mparts;
    definstsmap default_insts; // override global defaults
    defpercsmap default_percs; // override global defaults
    defmeasdefmap default_measdef;
    boost::shared_ptr<part_str> thedefpartdef;
    boost::shared_ptr<measdef_str> thedefmeasdef;
    boost::shared_ptr<mpart_str> theallmpartdef;

    boost::shared_ptr<measdef_str> curmeasdef;
    scorepartlist scoreparts; // has a default part--remove it later if
                              // unused--LAST PTR is the CURRENT PART

    numb merge;

public:
    void mergeinto(fomusdata& x);
    void setmerge(const numb& v) {
      merge = v;
    }

    boost::shared_ptr<measdef_str>& getcurmeasdef() {
      return curmeasdef;
    }
    const boost::shared_ptr<measdef_str>& getcurmeasdef() const {
      return curmeasdef;
    }

    void set_partmap_start() {
      mpartstack.push_back(mpart);
      mpart.reset(new mpart_str());
    }

    const scorepartlist& getscoreparts() const {
      return scoreparts;
    }
    scorepartlist& getscoreparts() {
      return scoreparts;
    }

    scoped_module_obj_list percinsts_cstr;
    const module_objlist& get_percinstslist() {
      percinsts_cstr.resize(default_percs.size());
      for_each2(default_percs.begin(), default_percs.end(),
                (module_obj*) percinsts_cstr.objs,
                boost::lambda::_2 = boost::lambda::bind(
                    &boost::shared_ptr<percinstr_str>::get,
                    boost::lambda::bind(&defpercsmap_val::second,
                                        boost::lambda::_1)));
      return percinsts_cstr;
    }
    const module_objlist& get_allpercinstslist() {
      globpercsvarvect& mp =
          ((percinstrs_var*) getvarbase(PERCINSTRS_ID))->getmap();
      percinsts_cstr.resize(mp.size() + default_percs.size());
      for_each2(mp.begin(), mp.end(), (module_obj*) percinsts_cstr.objs,
                boost::lambda::_2 = boost::lambda::bind(
                    &boost::shared_ptr<percinstr_str>::get,
                    boost::lambda::bind(&defpercsmap_val::second,
                                        boost::lambda::_1)));
      for_each2(default_percs.begin(), default_percs.end(),
                (module_obj*) percinsts_cstr.objs + mp.size(),
                boost::lambda::_2 = boost::lambda::bind(
                    &boost::shared_ptr<percinstr_str>::get,
                    boost::lambda::bind(&defpercsmap_val::second,
                                        boost::lambda::_1)));
      return percinsts_cstr;
    }
    scoped_module_obj_list insts_cstr;
    const module_objlist& get_instslist() {
      insts_cstr.resize(default_insts.size());
      for_each2(default_insts.begin(), default_insts.end(),
                (module_obj*) insts_cstr.objs,
                boost::lambda::_2 = boost::lambda::bind(
                    &boost::shared_ptr<instr_str>::get,
                    boost::lambda::bind(&definstsmap_val::second,
                                        boost::lambda::_1)));
      return insts_cstr;
    }
    const module_objlist& get_allinstslist() {
      globinstsvarvect& mp = ((instrs_var*) getvarbase(INSTRS_ID))->getmap();
      insts_cstr.resize(mp.size() + default_insts.size());
      for_each2(mp.begin(), mp.end(), (module_obj*) insts_cstr.objs,
                boost::lambda::_2 = boost::lambda::bind(
                    &boost::shared_ptr<instr_str>::get,
                    boost::lambda::bind(&definstsmap_val::second,
                                        boost::lambda::_1)));
      for_each2(default_insts.begin(), default_insts.end(),
                (module_obj*) insts_cstr.objs + mp.size(),
                boost::lambda::_2 = boost::lambda::bind(
                    &boost::shared_ptr<instr_str>::get,
                    boost::lambda::bind(&definstsmap_val::second,
                                        boost::lambda::_1)));
      return insts_cstr;
    }
    scoped_module_obj_list parts_cstr;
    const module_objlist& get_partslist() {
      parts_cstr.resize(default_parts.size() + default_mparts.size());
      for_each2(default_parts.begin(), default_parts.end(),
                (module_obj*) parts_cstr.objs,
                boost::lambda::_2 = boost::lambda::bind(
                    &boost::shared_ptr<part_str>::get,
                    boost::lambda::bind(&defpartsmap_val::second,
                                        boost::lambda::_1)));
      for_each2(default_mparts.begin(), default_mparts.end(),
                (module_obj*) parts_cstr.objs + default_parts.size(),
                boost::lambda::_2 = boost::lambda::bind(
                    &boost::shared_ptr<mpart_str>::get,
                    boost::lambda::bind(&defmpartsmap_val::second,
                                        boost::lambda::_1)));
      return parts_cstr;
    }
    const part_str&
    getdefpart(const std::string& id) const; // {return *getdefpartshptr(id);}
    void setdefpartormpartshptr(
        const std::string& id,
        boost::variant<boost::shared_ptr<part_str>,
                       boost::shared_ptr<mpart_str>, std::string>& p) const;
    const mpart_str& getdefmpart(const std::string& id) const;
    const measdef_str& getdefmeasdef(const std::string& id) {
      return *getdefmeasdefptr(id);
    }
    boost::shared_ptr<measdef_str>& getdefmeasdefptr(const std::string& id);
    const instr_str& getdefinstr(const std::string& id) const;
    const percinstr_str& getdefpercinstr(const std::string& id) const;
    const mpart_str* getdefmpart_noexc(const std::string& id) {
      defmpartsmap_it i(default_mparts.find(id));
      return (i == default_mparts.end()) ? 0 : i->second.get();
    }

    int getclef(const std::string& str);

    void set_import_setival(const fint val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      imp->setting(curvar, x, this);
    }
    void set_import_setrval(const rat& val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      imp->setting(curvar, x, this);
    }
    void set_import_setfval(const ffloat val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      imp->setting(curvar, x, this);
    }
    void set_import_setsval(const char* val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnewstr(this, val, pos));
      imp->setting(curvar, x, this);
    }
    void set_import_setlval() {
      try {
        std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(inlist, pos));
        imp->setting(curvar, x, this);
        inlist.clear();
      } catch (const errbase& e) {
        inlist.clear();
        throw;
      }
    }

    void set_export_setival(const fint val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      exp->setting(curvar, x, this);
    }
    void set_export_setrval(const rat& val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      exp->setting(curvar, x, this);
    }
    void set_export_setfval(const ffloat val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      exp->setting(curvar, x, this);
    }
    void set_export_setsval(const char* val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnewstr(this, val, pos));
      exp->setting(curvar, x, this);
    }
    void set_export_setlval() {
      try {
        std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(inlist, pos));
        exp->setting(curvar, x, this);
        inlist.clear();
      } catch (const errbase& e) {
        inlist.clear();
        throw;
      }
    }

    void set_clef_setival(const fint val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      clef->setting(curvar, x, this);
    }
    void set_clef_setrval(const rat& val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      clef->setting(curvar, x, this);
    }
    void set_clef_setfval(const ffloat val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      clef->setting(curvar, x, this);
    }
    void set_clef_setsval(const char* val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnewstr(this, val, pos));
      clef->setting(curvar, x, this);
    }
    void set_clef_setlval() {
      try {
        std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(inlist, pos));
        clef->setting(curvar, x, this);
        inlist.clear();
      } catch (const errbase& e) {
        inlist.clear();
        throw;
      }
    }

    void ins_staff_clefs() {
      clef->complete(*this);
      staff->insclef(clef);
    } //
    void set_staff_setival(const fint val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      staff->setting(curvar, x, this);
    }
    void set_staff_setrval(const rat& val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      staff->setting(curvar, x, this);
    }
    void set_staff_setfval(const ffloat val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      staff->setting(curvar, x, this);
    }
    void set_staff_setsval(const char* val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnewstr(this, val, pos));
      staff->setting(curvar, x, this);
    }
    void set_staff_setlval() {
      try {
        std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(inlist, pos));
        staff->setting(curvar, x, this);
        inlist.clear();
      } catch (const errbase& e) {
        inlist.clear();
        throw;
      }
    }

    void set_percinstr_template(const std::string& str) {
      perc->setbasedon(str);
    }
    void set_percinstr_id(const std::string& str) {
      perc->setid(str);
    }
    void ins_percinstr_imports() {
      imp->complete(*this);
      perc->insimport(imp);
    } //
    void set_percinstr_export() {
      exp->complete(*this);
      perc->setexport(exp);
    } //
    void set_percinstr_setival(const fint val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      perc->setting(curvar, x, this);
    }
    void set_percinstr_setrval(const rat& val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      perc->setting(curvar, x, this);
    }
    void set_percinstr_setfval(const ffloat val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      perc->setting(curvar, x, this);
    }
    void set_percinstr_setsval(const char* val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnewstr(this, val, pos));
      perc->setting(curvar, x, this);
    }
    void set_percinstr_setlval() {
      try {
        std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(inlist, pos));
        perc->setting(curvar, x, this);
        inlist.clear();
      } catch (const errbase& e) {
        inlist.clear();
        throw;
      }
    }
    void listclearpercinst() {
      inlistpercs.clear();
    }
    void listaddpercinst();
    void listsetpercinst();
    void listappendpercinst();

    void set_instr_template(const std::string& str) {
      instr->setbasedon(str);
    }
    void set_instr_id(const std::string& str) {
      instr->setid(str);
    }
    void ins_instr_staff() {
      staff->complete(*this);
      instr->insstaff(staff);
    } //
    void ins_instr_imports() {
      imp->complete(*this);
      instr->insimport(imp);
    } //
    void set_instr_export() {
      exp->complete(*this);
      instr->setexport(exp);
    } //
    void ins_instr_percinstr();
    void ins_instr_percinstr(const std::string& str) {
      instr->inspercinstrid(str);
    }
    void set_instr_setival(const fint val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      instr->setting(curvar, x, this);
    }
    void set_instr_setrval(const rat& val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      instr->setting(curvar, x, this);
    }
    void set_instr_setfval(const ffloat val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      instr->setting(curvar, x, this);
    }
    void set_instr_setsval(const char* val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnewstr(this, val, pos));
      instr->setting(curvar, x, this);
    }
    void set_instr_setlval() {
      try {
        std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(inlist, pos));
        instr->setting(curvar, x, this);
        inlist.clear();
      } catch (const errbase& e) {
        inlist.clear();
        throw;
      }
    }
    void listclearinst() {
      inlistinsts.clear();
    }
    void listaddinst();
    void listsetinst();
    void listappendinst();

    void setnumbset_aux(std::auto_ptr<varbase>& v);
    template <typename T>
    void setnumbset(const T& val) {
      std::auto_ptr<varbase> v(invars[curvar]->getnew(val, pos));
      setnumbset_aux(v);
    }
    void setstrset(const char* val) {
      std::auto_ptr<varbase> v(invars[curvar]->getnewstr(this, val, pos));
      setnumbset_aux(v);
    }

    template <typename T>
    void setnumbset_note(const T& val) {
      std::auto_ptr<varbase> v(invars[curvar]->getnew(val, pos));
      v->throwifinvalid(this);
      data->addset(v.release());
    }
    void setstrset_note(const char* val) {
      std::auto_ptr<varbase> v(invars[curvar]->getnewstr(this, val, pos));
      v->throwifinvalid(this);
      data->addset(v.release());
    }

    void setfset_note(const ffloat val) {
      checkiscurvar();
      setnumbset_note(val);
    }
    void setiset_note(const fint val) {
      checkiscurvar();
      setnumbset_note(val);
    }
    void setrset_note(const fint num, const fint den) {
      checkiscurvar();
      setnumbset_note(rat(num, den));
    }
    void setmset_note(const fint val, const fint num, const fint den) {
      checkiscurvar();
      setnumbset_note(val + rat(num, den));
    }
    void setsset_note(const char* val) {
      checkiscurvar();
      setstrset_note(val);
    }
    void appendlsetel_note(const listel& val);
    void setfsetapp_note(const ffloat val) {
      appendlsetel_note(val);
    }
    void setisetapp_note(const fint val) {
      appendlsetel_note(val);
    }
    void setrsetapp_note(const fint num, const fint den) {
      appendlsetel_note(rat(num, den));
    }
    void setmsetapp_note(const fint val, const fint num, const fint den) {
      appendlsetel_note(val + rat(num, den));
    }
    void setssetapp_note(const char* val) {
      appendlsetel_note(val);
    }
    void setlset_note();
    void appendlset_note();

    void set_part_id(const std::string& str) {
      apart->setid(str);
    }
    void set_part_instr(); //
    void set_part_instr(const std::string& str) {
      apart->setinstrid(str);
    }
    void set_part_setival(const fint val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      apart->setting(curvar, x, this);
    }
    void set_part_setrval(const rat& val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      apart->setting(curvar, x, this);
    }
    void set_part_setfval(const ffloat val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      apart->setting(curvar, x, this);
    }
    void set_part_setsval(const char* val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnewstr(this, val, pos));
      apart->setting(curvar, x, this);
    }
    void set_part_setlval() {
      try {
        std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(inlist, pos));
        apart->setting(curvar, x, this);
        inlist.clear();
      } catch (const errbase& e) {
        inlist.clear();
        throw;
      }
    }

    void set_partmap_setival(const fint val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      partmap->setting(curvar, x, this);
    }
    void set_partmap_setrval(const rat& val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      partmap->setting(curvar, x, this);
    }
    void set_partmap_setfval(const ffloat val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      partmap->setting(curvar, x, this);
    }
    void set_partmap_setsval(const char* val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnewstr(this, val, pos));
      partmap->setting(curvar, x, this);
    }
    void set_partmap_setlval() {
      try {
        std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(inlist, pos));
        partmap->setting(curvar, x, this);
        inlist.clear();
      } catch (const errbase& e) {
        inlist.clear();
        throw;
      }
    }

    void set_partmap_part();
    void set_partmap_part(const std::string& str) {
      partmap->setpartid(str);
    }
    void set_partmap_metapart();
    void set_partmap_metapart(const std::string& str) {
      partmap->setmpartid(str);
    }

    void set_mpart_id(const std::string& str) {
      mpart->setid(str);
    }
    void ins_mpart_partmap(const char* id = 0);
    void set_mpart_setival(const fint val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      mpart->setting(curvar, x, this);
    }
    void set_mpart_setrval(const rat& val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      mpart->setting(curvar, x, this);
    }
    void set_mpart_setfval(const ffloat val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      mpart->setting(curvar, x, this);
    }
    void set_mpart_setsval(const char* val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnewstr(this, val, pos));
      mpart->setting(curvar, x, this);
    }
    void set_mpart_setlval() {
      try {
        std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(inlist, pos));
        mpart->setting(curvar, x, this);
        inlist.clear();
      } catch (const errbase& e) {
        inlist.clear();
        throw;
      }
    }

    void set_measdef_id(const std::string& str) {
      measdef->setid(str);
    }
    void set_measdef_setival(const fint val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      measdef->setting(curvar, x, this);
    }
    void set_measdef_setrval(const rat& val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      measdef->setting(curvar, x, this);
    }
    void set_measdef_setfval(const ffloat val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(val, pos));
      measdef->setting(curvar, x, this);
    }
    void set_measdef_setsval(const char* val) {
      std::auto_ptr<varbase> x(getvarbase(curvar)->getnewstr(this, val, pos));
      measdef->setting(curvar, x, this);
    }
    void set_measdef_setlval() {
      try {
        std::auto_ptr<varbase> x(getvarbase(curvar)->getnew(inlist, pos));
        measdef->setting(curvar, x, this);
        inlist.clear();
      } catch (const errbase& e) {
        inlist.clear();
        throw;
      }
    }

    void clearallnotes() {
      std::for_each(
          scoreparts.begin(), scoreparts.end(),
          boost::lambda::bind(
              &partormpart_str::clearallnotes,
              boost::lambda::bind(&boost::shared_ptr<partormpart_str>::get,
                                  boost::lambda::_1)));
      DBG("makemeass.clear()" << std::endl);
      makemeass.clear();
      clearlist();
    }

    void remfrompartlist(const std::string& id0);
    void ins_part();
    void rem_part(const std::string& id) {
      defpartsmap_it i(default_parts.find(id));
      if (i != default_parts.end()) {
        i->second->clearid();
        default_parts.erase(i);
        remfrompartlist(id);
      }
    }
    void ins_metapart();
    void rem_metapart(const std::string& id) {
      defmpartsmap_it i(default_mparts.find(id));
      if (i != default_mparts.end()) {
        i->second->clearid();
        default_mparts.erase(i);
        remfrompartlist(id);
      }
    }
    void ins_percinst();
    void rem_percinst(const std::string& id) {
      defpercsmap_it i(default_percs.find(id));
      if (i != default_percs.end()) {
        i->second->clearid();
        default_percs.erase(i);
      }
    }
    void ins_inst();
    void rem_inst(const std::string& id) {
      definstsmap_it i(default_insts.find(id));
      if (i != default_insts.end()) {
        i->second->clearid();
        default_insts.erase(i);
      }
    }
    void ins_measdef();
    void rem_measdef(const std::string& id) {
      defmeasdefmap_it i(default_measdef.find(id));
      if (i != default_measdef.end()) {
        i->second->clearid();
        default_measdef.erase(i);
      }
    }

    void set_measdef();
    void set_measdef(std::string str);

private:
    fomsymbols<numb> note_inparse;
    fomsymbols<numb> acc_inparse;
    fomsymbols<numb> mic_inparse;
    fomsymbols<numb> oct_inparse;
    printmap note_inprint;
    printmap acc_inprint;
    printmap mic_inprint;
    printmap oct_inprint;

    fomsymbols<numb> durdot_inparse;
    fomsymbols<numb> dursyms_inparse;
    fomsymbols<numb> durtie_inparse;
    fomsymbols<numb> tupsyms_inparse;

public:
    const boostspirit::symbols<numb>& getdurdot() const {
      return durdot_inparse.empty() ? durdot_parse : durdot_inparse;
    }
    const boostspirit::symbols<numb>& getdursyms() const {
      return dursyms_inparse.empty() ? dursyms_parse : dursyms_inparse;
    }
    const boostspirit::symbols<numb>& getdurtie() const {
      return durtie_inparse.empty() ? durtie_parse : durtie_inparse;
    }
    const boostspirit::symbols<numb>& gettupsyms() const {
      return tupsyms_inparse.empty() ? tupsyms_parse : tupsyms_inparse;
    }

    const boostspirit::symbols<numb>& getnotepa() const {
      return note_inparse.empty() ? note_parse : note_inparse;
    }
    const boostspirit::symbols<numb>& getaccpa() const {
      return acc_inparse.empty() ? acc_parse : acc_inparse;
    }
    const boostspirit::symbols<numb>& getmicpa() const {
      return mic_inparse.empty() ? mic_parse : mic_inparse;
    }
    const boostspirit::symbols<numb>& getoctpa() const {
      return oct_inparse.empty() ? oct_parse : oct_inparse;
    }
    const printmap& getnotepr() const {
      return note_inprint.empty() ? note_print : note_inprint;
    }
    const printmap& getaccpr() const {
      return acc_inprint.empty() ? acc_print : acc_inprint;
    }
    const printmap& getmicpr() const {
      return mic_inprint.empty() ? mic_print : mic_inprint;
    }
    const printmap& getoctpr() const {
      return oct_inprint.empty() ? oct_print : oct_inprint;
    }
    const listelmap& getnoteprmap() const {
      return get_varbase(NOTESYMBOLS_ID).getmapval();
    }
    const listelmap& getaccprmap() const {
      return get_varbase(NOTEACCS_ID).getmapval();
    }
    const listelmap& getmicprmap() const {
      return get_varbase(NOTEMICROTONES_ID).getmapval();
    }
    const listelmap& getoctprmap() const {
      return get_varbase(NOTEOCTAVES_ID).getmapval();
    }

private:
    scoped_info_objinfo_list
        inoutpercinsts; // keep these separate so user can
                        // call/reference all of them at once
    scoped_info_objinfo_list inoutinsts;
    scoped_info_objinfo_list inoutparts;
    scoped_info_objinfo_list inoutmparts;
    scoped_info_objinfo_list inoutmeasdef;

public:
    fomusdata();
    fomusdata(fomusdata& x);

    numb prevnote;
    bool prevnotegup;

    measmapview tmpmeass;
    void filltmppart();

    void checkiscurvar() {
      if (curvar < 0) {
        CERR << "no setting name or id given";
        pos.printerr();
        throw errbase();
      }
    }
    // SET/CHANGE SETTING VALUES
    void setfset(const ffloat val) {
      checkiscurvar();
      setnumbset(val);
    }
    void setiset(const fint val) {
      checkiscurvar();
      setnumbset(val);
    }
    void setrset(const fint num, const fint den) {
      checkiscurvar();
      setnumbset(rat(num, den));
    }
    void setmset(const fint val, const fint num, const fint den) {
      checkiscurvar();
      setnumbset(rat(val + rat(num, den)));
    }
    void setsset(const char* val) {
      checkiscurvar();
      setstrset(val);
    }
    void appendlset_el(const listel& val);
    void setfsetapp(const ffloat val) {
      appendlset_el(val);
    }
    void setisetapp(const fint val) {
      appendlset_el(val);
    }
    void setrsetapp(const fint num, const fint den) {
      appendlset_el(rat(num, den));
    }
    void setmsetapp(const fint val, const fint num, const fint den) {
      appendlset_el(val + rat(num, den));
    }
    void setssetapp(const char* val) {
      appendlset_el(val);
    }
    void setlset();
    void appendlset();

    struct scoped_info_setlist& getinfosetlist() {
      return setlist;
    }

    template <typename T>
    void fillinoutinfo(info_objinfo& str, const T* obj) const {
      str.obj = (modobjbase*) obj;
      obj->getwholemodval(str.val); // this must be deleted!
      std::ostringstream o;
      obj->print(o, this); // this too
      str.valstr = make_charptr0(o);
    }
    template <typename T>
    void fillinoutinfo2(info_objinfo& str, const T* obj) const {
      str.obj = (modobjbase*) obj;
      obj->getwholemodval(str.val); // this must be deleted!
      std::ostringstream o;
      obj->print(o, this, false); // this too
      str.valstr = make_charptr0(o);
    }
    void fillinoutinfo2m(info_objinfo& str, const mpart_str* obj) const {
      str.obj = (modobjbase*) obj;
      obj->getwholemodval(str.val); // this must be deleted!
      std::ostringstream o;
      obj->print(o, this, false); // this too
      str.valstr = make_charptr0(o);
    }

    const info_objinfo_list& getpercinstsinfo();
    const info_objinfo_list& getinstsinfo();
    const info_objinfo_list& getpartsinfo();
    const info_objinfo_list& getmpartsinfo();
    const info_objinfo_list& getmeasdefinfo();

    boost::shared_ptr<str_base> lastent;
    struct info_objinfo getlastentry() {
      struct info_objinfo r;
      if (lastent.get())
        lastent->getlastentry(r, *this);
      else
        throw errbase(); // info function--don't bother printing error
      return r;
    }

    void set_varbase(const int id, varbase* x) {
      invars[id].reset(x);
    }

    // GET STUFF!
    const varbase& get_varbase(const int id) const {
      return *invars[id].get();
    }
    fint get_ival(const int id) const {
      return invars[id]->getival();
    }
    ffloat get_fval(const int id) const {
      return invars[id]->getfval();
    }
    rat get_rval(const int id) const {
      return invars[id]->getrval();
    }
    const numb& get_num(const int id) const {
      return invars[id]->getnval();
    }
    const std::string& get_sval(const int id) const {
      return invars[id]->getsval();
    }
    const listelvect& get_vect(const int id) const {
      return invars[id]->getvectval();
    }
    const listelmap& get_map(const int id) const {
      return invars[id]->getmapval();
    }
    const module_value& get_lval(const int id) const {
      return invars[id]->getmodval();
    }

    int getvarid(const std::string& str) const;
    const varbase* getvarbase(const std::string& str) const {
      return invars[getvarid(str)].get();
    }
    const varbase* getvarbase(const int id) const {
      return invars[id].get();
    }

    fint get_ival(const std::string& str) const {
      return getvarbase(str)->getival();
    }
    ffloat get_fval(const std::string& str) const {
      return getvarbase(str)->getfval();
    }
    rat get_rval(const std::string& str) const {
      return getvarbase(str)->getrval();
    }
    const numb& get_num(const std::string& str) const {
      return getvarbase(str)->getnval();
    }
    const std::string& get_sval(const std::string& str) const {
      return getvarbase(str)->getsval();
    }
    const listelvect& get_vect(const std::string& str) const {
      return getvarbase(str)->getvectval();
    }
    const listelmap& get_map(const std::string& str) const {
      return getvarbase(str)->getmapval();
    }
    const module_value& get_lval(const std::string& str) const {
      return getvarbase(str)->getmodval();
    }

    void reset_set() {
      checkiscurvar();
      invars[curvar] = vars[curvar];
    }

    void get_settinginfo(info_setting& info, const varbase& var) const;
    info_setlist& getsettinginfo() {
      setlist.resize(vars.size());
      for_each2(setlist.sets, setlist.sets + setlist.n, invars.begin(),
                boost::lambda::bind(&fomusdata::get_settinginfo, this,
                                    boost::lambda::_1, *boost::lambda::_2));
      return setlist;
    }

    void makein(boost::shared_ptr<varbase>& v) { // aux. fun
      invars.push_back(boost::shared_ptr<varbase>(v));
      strinvars.insert(varsptrmap_val(v->getname(), v->getid()));
    }

    void throwfpe() {
      CERR << "division by zero error";
      pos.printerr();
      throw errbase();
    }

public:
    // EXECUTE STUFF
    void singlethread(const int v, const std::vector<runpair>::iterator& b1,
                      const std::vector<runpair>::iterator& b2, const int pa,
                      int& endpass, bool& efix);
    void multithread(const int n, const int v,
                     const std::vector<runpair>::iterator& b1,
                     const std::vector<runpair>::iterator& b2, const int pa,
                     int& endpass, bool& efix);
    void runfomus(std::vector<runpair>::iterator b1,
                  const std::vector<runpair>::iterator& b2);

private:
    fint stagenum;

public:
    int getstages(stagesvect& sta, syncs& sys,
                  const std::vector<runpair>::iterator& b1,
                  const std::vector<runpair>::iterator& b2, const int pa,
                  int endpass, bool& efix);
    int getsubstages(const std::string& msg, const int modssetid,
                     stagesvect& sta, syncs& sys, bool& filled,
                     const int endpass, bool& fi, const runpair* fn = 0,
                     const bool invvoicesonly = false);
    void writeout(stagesvect& sta, syncs& sys,
                  std::vector<runpair>::iterator b1,
                  const std::vector<runpair>::iterator& b2, const bool pre);

    const char* gettype() const {
      return "a fomus data";
    };

public:
    fint partind;
    fint nextpartind() {
      return partind++;
    }

    int grpcnt;

    std::vector<int> voicescache;
    struct module_intslist getvoices() {
      module_intslist r = {voicescache.size(), &voicescache[0]};
      return r;
    }
    std::vector<int> stavescache;
    struct module_intslist getstaves() {
      module_intslist r = {stavescache.size(), &stavescache[0]};
      return r;
    }

    void collectallvoices();
    void collectallstaves();
    void sortorder();
    void postmparts();
    // void postmeas();
    void fillnotes1();
    void fillholes1();
    void fillholes2();
    void fillholes3();
    void postparts();
    void insfills();
    void delfills();
    void preprocess();

    void prepare() {
      collectallvoices();
      collectallstaves();
      sortorder();
    }

#ifndef NDEBUGOUT
    void dumpall() const {
      DBG("--------------------------------------------------------------------"
          "--"
          "--------------------------------------------------------------------"
          "--"
          "------------------------------------------------------------"
          << std::endl);
      DBG("DUMP" << std::endl);
      for (scorepartlist_constit i(scoreparts.begin()); i != scoreparts.end();
           ++i)
        (*i)->dumpall();
      DBG("--------------------------------------------------------------------"
          "--"
          "--------------------------------------------------------------------"
          "--"
          "------------------------------------------------------------"
          << std::endl);
    }
#endif

    std::set<std::string> percinstnames;

#ifndef NDEBUG
    bool fu() const {
      ((const var_keysigs&) get_varbase(KEYSIG_ID)).fu();
      return true;
    }
#endif

    // std::vector<eventmap_it> its;
  };

  inline void dataholderreg::stickin(fomusdata& fd) {
    CERR << "cannot set note while defining a region";
    fd.getpos().printerr();
    throw errbase();
  }

  struct badforceacc : public errbase {};
  std::string notenumtostring(const fomusdata* fd, const numb& val,
                              const char* str, const bool withoct = true,
                              const int forceacc = 0);

  inline std::string notevar::getvalstr(const fomusdata* fd,
                                        const char* st) const {
    return notenumtostring(fd, val, st);
  }

  class part_mustconv : public boost::static_visitor<void> {
    const fomusdata& fd;
    boost::variant<boost::shared_ptr<part_str>, boost::shared_ptr<mpart_str>,
                   std::string>& p;

public:
    part_mustconv(const fomusdata& fd,
                  boost::variant<boost::shared_ptr<part_str>,
                                 boost::shared_ptr<mpart_str>, std::string>& p)
        : fd(fd), p(p) {}
    void operator()(const std::string& x) const {
      fd.setdefpartormpartshptr(x, p);
    }
    void operator()(const boost::shared_ptr<part_str>& x) const {
      if (!x.get())
        fd.setdefpartormpartshptr("default", p);
    }
    void operator()(const boost::shared_ptr<mpart_str>& x) const {
      assert(x.get());
    } // should never be 0
  };
  inline void partmap_str::complete(fomusdata& fd) {
    boost::apply_visitor(
        part_mustconv(fd, part),
        part); // convert string or if NULL, supply default part
  }
  inline void measdef_str::complete(fomusdata& fd) {
    if (!basedon.empty())
      completesets(fd.getdefmeasdef(basedon));
  }
  inline void percinstr_str::complete(fomusdata& fd) {
    if (!basedon.empty())
      completeaux(fd.getdefpercinstr(basedon));
  }
  inline void percinstr_str::complete(const filepos& pos) {
    if (!basedon.empty())
      completeaux(getaglobpercinstr(basedon, pos));
  }

  inline const varbase& instr_str::get_varbase(const int id,
                                               const event& ev) const {
    const percinstr_str* i = ev.getpercinst0();
    if (i) {
      const varbase* ret;
      if (i->get_varbase0(id, ret))
        return *ret;
    }
    return get_varbase(id);
  }
  inline fint instr_str::get_ival(const int id, const event& ev) const {
    const percinstr_str* i = ev.getpercinst0();
    if (i) {
      fint ret;
      if (i->get_ival0(id, ret))
        return ret;
    }
    return get_ival(id);
  }
  inline rat instr_str::get_rval(const int id, const event& ev) const {
    const percinstr_str* i = ev.getpercinst0();
    if (i) {
      rat ret;
      if (i->get_rval0(id, ret))
        return ret;
    }
    return get_rval(id);
  }
  inline ffloat instr_str::get_fval(const int id, const event& ev) const {
    const percinstr_str* i = ev.getpercinst0();
    if (i) {
      ffloat ret;
      if (i->get_fval0(id, ret))
        return ret;
    }
    return get_fval(id);
  }
  inline const std::string& instr_str::get_sval(const int id,
                                                const event& ev) const {
    const percinstr_str* i = ev.getpercinst0();
    if (i) {
      const std::string* ret;
      if (i->get_sval0(id, ret))
        return *ret;
    }
    return get_sval(id);
  }
  inline const module_value& instr_str::get_lval(const int id,
                                                 const event& ev) const {
    const percinstr_str* i = ev.getpercinst0();
    if (i) {
      const module_value* ret;
      if (i->get_lval0(id, ret))
        return *ret;
    }
    return get_lval(id);
  }

  inline void dataholder::checktimenumbs(const filepos& pos) const {
    if (off.type() == module_none || off < (fint) 0) {
      CERR << "expected time value of type `real>=0'";
      pos.printerr();
      throw errbase();
    }
    if (groff.type() != module_none &&
        (groff < (fint) /*-1000*/ 0 || groff > (fint) 1000)) {
      CERR << "expected grace time value of type `real0..1000'";
      pos.printerr();
      throw errbase();
    }
  }
  inline void dataholder::checkdurnumb(const filepos& pos) const {
    if (dur.type() == module_none || dur < (fint) 0) {
      CERR << "expected duration value of type `string_dursymbol|real>=0'";
      pos.printerr();
      throw errbase();
    }
  }
  inline numb dataholder::checkmdurnumb(const filepos& pos) const {
    if (dur.type() == module_none)
      return (fint) 0;
    if (dur < (fint) 0) {
      CERR << "expected duration value of type `string_dursymbol|real>=0'";
      pos.printerr();
      throw errbase();
    }
    return dur;
  }

  inline void dataholdernorm::checknumbs(const filepos& pos) const {
    checktimenumbs(pos);
    checkdurnumb(pos);
    if (perc.empty() && (pitch.type() == module_none || pitch < (fomus_int) 0 ||
                         pitch > (fomus_int) 128)) {
      CERR << "expected pitch value of type "
              "`string_notesymbol|real0..128|string_percinst'";
      pos.printerr();
      throw errbase();
    }
  }

  inline void var_keysig::redo(const fomusdata* fd) {
    el.clear();
    redosig(user, el, (fd ? fd->getnotepr() : note_print),
            (fd ? fd->getaccpr() : acc_print),
            (fd ? fd->getmicpr() : mic_print),
            (fd ? fd->getoctpr() : oct_print));
    mval.reset();
    initmodval();
  }

  inline void part_str::getlastentry(info_objinfo& str,
                                     const fomusdata& fd) const {
    fd.fillinoutinfo2(str, this);
  }
  inline void instr_str::getlastentry(info_objinfo& str,
                                      const fomusdata& fd) const {
    fd.fillinoutinfo2(str, this);
  }
  inline void percinstr_str::getlastentry(info_objinfo& str,
                                          const fomusdata& fd) const {
    fd.fillinoutinfo2(str, this);
  }
  inline void mpart_str::getlastentry(info_objinfo& str,
                                      const fomusdata& fd) const {
    fd.fillinoutinfo2m(str, this);
  }
  inline void measdef_str::getlastentry(info_objinfo& str,
                                        const fomusdata& fd) const {
    fd.fillinoutinfo(str, this);
  }

} // namespace fomus

#endif
