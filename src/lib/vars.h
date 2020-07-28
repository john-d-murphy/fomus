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

#ifndef FOMUS_VARS_H
#define FOMUS_VARS_H

#ifndef BUILD_LIBFOMUS
#error "vars.h shouldn't be included"
#endif

#include "heads.h"

#include "algext.h"  // for_each2, none
#include "error.h"   // class errbase
#include "infoapi.h" // info_setwhere
#include "marks.h"
#include "mods.h"     // modbase
#include "modtypes.h" // module_setting, type module_setting_loc
#include "module.h"
#include "numbers.h"  // class rat, number
#include "parse.h"    // class asstn
#include "userstrs.h" // clefstypestr

namespace fomus {

  extern bool initing;
  extern info_setwhere currsetwhere;

  typedef std::map<std::string, boost::filesystem::path, isiless> presetsmap;
  typedef presetsmap::const_iterator presetsmap_constit;
  typedef presetsmap::iterator presetsmap_it;

  numb doparsenote(fomusdata* fd, const char* no, const bool noneonerr = false,
                   module_noteparts* xtra = 0);
  numb doparseacc(const fomusdata* fd, const char* no,
                  const bool noneonerr = false, module_noteparts* xtra = 0);
  numb doparsedur(fomusdata* fd, const char* no, const bool noneonerr = false);

  bool localallowed(const enum module_setting_loc in,
                    const enum module_setting_loc setloc);

  std::string stringify(std::string s, const char* inlist = 0);

  std::ostream& operator<<(std::ostream& os, const listelvect& x);
  inline std::ostream& operator<<(std::ostream& os, const listelvect* x) {
    return operator<<(os, *x);
  }
  class listel_out : public boost::static_visitor<void> {
    std::ostream& os;

public:
    listel_out(std::ostream& os) : os(os) {}
    void operator()(const numb& x) const {
      os << x;
    }
    void operator()(const std::string& x) const {
      os << stringify(x, "),");
    }
    void operator()(const boost::shared_ptr<listelvect>& x) const {
      os << x;
    }
  };
  inline std::ostream& operator<<(std::ostream& os, const listel& x) {
    apply_visitor(listel_out(os), x);
    return os;
  }

  typedef std::map<const rat, std::string> printmap;
  typedef printmap::const_iterator printmap_constit;
  typedef printmap::iterator printmap_it;
  typedef printmap::value_type printmap_val;

  extern printmap note_print;
  extern printmap acc_print;
  extern printmap mic_print;
  extern printmap oct_print;

  class instr_str;
  class percinstr_str;
  struct symtabs {
    fomsymbols<numb>& note;
    fomsymbols<numb>& acc;
    fomsymbols<numb>& mic;
    fomsymbols<numb>& oct;
    printmap& notepr;
    printmap& accpr;
    printmap& micpr;
    printmap& octpr;
    fomusdata* fd;

    fomsymbols<numb>& durdot;
    fomsymbols<numb>& dursyms;
    fomsymbols<numb>& durtie;
    fomsymbols<numb>& tupsyms;

    symtabs(fomusdata* fd)
        : note(note_parse), acc(acc_parse), mic(mic_parse), oct(oct_parse),
          notepr(note_print), accpr(acc_print), micpr(mic_print),
          octpr(oct_print), fd(fd), durdot(durdot_parse),
          dursyms(dursyms_parse), durtie(durtie_parse), tupsyms(tupsyms_parse) {
    }
    symtabs(fomsymbols<numb>& note, fomsymbols<numb>& acc,
            fomsymbols<numb>& mic, fomsymbols<numb>& oct, printmap& notepr,
            printmap& accpr, printmap& micpr, printmap& octpr, fomusdata* fd,
            fomsymbols<numb>& durdot, fomsymbols<numb>& dursyms,
            fomsymbols<numb>& durtie, fomsymbols<numb>& tupsyms)
        : note(note), acc(acc), mic(mic), oct(oct), notepr(notepr),
          accpr(accpr), micpr(micpr), octpr(octpr), fd(fd), durdot(durdot),
          dursyms(dursyms), durtie(durtie), tupsyms(tupsyms) {}
  };

  struct scopedmodval
      : public module_value { // this should always be initialized when vars are
                              // constructed!
    scopedmodval() {
      type = module_none;
    }
    ~scopedmodval() {
      freevalue(*this);
    }
    void reset() {
      freevalue(*this);
      type = module_none;
    }
    void operator=(const module_value& mv) {
      assert(type == module_none);
      (module_value&) * this = mv;
    }
#ifndef NDEBUG
    bool notyet() const {
      return type == module_none;
    }
#endif
  };

  // exceptions
  class stage;
  struct badset : public errbase {};
  // extern boost::thread_specific_ptr<stage> stageobj;

  inline void throwgtype(const char* typ, const char* name, const filepos& p) {
    CERR << "cannot get " << typ << " from setting `" << name << '\'';
    if (stageobj.get())
      throw badset();
    p.printerr();
  }
  inline void throwstype(const char* name, const char* settyp,
                         const filepos& p) {
    CERR << "expected value of type `" << settyp << "' for setting `" << name
         << '\'';
    p.printerr();
  }
  inline void throwapp(const char* name, const filepos& p) {
    CERR << "cannot append to setting `" << name << '\'';
    p.printerr();
  }

  class varbase;
  struct confscratch _NONCOPYABLE {
    fint pt1, pt2, pt3;
    numb num, num0, num1;
    std::string str;
    std::string name;
    listelmap map;  // for garbage
    listelvect lst; //, lst2; //, biglst, biglst for maps
    numb prevnum;   // for parsing notes
    bool gup;       // whether going up or not
    symtabs ta;
    bool isplus; //, plgb; // plgb = garbage
    filepos pos; //, spos; // spos is for the beginning of a struct
    boost::shared_ptr<varbase> newvar;
    std::auto_ptr<listelvect> autolst;
    confscratch(fomusdata* fd)
        : gup(true), ta(fd), pos(currsetwhere), autolst(new listelvect) {}
  };

  // vars
  typedef std::map<const std::string, boost::shared_ptr<instr_str>, isiless>
      globinstsvarvect;
  typedef globinstsvarvect::const_iterator globinstsvarvect_constit;
  typedef globinstsvarvect::iterator globinstsvarvect_it;
  typedef globinstsvarvect::value_type globinstsvarvect_val;
  typedef std::map<const std::string, boost::shared_ptr<percinstr_str>, isiless>
      globpercsvarvect;
  typedef globpercsvarvect::const_iterator globpercsvarvect_constit;
  typedef globpercsvarvect::iterator globpercsvarvect_it;
  typedef globpercsvarvect::value_type globpercsvarvect_val;

  // map for instrs and notes
  typedef std::map<const int, boost::shared_ptr<const varbase>> setmap;
  typedef setmap::value_type setmap_val;
  typedef setmap::iterator setmap_it;
  typedef setmap::const_iterator setmap_constit;

  class str_base;
  class import_str;
  class export_str;
  class clef_str;
  class staff_str;
  class percinstr_str;
  class instr_str;

  template <typename T>
  struct sticknewvar {
    boost::shared_ptr<T>& map;
    boost::shared_ptr<varbase>& var;
    sticknewvar(boost::shared_ptr<varbase>& var, boost::shared_ptr<T>& map)
        : map(map), var(var) {}
    void operator()(const parse_it& s1, const parse_it& s2) const;
  };

  struct thrownotallowed {
    const char* set;
    const char* str;
    filepos& pos;
    thrownotallowed(const char* set, const char* str, filepos& pos)
        : set(set), str(str), pos(pos) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      CERR << "cannot set setting `" << set << "' inside " << str
           << " structure";
      throw_(s1, &pos);
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      notallowed,
      (3, (((const char*), set), ((const char*), str), ((filepos&), pos))), -,
      boostspirit::eps_p[thrownotallowed(set, str, pos)])
  // classes
  class fomusdata;
  class varbase _NONCOPYABLE {
private:
    int id; // index into vars vector
protected:
    filepos pos;
    scopedmodval mval; // cached modval for user
#ifndef NDEBUG
    int debugvalid;
#endif
public:
    varbase();
    varbase(const varbase& x, const filepos& pos)
        : id(x.id), pos(pos)
#ifndef NDEBUG
          ,
          debugvalid(12345)
#endif
    {
      assert(varbase::pos.modif >= 1);
    }
    virtual ~varbase() {
#ifndef NDEBUG
      debugvalid = 0;
#endif
    }
    void resetmval() {
      mval.reset();
    }
#ifndef NDEBUG
    bool mvalexists() const {
      return !mval.notyet();
    }
    const module_value& getmval() const {
      return mval;
    }
    bool varisvalid() const {
      return debugvalid == 12345;
    }
#endif
    // void setwhere() {pos.modif = currsetwhere;}
    int getid() const {
      return id;
    }
    filepos& getpos() {
      return pos;
    }
    virtual void initmodval() {
      assert(false);
    }
    virtual fint getival() const {
      throwgtype("integer", getname(), pos);
      throw typeerr();
    }
    virtual rat getrval() const {
      throwgtype("rational", getname(), pos);
      throw typeerr();
    }
    virtual ffloat getfval() const {
      throwgtype("float", getname(), pos);
      throw typeerr();
    }
    virtual const numb& getnval() const {
      throwgtype("number", getname(), pos);
      throw typeerr();
    }
    virtual const std::string& getsval() const {
      throwgtype("string", getname(), pos);
      throw typeerr();
    }
    virtual const listelvect& getvectval() const {
      throwgtype("list", getname(), pos);
      throw typeerr();
    }
    virtual const listelmap& getmapval() const {
      throwgtype("map", getname(), pos);
      throw typeerr();
    }

    virtual varbase* getnew(const fint v, const filepos& p) const {
      throwstype(getname(), gettypedoc(), p);
      throw typeerr();
    }
    virtual varbase* getnew(const rat& val, const filepos& p) const {
      throwstype(getname(), gettypedoc(), p);
      throw typeerr();
    }
    virtual varbase* getnew(const ffloat v, const filepos& p) const {
      throwstype(getname(), gettypedoc(), p);
      throw typeerr();
    }
    virtual varbase* getnew(const numb& v, const filepos& p) const {
      throwstype(getname(), gettypedoc(), p);
      throw typeerr();
    }
    virtual varbase* getnewstr(fomusdata* fd, const char* str,
                               const filepos& p) const {
      throwstype(getname(), gettypedoc(), p);
      throw typeerr();
    }
    virtual varbase* getnewstr(fomusdata* fd, const std::string& str,
                               const filepos& p) const {
      throwstype(getname(), gettypedoc(), p);
      throw typeerr();
    }
    virtual varbase* getnew(const char* str, const filepos& p) const {
      return getnewstr(0, str, p);
    }
    virtual varbase* getnew(const std::string& str, const filepos& p) const {
      return getnewstr(0, str, p);
    }
    virtual varbase* getnew(const listelvect& val, const filepos& p) const {
      throwstype(getname(), gettypedoc(), p);
      throw typeerr();
    }
    virtual varbase* getnew(const listelmap& val, const filepos& p) const {
      throwstype(getname(), gettypedoc(), p);
      throw typeerr();
    }
    virtual varbase* getnewprepend(const listelvect& val, const filepos& p) {
      throwapp(getname(), p);
      throw typeerr();
    } // the getnew function, put appends itself
    virtual varbase* getnewprepend(const globpercsvarvect& val,
                                   const filepos& p) {
      throwapp(getname(), p);
      throw typeerr();
    } // the getnew function, put appends itself
    virtual varbase* getnewprepend(const globinstsvarvect& val,
                                   const filepos& p) {
      throwapp(getname(), p);
      throw typeerr();
    } // the getnew function, put appends itself
    virtual varbase* getnew(globpercsvarvect& v, const filepos& p) const {
      throwstype(getname(), gettypedoc(), p);
      throw typeerr();
    }
    virtual varbase* getnew(globinstsvarvect& v, const filepos& p) const {
      throwstype(getname(), gettypedoc(), p);
      throw typeerr();
    }

    std::string bad() const {
      std::ostringstream s;
      s << "expected `" << gettypedoc() << "' for setting `" << getname()
        << '\'';
      return s.str();
    }

    virtual void activate(const symtabs& ta) const {}

    void addsymbol(boostspirit::symbols<parserule*>& syms,
                   parserule* rules) const {
      syms.add(getname(), rules + getid());
    }
    virtual void addconfrule(parserule* rules, confscratch& x) {
      assert(false);
    }
    virtual void addconfrulestr(parserule* rules, confscratch& x,
                                boost::shared_ptr<import_str>& map) {
      assert(false);
    }
    virtual void addconfrulestr(parserule* rules, confscratch& x,
                                boost::shared_ptr<export_str>& map) {
      assert(false);
    }
    virtual void addconfrulestr(parserule* rules, confscratch& x,
                                boost::shared_ptr<clef_str>& map) {
      assert(false);
    }
    virtual void addconfrulestr(parserule* rules, confscratch& x,
                                boost::shared_ptr<staff_str>& map) {
      assert(false);
    }
    virtual void addconfrulestr(parserule* rules, confscratch& x,
                                boost::shared_ptr<percinstr_str>& map) {
      assert(false);
    }
    virtual void addconfrulestr(parserule* rules, confscratch& x,
                                boost::shared_ptr<instr_str>& map) {
      assert(false);
    }
    void addnotallowedrule(parserule* rules, const char* typ, filepos& fp) {
      rules[getid()] = notallowed(getname(), typ, fp);
    }

    virtual bool isvalid(const fomusdata* fd) {
      assert(false);
    }
    bool checkvalid(const bool isval) const {
      if (!isval) {
        pos.printerr(getname());
        return false;
      }
      return true;
    } // pass it the result if isvalid or isvaliddeps--doesn't throw
    void throwifinvalid(const fomusdata* fd) {
      if (!checkvalid(isvalid(fd)))
        throw errbase(); /*throw validerr();*/
    }                    // for user input--throws
    bool isallowed(const module_setting_loc loc) const {
      return localallowed(loc, getloc());
    }

    virtual const std::string getmodsname() const {
      return getmodcname();
    }
    virtual const char* getmodcname() const {
      return "(fomus)";
    }
    virtual const char* getmodlongname() const {
      return "(fomus)";
    }
    virtual const char* getmodauthor() const {
      return "(fomus)";
    }
    virtual const char* getmoddoc() const {
      assert(false);
    }
    virtual enum module_type getmodtype() const {
      return module_nomodtype;
    }

    virtual const char* getname() const {
      assert(false);
    }
    virtual module_setting_loc getloc() const {
      assert(false);
    } // implement
    virtual int getuselevel() const {
      assert(false);
    } // implement
    virtual const char* getdescdoc() const {
      assert(false);
    }
    virtual const char* gettypedoc() const {
      assert(false);
    }
    virtual enum module_value_type gettype() const {
      assert(false);
    }

    bool partallowed() const {
      return getloc() == module_locpart;
    }
    bool noteallowed() const {
      return getloc() == module_locnote;
    }

    const module_value& getmodval() const {
      assert(!mval.notyet());
      return mval;
    } // returns cached value
    virtual const char* getnewvalstr(const fomusdata* fd,
                                     const char* st) const {
      assert(false);
    }
    virtual std::string getvalstr(const fomusdata* fd, const char* st) const {
      assert(false);
    }

    listelmap nestedlisttomap(const listelvect& vect, const filepos& pos) const;

    int getord() const {
      return pos.ord;
    }
    fint getline() const {
      return pos.line;
    }
    fint getcol() const {
      return pos.col;
    }
    info_setwhere getmodif() const {
      return pos.modif;
    }
    virtual bool getreset() const {
      return false;
    }
  };

  struct noplus {
    varbase& var;
    bool& fl;
    noplus(varbase& var, bool& fl) : var(var), fl(fl) {}
    void operator()(const parse_it& s1, const parse_it& s2) const;
  };

  // types
  typedef std::vector<boost::shared_ptr<varbase>> varsvect;
  typedef varsvect::iterator varsvect_it;
  typedef varsvect::const_iterator varsvect_constit;

  typedef std::map<std::string, varbase*, isiless> varsmap;
  typedef varsmap::value_type varsmap_val;
  typedef varsmap::iterator varsmap_it;
  typedef varsmap::const_iterator varsmap_constit;

  // vars
  extern varsvect vars; // from default/conffile
  extern varsmap varslookup;

  // (methods)
  inline varbase::varbase()
      : id(vars.size()), pos(-1, currsetwhere)
#ifndef NDEBUG
        ,
        debugvalid(12345)
#endif
  {
  }

  // extern boost::thread_specific_ptr<char> threadcharptr;
  inline const char* make_charptr(const std::string& str) {
    size_t l;
    threadcharptr.reset(new char[l = str.length() + 1]);
    return (const char*) memcpy(threadcharptr.get(), str.c_str(), l);
  }
  inline const char* make_charptr(const std::ostringstream& str) {
    return make_charptr(str.str());
  }
  inline const char* make_charptr(const char* str) {
    size_t l;
    threadcharptr.reset(new char[l = strlen(str) + 1]);
    return (const char*) memcpy(threadcharptr.get(), str, l);
  }
  inline const char* make_charptr0(const std::string& str) {
    size_t l;
    char* c = new char[l = str.length() + 1];
    return (const char*) memcpy(c, str.c_str(), l);
  }
  inline const char* make_charptr0(const std::ostringstream& str) {
    return make_charptr0(str.str());
  }
  inline const char* make_charptr0(const char* str) {
    size_t l;
    char* c = new char[l = strlen(str) + 1];
    return (const char*) memcpy(c, str, l);
  }
  inline void delete_charptr0(const char* str) {
    delete[] str;
  }
  inline module_value* newmodvals(const int sz) {
    return sz > 0 ? new module_value[sz] : 0;
  }
  inline void deletemodvals(const module_value* vals) {
    delete[] vals;
  }

