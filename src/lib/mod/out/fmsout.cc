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

// output a .fms score file
#include "config.h"

#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/iostreams/concepts.hpp> // sink
#include <boost/iostreams/stream.hpp>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/algorithm/string/trim.hpp>

#include <boost/rational.hpp>
#include <boost/variant.hpp>

#include <boost/ptr_container/ptr_vector.hpp>

#include "fomusapi.h"
#include "infoapi.h"
#include "module.h"

#define stdtostr(xxx) xxx
#define strtostd(xxx) xxx

#include "ilessaux.h"
using namespace ilessaux;
#include "ferraux.h"
using namespace ferraux;
// #include "debugaux.h"

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

namespace fmsout {

  const char* ierr = 0;

  int whichsettingsid, equalsstrid, filemaxwidthid, tabwidthid, bypartsid,
      nonrepeventsid, nbeatsid; //, orderid;

  template <typename I, typename T>
  inline bool hasnone(I first, const I& last, const T& pred) {
    while (first != last)
      if (pred(*first++))
        return false;
    return true;
  }

  std::set<int> mustbefirst;

  enum evtype {
    type_time,
    type_gtime,
    type_part,
    type_voice,
    type_dur,
    type_pitch,
    type_dyn,
    type_n
  };
  typedef std::map<const std::string, evtype, isiless> strtoevtypetype;
  strtoevtypetype evtypemap, evtyperetmap;
  inline void addtomap(const char* k, const evtype t) {
    evtypemap.insert(strtoevtypetype::value_type(k, t));
    evtyperetmap.insert(strtoevtypetype::value_type(std::string(k) + '*', t));
  }

  enum equalsstr_enum { equalsstr_equals, equalsstr_colon };

  std::map<const std::string, info_setwhere, isiless> whichmap;
  typedef std::map<const std::string, info_setwhere, isiless>::value_type
      whichmap_val;
  std::map<const std::string, equalsstr_enum, isiless> equalsmap;
  typedef std::map<const std::string, equalsstr_enum, isiless>::value_type
      equalsstr_val;

  struct outdata {
    enum info_setwhere whset;
    enum equalsstr_enum eqstr;
    fomus_int fwidth;
    int twidth;
    bool first;
    // bool userdefpart;
    outdata(FOMUS fom)
        : whset(
              whichmap.find(module_setting_sval(fom, whichsettingsid))->second),
          eqstr(equalsmap.find(module_setting_sval(fom, equalsstrid))->second),
          fwidth(module_setting_ival(fom, filemaxwidthid)),
          twidth(module_setting_ival(fom, tabwidthid)),
          first(true) /*, userdefpart(false)*/ {
      assert(nbeatsid >= 0);
      // assert(!module_reterr());
    }
    void printset(std::ostream& f, const info_setting& set);
    void printobj(std::ostream& f, const std::string& typ,
                  const info_objinfo& set);
    void modout_write(FOMUS f, const char* filename);
    void printit(std::ostream& f, const std::string& x);
    void printnotes(std::ostream& out, const module_partobj p,
                    module_measobj& m0, module_noteobj& n,
                    const std::vector<module_markobj>& mevs);
  };

