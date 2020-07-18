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

// output a .fms score file
#include "config.h"

// can't use my special strings unless I do this:
//#define BOOST_FILESYSTEM_NARROW_ONLY

#include <cctype> // isalpha
#include <new>
#include <sstream>
//#include <cmath>
#include <algorithm>
#include <cstring> // strlen
#include <ctime>   // need this
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <utility> // pair
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>
//#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include <boost/integer/common_factor_rt.hpp>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <ostream>

#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/utility.hpp>

#include <boost/iostreams/concepts.hpp> // sink
#include <boost/iostreams/stream.hpp>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/rational.hpp>
#include <boost/variant.hpp>

#include "exec.h"
#include "module.h"
#include "modutil.h"

#include "ilessaux.h"
using namespace ilessaux;
#include "foutaux.h"
using namespace foutaux;
#include "debugaux.h"

#ifndef NDEBUG
#define NONCOPYABLE , boost::noncopyable
#define _NONCOPYABLE :boost::noncopyable
#else
#define NONCOPYABLE
#define _NONCOPYABLE
#endif

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

namespace xmlout {

  const char* ierr = 0;

#define XML_VERSION_STRING "2.0"

  struct errbase {};

  // enum fermata_types {fermata_norm, fermata_short, fermata_long,
  // fermata_long2}; typedef std::map<const char*, fermata_types, charisiless>
  // fermatasmaptype; fermatasmaptype fermatasmap;

  // enum arpeggio_types {arpeggio_none, arpeggio_norm, arpeggio_up,
  // arpeggio_down, arpeggio_bracket, arpeggio_paren}; typedef std::map<const
  // char*, arpeggio_types, charisiless> arpeggiosmaptype; arpeggiosmaptype
  // arpeggiosmap;

  // enum slur_types {slur_none, slur_norm, slur_dotted, slur_dashed};
  // typedef std::map<const char*, slur_types, charisiless> slursmaptype;
  // slursmaptype slursmap;

  enum ps_types { ps_text, ps_bracket };
  typedef std::map<const char*, bool, charisiless> pedmaptype;
  pedmaptype pedstyles;

  // XML STUFF
  class enclose;
  struct encloseinfo {
    boost::filesystem::ofstream f;
    int ind; // indent size
    const int inc;
    bool atnewl; //, fintag;
    // std::ostringstream deltxt; bool del;
    enclose* encl;
    encloseinfo(const int inc)
        : ind(0), inc(inc), atnewl(false),
          encl(0) /*, eol(true)*/ /*, ok(true)*/ {
#ifndef NDEBUG
      debug = 12345;
#endif
    }
    void incind() {
      if (inc > 0)
        ind += inc;
      else
        ++ind;
    }
    void decind() {
      if (inc > 0)
        ind -= inc;
      else
        --ind;
    }
#ifndef NDEBUG
    int debug;
    bool isvalid() const {
      return debug == 12345;
    }
#endif
  };

  class enclose {
    encloseinfo& info;
    const char* tag;
    enclose* save;
    std::ostringstream deltxt;
    bool del;
    bool fintag;
#ifndef NDEBUG
    int debug;

public:
    bool isvalid() const {
      return debug == 12345;
    }
#endif
public:
    bool notdel() const {
      return !del;
    }
    enclose(encloseinfo& info, const char* tag)
        : info(info), tag(tag), save(info.encl), del(false), fintag(true) {
#ifndef NDEBUG
      debug = 12345;
#endif
      assert(info.isvalid());
      if (info.encl)
        info.encl->flushenc();
      info.f << '\n' << std::string(info.ind, (info.inc > 0 ? ' ' : '\t'));
      info.f << '<' << tag;
      info.incind();
      info.encl = this;
    }
    enclose(encloseinfo& info, const char* tag, const int maybedel)
        : info(info), tag(tag), save(info.encl), del(true), fintag(true) {
#ifndef NDEBUG
      debug = 12345;
#endif
      deltxt << '\n'
             << std::string(info.ind,
                            (info.inc > 0 ? ' ' : '\t')) // put rest in deltxt
             << '<' << tag;
      info.incind();
      info.encl = this;
    }
    ~enclose() {
      assert(info.isvalid());
      info.encl = save;
      assert(!save || save->isvalid());
      info.decind();
      if (del)
        return;
      info.f.exceptions(
          boost::filesystem::ofstream::goodbit); // avoid exceptions in
                                                 // destructor
      if (fintag) {
        info.f << "/>";
        fintag = false;
      } else {
        if (!info.atnewl)
          info.f << '\n' << std::string(info.ind, (info.inc > 0 ? ' ' : '\t'));
        info.f << "</" << tag << ">";
      }
      info.atnewl = false;
      info.f.exceptions(boost::filesystem::ofstream::eofbit |
                        boost::filesystem::ofstream::failbit |
                        boost::filesystem::ofstream::badbit);
    }
    void flushenc() {
      assert(isvalid());
      assert(info.isvalid());
      if (del) {
        if (save) {
          assert(save->isvalid());
          save->flushenc();
        }
        assert(info.isvalid());
        info.f << deltxt.str();
        del = false;
      }
      if (fintag) {
        info.f << '>';
        fintag = false;
      }
    }
    std::ostream& getf() {
      assert(isvalid());
      assert(info.isvalid());
      return (del ? (std::ostream&) deltxt : (std::ostream&) info.f);
    }
  };

  inline std::ostream& operator<<(std::ostream& o, const fomus_rat& x) {
    o << module_rattostr(x);
    return o;
  }
  inline std::ostream& operator<<(std::ostream& ou, const module_value& x) {
    switch (x.type) {
    case module_int:
    case module_rat:
    case module_float: {
      ou << module_valuetostr(x);
      break;
    }
    default:
      assert(false);
    }
    return ou;
  }

  template <typename T>
  inline void encprval(std::ostream& ou, const T& x) {
    ou << x;
  }
  template <>
  inline void encprval(std::ostream& ou, const module_value& x) {
    xmlout::operator<<(ou, x);
  }

  template <typename T>
  inline encloseinfo& operator<<(encloseinfo& out,
                                 const T& o) { // CDATA text output
    if (out.encl) {
      out.encl->flushenc();
    }
    std::stringstream str;
    encprval(str, o); // str << o;
    while (!str.eof()) {
      char x = str.get();
      if (x < 0)
        break;
      switch (x) {
      case '&':
        out.f << "&amp;";
        break;
      case '<':
        out.f << "&lt;";
        break;
      case '>':
        out.f << "&gt;";
        break;
      case '\'':
        out.f << "&apos;";
        break;
      case '"':
        out.f << "&quot;";
        break;
      default:
        out.f << x;
      }
    }
    out.atnewl = true; // so end tag prints right after it
    return out;
  }

  template <typename T>
  class attr {
    const char* id;
    const T& x;

public:
    attr(const char* id, const T& x) : id(id), x(x) {}
    const char* getid() const {
      return id;
    }
    const T& getval() const {
      return x;
    }
  };
  template <>
  inline encloseinfo&
  operator<<<attr<std::string>>(encloseinfo& out, const attr<std::string>& o) {
    assert(out.isvalid());
    (out.encl ? out.encl->getf() : out.f)
        << ' ' << o.getid() << "=\"" << o.getval() << '"';
    return out;
  }
  template <>
  inline encloseinfo& operator<<<attr<int>>(encloseinfo& out,
                                            const attr<int>& o) {
    assert(out.isvalid());
    (out.encl ? out.encl->getf() : out.f)
        << ' ' << o.getid() << "=\"" << o.getval() << '"';
    return out;
  }
  template <>
  inline encloseinfo&
  operator<<<attr<const char*>>(encloseinfo& out, const attr<const char*>& o) {
    assert(out.isvalid());
    (out.encl ? out.encl->getf() : out.f)
        << ' ' << o.getid() << "=\"" << o.getval() << '"';
    return out;
  }

  int indentid, partnameid, partabbrid, timesigstyleid, thetitleid, theauthorid,
      beatid, pedstyleid, praccid, altervalsid, verboseid, xmlviewextid,
      xmlviewexepathid, xmlviewexeargsid;

  struct part {
    module_partobj p;
    std::string id;
    module_barlines bls;
    part(const module_partobj p, const int id0) : p(p) {
      std::ostringstream x;
      x << "P" << id0;
      id = x.str();
    }
  };

  struct scoped_enc {
    encloseinfo& x;
    const char* tag;
    char enc[sizeof(enclose)];
    bool isin;
    scoped_enc(encloseinfo& x, const char* tag) : x(x), tag(tag), isin(false) {}
    void inscope() {
      if (!isin) {
        new (enc) enclose(x, tag, 0);
        isin = true;
        DBG("RESET1" << std::endl);
      }
    }
    void inscopenochk() {
      assert(!isin);
      new (enc) enclose(x, tag, 0);
      isin = true;
      DBG("RESET1" << std::endl);
    }
    void outscope() {
      if (isin) {
        ((enclose*) enc)->~enclose();
        isin = false;
      }
    }
    ~scoped_enc() {
      if (isin)
        ((enclose*) enc)->~enclose();
    }
    enclose* operator->() {
      return ((enclose*) enc);
    }
    const enclose* operator->() const {
      return ((enclose*) enc);
    }
    void resetin() {
      if (isin)
        ((enclose*) enc)->~enclose();
      new (enc) enclose(x, tag, 0);
      isin = true;
      DBG("RESET2" << std::endl);
    }
  };

