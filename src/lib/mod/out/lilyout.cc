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
#include <sstream>
//#include <cmath>
#include <algorithm>
#include <cstring> // strlen
#include <ctime>   // need this
#include <fstream>
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

//#define LILYPOND_VERSION_STRING "2.12.1"
#define LILYPOND_VERSION_STRING "2.12"

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

namespace lilyout {

  const char* ierr = 0;

#define CMD_MACRO_STRINGIFY(xxx) #xxx
#define CMD_MACRO(xxx) CMD_MACRO_STRINGIFY(xxx)

// things that might change...
#define LILY_DYNMARKUP_CONST(xxx) "\\markup \\dynamic \"" << xxx << "\""
#define LILY_DYNMARKUP(xxx) ("\\markup \\dynamic \"" + quotify(xxx) + "\"")

#define LILY_TEXT_CONST(xxx) "\"" << xxx << '"'
#define LILY_TEXT(xxx) ("\"" + quotify(xxx) + '"')

#define LILY_MEASTEXT_CONST(xxx) "\\markup \\bold \"" << xxx << '"'
#define LILY_MEASTEXT(xxx) ("\\markup \\bold \"" + quotify(xxx) + '"')
#define LILY_ITALTEXT(xxx) ("\\markup \\italic \"" + quotify(xxx) + '"')

#define LILY_GRACESLASH "\\once \\override Stem #'stroke-style = #\"grace\""