  const char* writesettingstype = "none|score|config|all";
  int valid_writesetsstring(const char* val) {
    return whichmap.find(val) != whichmap.end();
  }
  int valid_writesets(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_writesetsstring,
                               writesettingstype);
  }
  const char* writeequalstype = "equals|colon";
  int valid_writeequalsstring(const char* val) {
    return equalsmap.find(val) != equalsmap.end();
  }
  int valid_writeequals(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_writeequalsstring,
                               writeequalstype);
  }
  const char* writemaxwidthtype = "integer>=79";
  int valid_maxwidth(const struct module_value val) {
    return module_valid_int(val, 79, module_incl, -1, module_nobound, 0,
                            writemaxwidthtype);
  }
  const char* writetabwidthtype = "integer0..20";
  int valid_tabwidth(const struct module_value val) {
    return module_valid_int(val, 0, module_incl, 20, module_incl, 0,
                            writemaxwidthtype);
  }
  const char* nonreptype = "(eventtype eventtype ...), eventtype = "
                           "t|ti|tim|time|t*|ti*|tim*|time*|d|du|dur|duration|"
                           "d*|du*|dur*|duration*|...";
  // const char* ordertype = "(eventtype eventtype ...), eventtype =
  // t|ti|tim|time|d|du|dur|duration|...";
  int valid_nonrep_aux(int n, const char* val) {
    return evtypemap.find(val) != evtypemap.end() ||
           evtyperetmap.find(val) != evtyperetmap.end();
  }
  int valid_nonrep(struct module_value val) {
    return module_valid_listofstrings(val, -1, -1, -1, -1, valid_nonrep_aux,
                                      nonreptype);
  }

  struct settingslist { // info interface is better in this case--sorts it
    struct info_setlist l;
    settingslist(FOMUS f, const info_setwhere wh) {
      info_setfilter fi = {0, 0, 0, module_nomodtype, 0, module_noloc, 3, wh};
      info_setfilterlist fl = {1, &fi};
      info_sortpair so[] = {{info_modname, info_ascending},
                            {info_setname, info_ascending}};
      info_sortlist sl = {2, so};
      l = info_list_settings(f, &fl, 0, &sl, 0);
    }
    //~settingslist() {info_free_setlist(l);}
  };

  struct repl_str {
    const std::string re;
    const std::string wi;
  };
  std::string stringify(std::string s, const char* inlist) {
    static const repl_str repl[] = {
        {"\\", "\\\\"},
        {"\t", "\\t"},
        {"\n", "\\n"}}; // the only escapes allowed in parse.h
    std::for_each(repl, repl + 3,
                  boost::lambda::bind(
                      boost::replace_all<std::string, const std::string,
                                         const std::string>,
                      boost::lambda::var(s),
                      boost::lambda::bind(&repl_str::re, boost::lambda::_1),
                      boost::lambda::bind(&repl_str::wi, boost::lambda::_1)));
    if (!s.empty() && s[0] != '"' && s[0] != '\'' &&
        hasnone(s.begin(), s.end(), boost::lambda::_1 == ' ') &&
        !boost::contains(s, "//") && !boost::contains(s, "//") &&
        (inlist == 0 || s.find_first_of(inlist) == std::string::npos))
      return s;
    boost::replace_all(s, "\"", "\\\"");
    return '"' + s + '"';
  };

  void outdata::printit(std::ostream& f, const std::string& x) {
    std::string::size_type j = 0, k = 0;
    bool inq = false;
    int ll = fwidth;
    for (std::string::size_type i = 0, ie = x.size(); i < ie; ++i) {
      char x1 = x[i];
      char x2 = i + 1 < ie ? x[i + 1] : 0;
      if (inq) {
        if (x1 == '\\' && x2 == '"')
          ++i;
        else if (x1 == '"')
          inq = false;
      } else {
        if (x1 == '\\' && x2 == ' ')
          ++i;
        else if (x1 == ' ' && x2 != ' ') {
          if (i - j >= (std::string::size_type) ll && k - j > 0) {
            f << x.substr(j, k - j) << '\n' << std::string(twidth, ' ');
            ll = fwidth - twidth;
            j = k;
          }
          k = i + 1;
        } else if (x1 == '"')
          inq = true;
      }
    }
    f << x.substr(j) << '\n'; // j must be < ie
  }

  void outdata::printset(std::ostream& f, const info_setting& set) {
    std::ostringstream os;
    os << set.name;
    switch (eqstr) {
    case equalsstr_equals:
      os << " = ";
      break;
    case equalsstr_colon:
      os << ": ";
    }
    os << set.valstr; // << '\n';
    printit(f, os.str());
  }
  void outdata::printobj(std::ostream& f, const std::string& typ,
                         const info_objinfo& set) {
    std::ostringstream os;
    os << typ << ' ' << set.valstr;
    printit(f, os.str());
  }

  union multival {
    module_value modval;
    module_intslist ints;
    void* ptr;
  };

  struct printbase {
    bool prret;
    std::string str;
    multival val;
    fomus_int srt;
    bool free;
    int ind;
    printbase(const std::string& str, const multival& mv, const fomus_int srt,
              const bool prret = false, const int ind = 0)
        : prret(prret), str(str), val(mv), srt(srt), free(free), ind(ind) {}
    void print(std::ostream& f, bool& isnl);
  };
  inline bool operator<(const printbase& x, const printbase& y) {
    return x.srt == y.srt ? (x.prret ? 1 : 0) > (y.prret ? 1 : 0)
                          : x.srt < y.srt;
  }
  void printbase::print(std::ostream& f, bool& isnl) {
    if (isnl && ind)
      f << std::string(ind, ' ');
    f << str;
    if (prret) {
      f << '\n';
      isnl = true;
    } else {
      f << ' ';
      isnl = false;
    }
  }

  inline bool valeq(const module_value& x, const module_value& y) {
    return (
        x.type == y.type &&
        (x.type == module_string ? boost::iequals(x.val.s, y.val.s) : x == y));
  }
  inline std::ostream& operator<<(std::ostream& ou, const module_value& x) {
    switch (x.type) {
    case module_int:
    case module_rat:
    case module_float: {
      ou << module_valuetostr(x);
      break;
    } break;
    case module_string:
      ou << x.val.s;
      break;
    default:
      assert(false);
    }
    return ou;
  }

  void outdata::printnotes(std::ostream& out, const module_partobj p,
                           module_measobj& m, module_noteobj& n0,
                           const std::vector<module_markobj>& mevs) {
    boost::ptr_vector<printbase> prints;
    printbase* regs[type_n] = {0}; // "registers"
    fomus_int nn = 0;
    std::vector<module_markobj>::const_iterator mev(mevs.begin());
    while (true) {
      module_noteobj n = (!n0 || (p && module_part(n0) != p)) ? 0 : n0;
      bool ism;
      if (mev != mevs.end()) {
        module_value ti;
        if (n)
          ti = module_vtime(n);
        else {
          ti.type = module_rat;
          ti.val.r =
              module_makerat(std::numeric_limits<fomus_int>::max() - 1, 1);
        }
        if (module_vtime(*mev) < ti) {
          n = *mev++;
          ism = true;
        } else
          ism = false;
      } else
        ism = false;
      if (!n)
        break;
      if (first) {
        out << "// events\n";
        first = false;
      }
      bool isr = !ism && module_isrest(n);
      module_value wh(module_setting_val(n, nonrepeventsid));
      assert(wh.type == module_list);
      bool keepregs[type_n] = {false};
      bool withrets[type_n] = {false};
      for (module_value *i = wh.val.l.vals, *ie = wh.val.l.vals + wh.val.l.n;
           i < ie; ++i) {
        assert(i->type == module_string);
        strtoevtypetype::const_iterator z(evtypemap.find(i->val.s));
        if (z == evtypemap.end()) {
          z = evtyperetmap.find(i->val.s);
          assert(z != evtyperetmap.end());
          withrets[z->second] = true;
        }
        keepregs[z->second] = true;
      }
      if (m) {
        multival mv;
        mv.modval = module_vtime(m);
        if (module_vtime(n) >= mv.modval && module_part(m) == p) {
          std::ostringstream mm;
          mm << "time" << ' ' << mv.modval; // << " ";
          prints.push_back(regs[type_time] = new printbase(
                               mm.str(), mv, nn, withrets[type_time],
                               withrets[type_time] ? 0 : twidth));
          mm.str("");
          if (module_setting_rval(m, nbeatsid) <= (fomus_int) 0) {
            mm << "duration" << ' ' << module_vdur(m); // << " ";
            prints.push_back(regs[type_dur] = new printbase(
                                 mm.str(), mv, nn, withrets[type_dur],
                                 withrets[type_dur] ? 0 : twidth));
            mm.str("");
          }
          mm << info_valstr(m);
          prints.push_back(regs[type_time] =
                               new printbase(mm.str(), mv, ++nn, true, twidth));
          m = module_nextmeas();
        }
      }
      for (int i = 0; i < type_n; ++i) {
        if (!keepregs[i])
          regs[i] = 0;
        std::ostringstream valstr;
        multival mv;
        switch (i) {
        case type_voice: {
          mv.ints = module_voices(n);
          assert(mv.ints.n > 0);
          if (regs[type_voice]) {
            if (regs[type_voice]->val.ints.n == mv.ints.n) {
              for (const int *i1 = regs[type_voice]->val.ints.ints,
                             *i2 = mv.ints.ints,
                             *i2e = mv.ints.ints + mv.ints.n;
                   i2 < i2e; ++i1, ++i2) {
                if (*i1 != *i2)
                  goto VSKIP;
              }
              goto MORE;
            }
          }
        VSKIP:
          valstr << "voice" << ' ';
          if (mv.ints.n <= 1) {
            valstr << *mv.ints.ints;
          } else {
#warning "*** make these prefixes user configurable ***"
            valstr << '(';
            for (const int *v = mv.ints.ints, *ve = mv.ints.ints + mv.ints.n;
                 v < ve; ++v) {
              if (v > mv.ints.ints)
                valstr << ' ';
              valstr << *v;
            }
            valstr << ')';
          }
        } break;
        case type_time: {
          mv.modval = module_vtime(n);
          module_value gt(module_vgracetime(n));
          if (regs[type_time] &&
              valeq(regs[type_time]->val.modval, mv.modval) &&
              !(regs[type_gtime] && module_vgracetime(n).type == module_none))
            goto MORE;
          valstr << "time" << ' ' << mv.modval;
          regs[type_gtime] = 0;
        } break;
        case type_gtime: {
          mv.modval = module_vgracetime(n);
          if (mv.modval.type == module_none || mv.modval < (fomus_int) -1000 ||
              mv.modval > (fomus_int) 1000 ||
              (regs[type_gtime] &&
               valeq(regs[type_gtime]->val.modval, mv.modval)))
            goto MORE;
          valstr << "grace" << ' ' << mv.modval;
        } break;
        case type_dyn:
          if (ism || isr)
            goto MORE;
          {
            mv.modval = module_dyn(n);
            if (regs[type_dyn] && valeq(regs[type_dyn]->val.modval, mv.modval))
              goto MORE;
            valstr << "dynamic" << ' ' << mv.modval;
          }
          break;
        case type_dur: {
          module_value x(module_vgracetime(n));
          if (x.type != module_none &&
              (x < (fomus_int) -1000 || x > (fomus_int) 1000)) {
            valstr << "duration" << ' ' << (x < (fomus_int) 0 ? '-' : '+');
            regs[type_dur] = 0;
          } else {
            mv.modval = module_vdur(n);
            if (regs[type_dur] && valeq(regs[type_dur]->val.modval, mv.modval))
              goto MORE;
            valstr << "duration" << ' ' << mv.modval;
          }
        } break;
        case type_part: {
          if (p)
            goto MORE;
          mv.ptr = module_part(n);
          if (regs[type_part] && mv.ptr != regs[type_part]->val.ptr)
            goto MORE;
          valstr << "part" << ' ' << module_id(mv.ptr);
        } break;
        case type_pitch:
          if (ism || isr)
            goto MORE;
          {
            if (module_isperc(n)) {
              mv.modval.type = module_string;
              mv.modval.val.s = module_percinststr(n);
            } else
              mv.modval = module_vpitch(n);
            if (regs[type_pitch] &&
                valeq(regs[type_pitch]->val.modval, mv.modval))
              goto MORE;
            valstr << "pitch" << ' ' << mv.modval;
          }
          break;
        default:
          assert(false);
        }
        prints.push_back(regs[i] =
                             new printbase(valstr.str(), mv, nn, withrets[i],
                                           withrets[i] ? 0 : twidth));
      MORE:;
      }
      info_setlist ss(info_get_settings(n));
      for (struct info_setting *i(ss.sets), *ie(ss.sets + ss.n); i < ie; ++i) {
        std::ostringstream valstr;
        multival mv;
        valstr << i->name << ' ' << i->valstr;
        mv.modval = i->val;
        prints.push_back(new printbase(valstr.str(), mv, nn, false, twidth));
      }
      std::ostringstream xx;
      module_markslist ml(module_marks(n));
      for (const module_markobj *i = ml.marks, *ie = ml.marks + ml.n; i < ie;
           ++i) {
        xx << '[' << module_id(*i);
        const char* x = module_markstring(*i);
        if (x)
          xx << ' ' << stringify(x, "]");
        module_value m(module_marknum(*i));
        if (m.type != module_none)
          xx << ' ' << m;
        xx << ']';
      }
      if (ml.n > 0)
        xx << ' ';
      if (ism)
        xx << "mark ";
      else if (isr)
        xx << "rest ";
      xx << ';';
      prints.push_back(new printbase(xx.str(), multival(), ++nn, true, twidth));
      if (!ism) {
        module_skipassign(n);
        n0 = module_nextnote();
      }
    }
    std::stable_sort(prints.begin(), prints.end());
    bool di = true;
    std::for_each(prints.begin(), prints.end(),
                  boost::lambda::bind(&printbase::print, boost::lambda::_1,
                                      boost::lambda::var(out),
                                      boost::lambda::var(di)));
  }

  int filenameid, outputid, nthreadsid;

  void
  outdata::modout_write(FOMUS fom,
                        const char* filename) { // filename should be complete
    try {
      boost::filesystem::path fn(filename);
      boost::filesystem::ofstream f;
      try {
        f.exceptions(boost::filesystem::ofstream::eofbit |
                     boost::filesystem::ofstream::failbit |
                     boost::filesystem::ofstream::badbit);
        f.open(fn.FS_FILE_STRING(), boost::filesystem::ofstream::out |
                                        boost::filesystem::ofstream::trunc);
        f << "// " << PACKAGE_STRING << '\n';
        {
          time_t tim;
          if (time(&tim) != -1) {
#ifdef HAVE_CTIME_R
            char buf[27]; // user-supplied buffer of length at least 26
            f << "// " << ctime_r(&tim, buf) << '\n';
#else
            f << "// " << ctime(&tim) << '\n';
#endif
          }
        }
        { // --------------------SETTINGS
          if (whset != info_none) {
            f << "// settings\n";
            settingslist sets(fom,
                              whset); // get sorted settings???  by module...
            std::string curmod;
            for (info_setting *i = sets.l.sets, *ie = sets.l.sets + sets.l.n;
                 i < ie; ++i) {
              if (i->id == filenameid || i->id == outputid ||
                  i->id == nthreadsid)
                continue;
              if (mustbefirst.find(i->id) != mustbefirst.end()) {
                if (curmod != i->modname) {
                  curmod = i->modname;
                  f << "// module `" << curmod << "'\n";
                }
                printset(f, *i);
              }
            }
            for (info_setting *i = sets.l.sets, *ie = sets.l.sets + sets.l.n;
                 i < ie; ++i) {
              if (i->id == filenameid || i->id == outputid ||
                  i->id == nthreadsid)
                continue;
              if (mustbefirst.find(i->id) == mustbefirst.end()) {
                if (curmod != i->modname) {
                  curmod = i->modname;
                  f << "// module `" << curmod << "'\n";
                }
                printset(f, *i);
              }
            }
            f << "\n// " << std::string(fwidth - 3, '-') << "\n\n";
          }
        }
        { // --------------------PARTS, INSTS, ETC..
          info_objinfo_list pi = info_get_percinsts(fom);
          bool x = false;
          if (pi.n > 0) {
            f << "// percussion instruments\n";
            for (info_objinfo *i = pi.objs, *e = pi.objs + pi.n; i < e; ++i)
              printobj(f, "percinst", *i);
            f << '\n';
            x = true;
          }
          pi = info_get_insts(fom);
          if (pi.n > 0) {
            f << "// instruments\n";
            for (info_objinfo *i = pi.objs, *e = pi.objs + pi.n; i < e; ++i)
              printobj(f, "inst", *i);
            f << '\n';
            x = true;
          }
          pi = info_get_parts(fom);
          if (pi.n > 0) {
            f << "// parts\n";
            for (info_objinfo *i = pi.objs, *e = pi.objs + pi.n; i < e; ++i) {
              printobj(f, "part", *i);
            }
            f << '\n';
            x = true;
          }
          pi = info_get_metaparts(fom);
          if (pi.n > 0) {
            f << "// metaparts\n";
            for (info_objinfo *i = pi.objs, *e = pi.objs + pi.n; i < e; ++i)
              printobj(f, "metapart", *i);
            f << '\n';
            x = true;
          }
          pi = info_get_measdefs(fom);
          if (pi.n > 0) {
            f << "// measure definition\n";
            for (info_objinfo *i = pi.objs, *e = pi.objs + pi.n; i < e; ++i)
              printobj(f, "measdef", *i);
            f << '\n';
            x = true;
          }
          if (x)
            f << "// " << std::string(fwidth - 3, '-') << "\n\n";
        }
        module_noteobj nn = module_nextnote();
        module_measobj m = module_nextmeas();
        { // --------------------THE NOTES
          if (module_setting_ival(fom, bypartsid)) {
            while (true) {
              module_partobj p = module_nextpart();
              if (!p)
                break;
              const char* pid = module_id(p);
              if (first) {
                f << "// events\n";
                first = false;
              }
              f << "part " << pid << '\n';
              module_objlist me0(module_getmarkevlist(p));
              std::vector<module_markobj> me(me0.objs, me0.objs + me0.n);
              printnotes(f, p, m, nn, me);
              f << '\n';
            }
          } else {
            std::vector<module_markobj> me;
            while (true) {
              module_partobj p = module_nextpart();
              if (!p)
                break;
              module_objlist me0(module_getmarkevlist(p));
              std::copy(me0.objs, me0.objs + me0.n, std::back_inserter(me));
            }
            printnotes(f, 0, m, nn, me);
            f << '\n';
          }
        }
        f.close();
      } catch (const boost::filesystem::ofstream::failure& e) {
        CERR << "error writing `" << fn.FS_FILE_STRING() << '\'' << std::endl;
      }
    } catch (const boost::filesystem::filesystem_error& e) {
      CERR << "invalid path/filename `" << filename << '\'' << std::endl;
    }
  }

} // namespace fmsout

