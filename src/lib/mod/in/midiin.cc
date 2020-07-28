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

// FOMUS FILE INPUT

#include "config.h"

#include <boost/cstdint.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include <cassert>
#include <limits>
#include <set>
#include <sstream>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "fomusapi.h"

#include "debugaux.h"
#include "ferraux.h"
using namespace ferraux;
#include "foutaux.h"
using namespace foutaux;

#ifdef BOOST_FILESYSTEM_OLDAPI
#define FS_COMPLETE boost::filesystem::complete
#define FS_FILE_STRING file_string
#define FS_BASENAME(xxx) boost::filesystem::basename(xxx)
#else
#define FS_COMPLETE boost::filesystem::absolute
#define FS_FILE_STRING string
#define FS_BASENAME(xxx) xxx.stem().string()
#endif

namespace midiin {

  const char* ierr = 0;

  template <typename I, typename T>
  inline bool hassome(I first, const I& last, T& pred) {
    while (first != last)
      if (pred(*first++))
        return true;
    return false;
  }

  int trackid, drumtrackid, chanid, progid, noteid, minid, majid, scaleid,
      keysigid, setpercid, setvoiceid, tracknameid, seqnameid, filenamematchid,
      innameid, seqnumsid, tposeid, verboseid;

  struct errbase {};
  struct badmidi {};

  struct midiindata {
    // FOMUS fom;
    std::string maj, min;
    midiindata(FOMUS fom) {
      module_value x(module_setting_val(fom, majid));
      assert(x.type == module_list);
      if (x.val.l.n > 0) {
        assert(x.val.l.vals->type == module_string);
        maj = x.val.l.vals->val.s;
      }
      module_value y(module_setting_val(fom, minid));
      assert(y.type == module_list);
      if (y.val.l.n > 0) {
        assert(y.val.l.vals->type == module_string);
        min = y.val.l.vals->val.s;
      }
    }
    bool modin_load(FOMUS fom, const char* filename, const bool isfile);
  };

  uint_fast32_t readvar(std::istream& in) {
    uint_fast32_t ret = 0;
    int i, c = 0;
    do {
      i = in.get();
      ret = (ret << 7) | (i & 0x7F);
      if (++c > 4)
        throw badmidi();
    } while (i & 0x80);
    return ret;
  }

  inline uint_fast8_t read1(std::istream& in) {
    return in.get();
  }
  inline uint_fast16_t read2(std::istream& in) {
    uint_fast16_t ret = in.get();
    return (ret << 8) | in.get();
  }
  inline uint_fast32_t read3(std::istream& in) {
    uint_fast32_t ret = in.get();
    ret = (ret << 8) | in.get();
    return (ret << 8) | in.get();
  }
  inline uint_fast32_t read4(std::istream& in) {
    uint_fast32_t ret = in.get();
    ret = (ret << 8) | in.get();
    ret = (ret << 8) | in.get();
    return (ret << 8) | in.get();
  }

  inline std::string readstr(std::istream& in, const std::streamsize n) {
    static std::vector<char> buf;
    buf.resize(n);
    in.read(&buf[0], n);
    return std::string(buf.begin(), buf.end());
  }

  enum evtype {
    ev_noteoff,
    ev_noteon,
    ev_prog,
    ev_bend,
    ev_tempo,
    ev_timesig,
    ev_keysig
  };

  struct mevent {
    long tim;
    evtype typ;
    int tr, ch;
    uint_fast32_t val1, val2;
    std::string trname, inname;
    mevent(const long tim, const evtype typ, const int tr, const int ch,
           const uint_fast32_t val1, const uint_fast32_t val2)
        : tim(tim), typ(typ), tr(tr), ch(ch), val1(val1), val2(val2) {}
    void set(const std::string& tr, const std::vector<std::string>& in) {
      trname = tr;
      assert(ch >= -1 && ch <= 15);
      assert(in.size() == 16);
      if (ch >= 0)
        inname = in[ch];
    }
  };
  inline bool operator<(const mevent& x, const mevent& y) {
    return x.tim < y.tim;
  }

