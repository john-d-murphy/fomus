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

#ifndef FOMUS_PARSEINS_H
#define FOMUS_PARSEINS_H

#ifndef BUILD_LIBFOMUS
#error "parseins.h shouldn't be included"
#endif

#include "heads.h"
#include "instrs.h" // instr structs
#include "parse.h"
#include "userstrs.h" // strtoclef
#include "vars.h"     // confscratch

namespace fomus {

  // ***** SYNCHRONIZE THESE W/ FMSIN.H
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      structmatch,
      (4, (((const boostspirit::symbols<parserule*>&), table1),
           ((const boostspirit::symbols<parserule*>&), table2),
           ((parserule&), rest), ((confscratch&), xx))),
      -,
      boostspirit::chset_p("(<") >> commatch >>
          *(if_p(boostspirit::chset_p(")>"))[boostspirit::nothing_p]
                .else_p[recerrpos(xx.pos.file, xx.pos.line, xx.pos.col,
                                  xx.pos.modif) >>
                        (symmatch(table1, rest, ":=,") |
                         symmatcherr(table2, rest, "=:,", xx.str, xx.pos,
                                     ferr)) >>
                        eqldelmatch(xx.isplus) >> rest >> listmatchdelim]) >>
          recerrpos(xx.pos.file, xx.pos.line, xx.pos.col, xx.pos.modif) >>
          boostspirit::anychar_p) // MUST SET NAME!
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      structmatches, (1, (((parserule&), onematch))), -,
      if_p(LISTSTART >>
           commatch /*>> boostspirit::eps_p(boostspirit::chset_p("()<"))*/)
          [*if_p(LISTEND)[boostspirit::nothing_p]
                .else_p[onematch >> commatch] >>
           boostspirit::anychar_p]
              .else_p[onematch]) // MUST SET NAME!

  template <typename S>
  struct insstrval {
    boost::shared_ptr<S>& str;
    void (S::*fun)(const std::string&);
    confscratch& xx;
    insstrval(boost::shared_ptr<S>& str, void (S::*fun)(const std::string&),
              confscratch& xx)
        : str(str), fun(fun), xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      (str.get()->*fun)(xx.str);
    }
  };
  template <typename S>
  struct insclefstrval {
    boost::shared_ptr<S>& str;
    void (S::*fun)(const int x);
    confscratch& xx;
    insclefstrval(boost::shared_ptr<S>& str, void (S::*fun)(const int x),
                  confscratch& xx)
        : str(str), fun(fun), xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      (str.get()->*fun)(strtoclef(xx.str));
    }
  };

  template <typename S>
  struct insintval {
    boost::shared_ptr<S>& str;
    void (S::*fun)(const fint);
    confscratch& xx;
    insintval(boost::shared_ptr<S>& str, void (S::*fun)(const fint),
              confscratch& xx)
        : str(str), fun(fun), xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      switch (xx.num.type()) {
      case module_int:
        (str.get()->*fun)(xx.num.geti());
        break;
      case module_float:
        (str.get()->*fun)(lround(xx.num.getf()));
        break;
      case module_rat:
        (str.get()->*fun)(rattoint(xx.num.getr()));
        break;
      default:
        assert(false);
      }
    }
  };
  template <typename S>
  struct insnumbval {
    boost::shared_ptr<S>& str;
    void (S::*fun)(const numb&);
    confscratch& xx;
    insnumbval(boost::shared_ptr<S>& str, void (S::*fun)(const numb&),
               confscratch& xx)
        : str(str), fun(fun), xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      (str.get()->*fun)(xx.num);
    }
  };
  template <typename S>
  struct insboolval {
    boost::shared_ptr<S>& str;
    void (S::*fun)(const bool);
    confscratch& xx;
    insboolval(boost::shared_ptr<S>& str, void (S::*fun)(const bool),
               confscratch& xx)
        : str(str), fun(fun), xx(xx) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      switch (xx.num.type()) {
      case module_int:
        (str.get()->*fun)(xx.num.geti());
        break;
      case module_float:
        (str.get()->*fun)(lround(xx.num.getf()));
        break;
      case module_rat:
        (str.get()->*fun)(rattoint(xx.num.getr()));
        break;
      default:
        assert(false);
      }
    }
  };

  template <typename S1, typename S2>
  struct insstruct {
    boost::shared_ptr<S1>& str1;
    boost::shared_ptr<S2>& str2;
    void (S1::*fun)(boost::shared_ptr<S2>&);
    filepos& pos;
    insstruct(boost::shared_ptr<S1>& str1,
              void (S1::*fun)(boost::shared_ptr<S2>&),
              boost::shared_ptr<S2>& str2, filepos& pos)
        : str1(str1), str2(str2), fun(fun), pos(pos) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      str2->complete(pos);
      (str1.get()->*fun)(str2);
    }
  };

  // THE RULES
  struct import_rules _NONCOPYABLE {
    boost::shared_ptr<import_str> str;
    boostspirit::symbols<parserule*> conts;
    boostspirit::symbols<parserule*> importsyms;
    std::vector<parserule> importrules; // don't put in vector<>
    parserule rest;
    import_rules(confscratch& xx) : str(new import_str(/*0*/)) {
      import_addstrconfrule(str, importsyms, importrules, xx);
    }
  };
  struct export_rules _NONCOPYABLE {
    boost::shared_ptr<export_str> str;
    boostspirit::symbols<parserule*> conts;
    boostspirit::symbols<parserule*> exportsyms;
    std::vector<parserule> exportrules; // don't put in vector<>
    parserule rest;
    export_rules(confscratch& xx) : str(new export_str(/*0*/)) {
      export_addstrconfrule(str, exportsyms, exportrules, xx);
    }
  };

  // also used in fms.in
  struct clef_rules _NONCOPYABLE {
    boost::shared_ptr<clef_str> str;
    boostspirit::symbols<parserule*> conts;
    boostspirit::symbols<parserule*> clefsyms;
    std::vector<parserule> clefrules; // don't put in vector<>
    parserule rest;
    clef_rules(confscratch& xx) : str(new clef_str(/*0*/)) {
      clef_addstrconfrule(str, clefsyms, clefrules, xx);
    }
  };
  struct staves_rules _NONCOPYABLE {
    boost::shared_ptr<staff_str> str;
    boostspirit::symbols<parserule*> conts;
    boostspirit::symbols<parserule*> staffsyms;
    std::vector<parserule> staffrules;
    clef_rules clefs;
    parserule clefs1rest, clefsrest;
    parserule rest;
    staves_rules(confscratch& xx)
        : str(new staff_str()), clefs(xx),
          clefs1rest(structmatch(clefs.conts, clefs.clefsyms, clefs.rest,
                                 xx)[insstruct<staff_str, clef_str>(
              str, &staff_str::insclef, clefs.str,
              xx.pos)]), // <-- insert the str
          clefsrest(structmatches(clefs1rest)) {
      conts.add("clefs", &clefsrest);
      staff_addstrconfrule(str, staffsyms, staffrules, xx);
    }
  };
  struct percinstr_rules _NONCOPYABLE {
    boost::shared_ptr<percinstr_str> str;
    boostspirit::symbols<parserule*> conts;
    boostspirit::symbols<parserule*> percinstrsyms;
    std::vector<parserule> percinstrrules;
    import_rules imports;
    export_rules exports;
    parserule templaterest, idrest, namerest, abbrrest, imports1rest,
        importsrest, exports1rest;
    parserule rest;
    percinstr_rules(confscratch& xx)
        : str(new percinstr_str()), imports(xx), exports(xx),
          templaterest(strmatch(xx.str, ")>,")[insstrval<percinstr_str>(
              str, &percinstr_str::setbasedon, xx)]),
          idrest(strmatch(
              xx.str,
              ")>,")[insstrval<percinstr_str>(str, &percinstr_str::setid, xx)]),
          imports1rest(structmatch(imports.conts, imports.importsyms,
                                   imports.rest,
                                   xx)[insstruct<percinstr_str, import_str>(
              str, &percinstr_str::insimport, imports.str, xx.pos)]),
          importsrest(structmatches(imports1rest)),
          exports1rest(structmatch(exports.conts, exports.exportsyms,
                                   exports.rest,
                                   xx)[insstruct<percinstr_str, export_str>(
              str, &percinstr_str::setexport, exports.str, xx.pos)]) {
      conts.add("template", &templaterest);
      conts.add("id", &idrest);
      conts.add("imports", &importsrest);
      conts.add("export", &exports1rest);
      percinstr_addstrconfrule(str, percinstrsyms, percinstrrules, xx);
    }
  };
  struct instr_rules _NONCOPYABLE {
    boost::shared_ptr<instr_str> str;
    boostspirit::symbols<parserule*> conts;
    boostspirit::symbols<parserule*> instrsyms;
    std::vector<parserule> instrrules;
    staves_rules staves;
    import_rules imports;
    export_rules exports;
    percinstr_rules percs;
    parserule templaterest, idrest, namerest, abbrrest, staves1rest, stavesrest,
        imports1rest, importsrest, exports1rest, percs1rest, percsrest;
    parserule rest;
    instr_rules(confscratch& xx)
        : str(new instr_str()), staves(xx), imports(xx), exports(xx), percs(xx),
          templaterest(strmatch(
              xx.str,
              ")>,")[insstrval<instr_str>(str, &instr_str::setbasedon, xx)]),
          idrest(strmatch(
              xx.str, ")>,")[insstrval<instr_str>(str, &instr_str::setid, xx)]),
          staves1rest(structmatch(staves.conts, staves.staffsyms, staves.rest,
                                  xx)[insstruct<instr_str, staff_str>(
              str, &instr_str::insstaff, staves.str, xx.pos)]),
          stavesrest(structmatches(staves1rest)),
          imports1rest(structmatch(imports.conts, imports.importsyms,
                                   imports.rest,
                                   xx)[insstruct<instr_str, import_str>(
              str, &instr_str::insimport, imports.str, xx.pos)]),
          importsrest(structmatches(imports1rest)),
          exports1rest(structmatch(exports.conts, exports.exportsyms,
                                   exports.rest,
                                   xx)[insstruct<instr_str, export_str>(
              str, &instr_str::setexport, exports.str, xx.pos)]),
          percs1rest(strmatch(xx.str, ")>,")[insstrval<instr_str>(
                         str, &instr_str::inspercinstrid, xx)] |
                     structmatch(percs.conts, percs.percinstrsyms, percs.rest,
                                 xx)[insstruct<instr_str, percinstr_str>(
                         str, &instr_str::inspercinstrstr, percs.str, xx.pos)]),
          percsrest(structmatches(percs1rest)) {
      conts.add("template", &templaterest);
      conts.add("id", &idrest);
      conts.add("staves", &stavesrest);
      conts.add("imports", &importsrest);
      conts.add("export", &exports1rest);
      conts.add("percinsts", &percsrest);
      instr_addstrconfrule(str, instrsyms, instrrules, xx);
    }
  };

  template <typename S1>
  struct instopstruct {
    typedef std::map<const std::string, boost::shared_ptr<S1>, isiless>
        mapworkaround;
    mapworkaround& vect;
    boost::shared_ptr<S1>& str1;
    filepos& pos;
    instopstruct(mapworkaround& vect, boost::shared_ptr<S1>& str1, filepos& pos)
        : vect(vect), str1(str1), pos(pos) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      if (str1->getid().empty()) {
        CERR << str1->structtypestr() << " id missing";
        throw_(s1, &pos);
      }
      assert(str1->isvalid());
      // if (vect.find(str1->getid()) != vect.end()) {
      // 	CERR << "duplicate " << str1->structtypestr() << " id `" <<
      // str1->getid() << '\''; 	throw_(s1, &pos);
      // }
      vect.erase(str1->getid());
      vect.insert(typename mapworkaround::value_type(str1->getid(), str1));
      str1.reset(new S1());
    }
  };

  struct percs_rules _NONCOPYABLE {
    std::map<const std::string, boost::shared_ptr<percinstr_str>, isiless>&
        percsvect;
    percinstr_rules percs;
    parserule percs1rest, percsrest;
    percs_rules(confscratch& xx,
                std::map<const std::string, boost::shared_ptr<percinstr_str>,
                         isiless>& percsvect,
                filepos& spos)
        : percsvect(percsvect), percs(xx),
          percs1rest(structmatch(
              percs.conts, percs.percinstrsyms, percs.rest,
              xx)[instopstruct<percinstr_str>(percsvect, percs.str, spos)]),
          percsrest(structmatches(percs1rest)) {}
  };
  struct instrs_rules _NONCOPYABLE {
    std::map<const std::string, boost::shared_ptr<instr_str>, isiless>&
        instsvect;
    instr_rules insts;
    parserule insts1rest, instsrest;
    instrs_rules(confscratch& xx,
                 std::map<const std::string, boost::shared_ptr<instr_str>,
                          isiless>& instsvect,
                 filepos& spos)
        : instsvect(instsvect),
          insts(xx), // instsvect is the variable in _var struct
          insts1rest(structmatch(
              insts.conts, insts.instrsyms, insts.rest,
              xx)[instopstruct<instr_str>(instsvect, insts.str, spos)]),
          instsrest(structmatches(insts1rest)) {}
  };

  struct strscratch : public confscratch {
    std::auto_ptr<percs_rules> prules;
    std::auto_ptr<instrs_rules> irules;
    strscratch(/*const info_setwhere wh,*/ fomusdata* fd)
        : confscratch(/*wh,*/ fd) {}
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      percinstrsparse,
      (5, (((varbase&), var), ((filepos&), pos), ((percs_rules&), i),
           ((globpercsvarvect&), m), ((confscratch&), x))),
      -,
      recerrpos(pos.file, pos.line, pos.col,
                pos.modif)[maybeclearcont<globpercsvarvect>(m, x.isplus)] >>
          (((i.percsrest | boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<globpercsvarvect>>(
                valid_f<globpercsvarvect>(var, x, m, false))) |
           boostspirit::eps_p[badvar(pos)]))
  inline void percinstrs_var::addconfrule(parserule* rules, confscratch& x) {
    ((strscratch&) x).prules.reset(new percs_rules(x, map, pos));
    rules[PERCINSTRS_ID] =
        percinstrsparse(*this, pos, *((strscratch&) x).prules.get(), map, x);
  }

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      instrsparse,
      (5, (((varbase&), var), ((filepos&), pos), ((instrs_rules&), i),
           ((globinstsvarvect&), m), ((confscratch&), x))),
      -,
      recerrpos(pos.file, pos.line, pos.col,
                pos.modif)[maybeclearcont<globinstsvarvect>(m, x.isplus)] >>
          (((i.instsrest | boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<globinstsvarvect>>(
                valid_f<globinstsvarvect>(var, x, m, false))) |
           boostspirit::eps_p[badvar(pos)]))
  inline void instrs_var::addconfrule(parserule* rules, confscratch& x) {
    ((strscratch&) x).irules.reset(new instrs_rules(x, map, pos));
    rules[INSTRS_ID] =
        instrsparse(*this, pos, *((strscratch&) x).irules.get(), map, x);
  }

} // namespace fomus
#endif
