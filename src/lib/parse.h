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

#ifndef FOMUS_PARSE_H
#define FOMUS_PARSE_H

#ifdef BUILD_LIBFOMUS
#include "heads.h"
#endif

#include "error.h"   // errbase
#include "infoapi.h" // info_setwhere
#include "numbers.h" // class rat, number

namespace FNAMESPACE {

#define VERBOSE_ID 0
#define USELEVEL_ID 1
#define FILENAME_ID 2
#define INPUTMOD_ID 3
#define OUTPUTMOD_ID 4
#define OUTPUTS_ID 5
#define CLEF_ID 6
#define NOTESYMBOLS_ID 7
#define NOTEACCS_ID 8
#define NOTEMICROTONES_ID 9
#define NOTEOCTAVES_ID 10
#define NOTEPRINT_ID 11
#define PERCINSTRS_ID 12
#define INSTRS_ID 13
#define NTHREADS_ID 14
#define CHOOSECLEF_ID 15
#define KEYSIG_ID 16
#define ACCRULE_ID 17
#define TQUANTMOD_ID 18
#define PQUANTMOD_ID 19
#define MEASMOD_ID 20
#define CHECKMOD_ID 21
#define TPOSEMOD_ID 22
#define VOICESMOD_ID 23
#define PRUNEMOD_ID 24
#define VMARKSMOD_ID 25
#define ACCSMOD_ID 26
#define CAUTACCSMOD_ID 27
#define STAVESMOD_ID 28
#define SMARKSMOD_ID 29
#define OCTSMOD_ID 30
#define DYNSMOD_ID 31
#define DIVMOD_ID 32
#define MERGEMOD_ID 33
#define BEAMMOD_ID 34
#define PMARKSMOD_ID 35
#define MARKSMOD_ID 36
#define SPECIALMOD_ID 37
#define PARTSMOD_ID 38
#define DEFAULTGRACEDUR_ID 39
#define RESTSTAVESMOD_ID 40
#define NBEATS_ID 41
#define COMP_ID 42
#define USERSTAVES_ID 43
#define TIMESIGDEN_ID 44
#define TIMESIGSTYLE_ID 45
#define NAME_ID 46
#define ABBR_ID 47
#define PERCNAME_ID 48
#define METAPARTSMOD_ID 49
#define PERCNOTESMOD_ID 50
#define COMMKEYSIG_ID 51
#define KEYSIGDEF_ID 52
#define MARKEVS1MOD_ID 53
#define DURSYMS_ID 54
#define DURDOTS_ID 55
#define DURTIES_ID 56
#define TUPSYMS_ID 57
#define DUMPINGMSG_ID 58
#define MAJMODE_ID 59
#define MINMODE_ID 60
#define TITLE_ID 61
#define AUTHOR_ID 62
#define ACC_ID 63
#define CLIP_ID 64
#define PRESETS_ID 65
#define TIMESIG_ID 66
#define BEAT_ID 67
#define TIMESIGS_ID 68
#define PARTS_ID 69
#define BARLINEL_ID 70
#define BARLINER_ID 71
#define FINALBAR_ID 72
#define SLURCANTOUCHDEF_ID 73
#define SLURCANSPANRESTSDEF_ID 74
#define WEDGECANTOUCHDEF_ID 75
#define WEDGECANSPANONEDEF_ID 76
#define WEDGECANSPANRESTSDEF_ID 77
#define TEXTCANTOUCHDEF_ID 78
#define TEXTCANSPANONEDEF_ID 79
#define TEXTCANSPANRESTSDEF_ID 80
#define SULSTYLE_ID 81
#define DEFAULTMARKTEXTS_ID 82
#define MARKTEXTS_ID 83
#define MARKALIASES_ID 84
#define PEDCANTOUCHDEF_ID 85
#define PEDCANSPANONEDEF_ID 86
#define PEDSTYLE_ID 87
#define TREMTIE_ID 88
#define FILL_ID 89
#define INITTEMPOTXT_ID 90
#define INITTEMPO_ID 91
#define DETACH_ID 92
#define PICKUP_ID 93
  // #define LEFTPICKUP_ID 94

  typedef boostspirit::position_iterator<
      char const*, boostspirit::file_position_base<std::string>>
      parse_it;
  typedef boostspirit::rule<boostspirit::scanner<parse_it>> parserule;
  // typedef assertion<string> asstn;

#ifndef NDEBUG
  struct printstr {
    const char* str;
    printstr(const char* str) : str(str) {}
    printstr() : str(0) {}
    template <typename A, typename B>
    void operator()(const A& s1, const B& s2) const {
#ifndef NDEBUGOUT
      if (str)
        DBG(str << ": |");
      else
        DBG("|");
#endif
      DBG(std::string(s1, s2) << "|" << std::endl);
    }
    template <typename A>
    void operator()(const A& s1) const {
#ifndef NDEBUGOUT
      if (str)
        DBG(str << ": |");
      else
        DBG("|");
#endif
      DBG(s1 << "|" << std::endl);
    }
  };
#endif

#ifndef NDEBUG
  template <typename I>
  struct print {
    const char* str;
    I& val;
    print(const char* str, I& val) : str(str), val(val) {}
    print(I& val) : str(0), val(val) {}
    template <typename A, typename B>
    void operator()(const A& s1, const B& s2) const {
#ifndef NDEBUGOUT
      if (str)
        DBG(str << ": |");
      else
        DBG("|");
#endif
      DBG(val << "|" << std::endl);
    }
    template <typename A>
    void operator()(const A& s1) const {
#ifndef NDEBUGOUT
      if (str)
        DBG(str << ": |");
      else
        DBG("|");
#endif
      DBG(val << "|" << std::endl);
    }
  };
#endif

