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

// TODO:
// finish slurs and other articulations
// tremelos, trills, harmonics
// crescendo wedges (apply them afterwards, along with dynamics in invisible
// track?)

#include "config.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/shared_ptr.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include "module.h"
#include "modutil.h"

#include "debugaux.h"
#include "ilessaux.h"
using namespace ilessaux;

#ifdef BOOST_FILESYSTEM_OLDAPI
#define FS_COMPLETE boost::filesystem::complete
#define FS_FILE_STRING file_string
#define FS_BASENAME(xxx) boost::filesystem::basename(xxx)
#define FS_CHANGE_EXTENSION(xxx, yyy)                                          \
  boost::filesystem::change_extension(xxx, yyy)
#else
#define FS_COMPLETE boost::filesystem::absolute
#define FS_FILE_STRING string
#define FS_BASENAME(xxx) xxx.stem().string()
#define FS_CHANGE_EXTENSION(xxx, yyy) xxx.replace_extension(yyy)
#endif

namespace midiout {

  const char* ierr = 0;

  struct errbase {};

  int progid, noteid, progchangesid, defaultdynid, minvelid, maxvelid,
      mindynmarkid, maxdynmarkid, timevarid, velvarid, sfzvelid, staccmultid,
      accentvelid, staccatissimomultid, tenutovelid, tenutoaddid, marcatomultid,
      /*sluraddid,*/ sluraddid, mezzostaccmultid, attacktimeid, releasetimeid,
      deltatimeid, ppqnid, percchsid, usenoteoffsid, envcanchangeid,
      transposeid, chordbalid, partallid, gracedurid, sulstyleid, openstringsid,
      beatid, compid;

  std::map<std::string, fomus_float, isiless> symvals;

  enum enum_sulstyles { ss_letter, ss_sulletter, ss_roman, ss_sulroman };
  std::map<std::string, enum_sulstyles, isiless> sulstyles;

  struct badmidi {};

  inline void writestr(std::ostream& out, const std::string& str) {
    out.write(str.c_str(), str.size());
  }
  inline void writestr(std::ostream& out, const char* str,
                       const std::streamsize n) {
    out.write(str, n);
  }

  void writevar(std::ostream& out, uint_fast32_t x) {
    // int n = 0;
    char wr[4];
    char* i = wr + 4;
    while (true) {
      uint_fast8_t c = x & 0x7F;
      x >>= 7;
      if (--i < wr)
        throw badmidi();
      *i = (i >= wr + 3) ? c : c | 0x80;
      if (!x)
        break;
    }
    out.write(i, wr + 4 - i);
  }

  inline void write1(std::ostream& out, const uint_fast8_t x) {
    out.put(x);
  }
  inline void write2(std::ostream& out, const uint_fast16_t x) {
    out.put((x >> 8) & 0xff);
    out.put((x >> 0) & 0xff);
  }
  inline void write3(std::ostream& out, const uint_fast32_t x) {
    out.put((x >> 16) & 0xff);
    out.put((x >> 8) & 0xff);
    out.put((x >> 0) & 0xff);
  }
  inline void write4(std::ostream& out, const uint_fast32_t x) {
    out.put((x >> 24) & 0xff);
    out.put((x >> 16) & 0xff);
    out.put((x >> 8) & 0xff);
    out.put((x >> 0) & 0xff);
  }

  enum evtype {
    ev_tempo,
    ev_timesig,
    ev_keysig,
    ev_noteoff,
    ev_prog,
    ev_ctrl1,
    ev_ctrl2,
    /*ev_press,*/ ev_bend,
    ev_noteon // ctrl1 and ctrl2 exist for sorting purposes
  };

  struct midioutdata;
  struct meventetim;
  struct mevent {
    fomus_float tim;
    evtype typ;
    uint_fast32_t val1, val2;
    int voi;
    mevent(midioutdata& dat, const fomus_float tim, const evtype typ,
           const uint_fast32_t val1, const uint_fast32_t val2)
        : tim(tim), typ(typ), val1(val1), val2(val2) {}
    mevent(midioutdata& dat, const fomus_float tim, const evtype typ,
           const uint_fast32_t val1)
        : tim(tim), typ(typ), val1(val1) {}
    mevent(const fomus_float tim, const evtype typ, const uint_fast32_t val1,
           const uint_fast32_t val2)
        : tim(tim), typ(typ), val1(val1), val2(val2) {}
    mevent(const fomus_float tim, const evtype typ, const uint_fast32_t val1)
        : tim(tim), typ(typ), val1(val1) {}
    mevent(const fomus_float tim, const evtype typ, const uint_fast32_t val1,
           const uint_fast32_t val2, const int voi)
        : tim(tim), typ(typ), val1(val1), val2(val2), voi(voi) {}
    void writetim(std::ostream& f, const int ppqn, uint_fast32_t& ctim)
        const { // ctim = current time, because midi format requires delta times
      assert((uint_fast32_t)(tim * ppqn + 0.5) >= 0);
      uint_fast32_t t = (uint_fast32_t)(tim * ppqn + 0.5);
      assert(t >= ctim);
      writevar(f, t - ctim);
      DBG("  (delta t = " << (t - ctim) / (double) ppqn << ")" << std::endl);
      ctim = t;
    }
    virtual void writeout(std::ostream& f, const int ppqn, const int port,
                          const int ch, const bool usenoteoffs,
                          uint_fast32_t& ctim) const;
    virtual void dopitsh(boost::ptr_multiset<mevent>& events,
                         std::vector<std::pair<int, int>>& shs,
                         std::vector<meventetim*>& onoff);
    bool ispass(const int v) const {
      return voi < 0 || voi == v;
    }
  };

  void mevent::writeout(std::ostream& f, const int ppqn, const int port,
                        const int ch, const bool usenoteoffs,
                        uint_fast32_t& ctim) const {
    writetim(f, ppqn, ctim);
    switch (typ) {
      // case ev_timesig:
      // case ev_keysig:
    case ev_noteoff: {
      DBG("writing noteoff (" << val1 << ") @ " << tim << std::endl);
      write1(f, (usenoteoffs ? 0x80 : 0x90) | ch);
      write1(f, val1);
      write1(f, 0);
      break;
    }
    case ev_prog: {
      DBG("writing progch (" << val1 << ") @ " << tim << std::endl);
      write1(f, 0xC0 | ch);
      write1(f, val1);
      break;
    }
    case ev_ctrl1:
    case ev_ctrl2: {
      DBG("writing ctrl (" << val1 << ", " << val2 << ") @ " << tim
                           << std::endl);
      write1(f, 0xB0 | ch);
      write1(f, val1);
      write1(f, val2);
      break;
    }
    // case ev_press: {
    //   DBG("writing pressure (" << val1 << ", " << val2 << ") @ " << tim <<
    //   std::endl); write1(f, 0xA0 | ch); write1(f, val1); write1(f, val2);
    //   break;
    // }
    case ev_bend: {
      DBG("writing bend (" << val1 << ", " << val2 << ") @ " << tim
                           << std::endl);
      write1(f, 0xE0 | ch);
      write1(f, val1);
      write1(f, val2);
      break;
    }
#ifndef NDEBUG
    case ev_noteon:
      assert(false);
    default:
      assert(false);
#endif
    }
  }

  struct meventetim
      : public mevent { // extra stuff for when end of tied note is reached
    fomus_float rawtim, rawetim, etim, etimgauss;
    boost::shared_ptr<std::vector<module_markobj>> ml;
    int pitsh;
    fomus_float dynbase;
    bool artenv;
    meventetim(midioutdata& dat, const fomus_float tim, const evtype typ,
               const uint_fast32_t val1, const uint_fast32_t val2,
               const fomus_float rawtim, const fomus_float rawetim,
               const fomus_float etim, const fomus_float etimgauss,
               const int pitsh,
               boost::shared_ptr<std::vector<module_markobj>>& ml,
               const fomus_float dynbase, const bool artenv)
        : mevent(dat, tim, typ, val1, val2), rawtim(rawtim), rawetim(rawetim),
          etim(etim), etimgauss(etimgauss), ml(ml), pitsh(pitsh),
          dynbase(dynbase), artenv(artenv) {}
    meventetim(const fomus_float tim, const fomus_float rawtim,
               const evtype typ, const uint_fast32_t val1,
               const uint_fast32_t val2, const int pitsh,
               boost::shared_ptr<std::vector<module_markobj>>& ml,
               const fomus_float dynbase, const bool artenv)
        : mevent(tim, typ, val1, val2), rawtim(rawtim), ml(ml), pitsh(pitsh),
          dynbase(dynbase), artenv(artenv) {}
    void writeout(std::ostream& f, const int ppqn, const int port, const int ch,
                  const bool usenoteoffs, uint_fast32_t& ctim) const;
    void dopitsh(boost::ptr_multiset<mevent>& events,
                 std::vector<std::pair<int, int>>& shs,
                 std::vector<meventetim*>& onoff);
  };

  inline void mevent::dopitsh(boost::ptr_multiset<mevent>& events,
                              std::vector<std::pair<int, int>>& shs,
                              std::vector<meventetim*>& onoff) {
    assert(typ != ev_noteon);
    if (typ == ev_noteoff) {
      assert(onoff[val1]);
      voi = onoff[val1]->voi;
      assert(voi < (int) shs.size());
      assert(shs[voi].first == onoff[val1]->pitsh);
      --shs[voi].second;
    } else
      voi = -1;
  }

  void meventetim::dopitsh(boost::ptr_multiset<mevent>& events,
                           std::vector<std::pair<int, int>>& shs,
                           std::vector<meventetim*>& onoff) {
    std::vector<std::pair<int, int>>::iterator i(shs.begin());
    for (; i != shs.end(); ++i) {
      if (i->first == pitsh) {
        assert(i->second >= 0);
        ++i->second;
        goto SKIPPITSH;
      }
    }
    for (i = shs.begin(); i != shs.end(); ++i) {
      if (i->second <= 0) {
        assert(i->second >= 0);
        ++i->second;
        goto SKIPPITSH;
      }
    }
    shs.push_back(std::pair<int, int>(8192, 1));
    i = boost::prior(shs.end());
  SKIPPITSH:
    voi = i - shs.begin();
    if (pitsh != i->first) {
      events.insert(new mevent(tim, ev_bend, pitsh & 0x7F, (pitsh >> 7), voi));
      i->first = pitsh;
    }
    onoff[val1] = this;
  }

  void meventetim::writeout(std::ostream& f, const int ppqn, const int port,
                            const int ch, const bool usenoteoffs,
                            uint_fast32_t& ctim) const {
    assert(typ == ev_noteon);
    writetim(f, ppqn, ctim);
    DBG("writing noteon (" << val1 << ", " << val2 << ") @ " << tim
                           << std::endl);
    write1(f, 0x90 | ch);
    write1(f, val1);
    write1(f, val2);
  }
  inline bool operator<(const mevent& x, const mevent& y) {
    if (x.tim != y.tim)
      return x.tim < y.tim;
    if (x.typ != y.typ)
      return x.typ < y.typ;
    if (x.val1 > y.val1)
      return x.val1 > y.val1;
    // if (x.ch < y.ch) return x.ch < y.ch;
    return false;
  }
  struct meventtempo : public mevent {
    fomus_float tpo;
    meventtempo(midioutdata& dat, const fomus_float tim, const evtype typ,
                const uint_fast32_t val1, const fomus_float tpo)
        : mevent(dat, tim, typ, /*-1,*/ val1, 0), tpo(tpo) {}
    void writeout(std::ostream& f, const int ppqn, uint_fast32_t& ctim) const;
  };
  void meventtempo::writeout(std::ostream& f, const int ppqn,
                             uint_fast32_t& ctim) const {
    writetim(f, ppqn, ctim); // FF 51 03 tt tt tt
    DBG("writing tempo (" << val1 << ") @ " << tim << std::endl);
    write1(f, 0xFF);
    write1(f, 0x51);
    write1(f, 0x03);
    write3(f, val1);
  }

  template <typename V, typename T1, typename T2>
  inline fomus_float scale(const V& val, const T1& a, const T1& b, const T2& x,
                           const T2& y) {
    return x + (((val - a) / (fomus_float)(b - a)) * (y - x));
  }

  struct progch {
    int val;
    bool isperm;
    int artenv;
    progch(const int val, const bool isperm, const int artenv)
        : val(val), isperm(isperm), artenv(artenv) {}
  };