using namespace fmsout;
// ------------------------------------------------------------------------------------------------------------------------
// END OF NAMESPACE

const char* module_longname() {
  return "FOMUS File Output";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Writes `.fms' files (FOMUS's native file format).";
}
void* module_newdata(FOMUS f) {
  return new fmsout::outdata(f);
}
void module_freedata(void* dat) {
  delete (fmsout::outdata*) dat;
}
const char* module_err(void* dat) {
  return 0;
}

enum module_type module_type() {
  return module_modoutput;
}
const char* module_initerr() {
  return ierr;
}
int module_itertype() {
  return module_all;
}

void module_init() {
  fmsout::whichmap.insert(
      fmsout::whichmap_val("none", info_none)); // return nothing
  fmsout::whichmap.insert(
      fmsout::whichmap_val("all", info_default)); // return all
  fmsout::whichmap.insert(fmsout::whichmap_val(
      "config", info_config)); // return only config changed
  fmsout::whichmap.insert(fmsout::whichmap_val(
      "score", info_global)); // return only changed in input file
  fmsout::equalsmap.insert(
      fmsout::equalsstr_val("equals", fmsout::equalsstr_equals));
  fmsout::equalsmap.insert(
      fmsout::equalsstr_val("colon", fmsout::equalsstr_colon));
  addtomap("v", type_voice);
  addtomap("vo", type_voice);
  addtomap("voi", type_voice);
  addtomap("voice", type_voice);
  addtomap("t", type_time);
  addtomap("ti", type_time);
  addtomap("tim", type_time);
  addtomap("time", type_time);
  addtomap("g", type_gtime);
  addtomap("gr", type_gtime);
  addtomap("gra", type_gtime);
  addtomap("grace", type_gtime);
  addtomap("y", type_dyn);
  addtomap("dy", type_dyn);
  addtomap("dyn", type_dyn);
  addtomap("dynamic", type_dyn);
  addtomap("d", type_dur);
  addtomap("du", type_dur);
  addtomap("dur", type_dur);
  addtomap("duration", type_dur);
  addtomap("a", type_part);
  addtomap("pa", type_part);
  addtomap("par", type_part);
  addtomap("part", type_part);
  addtomap("p", type_pitch);
  addtomap("pi", type_pitch);
  addtomap("pit", type_pitch);
  addtomap("pitch", type_pitch);
}
void module_free() { /*assert(newcount == 0);*/
}

int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "write-which-settings"; // docscat{fmsout}
    set->type = module_string;
    set->descdoc =
        "Indicates which global settings are saved to an `.fms' output file.  "
        "`none' means none, `score' means only those set globally in a score, "
        "`config' means those set in a score and set in the `.fomus' config "
        "file, "
        "and `all' means dump all setting values (including internal "
        "defaults).";
    set->typedoc = fmsout::writesettingstype;

    module_setval_string(&set->val, "score");

    set->loc = module_locscore;
    set->valid = fmsout::valid_writesets;
    set->uselevel = 2; // more useful than other "aesthetics" settings
    whichsettingsid = id;
    break;
  }
  case 1: {
    set->name = "write-equals-str"; // docscat{fmsout}
    set->type = module_string;
    set->descdoc =
        "Whether to use `=' or `:' when outputting setting values to an `.fms' "
        "output file.  "
        "Set this value to `equals' to get `=' characters and `colon' to get "
        "`:' characters between setting names and values.";
    set->typedoc = fmsout::writeequalstype;

    module_setval_string(&set->val, "equals");

    set->loc = module_locscore;
    set->valid = fmsout::valid_writeequals;
    set->uselevel = 2; // less useful
    equalsstrid = id;
    break;
  }
  case 2: {
    set->name = "write-file-width"; // docscat{fmsout}
    set->type = module_int;
    set->descdoc =
        "Number of characters allowed per line in an `.fms' output file."
        "  Used to wrap long lines into a more readable format.";
    set->typedoc = fmsout::writemaxwidthtype;

    module_setval_int(&set->val, 119);

    set->loc = module_locscore;
    set->valid = fmsout::valid_maxwidth;
    set->uselevel = 2; // nitpicky
    filemaxwidthid = id;
    break;
  }
  case 3: {
    set->name = "write-indent-width"; // docscat{fmsout}
    set->type = module_int;
    set->descdoc =
        "Number of spaces used for indentation in an `.fms' output file.";
    set->typedoc = fmsout::writetabwidthtype;

    module_setval_int(&set->val, 8);

    set->loc = module_locscore;
    set->valid = fmsout::valid_tabwidth;
    set->uselevel = 2; // nitpicky
    tabwidthid = id;
    break;
  }
  case 4: {
    set->name = "write-bypart"; // docscat{fmsout}
    set->type = module_bool;
    set->descdoc =
        "Whether or not to group output events by part in an `.fms' output "
        "file."
        "  Setting this to `yes' creates separate sections for each part."
        "  Setting this to `no' organizes all events by their times, switching "
        "parts as necessary.";
    // set->typedoc = fmsout::writetabwidthtype;

    module_setval_int(&set->val, 1);

    set->loc = module_locscore;
    // set->valid = fmsout::valid_tabwidth;
    set->uselevel = 2;
    bypartsid = id;
    break;
  }
  case 5: {
    set->name = "write-nonrepeat-events"; // docscat{fmsout}
    set->type = module_list_strings;
    set->descdoc =
        "A list of event parameters to write without repetition in an `.fms' "
        "output file "
        "(i.e., values are written only when they change), making the output "
        "more concise and easier to read."
        "  Set this to the parameters that you don't want repeated in each "
        "note event."
        "  An asterisk after a type string indicates that non-repeating events "
        "should also be followed by a newline to aid visibility.";
    set->typedoc = fmsout::nonreptype;

    module_setval_list(&set->val, 1);
    module_setval_string(set->val.val.l.vals, "time*");
    // module_setval_string(set->val.val.l.vals + 1, "grace*");
    // module_setval_string(set->val.val.l.vals + 1, "voice*");

    set->loc = module_locnote;
    set->valid = fmsout::valid_nonrep;
    set->uselevel = 2;
    nonrepeventsid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}

