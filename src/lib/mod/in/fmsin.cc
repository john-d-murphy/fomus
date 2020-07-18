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

// FOMUS FILE INPUT
#include <cmath>
#include <cstring>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector> // parse.h needs it

#include <boost/rational.hpp>
#include <boost/variant.hpp>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/iostreams/concepts.hpp> // sink
#include <boost/iostreams/stream.hpp>

#ifndef BOOST_SPIRIT_CLASSIC
#include <boost/spirit/core.hpp>
#include <boost/spirit/dynamic.hpp>
#include <boost/spirit/error_handling/exceptions.hpp>
#include <boost/spirit/iterator/position_iterator.hpp>
#include <boost/spirit/symbols.hpp>
#include <boost/spirit/utility.hpp>
#include <boost/spirit/utility/confix.hpp>
#include <boost/spirit/utility/rule_parser.hpp>
namespace boostspirit = boost::spirit;
#else
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_dynamic.hpp>
#include <boost/spirit/include/classic_error_handling.hpp>
#include <boost/spirit/include/classic_exceptions.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/classic_rule_parser.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_utility.hpp>
namespace boostspirit = boost::spirit::classic;
#endif
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#define BOOST_SPIRIT__NAMESPACE -

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include <boost/utility.hpp>

#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#define FNAMESPACE fmsin

#define CMD_MACRO_STRINGIFY(xxx) #xxx
#define stdtostr(xxx) xxx
#define strtostd(xxx) xxx
#define CMD_MACRO(xxx) CMD_MACRO_STRINGIFY(xxx)
#include "algext.h"

#include "fomusapi.h"

#include "debugaux.h"
#include "ferraux.h"
using namespace ferraux;

#include "numbers.h"
#include "parse.h"

// ------------------------------------------------------------------------------------------------------------------------
// MODULE STUFF
// ------------------------------------------------------------------------------------------------------------------------

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

namespace fmsin {

  bool boolfalse = false;

  void filepos::printerr0(std::ostream& ou) const {
    if (line >= 0) {
      ou << " in " << linestr << ' ' << line;
      if (col >= 0)
        ou << ", " << colstr << ' ' << col;
      if (!file.empty())
        ou << " of `" << file << '\'';
    } else if (!file.empty())
      ou << " in `" << file << '\'';
  }

  // int tupsymbsid, dursymbsid, durdotid, durtieid;
  int tabcharsid;

  struct indata {
    int tabchars;
    indata(FOMUS f) : tabchars(module_setting_ival(f, tabcharsid)) {}
    bool modin_load(FOMUS fom, const char* filename, const bool isfile);
  };

  const char* dursymstype = "(string |rational>0|, string |rational>0|, ...)";
  int valid_tupsyms(const struct module_value val) {
    return module_valid_maptorats(val, -1, -1, module_makerat(0, 1),
                                  module_excl, module_makerat(0, 1),
                                  module_nobound, 0, dursymstype);
  }
  int valid_dursyms(const struct module_value val) {
    return module_valid_maptorats(val, -1, -1, module_makerat(0, 1),
                                  module_excl, module_makerat(0, 1),
                                  module_nobound, 0, dursymstype);
  }
  const char* stringstype = "(string string ...)";
  int valid_listofdurdots(const struct module_value val) {
    return module_valid_listofstrings(val, -1, -1, 1, -1, 0, stringstype);
  }
  int valid_listofties(const struct module_value val) {
    return module_valid_listofstrings(val, -1, -1, 1, -1, 0, stringstype);
  }

  typedef std::map<const std::string, int> bracketmap;
  typedef bracketmap::value_type bracketmaptype;
  typedef bracketmap::iterator bracketmap_it;

  // scratch structure
  struct rulespackage;
  struct inscratch _NONCOPYABLE {
    const FOMUS fom;
    filepos pos;
    fint pt1, pt2, pt3;
    bool b, b2;
    fint i;
    rat r;
    ffloat f;
    numb num;
    std::string str;
    const std::string* strptr;
    listelvect lst; //, lst2; // lst2 used for inner lists
    std::string nam;
    bracketmap brs; // brackets
    fint idcnt;
    fomus_param par;
    fomus_action act;
    bool attop;
    bool isplus, isplus2;
    std::list<std::string> psymsv;
    boostspirit::symbols<std::string*>& psyms;
    bool& err;
    std::auto_ptr<listelvect> autolst;

    inscratch(const FOMUS fo, const std::string& file,
              boostspirit::symbols<std::string*>& psyms, bool& err)
        : fom(fo), pos(file), idcnt(0), attop(true), isplus2(false),
          psyms(psyms), err(err), autolst(new listelvect) {}
    void getrule(parserule& rule, const struct info_setting& set,
                 const char* endstr, rulespackage& pack);
    void printerr() const {
      pos.printerr(ferr);
    }
  };

  // ------------------------------------------------------------------------------------------------------------------------
  // RULES PACKAGE

  struct macroinfo {
    std::vector<std::string> args;
    std::string body;
    macroinfo(const listelvect& args0, const std::string& body) : body(body) {
      std::transform(args0.begin(), args0.end(), std::back_inserter(args),
                     listel_getstring);
    }
  };

  struct macrodef {
    rulespackage& pkg;
    boost::ptr_map<int, macroinfo> bodies; // stored by num of arguments
    listelvect repls;
    parserule rule;
    std::string name;
    bool haszero;
    macrodef(rulespackage& pkg, const std::string& name);
    void execute(const parse_it& s1);
    void insert(const listelvect& args, const std::string& body) {
      int n = args.size();
      bodies.erase(n);
      bodies.insert(n, new macroinfo(args, body));
      if (n <= 0)
        haszero = true;
    }
  };

  struct exemacro {
    macrodef& mac;
    exemacro(macrodef& mac) : mac(mac) {}
    void operator()(const parse_it& s1, const parse_it& s2) const;
  };
  struct defmacro {
    rulespackage& pkg;
    std::string& name;
    listelvect& args;
    std::string& body;
    defmacro(rulespackage& pkg, std::string& name, listelvect& args,
             std::string& body)
        : pkg(pkg), name(name), args(args), body(body) {}
    void operator()(const parse_it& s1, const parse_it& s2) const;
  };

  typedef boost::ptr_vector<macrodef> macrovect;
  typedef macrovect::iterator macrovect_it;

  // ------------------------------------------------------------------------------------------------------------------------
  // SETTINGS

  // struct to get setting info
  struct settingslist {
    struct info_setlist l;
    settingslist(FOMUS fom) {
      l = info_get_settings(fom);
    }
    inline int count_loctype(const enum module_setting_loc ty) const {
      return std::count_if(l.sets, l.sets + l.n,
                           bind(module_setting_allowed,
                                bind(&info_setting::loc, boost::lambda::_1),
                                ty));
    }
    void getrules(boostspirit::symbols<parserule*>& conts, inscratch& xx,
                  parserule* p, const char* endstr, rulespackage& pack) const;
  };

  void settingslist::getrules(boostspirit::symbols<parserule*>& conts,
                              inscratch& xx, parserule* p, const char* endstr,
                              rulespackage& pack) const {
    for (info_setting *i = l.sets, *ie = l.sets + l.n; i < ie; ++i) {
      conts.add(i->name, p);
      xx.getrule(*p, *i, endstr, pack);
      ++p;
    }
  }

  inline void insnumb(FOMUS fom, const enum fomus_param par,
                      const enum fomus_action act, const numb& x) {
    switch (x.type()) {
    case module_int:
      fomus_ival(fom, par, act, x.geti());
      break;
    case module_float:
      fomus_fval(fom, par, act, x.getf());
      break;
    case module_rat:
      fomus_rval(fom, par, act, x.getnum(), x.getden());
      break;
    default:
      assert(false);
    }
  }
  inline void insnumb(const inscratch& xx, const enum fomus_param par,
                      const enum fomus_action act) {
    insnumb(xx.fom, par, act, xx.num);
  }