  inline std::ostream& operator<<(std::ostream& out, const fomus_rat& x) {
    out << module_rattostr(x);
    return out;
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

  struct errbase {};

  int indentid, userpartheadid, lilyexepathid, usertopheadid, extramacrosid,
      userheadid, writewidthid, accslistid, /*maxtupslvlid,*/ timesigstyleid,
      autobeamid, autoaccsid, // clefid,
      lilyexeargsid, lilyviewexepathid, lilyviewextid, lilyviewexeargsid,
      verboseid, lilymacrostaffid, partnameid, partabbrid, lilymacroslashid,
      lilymacroppppppid, lilymacrosfffid, lilymacrosffzid, lilymacrosfffzid,
      lilymacroffzid, lilymacrofffzid, lilymacrorfzid, lilymacrorfid,
      lilypartprefixid, lilymacrobeamid, notenameslistid, thetitleid,
      theauthorid, lilymacropretextspanid, lilymacropremeastextspanid,
      defgracedurid, pitchedtrillid, beatid, lilymacropreitaltextspanid,
      lilytextinsertid, lilymacropedstyletextid, lilymacropedstylebracketid,
      pedstyleid, lilypapersizeid, lilypaperorientid, lilystaffsizeid,
      lilymacrofzpid, lilymacrosfpid, lilymacrosfzpid;

  struct noteholder {
    module_noteobj n;
    int st,
        cl; // staff, adjusted because lilypond can't split chords across staves
    bool chcl;
    noteholder(const module_noteobj n)
        : n(n), st(module_staff(n)), cl(module_clef(n)), chcl(false) {}
  };
  bool order_aux(const module_noteobj x, const module_noteobj y) {
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
    return module_isrest(x)
               ? module_isrest(y)
               : (module_isrest(y) || module_pitch(x) < module_pitch(y));
  }
  struct order
      : std::binary_function<const noteholder&, const noteholder&, bool> {
    bool operator()(const noteholder& x, const noteholder& y) const {
      return order_aux(x.n, y.n);
    }
  };

  struct encloseinfo {
    boost::filesystem::ofstream f;
    int ind; // indent size
    const int inc;
    bool eol;
    bool ok;
    encloseinfo(const int inc) : ind(0), inc(inc), eol(true), ok(true) {}
    void incind() {
      ind += inc;
    }
    void decind() {
      ind -= inc;
    }
  };
  template <typename T>
  inline encloseinfo& operator<<(encloseinfo& out, const T& o) {
    if (out.ok) {
      if (out.eol) {
        out.f << std::string(out.ind, ' ');
        out.eol = false;
      }
      out.f << o;
    }
    return out;
  }
  struct el {};
  template <>
  inline encloseinfo& operator<<<el>(encloseinfo& out, const el& el) {
    if (out.ok) {
      if (out.eol)
        out.f << std::string(out.ind, ' ');
      else
        out.eol = true;
      out.f << '\n'; // << string(el.info.ind, ' ');
                     // //endl.info.setnpos(out.tellp());
    }
    return out;
  }

  class enclose {
    encloseinfo& info;
    const char* en;

public:
    enclose(encloseinfo& info, const char* be, const char* en)
        : info(info), en(en) {
      info << be;
      info.incind();
    }
    enclose(encloseinfo& info, const std::string& be, const char* en)
        : info(info), en(en) {
      info << be;
      info.incind();
    }
    ~enclose() {
      info.decind();
      info.f.exceptions(
          boost::filesystem::ofstream::goodbit); // avoid exceptions in
                                                 // destructor
      info << en << '\n';
      info.eol = true;
      info.f.exceptions(boost::filesystem::ofstream::eofbit |
                        boost::filesystem::ofstream::failbit |
                        boost::filesystem::ofstream::badbit);
    }
  };

  // lilypond is case sensitive
  inline std::string macrofy(const std::string& str) {
    std::ostringstream out;
    for (std::string::const_iterator i(str.begin()); i != str.end(); ++i) {
      if (isalpha(*i))
        out << *i;
    }
    return out.str();
  }

  struct part {
    module_partobj p;
    std::string var;     // lilypond variable name
    bool ingrsl, isaphr; // flags for slurs which cross measure boundaries
    part(const module_partobj p, const std::string& var)
        : p(p), var(var),
          /*altarps(false), altslurs(false), altphrases(false),*/ ingrsl(false),
          isaphr(false) {}
    void print(std::set<std::string>& names, encloseinfo& x, module_measobj& m,
               module_noteobj& o);
  };

  std::map<int, const char*> specaccs;

  struct lilyoutdata {
    struct aaas {
      fomus_rat a1, a2;
    };
    FOMUS fom;
    bool cerr;
    std::stringstream CERR;
    std::string errstr;
    encloseinfo x;
    fomus_int width;
    std::map<std::pair<fomus_rat, fomus_rat>, std::string> accs;
    bool autobeam;
    std::string autoaccs;
    // macros
    std::string macrostaff, macrobeam, macroslash, macropppppp, macrosfff,
        macrosffz, macrosfffz, macroffz, macrofffz, macrorfz, macrorf,
        macropretextspan, macropremeastextspan, macropreitaltextspan,
        macropedstyletext, macropedstylebracket, macrofzp, macrosfp, macrosfzp;
    bool mpppppp, msfff, msffz, msfffz, mffz, mfffz, mrfz, mrf, mslash,
        mpretextspan, mpremeastextspan, mpreitaltextspan, mpedstyletext,
        mpedstylebracket, mfzp, msfp, msfzp;
    int mstaves, mbeams;
    const char* partprefix;

    const char* dnotearr[7]; // = {'c', 'd', 'e', 'f', 'g', 'a', 'b'};

    const char* module_err() {
      if (!cerr)
        return 0;
      std::getline(CERR, errstr);
      return errstr.c_str();
    }
    void dostaves(boost::ptr_vector<part>::const_iterator& p1,
                  const boost::ptr_vector<part>::const_iterator& pend,
                  const int grlvl, const bool ingr /*, const int grlvl0*/);
    lilyoutdata(FOMUS fom)
        : fom(fom), cerr(false), x(module_setting_ival(fom, indentid)),
          width(module_setting_ival(fom, writewidthid)),
          macrostaff(module_setting_sval(fom, lilymacrostaffid)),
          macrobeam(module_setting_sval(fom, lilymacrobeamid)),
          macroslash(module_setting_sval(fom, lilymacroslashid)),
          macropppppp(module_setting_sval(fom, lilymacroppppppid)),
          macrosfff(module_setting_sval(fom, lilymacrosfffid)),
          macrosffz(module_setting_sval(fom, lilymacrosffzid)),
          macrosfffz(module_setting_sval(fom, lilymacrosfffzid)),
          macroffz(module_setting_sval(fom, lilymacroffzid)),
          macrofffz(module_setting_sval(fom, lilymacrofffzid)),
          macrorfz(module_setting_sval(fom, lilymacrorfzid)),
          macrorf(module_setting_sval(fom, lilymacrorfid)),
          macropretextspan(module_setting_sval(fom, lilymacropretextspanid)),
          macropremeastextspan(
              module_setting_sval(fom, lilymacropremeastextspanid)),
          macropreitaltextspan(
              module_setting_sval(fom, lilymacropreitaltextspanid)),
          macropedstyletext(module_setting_sval(fom, lilymacropedstyletextid)),
          macropedstylebracket(
              module_setting_sval(fom, lilymacropedstylebracketid)),
          macrofzp(module_setting_sval(fom, lilymacrofzpid)),
          macrosfp(module_setting_sval(fom, lilymacrosfpid)),
          macrosfzp(module_setting_sval(fom, lilymacrosfzpid)), mpppppp(false),
          msfff(false), msffz(false), msfffz(false), mffz(false), mfffz(false),
          mrfz(false), mrf(false), mslash(false), mpretextspan(false),
          mpremeastextspan(false), mpreitaltextspan(false),
          mpedstyletext(false), mpedstylebracket(false), mfzp(false),
          msfp(false), msfzp(false), mstaves(0), mbeams(0),
          partprefix(
              module_setting_sval(fom, lilypartprefixid)) { // global setting
      module_value x(module_setting_val(fom, notenameslistid));
      assert(x.type == module_list);
      int z = 0;
      for (module_value *i = x.val.l.vals, *ie = x.val.l.vals + x.val.l.n;
           i < ie; ++i, ++z) {
        assert(i->type == module_string);
        dnotearr[z] = i->val.s;
      }
      module_value a(module_setting_val(fom, accslistid));
      assert(a.type == module_list);
      static aaas aaa[] = {
          {{-2, 1}, {0, 1}}, {{-1, 1}, {-1, 2}}, {{-1, 1}, {0, 1}},
          {{-1, 1}, {1, 2}}, {{0, 1}, {-1, 2}},  {{0, 1}, {0, 1}},
          {{0, 1}, {1, 2}},  {{1, 1}, {-1, 2}},  {{1, 1}, {0, 1}},
          {{1, 1}, {1, 2}},  {{2, 1}, {0, 1}}};
      aaas* aaa0 = aaa;
      for (module_value *i = a.val.l.vals, *ie = a.val.l.vals + a.val.l.n;
           i < ie; ++i, ++aaa0) {
        assert(i->type == module_string);
        accs.insert(
            std::map<std::pair<fomus_rat, fomus_rat>, std::string>::value_type(
                std::pair<fomus_rat, fomus_rat>(aaa0->a1, aaa0->a2), i->val.s));
      }
    }
    void insertuserstuff(const int id);
    void insertuserstuffaux(const module_value& uh);
    bool printnoteacc(module_noteobj no, std::ostream& ou,
                      const struct module_keysigref* keyacc, bool actual);
    bool printnoteaccaux(std::ostream& ou, const fomus_rat& ac1,
                         const fomus_rat& ac2,
                         const struct module_keysigref* keyacc);
    void measprint(part& prt, std::vector<noteholder>::iterator& ni,
                   const std::vector<noteholder>& nos,
                   const std::vector<module_measobj>& meass); // no is modified
    void lyrprint(part& prt, std::vector<noteholder>::iterator& ni,
                  const std::vector<noteholder>& nos,
                  const std::vector<module_measobj>& meass); // no is modified
    void partprint(part& prt, std::set<std::string>& names, module_measobj& m,
                   module_noteobj& o);
    void modout_write(FOMUS fom, const char* filename);
    void printnotedur(const module_noteobj no, const fomus_rat& wrm,
                      std::ostream& ou, const bool gr, const fomus_int div);
    void printnoteduraux(std::ostream& ou, const fomus_rat& r);
    void justify(std::string s);
    void printnotenote(const module_noteobj no, std::ostream& ou,
                       const struct module_keysigref* keyref, bool actual);
  };

  void lilyoutdata::insertuserstuffaux(const module_value& uh) {
    assert(uh.type == module_list);
    for (module_value *i = uh.val.l.vals, *ie = uh.val.l.vals + uh.val.l.n;
         i < ie; ++i) {
      assert(i->type == module_string);
      x << i->val.s << el();
    }
  }
  inline void lilyoutdata::insertuserstuff(const int id) {
    insertuserstuffaux(module_setting_val(fom, id));
  }
  // ------------------------------------------------------------------------------------------------------------------------

  void lilyoutdata::printnotenote(const module_noteobj no, std::ostream& ou,
                                  const struct module_keysigref* keyref,
                                  bool actual) {
    static const char* oarr[11] = {",,,,", ",,,", ",,",   ",",     "",      "'",
                                   "''",   "'''", "''''", "'''''", "''''''"};
    if (module_isrest(no)) {
      if (modout_isinvisible(no))
        ou << 's';
      else
        ou << 'r';
    } else {
      fomus_int wn(module_writtennote(no));
      ou << dnotearr[todiatonic(wn % 12)];
      bool exc = printnoteacc(no, ou, &keyref[todiatonic(wn)], actual);
      ou << oarr[wn / 12];
      if (exc)
        ou << '!';
    }
  }
  bool lilyoutdata::printnoteaccaux(std::ostream& ou, const fomus_rat& ac1,
                                    const fomus_rat& ac2,
                                    const struct module_keysigref* keyacc) {
    std::map<std::pair<fomus_rat, fomus_rat>, std::string>::const_iterator i(
        accs.find(std::pair<fomus_rat, fomus_rat>(ac1, ac2)));
    if (i == accs.end()) {
      CERR << "can't write accidental";
      throw errbase();
    }
    if (i->second.empty())
      return keyacc;
    else {
      ou << i->second;
      return keyacc ? (keyacc->acc1 == ac1 && keyacc->acc2 == ac2) : false;
    }
  }
  bool lilyoutdata::printnoteacc(module_noteobj no, std::ostream& ou,
                                 const struct module_keysigref* keyacc,
                                 bool actual) {
    // fomus_rat ac1;
    bool tl = module_istiedleft(no);
    if (tl) {
      no = module_leftmosttiednote(
          no); // get the exact same accidental, or lilypond won't tie it
    }
    if (actual) {
      fomus_rat ac1 = module_acc1(no);
      fomus_rat ac2 = module_acc2(no);
      if (ac1 != (fomus_int) 0 || ac2 != (fomus_int) 0) {
        printnoteaccaux(ou, ac1, ac2, keyacc);
      }
      return false; // return true to force it
    } else {
      fomus_rat ac1 = module_writtenacc1(no);
      if (ac1 == std::numeric_limits<fomus_int>::max()) { // fomus sez none
                                                          // should appear
        if (keyacc->acc1 == (fomus_int) 0 && keyacc->acc2 == (fomus_int) 0)
          return false;
        printnoteaccaux(ou, keyacc->acc1, keyacc->acc2, keyacc);
        return false;
      } else { // fomus sez show an accidental
        assert(module_writtenacc2(no) != std::numeric_limits<fomus_int>::max());
        bool em = printnoteaccaux(ou, ac1, module_writtenacc2(no), keyacc);
        return (em && !tl && autoaccs.empty()); // return true to force it
      }
    }
  }
  inline void printnotetie(const module_noteobj no, std::ostream& ou) {
    if (module_istiedright(no))
      ou << '~';
  }
  void lilyoutdata::printnoteduraux(std::ostream& ou, const fomus_rat& r) {
    struct modutil_rhythm rh(rhythm(r));
#warning                                                                       \
    "*** FOMUS should make sure this is always the case!--make it a check stage after divide stage ***"
    assert(rh.dur.den != 0);
    switch (rh.dur.num) {
    case 1:
      ou << rh.dur.den;
      break;
    case 2:
      ou << "\\breve";
      break;
    case 4:
      ou << "\\longa";
      break;
    default:
      CERR << "can't write duration" << std::endl;
      throw errbase();
    }
    for (int i = 0; i < rh.dots; ++i)
      ou << '.';
  }
  void lilyoutdata::printnotedur(const module_noteobj no, const fomus_rat& wrm,
                                 std::ostream& ou, const bool gr,
                                 const fomus_int div = 0) {
    fomus_rat du(gr ? module_adjgracedur(no, -1) : module_adjdur(no, -1));
    if (div)
      du = du / div;
    printnoteduraux(ou, du * wrm);
  }
  inline void printfmrdur(const fomus_rat& du, const fomus_rat& wrm,
                          std::ostream& ou) {
    ou << '1';
    fomus_rat rh(du * wrm);
    if (rh != (fomus_int) 1)
      ou << '*' << rh.num << '/' << rh.den;
  }

  void lilyoutdata::justify(std::string s) {
    // const int first = fi ? 0 : start;
    boost::replace_all(s, "\t", std::string(8, ' '));
    std::string::size_type i = 0;
    std::string::size_type je = s.length();
    // const string exc("\"'`{[(");
    for (std::string::size_type j = width - x.ind; j < je; j += width - x.ind) {
      std::string::size_type js = i;
      while (js < j && s[js] != '\n')
        ++js;                         // js is first \n or j
      if (js < j) {                   // found \n
        x << s.substr(i, (js++) - i); // << string(x.ind, ' ');
        i = j = js;
      } else {
        js = j;
        if (j < je && s[j] == ' ')
          ++j;
        while (j > i && s[j] == ' ' /*(va ? s[j] == ' ' : !(isalnum(s[j]) || exc.find(s[j]) < exc.length()))*/)
          --j;
        while (j > i && s[j - 1] != ' ' /*(va ? s[j - 1] != ' ' : isalnum(s[j - 1]) || exc.find(s[j - 1]) < exc.length())*/)
          --j;
        if (j > i) {
          x << s.substr(i, std::min(j, js) - i)
            << el(); // << string(x.ind, ' ');
          while (j < je && s[j] == ' ')
            ++j;
          i = j;
        } else {
          x << s.substr(i, js - i) << el(); // << string(x.ind, ' ');
          i = j = js;
        }
      }
    }
    x << s.substr(i) << el();
  }

  typedef std::map<int, const char*> clefsmaptype;
  clefsmaptype clefsmap;
  inline const char* tolilyclef(const int cl) {
#ifndef NDEBUG
    clefsmaptype::const_iterator i(clefsmap.find(cl));
    assert(i != clefsmap.end());
    return i->second;
#else
    return clefsmap.find(cl)->second;
#endif
  }

  // enum fermata_types {fermata_norm, fermata_short, fermata_long,
  // fermata_long2};
  // //struct charisiless:std::binary_function<const char*, const char*, bool> {
  // //bool operator()(const char* x, const char* y) const {return
  // boost::algorithm::ilexicographical_compare(x, y);}
  // //};
  // typedef std::map<const char*, fermata_types, charisiless> fermatasmaptype;
  // fermatasmaptype fermatasmap;

  // enum arpeggio_types {arpeggio_none, arpeggio_norm, arpeggio_up,
  // arpeggio_down, arpeggio_bracket, arpeggio_paren}; typedef std::map<const
  // char*, arpeggio_types, charisiless> arpeggiosmaptype; arpeggiosmaptype
  // arpeggiosmap;

  // enum slur_types {slur_none, slur_norm, slur_dotted, slur_dashed};
  // typedef std::map<const char*, slur_types, charisiless> slursmaptype;
  // slursmaptype slursmap;

  enum ps_types { ps_text, ps_bracket };
  typedef std::map<const char*, ps_types, charisiless> pedmaptype;
  pedmaptype pedstyles;

#warning "*** test for autobeam ***"
#warning "*** test for autoacc ***"

  struct scoped_rangeobj {
    modutil_rangesobj o;
    scoped_rangeobj() : o(ranges_init()) {}
    ~scoped_rangeobj() {
      ranges_free(o);
    }
    void insert(const fomus_rat& x1, const fomus_rat& x2) const {
      modutil_range v = {module_makeval(x1), module_makeval(x2)};
      ranges_insert(o, v);
    }
    void remove(const fomus_rat& x1, const fomus_rat& x2) const {
      modutil_range v = {module_makeval(x1), module_makeval(x2)};
      ranges_remove(o, v);
    }
    fomus_int size() const {
      return ranges_size(o);
    }
  };

#warning                                                                       \
    "*** more pre-output checks needed: chords are all same time/endtime, voices stretch across measure ***"

  inline std::string quotify(std::string s) {
    boost::replace_all(s, "\"", "\\\"");
    return s;
  }

  inline char where(const module_markobj m) {
    switch (module_markpos(m)) {
    case markpos_notehead:
      return '-';
    case markpos_above:
      return '^';
    case markpos_below:
      return '_';
    default:
      DBG("failed with pos = " << module_markpos(m) << std::endl);
      assert(false);
    }
  }

  // *******************VOICES are sorted so SEPARATED only when they are
  // MULTIPLE, one after the other
  // *******************voice 0 = single voice, real voices start at 1 when they
  // are in ||
  void lilyoutdata::measprint(
      part& prt, std::vector<noteholder>::iterator& ni,
      const std::vector<noteholder>& nos,
      const std::vector<module_measobj>& meass) { // no is modified
    std::vector<noteholder>::const_iterator bg(ni);
    int nst = module_totalnstaves(prt.p);
    bool inchd = false;
    int vs0 = -1;
    int octch = 0;
    bool ingrbr = false, incresc = false, indim = false;
    int curarptype = mark_arpeggio;
    ps_types curpedtype = ps_text;
    int curslurtype = mark_slur_begin;
    int curphrasetype = mark_phrase_begin;
    enum module_markpos txspos = markpos_prefmiddleorabove;
    assert(!meass.empty());
    std::vector<module_measobj>::const_iterator me(meass.begin());
    assert(*me == 0);
    std::ostringstream ou;
    int cstaff = (nst > 1) ? -1 : 1; // current staff
    fomus_int mn = 0;                // measure number
    fomus_rat curts = {0, 1};        // current time signature
    bool csy = true;
    int thisv = module_voice(ni->n);
    bool mat = false;
    modout_keysig lmkey;
    const struct module_keysigref* ksref;
    lmkey.n = -1;
    fomus_int isintrem = 0;
    bool inalongtr = false;
    bool isbreak = false;
    while (true) {
      bool done = (ni == nos.end() || module_voice(ni->n) != thisv);
      module_measobj me0;
      if (!done)
        me0 = module_meas(ni->n);
      while (done || me0 != *me) {
        bool isfirst;
        if (*me) {
          fomus_rat ts(module_timesig(*me));
          if (!mat) {
            ou << "\\skip 1*" << ts.num << '/' << ts.den << ' ';
          } else
            mat = false;
          if (ingrbr) {
            ou << "} ";
            ingrbr = false;
          }
          if (boost::next(me) == meass.end() && inalongtr) {
            ou << "\\grace { s"
               << (module_setting_rval(*me, defgracedurid) *
                   module_writtenmult(*me))
                      .den
               << "\\stopTrillSpan } ";
          }
          switch (modout_rightbarline(*me)) {
          case barline_dotted:
            ou << "\\bar \":\" ";
            break;
          case barline_dashed:
            ou << "\\bar \";\" ";
            break;
          case barline_double:
            ou << "\\bar \"||\" ";
            break;
          case barline_initial:
            ou << "\\bar \".|\" ";
            break;
          case barline_final:
            ou << "\\bar \"|.\" ";
            break;
          case barline_initfinal:
            ou << "\\bar \"|.|\" ";
            break;
          case barline_repeatleft:
            ou << "\\bar \":|\" ";
            break;
          case barline_repeatright:
            ou << "\\bar \"|:\" ";
            break;
          case barline_repeatleftright:
            ou << "\\bar \":|.|:\" ";
            break;
#ifndef NDEBUG
          case barline_normal:;
#endif
          }
          if (isbreak) {
            ou << "\\break ";
            isbreak = false;
          }
          ou << '|';
          if (x.ok)
            justify(ou.str());
          ou.str("");
          isfirst = false;
        } else
          isfirst = true;
        ++me;
        if (me == meass.end())
          break;
        fomus_rat ts(module_timesig(*me));
        x << el() << std::string(x.inc + 2, ' ') << "% measure " << ++mn
          << " -- " << ts.num << '/' << ts.den << el();
        DBG("measure = " << mn << std::endl);
        bool sy(module_setting_ival(*me, timesigstyleid));
        bool dosy, dots, kss;
        if (sy != csy) {
          csy = sy;
          dosy = true;
        } else
          dosy = false;
        if (ts.num != curts.num || ts.den != curts.den) {
          curts = ts;
          dots = true;
        } else
          dots = false;
        bool prsm = false;
        struct modout_keysig ks(modout_keysigdef(*me));
        if (!modout_keysigequal(ks, lmkey)) {
          lmkey = ks;
          ksref = module_keysigref(*me);
          kss = true;
        } else
          kss = false;
        for (int i = 1; i <= nst; ++i) {
          if (i != cstaff && (isfirst || dots || dosy || kss)) {
            if (macrostaff.empty())
              x << "\\change Staff = ";
            else {
              x << '\\' << macrostaff;
              if (i > mstaves)
                mstaves = i;
            }
            x << toroman(i) << ' ';
            cstaff = i;
          }
          if (kss || isfirst) {
            prsm = true;
            switch (ks.mode) {
            case keysig_common_maj:
            case keysig_common_min: {
              assert(ks.dianote >= 0 && ks.dianote < 7);
              x << "\\key " << dnotearr[ks.dianote];
              std::map<std::pair<fomus_rat, fomus_rat>,
                       std::string>::const_iterator
                  i(accs.find(std::pair<fomus_rat, fomus_rat>(
                      module_makerat(ks.acc, 1), module_makerat(0, 1))));
              if (i == accs.end()) {
                CERR << "can't write key signature accidental";
                throw errbase();
              }
              x << i->second
                << (ks.mode == keysig_common_maj ? " \\major " : " \\minor ");
              break;
            }
            case keysig_indiv: {
              x << "\\set Staff.keySignature = #`(";
              for (const struct modout_keysig_indiv *i(ks.indiv),
                   *ie(ks.indiv + ks.n);
                   i < ie; ++i) {
                assert(i->dianote >= 0 && i->dianote < 12);
                x << '(' << todiatonic(i->dianote) << " . ,";
                std::map<int, const char*>::const_iterator j(
                    specaccs.find(ks.acc));
                if (j == specaccs.end() || i->acc2 != (fomus_int) 0) {
                  CERR << "can't write key signature accidental";
                  throw errbase();
                }
                x << j->second << ')';
              }
              x << ") ";
              break;
            }
            case keysig_fullindiv: {
              x << "\\set Staff.keySignature = #`(";
              for (const struct modout_keysig_indiv *i(ks.indiv),
                   *ie(ks.indiv + ks.n);
                   i < ie; ++i) {
                x << "((" << (i->dianote / 7 - 5) << " . "
                  << todiatonic(i->dianote) << ") . ,";
                std::map<int, const char*>::const_iterator j(
                    specaccs.find(ks.acc));
                if (j == specaccs.end() || i->acc2 != (fomus_int) 0) {
                  CERR << "can't write key signature accidental";
                  throw errbase();
                }
                x << j->second << ')';
              }
              x << ") ";
            }
            default:
              CERR << "can't write key signature";
              throw errbase();
            }
          }
          if (dosy || isfirst) {
            prsm = true;
            x << (sy ? "\\defaultTimeSignature " : "\\numericTimeSignature ");
          }
          if (dots || isfirst) {
            prsm = true;
            x << "\\time " << ts.num << '/' << ts.den << ' ';
          }
        }
        if (prsm)
          x << el();
      }
      if (done) {
        assert(!mat);
        break;
      }
      module_noteobj no = ni->n;
      fomus_rat wrm(module_writtenmult(*me));
      mat = true;
      bool fmr = module_isfullmeasrest(*me);
      if (!inchd && !fmr) {
        for (int i = 0;; ++i) { // TUPLETS BEGIN
          fomus_rat fr(module_tuplet(no, i));
          if (fr <= (fomus_int) 0)
            break;
          if (module_tupletbegin(no, i)) {
            if (ingrbr) {
              ou << "} ";
              ingrbr = false;
            }
            ou << "\\times " << fr.den << '/' << fr.num << " { ";
          }
        }
      }
      int s = ni->st; // module_staff(no); // STAFF
      if (s != cstaff) {
        assert(!inchd);
        cstaff = s;
        if (macrostaff.empty())
          ou << "\\change Staff = ";
        else {
          ou << '\\' << macrostaff;
          if (s > mstaves)
            mstaves = s;
        }
        ou << toroman(s) << ' ';
      }
      if (!inchd && ni->chcl && module_voice(ni->n) < 1000) {
        ou << "\\clef " << tolilyclef(ni->cl) << ' ';
      }
      module_markslist mks(module_marks(no));
      bool isgr = module_isgrace(no);
      bool ischlow = module_ischordlow(no);
      bool ischhigh = module_ischordhigh(no);
      bool forcech = false;
      if (ischlow && ischhigh) {
        for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n); m < me;
             ++m) {
          switch (module_markbaseid(module_markid(*m))) {
          case mark_natharm_touched: // goto next
          case mark_artharm_touched:
            // case mark_artharm_sounding:
            // case mark_natharm_sounding:
            forcech = true;
            goto EXITM;
          }
        }
      }
    EXITM:
      bool istrillwnote = false;
      if (!inchd) {
        if (ingrbr && !isgr) {
          ou << "} ";
          ingrbr = false;
        } else if (isgr && !ingrbr) {
          ou << "\\grace { ";
          ingrbr = true;
        }
        int vs00 = module_staffvoice(no); // voice within 1 staff
        if (vs0 != vs00) {
          vs0 = vs00;
          switch (vs0) {
          case 1:
            ou << "\\voiceOne ";
            break;
          case 2:
            ou << "\\voiceTwo ";
            break;
          case 3:
            ou << "\\voiceThree ";
            break;
          case 4:
            ou << "\\voiceFour ";
            break;
          default:
            ou << "\\oneVoice ";
          }
        }
        for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n); m < me;
             ++m) { // marks that need toggles
          assert(module_markbaseid(module_markid(*m)) < mark_nmarks);
          int id = module_markbaseid(module_markid(*m));
          switch (id) {
          case mark_arpeggio:
          case mark_arpeggio_up:
          case mark_arpeggio_down: {
            if (id != curarptype) {
              switch (id) {
              case mark_arpeggio:
                ou << "\\arpeggioNormal ";
                break;
              case mark_arpeggio_up:
                ou << "\\arpeggioArrowUp ";
                break;
              case mark_arpeggio_down:
                ou << "\\arpeggioArrowDown ";
                break;
#ifndef NDEBUG
              default:
                assert(false);
#endif
              }
              curarptype = id;
            }
            break;
          }
          case mark_graceslur_begin:
            prt.ingrsl = true;
            goto TOSLUR1;
          case mark_slur_begin:
          case mark_dottedslur_begin:
          case mark_dashedslur_begin: {
            if (prt.ingrsl) {
              prt.isaphr = true;
              goto TOPHRASE1;
            }
          TOSLUR1:
            if (id != curslurtype) {
              switch (id) {
              case mark_slur_begin:
              case mark_graceslur_begin:
                ou << "\\slurSolid ";
                break;
              case mark_dottedslur_begin:
                ou << "\\slurDotted ";
                break;
              case mark_dashedslur_begin:
                ou << "\\slurDashed ";
                break;
#ifndef NDEBUG
              default:
                assert(false);
#endif
              }
              curslurtype = id;
            }
            break;
          }
          case mark_phrase_begin:
          case mark_dottedphrase_begin:
          case mark_dashedphrase_begin: {
          TOPHRASE1:
            if (id != curphrasetype) {
              switch (id) {
              case mark_slur_begin:
              case mark_phrase_begin:
                ou << "\\phrasingSlurSolid ";
                break;
              case mark_dottedslur_begin:
              case mark_dottedphrase_begin:
                ou << "\\phrasingSlurDotted ";
                break;
              case mark_dashedslur_begin:
              case mark_dashedphrase_begin:
                ou << "\\phrasingSlurDashed ";
                break;
#ifndef NDEBUG
              default:
                assert(false);
#endif
              }
              curphrasetype = id;
            }
            break;
          }
          case mark_stafftext_begin: {
            std::string ms0;
            const char* ms = module_markstring(*m);
            if (ms)
              ms0 = quotify(ms);
            if (!macropremeastextspan.empty()) {
              ou << '\\' << macropremeastextspan << " \"" << ms0 << "\" ";
              mpremeastextspan = true;
            } else {
              ou << "\\once \\override TextSpanner #'(bound-details left text) "
                    "= "
                    "\\markup \\bold \""
                 << ms0 << "\" ";
            }
            goto TEXTSPAN;
          }
          case mark_italictextabove_begin:
          case mark_italictextbelow_begin: {
            std::string ms0;
            const char* ms = module_markstring(*m);
            if (ms)
              ms0 = quotify(ms);
            if (!macropreitaltextspan.empty()) {
              ou << '\\' << macropreitaltextspan << " \"" << ms0 << "\" ";
              mpreitaltextspan = true;
            } else {
              ou << "\\once \\override TextSpanner #'(bound-details left text) "
                    "= "
                    "\\markup \\italic \""
                 << ms0 << "\" ";
            }
          } // case mark_graceslur_end: ingrsl = false; break;
            {
            TEXTSPAN:
              enum module_markpos z = txspos;
              txspos = module_markpos(*m);
              if (txspos != z) {
                switch (txspos) {
                case markpos_notehead:
                  ou << "\\textSpannerNeutral ";
                  break;
                case markpos_above:
                  ou << "\\textSpannerUp ";
                  break;
                case markpos_below:
                  ou << "\\textSpannerDown ";
                  break;
                default:
                  DBG("failed with pos = " << module_markpos(*m) << std::endl);
                  assert(false);
                }
              }
              break;
            }
          case mark_trem:
            if (!isintrem) {
              module_value n(module_marknum(*m));
              assert(n.type == module_int);
              if (n.val.i < 0) { // double trem
                assert((module_adjdur(no, -1) * wrm * n.val.i).den == 1);
                isintrem = -(module_adjdur(no, -1) * wrm * n.val.i).num;
                ou << "\\repeat tremolo " << isintrem << " { ";
              }
            }
            break;
          case mark_longtrill2:
            if (module_setting_ival(no, pitchedtrillid)) {
              istrillwnote = true;
              if (!inalongtr)
                ou << "\\pitchedTrill ";
            }
            break;
          case mark_graceslash: {
            if (macroslash.empty())
              ou << LILY_GRACESLASH;
            else {
              ou << '\\' << macroslash;
              mslash = true;
            }
            ou << ' ';
            break;
          }
          case mark_ped_begin: {
            ps_types nt =
                pedstyles.find(module_setting_sval(no, pedstyleid))->second;
            if (nt != curpedtype) {
              curpedtype = nt;
              switch (nt) {
              case ps_text:
                if (!macropedstyletext.empty()) {
                  ou << '\\' << macropedstyletext;
                  mpedstyletext = true;
                } else {
                  ou << "\\set Staff.pedalSustainStyle = #'text ";
                }
                break;
              case ps_bracket:
                if (!macropedstylebracket.empty()) {
                  ou << '\\' << macropedstylebracket;
                  mpedstylebracket = true;
                } else {
                  ou << "\\set Staff.pedalSustainStyle = #'bracket ";
                }
                break;
              }
            }
            break;
          }
          default:;
          }
        }
        if (!autobeam && !isintrem) {
          int bl = modout_beamsleft(no), br = modout_beamsright(no);
          if (bl) {
            if (macrobeam.empty())
              ou << "\\set stemLeftBeamCount = #" << bl;
            else {
              ou << '\\' << macrobeam << 'L'
                 << boost::algorithm::to_lower_copy(std::string(toroman(bl)));
              if (bl > mbeams)
                mbeams = bl;
            }
            ou << ' ';
          }
          if (br) {
            if (macrobeam.empty())
              ou << "\\set stemRightBeamCount = #" << br;
            else {
              ou << '\\' << macrobeam << 'R'
                 << boost::algorithm::to_lower_copy(std::string(toroman(br)));
              if (br > mbeams)
                mbeams = br;
            }
            ou << ' ';
          }
        }
        int oc = module_octsign(no);
        if (oc != octch) {
          ou << "\\ottava #" << oc << ' ';
          octch = oc;
        }
        // const char* b = module_setting_sval(no, lilybeforenoteid);
        // if (b) ou << b;
        struct module_list b(module_setting_val(no, lilytextinsertid).val.l);
        bool tl = module_istiedleft(no);
        for (const module_value *i(b.vals), *ie(b.vals + b.n); i < ie; ++i) {
          assert(i->type == module_string);
          if (tl) {
            if (boost::algorithm::starts_with(i->val.s, "<~"))
              ou << (i->val.s + 2);
          } else {
            if (boost::algorithm::starts_with(i->val.s, "<"))
              ou << (i->val.s + 1);
          }
        }
      }
      if ((ischlow && !ischhigh) || forcech) {
        ou << '<';
        inchd = true; // suppress stuff that shouldn't be inside `<' and '>'
      }
      if (inchd) {
        // const char* b1 = module_setting_sval(no, lilybeforenotechordid);
        // if (b1) ou << b1;
        struct module_list b(module_setting_val(no, lilytextinsertid).val.l);
        for (const module_value *i(b.vals), *ie(b.vals + b.n); i < ie; ++i) {
          assert(i->type == module_string);
          if (boost::algorithm::starts_with(i->val.s, "<<"))
            ou << (i->val.s + 2);
        }
        for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n); m < me;
             ++m) { // marks in chord before the note
          switch (module_markbaseid(module_markid(*m))) {
          case mark_artharm_sounding:
            ou << "\\parenthesize ";
            break;
          }
        }
      }
      if (fmr) {
        if (modout_isinvisible(no))
          ou << "\\skip ";
        else
          ou << 'R';
      } else
        printnotenote(no, ou, ksref, !autoaccs.empty()); // begin printing note
      bool tieprinted;
      if (inchd) {
        printnotetie(no, ou);
        tieprinted = true;
      } else
        tieprinted = false;
      // bool parenth = false;
      if (inchd) { // marks that go in the chord
        for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n); m < me;
             ++m) {
          assert(module_markbaseid(module_markid(*m)) < mark_nmarks);
          switch (module_markbaseid(module_markid(*m))) {
          case mark_natharm_touched: // goto next
          case mark_artharm_touched:
            ou << "\\harmonic";
            break;
            // case mark_artharm_sounding:
            // case mark_natharm_sounding:
            // ou << "\\parenthesize"; parenth = true; break;
          }
        }
        // const char* b2 = module_setting_sval(no, lilyafternotechordid);
        // if (b2) ou << b2;
        struct module_list b(module_setting_val(no, lilytextinsertid).val.l);
        for (const module_value *i(b.vals), *ie(b.vals + b.n); i < ie; ++i) {
          assert(i->type == module_string);
          if (boost::algorithm::starts_with(i->val.s, ">>"))
            ou << (i->val.s + 2);
        }
      }
      if ((ischhigh && !ischlow) || forcech) {
        ou << '>';
        inchd = false;
      }
      if (!inchd) {
        if (fmr)
          printfmrdur(module_dur(*me), wrm, ou);
        else
          printnotedur(no, wrm, ou, isgr, isintrem);
        if (!tieprinted)
          printnotetie(no, ou);
        //#warning "beams should be spread across chord along with marks"
        int bl = modout_beamsleft(no), br = modout_beamsright(no);
        if (!isintrem) {
          if (bl && !br) {
            ou << ']'; /*inbm = false*/
          } else if (br && !bl) {
            ou << '['; /*inmb = true;*/
          }
        }
        module_markslist mks(module_marks(no));
        bool crbeg = false, crend = false, dmbeg = false, dmend = false,
             endtrem = false;
        module_value gotalongtr1, gotalongtr2 = module_makeval(0, 1);
        gotalongtr1.type = module_special;
        char longtrwh;
        for (int ord = 0; ord < 3; ++ord) {
          for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
               m < me; ++m) {
            assert(module_markbaseid(module_markid(*m)) < mark_nmarks);
            assert(modout_markorder(*m) >= 0 && modout_markorder(*m) < 3);
            if (modout_markorder(*m) != ord)
              continue;
            switch (module_markbaseid(module_markid(*m))) {
            case mark_staccato:
              ou << where(*m) << '.';
              break;
            case mark_staccatissimo:
              ou << where(*m) << '|';
              break;
            case mark_accent:
              ou << where(*m) << '>';
              break;
            case mark_tenuto:
              ou << where(*m) << '-';
              break;
            case mark_marcato:
              ou << where(*m) << '^';
              break;
            case mark_mezzostaccato:
              ou << where(*m) << '_';
              break;
              // case mark_open: ou << where(*m) << "\\open"; break;
            case mark_stopped:
              ou << where(*m) << "\\stopped";
              break;
            case mark_pppppp: {
              ou << where(*m);
              if (!macropppppp.empty()) {
                ou << '\\' << macropppppp;
                mpppppp = true;
              } else
                ou << LILY_DYNMARKUP_CONST("pppppp");
              break;
            }
            case mark_ppppp:
              ou << where(*m) << "\\ppppp";
              break;
            case mark_pppp:
              ou << where(*m) << "\\pppp";
              break;
            case mark_ppp:
              ou << where(*m) << "\\ppp";
              break;
            case mark_pp:
              ou << where(*m) << "\\pp";
              break;
            case mark_p:
              ou << where(*m) << "\\p";
              break;
            case mark_mp:
              ou << where(*m) << "\\mp";
              break;
            case mark_ffff:
              ou << where(*m) << "\\ffff";
              break;
            case mark_fff:
              ou << where(*m) << "\\fff";
              break;
            case mark_ff:
              ou << where(*m) << "\\ff";
              break;
            case mark_f:
              ou << where(*m) << "\\f";
              break;
            case mark_mf:
              ou << where(*m) << "\\mf";
              break;
            case mark_sf:
              ou << where(*m) << "\\sf";
              break;
            case mark_sff:
              ou << where(*m) << "\\sff";
              break;
            case mark_sfff: {
              ou << where(*m);
              if (!macrosfff.empty()) {
                ou << '\\' << macrosfff;
                msfff = true;
              } else
                ou << LILY_DYNMARKUP_CONST("sfff");
              break;
            }
            case mark_sfz:
              ou << where(*m) << "\\sfz";
              break;
            case mark_sffz: {
              ou << where(*m);
              if (!macrosffz.empty()) {
                ou << '\\' << macrosffz;
                msffz = true;
              } else
                ou << LILY_DYNMARKUP_CONST("sffz");
              break;
            }
            case mark_sfffz: {
              ou << where(*m);
              if (!macrosfffz.empty()) {
                ou << '\\' << macrosfffz;
                msfffz = true;
              } else
                ou << LILY_DYNMARKUP_CONST("sfffz");
              break;
            }
            case mark_fz:
              ou << where(*m) << "\\fz";
              break;
            case mark_ffz: {
              ou << where(*m);
              if (!macroffz.empty()) {
                ou << '\\' << macroffz;
                mffz = true;
              } else
                ou << LILY_DYNMARKUP_CONST("ffz");
              break;
            }
            case mark_fffz: {
              ou << where(*m);
              if (!macrofffz.empty()) {
                ou << '\\' << macrofffz;
                mfffz = true;
              } else
                ou << LILY_DYNMARKUP_CONST("fffz");
              break;
            }
            case mark_rfz: {
              ou << where(*m);
              if (!macrorfz.empty()) {
                ou << '\\' << macrorfz;
                mrfz = true;
              } else
                ou << LILY_DYNMARKUP_CONST("rfz");
              break;
            }
            case mark_rf: {
              ou << where(*m);
              if (!macrorf.empty()) {
                ou << '\\' << macrorf;
                mrf = true;
              } else
                ou << LILY_DYNMARKUP_CONST("rf");
              break;
            }
            case mark_fp:
              ou << where(*m) << "\\fp";
              break;
            case mark_fzp: {
              ou << where(*m);
              if (!macrofzp.empty()) {
                ou << '\\' << macrofzp;
                mfzp = true;
              } else
                ou << LILY_DYNMARKUP_CONST("fzp");
              break;
            }
            case mark_sfp: {
              ou << where(*m);
              if (!macrosfp.empty()) {
                ou << '\\' << macrosfp;
                msfp = true;
              } else
                ou << LILY_DYNMARKUP_CONST("sfp");
              break;
            }
            case mark_sfzp: {
              ou << where(*m);
              if (!macrosfzp.empty()) {
                ou << '\\' << macrosfzp;
                msfzp = true;
              } else
                ou << LILY_DYNMARKUP_CONST("sfzp");
              break;
            }
            case mark_cresc_begin:
              crbeg = true;
              break;
            case mark_cresc_end:
              crend = true;
              break;
            case mark_dim_begin:
              dmbeg = true;
              break;
            case mark_dim_end:
              dmend = true;
              break;
            case mark_graceslur_begin:
              ou << '(';
              break;
            case mark_graceslur_end:
              ou << ')';
              prt.ingrsl = false;
              break;
            case mark_slur_begin:
            case mark_dottedslur_begin:
            case mark_dashedslur_begin:
              ou << (prt.isaphr ? "\\(" : "(");
              break;
            case mark_slur_end:
            case mark_dottedslur_end:
            case mark_dashedslur_end:
              ou << (prt.isaphr ? "\\)" : ")");
              prt.isaphr = false;
              break;
            case mark_phrase_begin:
            case mark_dottedphrase_begin:
            case mark_dashedphrase_begin:
              ou << "\\(";
              break;
            case mark_phrase_end:
            case mark_dottedphrase_end:
            case mark_dashedphrase_end:
              ou << "\\)";
              break;
            case mark_text: {
              const char* ms = module_markstring(*m);
              if (!ms)
                ms = "";
              ou << where(*m) << LILY_TEXT(ms);
              break;
            }
            case mark_stafftext_begin:
            case mark_italictextabove_begin:
            case mark_italictextbelow_begin:
              ou << "\\startTextSpan";
              break; // need \\textSpannerUp or Down?
            case mark_stafftext_end:
            case mark_italictextabove_end:
            case mark_italictextbelow_end:
              ou << "\\stopTextSpan";
              break;
            case mark_stafftext: {
              const char* ms = module_markstring(*m);
              if (!ms)
                ms = "";
              ou << where(*m) << LILY_MEASTEXT(ms);
              break;
            }
            case mark_italictextabove:
            case mark_italictextbelow: {
              const char* ms = module_markstring(*m);
              if (!ms)
                ms = "";
              ou << where(*m) << LILY_ITALTEXT(ms);
              break;
            }
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
            case mark_ord:
              assert(module_markstring(*m));
              ou << where(*m) << LILY_TEXT(module_markstring(*m));
              break;
            case mark_fermata:
              ou << where(*m) << "\\fermata";
              break;
            case mark_fermata_short:
              ou << where(*m) << "\\shortfermata";
              break;
            case mark_fermata_long:
              ou << where(*m) << "\\longfermata";
              break;
            case mark_fermata_verylong:
              ou << where(*m) << "\\verylongfermata";
              break;
            case mark_arpeggio:
              ou << "\\arpeggio";
              break;
            case mark_gliss_before: // *** SKIP TO NEXT *** for now, same as
                                    // port
            case mark_gliss_after:
            case mark_port_before:
            case mark_port_after:
              ou << "\\glissando";
              break;
            case mark_breath_before:
            case mark_breath_after:
              ou << "\\breathe";
              break;
            case mark_break_before:
            case mark_break_after:
              isbreak = true;
              break;
            case mark_tempo: {
              struct modout_tempostr te(modout_tempostr(no, *m));
              ou << where(*m) << "\\markup \\bold { ";
              if (te.str1)
                ou << '"' << quotify(te.str1) << "\" ";
              if (te.beat != (fomus_int) 0) {
                ou << "\\note #\"";
                printnoteduraux(ou, te.beat);
                ou << "\" #UP ";
              }
              if (te.str2)
                ou << '"' << quotify(te.str2) << "\" ";
              ou << '}';
              // ou << te.str2;
              // ou << where(*m) << "\\markup \\bold { ";
              // const char* str = module_markstring(*m);
              // if (str) ou << '"' << quotify(str);
              // module_value n(module_marknum(*m));
              // if (n.type != module_none) {
              //   ou << " \" \\note #\"";
              //   #warning "what about compound meter?"
              //   printnoteduraux(ou, module_setting_rval(no, beatid));
              //   ou << "\" #UP \" = ";
              //   lilyout::operator<<(ou, n);
              //   ou << "\" ";
              // } else ou << "\" ";
              // ou << '}';
            } break;
            case mark_trem: {
              module_value n(module_marknum(*m));
              assert(n.type == module_int);
              if (n.val.i >= 0)
                ou << ':' << n.val.i;
              break;
            }
            case mark_trem2:
              endtrem = true;
              break;
            case mark_longtrill: { // 1st acc
              gotalongtr1 = module_marknum(*m);
              longtrwh = where(*m);
              break;
            }
            case mark_longtrill2: { // 2nd acc
              gotalongtr2 = module_marknum(*m);
              break;
            }
            case mark_open:
              ou << where(*m) << "\\open";
              break;
            case mark_harm:
              ou << where(*m) << "\\flageolet";
              break;
              // case mark_sul: ou << where(*m) <<
              // LILY_TEXT(module_markstring(*m)); break;
            case mark_ped_begin:
              ou << "\\sustainOn";
              break;
            case mark_ped_end:
              ou << "\\sustainOff";
              break;
#ifndef NDEBUG
            case mark_tie:;
#else
            default:;
#endif
            }
          }
        }
        bool crends = false;
        if (incresc) {
          if (crend) {
            if (!crbeg)
              ou << "\\!";
            incresc = false;
          }
          if (crbeg) {
            ou << "\\<";
            incresc = true;
          }
        } else {
          if (crbeg) {
            ou << "\\<";
            incresc = true;
          }
          if (crend) {
            if (crbeg) {
              crends = true;
            } else {
              ou << "\\!";
            }
            incresc = false;
          }
        }
        if (indim) {
          if (dmend) {
            if (!dmbeg)
              ou << "\\!";
            indim = false;
          }
          if (dmbeg) {
            ou << "\\>";
            indim = true;
          }
        } else {
          if (dmbeg) {
            ou << "\\>";
            indim = true;
          }
          if (dmend) {
            if (dmbeg) {
              crends = true;
            } else {
              ou << "\\!";
            }
            indim = false;
          }
        }
        if (!module_istiedleft(no) &&
            inalongtr) { // this should be as far down as possible
          ou << "\\stopTrillSpan";
          inalongtr = false;
        }