  inline void fillup(std::set<int>& x, const module_obj imp, const int id) {
    module_value l(module_setting_val(imp, id));
    assert(l.type == module_list);
    for (module_value *i = l.val.l.vals, *ie = l.val.l.vals + l.val.l.n; i < ie;
         ++i)
      x.insert(GET_I(*i));
  }
  struct importstr {
    std::string pname;
    std::set<int> trs, chs, prgs, nos, dtrs, sqns; // dtrs = drum tracks
    std::string trackname, seqname, filename, inname;
    std::string setperc;
    std::vector<int> setvoices;
    fomus_float tpose;
    importstr(const module_obj imp, const std::string& p)
        : pname(p), trackname(module_setting_sval(imp, tracknameid)),
          seqname(module_setting_sval(imp, seqnameid)),
          filename(module_setting_sval(imp, filenamematchid)),
          inname(module_setting_sval(imp, innameid)),
          setperc(module_setting_sval(imp, setpercid)),
          tpose(module_setting_fval(imp, tposeid)) {
      fillup(trs, imp, trackid);
      fillup(chs, imp, chanid);
      fillup(prgs, imp, progid);
      fillup(nos, imp, noteid);
      fillup(dtrs, imp, drumtrackid);
      fillup(sqns, imp, seqnumsid);
      const module_value l(module_setting_val(imp, setvoiceid));
      assert(l.type == module_list);
      for (module_value *i = l.val.l.vals, *ie = l.val.l.vals + l.val.l.n;
           i < ie; ++i)
        setvoices.push_back(GET_I(*i));
      if (setperc.empty()) {
        const char* x = modin_importpercid(imp);
        if (x)
          setperc = x;
      }
    }
    bool haspartname(const std::string& nm) const {
      return boost::algorithm::ilexicographical_compare(nm, pname);
    }
    void setpartandpitch(FOMUS fom, const int pit) const {
      if (setperc.empty()) { // pitch
        fomus_fval(fom, fomus_par_pitch, fomus_act_set, pit + tpose);
      } else
        fomus_sval(fom, fomus_par_pitch, fomus_act_set, setperc.c_str());
      fomus_act(fom, fomus_par_voice, fomus_act_clear);
      for (std::vector<int>::const_iterator i(setvoices.begin());
           i != setvoices.end(); ++i) {
        fomus_ival(fom, fomus_par_voice, fomus_act_add, *i);
      }
      fomus_sval(fom, fomus_par_part, fomus_act_set, pname.c_str());
      fomus_act(fom, fomus_par_noteevent, fomus_act_add);
    }
    bool midimatches(const int track, const int channel, const int prog,
                     const int note, const std::string& trackname0,
                     const std::string& seqname0, const std::string& filename0,
                     const std::string& inname0) const {
      return ((trs.empty() || trs.find(track) != trs.end()) &&
              (chs.empty() || chs.find(channel) != chs.end()) &&
              (prgs.empty() || prgs.find(prog) != prgs.end()) &&
              (nos.empty() || (dtrs.find(channel) != dtrs.end() &&
                               nos.find(note) != nos.end())) &&
              (sqns.empty() || (dtrs.find(channel) != dtrs.end() &&
                                sqns.find(note) != sqns.end())) &&
              (trackname.empty() || trackname0.empty() ||
               trackname == trackname0) &&
              (seqname.empty() || seqname0.empty() || seqname == seqname0) &&
              (inname.empty() || inname0.empty() || inname == inname0) &&
              (filename.empty() || filename == filename0));
    }
  };