  struct grbound {
    fomus_rat o1, o2; // adjusted end and onset points
    std::set<fomus_rat> nb, na;
    std::map<fomus_rat, fomus_int> gr;
    fomus_rat dur;
    grbound(const bool slash, const fomus_rat& off, const fomus_rat& eoff,
            const fomus_rat& dur)
        : dur(dur) {
      add(slash, off, eoff);
    }
    void add(const bool slash, const fomus_rat& off, const fomus_rat& eoff);
    bool isntmovelt() const {
      return nb.empty();
    } // no grace notes before normal offset
    bool isntmovert() const {
      return na.empty();
    } // no grace notes after normal offset
    void seto1o2(const fomus_rat& o, const fomus_rat& t1, const fomus_rat& t2);
    fomus_rat fixgro(const fomus_rat& o) const {
      assert(gr.find(o) != gr.end());
      return o1 + module_makerat(gr.find(o)->second, gr.size()) * (o2 - o1);
    }
  };

  void grbound::add(const bool slash, const fomus_rat& off,
                    const fomus_rat& eoff) {
    if (slash) {
      nb.insert(off);
      na.erase(off);
      nb.insert(eoff);
      na.erase(eoff);
    } else {
      na.insert(off);
      nb.erase(off);
      na.insert(eoff);
      nb.erase(eoff);
    }
  }

  void grbound::seto1o2(const fomus_rat& o, const fomus_rat& t1,
                        const fomus_rat& t2) {
    o1 = std::max(t1,
                  o - std::max((fomus_int) nb.size() - 1, (fomus_int) 0) * dur);
    o2 = std::min(t2,
                  o + std::max((fomus_int) na.size() - 1, (fomus_int) 0) * dur);
    fomus_int c = 0;
    for (std::set<fomus_rat>::const_iterator i(nb.begin()); i != nb.end(); ++i)
      gr.insert(std::map<fomus_rat, fomus_int>::value_type(*i, c++));
    for (std::set<fomus_rat>::const_iterator i(na.begin()); i != na.end(); ++i)
      gr.insert(std::map<fomus_rat, fomus_int>::value_type(*i, c++));
    na.clear();
    nb.clear();
  }

  enum specenum {
    spec_trem,
    spec_trem1,
    spec_trem2,
    spec_longtr,
    spec_cresc,
    spec_dim
  };
  struct track;
  struct specstr { // structure for special things
    specenum type;
    boost::ptr_multiset<mevent>::iterator ev1, ev2;
    fomus_rat o1, o2;
    fomus_rat val, val2;
    specstr(const specenum s) : type(s) {}
    specstr(const specenum s, const fomus_rat& val0, const fomus_rat& val20)
        : type(s), val(val0), val2(val20) {}
    specstr(const specenum s, const fomus_rat& val0)
        : type(s), val(val0) {} // for trems
    specstr(const specenum s, const fomus_int& val0, const fomus_int& val20,
            const fomus_rat& val30)
        : type(s), val2(val30) { // for longtr
      val.num = val0;
      val.den = val20;
    }
    void set(const specenum s, const fomus_rat& val0) {
      type = s;
      val = val0;
    }
    void set(const specenum s, const fomus_rat& val0, const fomus_rat& val20) {
      type = s;
      val = val0;
      val2 = val20;
    }
    void setstart(const boost::ptr_multiset<mevent>::iterator& ev10,
                  const fomus_rat o10, const fomus_rat o20) {
      ev1 = ev10;
      o1 = o10;
      o2 = o20;
    }
    void dospec(midioutdata& mod, track& tr);
    void setend(const boost::ptr_multiset<mevent>::iterator& ev20) {
      ev2 = ev20;
    }
  };

  struct track {
    int ord;
    std::string trname, inname;
    boost::ptr_multiset<mevent> events;
    module_obj exp;
    std::map<module_markids, progch>
        progchs; // bool value is whether it means continue w/ program change
    // mevent* noffs[128];
    mevent* nons[128];
    // std::vector<std::pair<int, int> > curpitsh;
    std::vector<std::pair<int, int>>
        shs; // key is pitsh adju., pair is curpitsh, how many currently playing
    fomus_float curdyn, timedev, dyndev;
    fomus_float dymin, dymax, dyscmin, dyscmax;
    fomus_float dynsfzadd, dynaccentadd, dyntenutoadd;
    fomus_float attacktim, releasetim, deltatim;
    fomus_float tenadd, marcmult, mezzstaccmult, staccmult, staccissmult;
    fomus_int isp, isntp;
    int prog, lprog, artenv;
    int chordminus;
    fomus_rat chordtim;
    std::vector<mevent*> chord;
    boost::ptr_vector<boost::nullable<specstr>> spec, wspec; // lookup
    boost::ptr_vector<specstr> specs;                        // storage
    boost::ptr_map<const fomus_rat, boost::nullable<grbound>>
        grpts; // min and max grace offsets at various locations (including
               // arpeggios)
    std::vector<module_noteobj> measnotes;
    module_measobj curmeas;
    fomus_float sluradd;
    bool inaslur;
    std::auto_ptr<specstr> wedge;

    track(midioutdata& dat, const int ord, const module_partobj p,
          const int v /*, const int ch*/);
    const boost::ptr_multiset<mevent>::iterator insert(mevent* ev,
                                                       const bool isperc) {
      if (isperc)
        ++isp;
      else
        ++isntp;
      return events.insert(ev);
    }
    const boost::ptr_multiset<mevent>::iterator insert(mevent* ev) {
      return events.insert(ev);
    }
    bool isperc() const {
      return isp > isntp;
    }
    void writeout(std::ostream& f, const int ppqn, const int port, const int ch,
                  const bool usenoteoffs, const int pass,
                  uint_fast32_t ctim) const;
    void dochord();
    void dofixctrls(const midioutdata& mod);
    void dopitsh() {
#ifndef NDEBUG
      std::vector<meventetim*> onoff(128, (meventetim*) 0);
#else
      std::vector<meventetim*> onoff(128);
#endif
      for (boost::ptr_multiset<mevent>::iterator i(events.begin());
           i != events.end(); ++i)
        i->dopitsh(events, shs, onoff);
    }
    void dotremelos(midioutdata& mod) {
      for (boost::ptr_vector<specstr>::iterator i(specs.begin());
           i != specs.end(); ++i)
        i->dospec(mod, *this);
    }
    void dopostproc(midioutdata& mod) {
      dotremelos(mod);
      fixoffs(); // if any offs occur before ons, fix them
      dopitsh();
      dofixctrls(mod);
      // removextractl(); // remove extra control messages
    }
    void fixoffs();
    void getmaxtim(const int ppqn, uint_fast32_t& mti,
                   uint_fast32_t& iti) const {
      assert(!events.empty());
      uint_fast32_t t = boost::prior(events.end())->tim * ppqn + 0.5;
      if (t > mti)
        mti = t;
      if (t < iti)
        iti = t;
    }
  };

  void track::fixoffs() {
    std::vector<boost::ptr_multiset<mevent>::iterator> onoff(128, events.end());
    for (boost::ptr_multiset<mevent>::iterator i(events.begin());
         i != events.end();) {
      assert(i->val1 >= 0 && i->val1 < 128);
      int iv = i->val1;
      switch (i->typ) {
      case ev_noteoff: {
        boost::ptr_multiset<mevent>::iterator& pr(onoff[iv]);
        if (pr == events.end())
          events.erase(i++);
        else {
          assert(pr != i);
          if (pr->typ == ev_noteoff)
            events.erase(pr);
          onoff[iv] = i++;
        }
      } break;
      case ev_noteon: {
        boost::ptr_multiset<mevent>::iterator& pr(onoff[iv]);
        if (pr != events.end() && pr->typ == ev_noteon)
          events.insert(new mevent(i->tim, ev_noteoff, iv, 0));
        onoff[iv] = i++;
      } // continue on
      break;
      default:
        onoff[iv] = i++;
      }
    }
    for (std::vector<boost::ptr_multiset<mevent>::iterator>::iterator i(
             onoff.begin());
         i != onoff.end(); ++i) {
      if (*i != events.end() && (*i)->typ == ev_noteon) {
        events.erase(*i);
      }
    }
  }

  bool operator<(const track& x, const track& y) {
    return x.ord < y.ord;
  }
  typedef boost::ptr_map<const std::pair<module_partobj, int>, track>
      trmaptype; // sort by partobj, int
  track::track(midioutdata& dat, const int ord, const module_partobj p,
               const int v)
      : ord(ord), inname(module_id(module_inst(p))), exp(modout_export(p)),
        // curpitsh(1, std::pair<int, int>(8192, 0)),
        curdyn(symvals.find(module_setting_sval(p, defaultdynid))->second),
        timedev(module_setting_fval(p, timevarid)),
        dyndev(module_setting_fval(p, velvarid)),
        dymin(module_setting_fval(p, minvelid)),
        dymax(module_setting_fval(p, maxvelid)),
        dyscmin(symvals.find(module_setting_sval(p, mindynmarkid))->second),
        dyscmax(symvals.find(module_setting_sval(p, maxdynmarkid))->second),
        dynsfzadd(module_setting_fval(p, sfzvelid)),
        dynaccentadd(module_setting_fval(p, accentvelid)),
        dyntenutoadd(module_setting_fval(p, tenutovelid)),
        attacktim(module_setting_fval(p, attacktimeid)),
        releasetim(module_setting_fval(p, releasetimeid)),
        deltatim(module_setting_fval(p, deltatimeid)),
        tenadd(module_setting_fval(p, tenutoaddid)),
        marcmult(module_setting_fval(p, marcatomultid)),
        mezzstaccmult(module_setting_fval(p, mezzostaccmultid)),
        staccmult(module_setting_fval(p, staccmultid)),
        staccissmult(module_setting_fval(p, staccatissimomultid)), isp(0),
        isntp(0), prog(-1), lprog(-1), artenv(-1),
        chordminus(std::min(module_setting_fval(p, chordbalid) * 127, 127.0)),
        chordtim(module_makerat(0, 1)), curmeas(0),
        sluradd(module_setting_fval(p, sluraddid)), inaslur(false) {
    spec.resize(128, 0);
    wspec.resize(128, 0);
    std::ostringstream o;
    o << module_id(p);
    if (v < 1000 && module_voices(p).n > 1)
      o << '/' << v;
    trname = o.str();
    // std::fill(noffs, noffs + 128, (mevent*)0);
    std::fill(nons, nons + 128, (mevent*) 0);
    events.insert(
        new mevent(dat, 0, ev_ctrl1, 7, 64)); // set default  volume ctrl
    module_value pr(module_setting_val(exp, progchangesid));
    assert(pr.type == module_list);
    for (module_value *i(pr.val.l.vals), *ie(pr.val.l.vals + pr.val.l.n);
         i < ie; i += 2) {
      assert(i->type == module_string);
      assert((i + 1)->type == module_int);
      std::string s(i->val.s);
      assert(!s.empty());
      bool perm;
      int trim, over;
      if (boost::ends_with(s, "*+") || boost::ends_with(s, "+*")) {
        trim = 2;
        perm = true;
        over = 1;
      } else if (boost::ends_with(s, "*-") || boost::ends_with(s, "-*")) {
        trim = 2;
        perm = true;
        over = 0;
      } else if (boost::ends_with(s, "*")) {
        trim = 1;
        perm = true;
        over = -1;
      } else if (boost::ends_with(s, "+")) {
        trim = 1;
        perm = false;
        over = 1;
      } else if (boost::ends_with(s, "-")) {
        trim = 1;
        perm = false;
        over = 0;
      } else {
        perm = false;
        over = -1;
        goto SKIPTRIM;
      }
      s = s.substr(0, s.size() - trim);
    SKIPTRIM:
      assert(module_strtomark(s.c_str()) >= 0);
      progchs.insert(std::map<module_markids, progch>::value_type(
          (module_markids) module_strtomark(s.c_str()),
          progch((i + 1)->val.i, perm, over)));
    }
  }

  struct meventptrless
      : public std::binary_function<const mevent*, const mevent*, bool> {
    bool operator()(const mevent* x, const mevent* y) const {
      return *x < *y;
    }
  };