#warning "trills have changed, fix this!"
        if (gotalongtr1.type != module_special &&
            !inalongtr) { // start a long trill
          ou << "\\startTrillSpan";
          if (istrillwnote) { // show note in parens
            ou << ' ';
            ou << dnotearr[todiatonic((module_writtennote(no) + 1) % 12)];
            assert(gotalongtr1.type != module_none);
            printnoteaccaux(ou, GET_R(gotalongtr1), GET_R(gotalongtr2),
                            0);                         // ntr has the 2nd acc
          } else if (gotalongtr1.type != module_none) { // show accidental
            assert(gotalongtr2.type != module_none);
            fomus_rat ac(GET_R(gotalongtr1 + gotalongtr2));
            if (ac == (fomus_int) 0)
              ou << longtrwh << "\\markup { \\natural }";
            else if (ac == module_makerat(1, 2))
              ou << longtrwh << "\\markup { \\semisharp }";
            else if (ac == module_makerat(-1, 2))
              ou << longtrwh << "\\markup { \\semiflat }";
            else if (ac == (fomus_int) 1)
              ou << longtrwh << "\\markup { \\sharp }";
            else if (ac == (fomus_int) -1)
              ou << longtrwh << "\\markup { \\flat }";
            else if (ac == module_makerat(3, 2))
              ou << longtrwh << "\\markup { \\sesquisharp }";
            else if (ac == module_makerat(-3, 2))
              ou << longtrwh << "\\markup { \\sesquiflat }";
            else if (ac == (fomus_int) 2)
              ou << longtrwh << "\\markup { \\doublesharp }";
            else if (ac == (fomus_int) -2)
              ou << longtrwh << "\\markup { \\doubleflat }";
            else {
              CERR << "can't write accidental";
              throw errbase();
            }
          }
          inalongtr = true;
        }
        if (endtrem && ischhigh) {
          ou << " }";
          isintrem = 0;
        }
        if (crends) {
          ou << " \\grace { s"
             << (module_setting_rval(no, defgracedurid) * wrm).den << "\\! }";
        }
        for (int i = 0; /*i < maxtuplvl*/; ++i) { // TUPLETS END
          fomus_rat fr(module_tuplet(no, i));
          if (fr <= (fomus_int) 0)
            break;
          if (module_tupletend(no, i)) {
            if (ingrbr) {
              ou << " }";
              ingrbr = false;
            }
            ou << " }";
          }
        }
        // const char* a = module_setting_sval(no, lilyafternoteid);
        // if (a) ou << a;
        struct module_list b(module_setting_val(no, lilytextinsertid).val.l);
        bool tr = module_istiedleft(no);
        for (const module_value *i(b.vals), *ie(b.vals + b.n); i < ie; ++i) {
          assert(i->type == module_string);
          if (tr) {
            if (boost::algorithm::starts_with(i->val.s, ">~"))
              ou << (i->val.s + 2);
          } else {
            if (boost::algorithm::starts_with(i->val.s, ">"))
              ou << (i->val.s + 1);
            else if (!(boost::algorithm::starts_with(i->val.s, "<") ||
                       boost::algorithm::starts_with(i->val.s, "<~") ||
                       boost::algorithm::starts_with(i->val.s, "<<") ||
                       boost::algorithm::starts_with(i->val.s, ">>")))
              ou << i->val.s;
          }
        }
      }
      ou << ' ';
      ++ni;
    }
    assert(ou.str().empty());
  }

  void lilyoutdata::lyrprint(
      part& prt, std::vector<noteholder>::iterator& ni,
      const std::vector<noteholder>& nos,
      const std::vector<module_measobj>& meass) { // no is modified
    bool inchd = false;
    bool ingrbr = false; //, incresc = false, indim = false;
    std::vector<module_measobj>::const_iterator me(meass.begin());
    // assert(*me == 0);
    std::ostringstream ou;
    fomus_int mn = 0; // measure number
    int thisv = module_voice(ni->n);
    bool mat = false;
    while (true) {
      bool done = (ni == nos.end() || module_voice(ni->n) != thisv);
      module_measobj me0;
      if (!done)
        me0 = module_meas(ni->n);
      while (done || me0 != *me) {
        bool isfirst;
        if (*me) {
          fomus_rat ts(module_timesig(*me));
          if (!mat) {
            ou << "\\skip 1*" << ts.num << '/' << ts.den << ' ';
          } else
            mat = false;
          if (ingrbr) {
            ou << "} ";
            ingrbr = false;
          }
          ou << '|';
          if (x.ok)
            justify(ou.str());
          ou.str("");
          isfirst = false;
        } else
          isfirst = true;
        ++me;
        if (me == meass.end())
          break;
        fomus_rat ts(module_timesig(*me));
        x << el() << std::string(x.inc + 2, ' ') << "% measure " << ++mn
          << " -- " << ts.num << '/' << ts.den << el();
        DBG("measure = " << mn << std::endl);
      }
      if (done) {
        assert(!mat);
        break;
      }
      module_noteobj no = ni->n;
      fomus_rat wrm(module_writtenmult(*me));
      mat = true;
      bool fmr = module_isfullmeasrest(*me);
      if (!inchd && !fmr) {
        for (int i = 0; /*i < maxtuplvl*/; ++i) { // TUPLETS BEGIN
          fomus_rat fr(module_tuplet(no, i));
          if (fr <= (fomus_int) 0)
            break;
          if (module_tupletbegin(no, i)) {
            if (ingrbr) {
              ou << "} ";
              ingrbr = false;
            }
            ou << "\\times " << fr.den << '/' << fr.num << " { ";
          }
        }
      }
      bool isgr = module_isgrace(no);
      if (!inchd) {
        if (ingrbr && !isgr) {
          ou << "} ";
          ingrbr = false;
        } else if (isgr && !ingrbr) {
          ou << "\\grace { ";
          ingrbr = true;
        }
      }
      if (!inchd) {
        bool h;
        module_markslist mks(module_singlemarks(no));
        for (const module_markobj *m(mks.marks), *e(mks.marks + mks.n); m < e;
             ++m) {
          assert(module_markbaseid(module_markid(*m)) < mark_nmarks);
          if (module_markbaseid(module_markid(*m)) == mark_vocal_text) {
            std::string s;
            const char* s0 = module_markstring(*m);
            if (s0)
              s = quotify(s0);
            if (boost::algorithm::starts_with(s, "--")) {
              boost::algorithm::erase_head(s, 2);
              boost::algorithm::trim_left(s);
            }
            h = boost::algorithm::ends_with(s, "--");
            if (h) {
              boost::algorithm::erase_tail(s, 2);
              boost::algorithm::trim_right(s);
            }
            ou << '"' << s << '"';
            goto GOTLYR;
          }
        }
        ou << "\\skip ";
        h = false;
      GOTLYR:
        if (fmr)
          printfmrdur(module_dur(*me), wrm, ou);
        else
          printnotedur(no, wrm, ou, isgr);
        if (h)
          ou << " --";
        for (int i = 0; /*i < maxtuplvl*/; ++i) { // TUPLETS END
          fomus_rat fr(module_tuplet(no, i));
          if (fr <= (fomus_int) 0)
            break;
          if (module_tupletend(no, i)) {
            if (ingrbr) {
              ou << " }";
              ingrbr = false;
            }
            ou << " }";
          }
        }
      }
      ou << ' ';
      ++ni;
    }
    assert(ou.str().empty());
  }

  void
  lilyoutdata::partprint(part& prt, std::set<std::string>& names,
                         module_measobj& m,
                         module_noteobj& o) { // return the leftover measure
    if (!x.ok) {                              // only do this the first time
      std::ostringstream vn;
      assert(!prt.var.empty());
      vn << partprefix << (char) toupper(prt.var[0]) << prt.var.substr(1);
      while (names.find(vn.str()) != names.end())
        vn << 'x';
      prt.var = vn.str();
    }
    names.insert(prt.var);
    std::string pn(module_setting_sval(prt.p, partnameid));
    x << "%% part `" << (pn.empty() ? prt.var : pn) << '\'' << el();
    // struct module_intslist st(module_staves(prt.p));
    int nst = module_totalnstaves(prt.p);
    {
      x << prt.var;
      enclose zzz(x, " = {", "}");
      x << el();
      // ... header stuff ...
      module_value uh(module_setting_val(fom, userpartheadid));
      assert(uh.type == module_list);
      if (uh.val.l.n > 0) {
        // x << "%% part header material" << el();
        insertuserstuffaux(uh);
        x << el();
      }
      std::string wh(nst <= 1 ? "Staff" : "PianoStaff");
      { // NAME
        std::string na(module_setting_sval(prt.p, partnameid));
        if (!na.empty()) {
          x << "\\set " << wh << ".instrumentName = #\"" << na << '\"' << el();
          std::string ab(module_setting_sval(prt.p, partabbrid));
          if (!ab.empty()) {
            x << "\\set " << wh << ".shortInstrumentName = #\"" << ab << '\"'
              << el();
          }
        }
      }
      if (!autobeam)
        x << "\\autoBeamOff" << el();
      //--------------------------------------------------------------------------------
      std::vector<module_measobj> meass(1, (module_measobj) 0);
      while (m && module_part(m) == prt.p) {
        meass.push_back(m);
        m = (x.ok ? module_nextmeas() : module_peeknextmeas(m));
      }
      std::vector<int> inicl(nst, -1), cclef(nst);
      std::vector<noteholder> nos;
      std::set<int> lyrs; // voices w/ lyrics
      while (o && module_part(o) == prt.p) {
        nos.push_back(o);
        if (module_isendchord(o)) {
          std::vector<int> stvs, clfs;
          fomus_rat cti(module_time(o));
          int v = module_voice(o); // the voice we're dealing with
          module_markslist mks(module_marks(o));
          for (const module_markobj *m(mks.marks), *me(mks.marks + mks.n);
               m < me; ++m) {
            assert(module_markbaseid(module_markid(*m)) < mark_nmarks);
            if (module_markbaseid(module_markid(*m)) == mark_vocal_text) {
              lyrs.insert(v);
              break;
            }
          }
          for (std::vector<noteholder>::reverse_iterator i(nos.rbegin());
               i != nos.rend() && module_time(i->n) >= cti;
               ++i) { // iterate through notes in current chord
            if (module_voice(i->n) == v) { // only care about notes in voice
              stvs.push_back(i->st);
              clfs.push_back(i->cl);
            }
          }
          assert(!stvs.empty()); // find the average staff (lilypond doesn't
                                 // spread chords across staves)
          assert(!clfs.empty());
          std::sort(stvs.begin(), stvs.end());
          std::sort(clfs.begin(), clfs.end());
          int sst = stvs[(stvs.size() - 1) /
                         2]; // round down (higher staff breaks tie)
          int clt = clfs[clfs.size() / 2]; // round up (higher clef breaks tie)
          bool chcl = false;
          assert(sst - 1 >= 0 && sst - 1 < nst);
          if (inicl[sst - 1] < 0) { // initial clef
            inicl[sst - 1] = cclef[sst - 1] = clt;
          } else if (cclef[sst - 1] != clt) { // current clef
            cclef[sst - 1] = clt;
            chcl = true;
          }
          for (std::vector<noteholder>::reverse_iterator i(nos.rbegin());
               i != nos.rend() && module_time(i->n) >= cti;
               ++i) { // iterate through notes in current chord
            if (module_voice(i->n) == v) { // only care about notes in voice
              i->st = sst;
              i->cl = clt;
              i->chcl = chcl;
            }
          }
        }
        if (x.ok) {
          module_skipassign(o);
          o = module_nextnote();
        } else {
          o = module_peeknextnote(o);
        }
      }
      for (int s = 1; s <= nst; ++s) {
        if (nst > 1) {
          if (macrostaff.empty())
            x << "\\change Staff = ";
          else {
            x << '\\' << macrostaff;
            if (s > mstaves)
              mstaves = s;
          }
          x << toroman(s) << ' ';
        }
        if (inicl[s - 1] < 0)
          inicl[s - 1] = module_strtoclef(module_staffclef(prt.p, s));
        x << "\\clef " << tolilyclef(inicl[s - 1]) << ' ';
      }
      x << el();
      std::sort(nos.begin(), nos.end(), order());
      bool firstvoi = true;
      enclose yyy(x, "<<", ">>");
      for (std::vector<noteholder>::iterator ni(nos.begin());
           ni != nos.end();) {
        if (firstvoi) {
          x << el();
          firstvoi = false;
        }
        int v = module_voice(ni->n);
        enclose yyy(
            x,
            "\\new Voice = " +
                boost::algorithm::to_lower_copy(std::string(toroman(v))) + " {",
            "}");
        measprint(prt, ni, nos, meass);
      }
      for (std::vector<noteholder>::iterator ni(nos.begin());
           ni != nos.end();) {
        int v = module_voice(ni->n);
        if (lyrs.find(v) != lyrs.end()) {
          x << el();
          enclose yyy(x, "\\new Lyrics \\lyricmode {", "}");
          x << el() << "\\set associatedVoice = #\""
            << boost::algorithm::to_lower_copy(std::string(toroman(v))) << '\"'
            << el();
          lyrprint(prt, ni, nos, meass);
        } else {
          do
            ++ni;
          while (ni != nos.end() && module_voice(ni->n) == v);
        }
      }
    }
    x << el();
  }

  void lilyoutdata::modout_write(
      FOMUS fom, const char* filename) { // filename should be complete
    try {
      autobeam = module_setting_ival(fom, autobeamid);
      autoaccs = module_setting_sval(fom, autoaccsid);
      boost::filesystem::path fn(filename);
      try {
        x.f.exceptions(boost::filesystem::ofstream::eofbit |
                       boost::filesystem::ofstream::failbit |
                       boost::filesystem::ofstream::badbit);
        x.f.open(fn.FS_FILE_STRING(), boost::filesystem::ofstream::out |
                                          boost::filesystem::ofstream::trunc);
        x << "%% " << std::string(width - 3, '-') << el() << "%% LilyPond "
          << LILYPOND_VERSION_STRING << " Score File" << el()
          << "%% Generated by " << PACKAGE_STRING << el();
        { // --------------------HEADER
          time_t tim;
          if (time(&tim) != -1) {
#ifdef HAVE_CTIME_R
            char buf[27]; // user-supplied buffer of length at least 26
            x << "%% " << ctime_r(&tim, buf);
#else
            x << "%% " << ctime(&tim);
#endif
          }
          x << "%% " << std::string(width - 3, '-') << el() << el()
            << "\\version \"" << LILYPOND_VERSION_STRING << '"' << el();
        }
        const char* psz = module_setting_sval(fom, lilypapersizeid);
        bool pls = module_setting_ival(fom, lilypaperorientid);
        if (psz[0] || pls) {
          x << "#(set-default-paper-size";
          if (psz[0])
            x << " \"" << psz << '"';
          else if (pls)
            x << " (ly:get-option 'paper-size)";
          if (pls)
            x << " 'landscape";
          x << ')' << el();
        }
        int ssz = module_setting_ival(fom, lilystaffsizeid);
        if (ssz > 0) {
          x << "#(set-global-staff-size " << ssz << ')' << el();
        }
        insertuserstuff(usertopheadid);
        x << el();
        boost::ptr_vector<part> parts;
        { // prep
          std::map<std::string, std::pair<part*, int>>
              partnames; // get unique part names
          while (true) {
            module_partobj p = module_nextpart();
            if (!p)
              break;
            std::string na(macrofy(module_id(p)));
            if (na.empty())
              na = "x";
            std::map<std::string, std::pair<part*, int>>::iterator i(
                partnames.find(na));
            if (i == partnames.end()) {
              part* a;
              parts.push_back(a = new part(p, na));
              partnames.insert(
                  std::map<std::string, std::pair<part*, int>>::value_type(
                      na, std::pair<part*, int>(a, 1)));
            } else {
              int n = ++i->second.second;
              if (n <= 2)
                i->second.first->var += 'I';
              std::ostringstream ss;
              ss << na << toroman(n);
              parts.push_back(new part(p, ss.str()));
            }
          }
        }
        { // --------------------FAKE PASS, COLLECT MACROS
          x.ok = false;
          std::set<std::string> names;
          module_measobj m = module_peeknextmeas(0);
          module_noteobj o = module_peeknextnote(0);
          std::for_each(parts.begin(), parts.end(),
                        boost::lambda::bind(
                            &lilyoutdata::partprint, this, boost::lambda::_1,
                            boost::lambda::var(names), boost::lambda::var(m),
                            boost::lambda::var(o)));
          x.ok = true;
        }
        { // MACROS
          x << "%% macros" << el();
          for (int i = 1; i <= mstaves; ++i) {
            const char* r = toroman(i);
            x << macrostaff << r << " = \\change Staff = " << r
              << "  % change to staff " << i << el();
          }
          for (int i = 1; i <= mbeams; ++i) {
            x << macrobeam << 'L'
              << boost::algorithm::to_lower_copy(std::string(toroman(i)))
              << " = \\set stemLeftBeamCount = #" << i
              << "  % set number of left beams to " << i << el();
          }
          for (int i = 1; i <= mbeams; ++i) {
            x << macrobeam << 'R'
              << boost::algorithm::to_lower_copy(std::string(toroman(i)))
              << " = \\set stemRightBeamCount = #" << i
              << "  % set number of right beams to " << i << el();
          }
          if (mpppppp)
            x << macropppppp << " = " << LILY_DYNMARKUP_CONST("pppppp")
              << "  % `pppppp' dynamic marking" << el();
          if (msfff)
            x << macrosfff << " = " << LILY_DYNMARKUP_CONST("sfff")
              << "  % `sfff' dynamic marking" << el();
          if (msffz)
            x << macrosffz << " = " << LILY_DYNMARKUP_CONST("sffz")
              << "  % `sffz' dynamic marking" << el();
          if (msfffz)
            x << macrosfffz << " = " << LILY_DYNMARKUP_CONST("sfffz")
              << "  % `sfffz' dynamic marking" << el();
          if (mffz)
            x << macroffz << " = " << LILY_DYNMARKUP_CONST("ffz")
              << "  % `ffz' dynamic marking" << el();
          if (mfffz)
            x << macrofffz << " = " << LILY_DYNMARKUP_CONST("fffz")
              << "  % `fffz' dynamic marking" << el();
          if (mrfz)
            x << macrorfz << " = " << LILY_DYNMARKUP_CONST("rfz")
              << "  % `rfz' dynamic marking" << el();
          if (mrf)
            x << macrorf << " = " << LILY_DYNMARKUP_CONST("rf")
              << "  % `rf' dynamic marking" << el();
          if (mfzp)
            x << macrofzp << " = " << LILY_DYNMARKUP_CONST("fzp")
              << "  % `fzp' dynamic marking" << el();
          if (msfp)
            x << macrosfp << " = " << LILY_DYNMARKUP_CONST("sfp")
              << "  % `sfp' dynamic marking" << el();
          if (msfzp)
            x << macrosfzp << " = " << LILY_DYNMARKUP_CONST("sfzp")
              << "  % `sfzp' dynamic marking" << el();
          if (mslash)
            x << macroslash << " = " << LILY_GRACESLASH
              << "  % grace note slash" << el();
          if (mpretextspan)
            x << macropretextspan
              << " = #(define-music-function (par loc txt) (string?)  % set "
                 "text "
                 "spanner text\n  #{\\once \\override TextSpanner "
                 "#'(bound-details left text) = $txt #})"
              << el();
          if (mpremeastextspan)
            x << macropremeastextspan
              << " = #(define-music-function (par loc txt) (string?)  % set "
                 "text "
                 "spanner text\n  #{\\once \\override TextSpanner "
                 "#'(bound-details left text) = \\markup \\bold $txt #})"
              << el();
          if (mpreitaltextspan)
            x << macropreitaltextspan
              << " = #(define-music-function (par loc txt) (string?)  % set "
                 "text "
                 "spanner text\n  #{\\once \\override TextSpanner "
                 "#'(bound-details left text) = \\markup \\italic $txt #})"
              << el();
          if (mpedstyletext)
            x << macropedstyletext
              << " = \\set Staff.pedalSustainStyle = #'text  % text pedal style"
              << el();
          if (mpedstylebracket)
            x << macropedstylebracket
              << " = \\set Staff.pedalSustainStyle = #'bracket  % bracket "
                 "pedal "
                 "style"
              << el();
          module_value macs(module_setting_val(fom, extramacrosid));
          assert(macs.type == module_list);
          for (module_value *i = macs.val.l.vals,
                            *ie = macs.val.l.vals + macs.val.l.n;
               i < ie; i += 2) {
            assert(i->type == module_string);
            assert((i + 1)->type == module_string);
            x << i->val.s << " = " << (i + 1)->val.s << el();
          }
        }
        x << el();
        module_value uh(module_setting_val(fom, userheadid));
        assert(uh.type == module_list);
        if (uh.val.l.n > 0) {
          x << "%% header material" << el();
          insertuserstuffaux(uh);
          x << el();
        }
        { // --------------------THE NOTES
          std::set<std::string> names;
          module_measobj m = module_nextmeas();
          module_noteobj o = module_nextnote();
          std::for_each(parts.begin(), parts.end(),
                        boost::lambda::bind(
                            &lilyoutdata::partprint, this, boost::lambda::_1,
                            boost::lambda::var(names), boost::lambda::var(m),
                            boost::lambda::var(o)));
        }
        {
          x << "%% score" << el();
          // enclose zzz0(x, "\\book {", "}");
          // x << el();
          const char* title = module_setting_sval(fom, thetitleid);
          const char* author = module_setting_sval(fom, theauthorid);
          if (title[0] || author[0]) {
            enclose aaa(x, "\\header {", "}");
            x << el();
            if (title[0])
              x << "title = \"" << quotify(title) << '"' << el();
            if (author[0])
              x << "composer = \"" << quotify(author) << '"' << el();
          }
          // x << "%% score" << el();
          enclose zzz(x, "\\score {", "}");
          x << el();
          boost::ptr_vector<part>::const_iterator pa(parts.begin());
          enclose zzz2(x, "<<", ">>");
          x << el();
          x << "#(set-accidental-style '";
          if (autoaccs.empty())
            x << "forget";
          else
            x << autoaccs;
          x << " 'Score)" << el();
          dostaves(pa, parts.end(), 1, false);
        }
        x.f.close();
        int vs = module_setting_ival(fom, verboseid);
        try {
          const char* path = module_setting_sval(fom, lilyexepathid);
          if (strlen(path) <= 0)
            return;
          if (vs >= 1)
            fout << "running LilyPond..." << std::endl;
          execout::execout lout;
          struct module_list l(module_setting_val(fom, lilyexeargsid).val.l);
          DBG("n of extra args = " << l.n << std::endl);
          std::vector<std::string> a0;
          a0.push_back("-o" + FS_CHANGE_EXTENSION(fn, "").FS_FILE_STRING());
          bool verb = (vs >= 2);
          if (verb) {
            fout << "  executing `";
            execout::outputcmd(fout, path, a0, l, fn.FS_FILE_STRING().c_str());
            fout << "'..." << std::endl;
          }
          execout::execpid pid(
              execout::exec(&lout, path, a0, l, fn.FS_FILE_STRING().c_str()));
          std::string ln;
          std::string pr(FS_BASENAME(boost::filesystem::path(path)));
          while (lout.good()) {
            std::getline(lout, ln);
            if (verb && !ln.empty())
              fout << "  " << pr << ": " << ln << std::endl;
          }
          execout::waituntildone(pid);
          lout.close();
        } catch (const execout::execerr& e) {
          CERR << "error compiling `" << fn.FS_FILE_STRING() << '\''
               << std::endl;
          cerr = true;
          return;
        }
        std::string ext(module_setting_sval(fom, lilyviewextid));
        if (!ext.empty() && ext[0] != '.')
          ext = '.' + ext; // make sure it has a dot
        boost::filesystem::path opath(FS_CHANGE_EXTENSION(fn, ext));
        bool outok;
        try {
          outok = (boost::filesystem::last_write_time(opath) >=
                   boost::filesystem::last_write_time(fn));
        } catch (const boost::filesystem::filesystem_error& e) {
          outok = false;
        }
        if (outok) {
          try {
            const char* path = module_setting_sval(fom, lilyviewexepathid);
            if (strlen(path) <= 0)
              return;
            if (vs >= 1)
              fout << "opening viewer..." << std::endl;
            struct module_list l(
                module_setting_val(fom, lilyviewexeargsid).val.l);
            execout::exec(0, path, std::vector<std::string>(), l,
                          opath.FS_FILE_STRING().c_str());
          } catch (const execout::execerr& e) {
            CERR << "error viewing `" << fn.FS_FILE_STRING() << '\''
                 << std::endl;
            cerr = true;
          }
        }
        return;
      } catch (const boost::filesystem::ofstream::failure& e) {
        CERR << "error writing `" << fn.FS_FILE_STRING() << '\'' << std::endl;
      }
    } catch (const boost::filesystem::filesystem_error& e) {
      CERR << "invalid path/filename `" << filename << '\'' << std::endl;
    } catch (const errbase& e) {}
    cerr = true;
  }

  void
  lilyoutdata::dostaves(boost::ptr_vector<part>::const_iterator& p1,
                        const boost::ptr_vector<part>::const_iterator& pend,
                        const int grlvl, const bool inps) {
    while (true) {
      switch (module_partgroupbegin(p1->p, grlvl)) {
      case parts_group: {
        enclose zzz(x, "\\new StaffGroup <<", ">>");
        x << el();
        dostaves(p1, pend, grlvl + 1, false /*, grlvl0 + 1*/); // go up a level
        break;
      }
      case parts_grandstaff: {
        enclose zzz(x, "\\new PianoStaff <<", ">>");
        x << el();
        dostaves(p1, pend, grlvl + 1, true /*, grlvl0 + 1*/);
        break;
      }
      case parts_choirgroup: {
        enclose zzz(x, "\\new ChoirStaff <<", ">>");
        x << el();
        dostaves(p1, pend, grlvl + 1, false /*, grlvl0 + 1*/);
        break;
      }
      case parts_nogroup: {
        int st = module_totalnstaves(p1->p);
        if (st > 1) {
          if (inps) {
            for (int i = 1; i <= st; ++i) {
              x << "\\context Staff = " << toroman(i) << el();
            }
            x << "\\new Voice \\" << p1->var << el();
          } else {
            enclose zzz(x, "\\new PianoStaff <<", ">>");
            x << el();
            dostaves(p1, pend, grlvl, true /*, grlvl0 + 1*/);
          }
        } else {
          x << "\\new Staff \\" << p1->var << el();
        }
      }
      }
      if (inps || p1 == pend || module_partgroupend(p1->p, grlvl - 1))
        return;
      if (++p1 == pend)
        return;
    }
  }

  const char* indenttype = "integer>=0";
  int valid_indenttype(const struct module_value val) {
    return module_valid_int(val, 0, module_incl, 0, module_nobound, 0,
                            indenttype);
  }
  const char* writewidthtype = "integer>=79";
  int valid_writewidth(const struct module_value val) {
    return module_valid_int(val, 79, module_incl, -1, module_nobound, 0,
                            writewidthtype);
  }
  const char* accslisttype =
      "(string_acc string_acc string_acc string_acc string_acc string_acc "
      "string_acc string_acc string_acc string_acc string_acc)";
  int valid_accslist(const struct module_value val) {
    return module_valid_listofstrings(val, 11, 11, -1, -1, 0, accslisttype);
  }
  const char* notenameslisttype = "(string_name string_name string_name "
                                  "string_name string_name string_name "
                                  "string_name)";
  int valid_notenameslist(const struct module_value val) {
    return module_valid_listofstrings(val, 7, 7, -1, -1, 0, notenameslisttype);
  }
  const char* insstr = "string | (string string ...)";
} // namespace lilyout