  bool midiindata::modin_load(FOMUS fom, const char* filename,
                              const bool isfile) {
    if (isfile) {
      try {
        boost::filesystem::path fn(filename);
        std::string basefn(FS_BASENAME(fn));
        boost::filesystem::ifstream in;
        try {
          in.exceptions(boost::filesystem::ifstream::eofbit |
                        boost::filesystem::ifstream::failbit |
                        boost::filesystem::ifstream::badbit);
          in.open(fn.FS_FILE_STRING(), boost::filesystem::ifstream::in);
          if (read4(in) != 0x4D546864) {
            CERR << '`' << fn.FS_FILE_STRING()
                 << "' is not a Standard MIDI File" << std::endl;
            throw errbase();
          }
          unsigned long initl = read4(in);
          if (initl < 6)
            throw badmidi();
          int ft = read2(in); // MIDI file type
          if (ft < 0 || ft >= 2) {
            CERR << '`' << fn.FS_FILE_STRING()
                 << "' is not a Type 0 or 1 MIDI file" << std::endl;
            throw errbase();
          }
          int ntracks = read2(in);
          int tdiv = read2(in);
          double rate;
          bool secs;
          if (tdiv & 0x8000) { // ticks per second
            secs = true;
            int smpte = (tdiv & 0x7F00) >> 8;
            int ticks = (tdiv & 0x00FF);
            switch (smpte) {
            case 24:
            case 25:
            case 30:
              rate = smpte * ticks;
              break;
            case 29:
              rate = 29.97 * ticks;
              break;
            default:
              throw badmidi();
            }
          } else { // ticks per "beat" or "quarter note"
            secs = false;
            rate = tdiv;
          }
          std::vector<mevent> evs;
          int stat = 0;
          int seqnum = 0;      // sequence number
          std::string seqname; // sequence name
          in.ignore(initl - 6);
          for (int tr = 0; tr < ntracks; ++tr) {
            long fr = evs.size();
            std::string trname; // track name
            unsigned long chid = read4(in);
            unsigned long len = read4(in);
            int chprfx = -1;                        // meta event cahnnel prefix
            std::vector<std::string> instnames(16); // for ea. channel
            if (chid == 0x4D54726B) {               // track
              long tim = 0;
              unsigned long upto = (unsigned long) in.tellg() + len;
              while ((unsigned long) in.tellg() < upto) {
                tim += readvar(in);
                int s = read1(in); // status byte (maybe)
                if (s == 0xFF) {
                  int ty = read1(in);
                  long l = readvar(in);
                  switch (ty) {
                  case 0x00: {
                    if (l != 2)
                      throw badmidi();
                    seqnum = read2(in);
                    break;
                  }
                  case 0x03: {
                    if (ft == 0 || tr <= 0) { // sequence name
                      seqname = readstr(in, l);
                    } else { // track name
                      trname = readstr(in, l);
                    }
                    break;
                  }
                  case 0x04: {
                    std::string x(readstr(in, l));
                    if (chprfx < 0) {
                      for (std::vector<std::string>::iterator i(
                               instnames.begin());
                           i != instnames.end(); ++i)
                        *i = x;
                    } else {
                      instnames[chprfx] = x;
                    }
                    break;
                  }
                  case 0x20: { // channel prfx
                    if (l != 1)
                      throw badmidi();
                    chprfx = read1(in);
                    break;
                  }
                  case 0x2F:
                    if (l != 0)
                      throw badmidi();
                    goto TRACKDONE; // end of track
                  case 0x51: {
                    if (l != 3)
                      throw badmidi();
                    evs.push_back(mevent(tim, ev_tempo, -1, -1,
                                         /*60000000.0 /*/ read3(in), 0));
                    break;
                  }
                  case 0x58: {
                    if (l != 4)
                      throw badmidi();
                    int n = read1(in);
                    evs.push_back(
                        mevent(tim, ev_timesig, -1, -1, n, read1(in)));
                    in.ignore(2);
                    break;
                  }
                  case 0x59: {
                    if (l != 2)
                      throw badmidi();
                    int k = read1(in); // flats/sharps
                    evs.push_back(
                        mevent(tim, ev_keysig, -1, -1, (char) k, read1(in)));
                    break;
                  }
                  default:
                    in.ignore(l);
                  }
                } else if (s >= 0xF0) { // F0 or F7
                  in.ignore(readvar(in));
                } else {          // channel message
                  int v;          // v is the first byte
                  if (s & 0x80) { // status byte
                    stat = s;
                    v = read1(in);
                  } else
                    v = s;
                  int ch = (stat & 0x0F);
                  switch ((stat & 0xF0) >> 4) {
                  case 0x8:
                    evs.push_back(
                        mevent(tim, ev_noteoff, tr, ch, v, read1(in)));
                    break;
                  case 0x9: {
                    uint_fast8_t e = read1(in);
                    evs.push_back(mevent(tim, (e ? ev_noteon : ev_noteoff), tr,
                                         ch, v, e));
                    break;
                  }
                  case 0xA: // in.ignore(1); break; // aftertouch  GO TO NEXT
                  case 0xB:
                    in.ignore(1);
                    break; // evs.push_back(mevent(ev_ctrl, tr, ch, v, read(in,
                           // 1))); break;
                  case 0xC:
                    evs.push_back(mevent(tim, ev_prog, tr, ch, v, 0));
                    break;
                  case 0xD:
                    break; // aftertouch
                  case 0xE:
                    evs.push_back(mevent(tim, ev_bend, tr, ch, v, read1(in)));
                    break;
                  default:
                    throw badmidi();
                  }
                }
              }
            TRACKDONE:
              in.ignore(upto - in.tellg());
            } else {
              in.ignore(len);
            }
            for (std::vector<mevent>::iterator i(evs.begin() + fr);
                 i != evs.end(); ++i) {
              i->set(trname, instnames);
            }
          }
          in.close();
          bool verb = (module_setting_ival(fom, verboseid) >= 1);
          std::stable_sort(evs.begin(), evs.end());
          int progs[16];
          const mevent* states[16][128];
          for (int i = 0; i < 16; ++i) {
            progs[i] = 0;
            for (int j = 0; j < 128; ++j) {
              states[i][j] = 0;
            }
          }
          long ltrclicks =
              0; // last time reference (when last tempo change was)
          double ltrtime = 0; // last time ref.
          double tempo = 120;
          // fomus_rat tsig = {4, 4};
          fomus_float scale = module_setting_fval(fom, scaleid);
          long ltim = 0;
          bool insmeas = false;
          fomus_rat measdur = {4, 1};
          int keysig1 = 0, keysig2 = 0; // cmaj
          static int keys[15] = {11, 6, 1, 8, 3,  10, 5, 0,
                                 7,  2, 9, 4, 11, 6,  1};
          static int keyaccs[15] = {-1, -1, -1, -1, -1, -1, 0, 0,
                                    0,  0,  0,  0,  0,  1,  1};
          boost::ptr_list<importstr>
              impmap; // map from import objs to part objs
          boost::ptr_list<importstr>
              impmap2; // map from import objs to inst objs
          struct module_objlist prts(module_get_parts(fom));
          bool areparts;
          if (prts.n <= 0) {
            prts = module_get_insts(fom, 1);
            areparts = false;
          } else
            areparts = true;
          for (const module_obj *x(prts.objs), *xe(prts.objs + prts.n); x < xe;
               ++x) {
            if (module_ismetapart(*x))
              continue;
            const char* id(module_id(*x));
            if (id[0] == 0)
              continue; // zero-length == no id
            struct modin_imports imps(modin_imports(*x));
            for (const module_obj *i = imps.obj, *ie = imps.obj + imps.n;
                 i < ie; ++i) {
              impmap.push_back(
                  new importstr(*i, id)); // includes percussion imports!
            }
          }
          fomus_sval(fom, fomus_par_locfile, fomus_act_set,
                     fn.FS_FILE_STRING().c_str());
          for (std::vector<mevent>::iterator i(evs.begin()); i != evs.end();
               ++i) {
            if (i->tim > ltim) {
              if (insmeas) {
                double t = (ltim - ltrclicks) / rate;
                if (secs)
                  t *= (60.0 / tempo); // presumeably need to convert to beats
                                       // (or "quarternotes") using tempo (?)
                t = ltrtime + t;       // absolute time, supposedly in beats
                fomus_fval(fom, fomus_par_time, fomus_act_set, t * scale);
                // fomus_float d((i->val1 / (fomus_float)i->val2) * 4);
                fomus_rval(fom, fomus_par_duration, fomus_act_set, measdur.num,
                           measdur.den);
                assert(keysig2 >= 0 && keysig2 <= 1);
                assert(keysig1 >= -7 && keysig1 <= 7);
                const char* n = module_pitchnametostr(
                    module_makeval((fomus_int) keys[keysig1 + 7]),
                    keyaccs[keysig1 + 7]);
                if (!n)
                  throw badmidi();
                fomus_ival(fom, fomus_par_setting, fomus_act_set, keysigid);
                fomus_sval(fom, fomus_par_measdef_settingval, fomus_act_set,
                           (std::string(n) + (keysig2 ? min : maj)).c_str());
                fomus_act(fom, fomus_par_meas_measdef, fomus_act_set);
                fomus_act(fom, fomus_par_meas, fomus_act_add);
                insmeas = false;
              }
              ltim = i->tim;
            }
            switch (i->typ) {
            case ev_noteoff: {
              // break;
              const mevent* o = states[i->ch][i->val1];
              if (!o)
                break;
              double t = (o->tim - ltrclicks) / rate;
              if (secs)
                t *= (60.0 / tempo); // presumeably need to convert to beats (or
                                     // "quarternotes") using tempo (?)
              t = ltrtime + t;       // absolute time, supposedly in beats
              fomus_fval(fom, fomus_par_time, fomus_act_set, t * scale);
              double d = (i->tim - o->tim) / rate;
              if (secs)
                d *= (60.0 / tempo);
              fomus_fval(fom, fomus_par_duration, fomus_act_set, d * scale);
              fomus_fval(fom, fomus_par_dynlevel, fomus_act_set,
                         o->val2 / 127.0);
              if (areparts) { // if areparts, then impmap contains user's parts
                for (boost::ptr_list<importstr>::const_iterator j(
                         impmap.begin());
                     j != impmap.end();
                     ++j) { // iterator through user's parts first--can go to
                            // multiple parts
                  if (j->midimatches(o->tr, o->ch, progs[o->ch], o->val1,
                                     o->trname, seqname, basefn, o->inname)) {
                    j->setpartandpitch(fom, o->val1); // and add the note event
                  }
                }
              } else { // iterator through all on-thefly added parts--add only
                       // to first one found--if user didn't specify, only put
                       // event into one part
                for (boost::ptr_list<importstr>::const_iterator j(
                         impmap2.begin());
                     j != impmap2.end(); ++j) { // impmap2 is on the fly
                  if (j->midimatches(o->tr, o->ch, progs[o->ch], o->val1,
                                     o->trname, seqname, basefn, o->inname)) {
                    j->setpartandpitch(fom, o->val1); // and add the note event
                    goto ALLDONEWITHNOTEOFF;
                  }
                }
                // if still nothing found, iterate through all instruments, add
                // a part if one is found
                for (boost::ptr_list<importstr>::iterator j(impmap.begin());
                     j != impmap.end();
                     ++j) { // impmap here is all default instruments
                  if (j->midimatches(o->tr, o->ch, progs[o->ch], o->val1,
                                     o->trname, seqname, basefn, o->inname)) {
                    std::string nm;
                    if (o->trname.empty()) {
                      std::ostringstream x;
                      x << "track" << o->tr;
                      nm = x.str();
                    } else {
                      nm = o->trname;
                    }
                    std::string nm0(nm);
                    int n = 0;
                    while (hassome(
                        impmap2.begin(), impmap2.end(),
                        boost::lambda::bind(&importstr::haspartname,
                                            boost::lambda::_1,
                                            boost::lambda::constant_ref(nm)))) {
                      std::ostringstream x;
                      x << nm0 << '-' << ++n;
                      nm = x.str();
                    }
                    fomus_sval(fom, fomus_par_part_id, fomus_act_set,
                               nm.c_str());
                    fomus_sval(fom, fomus_par_part_inst, fomus_act_set,
                               j->pname.c_str());
                    fomus_act(fom, fomus_par_part, fomus_act_add);
                    if (verb)
                      fout << "adding part `" << nm << "' using instrument `"
                           << j->pname << '\'' << std::endl;
                    j->pname = nm; // pname is now the new part name
                    j->setpartandpitch(fom, o->val1); // and add the note event
                    impmap2.transfer(impmap2.end(), j, impmap);
                    goto ALLDONEWITHNOTEOFF;
                  }
                }
              }
            ALLDONEWITHNOTEOFF:
              states[i->ch][i->val1] = 0;
              break;
            }
            case ev_noteon: {
              assert(i->ch >= 0 && i->ch < 16);
              assert(i->val1 >= 0 && i->val1 < 128);
              if (!states[i->ch][i->val1])
                states[i->ch][i->val1] = (i->val2 > 0 ? &*i : (mevent*) 0);
              break;
            }
            case ev_prog: {
              assert(i->ch >= 0 && i->ch < 16);
              assert(i->val1 >= 0 && i->val1 < 128);
              progs[i->ch] = i->val1;
              break;
            }
            case ev_tempo: {
              double t = (i->tim - ltrclicks) / rate;
              if (secs)
                t *= (60.0 / tempo); // presumeably need to convert to beats (or
                                     // "quarternotes") using tempo (?)
              ltrtime += t;          // update time values for this tempo change
              ltrclicks = i->tim;
              tempo = 60000000.0 / i->val1;
              break;
            }
            case ev_timesig: {
              insmeas = true;
              measdur = module_makerat(i->val1, ((fomus_int) 1) << i->val2) *
                        (fomus_int) 4;
              break;
            }
            case ev_keysig: {
              insmeas = true;
              keysig1 = i->val1;
              keysig2 = i->val2;
              break;
            }
            case ev_bend: {
#warning "finish"
              break;
            }
            }
          }
        } catch (const std::ifstream::failure& e) {
          CERR << "error reading `" << fn.FS_FILE_STRING() << '\'' << std::endl;
          return true;
        } catch (const badmidi& e) {
          CERR << "error reading `" << fn.FS_FILE_STRING() << '\'' << std::endl;
          return true;
        }
      } catch (const boost::filesystem::filesystem_error& e) {
        CERR << "invalid path/filename `" << filename << '\'' << std::endl;
        return true;
      } catch (const errbase& e) { return true; }
    } else {
      CERR << "MIDI string parsing not supported" << std::endl;
      return true;
    }
    return false;
  }