  struct xmloutdata {
    FOMUS fom;
    bool cerr;
    std::stringstream CERR;
    std::string errstr;
    encloseinfo x;
    const char* module_err() {
      if (!cerr)
        return 0;
      std::getline(CERR, errstr);
      return errstr.c_str();
    }
    xmloutdata(FOMUS fom)
        : fom(fom), cerr(false), x(module_setting_ival(fom, indentid)) {}
    void modout_write(FOMUS fom, const char* filename);
    void dostaves(part& pa, int& grid, std::vector<int>& grids);
    void printpart(part& pa, module_noteobj& n, module_measobj& m);
    void printmeas(part& prt, const module_measobj me, module_noteobj& n,
                   std::vector<int>& mclfs, int& measn, fomus_rat& cts,
                   const bool first, modout_keysig& lmkey, int& octch,
                   bool& sysbreak);
    void printnote(const module_noteobj n, const fomus_rat& wrm,
                   const fomus_rat& divmult, const bool chd, int& lstaff,
                   int& lstaffv, std::set<int>& gliss, std::set<int>& port,
                   scoped_enc& direnc, int& octch, const bool fmr,
                   enum module_markpos& dirplace, const module_partobj pobj,
                   bool& sysbreak);
    const char* getdurtype(const fomus_rat& dur);
    void accidental(encloseinfo& x, const fomus_rat& acc);
    void pitch(encloseinfo& x, int p, const fomus_rat& a1,
               const fomus_rat& a2 = module_makerat(0, 1),
               const module_value& map = module_makenullval(),
               const bool isprc = false);
  };

  inline const char* stripnl(char* x) {
    for (char* i = x; i != 0; ++i) {
      if (*i == '\n') {
        *i = 0;
        break;
      }
    }
    return x;
  }

  const char step[] = {'C', 0, 'D', 0, 'E', 'F', 0, 'G', 0, 'A', 0, 'B'};

  const char* clefsgn[] = {"F", "F", "C", "F", "C", "F", "C",
                           "F", "C", "C", "F", "C", "C", "F",
                           "G", "C", "F", "C", "C", "F", "C",
                           "C", "G", "C", "C", "C", "G", "percussion"};
  const int clefln[] = {5,  -1, 5,  3, 4, 5, -1, -1, 2,  5,  3, 1, 4,  5,
                        -1, -1, -1, 2, 5, 3, 1,  4,  -1, -1, 2, 1, -1, -1};
  const int clefoc[] = {-1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 1,
                        -1, 0,  1,  0,  1,  1, 0,  1, 0,  1, 1, 1,  1, 0};

  std::map<fomus_rat, const char*> durtyps;
  std::map<fomus_rat, const char*> acctyps;

  const char* xmloutdata::getdurtype(const fomus_rat& dur) {
    std::map<fomus_rat, const char*>::const_iterator j(durtyps.find(dur));
    if (j == durtyps.end()) {
      CERR << "can't write duration" << std::endl;
      throw errbase();
    }
    return j->second;
  }

  bool order(const module_noteobj x, const module_noteobj y) {
    {
      int vx = module_voice(x);
      int yx = module_voice(y);
      if (vx != yx)
        return vx < yx;
    }
    {
      fomus_rat xt(module_time(x));
      fomus_rat yt(module_time(y));
      if (xt != yt)
        return xt < yt;
    }
    {
      module_value xg0(module_vgracetime(x));
      module_value yg0(module_vgracetime(y));
      if (xg0.type != module_none && yg0.type != module_none) {
        fomus_rat xg(GET_R(xg0));
        fomus_rat yg(GET_R(yg0));
        if (xg != yg)
          return xg < yg;
      } else if (xg0.type != module_none)
        return true; // x is grace, y is normal
      else if (yg0.type != module_none)
        return false;
    }
    if (x == y)
      return false;
    return module_pitch(x) < module_pitch(y);
  }

  inline void markpos(encloseinfo& x, const enum module_markpos p) {
    switch (p) {
    case markpos_above:
      x << attr<const char*>("placement", "above");
      break;
    case markpos_below:
      x << attr<const char*>("placement", "below");
      break;
    default:;
    }
  }
  inline void markpos(encloseinfo& x, const module_markobj m) {
    markpos(x, module_markpos(m));
  }

  void resetdirenc(encloseinfo& x, scoped_enc& direnc, const int lstaff,
                   const int lstaffv, const enum module_markpos dirplace) {
    if (direnc->notdel()) {
      if (lstaffv > 0) {
        enclose xxx(x, "voice");
        x << lstaffv; // stvo;
      }
      {
        enclose xxx(x, "staff");
        x << lstaff; // sta;
      }
    }
    direnc.resetin();
    markpos(x, dirplace);
  }

  void xmloutdata::accidental(encloseinfo& x, const fomus_rat& acc) {
    std::map<fomus_rat, const char*>::const_iterator i(acctyps.find(acc));
    if (i == acctyps.end()) {
      CERR << "can't write accidental" << std::endl;
      throw errbase();
    }
    x << i->second;
  }

  void xmloutdata::pitch(encloseinfo& x, int p, const fomus_rat& a1,
                         const fomus_rat& a2, const module_value& map,
                         const bool isprc) {
    if (isprc)
      p = tochromatic(todiatonic(p) + 6);
    enclose xxx(x, (isprc ? "unpitched" : "pitch"));
    {
      enclose xxx(x, (isprc ? "display-step" : "step"));
      assert(step[p % 12] != 0);
      x << step[p % 12];
    }
    assert(map.type == module_none || map.type == module_list);
    if (map.type == module_list && map.val.l.n > 0) {
      for (const module_value *i = map.val.l.vals + 1,
                              *ie = map.val.l.vals + map.val.l.n;
           i < ie; i += 2) {
        assert(i->type == module_list);
        assert(i->val.l.n >= 1 && i->val.l.n <= 2);
        if (a1 == *i->val.l.vals &&
            a2 == (i->val.l.n <= 1 ? module_makerat(0, 1)
                                   : GET_R(*(i->val.l.vals + 1)))) {
          enclose xxx(x, "alter");
          assert((i - 1)->type == module_string);
          x << (i - 1)->val.s;
          goto ALLDONEACC;
        }
      }
      CERR << "can't write accidental";
      throw errbase();
    } else {
      fomus_rat a(a1 + a2);
      if (a != (fomus_int) 0) {
        enclose xxx(x, "alter");
        if (a.den == 1)
          x << a.num;
        else
          x << module_rattofloat(a);
      }
    }
  ALLDONEACC : {
    enclose xxx(x, (isprc ? "display-octave" : "octave"));
    int o = (p / 12) - 1;
    if (o < 0 || o > 9) {
      CERR << "note out of range" << std::endl;
      throw errbase();
    }
    x << o;
  }
  }