void module_ready() {
  int id;
  mustbefirst.insert(id = module_settingid("note-accs"));
  if (id < 0) {
    ierr = "missing required setting `note-accs'";
    return;
  }
  mustbefirst.insert(id = module_settingid("note-microtones"));
  if (id < 0) {
    ierr = "missing required setting `note-microtones'";
    return;
  }
  mustbefirst.insert(id = module_settingid("note-octaves"));
  if (id < 0) {
    ierr = "missing required setting `note-octaves'";
    return;
  }
  mustbefirst.insert(id = module_settingid("note-symbols"));
  if (id < 0) {
    ierr = "missing required setting `note-symbols'";
    return;
  }
  mustbefirst.insert(id = module_settingid("dur-dots"));
  if (id < 0) {
    ierr = "missing required setting `dur-dots'";
    return;
  }
  mustbefirst.insert(id = module_settingid("dur-symbols"));
  if (id < 0) {
    ierr = "missing required setting `dur-symbols'";
    return;
  }
  mustbefirst.insert(id = module_settingid("dur-tie"));
  if (id < 0) {
    ierr = "missing required setting `dur-tie'";
    return;
  }
  mustbefirst.insert(id = module_settingid("tuplet-symbols"));
  if (id < 0) {
    ierr = "missing required setting `tuplet-symbols'";
    return;
  }
  nbeatsid = module_settingid("measdur");
  if (nbeatsid < 0) {
    ierr = "missing required setting `measdur'";
    return;
  }
  filenameid = module_settingid("filename");
  if (filenameid < 0) {
    ierr = "missing required setting `filename'";
    return;
  }
  outputid = module_settingid("output");
  if (outputid < 0) {
    ierr = "missing required setting `output'";
    return;
  }
  nthreadsid = module_settingid("n-threads");
  if (nthreadsid < 0) {
    ierr = "missing required setting `n-threads'";
    return;
  }
}

// only fmsout.cc returns true here (outputs before processing anything)
fomus_bool modout_ispre() {
  return true;
}

const char* modout_get_extension(int n) {
  switch (n) {
  case 0:
    return "fms";
  default:
    return 0;
  }
}

const char* modout_get_saveid() {
  return 0;
}

void modout_write(FOMUS f, void* dat, const char* filename) {
  ((fmsout::outdata*) dat)->modout_write(f, filename);
}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