  struct dosetting { // base class
    inscratch& xx;
    const int id;
    dosetting(inscratch& xx, const int id) : xx(xx), id(id) {}
  };
  // setting number
  struct dosettingnumb : public dosetting {
    dosettingnumb(inscratch& xx, const int id) : dosetting(xx, id) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_ival(xx.fom, fomus_par_setting, fomus_act_set, id);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
      insnumb(xx, xx.par, fomus_act_set);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  // setting strings
  struct dosettingstring : public dosetting {
    dosettingstring(inscratch& xx, const int id) : dosetting(xx, id) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_ival(xx.fom, fomus_par_setting, fomus_act_set, id);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
      fomus_sval(xx.fom, xx.par, fomus_act_set, xx.str.c_str());
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  // setting lists
  struct dosettinglist : public dosetting {
    dosettinglist(inscratch& xx, const int id) : dosetting(xx, id) {}
    void operator()(const parse_it& s1, const parse_it& s2) const;
  };
  class listel_set : public boost::static_visitor<void> {
    const inscratch& xx;

public:
    listel_set(const inscratch& xx) : xx(xx) {}
    void operator()(const numb& x) const {
      insnumb(xx.fom, fomus_par_list, fomus_act_add, x);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
    void operator()(const std::string& x) const {
      fomus_sval(xx.fom, fomus_par_list, fomus_act_add, x.c_str());
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
    void operator()(const boost::shared_ptr<listelvect>& x) const {
      fomus_act(xx.fom, fomus_par_list, fomus_act_start);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
      listel_set se(xx);
      for (listelvect::const_iterator i = x->begin(), ie = x->end(); i != ie;
           ++i)
        apply_visitor(se, *i);
      fomus_act(xx.fom, fomus_par_list, fomus_act_add);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  void dosettinglist::operator()(const parse_it& s1, const parse_it& s2) const {
    fomus_ival(xx.fom, fomus_par_setting, fomus_act_set, id);
    if (fomus_err()) {
      xx.err = true;
      DBG("FOMUS_ERR() WAS SET!");
    }
    fomus_act(xx.fom, fomus_par_list, fomus_act_start);
    if (fomus_err()) {
      xx.err = true;
      DBG("FOMUS_ERR() WAS SET!");
    }
    listel_set se(xx);
    for (listelvect::const_iterator i = xx.lst.begin(), ie = xx.lst.end();
         i != ie; ++i)
      apply_visitor(se, *i);
    fomus_act(xx.fom, fomus_par_list, fomus_act_end);
    if (fomus_err()) {
      xx.err = true;
      DBG("FOMUS_ERR() WAS SET!");
    }
    fomus_act(xx.fom, xx.par,
              xx.isplus2 ? fomus_act_append : fomus_act_set); // isplus2
    if (fomus_err()) {
      xx.err = true;
      DBG("FOMUS_ERR() WAS SET!");
    }
  }
  // record file position
  struct dofilepos {
    inscratch& xx;

public:
    dofilepos(inscratch& xx) : xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_ival(xx.fom, fomus_par_locline, fomus_act_set, xx.pos.line);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
      fomus_ival(xx.fom, fomus_par_loccol, fomus_act_set, xx.pos.col);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
      xx.isplus2 =
          xx.isplus; // have to save it because it is needed at the very end
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      totsetting,
      (5, (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), conts),
           ((parserule&), rule), ((const fomus_param), par),
           ((const char*), eqsyms))),
      -,
      symmatch(conts, rule, eqsyms)[setconstval<fomus_param>(xx.par, par)] >>
          pluseqlmatch(xx.isplus)
#ifndef NDEBUGOUT
              [print<bool>("isplus", xx.isplus)]
#endif
          >> rule)

  // ------------------------------------------------------------------------------------------------------------------------
  // GENERIC VALUE INSERTS

  // ***** generic value inserts
  struct insval { // base class (for generic insert operations where setting id
                  // doesn't have to be set first)
    inscratch& xx;
    const enum fomus_param par;
    const enum fomus_action act;
    insval(inscratch& xx, const enum fomus_param par,
           const enum fomus_action act)
        : xx(xx), par(par), act(act) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_act(xx.fom, par, act);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  struct insvalact { // base class (for generic insert operations where setting
                     // id doesn't have to be set first)
    inscratch& xx;
    const enum fomus_param par;
    insvalact(inscratch& xx, const enum fomus_param par) : xx(xx), par(par) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_act(xx.fom, par, xx.act);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  struct insact : public insval { // action
    insact(inscratch& xx, const enum fomus_param par,
           const enum fomus_action act)
        : insval(xx, par, act) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_act(xx.fom, par, act);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
    void operator()(const char x) const {
      fomus_act(xx.fom, par, act);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  // call a fomus api val entry function given par and act
  struct insnumbval
      : public insval { // just insert a numb value (offset, note, etc..)
    insnumbval(inscratch& xx, const enum fomus_param par,
               const enum fomus_action act)
        : insval(xx, par, act) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      insnumb(xx, par, act);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  struct insnumbvalact
      : public insvalact { // just insert a numb value (offset, note, etc..)
    insnumbvalact(inscratch& xx, const enum fomus_param par)
        : insvalact(xx, par) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      insnumb(xx, par, xx.act);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  // string value
  struct insstrval : public insval {
    insstrval(inscratch& xx, const enum fomus_param par,
              const enum fomus_action act)
        : insval(xx, par, act) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_sval(xx.fom, par, act, xx.str.c_str());
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  struct insstrval2 : public insval {
    insstrval2(inscratch& xx, const enum fomus_param par,
               const enum fomus_action act)
        : insval(xx, par, act) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_sval(xx.fom, par, act, xx.nam.c_str());
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  struct insparsedstrval1 {
    inscratch& xx;
    insparsedstrval1(inscratch& xx) : xx(xx) {}
    void operator()(const std::string* str) const {
      xx.strptr = str;
    }
  };
  struct insparsedstrval2 : public insval {
    insparsedstrval2(inscratch& xx, const enum fomus_param par,
                     const enum fomus_action act)
        : insval(xx, par, act) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_sval(xx.fom, par, act, xx.strptr->c_str());
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  // insert a list
  struct inslistval : public insval { // insert a list of numb values
    inslistval(inscratch& xx, const enum fomus_param par,
               const enum fomus_action act)
        : insval(xx, par, act) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_act(xx.fom, par, act); // startlist or appendlist
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
      listel_set se(xx);
      for (listelvect::const_iterator i = xx.lst.begin(), ie = xx.lst.end();
           i != ie; ++i)
        apply_visitor(se, *i);
      fomus_act(xx.fom, fomus_par_list, fomus_act_end);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      listmatchnums1,
      (6, (((listelvect&), lst), ((numb&), val), ((fint&), pt1), ((fint&), pt2),
           ((fint&), pt3), ((filepos&), pos))),
      -,
      listmatchhead(lst, boolfalse) >>
          numbermatch(val, pt1, pt2, pt3, pos, ferr)[setlistnum(lst, val)] >>
          listmatchdelim >>
          *(if_p(LISTEND)[boostspirit::nothing_p]
                .else_p[numbermatch(val, pt1, pt2, pt3, pos,
                                    ferr)[setlistnum(lst, val)] >>
                        listmatchdelim]) >>
          boostspirit::anychar_p)
  class listel_vset : public boost::static_visitor<void> {
    const inscratch& xx;
    const fomus_param par;
    const fomus_action act;

public:
    listel_vset(const inscratch& xx, const fomus_param par,
                const fomus_action act)
        : xx(xx), par(par), act(act) {}
    void operator()(const numb& x) const {
      insnumb(xx.fom, par, act, x);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
    void operator()(const std::string& x) const {
      assert(false);
    }
    void operator()(const boost::shared_ptr<listelvect>& x) const {
      assert(false);
    }
  };
  struct insvlistval : public insval { // insert a list of numb values
    insvlistval(inscratch& xx, const enum fomus_param par,
                const enum fomus_action act)
        : insval(xx, par, act) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      assert(!xx.lst.empty());
      if (act == fomus_act_set) {
        fomus_act(xx.fom, par, fomus_act_clear);
        if (fomus_err()) {
          xx.err = true;
          DBG("FOMUS_ERR() WAS SET!");
        }
      }
      if (par == fomus_par_region_voice) {
        fomus_act(xx.fom, fomus_par_region_voicelist, act);
        if (fomus_err()) {
          xx.err = true;
          DBG("FOMUS_ERR() WAS SET!");
        }
      } // w/ no list, set the region action
      listel_vset se(xx, par, (act == fomus_act_set ? fomus_act_add : act));
      for (listelvect::const_iterator i = xx.lst.begin(), ie = xx.lst.end();
           i != ie; ++i)
        apply_visitor(se, *i);
    }
  };

  // ------------------------------------------------------------------------------------------------------------------------
  // more complicated things

  // GET MARKLIST   REPLACE THIS WITH BETTER API
  struct markslist {
    const struct info_marklist l;
    markslist() : l(info_get_marks()) {}
    void getrules(boostspirit::symbols<parserule*>& conts, inscratch& xx,
                  parserule* p) const;
  };

// for marks..., assumes a string (the mark name) has already been parsed
#define MARKSTART boostspirit::ch_p('[')
#define MARKSTART_PLUS boostspirit::str_p("+[")
#define MARKSTART_MINUS boostspirit::str_p("-[")
#define MARKEND boostspirit::ch_p(']')

  struct markins {
    inscratch& xx;
    markins(inscratch& xx) : xx(xx) {}
    void operator()(const char x) const;
  };

  void markins::operator()(const char x) const {
    fomus_sval(xx.fom, fomus_par_markid, fomus_act_set, xx.nam.c_str());
    if (fomus_err()) {
      xx.err = true;
      DBG("FOMUS_ERR() WAS SET!");
    }
    if (xx.b) {
      fomus_sval(xx.fom, fomus_par_markval, fomus_act_add, xx.str.c_str());
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
    if (xx.b2) {
      insnumb(xx, fomus_par_markval, fomus_act_add);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
    fomus_act(xx.fom, xx.par,
              xx.act); // xx.par = fomus_par_marklist or
                       // fomus_par_region_marklist, xx.act = set, add or remove
    if (fomus_err()) {
      xx.err = true;
      DBG("FOMUS_ERR() WAS SET!");
    }
  }

  struct setstrval {
    std::string& str;
    const char* to;
    setstrval(std::string& str, const char* to) : str(str), to(to) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      str = to;
    }
  };

  // a list or sym...  ex. ([startslur- dotted] [startslur- dotted 3] [accent]
  // [staccato])
  // ---parses the rest of the mark after `[' and `sym'---
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      markresetaux, (2, (((inscratch&), xx), ((const char*), str))), -,
      boostspirit::eps_p[setstrval(xx.nam, str)][setconstval<bool>(xx.b, false)]
                        [setconstval<bool>(xx.b2, false)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      markstraux, (1, (((inscratch&), xx))), -,
      !(strmatch(xx.str, "],")[setconstval<bool>(xx.b, true)] >>
        listmatchdelim))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      marknumaux, (1, (((inscratch&), xx))), -,
      !(numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos,
                    ferr)[setconstval<bool>(xx.b2, true)] >>
        listmatchdelim))
  // rules following conts...
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      markstrnummatch, (2, (((inscratch&), xx), ((const char*), str))), -,
      markresetaux(xx, str) >>
          ((strmatch(xx.str, "],") >> listmatchdelim >>
            numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
            listmatchdelim)[setconstval<bool>(xx.b, true)]
                           [setconstval<bool>(xx.b2, true)] |
           (marknumaux(xx) >> markstraux(xx))) >>
          MARKEND[markins(xx)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      markstrmatch, (2, (((inscratch&), xx), ((const char*), str))), -,
      markresetaux(xx, str) >> markstraux(xx) >> MARKEND[markins(xx)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      marknummatch, (2, (((inscratch&), xx), ((const char*), str))), -,
      markresetaux(xx, str) >> marknumaux(xx) >> MARKEND[markins(xx)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      marknonematch, (2, (((inscratch&), xx), ((const char*), str))), -,
      markresetaux(xx, str) >> MARKEND[markins(xx)])

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      markmatch,
      (4, (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), conts),
           ((parserule&), rule), ((const fomus_action), theact))),
      -,
      commatch >> symmatch(conts, rule,
                           "],")[setconstval<fomus_action>(xx.act, theact)] >>
          listmatchdelim >> rule)
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      markmatchnopre,
      (3, (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), conts),
           ((parserule&), rule))),
      -, commatch >> symmatch(conts, rule, "],") >> listmatchdelim >> rule)
  // the big mark matcher... (must set xx.par and xx.act)
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      totmarkmatch,
      (4, (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), conts),
           ((parserule&), rule), ((const fomus_param), par))),
      -,
      boostspirit::eps_p[setconstval<fomus_param>(xx.par, par)] >>
          ((boostspirit::ch_p('[') >> markmatchnopre(xx, conts, rule)) |
           (boostspirit::str_p("+[") >>
            markmatch(xx, conts, rule, fomus_act_add)) |
           (boostspirit::str_p("-[") >>
            markmatch(xx, conts, rule, fomus_act_remove))))

  // conts are marks--rules are either str, strnum or num match parsers
  void markslist::getrules(boostspirit::symbols<parserule*>& conts,
                           inscratch& xx, parserule* p) const {
    for (info_mark *i = l.marks, *ie = l.marks + l.n; i < ie; ++i) {
      conts.add(i->name, p);
      // std::cout << i->name << std::endl;
      switch (i->type) {
      case module_none:
        *p = marknonematch(xx, i->name);
        break;
      case module_number:
        *p = marknummatch(xx, i->name);
        break;
      case module_string:
        *p = markstrmatch(xx, i->name);
        break;
      case module_stringnum:
        *p = markstrnummatch(xx, i->name);
        break;
      default:
        assert(false);
      }
      ++p;
    }
  }

  // brackets
  struct startregion { // followed by '{'
    inscratch& xx;
    startregion(inscratch& xx) : xx(xx) {}
    fint nextid() const { // get the next id for '{'
      if (xx.str.empty())
        return 0;
      bracketmap_it i = xx.brs.find(xx.str);
      if (i == xx.brs.end()) {
        xx.brs.insert(bracketmaptype(xx.str, ++xx.idcnt));
        return xx.idcnt;
      } else {
        return i->second;
      }
    }
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_ival(xx.fom, fomus_par_region, fomus_act_start,
                 nextid()); // id string is in str
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };

  // const string endbr_bad("Missing `{'");
  struct endbracket {
    inscratch& xx;
    endbracket(inscratch& xx) : xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      int e;
      if (xx.str.empty())
        e = 0;
      else {
        bracketmap_it i = xx.brs.find(xx.str);
        if (i == xx.brs.end()) {
          CERR << "missing `{'";
          throw_(s1, &xx.pos);
        }
        e = i->second;
        xx.brs.erase(i);
      }
      fomus_ival(xx.fom, fomus_par_region, fomus_act_end, e);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      startbrmatch, (1, (((std::string&), str))), -,
      (!strmatch(str, "{") >> boostspirit::ch_p('{')) |
          (boostspirit::str_p("..") >> !boostspirit::ch_p('.'))[clearstr(str)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(endbrmatch, (1, (((std::string&), str))), -,
                                  boostspirit::ch_p('}') >> !strmatch(str, "}"))

  BOOST_SPIRIT_RULE_PARSER(plusmatch, -, -, -, PLUSMATCH >> commatch)
  BOOST_SPIRIT_RULE_PARSER(minusmatch, -, -, -, MINUSMATCH >> commatch)
  BOOST_SPIRIT_RULE_PARSER(multmatch, -, -, -, MULTMATCH >> commatch)
  BOOST_SPIRIT_RULE_PARSER(divmatch, -, -, -, DIVMATCH >> commatch)

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(bracketstartnoent, (1, (((inscratch&), xx))),
                                  -, // must be a bracket
                                  startbrmatch(xx.str)[startregion(xx)])

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketstart,
      (2, (((inscratch&), xx), ((const enum fomus_param), regpar))),
      -, // xx.act is set -- enter number as a region num
      commatch >>
          startbrmatch(xx.str)[insnumbvalact(xx, regpar)][startregion(xx)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketpstart,
      (2, (((inscratch&), xx), ((const enum fomus_param), regpar))),
      -, // xx.act is set -- enter number as a region num
      commatch >>
          startbrmatch(
              xx.str)[insstrval2(xx, regpar, fomus_act_set)][startregion(xx)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketistart,
      (2, (((inscratch&), xx), ((const enum fomus_param), regpar))),
      -, // special for time/grtime incrementing
      commatch >>
          startbrmatch(
              xx.str)[insact(xx, regpar, fomus_act_inc)][startregion(xx)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketvstart,
      (2, (((inscratch&), xx), ((const enum fomus_action), act))),
      -, // special for voices
      commatch >> startbrmatch(xx.str)[insvlistval(xx, fomus_par_region_voice,
                                                   act)][startregion(xx)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketvstart0,
      (2, (((inscratch&), xx), ((const enum fomus_action), act))),
      -, // special for voices
      commatch >>
          startbrmatch(xx.str)[insact(xx, fomus_par_region_voicelist, act)]
                              [insnumbval(xx, fomus_par_region_voice,
                                          fomus_act_set)][startregion(xx)])

  // nosing = must be a bracket
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketstart_nosing,
      (3, (((inscratch&), xx), ((const enum fomus_param), regpar),
           ((const enum fomus_action), act))),
      -, // sing = no bracket is an option
      boostspirit::eps_p[setconstval<fomus_action>(xx.act, act)] >>
          bracketstart(xx, regpar))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketstart_vnosing,
      (2, (((inscratch&), xx), ((const enum fomus_action), act))),
      -, // sing = no bracket is an option
      boostspirit::eps_p[setconstval<fomus_action>(xx.act, act)] >>
          bracketvstart(xx, act))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketstart_vnosing0,
      (2, (((inscratch&), xx), ((const enum fomus_action), act))),
      -, // sing = no bracket is an option
      boostspirit::eps_p[setconstval<fomus_action>(xx.act, act)] >>
          bracketvstart0(xx, act))

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketstart_sing,
      (4,
       (((inscratch&), xx), ((const enum fomus_param), par),
        ((const enum fomus_param), regpar), ((const enum fomus_action), act))),
      -, // sing = no bracket is an option
      bracketstart_nosing(xx, regpar, act) |
          boostspirit::eps_p[insnumbval(xx, par, act)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketstart_psing,
      (3, (((inscratch&), xx), ((const enum fomus_param), regpar),
           ((const enum fomus_param), par))),
      -, // sing = no bracket is an option
      bracketpstart(xx, regpar) |
          boostspirit::eps_p[insstrval2(xx, par, fomus_act_set)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketstart_ising,
      (3, (((inscratch&), xx), ((const enum fomus_param), par),
           ((const enum fomus_param), regpar))),
      -, // sing = no bracket is an option
      bracketistart(xx, regpar) |
          boostspirit::eps_p[insact(xx, par, fomus_act_inc)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketstart_vsing,
      (2, (((inscratch&), xx), ((const enum fomus_action), act))),
      -, // sing = no bracket is an option
      bracketstart_vnosing(xx, act) |
          boostspirit::eps_p[insvlistval(xx, fomus_par_voice, act)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      bracketstart_vsing0,
      (2, (((inscratch&), xx), ((const enum fomus_action), act))),
      -, // sing = no bracket is an option
      bracketstart_vnosing0(xx, act) |
          boostspirit::eps_p[insnumbval(xx, fomus_par_voice, act)])

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setoff, (1, (((inscratch&), xx))), -, // assumes already found the `o'
      (plusmatch >> numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
       bracketstart_sing(xx, fomus_par_time, fomus_par_region_time,
                         fomus_act_inc)) |
          (minusmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_time, fomus_par_region_time,
                             fomus_act_dec)) |
          (multmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_nosing(xx, fomus_par_region_time, fomus_act_mult)) |
          (divmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_nosing(xx, fomus_par_region_time, fomus_act_div)) |
          (numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_time, fomus_par_region_time,
                             fomus_act_set)) |
          boostspirit::ch_p('+')[insact(xx, fomus_par_time, fomus_act_inc)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setgroff, (1, (((inscratch&), xx))), -,
      (plusmatch >> numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
       bracketstart_sing(xx, fomus_par_gracetime, fomus_par_region_gracetime,
                         fomus_act_inc)) |
          (minusmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_gracetime,
                             fomus_par_region_gracetime, fomus_act_dec)) |
          (multmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_nosing(xx, fomus_par_region_gracetime,
                               fomus_act_mult)) |
          (divmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_nosing(xx, fomus_par_region_gracetime, fomus_act_div)) |
          (numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_gracetime,
                             fomus_par_region_gracetime, fomus_act_set)) |
          boostspirit::ch_p(
              '+')[insact(xx, fomus_par_gracetime, fomus_act_inc)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setdur, (1, (((inscratch&), xx))), -,
      (plusmatch >> numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
       bracketstart_sing(xx, fomus_par_duration, fomus_par_region_duration,
                         fomus_act_inc)) |
          (minusmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_duration, fomus_par_region_duration,
                             fomus_act_dec)) |
          (multmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_duration, fomus_par_region_duration,
                             fomus_act_mult)) |
          (divmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_duration, fomus_par_region_duration,
                             fomus_act_div)) |
          (numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_duration, fomus_par_region_duration,
                             fomus_act_set)) |
          (boostspirit::ch_p(
              '+')[insact(xx, fomus_par_duration, fomus_act_inc)]) |
          (boostspirit::ch_p(
              '-')[insact(xx, fomus_par_duration, fomus_act_dec)]) |
          (strmatchspec(xx.nam, ";") >>
           bracketstart_psing(xx, fomus_par_region_duration,
                              fomus_par_duration)))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setdyn, (1, (((inscratch&), xx))), -,
      (plusmatch >> numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
       bracketstart_sing(xx, fomus_par_dynlevel, fomus_par_region_dynlevel,
                         fomus_act_inc)) |
          (minusmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_dynlevel, fomus_par_region_dynlevel,
                             fomus_act_dec)) |
          (multmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_dynlevel, fomus_par_region_dynlevel,
                             fomus_act_mult)) |
          (divmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_dynlevel, fomus_par_region_dynlevel,
                             fomus_act_div)) |
          (numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_dynlevel, fomus_par_region_dynlevel,
                             fomus_act_set)))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setpitch, (1, (((inscratch&), xx))), -,
      (plusmatch >> numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
       bracketstart_sing(xx, fomus_par_pitch, fomus_par_region_pitch,
                         fomus_act_inc)) |
          (minusmatch >>
           numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_pitch, fomus_par_region_pitch,
                             fomus_act_dec)) |
          (numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
           bracketstart_sing(xx, fomus_par_pitch, fomus_par_region_pitch,
                             fomus_act_set)) |
          (strmatchspec(xx.nam, ";") >>
           bracketstart_psing(xx, fomus_par_region_pitch, fomus_par_pitch)))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setvoice, (1, (((inscratch&), xx))), -,
      (plusmatch >>
       ((numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
         bracketstart_vsing0(xx, fomus_act_add)) |
        (listmatchnums1(xx.lst, xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos) >>
         bracketstart_vsing(xx, fomus_act_add)))) |
          (minusmatch >>
           ((numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
             bracketstart_vsing0(xx, fomus_act_remove)) |
            (listmatchnums1(xx.lst, xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos) >>
             bracketstart_vsing(xx, fomus_act_remove)))) |
          (((numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos, ferr) >>
             bracketstart_vsing0(xx, fomus_act_set)) |
            (listmatchnums1(xx.lst, xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos) >>
             bracketstart_vsing(xx, fomus_act_set)))))