  template <typename T>
  class fomsymbols : public boostspirit::symbols<T> {
private:
    bool empt;

public:
    fomsymbols() : boostspirit::symbols<T>(), empt(true) {}
    void add(const char* str, T const& data = T()) {
      boostspirit::symbols<T>::add(str, data);
      empt = false;
    }
    bool empty() const {
      return empt;
    }
  };

  // COMMENT PARSER
  // extern const parserule commatch;
  BOOST_SPIRIT_RULE_PARSER(commatch, -, -, -,
                           *(boostspirit::space_p |
                             boostspirit::comment_p("//") |
                             boostspirit::comment_nest_p("/-", "-/")))

// +/- PARSER
#define PLUSMATCH boostspirit::ch_p('+')
#define MINUSMATCH boostspirit::ch_p('-')
#define MULTMATCH boostspirit::ch_p('*')
#define DIVMATCH boostspirit::ch_p('/')

// LISTS
#define LISTSTART boostspirit::ch_p('(')
#define LISTEND boostspirit::ch_p(')')
#define LISTSTARTOREND boostspirit::chset_p("()")
#define LISTDELIM boostspirit::ch_p(',')

  struct filepos {
    int ord; // order in which vars were modified (
    std::string file;
    std::string linestr, colstr;
    fint line, col;
#ifdef BUILD_LIBFOMUS
    enum info_setwhere modif;
#else
    int maccnt;
#endif
    filepos(const int x)
        : ord(0), line(-1), col(-1),
#ifdef BUILD_LIBFOMUS
          modif(info_none)
#else
          maccnt(0)
#endif
    {
    }
    filepos()
        : ord(0), linestr("line"), colstr("col."), line(-1), col(-1),
#ifdef BUILD_LIBFOMUS
          modif(info_none)
#else
          maccnt(0)
#endif
    {
    }
    filepos(const std::string& file)
        : ord(0), file(file), linestr("line"), colstr("col."), line(-1),
          col(-1),
#ifdef BUILD_LIBFOMUS
          modif(info_none)
#else
          maccnt(0)
#endif
    {
    }
    filepos(const enum info_setwhere modif)
        : ord(0), linestr("line"), colstr("col."), line(-1), col(-1),
#ifdef BUILD_LIBFOMUS
          modif(modif)
#else
          maccnt(0)
#endif
    {
    }
    filepos(const int ord, const enum info_setwhere modif)
        : ord(ord), linestr("line"), colstr("col."), line(-1), col(-1),
#ifdef BUILD_LIBFOMUS
          modif(modif)
#else
          maccnt(0)
#endif
    {
    }
#ifdef BUILD_LIBFOMUS
    void printerr(const char* name) const {
      if (name)
        ferr << " for setting `" << name << '\'';
      printerr();
    }
    void printerr() const {
      printerr(ferr);
    }
#endif
    const filepos& operator++() {
      ++ord;
      return *this;
    }
    void printerr(std::ostream& ou) const {
      printerr0(ou);
      ou << std::endl;
    }
    void printerr0(std::ostream& ou) const;
  };

  // SYMBOL PARSER
  struct setrule {
    parserule& rule;
    setrule(parserule& rule) : rule(rule) {}
    void operator()(parserule* r) const {
      rule = *r;
    }
  };
  struct throwbadset {
    const std::string& str;
    filepos& pos;
    std::ostream& ou;
    throwbadset(const std::string& str, filepos& pos, std::ostream& ou)
        : str(str), pos(pos), ou(ou) {}
    void operator()(const parse_it& s1, const parse_it& s2) const;
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      symmatch,
      (3, (((const boostspirit::symbols<parserule*>&), table),
           ((parserule&), rest), ((const char*), eqsyms))),
      -,
      boostspirit::as_lower_d[table][setrule(rest)] >>
          boostspirit::eps_p(boostspirit::chset_p(eqsyms) |
                             boostspirit::space_p | boostspirit::str_p("//") |
                             boostspirit::str_p("/-")))