  struct badparse {
    varbase& var;
    badparse(varbase& var) : var(var) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      throw_(s1, &var.getpos());
    }
  };
  struct badvar {
    filepos& pos;
    badvar(filepos& pos) : pos(pos) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      throw_(s1, &pos);
    }
  };

  template <typename V>
  struct valid_f {
    typedef boostspirit::nil_t result_t;
    varbase& var;
    confscratch& x;
    V& val;
    const bool makenew;
    valid_f(varbase& var, confscratch& x, V& val, const bool makenew)
        : var(var), x(x), val(val), makenew(makenew) {}
    template <typename ScannerT>
    std::ptrdiff_t operator()(ScannerT const& scan, result_t& result) const {
      if (makenew) {
        x.newvar.reset(var.getnew(val, x.pos));
        return x.newvar->isvalid(x.ta.fd) ? 0 : -1;
      } else {
        assert(var.mvalexists());
        var.resetmval(); // need these two lines when parsing and storing in
                         // place
        var.initmodval();
        assert(var.mvalexists());
#ifndef NDEBUG
        x.newvar.reset();
#endif
        return var.isvalid(x.ta.fd) ? 0 : -1;
      }
    }
  };

  // classes
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      numvarparse,
      (5, (((numb&), val), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col,
                pos.modif)[noplus(var, x.isplus)] >>
          (((numbermatch(val, x.pt1, x.pt2, x.pt3, x.pos, ferr) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<numb>>(
                valid_f<numb>(var, x, val, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class numvar : public varbase {
protected:
    numb val;

public:
    numvar(const numb& val) : varbase(), val(val) { /*initmodval();*/
    }
    numvar(const numvar& x, const filepos& pos)
        : varbase(x, pos), val(x.val) { /*initmodval();*/
    }
    numvar(const numvar& x, const numb& val, const filepos& pos)
        : varbase(x, pos), val(val) { /*initmodval();*/
    }

    fint getival() const {
      return numtoint(val);
    }
    rat getrval() const {
      return numtorat(val);
    }
    ffloat getfval() const {
      return numtofloat(val);
    }
    const numb& getnval() const {
      return val;
    }

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = numvarparse(val, *this, pos, x, false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] = numvarparse(
          x.num, *this, x.pos, x, true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] = numvarparse(
          x.num, *this, x.pos, x, true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = numvarparse(x.num, *this, x.pos, x,
                                   true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = numvarparse(x.num, *this, x.pos, x,
                                   true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] =
          numvarparse(x.num, *this, x.pos, x,
                      true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = numvarparse(x.num, *this, x.pos, x,
                                   true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    const char* getname() const = 0;
    const char* getdescdoc() const = 0;
    const char* gettypedoc() const {
      return "number";
    }
    enum module_value_type gettype() const {
      return module_number;
    }

    void initmodval() {
      assert(mval.notyet());
      mval = val.modval();
    } // can't be a list
    const char* getnewvalstr(const fomusdata* fd, const char* st) const {
      return make_charptr0(getvalstr(fd, st));
    }
    std::string getvalstr(const fomusdata* fd, const char* st) const {
      std::ostringstream s;
      s.setf(std::ios_base::fixed, std::ios_base::floatfield);
      s << std::setprecision(3) << val;
      return s.str();
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      boolvarparse,
      (5, (((numb&), val), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col,
                pos.modif)[noplus(var, x.isplus)] >>
          (((boolmatch(val) | boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<numb>>(
                valid_f<numb>(var, x, val, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class boolvar
      : public numvar { // this is just a numvar with a different parser
public:
    boolvar(const numb& val) : numvar(val) {}
    boolvar(const boolvar& x, const filepos& pos) : numvar(x, pos) {}
    boolvar(const boolvar& x, const numb& val, const filepos& pos)
        : numvar(x, val, pos) {}

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = boolvarparse(val, *this, pos, x, false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] = boolvarparse(
          x.num, *this, x.pos, x, true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] = boolvarparse(
          x.num, *this, x.pos, x, true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = boolvarparse(x.num, *this, x.pos, x,
                                    true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = boolvarparse(
          x.num, *this, x.pos, x, true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] =
          boolvarparse(x.num, *this, x.pos, x,
                       true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = boolvarparse(
          x.num, *this, x.pos, x, true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    varbase* getnewstr(fomusdata* fd, const char* str, const filepos& p) const {
      fint* x = find(boolsyms,
                     boost::algorithm::to_lower_copy(std::string(str)).c_str());
      if (!x) {
        throwstype(getname(), gettypedoc(), p);
        throw typeerr();
      }
      return getnew(*x, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& str,
                       const filepos& p) const {
      fint* x = find(boolsyms, boost::algorithm::to_lower_copy(str).c_str());
      if (!x) {
        throwstype(getname(), gettypedoc(), p);
        throw typeerr();
      }
      return getnew(*x, p);
    }

    const char* gettypedoc() const {
      return "yes|no";
    }

    const char* getnewvalstr(const fomusdata* fd, const char* st) const {
      assert(numb(mval).isint());
      // if (val.getbool()) return make_charptr0("yes"); else return
      // make_charptr0("no");
      return make_charptr0(val.getbool() ? "yes" : "no");
    }
    std::string getvalstr(const fomusdata* fd, const char* st) const {
      assert(numb(mval).isint());
      if (val.getbool())
        return "yes";
      else
        return "no";
    }
    enum module_value_type gettype() const {
      return module_bool;
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      assert(numb(mval).isint());
      return true;
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      notevarparse,
      (5, (((numb&), val), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col,
                pos.modif)[noplus(var, x.isplus)] >>
          (((notematch(x.ta.note, x.ta.acc, x.ta.mic, x.ta.oct, val, x.pt1,
                       x.pt2, x.pt3, x.pos, ferr) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<numb>>(
                valid_f<numb>(var, x, val, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class notevar
      : public numvar { // this is just a numvar with a different parser
public:
    notevar(const numb& val) : numvar(val) {}
    notevar(const notevar& x, const filepos& pos) : numvar(x, pos) {}
    notevar(const notevar& x, const numb& val, const filepos& pos)
        : numvar(x, val, pos) {}

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = notevarparse(val, *this, pos, x, false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] = notevarparse(
          x.num, *this, x.pos, x, true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] = notevarparse(
          x.num, *this, x.pos, x, true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = notevarparse(x.num, *this, x.pos, x,
                                    true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = notevarparse(
          x.num, *this, x.pos, x, true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] =
          notevarparse(x.num, *this, x.pos, x,
                       true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = notevarparse(
          x.num, *this, x.pos, x, true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    varbase* getnewstr(fomusdata* fd, const char* str, const filepos& p) const {
      return getnew(doparsenote(fd, str), p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& str,
                       const filepos& p) const {
      return getnew(doparsenote(fd, str.c_str()), p);
    }

    enum module_value_type gettype() const {
      return module_notesym;
    }
    const char* getnewvalstr(const fomusdata* fd, const char* st) const {
      return make_charptr0(getvalstr(fd, st));
    } // declared inline in data.h
    std::string getvalstr(const fomusdata* fd, const char* st) const;
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      strvarparse,
      (6,
       (((std::string&), str), ((varbase&), var), ((filepos&), pos),
        ((confscratch&), x), ((const char*), ctxt), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col,
                pos.modif)[noplus(var, x.isplus)] >>
          (((strmatch(str, ctxt) | boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<std::string>>(
                valid_f<std::string>(var, x, str, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class strvar : public varbase {
protected:
    std::string str;

public:
    strvar(const std::string& str) : varbase(), str(str) { /*initmodval();*/
    }
    strvar(const strvar& x, const filepos& pos)
        : varbase(x, pos), str(x.str) { /*initmodval();*/
    }
    strvar(const strvar& x, const std::string& str, const filepos& pos)
        : varbase(x, pos), str(str) { /*initmodval();*/
    }

    const std::string& getsstr() const {
      return str;
    }

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = strvarparse(str, *this, pos, x, "", false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] =
          strvarparse(x.str, *this, x.pos, x, ")>,",
                      true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] =
          strvarparse(x.str, *this, x.pos, x, ")>,",
                      true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = strvarparse(x.str, *this, x.pos, x, ")>,",
                                   true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = strvarparse(x.str, *this, x.pos, x, ")>,",
                                   true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] =
          strvarparse(x.str, *this, x.pos, x, ")>,",
                      true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = strvarparse(x.str, *this, x.pos, x, ")>,",
                                   true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    const char* getname() const = 0;
    const char* getdescdoc() const = 0;
    const char* gettypedoc() const {
      return "string";
    }
    enum module_value_type gettype() const {
      return module_string;
    }

    const std::string& getsval() const {
      return str;
    }

    void initmodval() {
      assert(mval.notyet());
      mval.type = module_string;
      mval.val.s = str.c_str();
    }
    const char* getnewvalstr(const fomusdata* fd, const char* st) const {
      return make_charptr0(getvalstr(fd, st));
    }
    std::string getvalstr(const fomusdata* fd, const char* st) const {
      return stringify(str, st);
    }
  };

  void modvaltolistelvect(const modbase& mod, const module_setting& set,
                          listelvect& el);
  void modvaltolistelmap(const modbase& mod, const module_setting& set,
                         listelmap& el);

  class listvarbase : public varbase {
protected:
    listelvect el;

public:
    listvarbase() : varbase() { /*initmodval();*/
    }
    listvarbase(const modbase& mod, const module_setting& set) : varbase() {
      modvaltolistelvect(mod, set, el); /*initmodval();*/
    }
    listvarbase(const listelvect& el) : varbase(), el(el) { /*initmodval();*/
    }
    listvarbase(const listvarbase& x, const filepos& pos)
        : varbase(x, pos), el(x.el) { /*initmodval();*/
    }
    listvarbase(const listvarbase& x, const listelvect& el, const filepos& pos)
        : varbase(x, pos), el(el) { /*initmodval();*/
    }

    const char* getname() const = 0;
    const char* getdescdoc() const = 0;
    const listelvect& getvectval() const {
      return el;
    }

    virtual void initmodval();
    // const char* getnewvalstr(const fomusdata& fd) const;
    const char* getnewvalstr(const fomusdata* fd, const char* st) const {
      return make_charptr0(getvalstr(fd, st));
    }
    std::string getvalstr(const fomusdata* fd, const char* st) const {
      std::ostringstream s;
      s << el;
      return s.str();
    }
    varbase*
    getnewprepend(const listelvect& val,
                  const filepos& p) { // the getnew function, put appends itself
      listelvect z(el);
      std::copy(val.begin(), val.end(), back_inserter(z));
      return getnew(z, p);
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      listvarofnumsparse,
      (5, (((listelvect&), el), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col, pos.modif) >>
          (((listmatchnums(el, x.num, x.pt1, x.pt2, x.pt3, x.isplus, x.pos,
                           ferr) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<listelvect>>(
                valid_f<listelvect>(var, x, el, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class listvarofnums : public listvarbase {
public:
    listvarofnums() : listvarbase() {}
    listvarofnums(const listelvect& el) : listvarbase(el) {}
    listvarofnums(const modbase& mod, const module_setting& set)
        : listvarbase(mod, set) {}
    listvarofnums(const listvarofnums& x, const filepos& pos)
        : listvarbase(x, pos) {}
    listvarofnums(const listvarofnums& x, const listelvect& el,
                  const filepos& pos)
        : listvarbase(x, el, pos) {}

    varbase* getnew(const fint v, const filepos& p) const {
      return ((const varbase*) this)->getnew(listelvect(1, v), p);
    }
    varbase* getnew(const rat& val, const filepos& p) const {
      return ((const varbase*) this)->getnew(listelvect(1, val), p);
    }
    varbase* getnew(const ffloat v, const filepos& p) const {
      return ((const varbase*) this)->getnew(listelvect(1, v), p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return ((const varbase*) this)->getnew(listelvect(1, v), p);
    }

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = listvarofnumsparse(el, *this, pos, x, false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] = listvarofnumsparse(
          x.lst, *this, x.pos, x,
          true)[sticknewvar<import_str>(x.newvar, map)]; // [] is an action
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] = listvarofnumsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = listvarofnumsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = listvarofnumsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] =
          listvarofnumsparse(x.lst, *this, x.pos, x,
                             true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = listvarofnumsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    const char* gettypedoc() const {
      return "(number number ...)";
    }
    enum module_value_type gettype() const {
      return module_list_nums;
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      listvarofstringsparse,
      (6, (((listelvect&), el), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const char*), end), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col, pos.modif) >>
          (((listmatchstrs(el, x.str, x.isplus, end) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<listelvect>>(
                valid_f<listelvect>(var, x, el, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class listvarofstrings : public listvarbase {
public:
    listvarofstrings() : listvarbase() {}
    listvarofstrings(const listelvect& el) : listvarbase(el) {}
    listvarofstrings(const modbase& mod, const module_setting& set)
        : listvarbase(mod, set) {}
    listvarofstrings(const listvarofstrings& x, const filepos& pos)
        : listvarbase(x, pos) {}
    listvarofstrings(const listvarofstrings& x, const listelvect& el,
                     const filepos& pos)
        : listvarbase(x, el, pos) {}

    varbase* getnewstr(fomusdata* fd, const char* str, const filepos& p) const {
      return ((const varbase*) this)->getnew(listelvect(1, str), p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& str,
                       const filepos& p) const {
      return ((const varbase*) this)->getnew(listelvect(1, str), p);
    }

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = listvarofstringsparse(el, *this, pos, x, "),", false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] =
          listvarofstringsparse(x.lst, *this, x.pos, x, ")>,",
                                true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] =
          listvarofstringsparse(x.lst, *this, x.pos, x, ")>,",
                                true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] =
          listvarofstringsparse(x.lst, *this, x.pos, x, ")>,",
                                true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] =
          listvarofstringsparse(x.lst, *this, x.pos, x, ")>,",
                                true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] = listvarofstringsparse(
          x.lst, *this, x.pos, x, ")>,",
          true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] =
          listvarofstringsparse(x.lst, *this, x.pos, x, ")>,",
                                true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    const char* gettypedoc() const {
      return "(string string ...)";
    }
    enum module_value_type gettype() const {
      return module_list_strings;
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      listvaroflistofnumsparse,
      (5, (((listelvect&), el), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col, pos.modif) >>
          (((listmatchlistsofnums(x.autolst, x.num, x.pt1, x.pt2, x.pt3,
                                  x.isplus, x.pos, el, ferr) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<listelvect>>(
                valid_f<listelvect>(var, x, el, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class listvaroflistofnums : public listvarbase {
public:
    listvaroflistofnums() : listvarbase() {}
    listvaroflistofnums(const listelvect& el) : listvarbase(el) {}
    listvaroflistofnums(const modbase& mod, const module_setting& set)
        : listvarbase(mod, set) {}
    listvaroflistofnums(const listvaroflistofnums& x, const filepos& pos)
        : listvarbase(x, pos) {}
    listvaroflistofnums(const listvaroflistofnums& x, const listelvect& el,
                        const filepos& pos)
        : listvarbase(x, el, pos) {}

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = listvaroflistofnumsparse(el, *this, pos, x, false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] = listvaroflistofnumsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] = listvaroflistofnumsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = listvaroflistofnumsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = listvaroflistofnumsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] = listvaroflistofnumsparse(
          x.lst, *this, x.pos, x,
          true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = listvaroflistofnumsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    const char* gettypedoc() const {
      return "((number number ...) (number number ...) ...)";
    }
    enum module_value_type gettype() const {
      return module_list_numlists;
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      listvaroflistofstringsparse,
      (5, (((listelvect&), el), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col, pos.modif) >>
          (((listmatchlistsofstrs(x.autolst, x.str, x.isplus, x.pos, el) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<listelvect>>(
                valid_f<listelvect>(var, x, el, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class listvaroflistofstrings : public listvarbase {
public:
    listvaroflistofstrings() : listvarbase() {}
    listvaroflistofstrings(const listelvect& el) : listvarbase(el) {}
    listvaroflistofstrings(const modbase& mod, const module_setting& set)
        : listvarbase(mod, set) {}
    listvaroflistofstrings(const listvaroflistofstrings& x, const filepos& pos)
        : listvarbase(x, pos) {}
    listvaroflistofstrings(const listvaroflistofstrings& x,
                           const listelvect& el, const filepos& pos)
        : listvarbase(x, el, pos) {}

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = listvaroflistofstringsparse(el, *this, pos, x, false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] = listvaroflistofstringsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] = listvaroflistofstringsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = listvaroflistofstringsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = listvaroflistofstringsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] = listvaroflistofstringsparse(
          x.lst, *this, x.pos, x,
          true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = listvaroflistofstringsparse(
          x.lst, *this, x.pos, x, true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    const char* gettypedoc() const {
      return "((string string ...) (string string ...) ...)";
    }
    enum module_value_type gettype() const {
      return module_list_stringlists;
    }
  };

  class mapvarbase : public varbase {
protected:
    listelmap el;

public:
    mapvarbase() : varbase() { /*initmodval();*/
    }
    mapvarbase(const modbase& mod, const module_setting& set) : varbase() {
      modvaltolistelmap(mod, set, el); /*initmodval();*/
    }
    mapvarbase(const listelmap& el) : varbase(), el(el) { /*initmodval();*/
    }
    mapvarbase(const mapvarbase& x, const filepos& pos)
        : varbase(x, pos), el(x.el) { /*initmodval();*/
    }
    mapvarbase(const mapvarbase& x, const listelmap& el, const filepos& pos)
        : varbase(x, pos), el(el) { /*initmodval();*/
    }

    const char* getname() const = 0;
    const char* getdescdoc() const = 0;
    const listelmap& getmapval() const {
      return el;
    }

    virtual void initmodval();
    const char* getnewvalstr(const fomusdata* fd, const char* st) const {
      return make_charptr0(getvalstr(fd, st));
    }
    std::string getvalstr(const fomusdata* fd, const char* st) const;
    varbase* getnewprepend(const listelvect& val, const filepos& p) { // FIXME
      listelvect z;
      for_each2(el.begin(), el.end(), back_inserter(z),
                (boost::lambda::_2 = boost::lambda::bind(&listelmap_val::first,
                                                         boost::lambda::_1),
                 boost::lambda::_2 = boost::lambda::bind(&listelmap_val::second,
                                                         boost::lambda::_1)));
      std::copy(val.begin(), val.end(), back_inserter(z));
      return getnew(z, p);
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapvartonumsparse,
      (5, (((listelmap&), el), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col, pos.modif) >>
          (((mapmatchnums(el, x.lst, x.name, x.num, x.pt1, x.pt2, x.pt3,
                          x.isplus, x.pos, ferr) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<listelmap>>(
                valid_f<listelmap>(var, x, el, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class mapvartonums : public mapvarbase {
public:
    mapvartonums() : mapvarbase() {}
    mapvartonums(const listelmap& el) : mapvarbase(el) {}
    mapvartonums(const modbase& mod, const module_setting& set)
        : mapvarbase(mod, set) {}
    mapvartonums(const mapvartonums& x, const filepos& pos)
        : mapvarbase(x, pos) {}
    mapvartonums(const mapvartonums& x, const listelmap& el, const filepos& pos)
        : mapvarbase(x, el, pos) {}

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = mapvartonumsparse(el, *this, pos, x, false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] = mapvartonumsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] = mapvartonumsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = mapvartonumsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = mapvartonumsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] =
          mapvartonumsparse(x.map, *this, x.pos, x,
                            true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = mapvartonumsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    const char* gettypedoc() const {
      return "(string number, string number, ...)";
    }
    enum module_value_type gettype() const {
      return module_symmap_nums;
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapvartostringsparse,
      (5, (((listelmap&), el), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col, pos.modif) >>
          (((mapmatchstrs(el, x.lst, x.name, x.str, x.isplus) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<listelmap>>(
                valid_f<listelmap>(var, x, el, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class mapvartostrings : public mapvarbase {
public:
    mapvartostrings() : mapvarbase() {}
    mapvartostrings(const listelmap& el) : mapvarbase(el) {}
    mapvartostrings(const modbase& mod, const module_setting& set)
        : mapvarbase(mod, set) {}
    mapvartostrings(const mapvartostrings& x, const filepos& pos)
        : mapvarbase(x, pos) {}
    mapvartostrings(const mapvartostrings& x, const listelmap& el,
                    const filepos& pos)
        : mapvarbase(x, el, pos) {}

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = mapvartostringsparse(el, *this, pos, x, false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] = mapvartostringsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] = mapvartostringsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = mapvartostringsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = mapvartostringsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] =
          mapvartostringsparse(x.map, *this, x.pos, x,
                               true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = mapvartostringsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    const char* gettypedoc() const {
      return "(string string, string string, ...)";
    }
    enum module_value_type gettype() const {
      return module_symmap_strings;
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapvaroflistofnumsparse,
      (5, (((listelmap&), el), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col, pos.modif) >>
          (((mapmatchlistsofnums(el, x.lst, x.name, x.num, x.pt1, x.pt2, x.pt3,
                                 x.isplus, x.pos, x.autolst, ferr) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<listelmap>>(
                valid_f<listelmap>(var, x, el, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class mapvaroflistofnums : public mapvarbase {
public:
    mapvaroflistofnums() : mapvarbase() {}
    mapvaroflistofnums(const listelmap& el) : mapvarbase(el) {}
    mapvaroflistofnums(const modbase& mod, const module_setting& set)
        : mapvarbase(mod, set) {}
    mapvaroflistofnums(const mapvaroflistofnums& x, const filepos& pos)
        : mapvarbase(x, pos) {}
    mapvaroflistofnums(const mapvaroflistofnums& x, const listelmap& el,
                       const filepos& pos)
        : mapvarbase(x, el, pos) {}

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = mapvaroflistofnumsparse(el, *this, pos, x, false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] = mapvaroflistofnumsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] = mapvaroflistofnumsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = mapvaroflistofnumsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = mapvaroflistofnumsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] = mapvaroflistofnumsparse(
          x.map, *this, x.pos, x,
          true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = mapvaroflistofnumsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    const char* gettypedoc() const {
      return "(string (number number ...), string (number number ...) ...)";
    }
    enum module_value_type gettype() const {
      return module_symmap_numlists;
    }
  };
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      mapvaroflistofstringsparse,
      (5, (((listelmap&), el), ((varbase&), var), ((filepos&), pos),
           ((confscratch&), x), ((const bool), makenew))),
      -,
      recerrpos(pos.file, pos.line, pos.col, pos.modif) >>
          (((mapmatchlistsofstrs(el, x.lst, x.name, x.str, x.isplus,
                                 x.autolst) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<listelmap>>(
                valid_f<listelmap>(var, x, el, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class mapvaroflistofstrings : public mapvarbase {
public:
    mapvaroflistofstrings() : mapvarbase() {}
    mapvaroflistofstrings(const listelmap& el) : mapvarbase(el) {}
    mapvaroflistofstrings(const modbase& mod, const module_setting& set)
        : mapvarbase(mod, set) {}
    mapvaroflistofstrings(const mapvaroflistofstrings& x, const filepos& pos)
        : mapvarbase(x, pos) {}
    mapvaroflistofstrings(const mapvaroflistofstrings& x, const listelmap& el,
                          const filepos& pos)
        : mapvarbase(x, el, pos) {}

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = mapvaroflistofstringsparse(el, *this, pos, x, false);
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<import_str>& map) {
      rules[getid()] = mapvaroflistofstringsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<import_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<export_str>& map) {
      rules[getid()] = mapvaroflistofstringsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<export_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<clef_str>& map) {
      rules[getid()] = mapvaroflistofstringsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<clef_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<staff_str>& map) {
      rules[getid()] = mapvaroflistofstringsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<staff_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<percinstr_str>& map) {
      rules[getid()] = mapvaroflistofstringsparse(
          x.map, *this, x.pos, x,
          true)[sticknewvar<percinstr_str>(x.newvar, map)];
    }
    void addconfrulestr(parserule* rules, confscratch& x,
                        boost::shared_ptr<instr_str>& map) {
      rules[getid()] = mapvaroflistofstringsparse(
          x.map, *this, x.pos, x, true)[sticknewvar<instr_str>(x.newvar, map)];
    }

    const char* gettypedoc() const {
      return "(string (string string ...), string (string string ...) ...)";
    }
    enum module_value_type gettype() const {
      return module_symmap_stringlists;
    }
  };

  // MODVARS
#define MODVAR_FUNS                                                            \
  const std::string getmodsname() const {                                      \
    return mod.getsname();                                                     \
  }                                                                            \
  const char* getmodcname() const {                                            \
    return mod.getcname();                                                     \
  }                                                                            \
  const char* getmodlongname() const {                                         \
    return mod.getlongname();                                                  \
  }                                                                            \
  const char* getmodauthor() const {                                           \
    return mod.getauthor();                                                    \
  }                                                                            \
  const char* getmoddoc() const {                                              \
    return mod.getdoc();                                                       \
  }                                                                            \
  enum module_type getmodtype() const {                                        \
    return mod.gettype();                                                      \
  }                                                                            \
  const char* getname() const {                                                \
    return varname;                                                            \
  }                                                                            \
  module_setting_loc getloc() const {                                          \
    return loc;                                                                \
  }                                                                            \
  int getuselevel() const {                                                    \
    return uselevel;                                                           \
  }                                                                            \
  const char* getdescdoc() const {                                             \
    return vardescdoc;                                                         \
  }
  // bool isvalidwdeps(fomusdata* fd) const {if (modvar::isvaliddeps != 0)
  // {assert(!mval.notyet()); return validwrap(modvar::isvaliddeps(fd, mval));}
  // else return true;}

  class modvar {
protected:
    const modbase& mod;
    const char* varname;    // module var name
    const char* vardescdoc; // var doc
    const char* vardoctype; // var type doc
    module_setting_loc loc;
    int uselevel;
    module_valid_fun isvalid;
    // module_validdeps_fun isvaliddeps;
public:
    modvar(const modbase& mod, const module_setting& set)
        : mod(mod), varname(set.name), vardescdoc(set.descdoc),
          vardoctype(set.typedoc), loc(set.loc), uselevel(set.uselevel),
          isvalid(set.valid) /*, isvaliddeps(set.validdeps)*/ {}
    bool validwrap(const bool v) const {
      if (!v) {
        const char* e = mod.getiniterr();
        if (e)
          CERR << e; // otherwise, assume module has properly asked helper funs
                     // to print message
        return false;
      }
      return true;
    }
  };

  class var_modstr : public strvar, public modvar {
public:
    var_modstr(const modbase& mod, const module_setting& set)
        : strvar(set.val.val.s ? set.val.val.s : ""), modvar(mod, set) {
      initmodval();
    }
    var_modstr(const var_modstr& x, const filepos& pos)
        : strvar(x, pos), modvar(x) {
      initmodval();
    }
    var_modstr(const var_modstr& x, const std::string& str, const filepos& pos)
        : strvar(x, str, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : strvar::gettypedoc();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_modstr(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_modstr(*this, s, p);
    }
    //   bool isvalid() {if (modvar::isvalid != 0) {assert(!mval.notyet());
    //   return validwrap(modvar::isvalid(mval));} else return true;} bool
    //   isvalidwdeps(fomusdata* fd) const {if (modvar::isvaliddeps != 0)
    //   {assert(!mval.notyet()); return validwrap(modvar::isvaliddeps(fd,
    //   &mval));} else return true;}
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return true;
    }
  };

  class var_modnum : public numvar, public modvar {
public:
    var_modnum(const numb& val, const modbase& mod, const module_setting& set)
        : numvar(val), modvar(mod, set) {
      initmodval();
    }
    var_modnum(const var_modnum& x, const filepos& pos)
        : numvar(x, pos), modvar(x) {
      initmodval();
    }
    var_modnum(const var_modnum& x, const numb& val, const filepos& pos)
        : numvar(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : numvar::gettypedoc();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_modnum(*this, v, p);
    }
    varbase* getnew(const rat& v, const filepos& p) const {
      return new var_modnum(*this, v, p);
    }
    varbase* getnew(const ffloat v, const filepos& p) const {
      return new var_modnum(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_modnum(*this, v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return true;
    }
  };

  class var_modbool : public boolvar, public modvar {
public:
    var_modbool(const numb& val, const modbase& mod, const module_setting& set)
        : boolvar(val), modvar(mod, set) {
      initmodval();
    }
    var_modbool(const var_modbool& x, const filepos& pos)
        : boolvar(x, pos), modvar(x) {
      initmodval();
    }
    var_modbool(const var_modbool& x, const numb& val, const filepos& pos)
        : boolvar(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : boolvar::gettypedoc();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_modbool(*this, v, p);
    }
    varbase* getnew(const rat& v, const filepos& p) const {
      return new var_modbool(*this, v, p);
    }
    varbase* getnew(const ffloat v, const filepos& p) const {
      return new var_modbool(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_modbool(*this, v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return true;
    }
  };
  class var_modnote : public notevar, public modvar {
public:
    var_modnote(const numb& val, const modbase& mod, const module_setting& set)
        : notevar(val), modvar(mod, set) {
      initmodval();
    }
    var_modnote(const var_modnote& x, const filepos& pos)
        : notevar(x, pos), modvar(x) {
      initmodval();
    }
    var_modnote(const var_modnote& x, const numb& val, const filepos& pos)
        : notevar(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : numvar::gettypedoc();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_modnote(*this, v, p);
    }
    varbase* getnew(const rat& v, const filepos& p) const {
      return new var_modnote(*this, v, p);
    }
    varbase* getnew(const ffloat v, const filepos& p) const {
      return new var_modnote(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_modnote(*this, v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return true;
    }
  };

  class var_modlistofnums : public listvarofnums, public modvar {
public:
    var_modlistofnums(const modbase& mod, const module_setting& set)
        : listvarofnums(mod, set), modvar(mod, set) {
      initmodval();
    }
    var_modlistofnums(const modbase& mod, const module_setting& set,
                      const listelvect& val)
        : listvarofnums(val), modvar(mod, set) {
      initmodval();
    }
    var_modlistofnums(const var_modlistofnums& x, const filepos& pos)
        : listvarofnums(x, pos), modvar(x) {
      initmodval();
    }
    var_modlistofnums(const var_modlistofnums& x, const listelvect& val,
                      const filepos& pos)
        : listvarofnums(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : listvarofnums::gettypedoc();
    }

    varbase* getnew(const listelvect& v, const filepos& p) const {
      return new var_modlistofnums(*this, /*nestedlisttolist(v)*/ v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return module_valid_listofnums(mval, -1, -1, numb((fint) 0),
                                       module_nobound, numb((fint) 0),
                                       module_nobound, 0, gettypedoc());
    }
  };
  class var_modlistofstrings : public listvarofstrings, public modvar {
public:
    var_modlistofstrings(const modbase& mod, const module_setting& set)
        : listvarofstrings(mod, set), modvar(mod, set) {
      initmodval();
    }
    var_modlistofstrings(const modbase& mod, const module_setting& set,
                         const listelvect& val)
        : listvarofstrings(val), modvar(mod, set) {
      initmodval();
    }
    var_modlistofstrings(const var_modlistofstrings& x, const filepos& pos)
        : listvarofstrings(x, pos), modvar(x) {
      initmodval();
    }
    var_modlistofstrings(const var_modlistofstrings& x, const listelvect& val,
                         const filepos& pos)
        : listvarofstrings(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : listvarofstrings::gettypedoc();
    }

    varbase* getnew(const listelvect& v, const filepos& p) const {
      return new var_modlistofstrings(*this, /*nestedlisttolist(v)*/ v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return module_valid_listofstrings(mval, -1, -1, -1, -1, 0,
                                          gettypedoc());
    }
  };
  inline int valid_listnumlist(int n, struct module_value val) {
    return module_valid_listofnums(val, -1, -1, numb((fint) 0).modval(),
                                   module_nobound, numb((fint) 0).modval(),
                                   module_nobound, 0, 0);
  }
  class var_modlistofnumlists : public listvaroflistofnums, public modvar {
public:
    var_modlistofnumlists(const modbase& mod, const module_setting& set)
        : listvaroflistofnums(mod, set), modvar(mod, set) {
      initmodval();
    }
    var_modlistofnumlists(const modbase& mod, const module_setting& set,
                          const listelvect& val)
        : listvaroflistofnums(val), modvar(mod, set) {
      initmodval();
    }
    var_modlistofnumlists(const var_modlistofnumlists& x, const filepos& pos)
        : listvaroflistofnums(x, pos), modvar(x) {
      initmodval();
    }
    var_modlistofnumlists(const var_modlistofnumlists& x, const listelvect& val,
                          const filepos& pos)
        : listvaroflistofnums(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : listvaroflistofnums::gettypedoc();
    }

    varbase* getnew(const listelvect& v, const filepos& p) const {
      return new var_modlistofnumlists(*this, /*nestedlisttolist(v)*/ v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return module_valid_listofvals(mval, -1, -1, valid_listnumlist,
                                       gettypedoc());
    }
  };
  inline int valid_liststringlist(int n, struct module_value val) {
    return module_valid_listofstrings(val, -1, -1, -1, -1, 0, 0);
  }
  class var_modlistofstringlists : public listvaroflistofstrings,
                                   public modvar {
public:
    var_modlistofstringlists(const modbase& mod, const module_setting& set)
        : listvaroflistofstrings(mod, set), modvar(mod, set) {
      initmodval();
    }
    var_modlistofstringlists(const modbase& mod, const module_setting& set,
                             const listelvect& val)
        : listvaroflistofstrings(val), modvar(mod, set) {
      initmodval();
    }
    var_modlistofstringlists(const var_modlistofstringlists& x,
                             const filepos& pos)
        : listvaroflistofstrings(x, pos), modvar(x) {
      initmodval();
    }
    var_modlistofstringlists(const var_modlistofstringlists& x,
                             const listelvect& val, const filepos& pos)
        : listvaroflistofstrings(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : listvaroflistofstrings::gettypedoc();
    }

    varbase* getnew(const listelvect& v, const filepos& p) const {
      return new var_modlistofstringlists(*this, /*nestedlisttolist(v)*/ v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return module_valid_listofvals(mval, -1, -1, valid_liststringlist,
                                       gettypedoc());
    }
  };

  class var_modmaptonums : public mapvartonums, public modvar {
public:
    var_modmaptonums(const modbase& mod, const module_setting& set)
        : mapvartonums(mod, set), modvar(mod, set) {
      initmodval();
    }
    var_modmaptonums(const modbase& mod, const module_setting& set,
                     const listelmap& val)
        : mapvartonums(val), modvar(mod, set) {
      initmodval();
    }
    var_modmaptonums(const var_modmaptonums& x, const filepos& pos)
        : mapvartonums(x, pos), modvar(x) {
      initmodval();
    }
    var_modmaptonums(const var_modmaptonums& x, const listelmap& val,
                     const filepos& pos)
        : mapvartonums(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : mapvartonums::gettypedoc();
    }

    varbase* getnew(const listelvect& v, const filepos& p) const {
      return new var_modmaptonums(*this, nestedlisttomap(v, p), p);
    }
    varbase* getnew(const listelmap& v, const filepos& p) const {
      return new var_modmaptonums(*this, v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return module_valid_maptonums(mval, -1, -1, numb((fint) 0),
                                      module_nobound, numb((fint) 0),
                                      module_nobound, 0, gettypedoc());
    }
  };
  class var_modmaptostrings : public mapvartostrings, public modvar {
public:
    var_modmaptostrings(const modbase& mod, const module_setting& set)
        : mapvartostrings(mod, set), modvar(mod, set) {
      initmodval();
    }
    var_modmaptostrings(const modbase& mod, const module_setting& set,
                        const listelmap& val)
        : mapvartostrings(val), modvar(mod, set) {
      initmodval();
    }
    var_modmaptostrings(const var_modmaptostrings& x, const filepos& pos)
        : mapvartostrings(x, pos), modvar(x) {
      initmodval();
    }
    var_modmaptostrings(const var_modmaptostrings& x, const listelmap& val,
                        const filepos& pos)
        : mapvartostrings(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : mapvartostrings::gettypedoc();
    }

    varbase* getnew(const listelvect& v, const filepos& p) const {
      return new var_modmaptostrings(*this, nestedlisttomap(v, p), p);
    }
    varbase* getnew(const listelmap& v, const filepos& p) const {
      return new var_modmaptostrings(*this, v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return module_valid_maptostrings(mval, -1, -1, -1, -1, 0, gettypedoc());
    }
  };
  inline int valid_mapnumlist(int n, const char* sym, struct module_value val) {
    return module_valid_listofnums(val, -1, -1, numb((fint) 0), module_nobound,
                                   numb((fint) 0), module_nobound, 0, 0);
  }
  class var_modmaptonumlists : public mapvaroflistofnums, public modvar {
public:
    var_modmaptonumlists(const modbase& mod, const module_setting& set)
        : mapvaroflistofnums(mod, set), modvar(mod, set) {
      initmodval();
    }
    var_modmaptonumlists(const modbase& mod, const module_setting& set,
                         const listelmap& val)
        : mapvaroflistofnums(val), modvar(mod, set) {
      initmodval();
    }
    var_modmaptonumlists(const var_modmaptonumlists& x, const filepos& pos)
        : mapvaroflistofnums(x, pos), modvar(x) {
      initmodval();
    }
    var_modmaptonumlists(const var_modmaptonumlists& x, const listelmap& val,
                         const filepos& pos)
        : mapvaroflistofnums(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : mapvaroflistofnums::gettypedoc();
    }

    varbase* getnew(const listelvect& v, const filepos& p) const {
      return new var_modmaptonumlists(*this, nestedlisttomap(v, p), p);
    }
    varbase* getnew(const listelmap& v, const filepos& p) const {
      return new var_modmaptonumlists(*this, v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return module_valid_maptovals(mval, -1, -1, valid_mapnumlist,
                                      gettypedoc());
    }
  };
  inline int valid_mapstringlist(int n, const char* sym,
                                 struct module_value val) {
    return module_valid_listofstrings(val, -1, -1, -1, -1, 0, 0);
  }
  class var_modmaptostringlists : public mapvaroflistofstrings, public modvar {
public:
    var_modmaptostringlists(const modbase& mod, const module_setting& set)
        : mapvaroflistofstrings(mod, set), modvar(mod, set) {
      initmodval();
    }
    var_modmaptostringlists(const modbase& mod, const module_setting& set,
                            const listelmap& val)
        : mapvaroflistofstrings(val), modvar(mod, set) {
      initmodval();
    }
    var_modmaptostringlists(const var_modmaptostringlists& x,
                            const filepos& pos)
        : mapvaroflistofstrings(x, pos), modvar(x) {
      initmodval();
    }
    var_modmaptostringlists(const var_modmaptostringlists& x,
                            const listelmap& val, const filepos& pos)
        : mapvaroflistofstrings(x, val, pos), modvar(x) {
      initmodval();
    }

    const char* gettypedoc() const {
      return vardoctype ? vardoctype : mapvaroflistofstrings::gettypedoc();
    }

    varbase* getnew(const listelvect& v, const filepos& p) const {
      return new var_modmaptostringlists(*this, nestedlisttomap(v, p), p);
    }
    varbase* getnew(const listelmap& v, const filepos& p) const {
      return new var_modmaptostringlists(*this, v, p);
    }
    // bool isvalid(const fomusdata* fd) {return modvar::isvalid(mval);}
    MODVAR_FUNS
    bool isvalid(const fomusdata* fd) {
      if (modvar::isvalid != 0) {
        assert(!mval.notyet());
        return validwrap(modvar::isvalid(mval));
      } else
        return module_valid_maptovals(mval, -1, -1, valid_mapstringlist,
                                      gettypedoc());
    }
  };

  // exceptions
  //   class newmoderr:public errbase {};

  // types
  typedef std::map<const std::string, const int, isiless>
      varsptrmap; // from input file
  typedef varsptrmap::value_type varsptrmap_val;
  typedef varsptrmap::iterator varsptrmap_it;
  typedef varsptrmap::const_iterator varsptrmap_constit;

  // vars
  // extern bool conffileread;

  // functions
  void initvars(); // initialize default vales and call `loadconf'
  void loadconf(); // parse config file, modify default values

  // inline bool getdefaultbval(const int id) {return vars[id].getbval();}
  inline fint getdefaultival(const int id) {
    return vars[id]->getival();
  }
  inline ffloat getdefaultfval(const int id) {
    return vars[id]->getfval();
  }
  inline rat getdefaultrval(const int id) {
    return vars[id]->getrval();
  }
  inline const numb& getdefaultnum(const int id) {
    return vars[id]->getnval();
  }
  inline const std::string& getdefaultsval(const int id) {
    return vars[id]->getsval();
  }
  inline const listelvect& getdefaultvect(const int id) {
    return vars[id]->getvectval();
  }
  inline const listelmap& getdefaultmap(const int id) {
    return vars[id]->getmapval();
  }
  inline const module_value& getdefaultmodval(const int id) {
    return vars[id]->getmodval();
  }

  fint getdefaultival(const std::string& str);
  ffloat getdefaultfval(const std::string& str);
  rat getdefaultrval(const std::string& str);
  const std::string& getdefaultsval(const std::string& str);

  void addmodvar(const modbase& mod, const module_setting& s);

  // extern const asstn name_bad;
  inline fint getdefaultival(const std::string& str) {
    varsmap_constit i(varslookup.find(str));
    if (i == varslookup.end())
      throw errbase(); // throw novarfound();
    return i->second->getival();
  }
  inline ffloat getdefaultfval(const std::string& str) {
    varsmap_constit i(varslookup.find(str));
    if (i == varslookup.end())
      throw errbase(); // novarfound();
    return i->second->getfval();
  }
  inline rat getdefaultrval(const std::string& str) {
    varsmap_constit i(varslookup.find(str));
    if (i == varslookup.end())
      throw errbase(); // throw novarfound();
    return i->second->getrval();
  }
  inline const std::string& getdefaultsval(const std::string& str) {
    varsmap_constit i(varslookup.find(str));
    if (i == varslookup.end())
      throw errbase(); // throw novarfound();
    return i->second->getsval();
  }

  // ------------------------------------------------------------------------------------------------------------------------
  // extra stuff
  enum accruleenum { accrule_meas, accrule_note, accrule_notenats };
  typedef std::map<std::string, accruleenum, isiless> accrulestype;
  extern accrulestype accrules;

  template <typename T>
  struct activate_set {
    const T& var;
    const symtabs& ta;
    activate_set(const T& var, const symtabs& ta) : var(var), ta(ta) {}
    void operator()(const parse_it& s1, const parse_it& s2) const {
      var.activate(ta);
    }
  };

  class var_outputs : public listvarofstrings {
public:
    var_outputs() : listvarofstrings() {
      assert(getid() == OUTPUTS_ID);
      initmodval();
    }
    var_outputs(const listelvect& list) : listvarofstrings(list) {
      initmodval();
    }
    var_outputs(const var_outputs& x, const filepos& pos)
        : listvarofstrings(x, pos) {
      initmodval();
    }
    var_outputs(const var_outputs& x, const listelvect& s, const filepos& pos)
        : listvarofstrings(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_outputs(*this, s, p);
    }

    const char* getname() const {
      return "output";
    } // docscat{basic}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "One or more filename extensions, each indicating an output file "
             "format (one output file is written for each one of these)."
             "  Use this setting to output more than one file or set it in "
             "your `.fomus' init file to cause FOMUS to automatically output a "
             "certain file type."
             "  Example values are \"ly\" or (fms xml).";
    }
    const char* gettypedoc() const;
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofstrings(mval, -1, -1, -1, -1, 0, gettypedoc());
    }
  };

  class var_measparts : public listvarofstrings {
public:
    var_measparts() : listvarofstrings() {
      assert(getid() == PARTS_ID);
      initmodval();
    }
    var_measparts(const listelvect& list) : listvarofstrings(list) {
      initmodval();
    }
    var_measparts(const var_measparts& x, const filepos& pos)
        : listvarofstrings(x, pos) {
      initmodval();
    }
    var_measparts(const var_measparts& x, const listelvect& s,
                  const filepos& pos)
        : listvarofstrings(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_measparts(*this, s, p);
    }

    const char* getname() const {
      return "measparts";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "A list of parts to which a measure definition belongs.  "
             "By default, all measure and measure definitions are assigned to "
             "all parts.  "
             "Changing this setting value to a part id or list of part ids in "
             "a measure definition assigns that measure definition to only "
             "those parts.  "
             "You can use this setting, for example, to create polymeters.";
    }
    const char* gettypedoc() const {
      return "string_partid | (string_partid string_partid ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofstrings(mval, -1, -1, 1, -1, 0, gettypedoc());
    }
  };

  class var_clef : public strvar {
public:
    var_clef() : strvar("treble") {
      assert(getid() == CLEF_ID);
      initmodval();
    }
    var_clef(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_clef(const var_clef& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_clef(const var_clef& x, const std::string& s, const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_clef(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_clef(*this, s, p);
    }

    const char* getname() const {
      return "instclef";
    } // docscat{instsparts}
    module_setting_loc getloc() const {
      return module_locclef;
    }
    int getuselevel() const {
      return 2;
    } //
    const char* getdescdoc() const {
      return "A clef signature (used for instrument/part definitions and "
             "placed inside a clef object to specify what clef signature the "
             "object represents).  "
             "A clef object should always contain at least this setting.";
    }
    const char* gettypedoc() const {
      return clefstypestr.c_str();
    }
    bool isvalid(const fomusdata* fd) {
      if (isvalidclef(str))
        return true;
      CERR << "invalid clef signature";
      return false;
    }
  };

  class var_verbosity : public numvar {
public:
    var_verbosity() : numvar((fint) 1) {
      assert(getid() == VERBOSE_ID);
      initmodval();
    }
    var_verbosity(const fint val) : numvar(val) {
      initmodval();
    }
    var_verbosity(const var_verbosity& x, const filepos& pos) : numvar(x, pos) {
      initmodval();
    }
    var_verbosity(const var_verbosity& x, const numb& v, const filepos& pos)
        : numvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_verbosity(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_verbosity(*this, v, p);
    }

    const char* getname() const {
      return "verbose";
    } // docscat{basic}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 0;
    }
    const char* getdescdoc() const {
      return "Verbosity level (0 = none, 1 = some, 2 = a lot).";
    }
    const char* gettypedoc() const {
      return "integer0..2";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_int(mval, 0, module_incl, 2, module_incl, 0,
                              gettypedoc());
    }
  };

  class var_uselevel : public numvar {
public:
    var_uselevel() : numvar((fint) 2) {
      assert(getid() == USELEVEL_ID);
      initmodval();
    }
    var_uselevel(const fint val) : numvar(val) {
      initmodval();
    }
    var_uselevel(const var_uselevel& x, const filepos& pos) : numvar(x, pos) {
      initmodval();
    }
    var_uselevel(const var_uselevel& x, const numb& v, const filepos& pos)
        : numvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_uselevel(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_uselevel(*this, v, p);
    }

    const char* getname() const {
      return "use-level";
    } // docscat{basic}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 0;
    }
    const char* getdescdoc() const {
      return "Limits the experience level of the documentation returned by "
             "command-line help queries (0 = beginner, 1 = intermediate, 2 = "
             "advanced, 3 = guru)."
             "  Level 0 is for total newbies."
             "  1 offers limited choices for impatient people."
             "  2 is the recommended level to get any real use out of FOMUS."
             "  Level 3 settings require a bit of technical knowledge and are "
             "used to tweak FOMUS's algorithms and improve performance."
             "  Many settings for tweaking and directly inserting text into "
             "output files are also at level 3.";
    }
    const char* gettypedoc() const {
      return "integer0..3";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_int(mval, 0, module_incl, 3, module_incl, 0,
                              gettypedoc());
    }
  };

  class var_filename : public strvar {
public:
    var_filename() : strvar("") {
      assert(getid() == FILENAME_ID);
      initmodval();
    }
    var_filename(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_filename(const var_filename& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_filename(const var_filename& x, const std::string& s,
                 const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_filename(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_filename(*this, s, p);
    }

    const char* getname() const {
      return "filename";
    } // docscat{basic}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 0;
    }
    const char* getdescdoc() const {
      return "Output path/filename."
             "  The extension determines the type of file FOMUS outputs."
             "  If this setting isn't specified, the default is the input base "
             "filename minus its extension plus any extensions listed in the "
             "`extensions' setting.";
    }
    // typedoc is already provided
    // isvalid is already given
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      assert(mval.type == module_string);
      return true;
    }
  };

  class var_inputmod : public mapvartostrings {
public:
    var_inputmod() : mapvartostrings() {
      assert(getid() ==
             INPUTMOD_ID); /*el.insert(listelmap_val("fms", string("fmsin")));*/
      initmodval();
    }
    var_inputmod(const listelmap& map) : mapvartostrings(map) {
      initmodval();
    }
    var_inputmod(const var_inputmod& x, const filepos& pos)
        : mapvartostrings(x, pos) {
      initmodval();
    }
    var_inputmod(const var_inputmod& x, const listelmap& s, const filepos& pos)
        : mapvartostrings(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_inputmod(*this, nestedlisttomap(s, p), p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_inputmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-input";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "A mapping from filename extensions to modules used for loading "
             "input files into FOMUS (modules must be of type `input')."
             "  Use this setting to customize which frontend loader is invoked "
             "based on the input filename extension.";
    }
    const char* gettypedoc() const;
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptostrings(mval, -1, -1, -1, -1, 0, gettypedoc());
    }
  };

  class var_outputmod : public mapvartostrings {
public:
    var_outputmod() : mapvartostrings() {
      assert(getid() == OUTPUTMOD_ID);
      el.insert(listelmap_val("fms", std::string("fmsout")));
      initmodval();
    }
    var_outputmod(const listelmap& map) : mapvartostrings(map) {
      initmodval();
    }
    var_outputmod(const var_outputmod& x, const filepos& pos)
        : mapvartostrings(x, pos) {
      initmodval();
    }
    var_outputmod(const var_outputmod& x, const listelmap& s,
                  const filepos& pos)
        : mapvartostrings(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_outputmod(*this, nestedlisttomap(s, p), p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_outputmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-output";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "A mapping from filename extensions to modules used for "
             "outputting data (modules must be of type `output')."
             "  Use this setting to customize which backend is invoked based "
             "on the filename extensions given in `filename' and `extensions'.";
    }
    const char* gettypedoc() const;
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptostrings(mval, -1, -1, -1, -1, 0, gettypedoc());
    }
  };

  class var_defgracedur : public numvar {
public:
    var_defgracedur() : numvar(rat(1, 4)) {
      assert(getid() == DEFAULTGRACEDUR_ID);
      initmodval();
    }
    var_defgracedur(const fint val) : numvar(val) {
      initmodval();
    }
    var_defgracedur(const rat& val) : numvar(val) {
      initmodval();
    }
    var_defgracedur(const var_defgracedur& x, const filepos& pos)
        : numvar(x, pos) {
      initmodval();
    }
    var_defgracedur(const var_defgracedur& x, const numb& v, const filepos& pos)
        : numvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_defgracedur(*this, v, p);
    }
    varbase* getnew(const rat& val, const filepos& p) const {
      return new var_defgracedur(*this, val, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_defgracedur(*this, v, p);
    }

    const char* getname() const {
      return "default-gracedur";
    } // docscat{grnotes}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Default duration of a grace note."
             "  This value is used when a note event has a zero duration "
             "(which might happen after it is quantized) and the note is "
             "converted to a grace note."
             "  Other situations might also call for the creation of grace "
             "notes."
             "  The written result depends on the setting `beat' and is "
             "notated the same way as a regular note of the same duration"
             " (e.g., if a duration of 1/4 produces a sixteenth note then the "
             "same holds true for a grace note).";
    }
    const char* gettypedoc() const {
      return "rational>0..128";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_rat(mval, makerat(0, 1), module_excl, makerat(128, 1),
                              module_incl, 0, gettypedoc());
    }
  };

  inline fint default_nthreads() {
    // #ifndef NDEBUGOUT
    return 0;
    // #else
    //     fint n = boost::thread::hardware_concurrency();
    //     return (n <= 1) ? 0 : n;
    // #endif
  }

  class var_nthreads : public numvar {
public:
    var_nthreads() : numvar(default_nthreads()) {
      assert(getid() == NTHREADS_ID);
      initmodval();
    }
    var_nthreads(const fint val) : numvar(val) {
      initmodval();
    }
    var_nthreads(const rat& val) : numvar(val) {
      initmodval();
    }
    var_nthreads(const var_nthreads& x, const filepos& pos) : numvar(x, pos) {
      initmodval();
    }
    var_nthreads(const var_nthreads& x, const numb& v, const filepos& pos)
        : numvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_nthreads(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_nthreads(*this, v, p);
    }

    const char* getname() const {
      return "n-threads";
    } // docscat{basic}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Number of threads FOMUS can create at once to take advantage of "
             "multiple cores or CPUs."
             "  If set to 0, all of FOMUS's routines run in the calling "
             "process's own thread."
             "  A value of 1 causes FOMUS to do all of its processing in one "
             "separate thread."
             "  Values greater than 1 cause FOMUS to schedule and run its "
             "algorithms in parallel, decreasing computation time.";
    }
    const char* gettypedoc() const {
      return "integer>=0";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_int(mval, 0, module_incl, 10000, module_nobound, 0,
                              gettypedoc());
    }
  };

  inline int valid_chooseclef(int n, const char* val) {
    return isvalidclef(val);
  }
  class var_chooseclef : public listvarofstrings {
public:
    var_chooseclef() : listvarofstrings() {
      assert(getid() == CHOOSECLEF_ID);
      initmodval();
    }
    var_chooseclef(const listelvect& list) : listvarofstrings(list) {
      initmodval();
    }
    var_chooseclef(const var_chooseclef& x, const filepos& pos)
        : listvarofstrings(x, pos) {
      initmodval();
    }
    var_chooseclef(const var_chooseclef& x, const listelvect& s,
                   const filepos& pos)
        : listvarofstrings(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_chooseclef(*this, s, p);
    }

    const char* getname() const {
      return "clef";
    } // docscat{notes}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "Specifies a clef or list of clefs for FOMUS to choose from."
             "  Change the value of this setting for a note or measure "
             "definition to influence clef choices."
             "  The clef must be one that is defined for the instrument that "
             "the note or measure belongs to."
             "  Setting it to a single clef effectively forces a clef "
             "selection while a list of values offers FOMUS a choice.";
    }
    const char* gettypedoc() const; // {return typstr.c_str();}
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofstrings(mval, -1, -1, 1, -1, valid_chooseclef,
                                        gettypedoc());
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(
      ordmapvartonumsparse,
      (6, (((listelmap&), el), ((listelvect&), ord), ((varbase&), var),
           ((filepos&), pos), ((confscratch&), x), ((const bool), makenew))),
      -, // puts order of syms in x.lst
      recerrpos(pos.file, pos.line, pos.col, pos.modif) >>
          (((mapmatchnums(el, ord, x.name, x.num, x.pt1, x.pt2, x.pt3, x.isplus,
                          x.pos, ferr) |
             boostspirit::eps_p[badparse(var)]) &&
            boostspirit::functor_parser<valid_f<listelmap>>(
                valid_f<listelmap>(var, x, el, makenew))) |
           boostspirit::eps_p[badvar(x.pos)]))
  class ordmapvartonums : public mapvartonums {
protected:
    listelvect ord;

public:
    ordmapvartonums() : mapvartonums() {}
    ordmapvartonums(const listelmap& map) : mapvartonums(map) {}
    ordmapvartonums(const ordmapvartonums& x, const filepos& pos)
        : mapvartonums(x, pos) {}
    ordmapvartonums(const ordmapvartonums& x, const listelmap& s,
                    const filepos& pos)
        : mapvartonums(x, s, pos) {
      assert(false);
      // for (listelmap::const_iterator i(s.begin()); i != s.end(); ++i)
      // ord.push_back(i->first);
    }
    ordmapvartonums(const ordmapvartonums& x, const listelmap& s,
                    const listelvect& v, const filepos& pos)
        : mapvartonums(x, s, pos) {
      for (listelvect::const_iterator i(v.begin()); i < v.end(); i += 2)
        ord.push_back(*i);
    }
    // bool outsort() const {return true;}
    const listelvect& getvectval() const {
      return ord;
    }
    void numtostring_ins(printmap& to) const;
    varbase* getnewprepend(const listelvect& val, const filepos& p);
    void initmodval();
    std::string getvalstr(const fomusdata* fd, const char* st) const;
  };

  void redokeysigs(const symtabs& ta);

  class var_notesymbols : public ordmapvartonums {
public:
    var_notesymbols();
    var_notesymbols(const listelmap& map) : ordmapvartonums(map) {
      initmodval();
    }
    var_notesymbols(const var_notesymbols& x, const filepos& pos)
        : ordmapvartonums(x, pos) {
      initmodval();
    }
    var_notesymbols(const var_notesymbols& x, const listelmap& s,
                    const filepos& pos)
        : ordmapvartonums(x, s, pos) {
      initmodval();
    }
    var_notesymbols(const var_notesymbols& x, const listelmap& s,
                    const listelvect& v, const filepos& pos)
        : ordmapvartonums(x, s, v, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_notesymbols(*this, nestedlisttomap(s, p), s, p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_notesymbols(*this, s, p);
    }

    const char* getname() const {
      return "note-symbols";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A mapping from pitch names to diatonic pitches."
             "  Use this to customize note names for a different language "
             "and/or scale in a `.fms' file."
             "  By default, the strings symbols are the letter names from A to "
             "G and the numbers are the chromatic pitch classes from 0 (C) to "
             "11 (B)."
             "  The sum of `note-symbols', `note-accs', `note-microtones' and "
             "`note-octaves' gives the complete pitch number."
             "  Also, the order determines which symbols are used for printing "
             "(symbols earlier in the list take precedence over ones occurring "
             "later)."
          // "  The numbers are also rational to allow microtones, though a more
          // appropriate place for this might be `note-microtones'."
          ;
    }
    const char* gettypedoc() const {
      return "(string_symb rational0..<12, string_symb rational0..<12, ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptorats(mval, -1, -1, makerat(0, 1), module_incl,
                                    makerat(12, 1), module_excl, 0,
                                    gettypedoc());
    }
    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = ordmapvartonumsparse(
          el, ord, *this, pos, x,
          false)[activate_set<var_notesymbols>(*this, x.ta)];
    }
    void activate(const symtabs& ta) const;
    bool getreset() const {
      return true;
    }
  };

  class var_noteaccs : public ordmapvartonums {
public:
    var_noteaccs();
    var_noteaccs(const listelmap& map) : ordmapvartonums(map) {
      initmodval();
    }
    var_noteaccs(const var_noteaccs& x, const filepos& pos)
        : ordmapvartonums(x, pos) {
      initmodval();
    }
    var_noteaccs(const var_noteaccs& x, const listelmap& s, const filepos& pos)
        : ordmapvartonums(x, s, pos) {
      initmodval();
    }
    var_noteaccs(const var_noteaccs& x, const listelmap& s, const listelvect& v,
                 const filepos& pos)
        : ordmapvartonums(x, s, v, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_noteaccs(*this, nestedlisttomap(s, p), s, p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_noteaccs(*this, s, p);
    }

    const char* getname() const {
      return "note-accs";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A mapping from symbols to accidental adjustments."
             "  Use this to customize note names for a different language "
             "and/or scale in a `.fms' file."
             "  The numbers are chromatic alterations to the base note (see "
             "`note-symbols')."
             "  The sum of `note-symbols', `note-accs', `note-microtones' and "
             "`note-octaves' gives the complete pitch number."
             "  The order of strings in the list determines which symbols are "
             "used for printing (symbols earlier in the list take precedence "
             "over ones occurring later)."
          // "  The numbers are also rational to allow microtones, though a more
          // appropriate place for this might be `note-microtones'."
          ;
    }
    const char* gettypedoc() const {
      return "(string_acc rational-128..128, string_acc rational-128..128, "
             "...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptorats(mval, -1, -1, makerat(-128, 1), module_incl,
                                    makerat(128, 1), module_incl, 0,
                                    gettypedoc());
    }
    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] =
          ordmapvartonumsparse(el, ord, *this, pos, x,
                               false)[activate_set<var_noteaccs>(*this, x.ta)];
    }
    void activate(const symtabs& ta) const;
    bool getreset() const {
      return true;
    }
  };

  class var_notemicrotones : public ordmapvartonums {
public:
    var_notemicrotones();
    var_notemicrotones(const listelmap& map) : ordmapvartonums(map) {
      initmodval();
    }
    var_notemicrotones(const var_notemicrotones& x, const filepos& pos)
        : ordmapvartonums(x, pos) {
      initmodval();
    }
    var_notemicrotones(const var_notemicrotones& x, const listelmap& s,
                       const filepos& pos)
        : ordmapvartonums(x, s, pos) {
      initmodval();
    }
    var_notemicrotones(const var_notemicrotones& x, const listelmap& s,
                       const listelvect& v, const filepos& pos)
        : ordmapvartonums(x, s, v, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_notemicrotones(*this, nestedlisttomap(s, p), s, p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_notemicrotones(*this, s, p);
    }

    const char* getname() const {
      return "note-microtones";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A mapping from symbols to microtonal adjustments."
             "  Use this to customize note names for a different language "
             "and/or scale in a `.fms' file."
             "  The numbers are chromatic alterations to the base note (see "
             "`note-symbols')."
             "  The sum of `note-symbols', `note-accs', `note-microtones' and "
             "`note-octaves' gives the complete pitch number."
             // "  It is also important to note that the backends expect
             // quartertones to be specific combinations of `note-accs' and
             // `note-microtones'," " so three quarter-tones flat would be best
             // represented with a `note-accs' flat symbol set to -1 and a
             // `note-microtones' qflat symbol set to -1/2." "  You then must
             // combine the two to get the proper accidental."
             "  The order of strings in the list determines which symbols are "
             "used to print output (symbols earlier in the list take "
             "precedence over ones occurring later).";
    }
    const char* gettypedoc() const {
      return "(string_mic rational-128..128, string_mic rational-128..128, "
             "...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptorats(mval, -1, -1, makerat(-128, 1), module_incl,
                                    makerat(128, 1), module_incl, 0,
                                    gettypedoc());
    }
    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = ordmapvartonumsparse(
          el, ord, *this, pos, x,
          false)[activate_set<var_notemicrotones>(*this, x.ta)];
    }
    void activate(const symtabs& ta) const;
    bool getreset() const {
      return true;
    }
  };

  class var_noteoctaves : public ordmapvartonums {
public:
    var_noteoctaves();
    var_noteoctaves(const listelmap& map) : ordmapvartonums(map) {
      initmodval();
    }
    var_noteoctaves(const var_noteoctaves& x, const filepos& pos)
        : ordmapvartonums(x, pos) {
      initmodval();
    }
    var_noteoctaves(const var_noteoctaves& x, const listelmap& s,
                    const filepos& pos)
        : ordmapvartonums(x, s, pos) {
      initmodval();
    }
    var_noteoctaves(const var_noteoctaves& x, const listelmap& s,
                    const listelvect& v, const filepos& pos)
        : ordmapvartonums(x, s, v, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_noteoctaves(*this, nestedlisttomap(s, p), s, p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_noteoctaves(*this, s, p);
    }

    const char* getname() const {
      return "note-octaves";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A mapping from symbols to octave pitches."
             "  Use this to customize note names for a different language "
             "and/or scale in a `.fms' file."
             "  The sum of `note-symbols', `note-accs', `note-microtones' and "
             "`note-octaves' gives the complete pitch number."
             "  An appropriate value for octave 4 is 60, for example, because "
             "60 is the lowest note in the octave."
             "  The order of strings in the list determines which symbols are "
             "used to print output (symbols earlier in the list take "
             "precedence over ones occurring later).";
    }
    const char* gettypedoc() const {
      return "(string_oct rational0..128, string_oct rational0..128, ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptorats(mval, -1, -1, makerat(0, 1), module_incl,
                                    makerat(128, 1), module_incl, 0,
                                    gettypedoc());
    }
    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = ordmapvartonumsparse(
          el, ord, *this, pos, x,
          false)[activate_set<var_noteoctaves>(*this, x.ta)];
    }
    void activate(const symtabs& ta) const;
    bool getreset() const {
      return true;
    }
  };

  class var_noteprint : public boolvar {
public:
    var_noteprint() : boolvar((fint) true) {
      assert(getid() == NOTEPRINT_ID);
      initmodval();
    }
    var_noteprint(const fint val) : boolvar(val) {
      initmodval();
    }
    var_noteprint(const var_noteprint& x, const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_noteprint(const var_noteprint& x, const numb& v, const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_noteprint(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_noteprint(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_noteprint(*this, v, p);
    }

    const char* getname() const {
      return "print-notesymbol";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Print notes as symbols (rather than as numbers) when "
             "appropriate.";
    }
    // const char* gettypedoc() const {return "||";}
    // bool isvalid() {return module_valid_intaux(val.getmodval(), 0,
    // module_incl, 2, module_incl, gettypedoc());}
  };

  class percinstrs_var : public varbase {
protected:
    globpercsvarvect map;

public:
    percinstrs_var() : varbase() {
      assert(getid() == PERCINSTRS_ID);
      initmodval();
    }
    percinstrs_var(globpercsvarvect& map) : varbase(), map(map) {
      initmodval();
    }
    percinstrs_var(percinstrs_var& x, const filepos& pos)
        : varbase(x, pos), map(x.map) {
      initmodval();
    }
    percinstrs_var(const percinstrs_var& x, globpercsvarvect& map,
                   const filepos& pos)
        : varbase(x, pos), map(map) {
      initmodval();
    }

    varbase* getnew(globpercsvarvect& v, const filepos& p) const {
      return new percinstrs_var(*this, v, p);
    }

    const char* getname() const {
      return "percinst-defs";
    } // docscat{libs}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "This setting is FOMUS's percussion instrument library.  It's a "
             "list of default percussion instruments to choose from or use as "
             "templates for new ones."
             "  The base list is defined in the `fomus.conf' file installed "
             "with FOMUS, though you can append new definitions in your own "
             "`.fomus' file."
             "  Although it is possible, this should not be used in a `.fms' "
             "file or when defining instruments in a score.";
    }
    enum module_value_type gettype() const {
      return module_special;
    }
    const char* gettypedoc() const {
      return "(<percinst-def> <percinst-def> ...)";
    }
    void initmodval();
    const char* getnewvalstr(const fomusdata* fd, const char* st) const {
      return make_charptr0(getvalstr(fd, st));
    }
    std::string getvalstr(const fomusdata* fd, const char* st) const;
    varbase* getnewprepend(const globpercsvarvect& val, const filepos& p);
    void addconfrule(parserule* rules, confscratch& x);
    // void instspercsvalidwdeps(std::vector<const varbase*>& vs);
    const globpercsvarvect& getmap() const {
      return map;
    }
    globpercsvarvect& getmap() {
      return map;
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return true;
    }
  };

  class instrs_var : public varbase {
protected:
    globinstsvarvect map;

public:
    instrs_var() : varbase() {
      assert(getid() == INSTRS_ID);
      initmodval();
    }
    instrs_var(globinstsvarvect& map) : varbase(), map(map) {
      initmodval();
    }
    instrs_var(instrs_var& x, const filepos& pos)
        : varbase(x, pos), map(x.map) {
      initmodval();
    }
    instrs_var(const instrs_var& x, globinstsvarvect& map, const filepos& pos)
        : varbase(x, pos), map(map) {
      initmodval();
    }

    varbase* getnew(globinstsvarvect& v, const filepos& p) const {
      return new instrs_var(*this, v, p);
    }

    const char* getname() const {
      return "inst-defs";
    } // docscat{libs}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "This setting is FOMUS's instrument library.  It's a list of "
             "default instruments to choose from (or use as templates for new "
             "ones)."
             "  The base list is defined in the `fomus.conf' file installed "
             "with FOMUS, though you can append new definitions in your own "
             "`.fomus' file."
             "  Although it is possible, this should not be used in a `.fms' "
             "file or when defining instruments in a score.";
    }
    enum module_value_type gettype() const {
      return module_special;
    }
    const char* gettypedoc() const {
      return "(<inst-def> <inst-def> ...)";
    }
    void initmodval();
    const char* getnewvalstr(const fomusdata* fd, const char* st) const {
      return make_charptr0(getvalstr(fd, st));
    }
    std::string getvalstr(const fomusdata* fd, const char* st) const;
    varbase* getnewprepend(const globinstsvarvect& val, const filepos& p);
    void addconfrule(parserule* rules, confscratch& x);
    // void instspercsvalidwdeps(std::vector<const varbase*>& vs);
    const globinstsvarvect& getmap() const {
      return map;
    }
    globinstsvarvect& getmap() {
      return map;
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return true;
    }
  };

  class listofmods : public listvarofstrings {
protected:
    std::string typestr;

public:
    listofmods(const enum module_type ty)
        : listvarofstrings() { /*maketypedoc(ty);*/
    }
    listofmods(const listelvect& list, const enum module_type ty)
        : listvarofstrings(list) { /*maketypedoc(ty);*/
    }
    listofmods(const listofmods& x, const filepos& pos,
               const enum module_type ty)
        : listvarofstrings(x, pos) { /*maketypedoc(ty);*/
    }
    listofmods(const listofmods& x, const listelvect& s, const filepos& pos,
               const enum module_type ty)
        : listvarofstrings(x, s, pos) { /*maketypedoc(ty);*/
    }

    const char* gettypedoc(const enum module_type ty) const {
      return typestr.c_str();
    }
    bool isvalid(const enum module_type ty /*, const std::string& tys*/);
    // void maketypedoc(const enum module_type ty);
  };

  class var_tquantmod : public listofmods {
public:
    var_tquantmod() : listofmods(module_modtquant) {
      assert(getid() == TQUANTMOD_ID);
      el.push_back("tquant");
      el.push_back("grtquant"); /*initmodval();*/
    }
    var_tquantmod(const listelvect& str)
        : listofmods(str, module_modtquant) { /*initmodval();*/
    }
    var_tquantmod(const var_tquantmod& x, const filepos& pos)
        : listofmods(x, pos, module_modtquant) { /*initmodval();*/
    }
    var_tquantmod(const var_tquantmod& x, const listelvect& s,
                  const filepos& pos)
        : listofmods(x, s, pos, module_modtquant) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_tquantmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_tquantmod(*this, s, p);
    }

    const char* getname() const {
      return "tquant";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for quantizing note "
             "offsets and durations (modules must be of type `timequantize')."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modtquant);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modtquant /*, "timequantize"*/);
    }
  };
  class var_pquantmod : public listofmods {
public:
    var_pquantmod() : listofmods(module_modpquant) {
      assert(getid() == PQUANTMOD_ID);
      el.push_back("pquant"); /*initmodval();*/
    }
    var_pquantmod(const listelvect& str)
        : listofmods(str, module_modpquant) { /*initmodval();*/
    }
    var_pquantmod(const var_pquantmod& x, const filepos& pos)
        : listofmods(x, pos, module_modpquant) { /*initmodval();*/
    }
    var_pquantmod(const var_pquantmod& x, const listelvect& s,
                  const filepos& pos)
        : listofmods(x, s, pos, module_modpquant) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    //     varbase* getnew(const char* s, const filepos& p) const {return new
    //     var_pquantmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_pquantmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-pquant";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for quantizing pitches "
             "(modules must be of type `pitchquantize')."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modpquant);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modpquant /*, "pitchquantize"*/);
    }
  };
  class var_measmod : public listofmods {
public:
    var_measmod() : listofmods(module_modmeas) {
      assert(getid() == MEASMOD_ID);
      el.push_back("meas"); /*initmodval();*/
    }
    var_measmod(const listelvect& str)
        : listofmods(str, module_modmeas) { /*initmodval();*/
    }
    var_measmod(const var_measmod& x, const filepos& pos)
        : listofmods(x, pos, module_modmeas) { /*initmodval();*/
    }
    var_measmod(const var_measmod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_modmeas) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_measmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_measmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-meas";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locpart;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for creating measures "
             "(modules must be of type `measure')."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modmeas);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modmeas /*, "measure"*/);
    }
  };
  class var_checkmod : public listofmods {
public:
    var_checkmod() : listofmods(module_modcheck) {
      assert(getid() == CHECKMOD_ID);
      el.push_back("ranges"); /*initmodval();*/
    }
    var_checkmod(const listelvect& str)
        : listofmods(str, module_modcheck) { /*initmodval();*/
    }
    var_checkmod(const var_checkmod& x, const filepos& pos)
        : listofmods(x, pos, module_modcheck) { /*initmodval();*/
    }
    var_checkmod(const var_checkmod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_modcheck) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_checkmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_checkmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-check";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for performing useful "
             "checks on input data (modules must be of type `check')."
             "  Module `ranges', for example, checks to see if all notes are "
             "in the correct ranges for their instruments."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modcheck);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modcheck /*, "check"*/);
    }
  };
  class var_tposemod : public listofmods {
public:
    var_tposemod() : listofmods(module_modtpose) {
      assert(getid() == TPOSEMOD_ID);
      el.push_back("tpose"); /*initmodval();*/
    }
    var_tposemod(const listelvect& str)
        : listofmods(str, module_modtpose) { /*initmodval();*/
    }
    var_tposemod(const var_tposemod& x, const filepos& pos)
        : listofmods(x, pos, module_modtpose) { /*initmodval();*/
    }
    var_tposemod(const var_tposemod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_modtpose) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_tposemod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_tposemod(*this, s, p);
    }

    const char* getname() const {
      return "mod-tpose";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for transposing parts "
             "according to their assigned instruments (modules must be of type "
             "`transpose')."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modtpose);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modtpose /*, "transpose"*/);
    }
  };
  class var_voicesmod : public listofmods {
public:
    var_voicesmod() : listofmods(module_modvoices) {
      assert(getid() == VOICESMOD_ID);
      el.push_back("voices"); /*initmodval();*/
    }
    var_voicesmod(const listelvect& str)
        : listofmods(str, module_modvoices) { /*initmodval();*/
    }
    var_voicesmod(const var_voicesmod& x, const filepos& pos)
        : listofmods(x, pos, module_modvoices) { /*initmodval();*/
    }
    var_voicesmod(const var_voicesmod& x, const listelvect& s,
                  const filepos& pos)
        : listofmods(x, s, pos, module_modvoices) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_voicesmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_voicesmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-voices";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for sorting note events "
             "into separate voices (modules must be of type `voices')."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modvoices);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modvoices /*, "voices"*/);
    }
  };
  class var_prunemod : public listofmods {
public:
    var_prunemod() : listofmods(module_modprune) {
      assert(getid() == PRUNEMOD_ID);
      el.push_back("prune"); /*initmodval();*/
    }
    var_prunemod(const listelvect& str)
        : listofmods(str, module_modprune) { /*initmodval();*/
    }
    var_prunemod(const var_prunemod& x, const filepos& pos)
        : listofmods(x, pos, module_modprune) { /*initmodval();*/
    }
    var_prunemod(const var_prunemod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_modprune) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_prunemod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_prunemod(*this, s, p);
    }

    const char* getname() const {
      return "mod-prune";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for \"pruning\" "
             "overlapping note events (modules must be of type `prune')."
             "  Notes are truncated or eliminated if they overlap with other "
             "note events of the same pitch."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modprune);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modprune /*, "prune"*/);
    }
  };
  class var_vmarksmod : public listofmods {
public:
    var_vmarksmod() : listofmods(module_modvmarks) {
      assert(getid() == VMARKSMOD_ID);
      el.push_back("vmarks"); /*initmodval();*/
    }
    var_vmarksmod(const listelvect& str)
        : listofmods(str, module_modvmarks) { /*initmodval();*/
    }
    var_vmarksmod(const var_vmarksmod& x, const filepos& pos)
        : listofmods(x, pos, module_modvmarks) { /*initmodval();*/
    }
    var_vmarksmod(const var_vmarksmod& x, const listelvect& s,
                  const filepos& pos)
        : listofmods(x, s, pos, module_modvmarks) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_vmarksmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_vmarksmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-vmarks";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locpart;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for sorting out spanner "
             "marks in separate voices (modules must be of type `voicemarks')."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modvmarks);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modvmarks /*, "voicemarks"*/);
    }
  };
  class var_accsmod : public listofmods {
public:
    var_accsmod() : listofmods(module_modaccs) {
      assert(getid() == ACCSMOD_ID);
      el.push_back("accs");
      el.push_back("postaccs"); /*initmodval();*/
    }
    var_accsmod(const listelvect& str)
        : listofmods(str, module_modaccs) { /*initmodval();*/
    }
    var_accsmod(const var_accsmod& x, const filepos& pos)
        : listofmods(x, pos, module_modaccs) { /*initmodval();*/
    }
    var_accsmod(const var_accsmod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_modaccs) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_accsmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_accsmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-accs";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for assigning accidental "
             "spellings (modules must be of type `accidentals')."
             "  Although FOMUS's default module takes key signatures into "
             "account, other modules may ignore key signatures."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modaccs);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modaccs /*, "accidentals"*/);
    }
  };
  class var_cautaccsmod : public listofmods {
public:
    var_cautaccsmod() : listofmods(module_modcautaccs) {
      assert(getid() == CAUTACCSMOD_ID);
      el.push_back("cautaccs"); /*initmodval();*/
    }
    var_cautaccsmod(const listelvect& str)
        : listofmods(str, module_modcautaccs) { /*initmodval();*/
    }
    var_cautaccsmod(const var_cautaccsmod& x, const filepos& pos)
        : listofmods(x, pos, module_modcautaccs) { /*initmodval();*/
    }
    var_cautaccsmod(const var_cautaccsmod& x, const listelvect& s,
                    const filepos& pos)
        : listofmods(x, s, pos, module_modcautaccs) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_cautaccsmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_cautaccsmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-cautaccs";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for creating cautionary "
             "accidentals (modules must be of type `cautaccidentals')."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modcautaccs);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modcautaccs /*, "cautaccidentals"*/);
    }
  };
  class var_stavesmod : public listofmods {
public:
    var_stavesmod() : listofmods(module_modstaves) {
      assert(getid() == STAVESMOD_ID);
      el.push_back("staves"); /*initmodval();*/
    }
    var_stavesmod(const listelvect& str)
        : listofmods(str, module_modstaves) { /*initmodval();*/
    }
    var_stavesmod(const var_stavesmod& x, const filepos& pos)
        : listofmods(x, pos, module_modstaves) { /*initmodval();*/
    }
    var_stavesmod(const var_stavesmod& x, const listelvect& s,
                  const filepos& pos)
        : listofmods(x, s, pos, module_modstaves) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_stavesmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_stavesmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-staves";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for choosing staves and "
             "clefs (modules must be of type `staves')."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modstaves);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modstaves /*, "staves"*/);
    }
  };
  class var_rstavesmod : public listofmods { // rest staves
public:
    var_rstavesmod() : listofmods(module_modrstaves) {
      assert(getid() == RESTSTAVESMOD_ID);
      el.push_back("rstaves"); /*initmodval();*/
    }
    var_rstavesmod(const listelvect& str)
        : listofmods(str, module_modrstaves) { /*initmodval();*/
    }
    var_rstavesmod(const var_rstavesmod& x, const filepos& pos)
        : listofmods(x, pos, module_modrstaves) { /*initmodval();*/
    }
    var_rstavesmod(const var_rstavesmod& x, const listelvect& s,
                   const filepos& pos)
        : listofmods(x, s, pos, module_modrstaves) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_rstavesmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_rstavesmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-reststaves";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for distributing rests to "
             "individual staves (modules must be of type `stavesrests')."
             "  Rests are treated separately from notes because they depend "
             "largely on the placement of neighboring note events."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modrstaves);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modrstaves /*, "stavesrests"*/);
    }
  };
  class var_smarksmod : public listofmods {
public:
    var_smarksmod() : listofmods(module_modsmarks) {
      assert(getid() == SMARKSMOD_ID);
      el.push_back("smarks"); /*initmodval();*/
    }
    var_smarksmod(const listelvect& str)
        : listofmods(str, module_modsmarks) { /*initmodval();*/
    }
    var_smarksmod(const var_smarksmod& x, const filepos& pos)
        : listofmods(x, pos, module_modsmarks) { /*initmodval();*/
    }
    var_smarksmod(const var_smarksmod& x, const listelvect& s,
                  const filepos& pos)
        : listofmods(x, s, pos, module_modsmarks) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_smarksmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_smarksmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-smarks";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locpart;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for sorting out spanner "
             "marks (like octave signs) in separate staves (modules must be of "
             "type `staffmarks')."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modsmarks);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modsmarks /*, "staffmarks"*/);
    }
  };
  class var_octsmod : public listofmods {
public:
    var_octsmod() : listofmods(module_modocts) {
      assert(getid() == OCTSMOD_ID);
      el.push_back("octs"); /*initmodval();*/
    }
    var_octsmod(const listelvect& str)
        : listofmods(str, module_modocts) { /*initmodval();*/
    }
    var_octsmod(const var_octsmod& x, const filepos& pos)
        : listofmods(x, pos, module_modocts) { /*initmodval();*/
    }
    var_octsmod(const var_octsmod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_modocts) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_octsmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_octsmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-octs";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for creating octave signs "
             "in appropriate places (modules must be of type `octavesigns')."
             "  Whether or not they are actually allowed and where they may "
             "occur (above or below the staff) may change depending on the "
             "instrument."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modocts);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modocts /*, "octavesigns"*/);
    }
  };
  class var_dynsmod : public listofmods {
public:
    var_dynsmod() : listofmods(module_moddynamics) {
      assert(getid() == DYNSMOD_ID);
      el.push_back("phrdyns");
      el.push_back("dyns");
      /*initmodval();*/
    }
    var_dynsmod(const listelvect& str)
        : listofmods(str, module_moddynamics) { /*initmodval();*/
    }
    var_dynsmod(const var_dynsmod& x, const filepos& pos)
        : listofmods(x, pos, module_moddynamics) { /*initmodval();*/
    }
    var_dynsmod(const var_dynsmod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_moddynamics) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_dynsmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_dynsmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-dyns";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for converting numeric "
             "dynamic values into symbols (modules must be of type `dynamics')."
             "  Results may vary wildly depending how exactly this is done."
          //"  You might also want to override the module's decisions by
          // explicitly providing your own dynamic markings."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_moddynamics);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_moddynamics /*, "dynamics"*/);
    }
  };
  class var_divmod : public listofmods {
public:
    var_divmod() : listofmods(module_moddivide) {
      assert(getid() == DIVMOD_ID);
      el.push_back("divide");
      el.push_back("grdiv");
      el.push_back("untie"); /*initmodval();*/
    }
    var_divmod(const listelvect& str)
        : listofmods(str, module_moddivide) { /*initmodval();*/
    }
    var_divmod(const var_divmod& x, const filepos& pos)
        : listofmods(x, pos, module_moddivide) { /*initmodval();*/
    }
    var_divmod(const var_divmod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_moddivide) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_divmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_divmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-div";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for splitting and tying "
             "notes in each measure (modules must be of type `divide')."
          //"  Metrical subdivisions are also decided on."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_moddivide);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_moddivide /*, "divide"*/);
    }
  };
  class var_mergemod : public listofmods {
public:
    var_mergemod() : listofmods(module_modmerge) {
      assert(getid() == MERGEMOD_ID);
      el.push_back("merge"); /*initmodval();*/
    }
    var_mergemod(const listelvect& str)
        : listofmods(str, module_modmerge) { /*initmodval();*/
    }
    var_mergemod(const var_mergemod& x, const filepos& pos)
        : listofmods(x, pos, module_modmerge) { /*initmodval();*/
    }
    var_mergemod(const var_mergemod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_modmerge) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_mergemod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_mergemod(*this, s, p);
    }

    const char* getname() const {
      return "mod-merge";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for \"merging\" multiple "
             "voices that appear on one staff (modules must be of type "
             "`merge')."
             "  Simultaneous rests of equivalent durations, for example, are "
             "merged into a single rest while notes that are rhythmically "
             "equivalent"
             " and appear within the same metrical division are merged into "
             "chords."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modmerge);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modmerge /*, "merge"*/);
    }
  };
  class var_beammod : public listofmods {
public:
    var_beammod() : listofmods(module_modbeams) {
      assert(getid() == BEAMMOD_ID);
      el.push_back("beams"); /*initmodval();*/
    }
    var_beammod(const listelvect& str)
        : listofmods(str, module_modbeams) { /*initmodval();*/
    }
    var_beammod(const var_beammod& x, const filepos& pos)
        : listofmods(x, pos, module_modbeams) { /*initmodval();*/
    }
    var_beammod(const var_beammod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_modbeams) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_beammod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_beammod(*this, s, p);
    }

    const char* getname() const {
      return "mod-beam";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for beaming notes together "
             "(modules must be of type `beam')."
             "  The module should beams the notes according to metrical "
             "subdivisions and might take other factors into account."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modbeams);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modbeams /*, "beam"*/);
    }
  };
  class var_pmarksmod : public listofmods {
public:
    var_pmarksmod() : listofmods(module_modmarks) {
      assert(getid() == PMARKSMOD_ID);
      el.push_back("markgrps");
      el.push_back("grslurs");
      el.push_back("repdyns");
      /*initmodval();*/
    }
    var_pmarksmod(const listelvect& str)
        : listofmods(str, module_modmarks) { /*initmodval();*/
    }
    var_pmarksmod(const var_pmarksmod& x, const filepos& pos)
        : listofmods(x, pos, module_modmarks) { /*initmodval();*/
    }
    var_pmarksmod(const var_pmarksmod& x, const listelvect& s,
                  const filepos& pos)
        : listofmods(x, s, pos, module_modmarks) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_pmarksmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_pmarksmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-marks";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules that do various useful tasks related "
             "to marks (modules must be of type `markpairs')."
             "  For example, the `markgrps' module translates single marks "
             "into pairs of marks (like `pizz.' and `arco') where necessary so "
             "you don't have to keep track of them."
          //"  It also eliminates redundant dynamic markings."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modmarks);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modmarks /*, "marks"*/);
    }
  };
  class var_marksmod : public listofmods {
public:
    var_marksmod() : listofmods(module_modmarklayouts) {
      assert(getid() == MARKSMOD_ID);
      el.push_back("marks"); /*initmodval();*/
    }
    var_marksmod(const listelvect& str)
        : listofmods(str, module_modmarklayouts) { /*initmodval();*/
    }
    var_marksmod(const var_marksmod& x, const filepos& pos)
        : listofmods(x, pos, module_modmarklayouts) { /*initmodval();*/
    }
    var_marksmod(const var_marksmod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_modmarklayouts) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_marksmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_marksmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-marklayouts";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for taking care of "
             "additional special layout issues related to marks (modules must "
             "be of type `marks')."
             "  Special layout issues include such decisions as where text "
             "marks should appear (above or below the staff) and whether marks "
             "should"
             " be merged in cases where there are multiple voices."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modmarklayouts);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modmarklayouts /*, "marklayouts"*/);
    }
  };
  class var_specialmod : public listofmods {
public:
    var_specialmod() : listofmods(module_modspecial) {
      assert(getid() == SPECIALMOD_ID);
      el.push_back("harms");
      el.push_back("percchs");
      el.push_back("trems");
      /*el.push_back("text"); el.push_back("pos");*/ /*initmodval();*/
    }
    var_specialmod(const listelvect& str)
        : listofmods(str, module_modspecial) { /*initmodval();*/
    }
    var_specialmod(const var_specialmod& x, const filepos& pos)
        : listofmods(x, pos, module_modspecial) { /*initmodval();*/
    }
    var_specialmod(const var_specialmod& x, const listelvect& s,
                   const filepos& pos)
        : listofmods(x, s, pos, module_modspecial) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_specialmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_specialmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-special";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for taking care of any "
             "additional special layout issues (modules must be of type "
             "`special')."
             "  This is where tremolos and harmonics are handled, for example."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modspecial);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modspecial /*, "special"*/);
    }
  };
  class var_partsmod : public listofmods {
public:
    var_partsmod() : listofmods(module_modparts) {
      assert(getid() == PARTSMOD_ID);
      el.push_back("parts"); /*initmodval();*/
    }
    var_partsmod(const listelvect& str)
        : listofmods(str, module_modparts) { /*initmodval();*/
    }
    var_partsmod(const var_partsmod& x, const filepos& pos)
        : listofmods(x, pos, module_modparts) { /*initmodval();*/
    }
    var_partsmod(const var_partsmod& x, const listelvect& s, const filepos& pos)
        : listofmods(x, s, pos, module_modparts) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_partsmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_partsmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-parts";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for determining how parts "
             "appear in the score (modules must be of type `parts')."
             "  This includes the ordering and grouping of instruments into "
             "families and sections."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modparts);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modparts /*, "parts"*/);
    }
  };

  inline int validaccrulefun(const char* str) {
    return accrules.find(str) != accrules.end();
  }

  class var_accrule : public strvar {
public:
    var_accrule() : strvar("measure") {
      assert(getid() == ACCRULE_ID);
      initmodval();
    }
    var_accrule(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_accrule(const var_accrule& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_accrule(const var_accrule& x, const std::string& s, const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_accrule(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_accrule(*this, s, p);
    }

    const char* getname() const {
      return "acc-rule";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "Whether or not accidentals carry through to the end of the "
             "measure."
             "  The value `measure' indicates that accidentals affect all "
             "notes up the end of the measure while a value of `note' indicates"
             " that accidentals affect only the note they precede.  "
             "`note-naturals' is similar to `note' with the exception that "
             "accidentals (including naturals) always appear.";
    }
    const char* gettypedoc() const {
      return "measure|note|note-naturals";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_string(mval, -1, -1, validaccrulefun, gettypedoc());
    }
  };

  class var_commkeysig : public strvar {
public:
    var_commkeysig() : strvar("cmaj") {
      assert(getid() == COMMKEYSIG_ID);
      initmodval();
    }
    var_commkeysig(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_commkeysig(const var_commkeysig& x, const filepos& pos)
        : strvar(x, pos) {
      initmodval();
    }
    var_commkeysig(const var_commkeysig& x, const std::string& s,
                   const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_commkeysig(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_commkeysig(*this, s, p);
    }

    const char* getname() const {
      return "keysig";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "A key signature, specified as a string such as \"amin\" or "
             "\"bfmaj\"."
             "  Use this to change the key when defining a measure or measure "
             "definition in a score."
             "  The value must be one that is defined in `keysig-defs'.";
    }
    const char* gettypedoc() const {
      return "string_keysig";
    }

    // const char* gettypedoc() const {return "measure|note|note-naturals";}
    //#warning "check this against the map of keysig stings!--also update it
    // when keysigs are updated" can't really do that
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return true;
    }
  };

  struct userkeysigent {
#ifndef NDEBUG
    int valid;
#endif
    rat n, a, m, o;
    userkeysigent(const rat& n, const rat& a, const rat& m, const rat& o)
        :
#ifndef NDEBUG
          valid(12345),
#endif
          n(n), a(a), m(m), o(o) {
    }
#ifndef NDEBUG
    bool isvalid() const {
      return valid == 12345;
    }
#endif
  };

  void redosig(const boost::ptr_vector<userkeysigent>& user, listelvect& el,
               const printmap& notepr, const printmap& accpr,
               const printmap& micpr, const printmap& octpr);

  // single overriding definition
  bool doparsekeysig(const fomusdata* fd, const module_value& lst,
                     std::vector<std::pair<rat, rat>>& sig,
                     boost::ptr_vector<userkeysigent>& user);

  class var_keysig : public listvarofstrings {
    std::vector<std::pair<rat, rat>> sig;
    boost::ptr_vector<userkeysigent>
        user; // user's entries recorded for conversion back
public:
    var_keysig() : listvarofstrings(), sig(75, std::pair<rat, rat>(0, 0)) {
      assert(getid() == KEYSIGDEF_ID);
      initmodval();
    }
    var_keysig(const listelvect& str)
        : listvarofstrings(str), sig(75, std::pair<rat, rat>(0, 0)) {
      initmodval();
    }
    var_keysig(const var_keysig& x, const filepos& pos)
        : listvarofstrings(x, pos), sig(75, std::pair<rat, rat>(0, 0)) {
      initmodval();
    }
    var_keysig(const var_keysig& x, const listelvect& s, const filepos& pos)
        : listvarofstrings(x, s, pos), sig(75, std::pair<rat, rat>(0, 0)) {
      initmodval();
    }
    var_keysig(const var_keysig& x)
        : listvarofstrings(x, x.el, x.pos),
          sig(x.sig /*75, std::pair<rat, rat>(0, 0)*/), user(x.user) {
      assert(varisvalid());
      initmodval();
    }

    const std::vector<std::pair<rat, rat>>& getkeysigvect() const {
      return sig;
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_keysig(*this, s, p);
    }

    const char* getname() const {
      return "keysig-def";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A custom key signature definition that overrides the value of "
             "`keysig' when used."
             "  Use this instead of `keysig' when you want to specify a key "
             "signature that isn't defined in `keysig-defs'."
             "  See `keysig-defs' to see how to define a key signature.";
    }
    const char* gettypedoc() const {
      return "(string_note/acc string_note/acc ...)";
    }
    bool isvalid(const fomusdata* fd) { // isvalid dumps contents into sig
      assert(!mval.notyet());
      user.clear(); // assert(user.empty());
      sig.assign(75, std::pair<rat, rat>(0, 0));
      bool r = doparsekeysig(fd, mval, sig, user);
      redo(fd);
      return validerrwrap(r, gettypedoc());
    }
    void
    redo(const symtabs& ta) { // after user-defined note parsing has changed
      el.clear();
      redosig(user, el, ta.notepr, ta.accpr, ta.micpr, ta.octpr);
      mval.reset();
      initmodval();
    }
    void redo(const fomusdata* fd);
    bool userempty() const {
      return user.empty();
    }
    const std::vector<std::pair<rat, rat>>& getsig() const {
      return sig;
    }
    const boost::ptr_vector<userkeysigent>& getuserdef() const {
      return user;
    }
    // bool getreset() const {return true;}
  };

  struct keysigerr : public errbase {};
  class var_keysigs : public mapvaroflistofstrings {
    boost::ptr_map<const std::string, std::vector<std::pair<rat, rat>>>
        allsigs; // maps to the actual numbers, vectors are size 75
    boost::ptr_map<const std::string, boost::ptr_vector<userkeysigent>>
        user; // user's entries recorded for conversion back
public:
#ifndef NDEBUG
    void fu() const {
      assert(varisvalid());
    }
#endif
    var_keysigs();
    var_keysigs(const listelmap& sig) : mapvaroflistofstrings(sig) {
      DBG("NEW KEYSIGS " << this << std::endl);
      initmodval();
    }
    var_keysigs(const var_keysigs& x, const filepos& pos)
        : mapvaroflistofstrings(x, pos) {
      DBG("NEW KEYSIGS " << this << std::endl);
      initmodval();
    }
    var_keysigs(const var_keysigs& x, const listelmap& sig, const filepos& pos)
        : mapvaroflistofstrings(x, sig, pos) {
      DBG("NEW KEYSIGS " << this << std::endl);
      initmodval();
    }
    var_keysigs(const var_keysigs& x)
        : mapvaroflistofstrings(x, x.el, x.pos),
          allsigs(x.allsigs.begin(), x.allsigs.end()),
          user(x.user.begin(), x.user.end()) {
      assert(x.varisvalid());
      DBG("NEW KEYSIGS " << this
                         << std::endl); // constructor when redoing strings
      initmodval();
    }

    varbase* getnew(const listelvect& v, const filepos& p) const {
      return new var_keysigs(*this, nestedlisttomap(v, p), p);
    }
    varbase* getnew(const listelmap& v, const filepos& p) const {
      return new var_keysigs(*this, v, p);
    }

    const std::vector<std::pair<rat, rat>>&
    getkeysigvect(const std::string& str) const {
      boost::ptr_map<const std::string,
                     std::vector<std::pair<rat, rat>>>::const_iterator
          i(allsigs.find(str));
      if (i == allsigs.end()) {
        CERR << "invalid key signature `" << str << '\'';
        throw keysigerr();
      }
      return *i->second;
    }
    const boost::ptr_vector<userkeysigent>&
    getuserdef(const std::string& str) const {
      boost::ptr_map<const std::string,
                     boost::ptr_vector<userkeysigent>>::const_iterator
          i(user.find(str));
      assert(i != user.end());
      return *i->second;
    }

    const char* getname() const {
      return "keysig-defs";
    } // docscat{libs}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Defines a mapping from strings to key signature definitions."
             "  A key signature definition is a list of strings each "
             "containing a base note name and accidental (e.g., `e-', `f+', "
             "etc.)."
             "  Microtonal keysignatures are possible, though not all backends "
             "support this."
             "  If octaves are specified (e.g., `c4'), these are interpreted "
             "as individual exceptions."
             "  If a key signature is also to be interpreted as a major or "
             "minor mode, the identifying string must have"
             " a suffix that appears in `keysig-major-symbol' or "
             "`keysig-minor-symbol'.";
    }
    const char* gettypedoc() const {
      return "(string_keysig (string_note/acc string_note/acc ...), "
             "string_keysig (string_note/acc string_note/acc ...), ...)";
    }
    bool parsekeysigmap(const fomusdata* fd, const char* na,
                        const module_value& lst) { // valid for var_keysigs
      assert(varisvalid());
      std::vector<std::pair<rat, rat>>* sig;
      allsigs.insert(na, sig = new std::vector<std::pair<rat, rat>>(
                             75, std::pair<rat, rat>(0, 0)));
      assert(sig->size() == 75);
      boost::ptr_vector<userkeysigent>* us;
      user.insert(na, us = new boost::ptr_vector<userkeysigent>);
#ifndef NDEBUG
      fu();
#endif
      return doparsekeysig(fd, lst, *sig, *us);
    }
    bool keysig_isinvalid(const fomusdata* fd, int& n, const char*& s,
                          const struct module_value& val);
    bool module_valid_keysigs(const fomusdata* fd);
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      user.clear(); // assert(user.empty()); needs to be cleared if keysig is in
                    // .config
      allsigs.clear();
      bool r = module_valid_keysigs(fd);
#ifndef NDEBUG
      fu();
#endif
      redo(fd);
      return r;
    }
    void redo(const symtabs& ta);
    void redo(const fomusdata* fd);
    // bool getreset() const {return true;}
  };

  class var_majmode : public listvarofstrings {
public:
    var_majmode() : listvarofstrings() {
      assert(getid() == MAJMODE_ID);
      el.push_back("maj");
      initmodval();
    }
    var_majmode(const listelvect& map) : listvarofstrings(map) {
      initmodval();
    }
    var_majmode(const var_majmode& x, const filepos& pos)
        : listvarofstrings(x, pos) {
      initmodval();
    }
    var_majmode(const var_majmode& x, const listelvect& s, const filepos& pos)
        : listvarofstrings(x, s, pos) {
      initmodval();
    }

    // varbase* getnew(const listelvect &s, const filepos& p) const {return new
    // var_majmode(*this, nestedlisttomap(s, p), p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_majmode(*this, s, p);
    }

    const char* getname() const {
      return "keysig-major-symbol";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A string or list of suffixes that indicate the name of a major "
             "scale."
             "  For example, if you include the string \"maj\" on this list "
             "then any scale in `keysig-defs' with an identifier ending in "
             "\"maj\" is interpretted as a major scale.";
    }
    const char* gettypedoc() const {
      return "string_majsfx | (string_majsfx string_majsfx ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofstrings(mval, -1, -1, 1, -1, 0, gettypedoc());
    }
    // void addconfrule(parserule* rules, confscratch& x) {rules[getid()] =
    // listvarofstringsparse(el, ord, *this, pos,
    // x)[activate_set<var_majmode>(*this, x.ta)];}
  };

  class var_minmode : public listvarofstrings {
public:
    var_minmode() : listvarofstrings() {
      assert(getid() == MINMODE_ID);
      el.push_back("min");
      initmodval();
    }
    var_minmode(const listelvect& map) : listvarofstrings(map) {
      initmodval();
    }
    var_minmode(const var_minmode& x, const filepos& pos)
        : listvarofstrings(x, pos) {
      initmodval();
    }
    var_minmode(const var_minmode& x, const listelvect& s, const filepos& pos)
        : listvarofstrings(x, s, pos) {
      initmodval();
    }

    // varbase* getnew(const listelvect &s, const filepos& p) const {return new
    // var_minmode(*this, nestedlisttomap(s, p), p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_minmode(*this, s, p);
    }

    const char* getname() const {
      return "keysig-minor-symbol";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A string or list of suffixes that indicate the name of a minor "
             "scale."
             "  For example, if you include the string \"min\" on this list "
             "then any scale in `keysig-defs' with an identifier ending in "
             "\"min\" is interpretted as a minor scale.";
    }
    const char* gettypedoc() const {
      return "string_minsfx | (string_minsfx string_minsfx ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofstrings(mval, -1, -1, 1, -1, 0, gettypedoc());
    }
    // void addconfrule(parserule* rules, confscratch& x) {rules[getid()] =
    // listvarofstringsparse(el, ord, *this, pos,
    // x)[activate_set<var_minmode>(*this, x.ta)];}
  };

  class var_nbeats : public numvar {
public:
    var_nbeats() : numvar((fint) 0) {
      assert(getid() == NBEATS_ID);
      initmodval();
    }
    var_nbeats(const fint val) : numvar(val) {
      initmodval();
    }
    var_nbeats(const rat& val) : numvar(val) {
      initmodval();
    }
    var_nbeats(const var_nbeats& x, const filepos& pos) : numvar(x, pos) {
      initmodval();
    }
    var_nbeats(const var_nbeats& x, const numb& v, const filepos& pos)
        : numvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_nbeats(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_nbeats(*this, v, p);
    }
    varbase* getnew(const rat& val, const filepos& p) const {
      return new var_nbeats(*this, val, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_nbeats(*this, v, p);
    }

    const char* getname() const {
      return "measdur";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "The number of beats in a measure (i.e., the duration)."
             "  Usually the duration of a measure is specified by simply "
             "setting its duration (in the same way the duration of a note "
             "event is set)."
             "  Use this setting to override this and permanently set the "
             "duration of a measure definition."
          // "  For example, you might want to define a special measure
          // definition that always has a duration of 3+1/2."
          ;
    }
    const char* gettypedoc() const {
      return "rational>=0";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_rat(val, makerat(0, 1), module_incl, makerat(0, 1),
                              module_nobound, 0, gettypedoc());
    }
  };

  class var_comp : public boolvar {
public:
    var_comp() : boolvar((fint) false) {
      assert(getid() == COMP_ID);
      initmodval();
    }
    var_comp(const fint val) : boolvar(val) {
      initmodval();
    }
    var_comp(const var_comp& x, const filepos& pos) : boolvar(x, pos) {
      initmodval();
    }
    var_comp(const var_comp& x, const numb& v, const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_comp(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_comp(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_comp(*this, v, p);
    }

    const char* getname() const {
      return "comp";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "Whether or not a measure has a compound meter."
             "  A duration of 1 in a compound meter is equivalent to a dotted "
             "note value and is subdivided into 3 smaller rhythmic divisions."
             "  If the value of `timesig-den' is responsible for determining "
             "the actual time signature the number is interpretted as a dotted "
             "value."
             "  For example, if the meter is compound, `measdur' is 4 and "
             "`timesig-den' is 4, the denominator value of 4 is interpretted "
             "as a dotted quarter"
             " note (a group of three eighths), so altogether these specify a "
             "time signature of 12/8 (or 4 dotted quarter notes per measure).";
    }
    // const char* gettypedoc() const {return "||";}
    // bool isvalid() {return module_valid_intaux(val.getmodval(), 0,
    // module_incl, 2, module_incl, gettypedoc());}
  };

  class var_staff : public listvarofnums {
public:
    var_staff() : listvarofnums() {
      assert(getid() == USERSTAVES_ID);
      initmodval();
    }
    var_staff(const listelvect& list) : listvarofnums(list) {
      initmodval();
    }
    var_staff(const var_staff& x, const filepos& pos) : listvarofnums(x, pos) {
      initmodval();
    }
    var_staff(const var_staff& x, const listelvect& s, const filepos& pos)
        : listvarofnums(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_staff(*this, /*nestedlisttolist(s)*/ s, p);
    }

    const char* getname() const {
      return "staff";
    } // docscat{notes}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "Specifies a staff or list of staves for FOMUS to choose from."
             "  Change the value of this setting for a note or measure "
             "definition to influence which staff the notes appear on."
             "  Setting this to a single value effectively forces a staff "
             "selection while a list of values offers FOMUS a choice.";
    }
    const char* gettypedoc() const {
      return "integer>=1 | (integer>=1 integer>=1 ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofints(mval, -1, -1, 1, module_incl, 0,
                                     module_nobound, 0, gettypedoc());
    }
  };

  inline int timesigvalfun(int n, fomus_int val) {
    return n == 1 ? isexpof2(val) : true;
  }
  inline int timesigdenvalfun(fomus_int val) {
    return isexpof2(val);
  }

  class var_timesigden : public numvar {
public:
    var_timesigden() : numvar((fint) 4) {
      assert(getid() == TIMESIGDEN_ID);
      initmodval();
    }
    var_timesigden(const fint val) : numvar(val) {
      initmodval();
    }
    var_timesigden(const var_timesigden& x, const filepos& pos)
        : numvar(x, pos) {
      initmodval();
    }
    var_timesigden(const var_timesigden& x, const numb& v, const filepos& pos)
        : numvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_timesigden(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_timesigden(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_timesigden(*this, v, p);
    }

    const char* getname() const {
      return "timesig-den";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "Specifies the number that should normally appear in the "
             "denominator of the time signature."
             "  Set this globally or in a measure definition to influence how "
             "FOMUS calculates time signatures."
             "  FOMUS multiplies this value by 2 or powers of 2 when necessary "
             "(e.g., when `timesig-den' is 4 and a measure contains 4+1/2 "
             "beats, FOMUS multiples 4 by 2 to get "
             "a time signature of 9/8).  FOMUS also interprets this as a "
             "dotted value when there is a compound meter (e.g., when "
             "timesig-den is 4 and a measure with compound "
             "meter contains 4 beats, FOMUS multiplies this 4 by 2 to get a "
             "signature of 9/8)."
          //"If the time signature is explicitly given by setting `timesig' "
          //"then this setting has no effect (that is, as long as the explicit
          // time signature is valid).  " "This value may also be needed if
          // FOMUS needs to create a new measure (along with a new time
          // signature)."
          ;
    }
    const char* gettypedoc() const {
      return "integer_powof2>=1";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_int(val, 1, module_incl, 0, module_nobound,
                              timesigdenvalfun, gettypedoc());
    }
  };

  inline int validbeatfun(fomus_rat r) {
    return isexpof2(r);
  }

  class var_beat : public numvar {
public:
    var_beat() : numvar(makerat(1, 4)) {
      assert(getid() == BEAT_ID);
      initmodval();
    }
    var_beat(const fint val) : numvar(val) {
      initmodval();
    }
    var_beat(const var_beat& x, const filepos& pos) : numvar(x, pos) {
      initmodval();
    }
    var_beat(const var_beat& x, const numb& v, const filepos& pos)
        : numvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_beat(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_beat(*this, v, p);
    }
    varbase* getnew(const rat& v, const filepos& p) const {
      return new var_beat(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_beat(*this, v, p);
    }

    const char* getname() const {
      return "beat";
    } // docscat{basic}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "Specifies what notated rhythmic value is equivalent to 1 beat.  "
             "Set this value globally to change how durations are notated in "
             "the score.  "
             "The default value of 1/4, for example, specifies that a note "
             "with a duration of 1 is to be notated using a quarter note.  "
             "Like `timesig-den', the number becomes a dotted value when the "
             "meter is compound.  The default notation then for a note with "
             "duration 1 in compound meter "
             "(and a `beat' value of 1/4) is a dotted quarter note.";
    }
    const char* gettypedoc() const {
      return "rational_1/powof2>0 | rational_powof2>0";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_rat(mval, makerat(0, 1), module_excl, makerat(0, 1),
                              module_nobound, validbeatfun, gettypedoc());
    }
  };

  class var_timesig : public listvarofnums {
public:
    var_timesig() : listvarofnums() {
      assert(getid() == TIMESIG_ID);
      el.push_back((fomus_int) 0);
      el.push_back((fomus_int) 1);
      initmodval();
    }
    var_timesig(const listelvect& list) : listvarofnums(list) {
      initmodval();
    }
    var_timesig(const var_timesig& x, const filepos& pos)
        : listvarofnums(x, pos) {
      initmodval();
    }
    var_timesig(const var_timesig& x, const listelvect& s, const filepos& pos)
        : listvarofnums(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_timesig(*this, s, p);
    }

    const char* getname() const {
      return "timesig";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "An explicit time signature definition, which overrides the one "
             "FOMUS calculates for you using `timesig-den'.  "
             "The first value in the list is the numerator and the second "
             "value is the denominator.  "
             "The default value of `(0 1)' means no explicit time signature is "
             "given.  "
             "FOMUS looks at this setting first when trying to calculate time "
             "signatures.  "
             "If `timesigs' isn't valid (e.g., when a measure is split or two "
             "measures are merged together) then "
             "a valid time signature is searched for in `timesigs'.  If a "
             "valid one isn't found there either then FOMUS ignores all "
             "user-supplied time signatures and "
             "creates one using `timesig-den'.";
    }
    const char* gettypedoc() const {
      return "(integer>=0 integer_powof2>=1)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofints(mval, 2, 2, 0, module_incl, 0,
                                     module_nobound, timesigvalfun,
                                     gettypedoc());
    }
  };

  inline int timesigsvalfun(int n, struct module_value val) {
    return module_valid_listofints(val, 2, 2, 1, module_incl, 0, module_nobound,
                                   timesigvalfun, 0);
  }

  class var_timesigs : public listvaroflistofnums {
public:
    var_timesigs() : listvaroflistofnums() {
      assert(getid() == TIMESIGS_ID);
      initmodval();
    }
    var_timesigs(const listelvect& list) : listvaroflistofnums(list) {
      initmodval();
    }
    var_timesigs(const var_timesigs& x, const filepos& pos)
        : listvaroflistofnums(x, pos) {
      initmodval();
    }
    var_timesigs(const var_timesigs& x, const listelvect& s, const filepos& pos)
        : listvaroflistofnums(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_timesigs(*this, s, p);
    }

    const char* getname() const {
      return "timesigs";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A list of time signatures that FOMUS can choose from when it "
             "creates a new time signature.  "
             "The format of each time signature is the same as in `timesig'.  "
             //"When fomus creates a new measure (by splitting or merging
             // existing measures) it must also create a new time signature for
             // it.  "
             "FOMUS looks at `timesig' first when trying to calculate time "
             "signatures.  "
             "If `timesig' isn't valid then one is searched for in this list.  "
             "If a valid one isn't found here either then FOMUS ignores all "
             "user-supplied time signatures and "
             "creates one using `timesig-den'."
          // "By default the `timesig-den' setting is used to derive any new
          // time signatures--" "if FOMUS finds a valid time signature in this
          // list, however, it will use that one instead."
          ;
    }
    const char* gettypedoc() const {
      return "((integer>=1 integer_powof2>=1) (integer>=1 integer_powof2>=1) "
             "...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofvals(mval, -1, -1, timesigsvalfun,
                                     gettypedoc());
      // return module_valid_listofintsaux(mval, 1, module_incl, 0,
      // module_nobound, gettypedoc()) && mval.val.l.n == 2 &&
      // isexpof2(mval.val.l.vals[1].val.i);
    }
  };

  class var_timesigstyle : public boolvar {
public:
    var_timesigstyle() : boolvar((fint) false) {
      assert(getid() == TIMESIGSTYLE_ID);
      initmodval();
    }
    var_timesigstyle(const fint val) : boolvar(val) {
      initmodval();
    }
    var_timesigstyle(const var_timesigstyle& x, const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_timesigstyle(const var_timesigstyle& x, const numb& v,
                     const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_timesigstyle(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_timesigstyle(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_timesigstyle(*this, v, p);
    }

    const char* getname() const {
      return "timesig-c";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "If set to true, specifies that common and cut time signatures "
             "are to be notated with \"c\" symbols rather than numbers.  "
             "Set this globally or inside a measure definition.";
    }
    // const char* gettypedoc() const {return "||";}
    // bool isvalid() {return module_valid_intaux(val.getmodval(), 0,
    // module_incl, 2, module_incl, gettypedoc());}
  };

  class var_name : public strvar {
public:
    var_name() : strvar("") {
      assert(getid() == NAME_ID);
      initmodval();
    }
    var_name(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_name(const var_name& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_name(const var_name& x, const std::string& s, const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_name(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_name(*this, s, p);
    }

    const char* getname() const {
      return "name";
    } // docscat{instsparts}
    module_setting_loc getloc() const {
      return module_locpart;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "The name of an instrument or part.  This appears in the score to "
             "the left of the first staff system.  "
             "Set this when defining an instrument, percussion instrument or "
             "part.";
    }
    // const char* gettypedoc() const {return "measure|note|note-naturals";}
    bool isvalid(const fomusdata* fd) {
      return true;
    }
  };

  class var_abbr : public strvar {
public:
    var_abbr() : strvar("") {
      assert(getid() == ABBR_ID);
      initmodval();
    }
    var_abbr(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_abbr(const var_abbr& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_abbr(const var_abbr& x, const std::string& s, const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_abbr(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_abbr(*this, s, p);
    }

    const char* getname() const {
      return "abbr";
    } // docscat{instsparts}
    module_setting_loc getloc() const {
      return module_locpart;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "An abbreviated name of an instrument or part.  This string "
             "appears to the left of all staff systems except the first.  "
             "Set this when defining an instrument, percussion instrument or "
             "part.";
    }
    // const char* gettypedoc() const {return "measure|note|note-naturals";}
    bool isvalid(const fomusdata* fd) {
      return true;
    }
  };

  class var_percname : public strvar {
public:
    var_percname() : strvar("") {
      assert(getid() == PERCNAME_ID);
      initmodval();
    }
    var_percname(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_percname(const var_percname& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_percname(const var_percname& x, const std::string& s,
                 const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_percname(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_percname(*this, s, p);
    }

    const char* getname() const {
      return "perc-name";
    } // docscat{instsparts}
    module_setting_loc getloc() const {
      return module_locpercinst;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "The name of a percussion instrument.  This setting has a "
             "different meaning than the `name' setting.  "
             "It's value appears above the staff whenever a percussion "
             "instrument change needs to be indicated in the score.  "
             "Set this value when defining an percussion instrument that "
             "should be notated this way.";
    }
    // const char* gettypedoc() const {return "measure|note|note-naturals";}
    bool isvalid(const fomusdata* fd) {
      return true;
    }
  };

  class var_mpartsmod : public listofmods {
public:
    var_mpartsmod() : listofmods(module_modmetaparts) {
      assert(getid() == METAPARTSMOD_ID);
      el.push_back("mparts"); /*initmodval();*/
    }
    var_mpartsmod(const listelvect& str)
        : listofmods(str, module_modmetaparts) { /*initmodval();*/
    }
    var_mpartsmod(const var_mpartsmod& x, const filepos& pos)
        : listofmods(x, pos, module_modmetaparts) { /*initmodval();*/
    }
    var_mpartsmod(const var_mpartsmod& x, const listelvect& s,
                  const filepos& pos)
        : listofmods(x, s, pos, module_modmetaparts) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_mpartsmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_mpartsmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-metaparts";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for determining how notes "
             "are distributed from metaparts to score parts."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modmetaparts);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modmetaparts /*, "metaparts"*/);
    }
  };

  class var_pnotesmod : public listofmods {
public:
    var_pnotesmod() : listofmods(module_modpercnotes) {
      assert(getid() == PERCNOTESMOD_ID);
      el.push_back("pnotes"); /*initmodval();*/
    }
    var_pnotesmod(const listelvect& str)
        : listofmods(str, module_modpercnotes) { /*initmodval();*/
    }
    var_pnotesmod(const var_pnotesmod& x, const filepos& pos)
        : listofmods(x, pos, module_modpercnotes) { /*initmodval();*/
    }
    var_pnotesmod(const var_pnotesmod& x, const listelvect& s,
                  const filepos& pos)
        : listofmods(x, s, pos, module_modpercnotes) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_pnotesmod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_pnotesmod(*this, s, p);
    }

    const char* getname() const {
      return "mod-percnotes";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    } //*
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for determining how "
             "percussion notes are laid out."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modpercnotes);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modpercnotes /*, "percnotes"*/);
    }
  };

  class var_markevs1mod : public listofmods {
public:
    var_markevs1mod() : listofmods(module_modmarkevs) {
      assert(getid() == MARKEVS1MOD_ID);
      el.push_back("markevs1");
      el.push_back("markevs2"); /*initmodval();*/
    }
    var_markevs1mod(const listelvect& str)
        : listofmods(str, module_modmarkevs) { /*initmodval();*/
    }
    var_markevs1mod(const var_markevs1mod& x, const filepos& pos)
        : listofmods(x, pos, module_modmarkevs) { /*initmodval();*/
    }
    var_markevs1mod(const var_markevs1mod& x, const listelvect& s,
                    const filepos& pos)
        : listofmods(x, s, pos, module_modmarkevs) { /*initmodval();*/
    }

    // varbase* copy() const {return new var_quantmod(*this);}
    // varbase* getnew(const char* s, const filepos& p) const {return new
    // var_markevs1mod(*this, s, p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_markevs1mod(*this, s, p);
    }

    const char* getname() const {
      return "mod-markevents";
    } // docscat{mods}
    module_setting_loc getloc() const {
      return module_locnote;
    } //*
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "Module or list of modules responsible for determining how mark "
             "events are interpretted before quantizing."
          // "  The list order affects (but might not completely determine) the
          // order in which modules are called."
          ;
    }
    const char* gettypedoc() const {
      return listofmods::gettypedoc(module_modmarkevs);
    }
    bool isvalid(const fomusdata* fd) {
      return listofmods::isvalid(module_modmarkevs);
    }
  };

  class var_dursymbols : public ordmapvartonums {
public:
    var_dursymbols();
    var_dursymbols(const listelmap& map) : ordmapvartonums(map) {
      initmodval();
    }
    var_dursymbols(const var_dursymbols& x, const filepos& pos)
        : ordmapvartonums(x, pos) {
      initmodval();
    }
    var_dursymbols(const var_dursymbols& x, const listelmap& s,
                   const filepos& pos)
        : ordmapvartonums(x, s, pos) {
      initmodval();
    }
    var_dursymbols(const var_dursymbols& x, const listelmap& s,
                   const listelvect& v, const filepos& pos)
        : ordmapvartonums(x, s, v, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_dursymbols(*this, nestedlisttomap(s, p), s, p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_dursymbols(*this, s, p);
    }

    const char* getname() const {
      return "dur-symbols";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A mapping from duration symbols to duration values."
             "  Use this to customize the duration symbols used in a `.fms' "
             "file."
             "  `dur-symbols', `dur-dots', 'dur-tie' and 'tuplet-symbols' "
             "together determine a full duration by concatenating and "
             "\"tying\" them together."
             "  Although the symbols might signify notated rhythms, they "
             "translate to fomus time values and not necessarily the rhythmic "
             "values that appear in the score.";
    }
    const char* gettypedoc() const {
      return "(string_symb rational>0..128, string_symb rational>0..128, ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptorats(mval, -1, -1, makerat(0, 1), module_excl,
                                    makerat(128, 1), module_incl, 0,
                                    gettypedoc());
    }
    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = ordmapvartonumsparse(
          el, ord, *this, pos, x,
          false)[activate_set<var_dursymbols>(*this, x.ta)];
    }
    void activate(const symtabs& ta) const {
      fomsymbols<numb> ne;
      for (listelmap_constit i(el.begin()); i != el.end(); ++i)
        ne.add(i->first.c_str(), listel_getnumb(i->second));
      ta.dursyms = ne;
    }
    bool getreset() const {
      return true;
    }
  };
  class var_durdot : public ordmapvartonums {
public:
    var_durdot();
    var_durdot(const listelmap& map) : ordmapvartonums(map) {
      initmodval();
    }
    var_durdot(const var_durdot& x, const filepos& pos)
        : ordmapvartonums(x, pos) {
      initmodval();
    }
    var_durdot(const var_durdot& x, const listelmap& s, const filepos& pos)
        : ordmapvartonums(x, s, pos) {
      initmodval();
    }
    var_durdot(const var_durdot& x, const listelmap& s, const listelvect& v,
               const filepos& pos)
        : ordmapvartonums(x, s, v, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_durdot(*this, nestedlisttomap(s, p), s, p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_durdot(*this, s, p);
    }

    const char* getname() const {
      return "dur-dots";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A mapping from dot strings to duration multipliers."
             "  Use this to customize the duration symbols used in a `.fms' "
             "file."
             "  `dur-symbols', `dur-dots', 'dur-tie' and 'tuplet-symbols' "
             "together determine a full duration by concatenating and "
             "\"tying\" them together."
             "  A multipler of 3/2, for example, indicates a typical dotted "
             "note modification, though other non-standard multipliers may be "
             "added.";
    }
    const char* gettypedoc() const {
      return "(string_dot rational>0..128, string_dot rational>0..128, ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptorats(mval, -1, -1, makerat(0, 1), module_excl,
                                    makerat(128, 1), module_incl, 0,
                                    gettypedoc());
    }
    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = ordmapvartonumsparse(
          el, ord, *this, pos, x, false)[activate_set<var_durdot>(*this, x.ta)];
    }
    void activate(const symtabs& ta) const {
      fomsymbols<numb> ne;
      for (listelmap_constit i(el.begin()); i != el.end(); ++i)
        ne.add(i->first.c_str(), listel_getnumb(i->second));
      ta.durdot = ne;
    }
    bool getreset() const {
      return true;
    }
  };
  class var_durtie : public listvarofstrings {
public:
    var_durtie();
    var_durtie(const listelvect& map) : listvarofstrings(map) {
      initmodval();
    }
    var_durtie(const var_durtie& x, const filepos& pos)
        : listvarofstrings(x, pos) {
      initmodval();
    }
    var_durtie(const var_durtie& x, const listelvect& s, const filepos& pos)
        : listvarofstrings(x, s, pos) {
      initmodval();
    }

    // varbase* getnew(const listelvect &s, const filepos& p) const {return new
    // var_durtie(*this, nestedlisttomap(s, p), p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_durtie(*this, s, p);
    }

    const char* getname() const {
      return "dur-tie";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "This string can be used to add or \"tie\" durations together."
             "  Use this to customize the duration symbols used in a `.fms' "
             "file."
             "  Complex durations may be constructed by attaching "
             "symbol/dut/tuplet constructs together with one of these strings.";
    }
    const char* gettypedoc() const {
      return "(string_tie string_tie ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofstrings(mval, -1, -1, 1, -1, 0, gettypedoc());
    }
    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] =
          listvarofstringsparse(el, *this, pos, x, "),",
                                false)[activate_set<var_durtie>(*this, x.ta)];
    }
    void activate(const symtabs& ta) const {
      fomsymbols<numb> ne;
      for (listelvect_constit i(el.begin()); i != el.end(); ++i)
        ne.add(listel_getstring(*i).c_str(), numb((fomus_int) 0));
      ta.durtie = ne;
    }
    bool getreset() const {
      return true;
    }
  };
  class var_tupsyms : public ordmapvartonums {
public:
    var_tupsyms();
    var_tupsyms(const listelmap& map) : ordmapvartonums(map) {
      initmodval();
    }
    var_tupsyms(const var_tupsyms& x, const filepos& pos)
        : ordmapvartonums(x, pos) {
      initmodval();
    }
    var_tupsyms(const var_tupsyms& x, const listelmap& s, const filepos& pos)
        : ordmapvartonums(x, s, pos) {
      initmodval();
    }
    var_tupsyms(const var_tupsyms& x, const listelmap& s, const listelvect& v,
                const filepos& pos)
        : ordmapvartonums(x, s, v, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_tupsyms(*this, nestedlisttomap(s, p), s, p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_tupsyms(*this, s, p);
    }

    const char* getname() const {
      return "tuplet-symbols";
    } // docscat{syntax}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A mapping from tuplet strings to duration multipliers."
             "  Use this to customize the duration symbols used in a `.fms' "
             "file."
             "  `dur-symbols', `dur-dots', 'dur-tie' and 'tuplet-symbols' "
             "together determine a full duration by concatenating and "
             "\"tying\" them together."
             "  A multipler of 2/3, for example, indicates a typical triplet "
             "note modification.";
    }
    const char* gettypedoc() const {
      return "(string rational>0..128, string rational>0..128, ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptorats(mval, -1, -1, makerat(0, 1), module_excl,
                                    makerat(128, 1), module_incl, 0,
                                    gettypedoc());
    }
    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] =
          ordmapvartonumsparse(el, ord, *this, pos, x,
                               false)[activate_set<var_tupsyms>(*this, x.ta)];
    }
    void activate(const symtabs& ta) const {
      fomsymbols<numb> ne;
      for (listelmap_constit i(el.begin()); i != el.end(); ++i)
        ne.add(i->first.c_str(), listel_getnumb(i->second));
      ta.tupsyms = ne;
    }
    bool getreset() const {
      return true;
    }
  };

  extern bool dumping;
  class var_dumpingmsg : public boolvar {
public:
    var_dumpingmsg()
        : boolvar(
#ifndef NDEBUGOUT
              (fint) true
#else
              (fint) false
#endif
          ) {
      assert(getid() == DUMPINGMSG_ID);
      initmodval();
    }
    var_dumpingmsg(const fint val) : boolvar(val) {
      initmodval();
    }
    var_dumpingmsg(const var_dumpingmsg& x, const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_dumpingmsg(const var_dumpingmsg& x, const numb& v, const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_dumpingmsg(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_dumpingmsg(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_dumpingmsg(*this, v, p);
    }

    const char* getname() const {
      return "dump-api";
    } // docscat{other}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 3;
    }
    const char* getdescdoc() const {
      return "If set to true, FOMUS dumps API messages when they are received."
             "  This can be used to find the proper messages to send to "
             "FOMUS's C API for specifying input events and settings."
             "  To find the proper messages for entering in a list of voices, "
             "for example, enter `voice (1 2 3)' into a `.fms' file (or send "
             "it via the `fomus_parse' function) "
             "and watch what gets printed out.";
    }

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] = boolvarparse(
          val, *this, pos, x, false)[activate_set<var_dumpingmsg>(*this, x.ta)];
    }
    void activate(const symtabs& ta) const {
      dumping = numtoint(val);
    }
  };

  class var_title : public strvar {
public:
    var_title() : strvar("") {
      assert(getid() == TITLE_ID);
      initmodval();
    }
    var_title(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_title(const var_title& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_title(const var_title& x, const std::string& s, const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_title(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_title(*this, s, p);
    }

    const char* getname() const {
      return "title";
    } // docscat{basic}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "The title of the piece."
             "  This appears in the score at the top of the first page.";
    }
    // const char* gettypedoc() const {return "measure|note|note-naturals";}
    bool isvalid(const fomusdata* fd) {
      return true;
    }
  };

  class var_author : public strvar {
public:
    var_author() : strvar("") {
      assert(getid() == AUTHOR_ID);
      initmodval();
    }
    var_author(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_author(const var_author& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_author(const var_author& x, const std::string& s, const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_author(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_author(*this, s, p);
    }

    const char* getname() const {
      return "author";
    } // docscat{basic}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "The author of the piece."
             "  This appears in the score at the top right corner of the first "
             "page.";
    }
    // const char* gettypedoc() const {return "measure|note|note-naturals";}
    bool isvalid(const fomusdata* fd) {
      return true;
    }
  };

  class var_acc : public listvarofstrings {
public:
    var_acc() : listvarofstrings() {
      assert(getid() == ACC_ID);
      initmodval();
    }
    var_acc(const listelvect& map) : listvarofstrings(map) {
      initmodval();
    }
    var_acc(const var_acc& x, const filepos& pos) : listvarofstrings(x, pos) {
      initmodval();
    }
    var_acc(const var_acc& x, const listelvect& s, const filepos& pos)
        : listvarofstrings(x, s, pos) {
      initmodval();
    }

    // varbase* getnew(const listelvect &s, const filepos& p) const {return new
    // var_acc(*this, nestedlisttomap(s, p), p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_acc(*this, s, p);
    }

    const char* getname() const {
      return "acc";
    } // docscat{notes}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "Specifies an accidental or choice of accidentals."
             "  Use this in a note event or over a range of events to "
             "influence accidental spelling."
             "  The accidental strings are the same as note strings without "
             "the base pitch or octave."
             "  Specifying one accidental effectively overrides FOMUS's "
             "decision (if the accidental is valid)."
             "  An example of a safe way to override whether FOMUS chooses "
             "sharps or flats is to specify something like (n s ss) or (n f "
             "ff) "
             "(you must use whatever the symbols for naturals sharps and flats "
             "are as defined in `note-accs').";
    }
    const char* gettypedoc() const {
      return "string_acc | (string_acc string_acc ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      if (!module_valid_listofstrings(mval, -1, -1, 1, -1, 0, gettypedoc()))
        return false;
      for (const module_value *i(mval.val.l.vals),
           *ie(mval.val.l.vals + mval.val.l.n);
           i < ie; ++i) {
        if (doparseacc(fd, i->val.s, true, 0).isnull())
          return false;
      }
      return true;
    }
  };

  class var_truncate : public listvarofnums {
public:
    var_truncate() : listvarofnums() {
      assert(getid() == CLIP_ID);
      el.push_back((fomus_int) 0);
      el.push_back((fomus_int) 0);
      initmodval();
    }
    var_truncate(const listelvect& list) : listvarofnums(list) {
      initmodval();
    }
    var_truncate(const var_truncate& x, const filepos& pos)
        : listvarofnums(x, pos) {
      initmodval();
    }
    var_truncate(const var_truncate& x, const listelvect& s, const filepos& pos)
        : listvarofnums(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_truncate(*this, s, p);
    }

    const char* getname() const {
      return "clip";
    } // docscat{other}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 1;
    }
    const char* getdescdoc() const {
      return "A list of two numbers specifying a \"clipped\" segment of the "
             "score to appear in the output.  "
             "Useful if you only want to view a small sample of the output "
             "(all objects with time offsets outside the clip range are "
             "discarded).  "
             "Set the first number in the list to the start time of the "
             "segment and the second number to the end time.  "
             "Set either one to 0 to indicate no start or end time (two zeros "
             "indicates no clipping).";
    }
    const char* gettypedoc() const {
      return "(rational>=0 rational>=0)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofrats(mval, 2, 2, makerat(0, 1), module_incl,
                                     makerat(0, 1), module_nobound, 0,
                                     gettypedoc());
    }
  };

  extern presetsmap presets;
  inline int valid_ispreset(int n, const char* val) {
    return presets.find(val) != presets.end();
  }
  void doloadconf(const boost::filesystem::path& fn);
  extern std::string presetstype;

  class var_presets : public listvarofstrings {
public:
    var_presets() : listvarofstrings() {
      assert(getid() == PRESETS_ID); /*el.push_back("min");*/ /*maketypedoc();*/
      initmodval();
    }
    var_presets(const listelvect& map)
        : listvarofstrings(map) { /*maketypedoc();*/
      initmodval();
    }
    var_presets(const var_presets& x, const filepos& pos)
        : listvarofstrings(x, pos) { /*maketypedoc();*/
      initmodval();
    }
    var_presets(const var_presets& x, const listelvect& s, const filepos& pos)
        : listvarofstrings(x, s, pos) { /*maketypedoc();*/
      initmodval();
    }

    // varbase* getnew(const listelvect &s, const filepos& p) const {return new
    // var_presets(*this, nestedlisttomap(s, p), p);}
    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_presets(*this, s, p);
    }

    const char* getname() const {
      return "presets";
    } // docscat{basic}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 0;
    }
    const char* getdescdoc() const {
      return "A list of presets that are loaded immediately.  "
             "Each preset is a string representing a base filename (excluding "
             "the extension) in FOMUS's \"presets\" path.  "
             //"Specifying a list is the same as setting the value to each
             // string in the list individually.  "
             "If the preset file is found, it is effectively included at that "
             "point.  "
             "A preset file has the same format as the `.fomus' or "
             "`fomus.conf' file (i.e., it may only contain global settings) "
             "and has an `.fpr' extension.  "
             "You can define your own presets by placing `.fpr' files in a "
             "directory somewhere and adding the directory name "
             "to the FOMUS_PRESETS_PATH environment variable (e.g., put "
             "`export "
             "FOMUS_PRESETS_PATH=\"/home/me/presetdir1:/home/me/presetdir2\"' "
             "in your .bash_profile script).";
    }
    const char* gettypedoc() const; // {return typstr.c_str();}
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_listofstrings(mval, -1, -1, 1, -1, valid_ispreset,
                                        gettypedoc());
    }

    void addconfrule(parserule* rules, confscratch& x) {
      rules[getid()] =
          listvarofstringsparse(el, *this, pos, x, "),",
                                false)[activate_set<var_presets>(*this, x.ta)];
    }
    void activate(const symtabs& ta) const;
    bool getreset() const {
      return true;
    }
  };

  extern std::map<std::string, module_barlines, isiless> barlines;
  inline int valid_isbarline(const char* val) {
    DBG(" checking if barline `" << val << "' is valid" << std::endl);
    return barlines.find(val) != barlines.end();
  }

  class var_barlinel : public strvar {
public:
    var_barlinel() : strvar("") {
      assert(getid() == BARLINEL_ID);
      initmodval();
    }
    var_barlinel(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_barlinel(const var_barlinel& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_barlinel(const var_barlinel& x, const std::string& s,
                 const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_barlinel(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_barlinel(*this, s, p);
    }

    const char* getname() const {
      return "left-barline";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 1;
    }
    const char* gettypedoc() const {
      return "string_barline";
    }
    const char* getdescdoc() const {
      return "The barline that appears to the left of the first measure that "
             "contains this setting.  "
             "Possible values are `' or `!' (normal barline), `!!' (double "
             "barline), `|!' (initial barline), `!|' (final bareline), `:' "
             "(dotted), `;' (dashed), "
             "`|:' (repeat right), `:|:' (repeat left and right), `:|' (repeat "
             "left).";
    }
    // const char* gettypedoc() const {return "measure|note|note-naturals";}
    bool isvalid(const fomusdata* fd) {
      DBG("left varline isvalid?" << std::endl);
      return module_valid_string(mval, -1, -1, valid_isbarline, gettypedoc());
    }
  };

  class var_barliner : public strvar {
public:
    var_barliner() : strvar("") {
      assert(getid() == BARLINER_ID);
      initmodval();
    }
    var_barliner(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_barliner(const var_barliner& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_barliner(const var_barliner& x, const std::string& s,
                 const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_barliner(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_barliner(*this, s, p);
    }

    const char* getname() const {
      return "right-barline";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locmeasdef;
    }
    int getuselevel() const {
      return 1;
    }
    const char* gettypedoc() const {
      return "string_barline";
    }
    const char* getdescdoc() const {
      return "The barline that appears to the left of the first measure that "
             "contains this setting.  "
             "Possible values are `' or `!' (normal barline), `!!' (double "
             "barline), `|!' (initial barline), `!|' (final bareline), `:' "
             "(dotted), `;' (dashed), "
             "`|:' (repeat right), `:|:' (repeat left and right), `:|' (repeat "
             "left).";
    }
    // const char* gettypedoc() const {return "measure|note|note-naturals";}
    bool isvalid(const fomusdata* fd) {
      DBG("right varline isvalid?" << std::endl);
      return module_valid_string(mval, -1, -1, valid_isbarline, gettypedoc());
    }
  };

  class var_finalbar : public boolvar {
public:
    var_finalbar() : boolvar((fint) true) {
      assert(getid() == FINALBAR_ID);
      initmodval();
    }
    var_finalbar(const fint val) : boolvar(val) {
      initmodval();
    }
    var_finalbar(const var_finalbar& x, const filepos& pos) : boolvar(x, pos) {
      initmodval();
    }
    var_finalbar(const var_finalbar& x, const numb& v, const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_finalbar(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_finalbar(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_finalbar(*this, v, p);
    }

    const char* getname() const {
      return "final-barline";
    } // docscat{meas}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Whether or not to automatically add a final barline.  "
             "Set this at the global level to tell FOMUS whether or not to add "
             "one.";
    }
    // const char* gettypedoc() const {return "||";}
    // bool isvalid() {return module_valid_intaux(val.getmodval(), 0,
    // module_incl, 2, module_incl, gettypedoc());}
  };

  class var_slurcantouchdef : public boolvar {
public:
    var_slurcantouchdef() : boolvar((fint) false) {
      assert(getid() == SLURCANTOUCHDEF_ID);
      initmodval();
    }
    var_slurcantouchdef(const fint val) : boolvar(val) {
      initmodval();
    }
    var_slurcantouchdef(const var_slurcantouchdef& x, const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_slurcantouchdef(const var_slurcantouchdef& x, const numb& v,
                        const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_slurcantouchdef(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_slurcantouchdef(*this, v, p);
    }

    const char* getname() const {
      return "slurs-cantouch";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Specifies whether slur marks can touch each other by default.  "
             "Use this in notes events, measures, parts or at the score level "
             "to affect how slurs spanners are interpretted and processed.  "
             "This setting can be overriden when specifying a slur by adding a "
             "`-' (can touch) or `|' (cannot touch) character to the mark.";
    }
  };

  class var_slurcanspanrestsdef : public boolvar {
public:
    var_slurcanspanrestsdef() : boolvar((fint) false) {
      assert(getid() == SLURCANSPANRESTSDEF_ID);
      initmodval();
    }
    var_slurcanspanrestsdef(const fint val) : boolvar(val) {
      initmodval();
    }
    var_slurcanspanrestsdef(const var_slurcanspanrestsdef& x,
                            const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_slurcanspanrestsdef(const var_slurcanspanrestsdef& x, const numb& v,
                            const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_slurcanspanrestsdef(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_slurcanspanrestsdef(*this, v, p);
    }

    const char* getname() const {
      return "slurs-canspanrests";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Specifies whether slur marks can span across rests by default.  "
             "Use this in notes events, measures, parts or at the score level "
             "to affect how slurs spanners are interpretted and processed.  "
             "This setting can be overriden when specifying a slur by adding "
             "an "
             "`r' (can span rests) or `n' (can only span notes) character to "
             "the mark.";
    }
  };

  class var_wedgecantouchdef : public boolvar {
public:
    var_wedgecantouchdef() : boolvar((fint) false) {
      assert(getid() == WEDGECANTOUCHDEF_ID);
      initmodval();
    }
    var_wedgecantouchdef(const fint val) : boolvar(val) {
      initmodval();
    }
    var_wedgecantouchdef(const var_wedgecantouchdef& x, const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_wedgecantouchdef(const var_wedgecantouchdef& x, const numb& v,
                         const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_wedgecantouchdef(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_wedgecantouchdef(*this, v, p);
    }

    const char* getname() const {
      return "wedges-cantouch";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Specifies whether crescendo/diminuendo wedge marks can touch "
             "each other by default.  "
             "Use this in notes events, measures, parts or at the score level "
             "to affect how crescendo/diminuendo spanners are interpretted and "
             "processed.  "
             "This setting can be overriden when specifying a wedge by adding "
             "a "
             "`-' (can touch) or `|' (cannot touch) character to the mark.";
    }
  };

  class var_wedgecanspanonedef : public boolvar {
public:
    var_wedgecanspanonedef() : boolvar((fint) false) {
      assert(getid() == WEDGECANSPANONEDEF_ID);
      initmodval();
    }
    var_wedgecanspanonedef(const fint val) : boolvar(val) {
      initmodval();
    }
    var_wedgecanspanonedef(const var_wedgecanspanonedef& x, const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_wedgecanspanonedef(const var_wedgecanspanonedef& x, const numb& v,
                           const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_wedgecanspanonedef(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_wedgecanspanonedef(*this, v, p);
    }

    const char* getname() const {
      return "wedges-canspanone";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Specifies whether crescendo/diminuendo wedge marks can span a "
             "single note by default.  "
             "Use this in notes events, measures, parts or at the score level "
             "to affect how crescendo/diminuendo spanners are interpretted and "
             "processed.  "
             "This setting can be overriden when specifying a wedge by adding "
             "a "
             "`1' (can span a single note) or `m' (cannot span a single note) "
             "character to the mark.";
    }
  };

  class var_wedgecanspanrestsdef : public boolvar {
public:
    var_wedgecanspanrestsdef() : boolvar((fint) false) {
      assert(getid() == WEDGECANSPANRESTSDEF_ID);
      initmodval();
    }
    var_wedgecanspanrestsdef(const fint val) : boolvar(val) {
      initmodval();
    }
    var_wedgecanspanrestsdef(const var_wedgecanspanrestsdef& x,
                             const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_wedgecanspanrestsdef(const var_wedgecanspanrestsdef& x, const numb& v,
                             const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_wedgecanspanrestsdef(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_wedgecanspanrestsdef(*this, v, p);
    }

    const char* getname() const {
      return "wedges-canspanrests";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Specifies whether crescendo/diminuendo wedge marks can touch "
             "each other by default.  "
             "Use this in notes events, measures, parts or at the score level "
             "to affect how crescendo/diminuendo spanners are interpretted and "
             "processed.  "
             "This setting can be overriden when specifying a wedge by adding "
             "a "
             "`-' (can touch) or `|' (cannot touch) character to the mark.";
    }
  };

  class var_textcantouchdef : public boolvar {
public:
    var_textcantouchdef() : boolvar((fint) false) {
      assert(getid() == TEXTCANTOUCHDEF_ID);
      initmodval();
    }
    var_textcantouchdef(const fint val) : boolvar(val) {
      initmodval();
    }
    var_textcantouchdef(const var_textcantouchdef& x, const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_textcantouchdef(const var_textcantouchdef& x, const numb& v,
                        const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_textcantouchdef(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_textcantouchdef(*this, v, p);
    }

    const char* getname() const {
      return "texts-cantouch";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Specifies whether text spanner marks can touch each other by "
             "default.  "
             "Use this in notes events, measures, parts or at the score level "
             "to affect how text spanners are interpretted and processed.  "
             "This setting can be overriden when specifying a text spanner by "
             "adding a "
             "`-' (can touch) or `|' (cannot touch) character to the mark.";
    }
  };

  class var_textcanspanonedef : public boolvar {
public:
    var_textcanspanonedef() : boolvar((fint) false) {
      assert(getid() == TEXTCANSPANONEDEF_ID);
      initmodval();
    }
    var_textcanspanonedef(const fint val) : boolvar(val) {
      initmodval();
    }
    var_textcanspanonedef(const var_textcanspanonedef& x, const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_textcanspanonedef(const var_textcanspanonedef& x, const numb& v,
                          const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_textcanspanonedef(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_textcanspanonedef(*this, v, p);
    }

    const char* getname() const {
      return "texts-canspanone";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Specifies whether text spanner marks can span a single note by "
             "default.  "
             "Use this in notes events, measures, parts or at the score level "
             "to affect how text spanners are interpretted and processed.  "
             "This setting can be overriden when specifying a text spanner by "
             "adding a "
             "`1' (can span a single note) or `m' (cannot span a single note) "
             "character to the mark.";
    }
  };

  class var_textcanspanrestsdef : public boolvar {
public:
    var_textcanspanrestsdef() : boolvar((fint) false) {
      assert(getid() == TEXTCANSPANRESTSDEF_ID);
      initmodval();
    }
    var_textcanspanrestsdef(const fint val) : boolvar(val) {
      initmodval();
    }
    var_textcanspanrestsdef(const var_textcanspanrestsdef& x,
                            const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_textcanspanrestsdef(const var_textcanspanrestsdef& x, const numb& v,
                            const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_textcanspanrestsdef(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_textcanspanrestsdef(*this, v, p);
    }

    const char* getname() const {
      return "texts-canspanrests";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Specifies whether text spanner marks can touch each other by "
             "default.  "
             "Use this in notes events, measures, parts or at the score level "
             "to affect how text spanners are interpretted and processed.  "
             "This setting can be overriden when specifying a text spanner by "
             "adding a "
             "`-' (can touch) or `|' (cannot touch) character to the mark.";
    }
  };

  enum enum_sulstyles { ss_letter, ss_sulletter, ss_roman, ss_sulroman };
  extern std::map<std::string, enum_sulstyles, isiless> sulstyles;
  inline int valid_issulstyle(const char* val) {
    return sulstyles.find(val) != sulstyles.end();
  }

  class var_sulstyle : public strvar {
public:
    var_sulstyle() : strvar("sulletter") {
      assert(getid() == SULSTYLE_ID);
      initmodval();
    }
    var_sulstyle(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_sulstyle(const var_sulstyle& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_sulstyle(const var_sulstyle& x, const std::string& s,
                 const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_sulstyle(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_sulstyle(*this, s, p);
    }

    const char* getname() const {
      return "sul-style";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Determines the style of a `sul' text marking."
             "  Possible values are `sulletter' for \"sul\" to be followed by "
             "a note string name, `sulroman' for \"sul\" to be followed by a "
             "Roman numeral,"
             " `letter' for a letter only and `roman' for a Roman numeral "
             "only.";
    }
    // const char* gettypedoc() const {return "measure|note|note-naturals";}
    bool isvalid(const fomusdata* fd) {
      return module_valid_string(mval, -1, -1, valid_issulstyle, gettypedoc());
    }
  };

  inline int valid_marktexts(int n, const char* sym, const char* val) {
    return (marksbyname.find(sym) != marksbyname.end());
  }

  class var_defmarktexts : public mapvartostrings {
public:
    var_defmarktexts();
    var_defmarktexts(const listelmap& map) : mapvartostrings(map) {
      initmodval();
    }
    var_defmarktexts(const var_defmarktexts& x, const filepos& pos)
        : mapvartostrings(x, pos) {
      initmodval();
    }
    var_defmarktexts(const var_defmarktexts& x, const listelmap& s,
                     const filepos& pos)
        : mapvartostrings(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_defmarktexts(*this, nestedlisttomap(s, p), p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_defmarktexts(*this, s, p);
    }

    const char* getname() const {
      return "default-mark-texts";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A mapping from mark IDs to mark texts."
             "  Use this setting to customize the text that appears in the "
             "score for marks that are normally represented by text above or "
             "below the staff (e.g., `con sord.', `legato', etc.)."
             "  `default-mark-texts' contains the default texts for all such "
             "marks while `mark-texts' might contain overrides only for a "
             "particular score or part."
             // "  When FOMUS outputs the texts for these marks, it looks first
             // in `mark-texts' and then in `default-mark-texts'."
             "  This setting should appear in your `.fomus' file and should be "
             "modified using `+=' to insure that you are only replacing "
             "entries in this setting.";
    }
    const char* gettypedoc() const {
      return "(string_mark string_text, string_mark string_text, ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptostrings(mval, -1, -1, -1, -1, valid_marktexts,
                                       gettypedoc());
    }
  };

  class var_marktexts : public mapvartostrings {
public:
    var_marktexts() : mapvartostrings() {
      assert(getid() == MARKTEXTS_ID);
      initmodval();
    }
    var_marktexts(const listelmap& map) : mapvartostrings(map) {
      initmodval();
    }
    var_marktexts(const var_marktexts& x, const filepos& pos)
        : mapvartostrings(x, pos) {
      initmodval();
    }
    var_marktexts(const var_marktexts& x, const listelmap& s,
                  const filepos& pos)
        : mapvartostrings(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_marktexts(*this, nestedlisttomap(s, p), p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_marktexts(*this, s, p);
    }

    const char* getname() const {
      return "mark-texts";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locpart;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A mapping from mark IDs to mark texts."
             "  Use this setting to customize the text that appears in the "
             "score for marks that are normally represented by text above or "
             "below the staff (e.g., `con sord.', `legato', etc.)."
             "  `mark-texts' might contain overrides only for a particular "
             "score or part while `default-mark-texts' contains the default "
             "texts for all such marks."
             "  This setting should be used in a score or individual part.";
    }
    const char* gettypedoc() const {
      return "(string_mark string_text, string_mark string_text, ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptostrings(mval, -1, -1, -1, -1, valid_marktexts,
                                       gettypedoc());
    }
  };

  class var_markaliases : public mapvartostrings {
public:
    var_markaliases() : mapvartostrings() {
      assert(getid() == MARKALIASES_ID);
      initmodval();
    }
    var_markaliases(const listelmap& map) : mapvartostrings(map) {
      initmodval();
    }
    var_markaliases(const var_markaliases& x, const filepos& pos)
        : mapvartostrings(x, pos) {
      initmodval();
    }
    var_markaliases(const var_markaliases& x, const listelmap& s,
                    const filepos& pos)
        : mapvartostrings(x, s, pos) {
      initmodval();
    }

    varbase* getnew(const listelvect& s, const filepos& p) const {
      return new var_markaliases(*this, nestedlisttomap(s, p), p);
    }
    varbase* getnew(const listelmap& s, const filepos& p) const {
      return new var_markaliases(*this, s, p);
    }

    const char* getname() const {
      return "markaliases";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locpart;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "A mapping from text mark strings to texts."
             "  Use this setting to effectively create aliases for `x' text "
             "marks."
             "  By mapping \"pcresc\" to \"crescendo poco a poco\", for "
             "example, you can then specify `[x_ pcresc]' in a `.fms' file to "
             "make the desired text appear in the score."
             "  The abbreviated alias string is also recognized in the "
             "`mark-groups' and `mark-group-defs' settings."
             "  This setting should be used in a score or individual part.";
    }
    const char* gettypedoc() const {
      return "(string_alias string_text, string_alias string_text, ...)";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_maptostrings(mval, -1, -1, -1, -1, 0, gettypedoc());
    }
  };

  class var_pedcantouchdef : public boolvar {
public:
    var_pedcantouchdef() : boolvar((fint) true) {
      assert(getid() == PEDCANTOUCHDEF_ID);
      initmodval();
    }
    var_pedcantouchdef(const fint val) : boolvar(val) {
      initmodval();
    }
    var_pedcantouchdef(const var_pedcantouchdef& x, const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_pedcantouchdef(const var_pedcantouchdef& x, const numb& v,
                       const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_pedcantouchdef(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_pedcantouchdef(*this, v, p);
    }

    const char* getname() const {
      return "peds-cantouch";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Specifies whether piano pedal marks can touch each other by "
             "default.  "
             "Use this in notes events, measures, parts or at the score level "
             "to affect how pedal mark spanners are interpretted and "
             "processed.  "
             "This setting can be overriden when specifying a pedal sign by "
             "adding a "
             "`-' (can touch) or `|' (cannot touch) character to the mark.";
    }
  };

  class var_pedcanspanonedef : public boolvar {
public:
    var_pedcanspanonedef() : boolvar((fint) true) {
      assert(getid() == PEDCANSPANONEDEF_ID);
      initmodval();
    }
    var_pedcanspanonedef(const fint val) : boolvar(val) {
      initmodval();
    }
    var_pedcanspanonedef(const var_pedcanspanonedef& x, const filepos& pos)
        : boolvar(x, pos) {
      initmodval();
    }
    var_pedcanspanonedef(const var_pedcanspanonedef& x, const numb& v,
                         const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    varbase* getnew(const fint v, const filepos& p) const {
      return new var_pedcanspanonedef(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_pedcanspanonedef(*this, v, p);
    }

    const char* getname() const {
      return "peds-canspanone";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Specifies whether piano pedal marks can span a single note by "
             "default.  "
             "Use this in notes events, measures, parts or at the score level "
             "to affect how pedal mark spanners are interpretted and "
             "processed.  "
             "This setting can be overriden when specifying a pedal sign by "
             "adding a "
             "`1' (can span a single note) or `m' (cannot span a single note) "
             "character to the mark.";
    }
  };

  extern std::set<std::string, isiless> pedstyles;
  inline int valid_ispedstyle(const char* val) {
    return pedstyles.find(val) != pedstyles.end();
  }

  class var_pedstyle : public strvar {
public:
    var_pedstyle() : strvar("text") {
      assert(getid() == PEDSTYLE_ID);
      initmodval();
    }
    var_pedstyle(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_pedstyle(const var_pedstyle& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_pedstyle(const var_pedstyle& x, const std::string& s,
                 const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_pedstyle(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_pedstyle(*this, s, p);
    }

    const char* getname() const {
      return "ped-style";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Determines the style of a piano pedal marking."
             "  Possible values are `text' for \"Ped\" and \"*\" text marks "
             "and `bracket' for lines and V-shapes.";
    }
    bool isvalid(const fomusdata* fd) {
      return module_valid_string(mval, -1, -1, valid_ispedstyle, gettypedoc());
    }
  };

  enum enum_tiestyles {
    tiestyle_none,
    tiestyle_tied,
    tiestyle_dashed,
    tiestyle_dotted
  };
  extern std::map<std::string, enum_tiestyles, isiless> tremties;
  inline int valid_istremtie(const char* val) {
    return tremties.find(val) != tremties.end();
  }

  class var_tremtie : public strvar {
public:
    var_tremtie() : strvar("none") {
      assert(getid() == TREMTIE_ID);
      initmodval();
    }
    var_tremtie(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_tremtie(const var_tremtie& x, const filepos& pos) : strvar(x, pos) {
      initmodval();
    }
    var_tremtie(const var_tremtie& x, const std::string& s, const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_tremtie(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_tremtie(*this, s, p);
    }

    const char* getname() const {
      return "tremolo-style";
    } // docscat{special}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Determines whether or not tremolos are tied and if so, what kind "
             "of ties are used."
             "  Possible values are `none' for no ties, `tied' for solid ties, "
             "`dotted' for dotted ties, and `dashed' for dashed ties.";
    }
    bool isvalid(const fomusdata* fd) {
      return module_valid_string(mval, -1, -1, valid_istremtie, gettypedoc());
    }
  };

  class var_fill : public boolvar {
public:
    var_fill() : boolvar((fint) false) {
      assert(getid() == FILL_ID);
      initmodval();
    }
    var_fill(const fint val) : boolvar(val) {
      initmodval();
    }
    var_fill(const var_fill& x, const filepos& pos) : boolvar(x, pos) {
      initmodval();
    }
    var_fill(const var_fill& x, const numb& v, const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_fill(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_fill(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_fill(*this, v, p);
    }

    const char* getname() const {
      return "fill";
    } // docscat{voices}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "If set to true, specifies that the duration of a note extends to "
             "the onset of the next note in the same voice.  "
             "The duration parameter of the note is ignored and replaced with "
             "a new one.";
    }
    // const char* gettypedoc() const {return "||";}
    // bool isvalid() {return module_valid_intaux(val.getmodval(), 0,
    // module_incl, 2, module_incl, gettypedoc());}
  };

  class var_inittempotxt : public strvar {
public:
    var_inittempotxt() : strvar("* = #") {
      assert(getid() == INITTEMPOTXT_ID);
      initmodval();
    }
    var_inittempotxt(const std::string& str) : strvar(str) {
      initmodval();
    }
    var_inittempotxt(const var_inittempotxt& x, const filepos& pos)
        : strvar(x, pos) {
      initmodval();
    }
    var_inittempotxt(const var_inittempotxt& x, const std::string& s,
                     const filepos& pos)
        : strvar(x, s, pos) {
      initmodval();
    }

    varbase* getnewstr(fomusdata* fd, const char* s, const filepos& p) const {
      return new var_inittempotxt(*this, s, p);
    }
    varbase* getnewstr(fomusdata* fd, const std::string& s,
                       const filepos& p) const {
      return new var_inittempotxt(*this, s, p);
    }

    const char* getname() const {
      return "init-tempo-text";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Together with `init-tempo' specifies a tempo mark to insert at "
             "the beginning of the score.  "
             "FOMUS adds tempo marks with this string to every part in the "
             "score at time 0 (provided `init-tempo' is something other than "
             "0).  "
             "Use this if you want to quickly add a tempo mark to the "
             "beginning of the score without defining a detached `mark' event. "
             " "
             "See the documentation for `tempo' marks for an explanation of "
             "tempo text strings.";
    }
    bool isvalid(const fomusdata* fd) {
      return module_valid_string(mval, -1, -1, 0, gettypedoc());
    }
  };

  class var_inittempo : public numvar {
public:
    var_inittempo() : numvar((fint) 0) {
      assert(getid() == INITTEMPO_ID);
      initmodval();
    }
    var_inittempo(const fint val) : numvar(val) {
      initmodval();
    }
    var_inittempo(const var_inittempo& x, const filepos& pos) : numvar(x, pos) {
      initmodval();
    }
    var_inittempo(const var_inittempo& x, const numb& v, const filepos& pos)
        : numvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_inittempo(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_inittempo(*this, v, p);
    }
    varbase* getnew(const rat& v, const filepos& p) const {
      return new var_inittempo(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_inittempo(*this, v, p);
    }

    const char* getname() const {
      return "init-tempo";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locscore;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Together with `init-tempo-text' specifies a tempo mark to insert "
             "at the beginning of the score.  "
             "If this setting is set to anything other than 0, FOMUS adds "
             "tempo marks with this value to every part in the score at time "
             "0.  "
             "Use this if you want to quickly add a tempo mark to the "
             "beginning of the score without defining a detached `mark' event. "
             " "
             "See the documentation for `tempo' marks for an explanation of "
             "tempo values.";
    }
    const char* gettypedoc() const {
      return "real>=0";
    }
    bool isvalid(const fomusdata* fd) {
      assert(!mval.notyet());
      return module_valid_num(mval, numb((fint) 0), module_incl, numb((fint) 0),
                              module_nobound, 0, gettypedoc());
    }
  };

  class var_detach : public boolvar {
public:
    var_detach() : boolvar((fint) true) {
      assert(getid() == DETACH_ID);
      initmodval();
    }
    var_detach(const fint val) : boolvar(val) {
      initmodval();
    }
    var_detach(const var_detach& x, const filepos& pos) : boolvar(x, pos) {
      initmodval();
    }
    var_detach(const var_detach& x, const numb& v, const filepos& pos)
        : boolvar(x, v, pos) {
      initmodval();
    }

    // varbase* copy() const {return new var_detach(*this);}
    varbase* getnew(const fint v, const filepos& p) const {
      return new var_detach(*this, v, p);
    }
    varbase* getnew(const numb& v, const filepos& p) const {
      return new var_detach(*this, v, p);
    }

    const char* getname() const {
      return "detach";
    } // docscat{marks}
    module_setting_loc getloc() const {
      return module_locnote;
    }
    int getuselevel() const {
      return 2;
    }
    const char* getdescdoc() const {
      return "Some marks such as crescendo wedges often need to appear "
             "detached from notes events.  "
             "Setting `detach' to `yes' insures that these marks appear in the "
             "location given by the mark event's time attribute, "
             "regardless of whether or not any note events are at that "
             "location.  Setting `detach' to no forces all "
             "marks to be attached to note events.";
    }
    // const char* gettypedoc() const {return "||";}
    // bool isvalid() {return module_valid_intaux(val.getmodval(), 0,
    // module_incl, 2, module_incl, gettypedoc());}
  };

} // namespace fomus
#endif