#define MEASURE_START boostspirit::ch_p('|') // |3/4|
#define MEASURE_END boostspirit::ch_p('|')

  struct inspsym {
    inscratch& xx;
    inspsym(inscratch& xx) : xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      xx.psymsv.push_back(
          std::string(xx.str)); // need to save an uncopyable instance
      xx.psyms.add(xx.psymsv.back().c_str(), &xx.psymsv.back());
    }
  };

  // SYNCHRONIZE THESE W/ PARSEINS.H
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      structstr,
      (3, (((inscratch&), xx), ((const enum fomus_param), par),
           ((const fomus_action), act))),
      -, strmatch(xx.str, ")>,")[insstrval(xx, par, act)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mstructstr,
      (3, (((inscratch&), xx), ((const enum fomus_param), par),
           ((const fomus_action), act))),
      -, strmatch(xx.str, "|,")[insstrval(xx, par, act)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      structpartstr, (2, (((inscratch&), xx), ((const enum fomus_param), par))),
      -,
      strmatch(xx.str, ")>,")[insstrval(xx, par, fomus_act_set)][inspsym(xx)])

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      structmatch0,
      (9, (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), table1),
           ((boostspirit::symbols<parserule*>&), table2), ((parserule&), rest),
           ((const fomus_param), which), ((const fomus_action), act),
           ((const char*), open), ((const char*), close),
           ((const fomus_param), whichset))),
      -,
      boostspirit::chset_p(open) >> commatch >>
          *(if_p(boostspirit::chset_p(close))[boostspirit::nothing_p]
                .else_p[recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                                  xx.pos.maccnt) >>
                        (symmatch(table1, rest, ":=,") |
                         symmatcherr(table2, rest, ":=,", xx.str, xx.pos, ferr)
                             [setconstval<fomus_param>(xx.par, whichset)]) >>
                        eqldelmatch(xx.isplus) >> rest >> listmatchdelim]) >>
          recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                    xx.pos.maccnt)[dofilepos(xx)] >>
          boostspirit::anychar_p[insact(xx, which, act)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      structmatch,
      (7, (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), table1),
           ((boostspirit::symbols<parserule*>&), table2), ((parserule&), rest),
           ((const fomus_param), which), ((const fomus_action), act),
           ((const fomus_param), whichset))),
      -,
      structmatch0(xx, table1, table2, rest, which, act, "(<", ")>", whichset));

  struct blastit {
    inscratch& xx;
    const fomus_param par;
    blastit(inscratch& xx, const fomus_param par) : xx(xx), par(par) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_act(xx.fom, par, fomus_act_add);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
    void operator()(const char x) const {
      fomus_act(xx.fom, par, fomus_act_add);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mstructoridmatch,
      (7, (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), table1),
           ((boostspirit::symbols<parserule*>&), table2), ((parserule&), rest),
           ((const char*), d1), ((const char*), d2), ((const char*), d3))),
      -,
      ((boostspirit::chset_p(d1) >> commatch >> strmatch(xx.str, d3) >>
        commatch >>
        boostspirit::chset_p(
            d2))[insstrval(xx, fomus_par_meas_measdef, fomus_act_set)] |
       structmatch0(xx, table1, table2, rest, fomus_par_meas_measdef,
                    fomus_act_set, d1, d2,
                    fomus_par_measdef_settingval))[blastit(xx, fomus_par_meas)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      smstructoridmatch,
      (7, (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), table1),
           ((boostspirit::symbols<parserule*>&), table2), ((parserule&), rest),
           ((const char*), d1), ((const char*), d2), ((const char*), d3))),
      -,
      mstructoridmatch(xx, table1, table2, rest, d1, d2, d3) |
          boostspirit::eps_p[insact(xx, fomus_par_measevent, fomus_act_set)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      structoridmatch,
      (7, (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), table1),
           ((boostspirit::symbols<parserule*>&), table2), ((parserule&), rest),
           ((const fomus_param), which), ((const fomus_action), act),
           ((const fomus_param), whichset))),
      -,
      structmatch(xx, table1, table2, rest, which, act, whichset) |
          structstr(xx, which, act))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      structmatches, (1, (((parserule&), onematch))), -,
      if_p(LISTSTART >> commatch)[*if_p(LISTEND)[boostspirit::nothing_p]
                                       .else_p[onematch >> commatch] >>
                                  boostspirit::anychar_p]
          .else_p[onematch])

  // SYNCHRONIZE THESE W/ PARSEINS.H
  struct import_in {
    boostspirit::symbols<parserule*> conts;
    parserule rest; // temp placeholder
    import_in() {}
  };
  struct export_in {
    boostspirit::symbols<parserule*> conts;
    parserule rest;
    export_in() {}
  };

  // also used in fms.in
  struct clef_in {
    boostspirit::symbols<parserule*> conts;
    parserule rest;
    clef_in() {}
  };
  struct staves_in {
    boostspirit::symbols<parserule*> conts;
    clef_in clefs;
    parserule clefs1rest, clefsrest;
    parserule rest;
    staves_in(boostspirit::symbols<parserule*>& sets, inscratch& xx)
        : clefs1rest(structmatch(
              xx, clefs.conts, sets, clefs.rest, fomus_par_staff_clefs,
              fomus_act_add, fomus_par_clef_settingval)), // <-- insert the str
          clefsrest(structmatches(clefs1rest)) {
      conts.add("clefs", &clefsrest);
    }
  };
  struct percinstr_in {
    boostspirit::symbols<parserule*> conts;
    import_in imports;
    export_in exports;
    parserule templaterest, idrest, namerest, abbrrest, imports1rest,
        importsrest, exports1rest;
    parserule rest;
    percinstr_in(boostspirit::symbols<parserule*>& sets, inscratch& xx)
        : templaterest(
              structstr(xx, fomus_par_percinst_template, fomus_act_set)),
          idrest(structstr(xx, fomus_par_percinst_id, fomus_act_set)),
          imports1rest(structmatch(xx, imports.conts, sets, imports.rest,
                                   fomus_par_percinst_imports, fomus_act_add,
                                   fomus_par_import_settingval)),
          importsrest(structmatches(imports1rest)),
          exports1rest(structmatch(xx, exports.conts, sets, exports.rest,
                                   fomus_par_percinst_export, fomus_act_set,
                                   fomus_par_export_settingval)) {
      conts.add("template", &templaterest);
      conts.add("id", &idrest);
      conts.add("imports", &importsrest);
      conts.add("export", &exports1rest);
    }
  };
  struct percs_in {
    percinstr_in percs;
    parserule percs1rest, percsrest;
    percs_in(boostspirit::symbols<parserule*>& sets, inscratch& xx)
        : percs(sets, xx),
          percs1rest(structmatch(xx, percs.conts, sets, percs.rest,
                                 fomus_par_list_percinsts, fomus_act_add,
                                 fomus_par_percinst_settingval)),
          percsrest(structmatches(percs1rest)) {}
  };

  struct instr_in {
    boostspirit::symbols<parserule*> conts;
    staves_in staves;
    import_in imports;
    export_in exports;
    percinstr_in percs;
    parserule templaterest, idrest, namerest, abbrrest, staves1rest, stavesrest,
        imports1rest, importsrest, exports1rest, percs1rest, percsrest;
    parserule rest;
    instr_in(boostspirit::symbols<parserule*>& sets, inscratch& xx)
        : staves(sets, xx), percs(sets, xx),
          templaterest(structstr(xx, fomus_par_inst_template, fomus_act_set)),
          idrest(structstr(xx, fomus_par_inst_id, fomus_act_set)),
          staves1rest(structmatch(xx, staves.conts, sets, staves.rest,
                                  fomus_par_inst_staves, fomus_act_add,
                                  fomus_par_staff_settingval)),
          stavesrest(structmatches(staves1rest)),
          imports1rest(structmatch(xx, imports.conts, sets, imports.rest,
                                   fomus_par_inst_imports, fomus_act_add,
                                   fomus_par_import_settingval)),
          importsrest(structmatches(imports1rest)),
          exports1rest(structmatch(xx, exports.conts, sets, exports.rest,
                                   fomus_par_inst_export, fomus_act_set,
                                   fomus_par_export_settingval)),
          percs1rest(structoridmatch(xx, percs.conts, sets, percs.rest,
                                     fomus_par_inst_percinsts, fomus_act_add,
                                     fomus_par_percinst_settingval)),
          percsrest(structmatches(percs1rest)) {
      conts.add("template", &templaterest);
      conts.add("id", &idrest);
      conts.add("staves", &stavesrest);
      conts.add("imports", &importsrest);
      conts.add("export", &exports1rest);
      conts.add("percinsts", &percsrest);
    }
  };
  struct instrs_in { // for settings
    instr_in ins;
    parserule instrs1rest, instrsrest;
    instrs_in(boostspirit::symbols<parserule*>& sets, inscratch& xx)
        : ins(sets, xx),
          instrs1rest(structmatch(xx, ins.conts, sets, ins.rest,
                                  fomus_par_list_insts, fomus_act_add,
                                  fomus_par_inst_settingval)),
          instrsrest(structmatches(instrs1rest)) {}
  };

  struct part_in {
    boostspirit::symbols<parserule*> conts;
    instr_in ins;
    parserule idrest, namerest, abbrrest, instrrest;
    parserule rest;
    part_in(boostspirit::symbols<parserule*>& sets, inscratch& xx)
        : ins(sets, xx), idrest(structpartstr(xx, fomus_par_part_id)),
          instrrest(structoridmatch(xx, ins.conts, sets, ins.rest,
                                    fomus_par_part_inst, fomus_act_set,
                                    fomus_par_inst_settingval)) {
      conts.add("id", &idrest);
      conts.add("inst", &instrrest);
    }
  };

  struct mpart_in;
  struct newmpart {
    boostspirit::symbols<parserule*>& sets;
    inscratch& xx;
    std::auto_ptr<mpart_in>& ptr;
    newmpart(boostspirit::symbols<parserule*>& sets, inscratch& xx,
             std::auto_ptr<mpart_in>& ptr)
        : sets(sets), xx(xx), ptr(ptr) {}
    void operator()(const parse_it& s1, const parse_it& s2) const;
  };
  struct partmap_in { // will also be used for orchestral double parts
    boostspirit::symbols<parserule*> conts;
    part_in part;
    std::auto_ptr<mpart_in> mpart;
    parserule partrest, mpartrest;
    parserule rest;
    partmap_in(boostspirit::symbols<parserule*>& sets, inscratch& xx);
  };

  struct mpart_in {
    boostspirit::symbols<parserule*> conts;
    partmap_in pmp;
    parserule idrest, parts1rest, partsrest;
    parserule rest;
    mpart_in(boostspirit::symbols<parserule*>& sets, inscratch& xx)
        : pmp(sets, xx), idrest(structpartstr(xx, fomus_par_metapart_id)),
          parts1rest(boostspirit::eps_p[insact(xx, fomus_par_partmap,
                                               fomus_act_start)] >>
                     structoridmatch(xx, pmp.conts, sets, pmp.rest,
                                     fomus_par_metapart_partmaps, fomus_act_add,
                                     fomus_par_partmap_settingval)),
          partsrest(structmatches(parts1rest)) {
      conts.add("id", &idrest);
      conts.add("parts", &partsrest);
    }
  };
  partmap_in::partmap_in(boostspirit::symbols<parserule*>& sets, inscratch& xx)
      : part(sets, xx),
        partrest(structoridmatch(xx, part.conts, sets, part.rest,
                                 fomus_par_partmap_part, fomus_act_set,
                                 fomus_par_part_settingval)),
        mpartrest(boostspirit::eps_p[newmpart(sets, xx, mpart)] >>
                  structoridmatch(xx, mpart->conts, sets, mpart->rest,
                                  fomus_par_partmap_metapart, fomus_act_set,
                                  fomus_par_metapart_settingval)) {
    conts.add("part", &partrest);
    conts.add("metapart", &mpartrest);
  }
  inline void newmpart::operator()(const parse_it& s1,
                                   const parse_it& s2) const {
    if (!ptr.get())
      ptr.reset(new mpart_in(sets, xx));
  }

  struct measdef_in {
    boostspirit::symbols<parserule*> conts;
    parserule idrest;
    parserule rest;
    measdef_in(inscratch& xx)
        : idrest(structstr(xx, fomus_par_measdef_id, fomus_act_set)) {
      conts.add("id", &idrest);
    }
  };
  struct shmeasdef_in {
    boostspirit::symbols<parserule*> conts;
    parserule idrest;
    parserule rest;
    shmeasdef_in(inscratch& xx)
        : idrest(mstructstr(xx, fomus_par_measdef_id, fomus_act_set)) {
      conts.add("id", &idrest);
    }
  };

  std::string defstring("default");
  struct rulespackage {
    inscratch xx;

    boostspirit::symbols<parserule*> strconts;
    boostspirit::symbols<parserule*> evconts; // entry symbols/continuations
    boostspirit::symbols<parserule*> gsetconts;
    boostspirit::symbols<parserule*> lsetconts;
    boostspirit::symbols<parserule*> setconts;
    boostspirit::symbols<parserule*> markconts;
    boostspirit::symbols<parserule*> msetconts;
    boostspirit::symbols<parserule*> metconts;
    boostspirit::symbols<parserule*> macconts;
    boostspirit::symbols<std::string*> paconts;
    parserule rule;

    std::vector<parserule> gsetrules;
    std::vector<parserule> lsetrules;
    std::vector<parserule> setrules;
    std::vector<parserule> msetrules;
    std::vector<parserule> markrules;

    macrovect macros;

    boostspirit::guard<filepos*> theguard;

    instr_in istr; // for insts/parts section
    percinstr_in pistr;
    part_in pstr;
    mpart_in mpstr;
    measdef_in astr;
    shmeasdef_in shastr;

    instrs_in globistr; // only for global settings
    percs_in globpstr;

    parserule cont, ruleifok, ruleifrecov, curloop, rules;
    parserule voicerule, offrule, groffrule, durrule, dynrule, pitchrule,
        curpartrule;
    parserule includerule, defmacrorule;
    parserule partrule, mpartrule, pinstrrule, instrrule, measdef0rule,
        measdefrule, smeasdefrule;
    parserule isanoterule, isarestrule, isamarkrule;

    boost::ptr_map<std::string, macrodef> mcrs;

    rulespackage(FOMUS fom, const std::string& filename, indata& dat, bool& err,
                 const settingslist& sets);
  };

  struct settinglistinsts : public dosetting {
    const fomus_param par;
    settinglistinsts(inscratch& xx, const int id, const fomus_param par)
        : dosetting(xx, id), par(par) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_ival(xx.fom, fomus_par_setting, fomus_act_set, id);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
      fomus_act(xx.fom, par, fomus_act_start);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  struct dosettinglistinsts {
    inscratch& xx;
    const fomus_param par;
    dosettinglistinsts(inscratch& xx, const fomus_param par)
        : xx(xx), par(par) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_act(xx.fom, par, xx.isplus2 ? fomus_act_append : fomus_act_set);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setnumbparse, (2, (((inscratch&), xx), ((const int), id))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] >>
          numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos,
                      ferr)[dosettingnumb(xx, id)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setstrparse,
      (3, (((inscratch&), xx), ((const int), id), ((const char*), endstr))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] >>
          strmatch(xx.str, endstr)[dosettingstring(xx, id)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setnumbstrparse,
      (3, (((inscratch&), xx), ((const int), id), ((const char*), endstr))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] >>
          (numbermatch(xx.num, xx.pt1, xx.pt2, xx.pt3, xx.pos,
                       ferr)[dosettingnumb(xx, id)] |
           strmatch(xx.str, endstr)[dosettingstring(xx, id)]))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setlistnumsparse, (2, (((inscratch&), xx), ((const int), id))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] // isplus2 is set to isplus in
                                              // dofilepos()
          >> listmatchnums(xx.lst, xx.num, xx.pt1, xx.pt2, xx.pt3, xx.isplus,
                           xx.pos, ferr)[dosettinglist(xx, id)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setliststrsparse,
      (3, (((inscratch&), xx), ((const int), id), ((const char*), end))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)]
#ifndef NDEBUGOUT
                              [print<bool>("isplus", xx.isplus)]
#endif
          >>
          listmatchstrs(xx.lst, xx.str, xx.isplus, end)[dosettinglist(xx, id)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setmapnumsparse, (2, (((inscratch&), xx), ((const int), id))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] >>
          mapmatchnums(xx.lst, xx.nam, xx.num, xx.pt1, xx.pt2, xx.pt3,
                       xx.isplus, xx.pos, ferr)[dosettinglist(xx, id)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setmapstrsparse, (2, (((inscratch&), xx), ((const int), id))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] >>
          mapmatchstrs(xx.lst, xx.nam, xx.str,
                       xx.isplus)[dosettinglist(xx, id)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setlistnumslistparse, (2, (((inscratch&), xx), ((const int), id))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] >>
          listmatchlistsofnums(xx.autolst, xx.num, xx.pt1, xx.pt2, xx.pt3,
                               xx.isplus, xx.pos, xx.lst,
                               ferr)[dosettinglist(xx, id)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setliststringslistparse, (2, (((inscratch&), xx), ((const int), id))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] >>
          listmatchlistsofstrs(xx.autolst, xx.str, xx.isplus, xx.pos,
                               xx.lst)[dosettinglist(xx, id)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setmapnumslistparse, (2, (((inscratch&), xx), ((const int), id))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] >>
          mapmatchlistsofnums(xx.autolst, xx.nam, xx.num, xx.pt1, xx.pt2,
                              xx.pt3, xx.isplus, xx.pos, xx.lst,
                              ferr)[dosettinglist(xx, id)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setmapstringslistparse, (2, (((inscratch&), xx), ((const int), id))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] >>
          mapmatchlistsofstrs(xx.autolst, xx.nam, xx.str, xx.isplus,
                              xx.lst)[dosettinglist(xx, id)])

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setinstsparse,
      (3, (((inscratch&), xx), ((const int), id), ((rulespackage&), pack))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col, xx.pos.maccnt)[dofilepos(
          xx)][settinglistinsts(xx, id, fomus_par_list_insts)] >>
          structmatches(pack.globistr.instrsrest)[dosettinglistinsts(
              xx, fomus_par_settingval_insts)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setpercsparse,
      (3, (((inscratch&), xx), ((const int), id), ((rulespackage&), pack))), -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col, xx.pos.maccnt)[dofilepos(
          xx)][settinglistinsts(xx, id, fomus_par_list_percinsts)] >>
          structmatches(pack.globpstr.percsrest)[dosettinglistinsts(
              xx, fomus_par_settingval_percinsts)])

  struct parsecrapout {
    void operator()() const {
      assert(false);
    }
    void operator()(const parse_it& s1, const parse_it& s2) const {
      assert(false);
    }
  };

  void inscratch::getrule(
      parserule& rule, const struct info_setting& set, const char* endstr,
      rulespackage& pack) { // get/store a rule according to set.basetype---for
                            // global settings!! FIXME--fix for structures
    switch (set.basetype) { // xx.par must be set before executing one of these
                            // settings rules
    case module_number:
      rule = setnumbparse(*this, set.id);
      break;
    case module_bool:
    case module_string:
      rule = setstrparse(*this, set.id, endstr);
      break;
    case module_notesym:
      rule = setnumbstrparse(*this, set.id, endstr);
      break;

    case module_list_nums:
      rule = setlistnumsparse(*this, set.id);
      break;
    case module_list_strings:
      rule = setliststrsparse(*this, set.id, endstr);
      break;
    case module_list_numlists:
      rule = setlistnumslistparse(*this, set.id);
      break;
    case module_list_stringlists:
      rule = setliststringslistparse(*this, set.id);
      break;

    case module_symmap_nums:
      rule = setmapnumsparse(*this, set.id);
      break;
    case module_symmap_strings:
      rule = setmapstrsparse(*this, set.id);
      break;
    case module_symmap_numlists:
      rule = setmapnumslistparse(*this, set.id);
      break;
    case module_symmap_stringlists:
      rule = setmapstringslistparse(*this, set.id);
      break;

    case module_special: // list of insts, etc..
      switch (set.id) {
      case PERCINSTRS_ID:
        rule = setpercsparse(*this, set.id, pack);
        return;
      case INSTRS_ID:
        rule = setinstsparse(*this, set.id, pack);
        return;
      default:
        assert(false);
      }
    default:
      assert(false);
    }
  }

#define LOCALSET_START boostspirit::ch_p('<')
#define LOCALSET_END boostspirit::ch_p('>')
#define NOTE_ENTER boostspirit::chset_p(";,")

  struct warnnotattop {
    inscratch& xx;
    warnnotattop(inscratch& xx) : xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      if (!xx.attop) {
        CERR << "global setting must appear at top of file";
        xx.printerr();
        {
          xx.err = true;
          DBG("FOMUS_ERR() WAS SET!");
        }
      }
    }
  };

  struct includefile {
    const inscratch& xx;
    const std::string& thispath;
    includefile(const inscratch& xx, const std::string& thispath)
        : xx(xx), thispath(thispath) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_load(
          xx.fom,
          FS_COMPLETE(xx.str, boost::filesystem::path(thispath).parent_path())
              .FS_FILE_STRING()
              .c_str());
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(doinclude,
                                  (2, (((inscratch&), xx),
                                       ((const std::string&), thp))),
                                  -, strmatch(xx.str, "")[includefile(xx, thp)])

  // for macro
  struct setstrlist {
    std::vector<std::string>& list;
    std::string& str;

    setstrlist(std::vector<std::string>& list, std::string& str)
        : list(list), str(str) {}
    void operator()(const char x) const {
      list.push_back(str);
    }
    void operator()(const parse_it& s1, const parse_it& s2) const {
      list.push_back(str);
    }
  };

  const std::string macroargs_bad("wrong number of macro arguments");

  // copy of this is in vars.cc also
  struct catchinerr {
    typedef boostspirit::nil_t result_t;
    parserule& curloop;
    const parserule& setrule;
    bool& err;
    indata& dat;

    catchinerr(parserule& curloop, const parserule& setrule, bool& err,
               indata& dat)
        : curloop(curloop), setrule(setrule), err(err), dat(dat) {}
    boostspirit::error_status<>
    operator()(boostspirit::scanner<parse_it> const& s,
               const boostspirit::parser_error<filepos*, parse_it>& e) const;
  };

  boostspirit::error_status<> catchinerr::operator()(
      boostspirit::scanner<parse_it> const& s,
      const boostspirit::parser_error<filepos*, parse_it>& e) const {
    e.descriptor->printerr(ferr);
    curloop = setrule;
    s.first = e.where;
    {
      err = true;
      DBG("FOMUS_ERR() WAS SET!");
    }
    return boostspirit::error_status<>::retry;
  }

  BOOST_SPIRIT_RULE_PARSER(commatchmac, -, -, -,
                           +(boostspirit::space_p |
                             boostspirit::comment_p("//") |
                             boostspirit::comment_nest_p("/-", "-/")))
  struct setstrapp {
    std::string& str;
    std::string& str2;
    setstrapp(std::string& str, std::string& str2) : str(str), str2(str2) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      DBG("str2 = " << str2 << std::endl);
      str += str2;
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      dodefmacro,
      (4, (((rulespackage&), pkg), ((std::string&), name),
           ((listelvect&), args), ((std::string&), body))),
      -,
      (strmatch(name, "(") >> commatch >>
       !(listmatchstrs(args, pkg.xx.str, boolfalse, "),") >> commatch) >>
       strmatch(body, "") >> commatch)[defmacro(pkg, name, args, body)])

  struct istrue_f {
    typedef boostspirit::nil_t result_t;
    bool& var;
    istrue_f(bool& var) : var(var) {}
    template <typename ScannerT>
    std::ptrdiff_t operator()(ScannerT const& scan, result_t& result) const {
      DBG("istrue_f = " << var << std::endl);
      return var ? 0 : -1;
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      domacro, (2, (((inscratch&), xx), ((macrodef&), mac))), -,
      (((boostspirit::if_p(boostspirit::functor_parser<istrue_f>(istrue_f(
             mac.haszero)))[boostspirit::eps_p(boostspirit::chset_p(":=("))] >>
         eqlmatch(xx.isplus) >>
         listmatchstrs(mac.repls, xx.str, boolfalse,
                       "),")) // if macro accepts 0 args, then *must* have "=",
                              // ":" or "(" here
        | boostspirit::eps_p[clearlist<listel>(mac.repls)]) >>
       commatch)[exemacro(mac)])

  // copy of this is in vars.cc also
  struct setcurloop {
    parserule& rule;
    const parserule& setrule;
    setcurloop(parserule& rule, const parserule& setrule)
        : rule(rule), setrule(setrule) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      rule = setrule;
    }
  };
  struct esymbad {
    const std::string& str;
    filepos& pos;
    esymbad(const std::string& str, filepos& pos) : str(str), pos(pos) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      CERR << "invalid entry `" << str << '\'';
      throw_(s1, &pos);
    }
  };
  struct maybedice {
    inscratch& xx;
    maybedice(inscratch& xx) : xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_act(xx.fom, fomus_par_entry, fomus_act_queue);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  struct dice {
    inscratch& xx;
    dice(inscratch& xx) : xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_act(xx.fom, fomus_par_entry, fomus_act_resume);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };
  struct nodice {
    inscratch& xx;
    nodice(inscratch& xx) : xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      fomus_act(xx.fom, fomus_par_entry, fomus_act_cancel);
      if (fomus_err()) {
        xx.err = true;
        DBG("FOMUS_ERR() WAS SET!");
      }
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setpart,
      (2,
       (((inscratch&), xx), ((boostspirit::symbols<std::string*>&), paconts))),
      -,
      (boostspirit::as_lower_d[paconts][insparsedstrval1(xx)] >>
       boostspirit::eps_p(
           boostspirit::space_p | boostspirit::ch_p(';') |
           boostspirit::str_p("//") // make sure symbol is complete match!
           | boostspirit::str_p(
                 "/-")))[insparsedstrval2(xx, fomus_par_part, fomus_act_set)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      setpartstr,
      (2,
       (((inscratch&), xx), ((boostspirit::symbols<std::string*>&), paconts))),
      -,
      boostspirit::eps_p(~boostspirit::ch_p('<') & ~boostspirit::ch_p('(')) >>
          strmatch(xx.str, ";")[insstrval(xx, fomus_par_part, fomus_act_set)])
  struct istop_f {
    typedef boostspirit::nil_t result_t;
    bool& attop;
    istop_f(bool& attop) : attop(attop) {}
    template <typename ScannerT>
    std::ptrdiff_t operator()(ScannerT const& scan, result_t& result) const {
      return attop ? -1 : 0;
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      eventmatches,
      (12,
       (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), strconts),
        ((boostspirit::symbols<parserule*>&), evconts),
        ((boostspirit::symbols<parserule*>&), setconts),
        ((boostspirit::symbols<parserule*>&), lsetconts),
        ((boostspirit::symbols<parserule*>&), markconts),
        ((boostspirit::symbols<std::string*>&), paconts), ((parserule&), rule),
        ((boostspirit::guard<filepos*>&), theguard), ((parserule&), measrule),
        ((boostspirit::symbols<parserule*>&), metconts),
        ((boostspirit::symbols<parserule*>&), macconts))),
      -,
      recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                xx.pos.maccnt)[dofilepos(xx)] >>
#ifndef NDEBUGOUT
              ((symmatch(evconts, rule,
                         ":=0123456789+-*/.(")[printstr("ENTRY")] >>
                eqlmatch(xx.isplus) >> rule) // offset, dur, etc.
#else
              ((symmatch(evconts, rule, ":=0123456789+-*/.(") >>
                eqlmatch(xx.isplus) >> rule) // offset, dur, etc.
#endif
#ifndef NDEBUGOUT
               | (symmatch(strconts, rule, ":=(")[printstr("STRUCT")] >>
                  eqlmatch(xx.isplus) >>
                  recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                            xx.pos.maccnt)[dofilepos(xx)] >>
                  rule) // new toplevel structs
#else
               | (symmatch(strconts, rule, ":=(") >> eqlmatch(xx.isplus) >>
                  recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                            xx.pos.maccnt)[dofilepos(xx)] >>
                  rule) // new toplevel structs
#endif
               | NOTE_ENTER[blastit(xx, fomus_par_noteevent)] // enter note
               | ((boostspirit::eps_p[maybedice(xx)][setconstval<fomus_action>(
                       xx.act, fomus_act_add)] >>
                   +(totmarkmatch(xx, markconts, rule, fomus_par_region_mark) >>
                     commatch) >>
                   bracketstartnoent(xx)[dice(xx)]) |
                  (boostspirit::eps_p[nodice(xx)] >> boostspirit::nothing_p)) |
               (boostspirit::eps_p[setconstval<fomus_action>(xx.act,
                                                             fomus_act_add)] >>
                totmarkmatch(xx, markconts, rule,
                             fomus_par_mark)) // mark (if not defining a region)
               | endbrmatch(xx.str)[endbracket(xx)] // end bracket
#ifndef NDEBUGOUT
               | (boostspirit::functor_parser<istop_f>(istop_f(xx.attop)) >>
                  ((boostspirit::eps_p[printstr("LOCSET1")] >>
                    ((boostspirit::eps_p[maybedice(xx)] >>
                      +(totsetting(xx, lsetconts, rule,
                                   fomus_par_region_settingval, ":=+") >>
                        commatch) >>
                      bracketstartnoent(xx)[dice(xx)]) |
                     (boostspirit::eps_p[nodice(xx)] >>
                      boostspirit::nothing_p))) |
                   (boostspirit::eps_p[printstr("LOCSET2")] >>
                    totsetting(xx, lsetconts, rule, fomus_par_note_settingval,
                               ":=+")))) // (note) setting
#else
               |
               (boostspirit::functor_parser<istop_f>(istop_f(xx.attop)) >>
                (((boostspirit::eps_p[maybedice(xx)] >>
                   +(totsetting(xx, lsetconts, rule,
                                fomus_par_region_settingval, ":=+") >>
                     commatch) >>
                   bracketstartnoent(xx)[dice(xx)]) |
                  (boostspirit::eps_p[nodice(xx)] >> boostspirit::nothing_p)) |
                 totsetting(xx, lsetconts, rule, fomus_par_note_settingval,
                            ":=+"))) // (note) setting
#endif
               | measrule | setpart(xx, paconts) // should be the last one
               | (symmatch(macconts, rule, ":=(") >> rule)) // change part
#ifndef NDEBUGOUT
                  [setconstval<bool>(xx.attop, false)][printstr("*ENTRIES")]
#else
                  [setconstval<bool>(xx.attop, false)]
#endif
          | (symmatch(metconts, rule, "\"") >> commatch >>
             rule) // meta-instructions (include, define macro, etc.)
#ifndef NDEBUGOUT
          | (boostspirit::eps_p[printstr("GLOBSET")] >>
             totsetting(xx, setconts, rule, fomus_par_settingval,
                        ":=+")[warnnotattop(xx)])
#else
          | totsetting(xx, setconts, rule, fomus_par_settingval,
                       ":=+")[warnnotattop(xx)]
#endif
  )

#ifndef NDEBUG
  struct nomatchcrapout {
    template <typename A, typename B>
    void operator()(const A& s1, const B& s2) const {
      assert(false);
    }
  };
#endif

  // rules for settings--TOP of file
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      oksymrule,
      (12,
       (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), strconts),
        ((boostspirit::symbols<parserule*>&), evconts),
        ((boostspirit::symbols<parserule*>&), setconts),
        ((boostspirit::symbols<parserule*>&), lsetconts),
        ((boostspirit::symbols<parserule*>&), markconts),
        ((boostspirit::symbols<std::string*>&), paconts), ((parserule&), rule),
        ((boostspirit::guard<filepos*>&), theguard), ((parserule&), measrule),
        ((boostspirit::symbols<parserule*>&), metconts),
        ((boostspirit::symbols<parserule*>&), macconts))),
      -,
      (eventmatches(xx, strconts, evconts, setconts, lsetconts, markconts,
                    paconts, rule, theguard, measrule, metconts, macconts) |
       (strmatch(xx.str, "") | boostspirit::eps_p)[esymbad(xx.str, xx.pos)]) >>
          commatch)
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      recovsymrule,
      (14,
       (((inscratch&), xx), ((boostspirit::symbols<parserule*>&), strconts),
        ((boostspirit::symbols<parserule*>&), evconts),
        ((boostspirit::symbols<parserule*>&), setconts),
        ((boostspirit::symbols<parserule*>&), lsetconts),
        ((boostspirit::symbols<parserule*>&), markconts),
        ((boostspirit::symbols<std::string*>&), paconts), ((parserule&), rule),
        ((boostspirit::guard<filepos*>&), theguard), ((parserule&), curloop),
        ((parserule&), okrule), ((parserule&), measrule),
        ((boostspirit::symbols<parserule*>&), metconts),
        ((boostspirit::symbols<parserule*>&), macconts))),
      -,
      (
#ifndef NDEBUG
          ((strmatch(xx.str, "") | boostspirit::anychar_p)
#ifndef NDEBUGOUT
               [printstr("GARBAGE STRING")]
#endif
           | boostspirit::eps_p[nomatchcrapout()])
#else
          (strmatch(xx.str, "") | boostspirit::anychar_p)
#endif
          >> commatch >>
          !(eventmatches(xx, strconts, evconts, setconts, lsetconts, markconts,
                         paconts, rule, theguard, measrule, metconts,
                         macconts) >>
            commatch)[setcurloop(curloop, okrule)]))

  // mainrule
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mainrule,
      (6, (((boostspirit::guard<filepos*>&), theguard), ((parserule&), curloop),
           ((parserule&), recovrule), ((const std::string&), filename),
           ((bool&), err), ((indata & dat), data))),
      -,
      !boostspirit::str_p("\xef\xbb\xbf") >> commatch >>
          *(theguard(
              if_p(boostspirit::end_p)[boostspirit::nothing_p]
                  .else_p[curloop])[catchinerr(curloop, recovrule, err, data)]))

  rulespackage::rulespackage(FOMUS fom, const std::string& filename,
                             indata& dat, bool& err, const settingslist& sets)
      : xx(fom, filename, paconts, err), gsetrules(0), lsetrules(0),
        setrules(0), msetrules(0), markrules(0), istr(setconts, xx),
        pistr(setconts, xx), pstr(setconts, xx), mpstr(setconts, xx), astr(xx),
        shastr(xx), globistr(setconts, xx), globpstr(setconts, xx),
        ruleifok(oksymrule(xx, strconts, evconts, gsetconts, lsetconts,
                           markconts, paconts, rule, theguard, measdefrule,
                           metconts, macconts)),
        ruleifrecov(recovsymrule(xx, strconts, evconts, gsetconts, lsetconts,
                                 markconts, paconts, rule, theguard, curloop,
                                 ruleifok, measdefrule, metconts, macconts)),
        curloop(ruleifok),
        rules(mainrule(theguard, curloop, ruleifrecov, filename, err, dat)),

        voicerule(setvoice(xx)), offrule(setoff(xx)), groffrule(setgroff(xx)),
        durrule(setdur(xx)), dynrule(setdyn(xx)), pitchrule(setpitch(xx)),
        curpartrule(setpartstr(xx, paconts)),
        includerule(doinclude(xx, filename)),
        defmacrorule(dodefmacro(*this, xx.nam, xx.lst, xx.str)),

        partrule(structmatch(xx, pstr.conts, setconts, pstr.rest,
                             fomus_par_part, fomus_act_add,
                             fomus_par_part_settingval)),
        mpartrule(structmatch(xx, mpstr.conts, setconts, mpstr.rest,
                              fomus_par_metapart, fomus_act_add,
                              fomus_par_metapart_settingval)),
        pinstrrule(structmatch(xx, pistr.conts, setconts, pistr.rest,
                               fomus_par_percinst, fomus_act_add,
                               fomus_par_percinst_settingval)),
        instrrule(structmatch(xx, istr.conts, setconts, istr.rest,
                              fomus_par_inst, fomus_act_add,
                              fomus_par_inst_settingval)),
        measdef0rule(structmatch(
            xx, astr.conts, setconts, astr.rest, fomus_par_measdef,
            fomus_act_add,
            fomus_par_measdef_settingval)), // the same way as the other
                                            // structs: meas (...)
        measdefrule(mstructoridmatch(
            xx, shastr.conts, msetconts, shastr.rest, "|", "|",
            "|,:=")), // actually a measure w/ an optional measdef
        smeasdefrule(smstructoridmatch(
            xx, astr.conts, setconts, astr.rest, "(<", ")>",
            ")>,:=")), // actually a measure w/ an optional measdef
        isanoterule(
            boostspirit::eps_p[insact(xx, fomus_par_noteevent, fomus_act_set)]),
        isarestrule(
            boostspirit::eps_p[insact(xx, fomus_par_restevent, fomus_act_set)]),
        isamarkrule(boostspirit::eps_p[insact(xx, fomus_par_markevent,
                                              fomus_act_set)]) {
    lsetrules.resize(sets.l.n);
    sets.getrules(lsetconts, xx, &lsetrules[0], ";", *this);

    gsetrules.resize(sets.l.n);
    sets.getrules(gsetconts, xx, &gsetrules[0], "", *this);

    setrules.resize(sets.l.n);
    sets.getrules(setconts, xx, &setrules[0], ")>,", *this);

    msetrules.resize(sets.l.n);
    sets.getrules(msetconts, xx, &msetrules[0], "|,", *this);
    // event rules
    evconts.add("time", &offrule);
    evconts.add("tim", &offrule);
    evconts.add("ti", &offrule);
    evconts.add("t", &offrule);
    evconts.add("grace", &groffrule);
    evconts.add("gra", &groffrule);
    evconts.add("gr", &groffrule);
    evconts.add("g", &groffrule);
    evconts.add("duration", &durrule);
    evconts.add("dur", &durrule);
    evconts.add("du", &durrule);
    evconts.add("d", &durrule);
    evconts.add("pitch", &pitchrule);
    evconts.add("pit", &pitchrule);
    evconts.add("pi", &pitchrule);
    evconts.add("p", &pitchrule);
    evconts.add("dynamic", &dynrule);
    evconts.add("dyn", &dynrule);
    evconts.add("dy", &dynrule);
    evconts.add("y", &dynrule);
    evconts.add("voice", &voicerule);
    evconts.add("voi", &voicerule);
    evconts.add("vo", &voicerule);
    evconts.add("v", &voicerule);
    evconts.add("part", &curpartrule); ///////////////////
    evconts.add("par", &curpartrule);
    evconts.add("pa", &curpartrule);
    evconts.add("a", &curpartrule);
    strconts.add("measdef", &measdef0rule);
    strconts.add("part", &partrule);
    strconts.add("metapart", &mpartrule);
    strconts.add("percinst", &pinstrrule);
    strconts.add("inst", &instrrule);
    strconts.add("measure", &smeasdefrule);
    strconts.add("meas", &smeasdefrule);
    strconts.add("mea", &smeasdefrule);
    strconts.add("me", &smeasdefrule);
    strconts.add("e", &smeasdefrule);
    strconts.add("note", &isanoterule);
    strconts.add("not", &isanoterule);
    strconts.add("no", &isanoterule);
    strconts.add("n", &isanoterule);
    strconts.add("mark", &isamarkrule);
    strconts.add("mar", &isamarkrule);
    strconts.add("ma", &isamarkrule);
    strconts.add("m", &isamarkrule);
    strconts.add("rest", &isarestrule);
    strconts.add("res", &isarestrule);
    strconts.add("re", &isarestrule);
    strconts.add("r", &isarestrule);
    metconts.add("include", &includerule);
    metconts.add("macro", &defmacrorule);
    markslist mks;
    markrules.resize(mks.l.n);
    mks.getrules(markconts, xx, &markrules[0]);
    paconts.add(defstring.c_str(), &defstring);
  }

  inline void exemacro::operator()(const parse_it& s1,
                                   const parse_it& s2) const {
    mac.execute(s1);
  }
  inline macrodef::macrodef(rulespackage& pkg, const std::string& name)
      : pkg(pkg), rule(domacro(pkg.xx, *this)), name(name), haszero(false) {}

  struct argrepl {
    std::string arg;
    std::string repl;
    argrepl(const std::string& arg, const std::string& repl)
        : arg('@' + arg), repl(repl) {}
  };
  inline bool operator<(const argrepl& x, const argrepl& y) {
    return x.arg.length() > y.arg.length();
  }
  struct scopedcnt {
    int& x;
    scopedcnt(int& x) : x(x) {
      ++x;
    }
    ~scopedcnt() {
      --x;
    }
  };

  void defmacro::operator()(const parse_it& s1, const parse_it& s2) const {
    boost::ptr_map<std::string, macrodef>::iterator md(pkg.mcrs.find(name));
    if (md == pkg.mcrs.end())
      md = pkg.mcrs.insert(name, new macrodef(pkg, name)).first;
    DBG("body = \"" << body << '"' << std::endl);
    md->second->insert(args, body);
    pkg.macconts.add(md->second->name.c_str(), &md->second->rule);
  }

  void macrodef::execute(const parse_it& s1) {
    boost::ptr_map<int, macroinfo>::const_iterator b(bodies.find(repls.size()));
    if (b == bodies.end()) {
      DBG("repls.size() = " << repls.size() << std::endl);
      CERR << "invalid number of macro arguments";
      throw_(s1, &pkg.xx.pos);
    }
    assert(repls.size() == b->second->args.size());
    std::string exp(b->second->body);
    boost::ptr_vector<argrepl> argpairs;
    listelvect_constit j(repls.begin());
    for (std::vector<std::string>::const_iterator i(b->second->args.begin()),
         e(b->second->args.end());
         i != e; ++i, ++j)
      argpairs.push_back(new argrepl(*i, listel_getstring(*j)));
    argpairs.sort();
    for (boost::ptr_vector<argrepl>::reverse_iterator i(argpairs.rbegin());
         i != argpairs.rend(); ++i) {
      size_t p1 = 0;
      while (true) {
        DBG("looking for '" << i->arg << "'" << std::endl);
        p1 = exp.find(i->arg, p1);
        if (p1 == std::string::npos)
          break;
        else
          exp.replace(p1++, i->arg.length(), i->repl);
      }
    }
    assert(pkg.xx.pos.maccnt >= 0);
    scopedcnt zzz(pkg.xx.pos.maccnt);
    if (pkg.xx.pos.maccnt >= 100) {
      CERR << "too many nested macros";
      throw_(s1, &pkg.xx.pos);
    }
    assert(pkg.xx.pos.maccnt > 0);
    parse_it p(exp.c_str(), exp.c_str() + exp.size());
    parse(p, parse_it(), pkg.rules); // shouldn't throw
  }
} // namespace fmsin