  const char* midiintracktype =
      "integer0..127 | (integer0..127 integer0..127 ...)";
  int valid_midiintrack(const struct module_value val) {
    return module_valid_listofints(val, -1, -1, 0, module_incl, 128,
                                   module_excl, 0, midiintracktype);
  }
  const char* midiinchtype = "integer0..15 | (integer0..15 integer0..15 ...)";
  int valid_midiinch(const struct module_value val) {
    return module_valid_listofints(val, -1, -1, 0, module_incl, 16, module_excl,
                                   0, midiinchtype);
  }
  const char* midiinprogtype =
      "integer0..127 | (integer0..127 integer0..127 ...)";
  int valid_midiinprog(const struct module_value val) {
    return module_valid_listofints(val, -1, -1, 0, module_incl, 128,
                                   module_excl, 0, midiinprogtype);
  }
  const char* midiinnotetype =
      "integer0..127 | (integer0..127 integer0..127 ...)";
  int valid_midiinnote(const struct module_value val) {
    return module_valid_listofints(val, -1, -1, 0, module_incl, 128,
                                   module_excl, 0, midiinnotetype);
  }
  const char* midiinsetvoicetype =
      "integer1..128 | (integer1..128 integer1..128 ...)";
  int valid_midiinsetvoice(const struct module_value val) {
    return module_valid_listofints(val, -1, -1, 1, module_incl, 128,
                                   module_incl, 0, midiinsetvoicetype);
  }
  const char* midiinnotetimescaletype = "number_>0";
  int valid_midiinscale(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) 0), module_excl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            midiinnotetimescaletype);
  }
  const char* midiinseqnumtype =
      "integer0..65535 | (integer0..65535 integer0..65535 ...)";
  int valid_midiinseqnum(const struct module_value val) {
    return module_valid_listofints(val, -1, -1, 0, module_incl, 65535,
                                   module_incl, 0, midiinseqnumtype);
  }
  const char* midiintposetype =
      "integer-127..127 | (integer-127..127 integer-127..127 ...)";
  int valid_midiintpose(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) -127), module_incl,
                            module_makeval((fomus_int) 127), module_incl, 0,
                            midiintposetype);
  }

} // namespace midiin