  void xmloutdata::printnote(const module_noteobj n, const fomus_rat& wrm,
                             const fomus_rat& divmult, const bool chd,
                             int& lstaff, int& lstaffv, std::set<int>& gliss,
                             std::set<int>& slide, scoped_enc& direnc,
                             int& octch, const bool fmr,
                             enum module_markpos& dirplace,
                             const module_partobj pobj, bool& sysbreak) {
    fomus_rat tim(module_time(n));
    module_markslist mks(module_marks(n));
    int voi = module_voice(n);
    int vnum = voi % 6;              // a unique id, used for number-level
    int stvo = module_staffvoice(n); // voice as far as musicxml is concerned
    int sta = module_staff(n);
    bool grslash = false;
    bool mrst = module_ismarkrest(n);
    {
      if (direnc.isin) {
        if (stvo != lstaffv || sta != lstaff)
          resetdirenc(x, direnc, lstaff, lstaffv, dirplace);
      } else {
        direnc.inscopenochk();
        markpos(x, dirplace);
        // switch (dirplace) {
        // case markpos_above: x << attr<const char*>("placement", "above");
        // DBG("ABOVE1" << std::endl); break; case markpos_below: x <<
        // attr<const char*>("placement", "below"); DBG("BELOW1" << std::endl);
        // break; default: DBG("NOTEHEAD1" << std::endl);
        // }
      } // direnc is now "current"
      assert(direnc.isin);
      lstaffv = stvo;
      lstaff = sta;
      int oc = module_octsign(n);
      if (oc != octch) {
        assert(octch == 0 && oc != 0);
        enclose xxx(x, "direction-type");
        enclose yyy(x, "octave-shift");
        switch (oc) {
        case -2:
          x << attr<const char*>("type", "up") << attr<int>("number", oc + 3)
            << attr<int>("size", 15);
          break;
        case -1:
          x << attr<const char*>("type", "up") << attr<int>("number", oc + 3)
            << attr<int>("size", 8);
          break;
        case 1:
          x << attr<const char*>("type", "down") << attr<int>("number", oc + 3)
            << attr<int>("size", 8);
          break;
        case 2:
          x << attr<const char*>("type", "down") << attr<int>("number", oc + 3)
            << attr<int>("size", 15);
          break;
        default:
          assert(false);
        }
        octch = oc;
      }
      for (int o = 0; o < 9;
           ++o) { // o % 2 is order, o / 2 is placement (either, above, below)
        bool pedbegin = false, pedend = false;
        for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n); m < me;
             ++m) {
          int pl = o / 3;
          if ((!mrst && (o % 3) >= 2) || modout_markorder(*m) != (o % 3) ||
              module_markpos(*m) != pl)
            continue;
          if (pl != dirplace)
            resetdirenc(x, direnc, lstaff, lstaffv,
                        dirplace = (enum module_markpos) pl);
          switch (module_markbaseid(module_markid(*m))) {
          case mark_cresc_begin: {
            enclose xxx(x, "direction-type");
            enclose yyy(x, "wedge");
            x << attr<const char*>("type", "crescendo")
              << attr<int>("number", vnum);
          } break;
          case mark_dim_begin: {
            enclose xxx(x, "direction-type");
            enclose yyy(x, "wedge");
            x << attr<const char*>("type", "diminuendo")
              << attr<int>("number", vnum);
          } break;
          case mark_cresc_end: // go to next one...
          case mark_dim_end: {
            enclose xxx(x, "direction-type");
            enclose yyy(x, "wedge");
            x << attr<const char*>("type", "stop") << attr<int>("number", vnum);
          } break;
          case mark_text: {
            const char* ms = module_markstring(*m);
            if (!ms)
              ms = "";
            enclose xxx(x, "direction-type");
            enclose yyy(x, "words");
            x << ms;
          } break;
          case mark_ped_begin:
            pedbegin = true;
            break;
          case mark_ped_end:
            pedend = true;
            break;
          case mark_tempo: {
            struct modout_tempostr te(modout_tempostr(n, *m));
            if (te.str1) {
              enclose xxx(x, "direction-type");
              enclose yyy(x, "words");
              x << attr<const char*>(
                  "font-size",
                  "large"); // xx-small, x-small, small,
                            // medium, large, x-large, xx-large
              x << te.str1;
            }
            if (te.beat != (fomus_int) 0) {
              enclose xxx(x, "direction-type");
              enclose yyy(x, "metronome");
              enclose zzz(x, "metronome-note");
              struct modutil_rhythm rh(rhythm(te.beat));
              {
                enclose zzz(x, "metronome-type");
                x << getdurtype(rh.dur);
              }
              for (int i = 0; i < rh.dots; ++i) {
                enclose zzz(x, "metronome-dot");
              }
            }
            if (te.str2) {
              enclose xxx(x, "direction-type");
              enclose yyy(x, "words");
              x << attr<const char*>(
                  "font-size",
                  "large"); // xx-small, x-small, small,
                            // medium, large, x-large, xx-large
              x << te.str2;
            }
          } break;
          case mark_italictextabove_begin:
          case mark_italictextbelow_begin: {
            {
              const char* ms = module_markstring(*m);
              if (!ms)
                ms = "";
              enclose xxx(x, "direction-type");
              enclose yyy(x, "words");
              x << attr<const char*>("font-style", "italic");
              x << ms;
            }
            {
              enclose xxx(x, "direction-type");
              enclose yyy(x, "dashes");
              x << attr<const char*>("type", "start")
                << attr<int>("number", vnum);
            }
          } break;
          case mark_stafftext_begin: {
            {
              const char* ms = module_markstring(*m);
              if (!ms)
                ms = "";
              enclose xxx(x, "direction-type");
              enclose yyy(x, "words");
              x << attr<const char*>("font-size", "large");
              x << ms;
            }
            {
              enclose xxx(x, "direction-type");
              enclose yyy(x, "dashes");
              x << attr<const char*>("type", "start")
                << attr<int>("number", vnum);
            }
          } break;
          case mark_italictextabove_end:
          case mark_italictextbelow_end:
          case mark_stafftext_end: {
            enclose xxx(x, "direction-type");
            enclose yyy(x, "dashes");
            x << attr<const char*>("type", "stop") << attr<int>("number", vnum);
          } break;
          case mark_stafftext: {
            const char* ms = module_markstring(*m);
            if (!ms)
              ms = "";
            enclose xxx(x, "direction-type");
            enclose yyy(x, "words");
            x << attr<const char*>("font-size", "large");
            x << ms;
          } break;
          case mark_italictextabove:
          case mark_italictextbelow: {
            const char* ms = module_markstring(*m);
            if (!ms)
              ms = "";
            enclose xxx(x, "direction-type");
            enclose yyy(x, "words");
            x << attr<const char*>("font-style", "italic");
            x << ms;
          } break;
          // case mark_sul: {
          //   const char* ms = module_markstring(*m);
          //   if (!ms) ms = "";
          //   enclose xxx(x, "direction-type");
          //   enclose yyy(x, "words"); x << ms;
          // } break;
          case mark_pizz:
          case mark_arco:
          case mark_mute:
          case mark_unmute:
          case mark_vib:
          case mark_moltovib:
          case mark_nonvib:
          case mark_legato:
          case mark_moltolegato:
          case mark_nonlegato:
          case mark_sul:
          case mark_salt:
          case mark_ric:
          case mark_lv:
          case mark_flt:
          case mark_slap:
          case mark_breath:
          case mark_spic:
          case mark_tall:
          case mark_punta:
          case mark_pont:
          case mark_tasto:
          case mark_legno:
          case mark_flaut:
          case mark_etouf:
          case mark_table:
          case mark_cuivre:
          case mark_bellsup:
          case mark_ord: {
            assert(module_markstring(*m));
            enclose xxx(x, "direction-type");
            enclose yyy(x, "words");
            x << module_markstring(*m);
          } break;
          case mark_graceslash:
            grslash = true;
          default:;
          }
        }
        if (pedbegin && pedend &&
            pedstyles.find(module_setting_sval(n, pedstyleid))->second) {
          enclose xxx(x, "direction-type");
          enclose yyy(x, "pedal");
          x << attr<const char*>("type", "change")
            << attr<const char*>("line", "yes");
        } else {
          if (pedend) {
            enclose xxx(x, "direction-type");
            enclose yyy(x, "pedal");
            x << attr<const char*>("type", "stop")
              << attr<const char*>(
                     "line",
                     pedstyles.find(module_setting_sval(n, pedstyleid))->second
                         ? "yes"
                         : "no");
          }
          if (pedbegin) {
            enclose xxx(x, "direction-type");
            enclose yyy(x, "pedal");
            x << attr<const char*>("type", "start")
              << attr<const char*>(
                     "line",
                     pedstyles.find(module_setting_sval(n, pedstyleid))->second
                         ? "yes"
                         : "no");
          }
        }
      }
      if (direnc->notdel()) {
        if (stvo > 0) {
          enclose xxx(x, "voice");
          x << stvo;
        }
        {
          enclose xxx(x, "staff");
          x << sta;
        }
      }
      direnc.outscope();
    }
    if (modout_isinvisible(n)) {
      fomus_rat d(module_dur(n));
      if (d > (fomus_int) 0) {
        enclose xxx(x, "forward");
        enclose yyy(x, "duration");
        assert((d * divmult).den == 1);
        x << (module_dur(n) * divmult).num;
      }
    } else {
      bool pracc = !module_setting_ival(n, praccid);
      module_value ltr1, ltr2, ltrn;
      ltr2 = module_makeval(0, 1);
      ltr1.type = ltrn.type = module_special;
      enum module_markpos ltrw;
      {
        bool isgr = module_isgrace(n);
        bool nrst = module_isnote(n);
        enclose xxx(x, "note");
        if (isgr) {
          enclose xxx(x, "grace");
          x << attr<const char*>("slash", grslash ? "yes" : "no");
        }
        if (chd) {
          enclose xxx(x, "chord");
        }
        if (nrst) {
          pitch(x, module_writtennote(n), module_acc1(n), module_acc2(n),
                module_setting_val(n, altervalsid), module_isperc(n));
        } else {
          enclose xxx(x, "rest");
        }
        if (!isgr) {
          enclose xxx(x, "duration"); // only if not grace note
          assert((module_dur(n) * divmult).den == 1);
          x << (module_dur(n) * divmult).num;
        }
        bool istiel = module_istiedleft(n), istier = module_istiedright(n);
        if (istiel) {
          enclose xxx(x, "tie");
          x << attr<const char*>("type", "stop");
        }
        if (istier) {
          enclose xxx(x, "tie");
          x << attr<const char*>("type", "start");
        }
        if (stvo > 0) {
          enclose xxx(x, "voice");
          x << stvo;
        }
        if (fmr) {
          enclose xxx(x, "type");
          x << "whole";
        } else {
          struct modutil_rhythm rh(rhythm(
              (isgr ? module_adjgracedur(n, -1) : module_adjdur(n, -1)) * wrm));
          {
            enclose xxx(x, "type");
            for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
                 m < me; ++m) {
              if (module_markbaseid(module_markid(*m)) ==
                  mark_artharm_sounding) {
                x << attr<const char*>("size", "cue");
                break;
              }
            }
            x << getdurtype(rh.dur);
          }
          for (int i = 0; i < rh.dots; ++i) {
            enclose xxx(x, "dot");
          }
        }
        if (pracc && nrst) {
          fomus_rat wa(module_fullwrittenacc(n));
          if (wa != std::numeric_limits<fomus_int>::max()) {
            enclose xxx(x, "accidental");
            accidental(x, wa);
          }
        }
        int lvl = 0;
        fomus_rat tup;
        while (true) {
          fomus_rat tup0(module_tuplet(n, lvl));
          if (tup0 == (fomus_int) 0)
            break;
          tup = tup0;
          ++lvl;
        }
        if (lvl > 0) { // lvl is number of tuplets
          enclose xxx(x, "time-modification");
          {
            enclose xxx(x, "actual-notes");
            x << tup.num;
          }
          {
            enclose xxx(x, "normal-notes");
            x << tup.den;
          }
          struct modutil_rhythm trh(
              rhythm(module_beatstoadjdur(
                         n, module_fulltupdur(n, lvl - 1) / tup.num, lvl) *
                     wrm)); // full duration of highest-level tuplet
          assert(trh.dur.den != 0);
          {
            enclose xxx(x, "normal-type");
            x << getdurtype(trh.dur);
          }
          for (int i = 0; i < trh.dots; ++i) {
            enclose xxx(x, "normal-dot");
          }
        }
        { // notehead
          for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
               m < me; ++m) {
            switch (module_markbaseid(module_markid(*m))) {
            case mark_natharm_touched: // goto next
            case mark_artharm_touched: {
              enclose xxx(x, "notehead");
              x << "diamond";
            } break;
              // case mark_natharm_sounding: // goto next
            case mark_artharm_sounding: {
              enclose xxx(x, "notehead");
              x << attr<const char*>("parentheses", "yes");
              x << "normal";
            } break;
            }
          }
        }
        {
          enclose xxx(x, "staff");
          x << sta;
        }
        int beaml(modout_beamsleft(n)), beamr(modout_beamsright(n));
        int tbeams(std::max(beaml, beamr)); // total
        {
          int beamlc(modout_connbeamsleft(n)),
              beamrc(modout_connbeamsright(n)); // connected beams
          int cbeams(std::min(beamlc, beamrc)); // connected l/r
          for (int i = 0; i < tbeams; ++i) {    // connected l/r
            enclose xxx(x, "beam");
            x << attr<int>("number", i + 1);
            if (i < cbeams)
              x << "continue";
            else if (i < beamlc)
              x << "end";
            else if (i < beamrc)
              x << "begin";
            else if (i < beaml)
              x << "backward hook";
            else
              x << "forward hook";
          }
        }
        {
          enclose xxx(x, "notations", 0); // don't print if nothing inside
          if (istiel) {
            enclose xxx(x, "tied");
            x << attr<const char*>("type",
                                   "stop"); // << attr<int>("number", vnum);
          }
          if (istier) {
            enclose xxx(x, "tied");
            x << attr<const char*>("type",
                                   "start"); //  << attr<int>("number", vnum);
          }
          {
            for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
                 m < me; ++m) {
              switch (module_markbaseid(
                  module_markid(*m))) { // musicxml only allows ids from 1 to
                                        // 6--complex formula tries to avoid
                                        // sharing same id, but no guarantees
              case mark_phrase_begin: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "start")
                  << attr<int>("number", (voi * 3 + voi / 6) % 6 + 1);
              } break;
              case mark_dottedphrase_begin: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "start")
                  << attr<int>("number", (voi * 3 + voi / 6) % 6 + 1)
                  << attr<const char*>("line-type", "dotted");
              } break;
              case mark_dashedphrase_begin: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "start")
                  << attr<int>("number", (voi * 3 + voi / 6) % 6 + 1)
                  << attr<const char*>("line-type", "dashed");
              } break;
              case mark_slur_begin: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "start")
                  << attr<int>("number", (voi * 3 + voi / 6 + 1) % 6 + 1);
              } break;
              case mark_dottedslur_begin: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "start")
                  << attr<int>("number", (voi * 3 + voi / 6 + 1) % 6 + 1)
                  << attr<const char*>("line-type", "dotted");
              } break;
              case mark_dashedslur_begin: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "start")
                  << attr<int>("number", (voi * 3 + voi / 6 + 1) % 6 + 1)
                  << attr<const char*>("line-type", "dashed");
              } break;
              case mark_graceslur_begin: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "start")
                  << attr<int>("number", (voi * 3 + voi / 6 + 2) % 6 + 1);
              } break;
              case mark_graceslur_end: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "stop")
                  << attr<int>("number", (voi * 3 + voi / 6 + 2) % 6 + 1);
              } break;
              case mark_slur_end: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "stop")
                  << attr<int>("number", (voi * 3 + voi / 6 + 1) % 6 + 1);
              } break;
              case mark_dottedslur_end: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "stop")
                  << attr<int>("number", (voi * 3 + voi / 6 + 1) % 6 + 1)
                  << attr<const char*>("line-type", "dotted");
              } break;
              case mark_dashedslur_end: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "stop")
                  << attr<int>("number", (voi * 3 + voi / 6 + 1) % 6 + 1)
                  << attr<const char*>("line-type", "dashed");
              } break;
              case mark_phrase_end: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "stop")
                  << attr<int>("number", (voi * 3 + voi / 6) % 6 + 1);
              } break;
              case mark_dottedphrase_end: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "stop")
                  << attr<int>("number", (voi * 3 + voi / 6) % 6 + 1)
                  << attr<const char*>("line-type", "dotted");
              } break;
              case mark_dashedphrase_end: {
                enclose xxx(x, "slur");
                x << attr<const char*>("type", "stop")
                  << attr<int>("number", (voi * 3 + voi / 6) % 6 + 1)
                  << attr<const char*>("line-type", "dashed");
              } break;
              default:;
              }
            }
          }
          for (int tl = 0;; ++tl) {
            fomus_rat tu(module_tuplet(n, tl));
            if (tu == (fomus_int) 0)
              break;
            for (int i = 0; i < 2; ++i) {
              if (i == 0 ? module_tupletbegin(n, tl)
                         : module_tupletend(n, tl)) {
                enclose xxx(x, "tuplet");
                x << attr<const char*>("type", (i == 0 ? "start" : "stop"));
                x << attr<int>("number",
                               tl + 1); // attr: bracket %yes-no; #IMPLIED
#warning                                                                       \
    "explicit instructions for printing the tuplet--get user settings for this"
                struct modutil_rhythm trh(
                    rhythm(module_beatstoadjdur(
                               n, module_fulltupdur(n, tl) / tu.num, tl + 1) *
                           wrm));
                {
                  enclose xxx(x, "tuplet-actual");
                  {
                    enclose xxx(x, "tuplet-number");
                    x << tu.num;
                  }
                  {
                    enclose xxx(x, "tuplet-type");
                    x << getdurtype(trh.dur);
                  }
                  for (int i = 0; i < trh.dots; ++i) {
                    enclose xxx(x, "tuplet-dot");
                  }
                }
                {
                  enclose xxx(x, "tuplet-normal");
                  {
                    enclose xxx(x, "tuplet-number");
                    x << tu.den;
                  }
                  {
                    enclose xxx(x, "tuplet-type");
                    x << getdurtype(trh.dur);
                  }
                  for (int i = 0; i < trh.dots; ++i) {
                    enclose xxx(x, "tuplet-dot");
                  }
                }
              }
            }
          }
          std::set<int>::const_iterator git(gliss.find(voi));
          if (git != gliss.end()) {
            enclose xxx(x, "glissando");
            x << attr<const char*>("type", "stop") << attr<int>("number", vnum);
            gliss.erase(git);
          }
          {
            for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
                 m < me; ++m) {
              switch (module_markbaseid(module_markid(*m))) {
              case mark_gliss_before:
              case mark_gliss_after: {
                enclose xxx(x, "glissando");
                x << attr<const char*>("type", "start")
                  << attr<int>("number", vnum);
                gliss.insert(voi);
              }
              }
            }
          }
          std::set<int>::const_iterator sit(slide.find(voi));
          if (sit != slide.end()) {
            enclose xxx(x, "slide");
            x << attr<const char*>("type", "stop") << attr<int>("number", vnum);
            slide.erase(sit);
          }
          {
            enclose xxx(x, "ornaments", 0);
            // bool istrillwnote = false;
            for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
                 m < me; ++m) {
              switch (module_markbaseid(module_markid(*m))) {
              case mark_trem: {
                enclose yyy(x, "tremolo");
                module_value n(module_marknum(*m));
                assert(n.type == module_int);
                if (n.val.i >= 0) { // single trem
                  x << attr<const char*>("type", "single");
                } else { // first in double trem
                  x << attr<const char*>("type", "start");
                }
                int nn = 1;
                fomus_int i = abs_int(n.val.i) >> 3;
                while (i > 1) {
                  i >>= 1;
                  ++nn;
                }
                if (n.val.i >= 0) {
                  assert(nn - tbeams >= 0);
                  x << nn - tbeams;
                } else
                  x << nn;
                break;
              }
              case mark_trem2: {
                enclose yyy(x, "tremolo");
                x << attr<const char*>("type", "stop");
                module_value n(module_marknum(*m));
                assert(n.type == module_int);
                int nn = 1;
                fomus_int i = abs_int(n.val.i) >> 3;
                while (i > 1) {
                  i >>= 1;
                  ++nn;
                }
                x << nn;
                break;
              }
              case mark_longtrill: { // if num is a valid note (> 2), then it's
                                     // a note+parens and there's another
                                     // mark_longtrill and mark_longtrill2 pair
                                     // giving the accs
                module_value v(module_marknum(*m));
                if (v.type == module_none || v <= (fomus_int) 2)
                  ltr1 = v;
                else
                  ltrn = v;
                ltrw = module_markpos(*m);
                break;
              }
              case mark_longtrill2: { // trill note OR second accidental (if
                                      // ltr1 is <= 2)
                ltr2 = module_marknum(*m); // default = 0
                break;
              }
              }
            }
            if (ltr1.type != module_special) { // got a trill
              if (!istiel) {
                {
                  enclose xxx(x, "trill-mark");
                  markpos(x, ltrw);
                }
                if (pracc && ltrn.type == module_special &&
                    ltr1.type != module_none) { // ltr1 and ltr2 have the acc
                  enclose xxx(x, "accidental-mark");
                  markpos(x, ltrw);
                  assert(ltr1.type == module_int || ltr1.type == module_rat);
                  assert(ltr2.type == module_int || ltr2.type == module_rat);
                  accidental(x, GET_R(ltr1) + GET_R(ltr2));
                }
                {
                  enclose xxx(x, "wavy-line");
                  x << attr<const char*>("type", "start");
                  markpos(x, ltrw);
                }
              }
              if (istiel && istier) {
                enclose xxx(x, "wavy-line");
                x << attr<const char*>("type", "continue");
                markpos(x, ltrw);
              }
              if (!istier) {
                enclose xxx(x, "wavy-line");
                x << attr<const char*>("type", "stop");
                markpos(x, ltrw);
              }
            }
          }
          {
            for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
                 m < me; ++m) {
              switch (module_markbaseid(module_markid(*m))) {
              case mark_port_before:
              case mark_port_after: {
                enclose xxx(x, "portamento");
                x << attr<const char*>("type", "start")
                  << attr<int>("number", vnum);
                slide.insert(voi);
              }
              }
            }
          }
          {
            enclose xxx(x, "technical", 0);
            for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
                 m < me; ++m) {
              int id = module_markbaseid(module_markid(*m));
              switch (id) {
              case mark_open: {
                enclose xxx(x, "open-string");
              } break;
              case mark_stopped: {
                enclose xxx(x, "stopped");
                markpos(x, *m);
              } break;
              case mark_harm: {
                enclose xxx(x, "harmonic");
                markpos(x, *m);
              } break;
              default:;
              }
            }
          }
          {
            enclose xxx(x, "articulations", 0);
            for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
                 m < me; ++m) {
              switch (module_markbaseid(module_markid(*m))) {
              case mark_staccato: {
                enclose xxx(x, "staccato");
                markpos(x, *m);
              } break;
              case mark_staccatissimo: {
                enclose xxx(x, "staccatissimo");
                markpos(x, *m);
              } break;
              case mark_accent: {
                enclose xxx(x, "accent");
                markpos(x, *m);
              } break;
              case mark_tenuto: {
                enclose xxx(x, "tenuto");
                markpos(x, *m);
              } break;
              case mark_marcato: {
                enclose xxx(x, "strong-accent");
                markpos(x, *m);
              } break; // ?
              case mark_mezzostaccato: {
                enclose xxx(x, "detached-legato");
                markpos(x, *m);
              } break;
              case mark_breath_before:
              case mark_breath_after: {
                enclose xxx(x, "breath-mark");
                markpos(x, *m);
              } break;
              default:;
              }
            }
          }
          for (int p = (int) markpos_notehead; p < markpos_notehead + 3; ++p) {
            enclose xxx(x, "dynamics", 0);
            markpos(x, (enum module_markpos) p);
            for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
                 m < me; ++m) {
              if (module_markpos(*m) != p)
                continue;
              switch (module_markbaseid(module_markid(*m))) {
              case mark_pppppp: {
                enclose xxx(x, "pppppp");
              } break;
              case mark_ppppp: {
                enclose xxx(x, "ppppp");
              } break;
              case mark_pppp: {
                enclose xxx(x, "pppp");
              } break;
              case mark_ppp: {
                enclose xxx(x, "ppp");
              } break;
              case mark_pp: {
                enclose xxx(x, "pp");
              } break;
              case mark_p: {
                enclose xxx(x, "p");
              } break;
              case mark_mp: {
                enclose xxx(x, "mp");
              } break;
              case mark_ffff: {
                enclose xxx(x, "ffff");
              } break;
              case mark_fff: {
                enclose xxx(x, "fff");
              } break;
              case mark_ff: {
                enclose xxx(x, "ff");
              } break;
              case mark_f: {
                enclose xxx(x, "f");
              } break;
              case mark_mf: {
                enclose xxx(x, "mf");
              } break;
              case mark_sf: {
                enclose xxx(x, "sf");
              } break;
              case mark_sff: {
                enclose xxx(x, "sf");
              }
                { enclose xxx(x, "f"); }
                break; // musicxml dtd says I can do this
              case mark_sfff: {
                enclose xxx(x, "sf");
              }
                { enclose xxx(x, "ff"); }
                break;
              case mark_sfz: {
                enclose xxx(x, "sfz");
              } break;
              case mark_sffz: {
                enclose xxx(x, "sffz");
              } break;
              case mark_sfffz: {
                enclose xxx(x, "sf");
              }
                { enclose xxx(x, "f"); }
                { enclose xxx(x, "fz"); }
                break;
              case mark_fz: {
                enclose xxx(x, "fz");
              } break;
              case mark_ffz: {
                enclose xxx(x, "f");
              }
                { enclose xxx(x, "fz"); }
                break;
              case mark_fffz: {
                enclose xxx(x, "ff");
              }
                { enclose xxx(x, "fz"); }
                break;
              case mark_rfz: {
                enclose xxx(x, "rfz");
              } break;
              case mark_rf: {
                enclose xxx(x, "rf");
              } break;
              case mark_fp: {
                enclose xxx(x, "fp");
              } break;
              case mark_fzp: {
                enclose xxx(x, "fz");
              }
                { enclose xxx(x, "p"); }
                break;
              case mark_sfp: {
                enclose xxx(x, "sfp");
              } break;
              case mark_sfzp: {
                enclose xxx(x, "sfz");
              }
                { enclose xxx(x, "p"); }
                break;
              default:;
              }
            }
          }
          {
            for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
                 m < me; ++m) {
              int id = module_markbaseid(module_markid(*m));
              switch (id) {
              case mark_fermata: {
                enclose xxx(x, "fermata");
              } break;
              case mark_fermata_short: {
                enclose xxx(x, "fermata");
                x << "angled";
              } break;
              case mark_fermata_long: // goto next
              case mark_fermata_verylong: {
                enclose xxx(x, "fermata");
                x << "square";
              } break;
              default:;
              }
            }
          }
          {
            for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
                 m < me; ++m) {
              int id = module_markbaseid(module_markid(*m));
              switch (id) {
              case mark_arpeggio: {
                enclose xxx(x, "arpeggiate");
                markpos(x, *m);
                x << attr<int>("number", vnum);
              } break;
              case mark_arpeggio_up: {
                enclose xxx(x, "arpeggiate");
                markpos(x, *m);
                x << attr<int>("number", vnum)
                  << attr<const char*>("direction", "up");
              } break;
              case mark_arpeggio_down: {
                enclose xxx(x, "arpeggiate");
                markpos(x, *m);
                x << attr<int>("number", vnum)
                  << attr<const char*>("direction", "down");
              } break;
              default:;
              }
            }
          }
        }
        {
          enclose xxx(x, "lyric", 0);
          for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
               m < me; ++m) {
            if (m == mks.marks)
              markpos(x, *m);
            if (module_markbaseid(module_markid(*m)) == mark_vocal_text) {
              std::string s;
              const char* s0 = module_markstring(*m);
              if (s0)
                s = s0;
              int h = (boost::starts_with(s, "--") ? 0x2 : 0x0) |
                      (boost::ends_with(s, "--") ? 0x1 : 0x0);
              if (h & 0x2) {
                boost::erase_head(s, 2);
                boost::trim_left(s);
              }
              if (h & 0x1) {
                boost::erase_tail(s, 2);
                boost::trim_right(s);
              }
              switch (h) {
              case 0x1: {
                enclose yyy(x, "syllabic");
                x << "begin";
              } break;
              case 0x2: {
                enclose yyy(x, "syllabic");
                x << "end";
              } break;
              case 0x3: {
                enclose yyy(x, "syllabic");
                x << "middle";
              } break;
              }
              enclose yyy(x, "text");
              x << s;
              break;
            }
          }
        }
      }
      if (ltrn.type != module_special) { // trill w/ note, ltr1 and ltr2 have
                                         // the acc, ltrn has the note
        enclose xxx(x, "note");
        {
          enclose xxx(x, "grace");
          x << attr<const char*>("slash", "no");
        }
        assert(ltr1.type == module_int || ltr1.type == module_rat);
        assert(ltr2.type == module_int || ltr2.type == module_rat);
        fomus_rat a(GET_R(ltr1) + GET_R(ltr2));
        { pitch(x, GET_I(ltrn - a), a); }
        {
          enclose xxx(x, "type");
          x << "quarter";
        }
        if (pracc) {
          enclose xxx(x, "accidental");
          accidental(x, a);
        }
        {
          enclose xxx(x, "stem");
          x << "none";
        }
        {
          enclose xxx(x, "notehead");
          x << attr<const char*>("parentheses", "yes");
          x << "normal";
        }
        {
          enclose xxx(x, "staff");
          x << sta;
        }
      }
    }
    {
      direnc.inscope();
      markpos(x, dirplace);
      for (int pl = 0; pl < 3; ++pl) {
        for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n); m < me;
             ++m) {
          if (mrst || modout_markorder(*m) < 2 || module_markpos(*m) != pl)
            continue;
          if (pl != dirplace)
            resetdirenc(x, direnc, lstaff, lstaffv,
                        dirplace = (enum module_markpos) pl);
          switch (module_markbaseid(module_markid(*m))) {
          case mark_cresc_end: // go to next one...
          case mark_dim_end: {
            enclose xxx(x, "direction-type");
            enclose yyy(x, "wedge");
            x << attr<const char*>("type", "stop") << attr<int>("number", vnum);
          } break;
          case mark_italictextabove_end:
          case mark_italictextbelow_end:
          case mark_stafftext_end: {
            enclose xxx(x, "direction-type");
            enclose yyy(x, "dashes");
            x << attr<const char*>("type", "stop") << attr<int>("number", vnum);
          } break;
          default:;
          }
        }
      }
      for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n); m < me;
           ++m) {
        switch (module_markid(*m)) {
        case mark_break_before:
        case mark_break_after:
          sysbreak = true;
          break;
        default:;
        }
      }
      if (octch) {
        module_noteobj n0 = module_peeknextnote(n);
        int oc = (n0 && module_part(n0) == pobj ? module_octsign(n0) : 0);
        if (oc != octch) {
          enclose xxx(x, "direction-type");
          enclose yyy(x, "octave-shift");
          x << attr<const char*>("type", "stop")
            << attr<int>("number", octch + 3);
          octch = 0;
        }
      }
    }
  }

  void xmloutdata::printmeas(
      part& prt, const module_measobj me, module_noteobj& n,
      std::vector<int>& mclfs, int& measn, fomus_rat& cts, const bool first,
      modout_keysig& lmkey, int& octch,
      bool& sysbreak) { // mstvs = measure clefs, stvs = current clefs
    enclose xxx(x, "measure");
    x << attr<int>("number", ++measn);
    if (sysbreak) {
      enclose xxx(x, "print");
      x << attr<const char*>("new-system", "yes");
    }
    sysbreak = false;
    switch (prt.bls) {
    case barline_initfinal:
    case barline_initial: {
      enclose xxx(x, "barline");
      x << attr<const char*>("location", "left");
      {
        enclose yyy(x, "bar-style");
        x << "heavy-light";
      }
    } break;
    case barline_repeatleftright:
    case barline_repeatleft: {
      enclose xxx(x, "barline");
      {
        enclose yyy(x, "bar-style");
        x << "light-heavy";
      }
      {
        enclose yyy();
        x << attr<const char*>("direction", "backward");
      }
    } break;
    default:;
    }
    fomus_rat wrm(module_writtenmult(me));
    fomus_rat wrm4(wrm * (fomus_int) 4);
    std::vector<module_noteobj> nos;
    fomus_int divs = 1;
    module_noteobj n0 = n;
    while (n && module_meas(n) == me) {
      nos.push_back(n);
      fomus_rat dq(module_dur(n) * wrm4); // duration in # of quarter notes
      if (dq > (fomus_int) 0)
        divs = boost::integer::lcm(divs, dq.den);
      module_skipassign(n);
      n = module_nextnote();
    }
    wrm4 = wrm4 * divs; // wrm4 is multiplication factor now to get divisions
    std::sort(nos.begin(), nos.end(), order);
    {
      enclose xxx(x, "attributes");
      {
        enclose xxx(x, "divisions"); // per quarter note
        x << divs;
      }
      struct modout_keysig ks(modout_keysigdef(me));
      if (!modout_keysigequal(ks, lmkey)) {
        lmkey = ks;
        enclose xxx(x, "key");
        if (ks.mode == keysig_common_maj || ks.mode == keysig_common_min) {
          static int nua[][3] = {{-7, 0, 7},  // c
                                 {-5, 2, 0},  // d
                                 {-3, 4, 0},  // e
                                 {0, -1, 6},  // f
                                 {-6, 1, 0},  // g
                                 {-4, 3, 0},  // a
                                 {-2, 5, 0}}; // b
          static int nui[][3] = {{0, -3, 4},  // c
                                 {0, -1, 6},  // d
                                 {-6, 1, 0},  // e
                                 {0, -4, 3},  // f
                                 {0, -2, 5},  // g
                                 {-7, 0, 7},  // a
                                 {-5, 2, 0}}; // b
          enclose xxx(x, "fifths");
          assert(ks.dianote >= 0 && ks.dianote < 7);
          assert(ks.acc >= -1 && ks.acc <= 1);
          x << (ks.mode == keysig_common_maj ? nua[ks.dianote][ks.acc + 1]
                                             : nui[ks.dianote][ks.acc + 1]);
        } else if (ks.mode == keysig_indiv) {
          for (const modout_keysig_indiv *i(ks.indiv), *ie(ks.indiv + ks.n);
               i < ie; ++i) {
            {
              enclose xxx(x, "step");
              x << step[i->dianote];
            }
            {
              enclose xxx(x, "alter");
              fomus_rat a(i->acc1 + i->acc2);
              if (a.den == 1)
                x << a.num;
              else
                x << module_rattofloat(a);
            }
          }
        } else {
          CERR << "can't write key signature" << std::endl;
          throw errbase();
        }
      }
      fomus_rat ts(module_timesig(me));
      if (ts.num != cts.num || ts.den != cts.den) {
        cts = ts;
        enclose xxx(x, "time");
        if (module_setting_ival(me, timesigstyleid)) {
          if (ts.num == 4 && ts.den == 4)
            x << attr<const char*>("symbol", "common");
          else if (ts.num == 2 && ts.den == 2)
            x << attr<const char*>("symobl", "cut");
        }
        {
          enclose xxx(x, "beats");
          x << ts.num;
        }
        {
          enclose xxx(x, "beat-type");
          x << ts.den;
        }
      }
      if (first && mclfs.size() > 1) {
        enclose xxx(x, "staves");
        x << mclfs.size();
      }
      for (int i = 0; i < (int) mclfs.size(); ++i) {
        int cl = mclfs[i];
        while (n0 && module_part(n0) == prt.p) {
          if (module_staff(n0) == i + 1) {
            cl = module_clef(n0);
            break;
          }
          n0 = module_peeknextnote(n0);
        }
        if (cl < 0)
          cl = module_strtoclef(
              module_staffclef(prt.p, i + 1)); // get the default
        if (mclfs[i] != cl) {
          mclfs[i] = cl;
          enclose xxx(x, "clef");
          x << attr<int>("number", i + 1); // staff number
          {
            enclose xxx(x, "sign");
            x << clefsgn[cl];
          }
          if (clefln[cl] >= 0) {
            enclose xxx(x, "line"); // optional
            x << clefln[cl];
          }
          if (clefoc[cl] != 0) {
            enclose xxx(x, "clef-octave-change"); // optional
            x << clefoc[cl];
          }
        }
      }
    }
    fomus_int atdiv = 0;
    fomus_rat moff(module_time(me));
    bool chd = false;
    {
      scoped_enc direnc(x, "direction"); // (new enclose(x, "direction", 0));
      enum module_markpos dirplace = markpos_notehead;
      int lstaff = -1, lstaffv = -1;
      std::set<int> gliss, slide;
      bool fmr = module_isfullmeasrest(me);
      for (std::vector<module_noteobj>::iterator i(nos.begin()); i != nos.end();
           ++i) {
        // implement different voices & chords
        assert(((module_time(*i) - moff) * wrm4).den == 1);
        fomus_int d = ((module_time(*i) - moff) * wrm4).num;
        assert(d <= atdiv);
        if (!chd && d < atdiv) {
          if (direnc.isin && direnc->notdel()) {
            if (lstaffv > 0) {
              enclose xxx(x, "voice");
              x << lstaffv;
            }
            {
              enclose xxx(x, "staff");
              assert(lstaff >= 0);
              x << lstaff;
            }
          }
          direnc.outscope();
          {
            enclose xxx(x, "backup");
            enclose yyy(x, "duration");
            x << atdiv - d;
          }
        }
        assert(((module_endtime(*i) - moff) * wrm4).den == 1);
        atdiv = ((module_endtime(*i) - moff) * wrm4).num;
        printnote(*i, wrm, wrm4, chd, lstaff, lstaffv, gliss, slide, direnc,
                  octch, fmr, dirplace, prt.p, sysbreak);
        if (module_ischordlow(*i))
          chd = true;
        if (module_ischordhigh(*i))
          chd = false;
      }
    }
    switch (prt.bls = modout_rightbarline(me)) {
    case barline_dotted: {
      enclose xxx(x, "barline");
      {
        enclose yyy(x, "bar-style");
        x << "dotted";
      }
    } break;
    case barline_dashed: {
      enclose xxx(x, "barline");
      {
        enclose yyy(x, "bar-style");
        x << "dashed";
      }
    } break;
    case barline_double: {
      enclose xxx(x, "barline");
      {
        enclose yyy(x, "bar-style");
        x << "light-light";
      }
    } break;
    case barline_initfinal:
    case barline_final: {
      enclose xxx(x, "barline");
      {
        enclose yyy(x, "bar-style");
        x << "light-heavy";
      }
    } break;
    case barline_repeatleftright:
    case barline_repeatright: {
      enclose xxx(x, "barline");
      {
        enclose yyy(x, "bar-style");
        x << "heavy-light";
      }
      {
        enclose yyy();
        x << attr<const char*>("direction", "forward");
      }
    } break;
#ifndef NDEBUG
    case barline_initial:
    case barline_repeatleft:
    case barline_normal:;
#else
    default:;
#endif
    }
  }

  void xmloutdata::printpart(part& pa, module_noteobj& n, module_measobj& m) {
    enclose xxx(x, "part");
    x << attr<std::string>("id", pa.id);
    std::vector<int> clfs(module_totalnstaves(pa.p), -1);
    int measn = 0;
    fomus_rat cts = {-1, 1};
    bool fi = true;
    modout_keysig lm;
    lm.n = -1;
    int octch = 0;
    pa.bls = barline_normal;
    bool sysbreak = false;
    while (m && module_part(m) == pa.p) {
      printmeas(pa, m, n, clfs, measn, cts, fi, lm, octch, sysbreak);
      m = module_nextmeas();
      fi = false;
    }
  }

  void xmloutdata::modout_write(
      FOMUS fom, const char* filename) { // filename should be complete
    try {
      boost::filesystem::path fn(filename); //, mfn, dir;
      //   SAVE ME!
      //       std::string
      //       ext(boost::to_lower_copy(boost::filename::extension(fn)));
      //       boost::trim_left_if(ext, boost::lambda::_1 == '.');
      //       bool ismxl;
      //       if (ext == "mxl") {
      // 	ismxl = true;
      // 	mfn = fn;
      // 	dir = boost::filesystem::change_extension(fn, ".xml");
      // 	boost::filesystem::change_extension(fn, ".xml");
      //       } else ismxl = false;
      try {
        assert(x.isvalid());
        x.f.exceptions(boost::filesystem::ofstream::eofbit |
                       boost::filesystem::ofstream::failbit |
                       boost::filesystem::ofstream::badbit);
        assert(x.isvalid());
        x.f.open(fn.FS_FILE_STRING(), boost::filesystem::ofstream::out |
                                          boost::filesystem::ofstream::trunc);
        assert(x.isvalid());
        x.f << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
               "<!DOCTYPE score-partwise PUBLIC\n";
        if (x.inc > 0)
          x << std::string(x.inc * 2, ' ');
        else
          x << '\t';
        x.f << "\"-//Recordare//DTD MusicXML " << XML_VERSION_STRING
            << " Partwise//EN\"\n";
        if (x.inc > 0)
          x << std::string(x.inc * 2, ' ');
        else
          x << '\t';
        assert(x.isvalid());
        x.f << "\"http://www.musicxml.org/dtds/partwise.dtd\">\n"
               "<!-- MusicXML "
            << XML_VERSION_STRING
            << " Score File -->\n"
               "<!-- Generated by "
            << PACKAGE_STRING << " -->";
        time_t tim;
        { // --------------------HEADER
          if (time(&tim) != -1) {
            assert(x.isvalid());
#ifdef HAVE_CTIME_R
            char buf[27]; // user-supplied buffer of length at least 26
            x.f << "\n<!-- " << stripnl(ctime_r(&tim, buf)) << " -->";
#else
            x.f << "\n<!-- " << stripnl(ctime(&tim)) << " -->";
#endif
          }
        }
        {
          assert(x.isvalid());
          enclose xxx(x, "score-partwise");
          assert(x.isvalid());
          x << attr<const char*>("version", XML_VERSION_STRING);
          assert(x.isvalid());
          const char* title = module_setting_sval(fom, thetitleid);
          const char* author = module_setting_sval(fom, theauthorid);
          if (title[0]) {
            enclose xxx(x, "work");
            enclose yyy(x, "work-title");
            x << title;
          }
          {
            enclose xxx(x, "identification");
            if (author[0]) {
              enclose yyy(x, "creator");
              x << attr<const char*>("type", "composer") << author;
            }
            {
              enclose xxx(x, "encoding");
#ifdef HAVE_LOCALTIME_R
              struct tm ti;
              if (localtime_r(&tim, &ti)) {
                enclose xxx(x, "encoding-date");
                x << ti.tm_year + 1900 << '-';
                if (ti.tm_mon < 9)
                  x << '0';
                x << ti.tm_mon + 1 << '-';
                if (ti.tm_mday < 10)
                  x << '0';
                x << ti.tm_mday;
              }
#else
              struct tm* ti = localtime(&tim);
              if (ti) {
                enclose xxx(x, "encoding-date");
                x << ti->tm_year + 1900 << '-';
                if (ti->tm_mon < 9)
                  x << '0';
                x << ti->tm_mon + 1 << '-';
                if (ti->tm_mday < 10)
                  x << '0';
                x << ti->tm_mday;
              }
#endif
              {
                enclose xxx(x, "software");
                x << PACKAGE_STRING;
              }
            }
          }
          boost::ptr_vector<part> parts;
          int partid = 1;
          while (true) {
            module_partobj p(module_nextpart());
            if (!p)
              break;
            parts.push_back(new part(p, partid++));
          }
          {
            enclose xxx(x, "part-list");
            int groupid = 0;
            std::vector<int> grids;
            std::for_each(parts.begin(), parts.end(),
                          boost::lambda::bind(&xmloutdata::dostaves, this,
                                              boost::lambda::_1,
                                              boost::lambda::var(groupid),
                                              boost::lambda::var(grids)));
          }
          {
            module_noteobj n = module_nextnote();
            module_measobj m = module_nextmeas();
            std::for_each(parts.begin(), parts.end(),
                          boost::lambda::bind(
                              &xmloutdata::printpart, this, boost::lambda::_1,
                              boost::lambda::var(n), boost::lambda::var(m)));
          }
        }
        x.f << '\n';
        x.f.close();
        //#warning "use app-arch/zip for zip compression"
        std::string ext(module_setting_sval(fom, xmlviewextid));
        if (!ext.empty() && ext[0] != '.')
          ext = '.' + ext; // make sure it has a dot
        boost::filesystem::path opath(FS_CHANGE_EXTENSION(fn, ext));
        try {
          const char* path = module_setting_sval(fom, xmlviewexepathid);
          if (strlen(path) <= 0)
            return;
          if (module_setting_ival(fom, verboseid) >= 1)
            fout << "opening viewer..." << std::endl;
          struct module_list l(module_setting_val(fom, xmlviewexeargsid).val.l);
          execout::exec(0, path, std::vector<std::string>(), l,
                        opath.FS_FILE_STRING().c_str());
          return;
        } catch (const execout::execerr& e) {
          CERR << "error viewing `" << fn.FS_FILE_STRING() << '\'' << std::endl;
        }
      } catch (const boost::filesystem::ofstream::failure& e) {
        CERR << "error writing `" << fn.FS_FILE_STRING() << '\'' << std::endl;
      }
    } catch (const boost::filesystem::filesystem_error& e) {
      CERR << "invalid path/filename `" << filename << '\'' << std::endl;
    } catch (const errbase& e) {}
    cerr = true;
  }

  void xmloutdata::dostaves(part& pa, int& grid, std::vector<int>& grids) {
    while (true) {
      enum parts_grouptype gr = module_partgroupbegin(pa.p, grids.size() + 1);
      if (gr == parts_nogroup)
        break;
      enclose xxx(x, "part-group");
      x << attr<const char*>("type", "start") << attr<int>("number", grid);
      grids.push_back(grid++);
      {
        enclose xxx(x, "group-symbol");
        x << (gr == parts_grandstaff ? "brace" : "bracket");
      }
      {
        enclose xxx(x, "group-barline");
        x << (gr == parts_choirgroup ? "no" : "yes");
      }
    }
    {
      enclose xxx(x, "score-part");
      x << attr<std::string>("id", pa.id);
      {
        enclose xxx(x, "part-name");
        x << module_setting_sval(pa.p, partnameid);
      }
      {
        enclose xxx(x, "part-abbreviation");
        x << module_setting_sval(pa.p, partabbrid);
      }
    }
    while (!grids.empty() && module_partgroupend(pa.p, grids.size())) {
      enclose xxx(x, "part-group");
      x << attr<const char*>("type", "stop")
        << attr<int>("number", grids.back());
      grids.pop_back();
    }
  }

  const char* indenttype = "integer>=0";
  int valid_indenttype(const struct module_value val) {
    return module_valid_int(val, 0, module_incl, 0, module_nobound, 0,
                            indenttype);
  }
  const char* altervalstype = "(string (rational-128..128 rational-128..128), "
                              "string (rational-128..128 "
                              "rational-128..128), ...)";
  int valid_altervalstype_aux(int n, const char* sym, struct module_value val) {
    return module_valid_listofrats(val, 1, 2, module_makerat(-128, 1),
                                   module_incl, module_makerat(128, 1),
                                   module_incl, 0, altervalstype);
  }
  int valid_altervalstype(const struct module_value val) {
    return module_valid_maptovals(val, -1, -1, valid_altervalstype_aux,
                                  altervalstype);
  }
} // namespace xmlout