  void track::writeout(std::ostream& f, const int ppqn, const int port,
                       const int ch, const bool usenoteoffs, const int pass,
                       uint_fast32_t ctim) const {
    writevar(f, 0x00);
    write1(f, 0xFF); // trackname
    write1(f, 0x03);
    writevar(f, trname.size());
    writestr(f, trname);
    writevar(f, 0x00);
    write1(f, 0xFF); // inst
    write1(f, 0x04);
    writevar(f, inname.size());
    writestr(f, inname);
    writevar(f, 0x00);
    write1(f, 0xFF); // port
    write1(f, 0x21);
    write1(f, 0x01);
    write1(f, port);
    writevar(f, 0x00);
    write1(f, 0xFF); // channel, for meta-events
    write1(f, 0x20);
    write1(f, 0x01);
    write1(f, ch);
    // uint_fast32_t ctim = 0;
    std::multiset<mevent*, meventptrless> theseevs;
    std::set<int> onoffs;
    std::set<int> ctrls;
    bool prog = false, bend = false, ctrls0 = false;
    for (boost::ptr_multiset<mevent>::reverse_iterator i(events.rbegin());
         i != events.rend(); ++i) {
      if (i->ispass(pass)) {
        if (!onoffs.empty()) {
          prog = bend = ctrls0 = true;
          ctrls.clear();
        }
        switch (i->typ) {
        case ev_prog:
          if (prog || i->tim <= 0) {
            theseevs.insert(&*i);
            prog = false;
          }
          break;
        case ev_bend:
          if (bend) {
            theseevs.insert(&*i);
            bend = false;
          }
          break;
        case ev_ctrl1:
        case ev_ctrl2:
          if (ctrls0 && ctrls.insert(i->val1).second)
            theseevs.insert(&*i);
          break;
        // case ev_press:
        //   if (onoffs.find(i->val1)) theseevs.insert(&*i);
        //   break;
        case ev_noteoff:
          onoffs.insert(i->val1); // goto next
          theseevs.insert(&*i);
          break;
        case ev_noteon:
          onoffs.erase(i->val1);
          theseevs.insert(&*i);
          break;
        default:;
        }
      }
    }
    for (std::multiset<mevent*, meventptrless>::const_iterator i(
             theseevs.begin());
         i != theseevs.end(); ++i)
      (*i)->writeout(f, ppqn, port, ch, usenoteoffs, ctim);
  }

  struct midioutdata {
    trmaptype trmap;
    bool gausscalc;
    fomus_float gauss2;
    boost::ptr_map<const module_value, meventtempo> tempotr;
    bool cerr;
    std::stringstream CERR;
    std::string errstr;
    //     fomus_float tim;
    midioutdata() : gausscalc(true), cerr(false) /*, tim(0)*/ {}
    track& gettrack(const module_partobj p, const int v, const int ord) {
      trmaptype::iterator i(trmap.find(std::pair<module_partobj, int>(p, v)));
      if (i == trmap.end())
        i = trmap
                .insert(std::pair<module_partobj, int>(p, v),
                        new track(*this, ord, p, v /*, ch++*/))
                .first;
      return *i->second;
    }
    track& gettrack(const module_partobj p, const int v) {
      assert(trmap.find(std::pair<module_partobj, int>(p, v)) != trmap.end());
      return *trmap.find(std::pair<module_partobj, int>(p, v))->second;
    }
    // void updatetim(const fomus_float t) {if (tim < t) tim = t;}
    void getstuff();
    void modout_write(FOMUS fom, const char* filename);
    fomus_float gauss(const fomus_float d);
    fomus_float secstobeats(const module_value& ti,
                            const fomus_float conv) const;
    fomus_float secstobeats(const fomus_float ti,
                            const fomus_float conv) const {
      return secstobeats(module_makeval(ti), conv);
    }
    fomus_float secstobeats(const fomus_rat& ti, const fomus_float conv) const {
      return secstobeats(module_makeval(ti), conv);
    }
    const char* module_err() {
      if (!cerr)
        return 0;
      std::getline(CERR, errstr);
      return errstr.c_str();
    }
    // uint_fast32_t getmaxtim(const int ppq) nconst {return (uint_fast32_t)(tim
    // * ppqn + 0.5);}
  };

  fomus_float midioutdata::gauss(const fomus_float d) {
    fomus_float x1, x2, w;
    if (gausscalc) {
      do {
#ifdef HAVE_RANDOM
        x1 = 2.0 * (random() / (fomus_float) RAND_MAX) - 1.0;
        x2 = 2.0 * (random() / (fomus_float) RAND_MAX) - 1.0;
#else
        x1 = 2.0 * (rand() / (fomus_float) RAND_MAX) - 1.0;
        x2 = 2.0 * (rand() / (fomus_float) RAND_MAX) - 1.0;
#endif
        w = x1 * x1 + x2 * x2;
      } while (w >= 1.0);
      w = sqrt((-2.0 * log(w)) / w);
      gauss2 = x2 * w;
      gausscalc = false;
      return std::min(std::max(x1 * w, -1.5), 1.5) * d;
    } else {
      gausscalc = true;
      return std::min(std::max(gauss2, -1.5), 1.5) * d;
    }
  }

  fomus_float midioutdata::secstobeats(const module_value& ti,
                                       const fomus_float conv) const {
    assert(tempotr.upper_bound(ti) != tempotr.begin());
    boost::ptr_map<const module_value, meventtempo>::const_iterator i(
        boost::prior(tempotr.upper_bound(ti)));
    return i->second->tpo * conv;
  }

  inline bool evinc(boost::ptr_multiset<mevent>::iterator& e,
                    boost::ptr_multiset<mevent>& evs) {
    while (true) {
      if (e == evs.end())
        return true;
      switch (e->typ) {
      case ev_ctrl1:
      case ev_ctrl2:
        if (e->val1 == 7)
          return false;
      default:;
      }
      ++e;
    }
  }

  struct trackall {
    boost::ptr_map<fomus_rat, std::vector<module_markobj>> marks;
    void getmarksforall(const module_partobj p, module_noteobj n);
    void fillup(const fomus_rat& t, std::vector<module_markobj>& vect) const {
      boost::ptr_map<fomus_rat, std::vector<module_markobj>>::const_iterator i(
          marks.find(t));
      if (i != marks.end())
        std::copy(i->second->begin(), i->second->end(),
                  std::back_inserter(vect));
    }
  };