using namespace midiin;

void* module_newdata(FOMUS f) {
  return new midiindata(f);
}
void module_freedata(void* dat) {
  delete (midiindata*) dat;
}
const char* module_err(void* dat) {
  return 0;
}
const char* module_initerr() {
  return ierr;
}

// int module_priority() {return 0;}
enum module_type module_type() {
  return module_modinput;
}

void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "midiin-track"; // docscat{midiin}
    set->type = module_list_nums;
    set->descdoc = "Indicates which MIDI track or tracks input is read from "
                   "(an empty list indicates all tracks)."
                   "  Put this setting inside an import object to specify how "
                   "incoming MIDI data is matched and inserted into parts."
                   "  Lookup import objects in the FOMUS documentation for "
                   "more information.";
    set->typedoc = midiintracktype;

    module_setval_list(&set->val, 0);

    set->loc = module_locimport;
    set->valid = valid_midiintrack;
    set->uselevel = 2;
    trackid = id;
    break;
  }
  case 1: {
    set->name = "midiin-ch"; // docscat{midiin}
    set->type = module_list_nums;
    set->descdoc = "Indicates which MIDI channel or channels input is read "
                   "from (an empty list indicates all channels)"
                   "  Put this setting inside an import object to specify how "
                   "incoming MIDI data is matched and inserted into parts."
                   "  Lookup import objects in the FOMUS documentation for "
                   "more information.";
    set->typedoc = midiinchtype;

    module_setval_list(&set->val, 0);

    set->loc = module_locimport;
    set->valid = valid_midiinch;
    set->uselevel = 2;
    chanid = id;
    break;
  }
  case 2: {
    set->name = "midiin-prog"; // docscat{midiin}
    set->type = module_list_nums;
    set->descdoc = "Indicates a MIDI program change number or numbers."
                   "  Put this setting inside an import object to specify how "
                   "incoming MIDI data is matched and inserted into parts."
                   "  Lookup import objects in the FOMUS documentation for "
                   "more information.";
    set->typedoc = midiinprogtype;

    module_setval_list(&set->val, 0);

    set->loc = module_locimport;
    set->valid = valid_midiinprog;
    set->uselevel = 2;
    progid = id;
    break;
  }
  case 3: {
    set->name = "midiin-note"; // docscat{midiin}
    set->type = module_list_nums;
    set->descdoc =
        "Indicates a MIDI note or notes for importing percussion instruments "
        "(the MIDI message must also belong to a drum channel)."
        "  Put this setting inside an import object to specify how incoming "
        "MIDI data is matched and inserted into parts."
        "  Lookup import objects in the FOMUS documentation for more "
        "information.";
    set->typedoc = midiinnotetype;

    module_setval_list(&set->val, 0);

    set->loc = module_locimport;
    set->valid = valid_midiinnote;
    set->uselevel = 2;
    noteid = id;
    break;
  }
  case 4: {
    set->name = "midiin-setperc"; // docscat{midiin}
    set->type = module_string;
    set->descdoc =
        "Specifies which percussion instrument receives a MIDI event if the "
        "import object is a match."
        "  Use this setting if the import object is not inside a percussion "
        "instrument or you wish to override the default behavior."
        "  (The default behavior is that percussion instruments receive MIDI "
        "data that matches any of their enclosed import objects.)";
    // set->typedoc = midiinnotetype;

    module_setval_string(&set->val, "");

    set->loc = module_locimport;
    // set->valid = valid_midiinnote;
    set->uselevel = 2;
    setpercid = id;
    break;
  }
  case 5: {
    set->name = "midiin-setvoice"; // docscat{midiin}
    set->type = module_list_nums;
    set->descdoc = "Specifies which voice (or voice amoung several voices) a "
                   "MIDI event is assigned."
                   "  Put this setting inside an import object to specify how "
                   "incoming MIDI data is matched and inserted into parts."
                   "  Lookup import objects in the FOMUS documentation for "
                   "more information.";
    set->typedoc = midiinsetvoicetype;

    module_setval_list(&set->val, 1);
    module_setval_int(set->val.val.l.vals, 1);

    set->loc = module_locimport;
    set->valid = valid_midiinsetvoice;
    set->uselevel = 2;
    setvoiceid = id;
    break;
  }
  case 6: {
    set->name = "midiin-timescale"; // docscat{midiin}
    set->type = module_number;
    set->descdoc =
        "If set to anything other than 1, specifies that MIDI times and "
        "durations be scaled by that amount."
        "  A value of 2 doubles all incoming times and durations, for example.";
    set->typedoc = midiinnotetimescaletype;

    module_setval_int(&set->val, 1);

    set->loc = module_locscore;
    set->valid = valid_midiinscale;
    set->uselevel = 2;
    scaleid = id;
    break;
  }
  case 7: {
    set->name = "midiin-drumchannel"; // docscat{midiin}
    set->type = module_list_nums;
    set->descdoc = "Specifies which MIDI channel or channels are considered to "
                   "be drum channels."
                   "  When a MIDI event belongs to one of these channels, "
                   "`midiin-note' can be used to select and assign specific "
                   "notes to percussion instruments."
                   "  MIDI channels are numbered from 0 to 15 here, so channel "
                   "9 is the standard MIDI file drum channel.";
    set->typedoc = midiintracktype;

    module_setval_list(&set->val, 1);
    module_setval_int(set->val.val.l.vals, 9);

    set->loc = module_locimport;
    set->valid = valid_midiintrack;
    set->uselevel = 2;
    drumtrackid = id;
    break;
  }
  case 8: {
    set->name = "midiin-seqname"; // docscat{midiin}
    set->type = module_string;
    set->descdoc = "Specifies that MIDI events belong to a sequence whose name "
                   "matches this value be selected for import."
                   "  A Type 1 or 2 MIDI file only has one sequence name, so "
                   "this setting effectively distinguishes between MIDI files "
                   "with different sequence names."
                   "  Put this setting inside an import object to specify how "
                   "incoming MIDI data is matched and inserted into parts."
                   "  Lookup import objects in the FOMUS documentation for "
                   "more information.";
    // set->typedoc = midiinnotetype;

    module_setval_string(&set->val, "");

    set->loc = module_locimport;
    // set->valid = valid_midiinnote;
    set->uselevel = 2;
    seqnameid = id;
    break;
  }
  case 9: {
    set->name = "midiin-trackname"; // docscat{midiin}
    set->type = module_string;
    set->descdoc = "Specifies that MIDI events belong to a track whose name "
                   "matches this value be selected for import."
                   "  Put this setting inside an import object to specify how "
                   "incoming MIDI data is matched and inserted into parts."
                   "  Lookup import objects in the FOMUS documentation for "
                   "more information.";
    // set->typedoc = midiinnotetype;

    module_setval_string(&set->val, "");

    set->loc = module_locimport;
    // set->valid = valid_midiinnote;
    set->uselevel = 2;
    tracknameid = id;
    break;
  }
  case 10: {
    set->name = "midiin-filename"; // docscat{midiin}
    set->type = module_string;
    set->descdoc =
        "Specifies that MIDI events belong to a sequence whose base filename "
        "(without path or extension) matches this value be selected for import."
        "  This setting effectively distinguishes between MIDI files with "
        "different filenames."
        "  Put this setting inside an import object to specify how incoming "
        "MIDI data is matched and inserted into parts."
        "  Lookup import objects in the FOMUS documentation for more "
        "information.";
    // set->typedoc = midiinnotetype;

    module_setval_string(&set->val, "");

    set->loc = module_locimport;
    // set->valid = valid_midiinnote;
    set->uselevel = 2;
    filenamematchid = id;
    break;
  }
  case 11: {
    set->name = "midiin-instname"; // docscat{midiin}
    set->type = module_string;
    set->descdoc = "Specifies that MIDI events belong to an instrument whose "
                   "name matches this value be selected for import."
                   "  This is just a text string associated with a certain "
                   "MIDI track and isn't related to program changes."
                   "  Put this setting inside an import object to specify how "
                   "incoming MIDI data is matched and inserted into parts."
                   "  Lookup import objects in the FOMUS documentation for "
                   "more information.";
    // set->typedoc = midiinnotetype;

    module_setval_string(&set->val, "");

    set->loc = module_locimport;
    // set->valid = valid_midiinnote;
    set->uselevel = 2;
    innameid = id;
    break;
  }
  case 12: {
    set->name = "midiin-seqnum"; // docscat{midiin}
    set->type = module_list_nums;
    set->descdoc =
        "Specifies that MIDI events belong to a sequence whose number matches "
        "this value be selected for import."
        "  Sequence number starts at 0 here, like all other MIDI values."
        "  A Type 1 or 2 MIDI file only has one sequence, so this setting "
        "effectively distinguishes between MIDI files with different sequence "
        "names."
        "  Put this setting inside an import object to specify how incoming "
        "MIDI data is matched and inserted into parts."
        "  Lookup import objects in the FOMUS documentation for more "
        "information.";
    set->typedoc = midiinseqnumtype;

    module_setval_list(&set->val, 0);

    set->loc = module_locimport;
    set->valid = valid_midiinseqnum;
    set->uselevel = 2;
    seqnumsid = id;
    break;
  }
  case 13: {
    set->name = "midiin-tpose"; // docscat{midiin}
    set->type = module_number;
    set->descdoc = "Specifies that MIDI events matching the import structure "
                   "be transposed before being inserted into a part."
                   "  The value is the transposition in semitones.";
    set->typedoc = midiintposetype;

    module_setval_int(&set->val, 0);

    set->loc = module_locimport;
    set->valid = valid_midiintpose;
    set->uselevel = 2;
    tposeid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}