  // ERROR POSITION
  struct seterrpos {
    std::string& fi;
    fint& li;
    fint& co;
#ifndef BUILD_LIBFOMUS
    int& cnt; // used in fmsin.cc--if cnt > 0, don't set
#endif
#ifdef BUILD_LIBFOMUS
    seterrpos(std::string& fi, fint& li, fint& co) : fi(fi), li(li), co(co) {}
#else
    seterrpos(std::string& fi, fint& li, fint& co, int& cnt)
        : fi(fi), li(li), co(co), cnt(cnt) {}
#endif
    void operator()(const parse_it& s1, const parse_it& s2) const {
#ifndef BUILD_LIBFOMUS
      DBG("parse cnt = " << cnt << std::endl);
      if (cnt > 0)
        return;
#endif
      const boostspirit::file_position_base<std::string>& o(s1.get_position());
      fi = o.file;
      li = o.line;
      co = o.column;
    }
  };
#ifdef BUILD_LIBFOMUS
  extern info_setwhere currsetwhere;
  struct seterrwh {
    info_setwhere& wh;
    seterrwh(info_setwhere& wh) : wh(wh) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      wh = currsetwhere;
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      recerrpos,
      (4, (((std::string&), file), ((fint&), line), ((fint&), col),
           ((info_setwhere&), wh))),
      -, boostspirit::eps_p[seterrpos(file, line, col)][seterrwh(wh)])
#else
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      recerrpos,
      (4, (((std::string&), file), ((fint&), line), ((fint&), col),
           ((int&), maccnt))),
      -, boostspirit::eps_p[seterrpos(file, line, col, maccnt)])