  void trackall::getmarksforall(const module_partobj p, module_noteobj n) {
    module_markslist ml(module_marks(n));
    fomus_rat t(module_time(n));
    module_value li(module_setting_val(n, partallid));
    assert(li.type == module_list);
    int v = module_voice(n) % 1000;
    bool isallup, isalldn;
    for (const module_value *i(li.val.l.vals), *ie(li.val.l.vals + li.val.l.n);
         i < ie; ++i) {
      assert(i->type == module_int);
      if (i->val.i == v) {
        isallup = false;
        isalldn = false;
        goto SKIP;
      }
    }
    {
      int nst = module_totalnstaves(p);
      int s = module_staff(n);
      isallup = (s > 1);
      isalldn = (nst <= 1 || s < nst);
    }
  SKIP:
    for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) {
      switch (module_markpos(*m)) {
      case markpos_above:
        if (isallup)
          marks[t].push_back(*m);
        break;
      case markpos_below:
        if (isalldn)
          marks[t].push_back(*m);
        break;
      default:;
      }
    }
  }

  fomus_rat calctrem(const module_value& tre, const module_noteobj n) {
    fomus_rat be(abs_rat(GET_R(tre)));
    if (be >= (fomus_int) 32) { // unmeasured
      be = module_setting_rval(n, gracedurid);
    } else { // measured
      be = (fomus_int) 1 / (module_setting_rval(n, beatid) * be);
      if (module_setting_ival(n, compid))
        be = be * module_makerat(2, 3);
    }
    return be;
  }

  void midioutdata::getstuff() { // get shift regions first
    module_noteobj nn = 0;
    while (true) {
      nn = module_peeknextnote(nn);
      if (!nn)
        break;
      module_markslist ml(module_marks(nn));
      for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me;
           ++m) {
        switch (module_markbaseid(module_markid(*m))) {
          // case mark_fermata: {
          //   fomus_rat ti(module_endtime(nn));
          //   fomus_rat du(ti - module_time(module_leftmosttiednote(nn)));
          //   fomus_rat sh(du * shiftmult - du);
          //   std::map<fomus_rat, fomus_rat>::iterator i(shift.find(ti));
          //   if (i == shift.end()) shift.insert(std::map<fomus_rat,
          //   fomus_rat>::value_type(ti, sh)); else i->second =
          //   std::max(i->second, sh); break;
          // }
        }
      }
    }
    boost::ptr_map<module_partobj, trackall> tralls;
    nn = 0;                // get tempo regions next
    module_partobj p0 = 0; // get stuff
    int ord = -1;
    while (true) {
      nn = module_peeknextnote(nn);
      if (!nn)
        break;
      if (module_istiedleft(nn))
        continue;
      bool grsl = false, arp = false, arpd = false;
      module_markslist ml(module_marks(nn));
      for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me;
           ++m) {
        switch (module_markbaseid(module_markid(*m))) {
        case mark_tempo: {
          struct modout_tempostr te(modout_tempostr(nn, *m));
          module_value xt(module_marknum(*m));
          fomus_float x =
              (xt.type == module_none ? (fomus_float) 60 : GET_F(xt));
          if (te.beat != (fomus_int) 0)
            x = x * (te.beat / module_setting_rval(nn, beatid));
          fomus_rat mti(module_time(nn));
          tempotr.insert(
              module_makeval(mti),
              new meventtempo(*this, module_rattofloat(mti), ev_tempo,
                              (uint_fast32_t)(60000000 / x + 0.5), x / 60));
          break;
        }
        case mark_graceslash:
          grsl = true;
          break;
        case mark_arpeggio_down:
          arpd = true; // goto next
        case mark_arpeggio:
        case mark_arpeggio_up:
          arp = true;
          break;
        }
      }
      module_partobj p = module_part(nn);
      tralls[p].getmarksforall(p, nn);
      if (p != p0) { // new part
        ++ord;
        p0 = p;
      }
      track& tr = gettrack(p, module_voice(nn), ord);
      fomus_rat t(module_time(nn));
      bool isgr = module_isgrace(nn);
      if (arp || isgr) {
        boost::ptr_map<const fomus_rat, boost::nullable<grbound>>::iterator i(
            tr.grpts.find(t));
        bool isb = (arp || grsl);
        fomus_rat ti(
            isgr ? module_gracetime(nn)
                 : (arpd ? (fomus_int) 128 - module_pitch(nn)
                         : module_pitch(nn))); // second one is if arpeggio
        fomus_rat eti(isgr ? module_gracetiedendtime(nn)
                           : ti); // second one is if arpeggio
        if (i == tr.grpts.end())
          tr.grpts.insert(t, new grbound(isb, ti, eti,
                                         module_setting_rval(nn, gracedurid)));
        else if (i->second == 0) {
          tr.grpts.erase(i);
          tr.grpts.insert(t, new grbound(isb, ti, eti,
                                         module_setting_rval(nn, gracedurid)));
        } else
          i->second->add(isb, ti, eti);
      } else if (module_isnote(nn)) {
        tr.grpts.insert(t, (grbound*) 0);
        tr.grpts.insert(module_tiedendtime(nn), (grbound*) 0);
      }
    }
    for (trmaptype::iterator i(trmap.begin()); i != trmap.end(); ++i) {
      track& tr(*i->second);
      for (boost::ptr_map<const fomus_rat, boost::nullable<grbound>>::iterator
               i(tr.grpts.begin());
           i != tr.grpts.end(); ++i) { // set grace note time adjustments
        if (i->second != 0) {
          fomus_rat t1, t2;
          if (i == tr.grpts.begin()) {
            t1.num = std::numeric_limits<fomus_int>::min() + 1;
            t1.den = 1;
          } else {
            boost::ptr_map<const fomus_rat, boost::nullable<grbound>>::iterator
                j(boost::prior(i));
            t1 = ((j->second == 0 || j->second->isntmovert())
                      ? ((j->first + i->first) / (fomus_int) 2)
                      : (j->first +
                         (i->first - j->first) * module_makerat(2, 3)));
          }
          boost::ptr_map<const fomus_rat, boost::nullable<grbound>>::iterator j(
              boost::next(i));
          if (j == tr.grpts.end()) {
            t2.num = std::numeric_limits<fomus_int>::max();
            t2.den = 1;
          } else {
            t2 = ((j->second == 0 || j->second->isntmovelt())
                      ? ((j->first + i->first) / (fomus_int) 2)
                      : (i->first +
                         (j->first - i->first) * module_makerat(1, 3)));
          }
          i->second->seto1o2(i->first, t1, t2);
        }
      }
    }
    if (tempotr.empty())
      tempotr.insert(module_makeval((fomus_int) 0),
                     new meventtempo(*this, (fomus_float) 0, ev_tempo,
                                     (uint_fast32_t)(60000000 / 120.0 + 0.5),
                                     120.0 / 60));
    fomus_rat harmsnd = {-1, 1};
    fomus_rat harmtch = {-1, 1};
    fomus_rat harmbas = {-1, 1};
    bool isharm = false;
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n)
        break;
      module_partobj p = module_part(n);
      track& tr = gettrack(p, module_voice(n));
      assert(tralls.find(p) != tralls.end());
      const trackall& trall = *tralls.find(p)->second;
      fomus_rat rawtim0(module_time(n)); // adjusted if necessary
      fomus_rat rawtim00(
          rawtim0); // 00 is used to find chords, only adjusted if relevant
      boost::shared_ptr<std::vector<module_markobj>> marks(
          new std::vector<module_markobj>);
      trall.fillup(rawtim0, *marks);
      module_markslist ml(module_marks(n));
      std::copy(ml.marks, ml.marks + ml.n, std::back_inserter(*marks));
      bool isgr = module_isgrace(n); // adjust time for grace note
      assert(tr.grpts.find(rawtim0) != tr.grpts.end() || module_isrest(n) ||
             module_istiedleft(n));
      boost::ptr_map<const fomus_rat, boost::nullable<grbound>>::const_iterator
          pt(tr.grpts.find(rawtim0));
      bool arp = false, arpd = false;
      if (pt != tr.grpts.end() && pt->second) {
        if (isgr)
          rawtim00 = rawtim0 = pt->second->fixgro(module_gracetime(n));
        else {
          for (std::vector<module_markobj>::const_iterator m(marks->begin());
               m != marks->end(); ++m) {
            module_markids id =
                (module_markids) module_markbaseid(module_markid(*m));
            switch (id) {
            case mark_arpeggio_down:
              arpd = true;
            case mark_arpeggio:
            case mark_arpeggio_up:
              arp = true;
              break;
            default:;
            }
          }
          rawtim0 =
              (arp ? pt->second->fixgro(arpd ? (fomus_int) 128 - module_pitch(n)
                                             : module_pitch(n))
                   : pt->second
                         ->o2); // rawtim00 is still the same in either case
        }
      }
      if (module_isnote(n)) {
        bool isperc = module_isperc(n);
        module_percinstobj pexp;
        fomus_rat note;
        if (isperc) {
          pexp = modout_export(module_percinst(n));
          note = module_setting_rval(pexp, noteid);
        } else {
#ifndef NDEBUG
          pexp = 0;
#endif
          note = module_pitch(n) + module_setting_rval(n, transposeid);
        }
        int bnote = note.num / note.den;
        if (bnote < 0 || bnote >= 128)
          goto SKIPTONEXT;
        if (!module_istiedleft(n)) { // NOTEON
          for (std::vector<module_markobj>::const_iterator m(marks->begin());
               m != marks->end(); ++m) {
            module_markids id =
                (module_markids) module_markbaseid(module_markid(*m));
            switch (id) {
            case mark_harm:
              isharm = true;
              break;
            case mark_natharm_sounding: // goto next
            case mark_artharm_sounding:
              harmsnd = note;
              isharm = true;
              break;
            case mark_natharm_touched: // got next
            case mark_artharm_touched:
              harmtch = note;
              isharm = true;
              break;
            case mark_natharm_string: // got next
            case mark_artharm_base:
              harmbas = note;
              isharm = true;
              break;
            case mark_sul: {
              assert(sulstyles.find(module_setting_sval(n, sulstyleid)) !=
                     sulstyles.end());
              assert(module_marknum(*m).type == module_int);
              fomus_rat val(GET_R(module_marknum(*m)));
              module_value strs(module_setting_val(n, openstringsid));
              assert(strs.type == module_list);
              switch (
                  sulstyles.find(module_setting_sval(n, sulstyleid))->second) {
              case ss_sulletter:
              case ss_letter: {
                fomus_rat dif = {std::numeric_limits<fomus_int>::max(), 1};
                harmbas = val;
                for (const module_value *i(strs.val.l.vals),
                     *ie(strs.val.l.vals + strs.val.l.n);
                     i < ie; ++i) {
                  assert(i->type == module_int || i->type == module_rat);
                  fomus_rat d(diff(GET_R(*i), val));
                  if (d < dif) {
                    dif = d;
                    harmbas = GET_R(*i);
                  }
                }
                break;
              }
              case ss_sulroman:
              case ss_roman: {
                if (val.den == 1 && val.num < strs.val.l.n) {
                  assert(val.num > 0);
                  harmbas = GET_R(strs.val.l.vals[strs.val.l.n - val.num]);
                }
                break;
              }
              } // don't set isharm to true!
            }
            default:;
            }
          }
          if (isharm) {
            if (!module_isendchord(n))
              goto SKIPTONEXT;
            if (harmtch.num >= (fomus_int) 0 && harmbas.num >= (fomus_int) 0 &&
                harmsnd.num < (fomus_int) 0) {
              fomus_rat d(harmtch - harmbas);
              bool art = (harmtch.num >= (fomus_int) 0);
              if (d == (fomus_int) 12)
                note = harmbas + (fomus_int) 12;
              else if ((d == (fomus_int) 7) ||
                       (!art && d == (fomus_int) 7 + 12))
                note = harmbas + ((fomus_int) 12 + 7); // 12
              else if ((d == (fomus_int) 5) ||
                       (!art && d == (fomus_int) 5 + 7 + 12))
                note = harmbas + ((fomus_int) 12 + 7 + 5); // 12 + 7
              else if ((d == (fomus_int) 4) ||
                       (!art && ((d == (fomus_int) 4 + 5) ||
                                 (d == (fomus_int) 4 + 5 + 7) ||
                                 (d == (fomus_int) 4 + 5 + 7 + 12))))
                note = harmbas +
                       ((fomus_int) 12 + 7 + 5 + 4); // 12, 7 + 12, 5 + 7 + 12
              else if ((d == (fomus_int) 3) ||
                       (!art && d == (fomus_int) 3 + 4 + 5 + 7 + 12))
                note = harmbas +
                       ((fomus_int) 12 + 7 + 5 + 4 + 3); // 12 + 7 + 5 + 4
            } else if (harmsnd.num >= (fomus_int) 0)
              note = harmsnd;
            harmsnd = harmtch = harmbas = module_makerat(-1, 1);
            isharm = false;
            if (note.num < (fomus_int) 0)
              goto SKIPTONEXT;
            bnote = note.num / note.den;
            if (bnote < 0 || bnote >= 128)
              goto SKIPTONEXT;
          }
          fomus_float rawtim = module_rattofloat(rawtim0);
          fomus_float tim = rawtim + secstobeats(rawtim0, gauss(tr.timedev));
          fomus_rat rawetim0(module_tiedendtime(n));
          // fomus_rat rawetim_init(rawetim0);
          assert(tr.grpts.find(rawetim0) != tr.grpts.end());
          boost::ptr_map<const fomus_rat,
                         boost::nullable<grbound>>::const_iterator
              pt(tr.grpts.find(rawetim0));
          if (pt->second)
            rawetim0 = (isgr ? pt->second->fixgro(module_gracetiedendtime(n))
                             : pt->second->o1);
          fomus_float rawetim = module_rattofloat(rawetim0);
          fomus_float etimgauss = gauss(tr.timedev);
          fomus_float etim = rawetim + secstobeats(rawetim0, etimgauss);
          fomus_float dynadd = 0, dynaddmin = 0; // what to multiply/add todyn
          int curprog = tr.prog;
          int artenv = tr.artenv;
          module_value trem1, trem2, longtr, longtr1, longtr2;
          trem1.type = trem2.type = longtr.type = longtr1.type = longtr2.type =
              module_none;
          // get dyn marks from the part_dyn set of marks into a vector...
          // append the ml list to this vector, then iterate through that
          // instead
          for (std::vector<module_markobj>::const_iterator m(marks->begin());
               m != marks->end(); ++m) {
            module_markids id =
                (module_markids) module_markbaseid(module_markid(*m));
            switch (id) {
            case mark_trem: {
              trem1 = module_marknum(*m);
            } break;
            case mark_trem2: {
              trem2 = module_marknum(*m);
            } break;
            case mark_longtrill: {
              module_value v(module_marknum(*m));
              if (v.type == module_none)
                longtr1.type = module_special;
              else if (v > (fomus_int) 2)
                longtr = v;
              else
                longtr1 = v;
            } break;
            case mark_longtrill2: {
              longtr2 = module_marknum(*m);
            } break;
            case mark_accent:
              dynadd = std::max(tr.dynaccentadd, dynadd);
              break;
            case mark_tenuto:
              dynadd = std::max(tr.dyntenutoadd, dynadd);
              break;
            case mark_cresc_begin:
              tr.wedge.reset(new specstr(spec_cresc));
              break;
            case mark_dim_begin:
              tr.wedge.reset(new specstr(spec_dim));
              break;
              // case mark_mezzostaccato: break;
            case mark_pppppp:
              tr.curdyn = 0;
              break;
            case mark_ppppp:
              tr.curdyn = 1;
              break;
            case mark_pppp:
              tr.curdyn = 2;
              break;
            case mark_ppp:
              tr.curdyn = 3;
              break;
            case mark_pp:
              tr.curdyn = 4;
              break;
            case mark_p:
              tr.curdyn = 5;
              break;
            case mark_mp:
              tr.curdyn = 6;
              break;
            case mark_ffff:
              tr.curdyn = 11;
              break;
            case mark_fff:
              tr.curdyn = 10;
              break;
            case mark_ff:
              tr.curdyn = 9;
              break;
            case mark_f:
              tr.curdyn = 8;
              break;
            case mark_mf:
              tr.curdyn = 7;
              break;
            case mark_marcato:
            case mark_sf: // goto next
            case mark_sfz:
            case mark_fz:
            case mark_rfz:
            case mark_rf:
              dynadd = std::max(tr.dynsfzadd, dynadd);
              dynaddmin = std::max(dynaddmin, (fomus_float) 8);
              break;
            case mark_sff:
            case mark_sffz:
            case mark_ffz:
              dynadd = std::max(tr.dynsfzadd, dynadd);
              dynaddmin = std::max(dynaddmin, (fomus_float) 9);
              break;
            case mark_sfff:
            case mark_sfffz:
            case mark_fffz:
              dynadd = std::max(tr.dynsfzadd, dynadd);
              dynaddmin = std::max(dynaddmin, (fomus_float) 10);
              break;
            case mark_fp:
            case mark_fzp:
            case mark_sfp:
            case mark_sfzp:
              dynadd = std::max(tr.dynsfzadd, dynadd);
              dynaddmin = std::max(dynaddmin, (fomus_float) 8);
              tr.curdyn = 5;
              break;
              // case mark_fermata: incorporated into time warper
            default:;
            }
            if (!isperc) {
              std::map<module_markids, progch>::const_iterator i(
                  tr.progchs.find(id));
              if (i != tr.progchs.end()) {
                curprog = i->second.val;
                artenv = i->second.artenv; // module_setting_ival((isperc ? pexp
                                           // : tr.exp), envcanchangeid);
                if (i->second.isperm) {
                  tr.prog = curprog; // perm. change
                  tr.artenv = artenv;
                }
              }
            }
          }
          std::auto_ptr<specstr> spc;
          if (longtr1.type !=
              module_none) { // tr1/tr2 has the accidental for step above
            fomus_rat p;
            if (longtr.type != module_none)
              p = GET_R(longtr);
            else if (longtr1.type == module_special) {
              fomus_int d =
                  todiatonic(module_writtennote(n)) + 1; // diatonic pitch
              for (std::vector<module_measobj>::reverse_iterator x(
                       tr.measnotes.rbegin());
                   x != tr.measnotes.rend(); ++x) {
                if (todiatonic(module_writtennote(*x)) == d) {
                  p = tochromatic(d) + module_fullacc(*x);
                  goto TRILLOK;
                }
              }
              struct module_keysigref ac(module_keysigacc(n));
              p = tochromatic(d) + ac.acc1 + ac.acc2;
            } else if (longtr2.type != module_none) {
              assert(longtr1.type != module_none);
              p = tochromatic(todiatonic(module_writtennote(n)) + 1) +
                  GET_R(longtr1) + GET_R(longtr2);
            } else
              goto SKIPTREMS;
          TRILLOK:
            fomus_int specv = p.num / p.den;
            spc.reset(
                new specstr(spec_longtr, specv,
                            (fomus_int)((fomus_int) 8192 +
                                        (fomus_int) 8192 * (p - specv) + 0.5),
                            module_setting_rval(n, gracedurid)));
          } else if (trem1.type != module_none && trem1 > (fomus_int) 0) {
            spc.reset(new specstr(spec_trem, calctrem(trem1, n)));
          } else if (trem1.type != module_none) {
            module_noteobj en(module_rightmosttiednote(n));
            fomus_rat ti(module_time(en));
            rawetim0 = ti + (module_endtime(en) - ti) * (fomus_int) 2;
            assert(tr.grpts.find(rawetim0) != tr.grpts.end());
            boost::ptr_map<const fomus_rat,
                           boost::nullable<grbound>>::const_iterator
                pt(tr.grpts.find(rawetim0));
            if (pt->second)
              rawetim0 = (isgr ? pt->second->fixgro(module_gracetiedendtime(n))
                               : pt->second->o1);
            rawetim = module_rattofloat(rawetim0);
            etimgauss = gauss(tr.timedev);
            etim = rawetim + secstobeats(rawetim0, etimgauss);
            spc.reset(new specstr(spec_trem1, calctrem(trem1, n)));
          } else if (trem2.type != module_none) { // 2nd part of tremelo
            fomus_rat ti(module_time(n));         // need original value
            rawtim0 = ti - (module_endtime(n) - ti);
            assert(tr.grpts.find(rawtim0) != tr.grpts.end());
            boost::ptr_map<const fomus_rat,
                           boost::nullable<grbound>>::const_iterator
                pt(tr.grpts.find(rawtim0));
            if (pt->second)
              rawtim0 =
                  (arp ? pt->second->fixgro(arpd ? (fomus_int) 128 -
                                                       module_pitch(n)
                                                 : module_pitch(n))
                       : pt->second
                             ->o2); // rawtim00 is still the same in either case
            rawtim = module_rattofloat(rawtim0);
            tim = rawtim + secstobeats(rawtim0, gauss(tr.timedev));
            spc.reset(new specstr(spec_trem2, calctrem(trem2, n)));
          }
        SKIPTREMS:
          fomus_float dynbase =
              scale(tr.curdyn, tr.dyscmin, tr.dyscmax, tr.dymin,
                    tr.dymax); // the base note dynamic
          int pitsh =
              (int) ((fomus_int) 8192 +
                     (fomus_int) 8192 * (note - (fomus_int) bnote) + 0.5);
          if (pitsh < 0)
            pitsh = 0;
          if (pitsh > 16383)
            pitsh = 16383;
          // if (tr.noffs[bnote]) { // make sure previous note is off (doens't
          // overlap) first
          //   if ((int)tr.noffs[bnote]->val1 == bnote && tr.noffs[bnote]->tim >
          //   tim) {
          //     tr.noffs[bnote]->tim = tim;
          //     tr.noffs[bnote] = 0;
          //   }
          // }
          meventetim* me; // send the noteon
          if (!isperc) {
            if (tr.lprog < 0) { // no program change yet...
              if (curprog < 0)
                curprog = module_setting_ival(tr.exp, progid);
              if (tr.prog < 0)
                tr.prog = curprog;
              tr.insert(new mevent(*this, 0, ev_prog, tr.lprog = curprog));
            } else if (curprog != tr.lprog) {
              assert(curprog >= 0);
              tr.insert(new mevent(*this, tim, ev_prog, tr.lprog = curprog));
            }
          }
          bool aenv =
              (artenv < 0 ? module_setting_ival((isperc ? pexp : tr.exp),
                                                envcanchangeid)
                          : artenv);
          const boost::ptr_multiset<mevent>::iterator mei(tr.insert(
              me = new meventetim(
                  *this, tim, ev_noteon, bnote,
                  std::min(
                      std::max((int) ((dynbase + gauss(tr.dyndev)) * 127 + 1),
                               1),
                      127),
                  rawtim, rawetim, etim, etimgauss, pitsh, marks, dynbase,
                  aenv),
              isperc)); // isperc belongs there
          if (spc.get()) {
            spc->setstart(mei, rawtim0, rawetim0);
            assert(tr.spec.size() == 128);
            tr.spec.replace(tr.spec.begin() + bnote, spc);
          }
          if (tr.wedge.get()) {
            tr.wedge->setstart(mei, rawtim0, rawetim0);
            assert(tr.wspec.size() == 128);
            tr.wspec.replace(tr.wspec.begin() + bnote, tr.wedge);
          }
          assert(rawtim00 >=
                 (fomus_int) 0); // rawtim00 is unaffected by arpeggios or being
                                 // shoved over by grace notes
          if (rawtim00 > tr.chordtim) {
            tr.dochord();
            tr.chordtim = rawtim00;
          }
          tr.chord.push_back(me);
          tr.nons[bnote] = me;
          if (dynadd > 0 || dynaddmin > 0) { // send dyn attack/release
            fomus_float dyn = std::max(
                dynadd,
                scale(dynaddmin, tr.dyscmin, tr.dyscmax, tr.dymin, tr.dymax) -
                    dynbase); // dyn is amnt of vol to add (0.0-1.0, >= 0)
            tr.insert(new mevent(*this, tim, ev_ctrl2, 7,
                                 std::min(std::max((int) (dyn * 127 + 64), 0),
                                          127))); // initial vol
            if (aenv) {
              fomus_float st =
                  tim +
                  secstobeats(
                      rawtim0,
                      tr.attacktim); // rawtim0 is for tempo lookup,
                                     // tr.attacktim is converted to "beats"
              fomus_float et = st + secstobeats(st, tr.releasetim);
              fomus_float met = std::min(et, etim);
              for (fomus_float t = st + secstobeats(st, tr.deltatim),
                               d = et - st;
                   t + secstobeats(t, tr.deltatim) <= met;
                   t += secstobeats(t, tr.deltatim)) {
                tr.insert(new mevent(
                    *this, t, ev_ctrl2, 7,
                    std::min(
                        std::max((int) (dyn * ((et - t) / d) * 127 + 64), 0),
                        127)));
              }
              tr.insert(new mevent(*this, met, ev_ctrl2, 7, 64));
            } else
              tr.insert(new mevent(*this, etim, ev_ctrl2, 7, 64));
          }
        }
        if (tr.nons[bnote] &&
            !module_istiedright(n)) { // end of note... send NOTEOFF
          assert(tr.nons[bnote]);
          meventetim& ev = *(meventetim*) tr.nons[bnote];
          fomus_float timmlt = 1, timmrt = 0; // do modifications...
          bool endofw = false, pedbegin = false, pedend = false;
          for (std::vector<module_markobj>::const_iterator m(marks->begin());
               m != marks->end(); ++m) { // marks in this (last) event
            switch (module_markbaseid(module_markid(*m))) {
            case mark_spic: // goto next
            case mark_staccato:
              if (tr.staccmult < timmlt)
                timmlt = tr.staccmult;
              break;
            case mark_staccatissimo:
              if (tr.staccissmult < timmlt)
                timmlt = tr.staccissmult;
              break;
            case mark_graceslur_end: // goto next
            case mark_slur_end:
            case mark_dottedslur_end:
            case mark_dashedslur_end:
            case mark_phrase_end:
            case mark_dottedphrase_end:
            case mark_dashedphrase_end:
            case mark_nonlegato:
              tr.inaslur = false;
              break;
            case mark_cresc_end: // goto next
            case mark_dim_end:
              endofw = true;
              break;
            case mark_ped_end:
              pedend = true;
              break;
            }
          }
          for (std::vector<module_markobj>::const_iterator m(ev.ml->begin());
               m != ev.ml->end(); ++m) { // marks in the first event
            switch (module_markbaseid(module_markid(*m))) {
            case mark_tenuto:
              if (tr.tenadd > timmrt)
                timmrt = tr.tenadd;
              break;
            case mark_marcato:
              if (tr.marcmult < timmlt)
                timmlt = tr.marcmult;
              break;
            case mark_mezzostaccato:
              if (tr.mezzstaccmult < timmlt)
                timmlt = tr.mezzstaccmult;
              break;
            case mark_graceslur_begin: // goto next
            case mark_slur_begin:
            case mark_dottedslur_begin:
            case mark_dashedslur_begin:
            case mark_phrase_begin:
            case mark_dottedphrase_begin:
            case mark_dashedphrase_begin:
            case mark_legato:
            case mark_moltolegato:
              tr.inaslur = true;
              break;
            case mark_ped_begin:
              pedbegin = true;
              break;
            }
          }
          if (tr.inaslur && tr.sluradd > timmrt)
            timmrt = tr.sluradd;
          mevent* me; // insert a noteoff
          fomus_float ttt =
              (timmlt == 1 && timmrt == 0
                   ? ev.etim
                   : ev.rawtim + (ev.rawetim - ev.rawtim) * timmlt +
                         secstobeats(ev.etim, timmrt) + ev.etimgauss);
          const boost::ptr_multiset<mevent>::iterator mei(
              tr.insert(me = new mevent(*this, ttt, ev_noteoff, bnote, 0)));
          if (pedend)
            tr.insert(new mevent(*this, ttt, ev_ctrl1, 64, 0));
          if (pedbegin)
            tr.insert(new mevent(*this, ttt, ev_ctrl2, 64, 127));
          assert(tr.spec.size() == 128);
          if (!tr.spec.is_null(bnote)) {
            tr.specs.push_back(
                tr.spec.replace(tr.spec.begin() + bnote, 0).release());
            tr.specs.back().setend(mei);
          }
          assert(tr.wspec.size() == 128);
          if (endofw && !tr.wspec.is_null(bnote)) {
            tr.specs.push_back(
                tr.wspec.replace(tr.wspec.begin() + bnote, 0).release());
            tr.specs.back().setend(mei);
          }
          tr.nons[bnote] = 0;
        }
      }
      // else if (module_isrest(n)) {
      // for (std::vector<module_markobj>::const_iterator m(marks.begin()); m !=
      // marks.end(); ++m) {
      //   switch (module_markbaseid(module_markid(*m))) {

      //   }
      // }
      // }
    SKIPTONEXT:
      if (module_isnote(n) && !module_istiedleft(n)) {
        module_measobj m = module_meas(n);
        if (m != tr.curmeas) {
          tr.measnotes.clear();
          tr.curmeas = m;
        }
        tr.measnotes.push_back(n);
      }
      module_skipassign(n);
    }
    // POST PROCESSING
    for (trmaptype::iterator i(trmap.begin()); i != trmap.end(); ++i)
      i->second->dochord(); // finish off any remaining chords
    for (trmaptype::iterator i(trmap.begin()); i != trmap.end(); ++i)
      i->second->dopostproc(*this);
  }

  void specstr::dospec(midioutdata& mod, track& tr) {
    assert(ev1->typ == ev_noteon);
    assert(ev2->typ == ev_noteoff);
    switch (type) {
    case spec_trem: { // single tremelo, val contains duration of 1 tremelo
                      // stroke
      for (fomus_rat o(o1 + val), oe(o2 - val); o <= oe; o = o + val) {
        fomus_float o0 = o + mod.secstobeats(o, mod.gauss(tr.timedev));
        tr.events.insert(
            new mevent(o0, ev_noteoff, ev2->val1, 0)); // also get gauss adj.
        tr.events.insert(new meventetim(
            o0, module_rattofloat(o), ev_noteon, ev1->val1, ev1->val2,
            ((const meventetim&) *ev1).pitsh, ((meventetim&) *ev1).ml,
            ((const meventetim&) *ev1).dynbase,
            ((const meventetim&) *ev1).artenv)); // w/ gauss adj.
      }
    } break;
    case spec_trem1: { // double tremelo, first chord
      fomus_rat o0(o1 + val);
      fomus_float o00(o0 + mod.secstobeats(o0, mod.gauss(tr.timedev)));
      if (o00 < ev2->tim) {
        mevent* e = tr.events.release(ev2).release();
        e->tim = o00;
        ev2 = tr.insert(e);
      }
      for (fomus_rat v2(val * (fomus_int) 2), o(o1 + v2), oe(o2 - val); o <= oe;
           o = o + v2) {
        tr.events.insert(new meventetim(
            o + mod.secstobeats(o, mod.gauss(tr.timedev)), module_rattofloat(o),
            ev_noteon, ev1->val1, ev1->val2, ((const meventetim&) *ev1).pitsh,
            ((meventetim&) *ev1).ml, ((const meventetim&) *ev1).dynbase,
            ((const meventetim&) *ev1)
                .artenv)); // + overlap, also w/ gauss adj.
#warning "with overlap and dynamic variation"
        fomus_rat oo(o + val);
        fomus_float o0 = module_rattofloat(oo > oe ? o2 : oo);
        tr.events.insert(
            new mevent(o0 + mod.secstobeats(o0, mod.gauss(tr.timedev)),
                       ev_noteoff, ev2->val1, 0)); // w/ gauss adj.
      }
    } break;
    case spec_trem2: { // double tremelo, second chord
      fomus_rat o0(o1 + val);
      fomus_float o00(o0 + mod.secstobeats(o0, mod.gauss(tr.timedev)));
      assert(ev1->val1 == ev2->val1);
      {
        mevent* e = tr.events.release(ev1).release();
        e->tim = std::min(o00, ev2->tim);
        ev1 = tr.insert(e);
      }
      for (fomus_rat v2(val * (fomus_int) 2), o(o1 + v2), oe(o2 - v2); o <= oe;
           o = o + v2) {
#warning "with overlap"
        tr.events.insert(new mevent(
            o + mod.secstobeats(o, mod.gauss(tr.timedev)), ev_noteoff,
            ev2->val1, 0)); // + overlap, also w/ gauss adj.
        fomus_float o0 = module_rattofloat(o + val);
        tr.events.insert(new meventetim(
            o0 + mod.secstobeats(o0, mod.gauss(tr.timedev)), o0, ev_noteon,
            ev1->val1, ev1->val2, ((const meventetim&) *ev1).pitsh,
            ((meventetim&) *ev1).ml, ((const meventetim&) *ev1).dynbase,
            ((const meventetim&) *ev1).artenv)); // w/ gauss adj.
      }
    } break;
    case spec_longtr: { // long trill, val contains other note
      bool wh = true;
      int pi = ev1->val1;
      for (fomus_rat o(o1 + val2), oe(o2 - val2); o <= oe;
           o = o + val2) { // figure out the correct duration
        wh = !wh;
#warning "with overlap"
        tr.events.insert(
            new mevent(o + mod.secstobeats(o, mod.gauss(tr.timedev)),
                       ev_noteoff, pi, 0)); // also get gauss adj.
        int pb;
        if (wh) {
          pi = ev1->val1;
          pb = ((const meventetim&) *ev1).pitsh;
        } else {
          pi = val.num;
          pb = val.den;
        }
        tr.events.insert(new meventetim(
            o + mod.secstobeats(o, mod.gauss(tr.timedev)), module_rattofloat(o),
            ev_noteon, pi, ev1->val2, pb, ((meventetim&) *ev1).ml,
            ((const meventetim&) *ev1).dynbase,
            ((const meventetim&) *ev1).artenv)); // w/ gauss adj.
      }
      if ((int) ev2->val1 != pi) {
        mevent* e = tr.events.release(ev2).release();
        e->val1 = pi;
        tr.insert(e); // don't need to reassign ev2, we're done
      }
    } break;
    case spec_cresc:
    case spec_dim: {
      assert(ev1 != ev2);
      boost::ptr_multiset<mevent>::const_iterator ev2o(boost::prior(ev2));
      while (ev2o->typ != ev_noteon) {
        assert(ev2o != tr.events.begin());
        --ev2o;
      }
      if (type == spec_cresc ? ((const meventetim&) *ev2o).dynbase <=
                                   ((const meventetim&) *ev1).dynbase
                             : ((const meventetim&) *ev2o).dynbase >=
                                   ((const meventetim&) *ev1).dynbase) {
        boost::ptr_multiset<mevent>::const_iterator ev2o0(ev2o);
        do {
          if (++ev2o0 == tr.events.end())
            goto CRESCNEXT;
        } while (ev2o0->typ != ev_noteon);
        ev2o = ev2o0;
      }
    CRESCNEXT:
      assert(ev2o != tr.events.end() && ev2o->typ == ev_noteon);
      fomus_float x = std::min(
          std::max(scale(scale(((const meventetim&) *ev1).dynbase, tr.dymin,
                               tr.dymax, tr.dyscmin, tr.dyscmax) +
                             (type == spec_cresc ? 1 : -1),
                         tr.dyscmin, tr.dyscmax, tr.dymin, tr.dymax),
                   0.0),
          1.0);
      if (type == spec_cresc ? ((const meventetim&) *ev2o).dynbase > x
                             : ((const meventetim&) *ev2o).dynbase < x)
        x = ((const meventetim&) *ev2o).dynbase;
      fomus_float ti = -1;
      boost::ptr_multiset<mevent>::const_iterator le2;
      for (boost::ptr_multiset<mevent>::iterator i(ev1);; ++i) {
        assert(i != tr.events.end());
        switch (i->typ) {
        case ev_noteon:
          if (ti >= 0) {
            if (le2 == tr.events.end())
              le2 = ev2o;
            fomus_float d = (x - ((const meventetim&) *ev1).dynbase) /
                            (ev2o->tim - ev1->tim);
            for (fomus_float t = ti;
                 t + mod.secstobeats(t, tr.deltatim) <= le2->tim;
                 t += mod.secstobeats(t, tr.deltatim)) {
              tr.insert(new mevent(
                  t, ev_ctrl2, 7,
                  std::min(std::max((int) (d * (t - ti) * 127 + 64), 0), 127)));
            }
            tr.insert(new mevent(le2->tim, ev_ctrl2, 7, 64));
            ti = -1;
          }
          if (i == ev2o)
            goto EXITLOOP;
          {
            ((meventetim&) *i).dynbase =
                scale(i->tim, ev1->tim, ev2o->tim,
                      ((const meventetim&) *ev1).dynbase, x);
            if (((const meventetim&) *ev1).artenv) {
              ti = i->tim;
              assert(ti >= 0);
              le2 = tr.events.end();
            } else
              ti = -1;
            i->val2 =
                std::min(std::max((int) ((((const meventetim&) *i).dynbase +
                                          mod.gauss(tr.dyndev)) *
                                             127 +
                                         1),
                                  1),
                         127);
          }
          break;
        case ev_noteoff:
          le2 = i;
          break;
        default:;
        }
      }
    EXITLOOP:;
    } break;
    default:
      assert(false);
    }
  }

  void track::dofixctrls(const midioutdata& mod) {
    boost::ptr_multiset<mevent>::iterator e1(events.begin());
    if (evinc(e1, events))
      return; // eliminate ctrls that are too close
    fomus_float deltim(deltatim * 0.75);
    while (true) {
      boost::ptr_multiset<mevent>::iterator e2(boost::next(e1));
      if (evinc(e2, events))
        return;
      if (e1->val2 == e2->val2) {
        events.erase(e2);
      } else if (e1->tim + mod.secstobeats(e1->tim, deltim) >= e2->tim) {
        if (e1->val2 >= e2->val2)
          events.erase(e2);
        else {
          events.erase(e1);
          e1 = e2;
        }
      } else
        e1 = e2;
    }
  }

  void track::dochord() {
    if (chord.size() > 1) {
      const mevent* x;
      int n = -1;
      for (std::vector<mevent*>::const_iterator j(chord.begin());
           j != chord.end(); ++j) {
        if ((int) (*j)->val1 > n) {
          x = *j;
          n = x->val1;
        }
      }
      for (std::vector<mevent*>::const_iterator j(chord.begin());
           j != chord.end(); ++j) {
        if (*j != x)
          (*j)->val2 -= chordminus;
      }
    }
    chord.clear();
  }

  struct midichunk {
    boost::filesystem::ofstream& f;
    boost::filesystem::ofstream::streampos pos;
    midichunk(boost::filesystem::ofstream& f)
        : f(f), pos(f.tellp()) { /*writestr(f, "\0\0\0\0", 4);*/
      f.seekp(pos + (std::streamoff) 4);
    }
    ~midichunk() {
      boost::filesystem::ofstream::streampos epos = f.tellp();
      boost::filesystem::ofstream::streampos val =
          epos - (pos + (std::streamoff) 4);
      f.seekp(pos);
      write4(f, val);
      f.seekp(epos);
    }
  };

  void midioutdata::modout_write(
      FOMUS fom, const char* filename) { // filename should be complete
    getstuff();
    try {
      boost::filesystem::path fn(filename);
      boost::filesystem::ofstream f;
      try {
        f.exceptions(boost::filesystem::ofstream::eofbit |
                     boost::filesystem::ofstream::failbit |
                     boost::filesystem::ofstream::badbit);
        f.open(fn.FS_FILE_STRING(), boost::filesystem::ofstream::out |
                                        boost::filesystem::ofstream::trunc |
                                        boost::filesystem::ofstream::binary);
        writestr(f, "MThd", 4);
        write4(f, 6);                // always 6
        write2(f, 1);                // format type
        write2(f, trmap.size() + 1); // number of tracks
        int ppqn = module_setting_ival(fom, ppqnid);
        write2(f, ppqn);
        int ltrp = 0, ltrch = -1, lptrp = 0,
            lptrch = -1; // last port/channel, last perc port/channel
        module_value pcs(module_setting_val(fom, percchsid));
        assert(pcs.type == module_list);
        boost::ptr_vector<std::set<int>>
            pports; // percussion ports/channel sets
        for (const module_value *i(pcs.val.l.vals),
             *ie(pcs.val.l.vals + pcs.val.l.n);
             i < ie; ++i) {
          assert(i->type == module_list);
          std::set<int>* s;
          pports.push_back(s = new std::set<int>);
          for (const module_value *j(i->val.l.vals),
               *je(i->val.l.vals + i->val.l.n);
               j < je; ++j) {
            assert(j->type == module_int);
            s->insert(j->val.i);
          }
        }
        bool usenoteoffs = module_setting_ival(fom, usenoteoffsid);
        std::vector<track*> trs;
        for (trmaptype::iterator i(trmap.begin()); i != trmap.end(); ++i)
          trs.push_back((*i)->second);
        std::sort(trs.begin(), trs.end());
        uint_fast32_t maxtim = 0, mintim = 0;
        for (std::vector<track*>::const_iterator tr(trs.begin());
             tr != trs.end(); ++tr)
          (*tr)->getmaxtim(ppqn, maxtim, mintim);
        DBG("maxtim = " << maxtim / (fomus_float) ppqn << ", mintim = "
                        << mintim / (fomus_float) ppqn << std::endl);
        { // tempo track
          writestr(f, "MTrk", 4);
          midichunk xxx(f);
          uint_fast32_t ctim = mintim;
          for (boost::ptr_map<const module_value, meventtempo>::const_iterator
                   i(tempotr.begin());
               i != tempotr.end(); ++i)
            i->second->writeout(f, ppqn, ctim);
          writevar(f, maxtim);
          write1(f, 0xFF); // end of track
          write1(f, 0x2F);
          write1(f, 0x00);
        }
        for (std::vector<track*>::const_iterator tr(trs.begin());
             tr != trs.end(); ++tr) {
          int npas = std::max((int) (*tr)->shs.size(), 1);
          assert(npas > 0);
          for (int pas = 0; pas < npas; ++pas) {
            writestr(f, "MTrk", 4);
            midichunk xxx(f);
            bool isperc = (*tr)->isperc();
            if (isperc) { // get the port and channel
              if (pports.empty()) {
              NOPERCPORTS:
                CERR << "no percussion channels available";
                throw errbase();
              }
              int n = 0;
              while (true) {
                const std::set<int>& pp(
                    pports[std::min(lptrp, (int) pports.size() - 1)]);
                std::set<int>::const_iterator cc(pp.upper_bound(lptrch));
                if (cc != pp.end()) {
                  lptrch = *cc;
                  break;
                }
                if (++n >= 2 && lptrp >= (int) pports.size())
                  goto NOPERCPORTS;
                ++lptrp;
                lptrch = -1;
              }
            } else {
              int n = 0;
              while (true) {
                if (++ltrch < 16) {
                  if (pports.empty())
                    break;
                  const std::set<int>& pp(
                      pports[std::min(ltrp, (int) pports.size() - 1)]);
                  if (pp.find(ltrch) == pp.end())
                    break;
                  ++n;
                  if (n >= 2 &&
                      ltrp >= (int) pports.size()) { // no channels left
                    CERR << "no non-percussion channels available";
                    throw errbase();
                  }
                }
                ++ltrp;
                ltrch = -1;
              }
            }
            if (isperc)
              (*tr)->writeout(f, ppqn, lptrp, lptrch, usenoteoffs, pas, mintim);
            else
              (*tr)->writeout(f, ppqn, ltrp, ltrch, usenoteoffs, pas, mintim);
            writevar(f, maxtim);
            write1(f, 0xFF); // end of track
            write1(f, 0x2F);
            write1(f, 0x00);
          }
        }
        f.close();
        return;
      } catch (const boost::filesystem::ofstream::failure& e) {
        CERR << "error writing `" << fn.FS_FILE_STRING() << '\'' << std::endl;
      }
    } catch (const boost::filesystem::filesystem_error& e) {
      CERR << "invalid path/filename `" << filename << '\'' << std::endl;
    } catch (const errbase& e) {}
    cerr = true;
  }

  const char* midiouttracktype = "integer0..127";
  int valid_midiouttrack(const struct module_value val) {
    return module_valid_int(val, 0, module_incl, 128, module_excl, 0,
                            midiouttracktype);
  }
  const char* midioutchtype = "integer0..15";
  int valid_midioutch(const struct module_value val) {
    return module_valid_int(val, 0, module_incl, 16, module_excl, 0,
                            midioutchtype);
  }
  const char* midioutprogtype =
      "(string_mark integer0..127, string_mark integer0..127, ...)";
  int valid_midioutprogaux(int n, const char* sym, fomus_int x) {
    std::string s(sym);
    return module_strtomark(
               ((boost::ends_with(s, "*-") || boost::ends_with(s, "*+") ||
                 boost::ends_with(s, "-*") || boost::ends_with(s, "+*"))
                    ? s.substr(0, s.size() - 2)
                    : ((boost::ends_with(s, "*") || boost::ends_with(s, "+") ||
                        boost::ends_with(s, "-"))
                           ? s.substr(0, s.size() - 1)
                           : s))
                   .c_str()) >= 0;
  }
  int valid_midioutprog(const struct module_value val) {
    return module_valid_maptoints(val, -1, -1, 0, module_incl, 128, module_excl,
                                  valid_midioutprogaux, midioutprogtype);
  }
  const char* midioutnotetype = "integer0..127";
  int valid_midioutnote(const struct module_value val) {
    return module_valid_int(val, 0, module_incl, 128, module_excl, 0,
                            midioutnotetype);
  }
  const char* dynamicstringtype = "string_dynmark";
  int valid_dynamicstringtypeaux(const char* str) {
    return symvals.find(str) != symvals.end();
  }
  int valid_dynamicstringtype(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_dynamicstringtypeaux,
                               dynamicstringtype);
  }
  const char* velocitytype = "real0..1";
  int valid_velocitytype(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) 0), module_incl,
                            module_makeval((fomus_int) 1), module_incl, 0,
                            velocitytype);
  }
  const char* multtype = "real>0..2";
  int valid_multtype(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) 0), module_excl,
                            module_makeval((fomus_int) 2), module_incl, 0,
                            multtype);
  }
  // const char* timevartype = "real>=0";
  // int valid_timevartype(const struct module_value val) {
  //   return module_valid_number(val, fomus_makeval((fomus_int)0), module_incl,
  //   fomus_makeval((fomus_int)0), module_nobound, 0, timevartype);
  // }
  const char* ppqntype = "integer1..65535";
  int valid_ppqn(const struct module_value val) {
    return module_valid_int(val, 1, module_incl, 65535, module_incl, 0,
                            ppqntype);
  }
  const char* percchstype =
      "((integer0..15 integer0..15 ...) (integer0..15 integer0..15 ...) ...)";
  int valid_perchstypeaux(int n, struct module_value val) {
    return module_valid_listofints(val, -1, -1, 0, module_incl, 16, module_excl,
                                   0, percchstype);
  }
  int valid_percchstype(const struct module_value val) {
    return module_valid_listofvals(val, -1, -1, valid_perchstypeaux,
                                   percchstype);
  }
  const char* multivoicetype = "((integer1..128 integer1..128 ...) "
                               "(integer1..128 integer1..128 ...) ...)";
  int valid_multivoicetype(struct module_value val) {
    return module_valid_listofints(val, -1, -1, 1, module_incl, 128,
                                   module_incl, 0, multivoicetype);
  }
  const char* gracedurtype = "rational>0";
  int valid_gracedurtype(const struct module_value val) {
    return module_valid_rat(val, module_makerat(0, 1), module_excl,
                            module_makerat(0, 1), module_nobound, 0,
                            gracedurtype);
  }
} // namespace midiout