void module_ready() {
  minid = module_settingid("keysig-minor-symbol");
  if (minid < 0) {
    ierr = "missing required setting `keysig-minor-symbol'";
    return;
  }
  majid = module_settingid("keysig-major-symbol");
  if (majid < 0) {
    ierr = "missing required setting `keysig-major-symbol'";
    return;
  }
  keysigid = module_settingid("keysig");
  if (keysigid < 0) {
    ierr = "missing required setting `keysig'";
    return;
  }
  verboseid = module_settingid("verbose");
  if (verboseid < 0) {
    ierr = "missing required setting `verbose'";
    return;
  }
}

const char* modin_get_extension(int n) {
  switch (n) {
  case 0:
    return "mid"; // make this a setting
  case 1:
    return "midi"; // make this a setting
    // case 2: return "smf";
  default:
    return 0;
  }
}
const char* modin_get_loadid() {
  return 0;
}

int modin_load(FOMUS fom, void* dat, const char* filename, int isfile) {
  return ((midiindata*) dat)->modin_load(fom, filename, isfile);
}

const char* module_longname() {
  return "MIDI File Input";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Reads in MIDI files.";
}

//     static int tsigtab[][] = {{-1, -1, -1, -1, -1, -1, -1},
// 			      {-1, -1, -1,  0, -1, -1, -1},
// 			      { 0, -1, -1,  0, -1, -1, -1},
// 			      { 0, -1, -1,  0,  0, -1, -1},
// 			      { 0,  0, -1,  0,  0, -1, -1},
// 			      { 0,  0, -1,  0,  0,  0, -1},
// 			      { 0,  0,  0,  0,  0,  0, -1},
// 			      { 0,  0,  0,  0,  0,  0,  0}, // =  0
// 			      { 0,  0,  0,  1,  0,  0,  0},
// 			      { 1,  0,  0,  1,  0,  0,  0},
// 			      { 1,  0,  0,  1,  1,  0,  0},
// 			      { 1,  1,  0,  1,  1,  0,  0},
// 			      { 1,  1,  0,  1,  1,  1,  0},
// 			      { 1,  1,  1,  1,  1,  1,  0},
// 			      { 1,  1,  1,  1,  1,  1,  1},
// /usr/local/src/pure-data/branches/pd-extended/0.41/doc/media/kanon.mid
// /home/david/mytmp/record1.mid