#endif

  // SETTERS
  template <typename T>
  struct setval {
    T& val;
    setval(T& val) : val(val) {}
    void operator()(const T n) const {
      val = n;
    }
  };
  template <typename T>
  struct setconstval {
    T& val;
    const T set;
    setconstval(T& val, const T set) : val(val), set(set) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      val = set;
    }
    template <typename X>
    void operator()(const X& x) const {
      val = set;
    }
  };

  // EQUAL SIGN PARSER
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(eqlmatch, (1, (((bool&), isplus))), -,
                                  commatch[setconstval<bool>(isplus, false)] >>
                                      !(boostspirit::chset_p(":=") >> commatch))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(eqldelmatch, (1, (((bool&), isplus))), -,
                                  commatch[setconstval<bool>(isplus, false)] >>
                                      !(boostspirit::chset_p(":=,") >>
                                        commatch))
  BOOST_SPIRIT_RULE_PARSER(eqldelmatch0, -, -, -,
                           commatch >>
                               !(boostspirit::chset_p(":=,") >> commatch))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      pluseqlmatch, (1, (((bool&), isplus))), -,
      commatch >> ((((boostspirit::ch_p('+') >> !boostspirit::chset_p(":=")) |
                     (!boostspirit::chset_p("=:") >> boostspirit::ch_p('+'))) >>
                    commatch)[setconstval<bool>(isplus, true)] |
                   (!(boostspirit::chset_p(":=") >>
                      commatch))[setconstval<bool>(isplus, false)]))

  // NUMBER PARSERS
  template <typename T>
  struct setnumval {
    numb& val;

    setnumval(numb& val) : val(val) {}
    void operator()(const T& n) const {
      val = n;
    }
  };
  // store in a number obj.
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(numintmatch, (1, (((numb&), var))), -,
                                  boostspirit::int_p[setnumval<fint>(var)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(numfloatmatch, (1, (((numb&), var))), -,
                                  boostspirit::real_p[setnumval<ffloat>(var)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      numstrictfloatmatch, (1, (((numb&), var))), -,
      boostspirit::strict_real_p[setnumval<ffloat>(var)])
  // extern const string den0_bad;
  template <typename T>
  struct calcrat2 {
    fint& pt1;
    fint& pt2;
    T& rt;
    filepos& pos;
    std::ostream& ou;
    calcrat2(fint& pt1, fint& pt2, T& rt, filepos& pos, std::ostream& ou)
        : pt1(pt1), pt2(pt2), rt(rt), pos(pos), ou(ou) {}
    void operator()(const parse_it& s1, const parse_it& s2) const;
  };
  template <typename T>
  struct calcrat3pl : public calcrat2<T> {
    fint& pt3;
    calcrat3pl(fint& pt1, fint& pt2, fint& pt3, T& rt, filepos& pos,
               std::ostream& ou)
        : calcrat2<T>(pt1, pt2, rt, pos, ou), pt3(pt3) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      if (pt3 == 0) {
        calcrat2<T>::ou << "division by zero error";
        throw_(s1, &(filepos&) calcrat2<T>::pos);
      }
      calcrat2<T>::rt = calcrat2<T>::pt1 + rat(calcrat2<T>::pt2, pt3);
    }
  };
  template <typename T>
  struct calcrat3mi : public calcrat2<T> {
    fint& pt3;
    calcrat3mi(fint& pt1, fint& pt2, fint& pt3, T& rt, filepos& pos,
               std::ostream& ou)
        : calcrat2<T>(pt1, pt2, rt, pos, ou), pt3(pt3) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      if (pt3 == 0) {
        calcrat2<T>::ou << "division by zero error";
        throw_(s1, &(filepos&) calcrat2<T>::pos);
      }
      calcrat2<T>::rt = calcrat2<T>::pt1 - rat(calcrat2<T>::pt2, pt3);
    }
  };

  inline void throwbadset::operator()(const parse_it& s1,
                                      const parse_it& s2) const {
    // assert(false);
    ou << /*"fomus: " <<*/ "unknown setting `" << str
       << '\''; // fixed from fmsin.cc
    throw_(s1, &pos);
  }

  template <>
  inline void calcrat2<rat>::operator()(const parse_it& s1,
                                        const parse_it& s2) const {
    if (pt2 == 0) {
      ou << /*"fomus: " <<*/ "division by zero error"; // fixed from fmsin.cc
      throw_(s1, &pos);
    }
    rt = rat(pt1, pt2);
  }
  template <>
  inline void calcrat2<numb>::operator()(const parse_it& s1,
                                         const parse_it& s2) const {
    if (pt2 == 0) {
      ou << /*"fomus: " <<*/ "division by zero error"; // fixed from fmsin.cc
      throw_(s1, &pos);
    }
    rt = rat(pt1, pt2);
  }

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(ratauxchar,
                                  (2, (((const char), x), ((fint&), pt))), -,
                                  boostspirit::ch_p(x) >>
                                      boostspirit::uint_p[setval<fint>(pt)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      ratmatch,
      (6, (((fint&), pt1), ((fint&), pt2), ((fint&), pt3), ((rat&), rt),
           ((filepos&), pos), ((std::ostream&), ou))),
      -,
      (boostspirit::int_p[setval<fint>(pt1)] >>
       (ratauxchar('/', pt2)[calcrat2<rat>(pt1, pt2, rt, pos, ou)] |
        (ratauxchar('+', pt2) >>
         ratauxchar('/', pt3)[calcrat3pl<rat>(pt1, pt2, pt3, rt, pos, ou)]) |
        (ratauxchar('-', pt2) >>
         ratauxchar('/', pt3)[calcrat3mi<rat>(pt1, pt2, pt3, rt, pos, ou)]))))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      numratmatch,
      (6, (((fint&), pt1), ((fint&), pt2), ((fint&), pt3), ((numb&), rt),
           ((filepos&), pos), ((std::ostream&), ou))),
      -,
      (boostspirit::int_p[setval<fint>(pt1)] >>
       (ratauxchar('/', pt2)[calcrat2<numb>(pt1, pt2, rt, pos, ou)] |
        (ratauxchar('+', pt2) >>
         ratauxchar('/', pt3)[calcrat3pl<numb>(pt1, pt2, pt3, rt, pos, ou)]) |
        (ratauxchar('-', pt2) >>
         ratauxchar('/', pt3)[calcrat3mi<numb>(pt1, pt2, pt3, rt, pos, ou)]))))

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      numbermatch,
      (6, (((numb&), val), ((fint&), pt1), ((fint&), pt2), ((fint&), pt3),
           ((filepos&), pos), ((std::ostream&), ou))),
      -,
      (numratmatch(pt1, pt2, pt3, val, pos, ou) | numstrictfloatmatch(val) |
       numintmatch(val)))
  extern boostspirit::symbols<fint> boolsyms;
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      boolmatch, (1, (((numb&), val))), -,
      boostspirit::as_lower_d[boolsyms][setnumval<fint>(val)] >>
          (boostspirit::eps_p(~boostspirit::alnum_p) | boostspirit::end_p))

  struct addnumval {
    numb& val;
    addnumval(numb& val) : val(val) {}
    void operator()(const numb& n) const {
      val = val + n.modval();
    }
  };
  struct addconstnumval {
    numb &val1, &val2;
    addconstnumval(numb& val1, numb& val2) : val1(val1), val2(val2) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      val1 = val1 + val2.modval();
    }
  };
  struct multnumval {
    numb& val;
    multnumval(numb& val) : val(val) {}
    void operator()(const numb& n) const {
      val = val * n.modval();
    }
  };

#ifdef BUILD_LIBFOMUS
  struct nearestpitch {
    numb& val;
    numb& prval;
    bool& gup;
    nearestpitch(numb& val, numb& prval, bool& gup)
        : val(val), prval(prval), gup(gup) {}
    void operator()(const parse_it& s1, const parse_it& s2) const;
  };
  extern fomsymbols<numb> note_parse;
  extern fomsymbols<numb> acc_parse;
  extern fomsymbols<numb> mic_parse;
  extern fomsymbols<numb> oct_parse;
  // extern fomsymbols<int> ksig_parse;
  // ...was notesym[addnumval(val)]
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      notematch,
      (10, (((const boostspirit::symbols<numb>&), notesym),
            ((const boostspirit::symbols<numb>&), accsym),
            ((const boostspirit::symbols<numb>&), micsym),
            ((const boostspirit::symbols<numb>&), octsym), ((numb&), val),
            ((fint&), pt1), ((fint&), pt2), ((fint&), pt3), ((filepos&), pos),
            ((std::ostream&), ou))),
      -,
      (notesym[setnumval<numb>(val)] >> !accsym[addnumval(val)] >>
       !micsym[addnumval(val)] >> !octsym[addnumval(val)]) |
          numbermatch(val, pt1, pt2, pt3, pos, ou))

  extern fomsymbols<numb> durdot_parse;
  extern fomsymbols<numb> dursyms_parse;
  extern fomsymbols<numb> durtie_parse;
  extern fomsymbols<numb> tupsyms_parse;