using namespace xmlout;
// ------------------------------------------------------------------------------------------------------------------------
// END OF NAMESPACE

const char* module_longname() {
  return "MusicXML File Output";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Writes `.xml' MusicXML score files for importing into music notation "
         "programs.";
}

void* module_newdata(FOMUS f) {
  return new xmloutdata(f);
}
void module_freedata(void* dat) {
  delete (xmloutdata*) dat;
}
const char* module_err(void* dat) {
  return ((xmloutdata*) dat)->module_err();
}

enum module_type module_type() {
  return module_modoutput;
}
const char* module_initerr() {
  return ierr;
}
int module_itertype() {
  return module_all /*| module_noinvisible*/;
}

void module_init() {
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 256), "256th"));
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 128), "128th"));
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 64), "64th"));
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 32), "32nd"));
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 16), "16th"));
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 8), "eighth"));
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 4), "quarter"));
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 2), "half"));
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 1), "whole"));
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(2, 1), "breve"));
  durtyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(4, 1), "long"));
  // also natural-sharp, natural-flat
  acctyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 1), "sharp"));
  acctyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(0, 1), "natural"));
  acctyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(-1, 1), "flat"));
  acctyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(2, 1), "double-sharp"));
  acctyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(-2, 1), "flat-flat"));
  acctyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(-1, 2), "quarter-flat"));
  acctyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(1, 2), "quarter-sharp"));
  acctyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(-3, 2), "three-quarters-flat"));
  acctyps.insert(std::map<fomus_rat, const char*>::value_type(
      module_makerat(3, 2), "three-quarters-sharp"));
  // // fermatas
  // fermatasmap.insert(fermatasmaptype::value_type("", fermata_norm));
  // fermatasmap.insert(fermatasmaptype::value_type("short", fermata_short));
  // fermatasmap.insert(fermatasmaptype::value_type("long", fermata_long));
  // fermatasmap.insert(fermatasmaptype::value_type("verylong", fermata_long2));
  // // arpeggios
  // arpeggiosmap.insert(arpeggiosmaptype::value_type("", arpeggio_norm));
  // arpeggiosmap.insert(arpeggiosmaptype::value_type("up", arpeggio_up));
  // arpeggiosmap.insert(arpeggiosmaptype::value_type("down", arpeggio_down));
  // arpeggiosmap.insert(arpeggiosmaptype::value_type("bracket",
  // arpeggio_bracket));
  // arpeggiosmap.insert(arpeggiosmaptype::value_type("parenthesis",
  // arpeggio_paren));
  // // slurs
  // slursmap.insert(slursmaptype::value_type("", slur_norm));
  // slursmap.insert(slursmaptype::value_type("dotted", slur_dotted));
  // slursmap.insert(slursmaptype::value_type("dashed", slur_dashed));
  // pedal styles
  pedstyles.insert(pedmaptype::value_type("text", false));
  pedstyles.insert(pedmaptype::value_type("bracket", true));
}
void module_free() { /*assert(newcount == 0);*/
}

int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "xml-indent"; // docscat{xmlout}
    set->type = module_int;
    set->descdoc =
        "Number of spaces used for indenting in a MusicXML output file.  A "
        "value of 0 means tab characters are used for indentation.";
    set->typedoc = indenttype;

    module_setval_int(&set->val, 0);

    set->loc = module_locscore;
    set->valid = valid_indenttype;
    set->uselevel = 2;
    indentid = id;
    break;
  }
  case 1: {
    set->name = "xml-suppress-acctag"; // docscat{xmlout}
    set->type = module_bool;
    set->descdoc = "Whether to suppress the <accidental> and <accidental-mark> "
                   "tags in a MusicXML output file.  "
                   "This prevents FOMUS from complaining that it can't write "
                   "an accidental when it is outside the range of values "
                   "acceptable for MusicXML.";
    // set->typedoc = indenttype;

    module_setval_int(&set->val, 0);

    set->loc = module_locnote;
    // set->valid = valid_indenttype;
    set->uselevel = 3;
    praccid = id;
    break;
  }
  case 2: {
    set->name = "xml-altervals"; // docscat{xmlout}
    set->type = module_symmap_numlists;
    set->descdoc =
        "A mapping that translates accidentals into numbers to be inserted in "
        "the <alter> tag in a MusicXML output file.  "
        "The strings are the values to appear inside the <alter> tags (which "
        "should be integers or floats according the the MusicXML spec) and "
        "the pairs of rationals are the accidentals that are translated into "
        "the <alter> values (FOMUS stores two accidentals internally, "
        "a \"normal\" adjustment and a microtonal adjustment which correspond "
        "respectively to the `note-accs' and `note-microtones' settings).  "
        "Each pair may also be a single value, in which case the second value "
        "(the microtonal adjustment) is 0.  "
        "This setting allows for alternate methods of importing microtonal or "
        "other special accidentals that aren't directly supported in the "
        "MusicXML spec.  "
        "(You might also want to set `xml-suppress-acctag' to `no' when using "
        "this setting.)";
    set->typedoc = altervalstype;

    module_setval_list(&set->val, 0);

    set->loc = module_locnote;
    set->valid = valid_altervalstype;
    set->uselevel = 3;
    altervalsid = id;
    break;
  }
  case 3: {
    set->name = "xml-view-extension"; // docscat{xmlout}
    set->type = module_string;
    set->descdoc = "Filename extension expected by the MusicXML output viewer "
                   "application set in `xml-view-exe-path'."
                   "  (This setting will be more useful when it's possible to "
                   "save compressed `.mxl' files.)";
    // set->typedoc = writesettingstype;

    module_setval_string(&set->val, "xml");

    set->loc = module_locscore;
    // set->valid = valid_writesets;
    set->uselevel = 3;
    xmlviewextid = id;
    break;
  }
  case 4: {
    set->name = "xml-view-exe-path"; // docscat{xmlout}
    set->type = module_string;
    set->descdoc = "Path to executable of viewer application to launch for "
                   "viewing MusicXML output files."
                   "  If this is specified, FOMUS automatically displays the "
                   "output `.pdf' file."
                   "  If the viewer executable is in your path then only the "
                   "filename is necessary."
                   "  Set this to an empty string to prevent FOMUS from "
                   "invoking the viewer automatically.";
    // set->typedoc = writesettingstype;

    module_setval_string(&set->val, "");

    set->loc = module_locscore;
    // set->valid = valid_writesets;
    set->uselevel = 2;
    xmlviewexepathid = id;
    break;
  }
  case 5: {
    set->name = "xml-view-exe-args"; // docscat{xmlout}
    set->type = module_list_strings;
    set->descdoc = "A list of additional arguments to be passed to the "
                   "MusicXML output viewer application set in "
                   "`xml-view-exe-path' (in addition to the input filename)."
                   "  Each string is a shell command line argument.";
    // set->typedoc = accslisttype;

    module_setval_list(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    xmlviewexeargsid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}

void module_ready() {
  verboseid = module_settingid("verbose");
  if (verboseid < 0) {
    ierr = "missing required setting `verbose'";
    return;
  }
  timesigstyleid = module_settingid("timesig-c");
  if (timesigstyleid < 0) {
    ierr = "missing required setting `timesig-c'";
    return;
  }
  partnameid = module_settingid("name");
  if (partnameid < 0) {
    ierr = "missing required setting `name'";
    return;
  }
  partabbrid = module_settingid("abbr");
  if (partabbrid < 0) {
    ierr = "missing required setting `abbr'";
    return;
  }
  thetitleid = module_settingid("title");
  if (thetitleid < 0) {
    ierr = "missing required setting `title'";
    return;
  }
  theauthorid = module_settingid("author");
  if (theauthorid < 0) {
    ierr = "missing required setting `author'";
    return;
  }
  beatid = module_settingid("beat");
  if (beatid < 0) {
    ierr = "missing required setting `beat'";
    return;
  }
  pedstyleid = module_settingid("ped-style");
  if (pedstyleid < 0) {
    ierr = "missing required setting `ped-style'";
    return;
  }
}

// only fmsout.cc returns true here (outputs before processing anything)
fomus_bool modout_ispre() {
  return false;
}

const char* modout_get_extension(int n) {
  switch (n) {
  case 0:
    return "xml";
  default:
    return 0;
  }
}

// for non-file output
const char* modout_get_saveid() {
  return 0;
}

void modout_write(FOMUS f, void* dat, const char* filename) {
  ((xmloutdata*) dat)->modout_write(f, filename);
}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