using namespace fmsin;

const char* modin_get_extension(int n) {
  switch (n) {
  case 0:
    return "fms";
  default:
    return 0;
  }
}
const char* modin_get_loadid() {
  return 0;
}

// PARSE THE FILE
bool indata::modin_load(
    FOMUS fom, const char* filename,
    const bool isfile) { // no args are passed to file loaders
  bool err = false;
  if (isfile) {
    try {
      boost::filesystem::path fn(filename);
      boost::filesystem::ifstream f;
      try {
        f.exceptions(boost::filesystem::ifstream::eofbit |
                     boost::filesystem::ifstream::failbit |
                     boost::filesystem::ifstream::badbit);
        f.open(fn.FS_FILE_STRING(), boost::filesystem::ifstream::in |
                                        boost::filesystem::ifstream::ate |
                                        boost::filesystem::ifstream::binary);
        long l = f.tellg();
        f.seekg(0);
        std::vector<char> buf(l + 1);
        f.read(&buf[0], l);
        buf[l] = 0;
        parse_it p(&buf[0], &buf[l]);
        settingslist sts(fom); // need to keep this for recursive parts
        rulespackage pkg(fom, stdtostr(fn.FS_FILE_STRING()), *this, err, sts);
        p.set_tabchars(tabchars);
        p.set_position(boostspirit::file_position_base<std::string>(
            stdtostr(fn.FS_FILE_STRING())));
        fomus_sval(fom, fomus_par_locfile, fomus_act_set,
                   fn.FS_FILE_STRING().c_str());
        if (fomus_err()) {
          err = true;
          DBG("FOMUS_ERR() WAS SET!");
        }
        parse(p, parse_it(), pkg.rules);
        f.close();
      } catch (const std::ifstream::failure& e) {
        CERR << "error reading `" << fn.FS_FILE_STRING() << '\'' << std::endl;
        return true;
      }
#ifndef NDEBUG
      catch (const boostspirit::parser_error<const std::string, parse_it>& e) {
        assert(false);
      }
#endif
    } catch (const boost::filesystem::filesystem_error& e) {
      CERR << "invalid path/filename `" << filename << '\'' << std::endl;
      return true;
    }
  } else {
    parse_it p(filename, filename + strlen(filename));
    settingslist sts(fom); // need to keep this for recursive parts
    rulespackage pkg(fom, /*stdtostr(fn.FS_FILE_STRING())*/ "", *this, err,
                     sts);
    p.set_tabchars(tabchars);
    p.set_position(boostspirit::file_position_base<std::string>(""));
    fomus_sval(fom, fomus_par_locfile, fomus_act_set, "");
    if (fomus_err()) {
      err = true;
      DBG("FOMUS_ERR() WAS SET!");
    }
    parse(p, parse_it(), pkg.rules);
  }
  return err;
}

int modin_load(FOMUS fom, void* dat, const char* filename, int isfile) {
  return ((indata*) dat)->modin_load(fom, filename, isfile);
}

void* module_newdata(FOMUS f) {
  return new indata(f);
}
void module_freedata(void* dat) {
  delete (indata*) dat;
}
const char* module_err(void* dat) {
  return 0;
}
const char* module_initerr() {
  return 0;
}

// int module_priority() {return 0;}
enum module_type module_type() {
  return module_modinput;
}

void module_init() {}
void module_free() {}

const char* tabchartype = "integer1..20";
int valid_tabchars(const struct module_value val) {
  return module_valid_int(val, 1, module_incl, 20, module_incl, 0, tabchartype);
}

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "fmsin-tabchars"; // docscat{fmsin}
    set->type = module_int;
    set->descdoc =
        "Numbers of spaces occupied by a tab character (used to report correct "
        "column numbers in input file error messages).";
    set->typedoc = tabchartype;

    module_setval_int(&set->val, 1);

    set->loc = module_locscore;
    set->valid = valid_tabchars;
    set->uselevel = 2;
    tabcharsid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}
void module_ready() {}

const char* module_longname() {
  return "FOMUS File Input";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Reads in `.fms' files (FOMUS's native file format).";
}