#endif

  // STRING PARSERS
  struct clearstr {
    std::string& str;
    clearstr(std::string& str) : str(str) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      str.clear();
    }
  };
  struct setstr {
    std::string& str;
    setstr(std::string& str) : str(str) {}
    void operator()(const unsigned char x) const {
      if ((x <= 0x7F && (isprint(x) || x == '\t' || x == '\n')) ||
          (x >= 0x80 && x <= 0xBF)     // 2nd or later byte in UTF-8
          || (x >= 0xC2 && x <= 0xF4)) // 1st byte in UTF-8
        str += x;
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      strmatchaux,
      (3, (((std::string&), str), ((const char*), end), ((const char*), quo))),
      -,
      (boostspirit::eps_p[clearstr(str)] >>
       (('"' >> *(if_p(boostspirit::ch_p('"'))[boostspirit::nothing_p]
                      .else_p[boostspirit::lex_escape_ch_p[setstr(str)]]) >>
         boostspirit::anychar_p) |
        ('\'' >> *(if_p(boostspirit::ch_p('\''))[boostspirit::nothing_p]
                       .else_p[boostspirit::lex_escape_ch_p[setstr(str)]]) >>
         boostspirit::anychar_p) |
        +(if_p(boostspirit::space_p | boostspirit::chset_p(end) |
               boostspirit::str_p("//") | boostspirit::str_p("/-") |
               boostspirit::chset_p(quo))[boostspirit::nothing_p]
              .else_p[boostspirit::lex_escape_ch_p[setstr(str)]]))))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(strmatch,
                                  (2, (((std::string&), str),
                                       ((const char*), end))),
                                  -, strmatchaux(str, end, "\"'"))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      strmatchspec, (2, (((std::string&), str), ((const char*), end))), -,
      strmatchaux(
          str, end,
          "")) // for special strings that might have a quote char in them

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      symmatcherr,
      (6, (((const boostspirit::symbols<parserule*>&), table),
           ((parserule&), rest), ((const char*), eqsyms), ((std::string&), str),
           ((filepos&), pos), ((std::ostream&), ou))),
      -,
      symmatch(table, rest, eqsyms) |
          strmatch(str, eqsyms)[throwbadset(str, pos, ou)])

  // LISTS
  struct listelshptr;
  typedef boost::variant<numb, std::string,
                         boost::recursive_wrapper<listelshptr>>
      listel;
  typedef std::vector<listel> listelvect;
  struct listelshptr : public boost::shared_ptr<listelvect> {
    listelshptr() : boost::shared_ptr<listelvect>() {}
    listelshptr(listelvect* v) : boost::shared_ptr<listelvect>(v) {}
  };
  typedef listelvect::iterator listelvect_it;
  typedef listelvect::const_iterator listelvect_constit;

  class typeerr : public errbase {}; // ***** SAVE THIS
  class listel_fint : public boost::static_visitor<fint> {
public:
    fint operator()(const numb& x) const {
      return numtoint(x);
    }
    fint operator()(const std::string& x) const {
      throw typeerr();
    }
    fint operator()(const boost::shared_ptr<listelvect>& x) const {
      throw typeerr();
    }
  };
  class listel_numb : public boost::static_visitor<const numb&> {
public:
    const numb& operator()(const numb& x) const {
      return x;
    }
    const numb& operator()(const std::string& x) const {
      throw typeerr();
    }
    const numb& operator()(const boost::shared_ptr<listelvect>& x) const {
      throw typeerr();
    }
  };
  class listel_string : public boost::static_visitor<const std::string&> {
public:
    const std::string& operator()(const numb& x) const {
      throw typeerr();
    }
    const std::string& operator()(const std::string& x) const {
      return x;
    }
    const std::string&
    operator()(const boost::shared_ptr<listelvect>& x) const {
      throw typeerr();
    }
  };
  class listel_vect : public boost::static_visitor<const listelvect&> {
public:
    const listelvect& operator()(const numb& x) const {
      throw typeerr();
    }
    const listelvect& operator()(const std::string& x) const {
      throw typeerr();
    }
    const listelvect& operator()(const boost::shared_ptr<listelvect>& x) const {
      return *x.get();
    }
  };
  inline fint listel_getint(const listel& el) {
    return apply_visitor(listel_fint(), el);
  }
  inline const numb& listel_getnumb(const listel& el) {
    return apply_visitor(listel_numb(), el);
  }
  inline const std::string& listel_getstring(const listel& el) {
    return apply_visitor(listel_string(), el);
  }
  inline const listelvect& listel_getvect(const listel& el) {
    return apply_visitor(listel_vect(), el);
  }
  inline std::vector<fint> listelvect_tointvect(const listelvect& el) {
    std::vector<fint> vect;
    std::transform(el.begin(), el.end(), std::back_inserter(vect),
                   boost::lambda::bind(&listel_getint, boost::lambda::_1));
    return vect;
  }

  template <typename T>
  struct clearlist {
    typename std::vector<T>& list;
    clearlist(typename std::vector<T>& list) : list(list) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      list.clear();
    }
  };
  //   struct clearshlist {
  //     typename boost::shared_ptr<listelvect> &list;
  //     clearshlist(typename boost::shared_ptr<listelvect> &list):list(list) {}
  //     void operator()(const parse_it &s1, const parse_it &s2) const
  //     {list->reset(new listelvect);}
  //   };
  template <typename T>
  struct maybeclearcont {
    T& cont;
    bool& app;
    maybeclearcont(T& list, bool& app) : cont(list), app(app) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      /*DBG("should be CLEARING CONT" << std::endl);*/ if (app)
        app = false;
      else { /*DBG("CLEARING CONT" << std::endl);*/
        cont.clear();
      }
    }
    void operator()(
        const char x) const { /*DBG("should be CLEARING CONT" << std::endl);*/
      if (app)
        app = false;
      else { /*DBG("CLEARING CONT" << std::endl);*/
        cont.clear();
      }
    }
  };
  template <typename T>
  struct aclearcont {
    std::auto_ptr<T>& cont;
    aclearcont(std::auto_ptr<T>& cont) : cont(cont) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      cont->clear();
    }
    void operator()(const char x) const {
      cont->clear();
    }
  };
  struct setlistnum {
    listelvect& list;
    numb& num;
    setlistnum(listelvect& list, numb& num) : list(list), num(num) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      list.push_back(num);
    }
  };
  struct setliststr {
    listelvect& list;
    std::string& str;
    setliststr(listelvect& list, std::string& str) : list(list), str(str) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      list.push_back(str);
    }
  };
  struct asetlistnum {
    std::auto_ptr<listelvect>& list;
    numb& num;
    asetlistnum(std::auto_ptr<listelvect>& list, numb& num)
        : list(list), num(num) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      list->push_back(num);
    }
  };
  struct asetliststr {
    std::auto_ptr<listelvect>& list;
    std::string& str;
    asetliststr(std::auto_ptr<listelvect>& list, std::string& str)
        : list(list), str(str) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      list->push_back(str);
    }
  };
  struct setlistlist {
    std::auto_ptr<listelvect>& list;
    listelvect& biglist;
    setlistlist(std::auto_ptr<listelvect>& list, listelvect& biglist)
        : list(list), biglist(biglist) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      biglist.push_back(listelshptr(list.release()));
      list.reset(new listelvect);
    }
  };
  extern bool boolfalse;
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      listmatchhead, (2, (((listelvect&), lst), ((bool&), isplus))), -,
      boostspirit::eps_p[maybeclearcont<listelvect>(lst, isplus)] >>
          LISTSTART >> commatch)
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      alistmatchhead, (1, (((std::auto_ptr<listelvect>&), lst))), -,
      boostspirit::eps_p[aclearcont<listelvect>(lst)] >> LISTSTART >> commatch)
  BOOST_SPIRIT_RULE_PARSER(listmatchdelim, -, -, -,
                           commatch >> !(LISTDELIM >> commatch))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      listmatchnums,
      (8, (((listelvect&), lst), ((numb&), val), ((fint&), pt1), ((fint&), pt2),
           ((fint&), pt3), ((bool&), isplus), ((filepos&), pos),
           ((std::ostream&), ou))),
      -,
      (listmatchhead(lst, isplus) >>
       *(if_p(LISTEND)[boostspirit::nothing_p]
             .else_p[numbermatch(val, pt1, pt2, pt3, pos,
                                 ou)[setlistnum(lst, val)] >>
                     listmatchdelim]) >>
       boostspirit::anychar_p) |
          numbermatch(val, pt1, pt2, pt3, pos, ou)[setlistnum(lst, val)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      alistmatchnums,
      (7, (((std::auto_ptr<listelvect>&), lst), ((numb&), val), ((fint&), pt1),
           ((fint&), pt2), ((fint&), pt3), ((filepos&), pos),
           ((std::ostream&), ou))),
      -,
      (alistmatchhead(lst) >>
       *(if_p(LISTEND)[boostspirit::nothing_p]
             .else_p[numbermatch(val, pt1, pt2, pt3, pos,
                                 ou)[asetlistnum(lst, val)] >>
                     listmatchdelim]) >>
       boostspirit::anychar_p) |
          numbermatch(val, pt1, pt2, pt3, pos, ou)[asetlistnum(lst, val)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      listmatchstrs,
      (4, (((listelvect&), lst), ((std::string&), str), ((bool&), isplus),
           ((const char*), end))),
      -,
      (listmatchhead(lst, isplus) >>
       *(if_p(LISTEND)[boostspirit::nothing_p]
             .else_p[strmatch(str, "),")[setliststr(lst, str)] >>
                     listmatchdelim]) >>
       boostspirit::anychar_p) |
          strmatch(str, end)[setliststr(lst, str)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      alistmatchstrs,
      (3, (((std::auto_ptr<listelvect>&), lst), ((std::string&), str),
           ((const char*), end))),
      -,
      (alistmatchhead(lst) >>
       *(if_p(LISTEND)[boostspirit::nothing_p]
             .else_p[strmatch(str, "),")[asetliststr(lst, str)] >>
                     listmatchdelim]) >>
       boostspirit::anychar_p) |
          strmatch(str, end)[asetliststr(lst, str)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      listmatchlistsofnums,
      (9, (((std::auto_ptr<listelvect>&), lst), ((numb&), val), ((fint&), pt1),
           ((fint&), pt2), ((fint&), pt3), ((bool&), isplus), ((filepos&), pos),
           ((listelvect&), biglst), ((std::ostream&), ou))),
      -,
      (listmatchhead(biglst, isplus) >>
       *(if_p(LISTEND)[boostspirit::nothing_p]
             .else_p[alistmatchnums(lst, val, pt1, pt2, pt3, pos,
                                    ou)[setlistlist(lst, biglst)] >>
                     listmatchdelim]) >>
       boostspirit::anychar_p))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      listmatchlistsofstrs,
      (5, (((std::auto_ptr<listelvect>&), lst), ((std::string&), str),
           ((bool&), isplus), ((filepos&), pos), ((listelvect&), biglst))),
      -,
      (listmatchhead(biglst, isplus) >>
       *(if_p(LISTEND)[boostspirit::nothing_p]
             .else_p[alistmatchstrs(lst, str, "),")[setlistlist(lst, biglst)] >>
                     listmatchdelim]) >>
       boostspirit::anychar_p))

// MAP
#ifdef BUILD_LIBFOMUS

  typedef std::map<const std::string, listel> listelmap;
  typedef listelmap::value_type listelmap_val;
  typedef listelmap::iterator listelmap_it;
  typedef listelmap::const_iterator listelmap_constit;

  struct clearmap {
    listelmap& map;
    clearmap(listelmap& map) : map(map) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      map.clear();
    }
  };
  struct setmapbase {
    listelmap& map;
    std::string& name;
    setmapbase(listelmap& map, std::string& name) : map(map), name(name) {}
  };
  struct setmapnum : public setmapbase {
    numb& num;
    setmapnum(listelmap& map, std::string& name, numb& num)
        : setmapbase(map, name), num(num) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      map.erase(name); // make sure we're overwriting
      map.insert(listelmap_val(name, num));
    }
  };
  template <typename T>
  struct setmapval : public setmapbase {
    T& val;
    setmapval(listelmap& map, std::string& name, T& val)
        : setmapbase(map, name), val(val) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      map.erase(name);
      map.insert(listelmap_val(name, val));
    }
  };
  struct setmaplist : public setmapbase {
    std::auto_ptr<listelvect>& list;
    setmaplist(listelmap& map, std::string& name,
               std::auto_ptr<listelvect>& list)
        : setmapbase(map, name), list(list) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      map.erase(name);
      map.insert(listelmap_val(name, listelshptr(list.release())));
      list.reset(new listelvect);
    }
  };
  struct maybeclearconts {
    listelmap& cont1;
    listelvect& cont2;
    bool& app;
    maybeclearconts(listelmap& list1, listelvect& list2, bool& app)
        : cont1(list1), cont2(list2), app(app) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      if (app)
        app = false;
      else {
        cont1.clear();
        cont2.clear();
      }
    }
    void operator()(const char x) const {
      if (app)
        app = false;
      else {
        cont1.clear();
        cont2.clear();
      }
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchhead,
      (3, (((listelmap&), map), ((listelvect&), lst), ((bool&), isplus))), -,
      boostspirit::eps_p[maybeclearconts(map, lst, isplus)] >> LISTSTART >>
          commatch)
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchaux,
      (3, (((std::string&), nam), ((listelvect&), lst), ((bool&), isplus))), -,
      strmatch(nam, ":=,")[setliststr(lst, nam)] >> eqldelmatch(isplus))
  // also stores the names in lst
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchnums,
      (10, (((listelmap&), map), ((listelvect&), lst), ((std::string&), nam),
            ((numb&), val), ((fint&), pt1), ((fint&), pt2), ((fint&), pt3),
            ((bool&), isplus), ((filepos&), pos), ((std::ostream&), ou))),
      -,
      mapmatchhead(map, lst, isplus) >>
          *(if_p(LISTEND)[boostspirit::nothing_p]
                .else_p[mapmatchaux(nam, lst, isplus) >>
                        numbermatch(val, pt1, pt2, pt3, pos,
                                    ou)[setmapnum(map, nam, val)] >>
                        listmatchdelim]) >>
          boostspirit::anychar_p)
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchstrs,
      (5, (((listelmap&), map), ((listelvect&), lst), ((std::string&), nam),
           ((std::string&), str), ((bool&), isplus))),
      -,
      mapmatchhead(map, lst, isplus) >>
          *(if_p(LISTEND)[boostspirit::nothing_p]
                .else_p[mapmatchaux(nam, lst, isplus) >>
                        strmatch(str,
                                 "),")[setmapval<std::string>(map, nam, str)] >>
                        listmatchdelim]) >>
          boostspirit::anychar_p)
  // the names go into lst, lst2 is a scratch list
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchlistsofnums,
      (11, (((listelmap&), map), ((listelvect&), lst), ((std::string&), nam),
            ((numb&), val), ((fint&), pt1), ((fint&), pt2), ((fint&), pt3),
            ((bool&), isplus), ((filepos&), pos),
            ((std::auto_ptr<listelvect>&), lst2), ((std::ostream&), ou))),
      -,
      mapmatchhead(map, lst, isplus) >>
          *(if_p(LISTEND)[boostspirit::nothing_p]
                .else_p[mapmatchaux(nam, lst, isplus) >>
                        alistmatchnums(lst2, val, pt1, pt2, pt3, pos,
                                       ou)[setmaplist(map, nam, lst2)] >>
                        listmatchdelim]) >>
          boostspirit::anychar_p)
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchlistsofstrs,
      (6, (((listelmap&), map), ((listelvect&), lst), ((std::string&), nam),
           ((std::string&), str), ((bool&), isplus),
           ((std::auto_ptr<listelvect>&), lst2))),
      -,
      mapmatchhead(map, lst, isplus) >>
          *(if_p(LISTEND)[boostspirit::nothing_p]
                .else_p[mapmatchaux(nam, lst, isplus) >>
                        alistmatchstrs(lst2, str,
                                       "),")[setmaplist(map, nam, lst2)] >>
                        listmatchdelim]) >>
          boostspirit::anychar_p)