using namespace midiout;

const char* module_longname() {
  return "MIDI File Output";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Writes MIDI files (useful for hearing the results of FOMUS's time "
         "quantization and other side effects).";
}

void* module_newdata(FOMUS f) {
  return new midioutdata;
}
void module_freedata(void* dat) {
  delete (midioutdata*) dat;
}
const char* module_err(void* dat) {
  return ((midioutdata*) dat)->module_err();
}

// int module_priority() {return 0;}
enum module_type module_type() {
  return module_modoutput;
}
const char* module_initerr() {
  return ierr;
}

void module_init() {
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
  sulstyles.insert(std::map<std::string, enum_sulstyles>::value_type(
      "sulletter", ss_sulletter));
  sulstyles.insert(
      std::map<std::string, enum_sulstyles>::value_type("letter", ss_letter));
  sulstyles.insert(std::map<std::string, enum_sulstyles>::value_type(
      "sulroman", ss_sulroman));
  sulstyles.insert(
      std::map<std::string, enum_sulstyles>::value_type("roman", ss_roman));
}
void module_free() {}
int module_itertype() {
  return module_all;
}

int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "midiout-prog"; // docscat{midiout}
    set->type = module_int;
    set->descdoc = "Indicates a MIDI program change number for an instrument.  "
                   "Use this setting in an export object inside an instrument "
                   "definition to affect MIDI output.";
    set->typedoc = midioutnotetype;

    module_setval_int(&set->val, 0);

    set->loc = module_locexport;
    set->valid = valid_midioutnote;
    set->uselevel = 2;
    progid = id;
    break;
  }
  case 1: {
    set->name = "midiout-note"; // docscat{midiout}
    set->type = module_int;
    set->descdoc =
        "Indicates a MIDI note for a percussion instrument (FOMUS "
        "automatically sends percussion instrument events to MIDI drum tracks)."
        "  Use this setting in an export object inside an instrument "
        "definition to affect MIDI output.";
    set->typedoc = midioutnotetype;

    module_setval_int(&set->val, 60);

    set->loc = module_locexport;
    set->valid = valid_midioutnote;
    set->uselevel = 2;
    noteid = id;
    break;
  }
  case 2: {
    set->name = "midiout-progchanges"; // docscat{midiout}
    set->type = module_symmap_nums;
    set->descdoc =
        "Mapping from mark id strings to MIDI program change numbers.  "
        "Set this in an export object inside an instrument definition to cause "
        "a program change whenever certain marks occur.  "
        "By default, FOMUS inserts program changes so that only the note or "
        "chord with the mark is affected.  "
        "If a program change is to be permanent, affecting all notes/chords "
        "after the one with the mark (e.g., for a `pizz' or `arco' mark), "
        "then a `*' character should be appended to the ID string.  "
        "Also, appending `+' or `-' to the ID overrides the value specified in "
        "`midiout-artenv' when this change occurs (`+' indicates `yes' and '-' "
        "indicates 'no').";
    set->typedoc = midioutprogtype;

    module_setval_list(&set->val, 0);

    set->loc = module_locexport;
    set->valid = valid_midioutprog;
    set->uselevel = 2;
    progchangesid = id;
    break;
  }
  case 3: {
    set->name = "midiout-default-dynmark"; // docscat{midiout}
    set->type = module_string;
    set->descdoc = "The default MIDI dynamic level, used if no dynamic marks "
                   "are present in the score or part.  "
                   "Set this to the desired MIDI output level for scores that "
                   "have no dynamic markings.";
    set->typedoc = dynamicstringtype;

    module_setval_string(&set->val, "mf");

    set->loc = module_locpart;
    set->valid = valid_dynamicstringtype;
    set->uselevel = 3;
    defaultdynid = id;
    break;
  }
  case 4: {
    set->name = "midiout-min-velocity"; // docscat{midiout}
    set->type = module_number;
    set->descdoc =
        "The minimum MIDI output velocity, expressed as a number from 0 to 1 "
        "(which is scaled to an actual MIDI velocity value from 0 to 127).  "
        "Setting this to 0, for example, specifies that when the lowest "
        "dynamic mark (setting `midiout-min-dynmark') is encountered, the "
        "output MIDI velocity should be 0.";
    set->typedoc = velocitytype;

    module_setval_int(&set->val, 0);

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    minvelid = id;
    break;
  }
  case 5: {
    set->name = "midiout-max-velocity"; // docscat{midiout}
    set->type = module_number;
    set->descdoc =
        "The maximum MIDI output velocity, expressed as a number from 0 to 1 "
        "(which is scaled to an actual MIDI velocity value from 0 to 127).  "
        "Setting this to 1, for example, specifies that when the highest "
        "dynamic mark (setting `midiout-max-dynmark') is encountered, the "
        "output MIDI velocity should be 127.";
    set->typedoc = velocitytype;

    module_setval_int(&set->val, 1);

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    maxvelid = id;
    break;
  }
  case 6: {
    set->name = "midiout-min-dynmark"; // docscat{midiout}
    set->type = module_string;
    set->descdoc =
        "The dynamic mark used together with the value of "
        "`midiout-min-velocity' to scale MIDI output velocity values.";
    set->typedoc = dynamicstringtype;

    module_setval_string(&set->val, "pppp");

    set->loc = module_locpart;
    set->valid = valid_dynamicstringtype;
    set->uselevel = 3;
    mindynmarkid = id;
    break;
  }
  case 7: {
    set->name = "midiout-max-dynmark"; // docscat{midiout}
    set->type = module_string;
    set->descdoc =
        "The dynamic mark used together with the value of "
        "`midiout-max-velocity' to scale MIDI output velocity values.";
    set->typedoc = dynamicstringtype;

    module_setval_string(&set->val, "fff");

    set->loc = module_locpart;
    set->valid = valid_dynamicstringtype;
    set->uselevel = 3;
    maxdynmarkid = id;
    break;
  }
  case 8: {
    set->name = "midiout-time-variation"; // docscat{midiout}
    set->type = module_number;
    set->descdoc =
        "Approximately the amount of time (in seconds) that MIDI note onsets "
        "and offsets can be randomly adjusted with in an attempt to make the "
        "output sound more \"natural\".  "
        "(The adjustment is made using a Gaussian random generator with this "
        "setting value as the standard deviation "
        "and 1.5 times this setting value as the maximum allowed deviation.)"
        "  Increasing this value increases randomness in the output.  It might "
        "also be worthwhile to increase `midiout-deltatime' to a finer "
        "resolution so that the output doesn't sound quantized.";
    set->typedoc = velocitytype;

    module_setval_int(&set->val, 0); // 0.01

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    timevarid = id;
    break;
  }
  case 9: {
    set->name = "midiout-vel-variation"; // docscat{midiout}
    set->type = module_number;
    set->descdoc =
        "Approximately the amount (on a scale from 0 to 1) that MIDI note "
        "velocities can be randomly adjusted in an attempt to make the output "
        "sound more \"natural\".  "
        "(The adjustment is made using a Gaussian random generator with this "
        "setting value as the standard deviation "
        "and 1.5 times this setting value as the maximum allowed deviation.)  "
        "Increasing this value increases randomness in the output.";
    set->typedoc = velocitytype;

    module_setval_int(&set->val, 0); // 1/(fomus_float)25

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    velvarid = id;
    break;
  }
  case 10: {
    set->name = "midiout-sfz-velocity"; // docscat{midiout}
    set->type = module_number;
    set->descdoc = "The minimum amount of additional MIDI velocity (velocity "
                   "being on a scale from 0 to 1) required to achieve the "
                   "effect of a `szf' or any similar marking.  "
                   "Set this to the amount of velocity increase you might "
                   "expect from a `szf' mark in a loud passage.  "
                   "This setting exists to insure that the effect of `szf' "
                   "marks are always heard in the output regardless of the "
                   "current overall dynamic level.";
    set->typedoc = velocitytype;

    module_setval_rat(&set->val, module_makerat(1, 8));

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    sfzvelid = id;
    break;
  }
  case 11: {
    set->name = "midiout-staccato-mult"; // docscat{midiout}
    set->type = module_number;
    set->descdoc = "The amount to multiply the duration of a MIDI note by when "
                   "there is a staccato mark attached to it.  "
                   "Set this to the ratio of a staccato note duration to a "
                   "non-staccato note duration.";
    set->typedoc = multtype;

    module_setval_rat(&set->val, module_makerat(1, 2));

    set->loc = module_locpart;
    set->valid = valid_multtype;
    set->uselevel = 3;
    staccmultid = id;
    break;
  }
  case 12: {
    set->name = "midiout-accent-velocity"; // docscat{midiout}
    set->type = module_number;
    set->descdoc = "The amount of velocity (on a scale from 0 to 1) to add "
                   "when a MIDI note has an accent mark attached to it.  "
                   "Set this to a level of increase appropriate for an accent.";
    set->typedoc = velocitytype;

    module_setval_rat(&set->val, module_makerat(1, 8));

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    accentvelid = id;
    break;
  }
  case 13: {
    set->name = "midiout-staccatissimo-mult"; // docscat{midiout}
    set->type = module_number;
    set->descdoc = "The amount to multiply the duration of a MIDI note by when "
                   "there is a staccatissimo mark attached to it.  "
                   "Set this to the ratio of a staccatissimo note duration to "
                   "a non-staccato note duration.";
    set->typedoc = multtype;

    module_setval_rat(&set->val, module_makerat(1, 4));

    set->loc = module_locpart;
    set->valid = valid_multtype;
    set->uselevel = 3;
    staccatissimomultid = id;
    break;
  }
  case 14: {
    set->name = "midiout-tenuto-velocity"; // docscat{midiout}
    set->type = module_number;
    set->descdoc =
        "The amount of velocity to add when a MIDI note has a tenuto mark "
        "attached to it.  "
        "Set this to a level of increase appropriate for a tenuto mark.";
    set->typedoc = velocitytype;

    module_setval_rat(&set->val, module_makerat(1, 16));

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    tenutovelid = id;
    break;
  }
  case 15: {
    set->name = "midiout-tenuto-add"; // docscat{midiout}
    set->type = module_number;
    set->descdoc =
        "The maximum amount of time (in seconds) to add to the duration of a "
        "MIDI note when there is a tenuto mark attached to it.  "
        "Set this to a value representing the extra duration a note with a "
        "tenuto mark should be held.";
    set->typedoc = velocitytype;

    // module_setval_rat(&set->val, module_makerat(5, 4));
    module_setval_float(&set->val, 0.1);

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    tenutoaddid = id;
    break;
  }
  case 16: {
    set->name = "midiout-marcato-mult"; // docscat{midiout}
    set->type = module_number;
    set->descdoc = "The amount to multiply the duration of a MIDI note by when "
                   "there is a marcato mark attached to it.  "
                   "Set this to the ratio of a marcato note duration to a "
                   "non-marcato note duration.  "
                   "(The velocity is also altered as if there were a `sfz' "
                   "mark attached to it.)";
    set->typedoc = multtype;

    module_setval_rat(&set->val, module_makerat(2, 3));

    set->loc = module_locpart;
    set->valid = valid_multtype;
    set->uselevel = 3;
    marcatomultid = id;
    break;
  }
  case 17: {
    set->name = "midiout-slur-add"; // docscat{midiout}
    set->type = module_number;
    set->descdoc = "The maximum amount of time (in seconds) to add to the "
                   "duration of a MIDI note when there is a slur over it.  "
                   "Set this to a value representing the extra duration a "
                   "slurred note should be held.";
    // "The purpose of this is to prevent output that sounds incorrect due to
    // notes being held unevenly or for too long.";
    set->typedoc = velocitytype;

    module_setval_float(&set->val, 0.1);

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    sluraddid = id;
    break;
  }
  // case 17:
  //   {
  //     set->name = "midiout-slur-mult"; // docscat{midiout}
  //     set->type = module_number;
  //     set->descdoc = "The amount to multiply the duration of a MIDI note by
  //     when there is a slur over it.  "
  // 	"Set this to the ratio of a slurred note duration to a non-slurred note
  // duration.  " 	"An increase in duration causes pitches to overlap and create
  // a slurred effect.";
  //     set->typedoc = multtype;

  //     module_setval_rat(&set->val, module_makerat(5, 4));

  //     set->loc = module_locpart;
  //     set->valid = valid_multtype;
  //     set->uselevel = 3;
  //     slurmultid = id;
  //     break;
  //   }
  case 18: {
    set->name = "midiout-mezzostacc-mult"; // docscat{midiout}
    set->type = module_number;
    set->descdoc = "The amount to multiply the duration of a MIDI note by when "
                   "there is a mezzo staccato articulation attached to it.  "
                   "Set this to the ratio of a mezzo staccato note duration to "
                   "a non-mezzo-staccato note duration.";
    set->typedoc = multtype;

    module_setval_rat(&set->val, module_makerat(3, 4));

    set->loc = module_locpart;
    set->valid = valid_multtype;
    set->uselevel = 3;
    mezzostaccmultid = id;
    break;
  }
  case 19: {
    set->name = "midiout-attack"; // docscat{midiout}
    set->type = module_number;
    set->descdoc =
        "The duration (in seconds) of the attack portion of a MIDI instrument "
        "sound.  "
        "The MIDI channel volume is increased for notes with certain marks "
        "during this interval as a crude way of faking an accent."
        "  Set this globally or in an instrument definition to a value that "
        "sounds appropriate for the instrument.";
    set->typedoc = velocitytype;

    module_setval_float(&set->val, 0.1);

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    attacktimeid = id;
    break;
  }
  case 20: {
    set->name = "midiout-release"; // docscat{midiout}
    set->type = module_number;
    set->descdoc =
        "The duration (in seconds) of the release portion of a MIDI instrument "
        "sound.  "
        "The MIDI channel volume is gradually decreased for this duration "
        "after the attack interval specified in `midiout-attack'."
        "  Set this globally or in an instrument definition to a value that "
        "sounds appropriate for the instrument.";
    set->typedoc = velocitytype;

    module_setval_float(&set->val, 0.1);

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    releasetimeid = id;
    break;
  }
  case 21: {
    set->name = "midiout-ctrldelta"; // docscat{midiout}
    set->type = module_number;
    set->descdoc = "The delta time increment (in seconds) used when a "
                   "continuous change of any kind is created in a MIDI file.  "
                   "Continouous changes include crescendo/diminuendo wedges "
                   "and attack/release sound adjustments.  "
                   "Decreasing this value increases the time resolution of the "
                   "MIDI messages that execute these changes.";
    set->typedoc = velocitytype;

    module_setval_float(&set->val, 0.05);

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    deltatimeid = id;
    break;
  }
  case 22: {
    set->name = "midiout-deltatime"; // docscat{midiout}
    set->type = module_int;
    set->descdoc = "The delta time increment (or so-called \"pulses per "
                   "quarter note\") for the MIDI file, expressed as the number "
                   "of time increments per beat.  "
                   "All MIDI events are quantized to this delta time value.  "
                   "Set this to a higher value to increase the time resolution "
                   "of the entire MIDI file.";
    set->typedoc = ppqntype;

    module_setval_int(&set->val, /*96*/ 960);

    set->loc = module_locscore;
    set->valid = valid_ppqn;
    set->uselevel = 3;
    ppqnid = id;
    break;
  }
  case 23: {
    set->name = "midiout-percchannels"; // docscat{midiout}
    set->type = module_list_numlists;
    set->descdoc =
        "A list of channels to be used as MIDI percussion channels.  "
        "This is a list of lists so that different configurations may be "
        "specified for different MIDI output ports.  "
        "The first list specifies percussion channels for the first MIDI "
        "output port, etc..  The last list is repeated for any additional "
        "ports.  "
        "Channels numbers range from 0 to 15.";
    set->typedoc = percchstype;

    module_setval_list(&set->val, 1);
    struct module_list& li = set->val.val.l;
    module_setval_list(li.vals, 1);
    module_setval_int(li.vals[0].val.l.vals, 9);

    set->loc = module_locscore;
    set->valid = valid_percchstype;
    set->uselevel = 3;
    percchsid = id;
    break;
  }
  case 24: {
    set->name = "midiout-usenoteoffs"; // docscat{midiout}
    set->type = module_bool;
    set->descdoc =
        "If set to yes, use actual note-off events in MIDI output files.  "
        "If set to no, use note-on events with 0 velocities.  "
        "Most applications seem to expect the latter.";
    // set->typedoc = percchstype;

    module_setval_int(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_percchstype;
    set->uselevel = 3;
    usenoteoffsid = id;
    break;
  }
  case 25: {
    set->name = "midiout-artenv"; // docscat{midiout}
    set->type = module_bool;
    set->descdoc =
        "Indicates whether or not a tone produced by this instrument can "
        "change in volume after the attack portion of the sound.  "
        "For example, the appropriate value is 'yes' for wind and string "
        "instruments and 'no' for piano, plucked and percussion instruments.  "
        "Use this setting to specify how FOMUS should simulate (using the "
        "track volume control) various articulations such as accents for this "
        "instrument.";
    // set->typedoc = midioutnotetype;

    module_setval_int(&set->val, 0);

    set->loc = module_locexport;
    // set->valid = valid_midioutnote;
    set->uselevel = 3;
    envcanchangeid = id;
    break;
  }
  case 26: {
    set->name = "midiout-chordvoicing-velocity"; // docscat{midiout}
    set->type = module_number;
    set->descdoc = "The amount of velocity to subtract from the notes of a "
                   "chord that aren't at the top.  "
                   "This adjustment is applied only to chords in a single "
                   "voice (i.e., individual voices are processed separately).  "
                   "Set this to balance the volume of individual notes in "
                   "chords for instruments that play them.";
    set->typedoc = velocitytype;

    module_setval_rat(&set->val, module_makerat(1, 16));

    set->loc = module_locpart;
    set->valid = valid_velocitytype;
    set->uselevel = 3;
    chordbalid = id;
    break;
  }
  case 27: {
    set->name = "midiout-multivoice-marks"; // docscat{midiout}
    set->type = module_list_nums;
    set->descdoc = "A list of voices containing marks that affect only events "
                   "in the voice that they are in.  "
                   "FOMUS assumes marks such as dynamic marks and wedges "
                   "affect all voices in a part if the mark if found at the "
                   "bottom of a staff or in the middle of a grand staff.  "
                   "This assumption isn't always accurate.  "
                   "Use this setting to help FOMUS interpret these marks "
                   "correctly and produce more accurate MIDI output by listing "
                   "all voices that contain their own dynamic/wedge marks.";
    set->typedoc = multivoicetype;

    module_setval_list(&set->val, 0);

    set->loc = module_locnote;
    set->valid = valid_multivoicetype;
    set->uselevel = 3;
    partallid = id;
    break;
  }
  case 28: {
    set->name = "midiout-gracedur"; // docscat{midiout}
    set->type = module_number;
    set->descdoc = "The duration of a grace note, in beats.  "
                   "Set this to an appropriate value so that FOMUS can output "
                   "MIDI grace notes properly.";
    set->typedoc = gracedurtype;

    module_setval_rat(&set->val, module_makerat(1, 6));

    set->loc = module_locnote;
    set->valid = valid_gracedurtype;
    set->uselevel = 2;
    gracedurid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}

void module_ready() {
  transposeid = module_settingid("transpose");
  if (transposeid < 0) {
    ierr = "missing required setting `transpose'";
    return;
  }
  sulstyleid = module_settingid("sul-style");
  if (sulstyleid < 0) {
    ierr = "missing required setting `sul-style'";
    return;
  }
  openstringsid = module_settingid("open-strings");
  if (openstringsid < 0) {
    ierr = "missing required setting `open-strings'";
    return;
  }
  beatid = module_settingid("beat");
  if (beatid < 0) {
    ierr = "missing required setting `beat'";
    return;
  }
  compid = module_settingid("comp");
  if (compid < 0) {
    ierr = "missing required setting `comp'";
    return;
  }
}
fomus_bool modout_ispre() {
  return false;
}

const char* modout_get_extension(int n) {
  switch (n) {
  case 0:
    return "mid"; // make this a setting
  case 1:
    return "midi"; // make this a setting
  default:
    return 0;
  }
}

const char* modout_get_saveid() {
  return 0;
}
void modout_write(FOMUS f, void* dat, const char* filename) {
  ((midioutdata*) dat)->modout_write(f, filename);
}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