using namespace lilyout;
// ------------------------------------------------------------------------------------------------------------------------
// END OF NAMESPACE

const char* module_longname() {
  return "LilyPond File Output";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Writes `.ly' LilyPond score files and optionally compiles/displays "
         "the results.";
}

void* module_newdata(FOMUS f) {
  return new lilyoutdata(f);
}
void module_freedata(void* dat) {
  delete (lilyoutdata*) dat;
}
const char* module_err(void* dat) {
  return ((lilyoutdata*) dat)->module_err();
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
  //#warning "why using a map here??  use a vector"
  clefsmap.insert(clefsmaptype::value_type((int) clef_subbass_8down,
                                           "\"subbass_8\"" /*subbass-8down*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_bass_8down,
                                           "\"bass_8\"" /*bass-8down*/));
  clefsmap.insert(clefsmaptype::value_type(
      (int) clef_c_baritone_8down, "\"baritone_8\"" /*c-baritone-8down*/));
  clefsmap.insert(clefsmaptype::value_type(
      (int) clef_f_baritone_8down, "\"varbaritone_8\"" /*f-baritone-8down*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_tenor_8down,
                                           "\"tenor_8\"" /*tenor-8down*/));
  clefsmap.insert(
      clefsmaptype::value_type((int) clef_subbass, "subbass" /*subbass*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_alto_8down,
                                           "\"alto_8\"" /*alto-8down*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_bass, "bass" /*bass*/));
  clefsmap.insert(
      clefsmaptype::value_type((int) clef_mezzosoprano_8down,
                               "\"mezzosoprano_8\"" /*mezzosoprano-8down*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_c_baritone,
                                           "baritone" /*c-baritone*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_f_baritone,
                                           "varbaritone" /*f-baritone*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_soprano_8down,
                                           "\"soprano_8\"" /*soprano-8down*/));
  clefsmap.insert(
      clefsmaptype::value_type((int) clef_tenor, "tenor" /*tenor*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_subbass_8up,
                                           "\"subbass^8\"" /*subbass-8up*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_treble_8down,
                                           "\"treble_8\"" /*treble-8down*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_alto, "alto" /*alto*/));
  clefsmap.insert(
      clefsmaptype::value_type((int) clef_bass_8up, "\"bass^8\"" /*bass-8up*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_mezzosoprano,
                                           "mezzosoprano" /*mezzosoprano*/));
  clefsmap.insert(clefsmaptype::value_type(
      (int) clef_c_baritone_8up, "\"baritone^8\"" /*c-baritone-8up*/));
  clefsmap.insert(clefsmaptype::value_type(
      (int) clef_f_baritone_8up, "\"varbaritone^8\"" /*f-baritone-8up*/));
  clefsmap.insert(
      clefsmaptype::value_type((int) clef_soprano, "soprano" /*soprano*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_tenor_8up,
                                           "\"tenor^8\"" /*tenor-8up*/));
  clefsmap.insert(
      clefsmaptype::value_type((int) clef_treble, "treble" /*treble*/));
  clefsmap.insert(
      clefsmaptype::value_type((int) clef_alto_8up, "\"alto^8\"" /*alto-8up*/));
  clefsmap.insert(clefsmaptype::value_type(
      (int) clef_mezzosoprano_8up, "\"mezzosoprano^8\"" /*mezzosoprano-8up*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_soprano_8up,
                                           "\"soprano^8\"" /*soprano-8up*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_treble_8up,
                                           "\"treble^8\"" /*treble-8up*/));
  clefsmap.insert(clefsmaptype::value_type((int) clef_percussion,
                                           "percussion" /*treble-8up*/));
  // fermatas
  // fermatasmap.insert(fermatasmaptype::value_type("", fermata_norm));
  // fermatasmap.insert(fermatasmaptype::value_type("short", fermata_short));
  // fermatasmap.insert(fermatasmaptype::value_type("long", fermata_long));
  // fermatasmap.insert(fermatasmaptype::value_type("verylong", fermata_long2));
  // arpeggios
  // arpeggiosmap.insert(arpeggiosmaptype::value_type("", arpeggio_norm));
  // arpeggiosmap.insert(arpeggiosmaptype::value_type("up", arpeggio_up));
  // arpeggiosmap.insert(arpeggiosmaptype::value_type("down", arpeggio_down));
  // arpeggiosmap.insert(arpeggiosmaptype::value_type("bracket",
  // arpeggio_bracket));
  // arpeggiosmap.insert(arpeggiosmaptype::value_type("parenthesis",
  // arpeggio_paren)); slurs slursmap.insert(slursmaptype::value_type("",
  // slur_norm)); slursmap.insert(slursmaptype::value_type("dotted",
  // slur_dotted)); slursmap.insert(slursmaptype::value_type("dashed",
  // slur_dashed)); special accidental symbols
  specaccs.insert(std::map<int, const char*>::value_type(0, "NATURAL"));
  specaccs.insert(std::map<int, const char*>::value_type(1, "SHARP"));
  specaccs.insert(std::map<int, const char*>::value_type(2, "DOUBLE-SHARP"));
  specaccs.insert(std::map<int, const char*>::value_type(-1, "FLAT"));
  specaccs.insert(std::map<int, const char*>::value_type(-2, "DOUBLE-FLAT"));
  // pedal styles
  pedstyles.insert(pedmaptype::value_type("text", ps_text));
  pedstyles.insert(pedmaptype::value_type("bracket", ps_bracket));
}
void module_free() { /*assert(newcount == 0);*/
}

int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "lily-indent"; // docscat{lilyout}
    set->type = module_int;
    set->descdoc =
        "Number of spaces used for indentation in a LilyPond output file.";
    set->typedoc = indenttype;

    module_setval_int(&set->val, 2);

    set->loc = module_locscore;
    set->valid = valid_indenttype;
    set->uselevel = 2; // nitpicky
    indentid = id;
    break;
  }
  case 1: {
    set->name = "lily-part-topheader"; // docscat{lilyout}
    set->type = module_list_strings;
    set->descdoc =
        "List of strings to be inserted into the top of a part block in a "
        "LilyPond output file."
        "  Each string in the list represents a newline-separated line of text."
        "  Use this to insert your own custom LilyPond code into LilyPond "
        "parts.";
    // set->typedoc = writesettingstype;

    module_setval_list(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_writesets;
    set->uselevel = 3;
    userpartheadid = id;
    break;
  }
  case 2: {
    set->name = "lily-file-topheader"; // docscat{lilyout}
    set->type = module_list_strings;
    set->descdoc =
        "List of strings to be inserted into the top of a LilyPond output file "
        "before anything else."
        //"  This is in a similar position in the file as `lily-file-header'."
        "  Each string in the list represents a newline-separated line of text."
        "  Use this to insert your own custom LilyPond code into the output "
        "file.";
    // set->typedoc = writesettingstype;

    module_setval_list(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_writesets;
    set->uselevel = 3;
    usertopheadid = id;
    break;
  }
  case 3: {
    set->name = "lily-exe-path"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "Path to the LilyPond command line executable."
        "  If this is specified, FOMUS automatically executes LilyPond to "
        "process the output `.ly' file into a `.pdf' file."
        "  If LilyPond is in your path then only the executable filename is "
        "necessary."
        "  Set this to an empty string to prevent FOMUS from invoking LilyPond "
        "automatically.";
    // set->typedoc = writesettingstype;

    module_setval_string(&set->val, CMD_MACRO(LILYPOND_PATH));

    set->loc = module_locscore;
    // set->valid = valid_writesets;
    set->uselevel = 2;
    lilyexepathid = id;
    break;
  }
  case 4: {
    set->name = "lily-extra-macros"; // docscat{lilyout}
    set->type = module_symmap_strings;
    set->descdoc = "Mapping from macro names to definitions.  These are "
                   "inserted as macro definitions in a special section at the "
                   "top of a LilyPond output file.  "
                   "They are then there for your convenience if you go on to "
                   "edit the LilyPond file itself."
        //"The macros replace any of the convenience macros that FOMUS
        // automatically inserts."
        ;
    // set->typedoc = writesettingstype;

    module_setval_list(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_writesets;
    set->uselevel = 3;
    extramacrosid = id;
    break;
  }
  case 5: {
    set->name = "lily-file-header"; // docscat{lilyout}
    set->type = module_list_strings;
    set->descdoc =
        "List of strings to be inserted into the top of a LilyPond output "
        "file, after the macro section."
        //"  This is nearly the same as `lily-file-topheader'--it's just in a
        // slightly different location."
        "  Each string in the list represents a newline-separated line of text."
        "  Use this to insert your own custom LilyPond code into the output "
        "file.";
    // set->typedoc = writesettingstype;

    module_setval_list(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_writesets;
    set->uselevel = 3;
    userheadid = id;
    break;
  }
  case 6: {
    set->name = "lily-file-width"; // docscat{lilyout}
    set->type = module_int;
    set->descdoc =
        "The number of characters allowed per line in a LilyPond output file."
        "  Used to wrap long lines into a more readable format.";
    set->typedoc = writewidthtype;

    module_setval_int(&set->val, 119);

    set->loc = module_locscore;
    set->valid = valid_writewidth;
    set->uselevel = 2;
    writewidthid = id;
    break;
  }
  case 7: {
    set->name = "lily-accidentals"; // docscat{lilyout}
    set->type = module_list_strings;
    set->descdoc =
        "A list of accidental symbols to be used.  It should contain eleven "
        "strings corresponding to quartertones ranging from a wholetone flat "
        "to a wholetone sharp.  "
        "The expected semitone accidental adjustments are: -2, -1-1/2, -1, "
        "-1+1/2, -1/2, 0, +1/2, +1-1/2, +1, +1+1/2 and 2.  "
        "By using this together with `lily-file-topheader' and "
        "`lily-notenames' it is possible to import language settings (e..g, "
        "with a statement like `\\include \"english.ly\"')"
        " and change how accidentals are displayed and interpreted.";
    set->typedoc = accslisttype;

    module_setval_list(&set->val, 11);
    module_setval_string(set->val.val.l.vals + 0, "eses");
    module_setval_string(set->val.val.l.vals + 1, "eseh");
    module_setval_string(set->val.val.l.vals + 2, "es");
    module_setval_string(set->val.val.l.vals + 3, "eh");
    module_setval_string(set->val.val.l.vals + 4, "eh");
    module_setval_string(set->val.val.l.vals + 5, "");
    module_setval_string(set->val.val.l.vals + 6, "ih");
    module_setval_string(set->val.val.l.vals + 7, "ih");
    module_setval_string(set->val.val.l.vals + 8, "is");
    module_setval_string(set->val.val.l.vals + 9, "isih");
    module_setval_string(set->val.val.l.vals + 10, "isis");

    set->loc = module_locscore;
    set->valid = valid_accslist;
    set->uselevel = 3;
    accslistid = id;
    break;
  }
  case 8: {
    set->name = "lily-autobeams"; // docscat{lilyout}
    set->type = module_bool;
    set->descdoc =
        "Determines whether or not to let LilyPond handle all of the beaming.  "
        "Setting this to `no' lets FOMUS write all beam information explicitly "
        "into the output file, properly reflecting any measure division "
        "choices that were made.";
    // set->typedoc = ;

    module_setval_int(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_writewidth;
    set->uselevel = 3;
    autobeamid = id;
    break;
  }
  case 9: {
    set->name = "lily-autoaccs"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "Determines whether or not to let LilyPond handle note accidentals.  "
        "Setting this to an empty string lets FOMUS write all accidentals "
        "explicitly into the output file.  "
        "Setting this to anything else tells FOMUS not to handle note "
        "accidentals and indicates which style LilyPond should use "
        "(see \"Automatic Accidentals\" in the LilyPond manual).";
    // set->typedoc = ;

    module_setval_string(&set->val, "");

    set->loc = module_locscore;
    // set->valid = valid_writewidth;
    set->uselevel = 3;
    autoaccsid = id;
    break;
  }
  case 10: {
    set->name = "lily-view-exe-path"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc = "Path to executable of viewer application to launch for "
                   "viewing LilyPond output `.pdf' files."
                   "  If this is specified, FOMUS automatically displays the "
                   "output `.pdf' file."
                   "  If the viewer executable is in your path then only the "
                   "filename is necessary."
                   "  Set this to an empty string to prevent FOMUS from "
                   "invoking the viewer automatically.";
    // set->typedoc = writesettingstype;

    module_setval_string(&set->val, CMD_MACRO(LILYPOND_VIEWPATH));

    set->loc = module_locscore;
    // set->valid = valid_writesets;
    set->uselevel = 2;
    lilyviewexepathid = id;
    break;
  }
  case 11: {
    set->name = "lily-view-extension"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc = "Filename extension expected by the LilyPond output viewer "
                   "application set in `lily-view-exe-path'."
                   "  Change this if you use `lily-exe-args' to override the "
                   "type of file that LilyPond outputs.";
    // set->typedoc = writesettingstype;

    module_setval_string(&set->val, "pdf");

    set->loc = module_locscore;
    // set->valid = valid_writesets;
    set->uselevel = 3;
    lilyviewextid = id;
    break;
  }
  case 12: {
    set->name = "lily-exe-args"; // docscat{lilyout}
    set->type = module_list_strings;
    set->descdoc =
        "A list of extra arguments to be passed to the LilyPond executable set "
        "in `lily-exe-path'."
        "  Each string is a separate command line argument."
        "  In addition, FOMUS adds the input filename and an `-o' argument "
        "specifying the output file path."
        "  If you pass options to change the type of file that LilyPond "
        "outputs, make sure you also change `lily-view-extension' accordingly.";
    // set->typedoc = accslisttype;

    module_setval_list(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilyexeargsid = id;
    break;
  }
  case 13: {
    set->name = "lily-view-exe-args"; // docscat{lilyout}
    set->type = module_list_strings;
    set->descdoc = "A list of additional arguments to be passed to the "
                   "LilyPond output viewer application set in "
                   "`lily-view-exe-path' (in addition to the input filename)."
                   "  Each string is a shell command line argument.";
    // set->typedoc = accslisttype;

    module_setval_list(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilyviewexeargsid = id;
    break;
  }
  case 14: {
    set->name = "lily-macro-staff"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "Whether or not to replace `\\change Staff = ...' with a more concise "
        "macro in a LilyPond output file.  "
        "If set to an empty string, no macro is used.  If a string is given, "
        "sets the base name for series of macros (e.g., \"staff\" "
        "creates macros with the names `\\staffI', `\\staffII', etc..).";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "staff");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrostaffid = id;
    break;
  }
  case 15: {
    set->name = "lily-macro-graceslash"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc = "The name of the macro that contains an override command "
                   "that changes the \"stroke style\" of a grace note."
                   "  If set to an empty string, no macro is used and the "
                   "command is written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "slash");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacroslashid = id;
    break;
  }
  case 16: {
    set->name = "lily-macro-pppppp"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `pppppp' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "pppppp");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacroppppppid = id;
    break;
  }
  case 17: {
    set->name = "lily-macro-sfff"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `sfff' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "sfff");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrosfffid = id;
    break;
  }
  case 18: {
    set->name = "lily-macro-sffz"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `sffz' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "sffz");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrosffzid = id;
    break;
  }
  case 19: {
    set->name = "lily-macro-sfffz"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `sfffz' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "sfffz");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrosfffzid = id;
    break;
  }
  case 20: {
    set->name = "lily-macro-ffz"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `ffz' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "ffz");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacroffzid = id;
    break;
  }
  case 21: {
    set->name = "lily-macro-fffz"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `fffz' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "fffz");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrofffzid = id;
    break;
  }
  case 22: {
    set->name = "lily-macro-rfz"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `rfz' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "rfz");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrorfzid = id;
    break;
  }
  case 23: {
    set->name = "lily-macro-rf"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `rf' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "rf");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrorfid = id;
    break;
  }
  case 24: {
    set->name = "lily-part-prefix"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc = "A string to be used as a base or prefix for each part name "
                   "as it appears in the output file.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "part");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilypartprefixid = id;
    break;
  }
  case 25: {
    set->name = "lily-macro-beam"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "Whether or not to replace `\\set stemLeftBeamCount = ...' with a more "
        "concise macro in a LilyPond output file.  "
        "If set to an empty string, no macro is used.  If a string is given, "
        "sets the base name of a series of macros (e.g., \"beam\" "
        "creates macros with the names `\\beamLi', `\\beamRi', `\\beamRii', "
        "etc..).";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "b");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrobeamid = id;
    break;
  }
  case 26: {
    set->name = "lily-notenames"; // docscat{lilyout}
    set->type = module_list_strings;
    set->descdoc =
        "A list of note name symbols to be used in a LilyPond output file.  It "
        "should contain seven strings corresponding to the notes `C' through "
        "`B'.  "
        "By using this together with `lily-file-topheader' and "
        "`lily-accidentals' it is possible to import language settings (e.g., "
        "with a statement like `\\include \"english.ly\"')"
        " and change how notes appear in the file.";
    set->typedoc = notenameslisttype;

    module_setval_list(&set->val, 7);
    module_setval_string(set->val.val.l.vals + 0, "c");
    module_setval_string(set->val.val.l.vals + 1, "d");
    module_setval_string(set->val.val.l.vals + 2, "e");
    module_setval_string(set->val.val.l.vals + 3, "f");
    module_setval_string(set->val.val.l.vals + 4, "g");
    module_setval_string(set->val.val.l.vals + 5, "a");
    module_setval_string(set->val.val.l.vals + 6, "b");

    set->loc = module_locscore;
    set->valid = valid_notenameslist;
    set->uselevel = 2;
    notenameslistid = id;
    break;
  }
  case 27: {
    set->name = "lily-macro-textspan"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc = "Whether or not to replace `\\once \\override TextSpanner "
                   "#'(bound-details left text) = ...' with a more concise "
                   "macro in a LilyPond output file.  "
                   "This is for texts that appear attached to a note or chord "
                   "(i.e., created with the `x' mark).  "
                   "If set to an empty string, no macro is used and the "
                   "command is written out every time.  "
                   "Otherwise, sets the name of the macro.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "textspan");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacropretextspanid = id;
    break;
  }
  case 28: {
    set->name = "lily-macro-stafftextspan"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc = "Whether or not to replace `\\once \\override TextSpanner "
                   "#'(bound-details left text) = ...' with a more concise "
                   "macro in a LilyPond output file.  "
                   "This is for texts that usually appear above the staff "
                   "(i.e., created with the `x!' mark).  "
                   "If set to an empty string, no macro is used and the "
                   "command is written out every time.  "
                   "Otherwise, sets the name of the macro.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "stafftextspan");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacropremeastextspanid = id;
    break;
  }
  case 29: {
    set->name = "lily-insert"; // docscat{lilyout}
    set->type = module_list_strings;
    set->descdoc =
        "Extra text to insert immediately before or after a note in a LilyPond "
        "output file.  "
        "Strings are appended in the order that they appear in the list.  "
        "Optional prefix characters determine where the text is placed "
        "relative to the note text: `<' specifies before when the note is "
        "untied to the left, "
        "`>' specifies after when the note is untied to the right, `<~' "
        "specifies before when the note is tied to the left, `>~' specifies "
        "after when the note is "
        "tied to the right, `<<' specifies before when the note is inside a "
        "chord, and `>>' specifies after when the note is inside a chord.  "
        "`>' is assumed if no prefix is given.  "
        "If the inserted text must be separate from the note text, you must "
        "append a space to the beginning or end of the string.";
    // set->typedoc = writesettingstype;

    module_setval_list(&set->val, 0);

    set->loc = module_locnote;
    // set->valid = valid_writesets;
    set->uselevel = 3;
    lilytextinsertid = id;
    break;
  }
  case 30: {
    set->name = "lily-macro-italictextspan"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc = "Whether or not to replace `\\once \\override TextSpanner "
                   "#'(bound-details left text) = ...' with a more concise "
                   "macro in a LilyPond output file.  "
                   "This is for texts that usually appear in italics (i.e., "
                   "created with the `x/' mark).  "
                   "If set to an empty string, no macro is used and the "
                   "command is written out every time.  "
                   "Otherwise, sets the name of the macro.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "italtextspan");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacropreitaltextspanid = id;
    break;
  }
  case 31: {
    set->name = "lily-macro-pedaltext"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "Whether or not to replace `\\set Staff.pedalSustainStyle = #'text' "
        "with a more concise macro in a LilyPond output file.  "
        "If set to an empty string, no macro is used and the command is "
        "written out every time.  "
        "Otherwise, sets the name of the macro.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "pedtext");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacropedstyletextid = id;
    break;
  }
  case 32: {
    set->name = "lily-macro-pedalbracket"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "Whether or not to replace `\\set Staff.pedalSustainStyle = #'bracket' "
        "with a more concise macro in a LilyPond output file.  "
        "If set to an empty string, no macro is used and the command is "
        "written out every time.  "
        "Otherwise, sets the name of the macro.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "pedbracket");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacropedstylebracketid = id;
    break;
  }
  case 33: {
    set->name = "lily-papersize"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "Specifies a string argument for LilyPond's `set-default-paper-size' "
        "function.  "
        "Set this to a value like \"a4\", \"letter\", \"legal\" or \"11x17\" "
        "to specify a different paper size in a LilyPond output file.  "
        "Set it to an empty string to not specify a paper size and use "
        "LilyPond's default setting (in LilyPond version 2.12 the default is "
        "\"a4\").";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilypapersizeid = id;
    break;
  }
  case 34: {
    set->name = "lily-landscape"; // docscat{lilyout}
    set->type = module_bool;
    set->descdoc = "Specifies whether the paper orientation should be "
                   "\"landscape\" rather than \"portrait.\"  "
                   "Set this to `yes' to specify landscape paper orientation "
                   "in a LilyPond output file using LilyPond's "
                   "`set-default-paper-size' function.";
    // set->typedoc = ;

    module_setval_int(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_writewidth;
    set->uselevel = 3;
    lilypaperorientid = id;
    break;
  }
  case 35: {
    set->name = "lily-staffsize"; // docscat{lilyout}
    set->type = module_int;
    set->descdoc =
        "Specifies a default staff size.  "
        "Set this to change the font size for staves and all notation elements "
        "in a LilyPond output file using LilyPond's `set-global-staff-size' "
        "function.  "
        "Set to 0 to not specify a staff size and use LilyPond's default "
        "setting (in LilyPond version 2.12 the default is 20).";
    set->typedoc = indenttype;

    module_setval_int(&set->val, 0);

    set->loc = module_locscore;
    set->valid = valid_indenttype;
    set->uselevel = 3;
    lilystaffsizeid = id;
    break;
  }
  case 36: {
    set->name = "lily-macro-fzp"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `fzp' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "fzp");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrofzpid = id;
    break;
  }
  case 37: {
    set->name = "lily-macro-sfp"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `sfp' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "sfp");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrosfpid = id;
    break;
  }
  case 38: {
    set->name = "lily-macro-sfzp"; // docscat{lilyout}
    set->type = module_string;
    set->descdoc =
        "The name of the macro that inserts a `sfzp' dynamic text mark."
        "  If set to an empty string, no macro is used and the command is "
        "written out every time.";
    // set->typedoc = accslisttype;

    module_setval_string(&set->val, "sfzp");

    set->loc = module_locscore;
    // set->valid = valid_accslist;
    set->uselevel = 3;
    lilymacrosfzpid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}

void module_ready() {
  // maxtupslvlid = module_settingid("max-tuplets");
  // if (maxtupslvlid < 0) {ierr = "missing required setting `max-tuplets'";
  // return;}
  timesigstyleid = module_settingid("timesig-c");
  if (timesigstyleid < 0) {
    ierr = "missing required setting `timesig-c'";
    return;
  }
  verboseid = module_settingid("verbose");
  if (verboseid < 0) {
    ierr = "missing required setting `verbose'";
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
  defgracedurid = module_settingid("default-gracedur");
  if (defgracedurid < 0) {
    ierr = "missing required setting `default-gracedur'";
    return;
  }
  pitchedtrillid = module_settingid("show-trillnote");
  if (pitchedtrillid < 0) {
    ierr = "missing required setting `show-trillnote'";
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
    return "ly";
  default:
    return 0;
  }
}

const char* modout_get_saveid() {
  return 0;
}

void modout_write(FOMUS f, void* dat, const char* filename) {
  ((lilyoutdata*) dat)->modout_write(f, filename);
}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