#else

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchaux,
      (3, (((std::string&), nam), ((listelvect&), lst), ((bool&), isplus))), -,
      strmatch(nam, ":=,")[setliststr(lst, nam)] >> eqldelmatch(isplus))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchnums,
      (9, (((listelvect&), lst), ((std::string&), nam), ((numb&), val),
           ((fint&), pt1), ((fint&), pt2), ((fint&), pt3), ((bool&), isplus),
           ((filepos&), pos), ((std::ostream&), ou))),
      -,
      listmatchhead(lst, isplus) >>
          *(if_p(LISTEND)[boostspirit::nothing_p]
                .else_p[mapmatchaux(nam, lst, isplus) >>
                        numbermatch(val, pt1, pt2, pt3, pos,
                                    ou)[setlistnum(lst, val)] >>
                        listmatchdelim]) >>
          boostspirit::anychar_p)
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchstrs,
      (4, (((listelvect&), lst), ((std::string&), nam), ((std::string&), str),
           ((bool&), isplus))),
      -,
      listmatchhead(lst, isplus) >>
          *(if_p(LISTEND)[boostspirit::nothing_p]
                .else_p[mapmatchaux(nam, lst, isplus) >>
                        strmatch(str, "),")[setliststr(lst, str)] >>
                        listmatchdelim]) >>
          boostspirit::anychar_p)
  // everything goes onto biglst
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchlistsofnums,
      (10, (((std::auto_ptr<listelvect>&), lst), ((std::string&), nam),
            ((numb&), val), ((fint&), pt1), ((fint&), pt2), ((fint&), pt3),
            ((bool&), isplus), ((filepos&), pos), ((listelvect&), biglst),
            ((std::ostream&), ou))),
      -,
      listmatchhead(biglst, isplus) >>
          *(if_p(LISTEND)[boostspirit::nothing_p]
                .else_p[mapmatchaux(nam, biglst, isplus) >>
                        alistmatchnums(lst, val, pt1, pt2, pt3, pos,
                                       ou)[setlistlist(lst, biglst)] >>
                        listmatchdelim]) >>
          boostspirit::anychar_p)
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapmatchlistsofstrs,
      (5, (((std::auto_ptr<listelvect>&), lst), ((std::string&), nam),
           ((std::string&), str), ((bool&), isplus), ((listelvect&), biglst))),
      -,
      listmatchhead(biglst, isplus) >>
          *(if_p(LISTEND)[boostspirit::nothing_p]
                .else_p[mapmatchaux(nam, biglst, isplus) >>
                        alistmatchstrs(lst, str,
                                       "),")[setlistlist(lst, biglst)] >>
                        listmatchdelim]) >>
          boostspirit::anychar_p)

#endif

} // namespace FNAMESPACE
#endif
