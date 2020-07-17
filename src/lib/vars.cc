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

#include "vars.h"
#include "data.h"
#include "instrs.h"
#include "parseins.h"
#include "modtypes.h"

namespace fomus {

  varsvect vars;
  varsmap varslookup;
  
  info_setwhere currsetwhere = info_default;

  void ordmapvartonums::numtostring_ins(printmap& to) const {
    to.clear();
    for (listelvect_constit i(ord.begin()); i != ord.end(); ++i) {
      std::string str(listel_getstring(*i));
#ifndef NDEBUG
      listelmap_constit j(el.find(str));
      assert(j != el.end());
      to.insert(printmap_val(numtorat(listel_getnumb(j->second)), str));
#else    
      to.insert(printmap_val(numtorat(listel_getnumb(el.find(str)->second)), str));    
#endif    
    }  
  }
  bool listofmods::isvalid(const enum module_type ty) {
    if (typestr.empty()) {
      std::ostringstream s;
      s << "id_string | (id_string id_string ...), id_string = ";
      bool fi = true;
      for (modsvect_constit i = mods.begin(); i != mods.end(); ++i) {
	if (i->gettype() == ty) {
	  if (fi) fi = false; else s << '|'; 
	  s << i->getsname();
	}
      }	
      typestr = s.str();
    }
    mval.reset();
    initmodval();
    if (!module_valid_listofstrings(mval, -1, -1, -1, -1, 0, typestr.c_str())) return false;
    for (module_value* i = mval.val.l.vals, *ie = mval.val.l.vals + mval.val.l.n; i < ie; ++i) {
      modsmap_constit m(modsbyname.find(i->val.s));
      if (m == modsbyname.end()) {
	CERR << '`' << i->val.s << "' is not a valid module";
	return false;
      } else if (m->second->gettype() != ty) {
	CERR << "module `" << i->val.s << "' is not of type `" << modtypetostr(ty) << '\'';
	return false;
      }
    }
    return true;
  }
  
  void percinstrs_var::initmodval() {
    assert(mval.notyet());
    mval.type = module_list;
    mval.val.l.vals = newmodvals(mval.val.l.n = map.size()); 
    for_each2(map.begin(), map.end(), mval.val.l.vals, boost::lambda::bind(&percinstr_str::getmodval, boost::lambda::bind(&boost::shared_ptr<percinstr_str>::get,
															  boost::lambda::bind(&globpercsvarvect_val::second,
																	      boost::lambda::_1)),
									   boost::lambda::_2, false));
  }

  varbase* percinstrs_var::getnewprepend(const globpercsvarvect& val, const filepos& p) {
    globpercsvarvect z(map);
    for (globpercsvarvect_constit i(val.begin()); i != val.end(); ++i) {
      z.erase(i->second->getid());
      z.insert(*i);
    }
    return getnew(z, p);
  }

  std::string percinstrs_var::getvalstr(const fomusdata* fd, const char* st) const {
    std::ostringstream o;
    o << '(';
    if (!map.empty()) {
      map.begin()->second->print(o, fd, false);
      for (globpercsvarvect_constit i(next(map.begin())); i != map.end(); ++i) {
	o << ' ';
	i->second->print(o, fd, false);
      }
    }
    o << ')';
    return o.str();
  }

  void instrs_var::initmodval() {
    assert(mval.notyet());
    mval.type = module_list;
    mval.val.l.vals = newmodvals(mval.val.l.n = map.size()); 
    for_each2(map.begin(), map.end(), mval.val.l.vals, boost::lambda::bind(&instr_str::getmodval,
									   boost::lambda::bind(&boost::shared_ptr<instr_str>::get,
											       boost::lambda::bind(&globinstsvarvect_val::second, boost::lambda::_1)),
									   boost::lambda::_2, false));
  }
  
  varbase* instrs_var::getnewprepend(const globinstsvarvect& val, const filepos& p) {
    globinstsvarvect z(map);
    for (globinstsvarvect_constit i(val.begin()); i != val.end(); ++i) {
      z.erase(i->second->getid());
      z.insert(*i);
    }
    return getnew(z, p);
  }
  
  std::string instrs_var::getvalstr(const fomusdata* fd, const char* st) const {
    std::ostringstream o;
    o << '(';
    if (!map.empty()) {
      map.begin()->second->print(o, fd, false);
      for (globinstsvarvect_constit i(next(map.begin())); i != map.end(); ++i) {
	o << ' ';
	i->second->print(o, fd, false);
      }
    }
    o << ')';
    return o.str();
  }
  
  var_notesymbols::var_notesymbols():ordmapvartonums() {
    assert(getid() == NOTESYMBOLS_ID);
    el.insert(listelmap_val("c", (fint)0));
    ord.push_back("c");
    el.insert(listelmap_val("C", (fint)0));
    ord.push_back("C");
    el.insert(listelmap_val("d", (fint)2));
    ord.push_back("d");
    el.insert(listelmap_val("D", (fint)2));
    ord.push_back("D");
    el.insert(listelmap_val("e", (fint)4));
    ord.push_back("e");
    el.insert(listelmap_val("E", (fint)4));
    ord.push_back("E");
    el.insert(listelmap_val("f", (fint)5));
    ord.push_back("f");
    el.insert(listelmap_val("F", (fint)5));
    ord.push_back("F");
    el.insert(listelmap_val("g", (fint)7));
    ord.push_back("g");
    el.insert(listelmap_val("G", (fint)7));
    ord.push_back("G");
    el.insert(listelmap_val("a", (fint)9));
    ord.push_back("a");
    el.insert(listelmap_val("A", (fint)9));
    ord.push_back("A");
    el.insert(listelmap_val("b", (fint)11));
    ord.push_back("b");
    el.insert(listelmap_val("B", (fint)11));
    ord.push_back("B");
    initmodval();
  }
  var_noteaccs::var_noteaccs():ordmapvartonums() {
    assert(getid() == NOTEACCS_ID);
    el.insert(listelmap_val("-", (fint)-1));
    ord.push_back("-");
    el.insert(listelmap_val("b", (fint)-1));
    ord.push_back("b");
    el.insert(listelmap_val("f", (fint)-1));
    ord.push_back("f");
    el.insert(listelmap_val("F", (fint)-1));
    ord.push_back("F");
    el.insert(listelmap_val("+", (fint)1));
    ord.push_back("+");
    el.insert(listelmap_val("#", (fint)1));
    ord.push_back("#");
    el.insert(listelmap_val("s", (fint)1));
    ord.push_back("s");
    el.insert(listelmap_val("S", (fint)1));
    ord.push_back("S");
    el.insert(listelmap_val("n", (fint)0));
    ord.push_back("n");
    el.insert(listelmap_val("N", (fint)0));
    ord.push_back("N");
    el.insert(listelmap_val("_", (fint)0));
    ord.push_back("_");
    initmodval();
  }
  var_notemicrotones::var_notemicrotones():ordmapvartonums() {
    assert(getid() == NOTEMICROTONES_ID);
    el.insert(listelmap_val("-", rat(-1, 2)));
    ord.push_back("-");
    el.insert(listelmap_val("b", rat(-1, 2)));
    ord.push_back("b");
    el.insert(listelmap_val("f", rat(-1, 2)));
    ord.push_back("f");
    el.insert(listelmap_val("F", rat(-1, 2)));
    ord.push_back("F");
    el.insert(listelmap_val("+", rat(1, 2)));
    ord.push_back("+");
    el.insert(listelmap_val("#", rat(1, 2)));
    ord.push_back("#");
    el.insert(listelmap_val("s", rat(1, 2)));
    ord.push_back("s");
    el.insert(listelmap_val("S", rat(1, 2)));
    ord.push_back("S");
    el.insert(listelmap_val("n", (fint)0));
    ord.push_back("n");
    el.insert(listelmap_val("N", (fint)0));
    ord.push_back("N");
    el.insert(listelmap_val("_", (fint)0));
    ord.push_back("_");
    initmodval();
  }
  var_noteoctaves::var_noteoctaves():ordmapvartonums() {
    assert(getid() == NOTEOCTAVES_ID);
    el.insert(listelmap_val("0", (fint)60 - 12 * 4));
    ord.push_back("0");
    el.insert(listelmap_val("1", (fint)60 - 12 * 3));
    ord.push_back("1");
    el.insert(listelmap_val("2", (fint)60 - 12 * 2));
    ord.push_back("2");
    el.insert(listelmap_val("3", (fint)60 - 12 * 1));
    ord.push_back("3");
    el.insert(listelmap_val("4", (fint)60 + 12 * 0));
    ord.push_back("4");
    el.insert(listelmap_val("5", (fint)60 + 12 * 1));
    ord.push_back("5");
    el.insert(listelmap_val("6", (fint)60 + 12 * 2));
    ord.push_back("6");
    el.insert(listelmap_val("7", (fint)60 + 12 * 3));
    ord.push_back("7");
    el.insert(listelmap_val("8", (fint)60 + 12 * 4));
    ord.push_back("8");
    el.insert(listelmap_val(",,,", (fint)60 - 12 * 4));
    ord.push_back(",,,");
    el.insert(listelmap_val(",,", (fint)60 - 12 * 3));
    ord.push_back(",,");
    el.insert(listelmap_val(",", (fint)60 - 12 * 2));
    ord.push_back(",");
    el.insert(listelmap_val(".", (fint)60 - 12 * 1));
    ord.push_back(".");
    el.insert(listelmap_val("'", (fint)60 + 12 * 0));
    ord.push_back("'");
    el.insert(listelmap_val("''", (fint)60 + 12 * 1));
    ord.push_back("''");
    el.insert(listelmap_val("'''", (fint)60 + 12 * 2));
    ord.push_back("'''");
    el.insert(listelmap_val("''''", (fint)60 + 12 * 3));
    ord.push_back("''''");
    el.insert(listelmap_val("'''''", (fint)60 + 12 * 4));
    ord.push_back("'''''");
    initmodval();
  }

  var_dursymbols::var_dursymbols():ordmapvartonums() {
    assert(getid() == DURSYMS_ID);
    el.insert(listelmap_val("m", (fint)32));
    ord.push_back("m");
    el.insert(listelmap_val("l", (fint)16));
    ord.push_back("l");
    el.insert(listelmap_val("b", (fint)8));
    ord.push_back("b");
    el.insert(listelmap_val("w", (fint)4));
    ord.push_back("w");
    el.insert(listelmap_val("h", (fint)2));
    ord.push_back("h");
    el.insert(listelmap_val("q", (fint)1));
    ord.push_back("q");
    el.insert(listelmap_val("e", rat(1, 2)));
    ord.push_back("e");
    el.insert(listelmap_val("s", rat(1, 4)));
    ord.push_back("s");
    el.insert(listelmap_val("t", rat(1, 8)));
    ord.push_back("t");
    el.insert(listelmap_val("x", rat(1, 16)));
    ord.push_back("x");
    initmodval();
  }
  var_durdot::var_durdot():ordmapvartonums() {
    assert(getid() == DURDOTS_ID);
    el.insert(listelmap_val(".", rat(3, 2)));
    ord.push_back(".");
    el.insert(listelmap_val(":", rat(7, 4)));
    ord.push_back(":");
    initmodval();
  }
  var_durtie::var_durtie():listvarofstrings() {
    assert(getid() == DURTIES_ID);
    el.push_back("+");
    el.push_back("_");
    el.push_back("~");
    initmodval();
  }
  var_tupsyms::var_tupsyms():ordmapvartonums() {
    assert(getid() == TUPSYMS_ID);
    el.insert(listelmap_val("d", rat(3, 2)));
    ord.push_back("d");
    el.insert(listelmap_val("q", rat(4, 5)));
    ord.push_back("q");
    el.insert(listelmap_val("t", rat(2, 3)));
    ord.push_back("t");
    el.insert(listelmap_val("c", rat(2, 3)));
    ord.push_back("c");
    initmodval();
  }

  void var_keysigs::redo(const fomusdata* fd) {
    assert(varisvalid());
    el.clear();
    for (boost::ptr_map<const std::string, boost::ptr_vector<userkeysigent> >::const_iterator i(user.begin()); i != user.end(); ++i) {
      std::auto_ptr<listelvect> x(new listelvect);
      redosig(*i->second, *x, fd ? fd->getnotepr() : note_print, fd ? fd->getaccpr() : acc_print, fd ? fd->getmicpr() : mic_print, fd ? fd->getoctpr() : oct_print);
      el.insert(listelmap::value_type(i->first, x.release()));
    }
    mval.reset();
    initmodval();
  }

  bool var_keysigs::keysig_isinvalid(const fomusdata* fd, int& n, const char* &s, const struct module_value& val) {
    assert(varisvalid());
    if (++n % 2 == 0) {
      s = val.val.s; 
      return !module_valid_string(val, 1, -1, 0, gettypedoc());
    } else {
      if (!parsekeysigmap(fd, s, val)) {
	CERR << "expected value of type `" << gettypedoc() << '\'';
	return true;
      }
      return false;
    }  
  }

  bool var_keysigs::module_valid_keysigs(const fomusdata* fd) {
    assert(varisvalid());
    if (!module_valid_listaux(mval, -1, -1, gettypedoc())) return false; 
    int n = -1;
    const char* s;
    if (hassome(mval.val.l.vals, mval.val.l.vals + mval.val.l.n,
		boost::lambda::bind(&var_keysigs::keysig_isinvalid, this, fd, boost::lambda::var(n), boost::lambda::var(s),
				    boost::lambda::_1))) return false;
    if (n % 2 == 0) {
      CERR << "missing map value";
      return false;    
    }
    return true;
  }

  void var_keysigs::redo(const symtabs& ta) {
    assert(varisvalid());
    el.clear();
    DBG("allsigs size = " << allsigs.size() << std::endl);
    for (boost::ptr_map<const std::string, boost::ptr_vector<userkeysigent> >::const_iterator i(user.begin()); i != user.end(); ++i) {
      listelvect* x;
      el.insert(listelmap::value_type(i->first, x = new listelvect));
      redosig(*i->second, *x, ta.notepr, ta.accpr, ta.micpr, ta.octpr);
    }
    mval.reset();
    initmodval();
  }

  // ea is either 7 accs, or note name followed by any number of 
  var_keysigs::var_keysigs():mapvaroflistofstrings() {
    listelvect v;
    el.insert(listelmap::value_type("cmaj", new listelvect(v)));
    el.insert(listelmap::value_type("amin",  new listelvect(v)));
    v.push_back("f+");
    el.insert(listelmap::value_type("gmaj", new listelvect(v)));
    el.insert(listelmap::value_type("emin", new listelvect(v)));
    v.push_back("c+");
    el.insert(listelmap::value_type("dmaj", new listelvect(v)));
    el.insert(listelmap::value_type("bmin", new listelvect(v)));
    v.push_back("g+");    
    el.insert(listelmap::value_type("amaj", new listelvect(v)));
    el.insert(listelmap::value_type("f+min", new listelvect(v)));
    v.push_back("d+");        
    el.insert(listelmap::value_type("emaj", new listelvect(v)));
    el.insert(listelmap::value_type("c+min", new listelvect(v)));
    v.push_back("a+");            
    el.insert(listelmap::value_type("bmaj", new listelvect(v)));
    el.insert(listelmap::value_type("g+min", new listelvect(v)));
    v.push_back("e+");                
    el.insert(listelmap::value_type("f+maj", new listelvect(v)));
    el.insert(listelmap::value_type("d+min", new listelvect(v)));
    v.push_back("b+");                
    el.insert(listelmap::value_type("c+maj", new listelvect(v)));
    el.insert(listelmap::value_type("a+min", new listelvect(v)));
    v.clear(); v.push_back("b-");                
    el.insert(listelmap::value_type("fmaj", new listelvect(v)));
    el.insert(listelmap::value_type("dmin", new listelvect(v)));
    v.push_back("e-");
    el.insert(listelmap::value_type("b-maj", new listelvect(v)));
    el.insert(listelmap::value_type("gmin", new listelvect(v)));
    v.push_back("a-");    
    el.insert(listelmap::value_type("e-maj", new listelvect(v)));
    el.insert(listelmap::value_type("cmin", new listelvect(v)));
    v.push_back("d-");        
    el.insert(listelmap::value_type("a-maj", new listelvect(v)));
    el.insert(listelmap::value_type("fmin", new listelvect(v)));
    v.push_back("g-");            
    el.insert(listelmap::value_type("d-maj", new listelvect(v)));
    el.insert(listelmap::value_type("b-min", new listelvect(v)));
    v.push_back("c-");                
    el.insert(listelmap::value_type("g-maj", new listelvect(v)));
    el.insert(listelmap::value_type("e-min", new listelvect(v)));
    v.push_back("f-");                    
    el.insert(listelmap::value_type("c-maj", new listelvect(v)));
    el.insert(listelmap::value_type("a-min", new listelvect(v)));
    assert(getid() == KEYSIG_ID); initmodval();
  }

  void redosig(const boost::ptr_vector<userkeysigent>& user, listelvect& el, const printmap& notepr, const printmap& accpr, const printmap& micpr, const printmap& octpr) { // yes, sig is copied!
    for (boost::ptr_vector<userkeysigent>::const_iterator i(user.begin()); i != user.end(); ++i) {
      assert(i->isvalid());
      printmap_constit n(notepr.find(i->n));
      printmap_constit a(accpr.find(i->a));
      printmap_constit m(micpr.find(i->m));
      printmap_constit o(octpr.find(i->o));
      el.push_back((n == notepr.end() ? std::string() : n->second)
		   + (a == accpr.end() ? std::string() : a->second)
		   + (m == micpr.end() ? std::string() : m->second)
		   + (o == octpr.end() ? std::string() : o->second));      
    }
  }

  // must be a note symbol for keysigs
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(notematchhasoct,(8,(((const boostspirit::symbols<numb>&),notesym),((const boostspirit::symbols<numb>&),accsym),
						      ((const boostspirit::symbols<numb>&),micsym),((const boostspirit::symbols<numb>&),octsym),
						      ((numb&),nov),((numb&),acv),((numb&),miv),((numb&),ocv))),-,
				  (notesym[setnumval<numb>(nov)] >> !accsym[setnumval<numb>(acv)] >> !micsym[setnumval<numb>(miv)] >> !octsym[setnumval<numb>(ocv)]
				   >> boostspirit::end_p)
				  | boostspirit::eps_p[setconstval<numb>(nov, module_none)])
  
  inline const char* endof(const char* x) {while (*x) ++x; return x;}
  bool doparsekeysig(const fomusdata* fd, const module_value& lst, std::vector<std::pair<rat, rat> >& sig, boost::ptr_vector<userkeysigent>& user) {
    assert(sig.size() == 75);
    const boostspirit::symbols<numb>& mp(fd ? fd->getnotepa() : note_parse);
    const boostspirit::symbols<numb>& ap(fd ? fd->getaccpa() : acc_parse);
    const boostspirit::symbols<numb>& ip(fd ? fd->getmicpa() : mic_parse);
    const boostspirit::symbols<numb>& op(fd ? fd->getoctpa() : oct_parse);
    if (lst.type != module_list) return false;
    for (const module_value* i0 = lst.val.l.vals, *ie = lst.val.l.vals + lst.val.l.n; i0 < ie; ++i0) {
      if (i0->type != module_string) return false;
      numb nov(module_none), acv(module_none), miv(module_none), ocv(module_none);
      parserule rl(notematchhasoct(mp, ap, ip, op, nov, acv, miv, ocv));
      parse_it p(i0->val.s, endof(i0->val.s));
      parse(p, parse_it(), rl);
      rat m(nov.isnull() ? std::numeric_limits<fint>::min() + 1 : numtorat(nov));
      rat a(acv.isnull() ? std::numeric_limits<fint>::min() + 1 : numtorat(acv));
      rat i(miv.isnull() ? std::numeric_limits<fint>::min() + 1 : numtorat(miv));
      rat o(ocv.isnull() ? std::numeric_limits<fint>::min() + 1 : numtorat(ocv));
      user.push_back(new userkeysigent(m, a, i, o));
      if (m.numerator() == std::numeric_limits<fint>::min() + 1) return false; 
      if (a.numerator() == std::numeric_limits<fint>::min() + 1) a = 0; // must have at least these two things
      if (i.numerator() == std::numeric_limits<fint>::min() + 1) i = 0; // must have at least these two things
      if (o.numerator() != std::numeric_limits<fint>::min() + 1) { // got an octave
	rat x0(o + m);
	if (x0.denominator() != 1 || isblack(x0.numerator())) continue; // better to skip it 
	assert(todiatonic(x0.numerator()) >= 0 && todiatonic(x0.numerator()) < 75);
	sig[todiatonic(x0.numerator())] = std::pair<rat, rat>(a, i);
      } else {
	if (m.denominator() != 1 || isblack(m.numerator())) continue; // better to skip it 
	for (int x = todiatonic(m.numerator()) % 7; x < 75; x += 7) sig[x] = std::pair<rat, rat>(a, i);
      }
    }
    return true;
  }

  template<typename T>
  struct setnummodval {
    fomus_rat &val;
    
    setnummodval(fomus_rat &val):val(val) {}
    void operator()(const T& n) const {
      assert(n.israt());
      val = numtofrat(n);
    }
  };
  
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(relnotematch,(8,(((const boostspirit::symbols<numb>&),notesym),((const boostspirit::symbols<numb>&),accsym),
						   ((const boostspirit::symbols<numb>&),micsym),((const boostspirit::symbols<numb>&),octsym),
						   ((numb&),prval),((bool&),gup),((numb&),val),((module_noteparts&),nps))),-,
				  (notesym[setnumval<numb>(val)][setnummodval<numb>(nps.note)]
				   >> !accsym[addnumval(val)][setnummodval<numb>(nps.acc1)]
				   >> !micsym[addnumval(val)][setnummodval<numb>(nps.acc2)]
				   >> (octsym[addnumval(val)][setnummodval<numb>(nps.oct)] | boostspirit::eps_p[nearestpitch(val, prval, gup)])
				   >> boostspirit::end_p)
				  | boostspirit::eps_p[setconstval<numb>(val, module_none)])
  numb doparsenote(fomusdata *fd, const char* no, const bool noneonerr, module_noteparts* xtra) {
    assert(fd); // doparsenote should only get called when user is entering a string and the setting is a module_notesym type, or when a pitch is entered
    numb val0(module_none);
    assert(val0.type() == module_none);
    assert(val0.isnull());
    module_noteparts xtra0;
    parserule rl(relnotematch(fd->getnotepa(), fd->getaccpa(), fd->getmicpa(), fd->getoctpa(), fd->prevnote, fd->prevnotegup, val0, xtra ? *xtra : xtra0));
    parse_it p(no, endof(no));
    parse(p, parse_it(), rl);
    if (val0.isnull()) {
      if (!noneonerr) {
	CERR << "error parsing note";
	fd->getpos().printerr();
	throw errbase();
      }
    } else fd->prevnote = val0;
    DBG("parsed note " << no << " = " << val0);
    return val0;
  }

  struct addnumvalac {
    numb &val;
    addnumvalac(numb &val):val(val) {}
    void operator()(const numb& n) const {
      if (val.isnull()) {
	val = n;
      } else
	val = val + n.modval();
    }
  };

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(accmatch,(4,(((const boostspirit::symbols<numb>&),accsym),
					       ((const boostspirit::symbols<numb>&),micsym),
					       ((numb&),val),((module_noteparts&),nps))),-,
				  (!accsym[addnumvalac(val)][setnummodval<numb>(nps.acc1)]
				   >> !micsym[addnumvalac(val)][setnummodval<numb>(nps.acc2)]
				   >> boostspirit::end_p)
				  | boostspirit::eps_p[setconstval<numb>(val, module_none)])
  numb doparseacc(const fomusdata *fd, const char* no, const bool noneonerr, module_noteparts* xtra) {
    assert(fd); // doparsenote should only get called when user is entering a string and the setting is a module_notesym type, or when a pitch is entered
    numb val0(module_none);
    assert(val0.type() == module_none);
    assert(val0.isnull());
    module_noteparts xtra0;
    parserule rl(accmatch((fd ? fd->getaccpa() : acc_parse), (fd ? fd->getmicpa() : mic_parse), val0, xtra ? *xtra : xtra0));
    parse_it p(no, endof(no));
    parse(p, parse_it(), rl);
    if (val0.isnull() && !noneonerr && fd) {
      CERR << "error parsing accidental";
      fd->getpos().printerr();
      throw errbase();
    }
    DBG("parsed accidental " << no << " = " << val0);
    return val0;
  }
  
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(durmatchaux,(5,(((const boostspirit::symbols<numb>&),durdot),((const boostspirit::symbols<numb>&),dursyms),
						  ((const boostspirit::symbols<numb>&),durtie),((const boostspirit::symbols<numb>&),tupsyms),
						  ((numb&),val))),-,
				  dursyms[setnumval<numb>(val)] >> !durdot[multnumval(val)] >> *tupsyms[multnumval(val)])
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(durmatch,(6,(((const boostspirit::symbols<numb>&),durdot),((const boostspirit::symbols<numb>&),dursyms),
					       ((const boostspirit::symbols<numb>&),durtie),((const boostspirit::symbols<numb>&),tupsyms),
					       ((numb&),val),((numb&),val2))),-,
				  (durmatchaux(durdot, dursyms, durtie, tupsyms, val) >>
				  *(durtie >> durmatchaux(durdot, dursyms, durtie, tupsyms, val2)[addconstnumval(val, val2)]) >> boostspirit::end_p)
				  | boostspirit::eps_p[setconstval<numb>(val, module_none)])
  
  numb doparsedur(fomusdata *fd, const char* no, const bool noneonerr) {
    numb val0(module_none), vals;
    parserule rl(durmatch(fd->getdurdot(), fd->getdursyms(), fd->getdurtie(), fd->gettupsyms(), val0, vals));
    parse_it p(no, endof(no));
    parse(p, parse_it(), rl);
    if (val0.isnull() && !noneonerr && fd) {
      CERR << "error parsing duration";
      fd->getpos().printerr();
      throw errbase();
    }
    return val0;
  }
  
  accrulestype accrules;
  std::map<std::string, module_barlines, isiless> barlines;
  std::map<std::string, enum_sulstyles, isiless> sulstyles;
  std::set<std::string, isiless> pedstyles;
  std::map<std::string, enum_tiestyles, isiless> tremties;
  void initaccrules() {
    accrules.insert(accrulestype::value_type("measure", accrule_meas));
    accrules.insert(accrulestype::value_type("note", accrule_note));
    accrules.insert(accrulestype::value_type("note-naturals", accrule_notenats));
    barlines.insert(std::map<std::string, module_barlines>::value_type("", barline_normal));
    barlines.insert(std::map<std::string, module_barlines>::value_type("!", barline_normal));
    barlines.insert(std::map<std::string, module_barlines>::value_type("!!", barline_double));
    barlines.insert(std::map<std::string, module_barlines>::value_type("|!", barline_initial));
    barlines.insert(std::map<std::string, module_barlines>::value_type("!|", barline_final));
    barlines.insert(std::map<std::string, module_barlines>::value_type("!|!", barline_initfinal));
    barlines.insert(std::map<std::string, module_barlines>::value_type(":", barline_dotted));
    barlines.insert(std::map<std::string, module_barlines>::value_type(";", barline_dashed));
    barlines.insert(std::map<std::string, module_barlines>::value_type("|:", barline_repeatright));
    barlines.insert(std::map<std::string, module_barlines>::value_type(":|:", barline_repeatleftright));
    barlines.insert(std::map<std::string, module_barlines>::value_type(":|", barline_repeatleft));
    sulstyles.insert(std::map<std::string, enum_sulstyles>::value_type("sulletter", ss_sulletter));
    sulstyles.insert(std::map<std::string, enum_sulstyles>::value_type("letter", ss_letter));
    sulstyles.insert(std::map<std::string, enum_sulstyles>::value_type("sulroman", ss_sulroman));
    sulstyles.insert(std::map<std::string, enum_sulstyles>::value_type("roman", ss_roman));
    pedstyles.insert("text");
    pedstyles.insert("bracket");
    tremties.insert(std::map<std::string, enum_tiestyles, isiless>::value_type("none", tiestyle_none));
    tremties.insert(std::map<std::string, enum_tiestyles, isiless>::value_type("tied", tiestyle_tied));
    tremties.insert(std::map<std::string, enum_tiestyles, isiless>::value_type("dashed", tiestyle_dashed));
    tremties.insert(std::map<std::string, enum_tiestyles, isiless>::value_type("dotted", tiestyle_dotted));
  }

  inline void thrownewmod(const char* name, const std::string& modname) {
    CERR << "invalid default value for setting `" << name << "' in module `" << modname << '\'' << std::endl;
    throw errbase();
  }

  void modvaltolistelvect(const modbase& mod, const module_setting& set, listelvect& el) {
    for (struct module_value *i = set.val.val.l.vals, *ie = set.val.val.l.vals + set.val.val.l.n; i != ie; ++i) {
      switch (i->type) {
      case module_bool:
      case module_int:
      case module_float:
      case module_notesym:
      case module_rat:
	el.push_back(numb(*i)); 
	break;
      case module_string:
	el.push_back(std::string(i->val.s));
	break;
      case module_list:
	{
	  listelshptr x(new listelvect);
	  for (struct module_value *j = i->val.l.vals, *je = i->val.l.vals + i->val.l.n; j < je; ++j) {
	    switch (j->type) {
	    case module_bool:
	    case module_int:
	    case module_float:
	    case module_notesym:
	    case module_rat:
	      x->push_back(numb(*j)); 
	      break;
	    case module_string:
	      x->push_back(std::string(j->val.s));
	      break;
	    default:
	      thrownewmod(set.name, mod.getsname());
	    }
	  }
	  el.push_back(x);
	}
	break;
#ifndef NDEBUG
      case module_list_nums:
      case module_list_strings:
      case module_list_numlists:
      case module_list_stringlists:
      case module_stringnum:
      case module_symmap_nums:
      case module_symmap_strings:
      case module_symmap_numlists:
      case module_symmap_stringlists:
      case module_none:
      case module_number:
      case module_special:
#else      
      default:
#endif
	thrownewmod(set.name, mod.getsname());
      }
    }
  }

  void modvaltolistelmap(const modbase& mod, const module_setting& set, listelmap& el) {
    for (struct module_value *i = set.val.val.l.vals, *ie = set.val.val.l.vals + set.val.val.l.n; i < ie; i += 2) {
      if (i->type != module_string) thrownewmod(set.name, mod.getsname());
      struct module_value *j = i + 1;
      if (j >= ie) thrownewmod(set.name, mod.getsname());
      switch (j->type) {
      case module_bool:
      case module_int:
      case module_float:
      case module_notesym:
	el.insert(listelmap_val(i->val.s, numb(*j)));
	break;
      case module_rat:
	el.insert(listelmap_val(i->val.s, numb(rat(j->val.r.num, j->val.r.den))));
	break;
      case module_string:
	el.insert(listelmap_val(i->val.s, std::string(j->val.s)));
	break;
#ifndef NDEBUG
      case module_list:
      case module_list_nums:
      case module_list_strings:
      case module_list_numlists:
      case module_list_stringlists:
      case module_stringnum:
      case module_symmap_nums:
      case module_symmap_strings:
      case module_symmap_numlists:
      case module_symmap_stringlists:
      case module_none:
      case module_number:
      case module_special:
#else      
      default:
#endif
	thrownewmod(set.name, mod.getsname());
      }
    }
  }

  void newmodstrvar(const modbase& mod, const module_setting& set) {
    if (set.val.type != module_string) thrownewmod(set.name, mod.getsname());
    varbase* v;
    vars.push_back(boost::shared_ptr<varbase>(v = new var_modstr(mod, set)));
    varslookup.insert(varsmap_val(v->getname(), v));
  }

  void newmodnumvar(const modbase& mod, const module_setting& set) {
    varbase* v;
    if (set.val.type == module_int) {
      vars.push_back(boost::shared_ptr<varbase>(v = new var_modnum(set.val.val.i, mod, set)));
    } else if (set.val.type == module_float) {
      vars.push_back(boost::shared_ptr<varbase>(v = new var_modnum(set.val.val.f, mod, set)));
    } else if (set.val.type == module_rat) {
      vars.push_back(boost::shared_ptr<varbase>(v = new var_modnum(rat(set.val.val.r.num, set.val.val.r.den), mod, set)));
    } else {
      thrownewmod(set.name, mod.getsname());
    }
    varslookup.insert(varsmap_val(v->getname(), v));
  }
  void newmodboolvar(const modbase& mod, const module_setting& set) {
    varbase* v;
    if (set.val.type == module_int) {
      vars.push_back(boost::shared_ptr<varbase>(v = new var_modbool(set.val.val.i, mod, set)));
    } else if (set.val.type == module_float) {
      vars.push_back(boost::shared_ptr<varbase>(v = new var_modbool(set.val.val.f, mod, set)));
    } else if (set.val.type == module_rat) {
      vars.push_back(boost::shared_ptr<varbase>(v = new var_modbool(rat(set.val.val.r.num, set.val.val.r.den), mod, set)));
    } else {
      thrownewmod(set.name, mod.getsname());
    }
    varslookup.insert(varsmap_val(v->getname(), v));
  };
  void newmodnotevar(const modbase& mod, const module_setting& set) {
    varbase* v;
    if (set.val.type == module_int) {
      vars.push_back(boost::shared_ptr<varbase>(v = new var_modnote(set.val.val.i, mod, set)));
    } else if (set.val.type == module_float) {
      vars.push_back(boost::shared_ptr<varbase>(v = new var_modnote(set.val.val.f, mod, set)));
    } else if (set.val.type == module_rat) {
      vars.push_back(boost::shared_ptr<varbase>(v = new var_modnote(rat(set.val.val.r.num, set.val.val.r.den), mod, set)));
    } else {
      thrownewmod(set.name, mod.getsname());
    }
    varslookup.insert(varsmap_val(v->getname(), v));
  };

  void newmodlistofnumsvar(const modbase& mod, const module_setting& set) {
    if (set.val.type != module_list) thrownewmod(set.name, mod.getsname());
    varbase* v;
    vars.push_back(boost::shared_ptr<varbase>(v = new var_modlistofnums(mod, set)));
    varslookup.insert(varsmap_val(v->getname(), v));
  }
  void newmodlistofstringsvar(const modbase& mod, const module_setting& set) {
    if (set.val.type != module_list) thrownewmod(set.name, mod.getsname());
    varbase* v;
    vars.push_back(boost::shared_ptr<varbase>(v = new var_modlistofstrings(mod, set)));
    varslookup.insert(varsmap_val(v->getname(), v));
  }

  void newmodmaptonumsvar(const modbase& mod, const module_setting& set) {
    if (set.val.type != module_list) thrownewmod(set.name, mod.getsname()); 
    varbase* v;
    vars.push_back(boost::shared_ptr<varbase>(v = new var_modmaptonums(mod, set)));
    varslookup.insert(varsmap_val(v->getname(), v));
  }
  void newmodmaptostringsvar(const modbase& mod, const module_setting& set) {
    if (set.val.type != module_list) thrownewmod(set.name, mod.getsname()); 
    varbase* v;
    vars.push_back(boost::shared_ptr<varbase>(v = new var_modmaptostrings(mod, set)));
    varslookup.insert(varsmap_val(v->getname(), v));
  }
  void newmodlistofnumlistsvar(const modbase& mod, const module_setting& set) {
    if (set.val.type != module_list) thrownewmod(set.name, mod.getsname()); 
    varbase* v;
    vars.push_back(boost::shared_ptr<varbase>(v = new var_modlistofnumlists(mod, set)));
    varslookup.insert(varsmap_val(v->getname(), v));
  }
  void newmodlistofstringlistsvar(const modbase& mod, const module_setting& set) {
    if (set.val.type != module_list) thrownewmod(set.name, mod.getsname()); 
    varbase* v;
    vars.push_back(boost::shared_ptr<varbase>(v = new var_modlistofstringlists(mod, set)));
    varslookup.insert(varsmap_val(v->getname(), v));
  }
  void newmodmaptonumlistsvar(const modbase& mod, const module_setting& set) {
    if (set.val.type != module_list) thrownewmod(set.name, mod.getsname()); 
    varbase* v;
    vars.push_back(boost::shared_ptr<varbase>(v = new var_modmaptonumlists(mod, set)));
    varslookup.insert(varsmap_val(v->getname(), v));
  }
  void newmodmaptostringlistsvar(const modbase& mod, const module_setting& set) {
    if (set.val.type != module_list) thrownewmod(set.name, mod.getsname()); 
    varbase* v;
    vars.push_back(boost::shared_ptr<varbase>(v = new var_modmaptostringlists(mod, set)));
    varslookup.insert(varsmap_val(v->getname(), v));
  }

  void addmodvar(const modbase& mod, const module_setting& s) {
    if (s.name == NULL) {
      CERR << "missing setting name in module `" << mod.getsname() << '\'' << std::endl;
      throw errbase();
    }
    switch (s.type) {
    case module_bool:
      newmodboolvar(mod, s);
      break;
    case module_int:
    case module_float:
    case module_rat:
    case module_number:
      newmodnumvar(mod, s);
      break;
    case module_notesym:
      newmodnotevar(mod, s);
      break;
    case module_string:
      newmodstrvar(mod, s);
      break;
    case module_list_nums:
      newmodlistofnumsvar(mod, s);
      break;
    case module_list_strings:
      newmodlistofstringsvar(mod, s);
      break;
    case module_symmap_nums:
      newmodmaptonumsvar(mod, s);
      break;
    case module_symmap_strings:
      newmodmaptostringsvar(mod, s);
      break;
    case module_list_numlists:
      newmodlistofnumlistsvar(mod, s);
      break;
    case module_list_stringlists:
      newmodlistofstringlistsvar(mod, s);
      break;
    case module_symmap_numlists:
      newmodmaptonumlistsvar(mod, s);
      break;
    case module_symmap_stringlists:
      newmodmaptostringlistsvar(mod, s);
      break;
#ifndef NDEBUG    
    case module_none:
    case module_list:
    case module_stringnum:
    case module_special:
#else
    default:
#endif    
      CERR << "bad type for setting `" << s.name << "' in module `" << mod.getsname() << '\'' << std::endl;
    }
  }

  class listel_tomodval:public boost::static_visitor<void> {
  private:
    module_value& val;
  public:
    listel_tomodval(module_value& val):val(val) {}
    void operator()(const numb& x) const {val = x.modval();}
    void operator()(const std::string& x) const {val.type = module_string; val.val.s = x.c_str();}
    void operator()(const boost::shared_ptr<listelvect>& x) const;
  };
  void listel_tomodval::operator()(const boost::shared_ptr<listelvect>& x) const {
    val.type = module_list;
    struct module_list& l(val.val.l);
    l.vals = newmodvals(l.n = x->size());
    struct module_value* v = l.vals;
    for (listelvect_constit i(x->begin()); i != x->end(); ++i, ++v) apply_visitor(listel_tomodval(*v), *i);
  }
  
  void listvarbase::initmodval() {
    assert(mval.notyet());
    mval.type = module_list;
    struct module_list& l(mval.val.l);
    l.vals = newmodvals(l.n = el.size());
    struct module_value* v = l.vals;
    for (listelvect_constit i(el.begin()); i != el.end(); ++i, ++v) apply_visitor(listel_tomodval(*v), *i);
  }

  void mapvarbase::initmodval() {
    assert(mval.notyet());
    mval.type = module_list;
    struct module_list& l(mval.val.l);
    l.vals = newmodvals(l.n = el.size() * 2);
    struct module_value* v = l.vals;
    for (listelmap_constit i(el.begin()); i != el.end(); ++i) {
      v->type = module_string;
      v->val.s = i->first.c_str();
      ++v;
      apply_visitor(listel_tomodval(*v), i->second);
      ++v;
    }
  }

  void ordmapvartonums::initmodval() {
    assert(mval.notyet());
    mval.type = module_list;
    struct module_list& l(mval.val.l);
    l.vals = newmodvals(l.n = el.size() * 2);
    struct module_value* v = l.vals;
    assert(ord.size() == el.size());
    for (listelvect_constit o(ord.begin()); o != ord.end(); ++o) {
      listelmap_constit i(el.find(listel_getstring(*o)));
      assert(i != el.end());
      v->type = module_string;
      v->val.s = i->first.c_str();
      ++v;
      apply_visitor(listel_tomodval(*v), i->second);
      ++v;
    }
  }
  
  // ------------------------------------------------------------------------------------------------------------------------

  bool initing = false;
  void initvars() {
    vars.clear();
    varslookup.clear();
    initing = true;
    vars.push_back(boost::shared_ptr<varbase>(new var_verbosity));
    vars.push_back(boost::shared_ptr<varbase>(new var_uselevel));
    vars.push_back(boost::shared_ptr<varbase>(new var_filename));
    vars.push_back(boost::shared_ptr<varbase>(new var_inputmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_outputmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_outputs));
    vars.push_back(boost::shared_ptr<varbase>(new var_clef));
    const symtabs xx(0);
    {var_notesymbols* x; vars.push_back(boost::shared_ptr<varbase>(x = new var_notesymbols)); x->activate(xx);}
    {var_noteaccs* x; vars.push_back(boost::shared_ptr<varbase>(x = new var_noteaccs)); x->activate(xx);}
    {var_notemicrotones* x; vars.push_back(boost::shared_ptr<varbase>(x = new var_notemicrotones)); x->activate(xx);}
    {var_noteoctaves* x; vars.push_back(boost::shared_ptr<varbase>(x = new var_noteoctaves)); x->activate(xx);}
    vars.push_back(boost::shared_ptr<varbase>(new var_noteprint));
    vars.push_back(boost::shared_ptr<varbase>(new percinstrs_var));
    vars.push_back(boost::shared_ptr<varbase>(new instrs_var));
    vars.push_back(boost::shared_ptr<varbase>(new var_nthreads));
    vars.push_back(boost::shared_ptr<varbase>(new var_chooseclef));
    vars.push_back(boost::shared_ptr<varbase>(new var_keysigs));
    vars.push_back(boost::shared_ptr<varbase>(new var_accrule));
    vars.push_back(boost::shared_ptr<varbase>(new var_tquantmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_pquantmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_measmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_checkmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_tposemod));
    vars.push_back(boost::shared_ptr<varbase>(new var_voicesmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_prunemod));
    vars.push_back(boost::shared_ptr<varbase>(new var_vmarksmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_accsmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_cautaccsmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_stavesmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_smarksmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_octsmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_dynsmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_divmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_mergemod));
    vars.push_back(boost::shared_ptr<varbase>(new var_beammod));
    vars.push_back(boost::shared_ptr<varbase>(new var_pmarksmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_marksmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_specialmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_partsmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_defgracedur));
    vars.push_back(boost::shared_ptr<varbase>(new var_rstavesmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_nbeats));
    vars.push_back(boost::shared_ptr<varbase>(new var_comp));
    vars.push_back(boost::shared_ptr<varbase>(new var_staff));
    vars.push_back(boost::shared_ptr<varbase>(new var_timesigden));
    vars.push_back(boost::shared_ptr<varbase>(new var_timesigstyle));
    vars.push_back(boost::shared_ptr<varbase>(new var_name));
    vars.push_back(boost::shared_ptr<varbase>(new var_abbr));    
    vars.push_back(boost::shared_ptr<varbase>(new var_percname));    
    vars.push_back(boost::shared_ptr<varbase>(new var_mpartsmod));    
    vars.push_back(boost::shared_ptr<varbase>(new var_pnotesmod));
    vars.push_back(boost::shared_ptr<varbase>(new var_commkeysig));
    vars.push_back(boost::shared_ptr<varbase>(new var_keysig));
    vars.push_back(boost::shared_ptr<varbase>(new var_markevs1mod));
    {var_dursymbols* x; vars.push_back(boost::shared_ptr<varbase>(x = new var_dursymbols)); x->activate(xx);}
    {var_durdot* x; vars.push_back(boost::shared_ptr<varbase>(x = new var_durdot)); x->activate(xx);}
    {var_durtie* x; vars.push_back(boost::shared_ptr<varbase>(x = new var_durtie)); x->activate(xx);}
    {var_tupsyms* x; vars.push_back(boost::shared_ptr<varbase>(x = new var_tupsyms)); x->activate(xx);}
    {var_dumpingmsg* x; vars.push_back(boost::shared_ptr<varbase>(x = new var_dumpingmsg)); x->activate(xx);}
    vars.push_back(boost::shared_ptr<varbase>(new var_majmode));
    vars.push_back(boost::shared_ptr<varbase>(new var_minmode));
    vars.push_back(boost::shared_ptr<varbase>(new var_title));
    vars.push_back(boost::shared_ptr<varbase>(new var_author));
    vars.push_back(boost::shared_ptr<varbase>(new var_acc));
    vars.push_back(boost::shared_ptr<varbase>(new var_truncate));
    vars.push_back(boost::shared_ptr<varbase>(new var_presets));
    vars.push_back(boost::shared_ptr<varbase>(new var_timesig));
    vars.push_back(boost::shared_ptr<varbase>(new var_beat));
    vars.push_back(boost::shared_ptr<varbase>(new var_timesigs));
    vars.push_back(boost::shared_ptr<varbase>(new var_measparts));
    vars.push_back(boost::shared_ptr<varbase>(new var_barlinel));
    vars.push_back(boost::shared_ptr<varbase>(new var_barliner));
    vars.push_back(boost::shared_ptr<varbase>(new var_finalbar));
    vars.push_back(boost::shared_ptr<varbase>(new var_slurcantouchdef));
    vars.push_back(boost::shared_ptr<varbase>(new var_slurcanspanrestsdef));
    vars.push_back(boost::shared_ptr<varbase>(new var_wedgecantouchdef));
    vars.push_back(boost::shared_ptr<varbase>(new var_wedgecanspanonedef));
    vars.push_back(boost::shared_ptr<varbase>(new var_wedgecanspanrestsdef));
    vars.push_back(boost::shared_ptr<varbase>(new var_textcantouchdef));
    vars.push_back(boost::shared_ptr<varbase>(new var_textcanspanonedef));
    vars.push_back(boost::shared_ptr<varbase>(new var_textcanspanrestsdef));
    vars.push_back(boost::shared_ptr<varbase>(new var_sulstyle));
    vars.push_back(boost::shared_ptr<varbase>(new var_defmarktexts));
    vars.push_back(boost::shared_ptr<varbase>(new var_marktexts));
    vars.push_back(boost::shared_ptr<varbase>(new var_markaliases));
    vars.push_back(boost::shared_ptr<varbase>(new var_pedcantouchdef));
    vars.push_back(boost::shared_ptr<varbase>(new var_pedcanspanonedef));
    vars.push_back(boost::shared_ptr<varbase>(new var_pedstyle));
    vars.push_back(boost::shared_ptr<varbase>(new var_tremtie));
    vars.push_back(boost::shared_ptr<varbase>(new var_fill));
    vars.push_back(boost::shared_ptr<varbase>(new var_inittempotxt));
    vars.push_back(boost::shared_ptr<varbase>(new var_inittempo));
    vars.push_back(boost::shared_ptr<varbase>(new var_detach));
    vars.push_back(boost::shared_ptr<varbase>(new var_pickup));
    // vars.push_back(boost::shared_ptr<varbase>(new var_leftpickup));
    
    initing = false;    
    for (varsvect_constit i(vars.begin()); i != vars.end(); ++i) varslookup.insert(varsmap_val((*i)->getname(), i->get()));
  }

  // copy of this if in fmsin.cc also
  struct catcherr {
    typedef boostspirit::nil_t result_t;
    parserule& recov;
    const parserule& setrule;
    bool& err;
  
    catcherr(parserule& recov, const parserule& setrule, bool& err):recov(recov), setrule(setrule), err(err) {}
    boostspirit::error_status<> operator()(boostspirit::scanner<parse_it> const& s, const boostspirit::parser_error<filepos*, parse_it>& e) const;
  };

  boostspirit::error_status<> catcherr::operator()(boostspirit::scanner<parse_it> const& s, const boostspirit::parser_error<filepos*, parse_it>& e) const {
    e.descriptor->printerr();
    recov = setrule;
    s.first = e.where;
    err = true;
    return boostspirit::error_status<>::retry;
  }

  // copy of this is in fmsin.cc also
  struct setcurloop {   
    parserule& rule;
    const parserule& setrule;
    setcurloop(parserule& rule, const parserule& setrule):rule(rule), setrule(setrule) {}
    void operator()(const parse_it &s1, const parse_it &s2) const {rule = setrule;}
  };

#ifndef NDEBUG
  struct zerocrapout {
    template<typename A, typename B>
    void operator()(const A &s1, const B &s2) const {assert(false);}
  };  
#endif  
  
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(rest,(2,(((confscratch&),xx),((parserule&),cont))),-,
				  cont >> commatch)

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(oksymrule,(3,(((const boostspirit::symbols<parserule*>),conts),((parserule&),cont),((confscratch&),xx))),-,
				  recerrpos(xx.pos.file, xx.pos.line, xx.pos.col, xx.pos.modif) >> symmatcherr(conts, cont, "+:=", xx.str, xx.pos, ferr) >> pluseqlmatch(xx.isplus) >> rest(xx, cont) >> commatch)
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(recovsymrule,(5,(((const boostspirit::symbols<parserule*>),conts),((parserule&),cont),((parserule&),curloop),((parserule&),okrule),((confscratch&),xx))),-,
				  (
#ifndef NDEBUG				 
				   (strmatch(xx.str, "") | boostspirit::eps_p[zerocrapout()])
#else
				   strmatch(xx.str, "")
#endif				 
				   >> commatch
				   >> !(recerrpos(xx.pos.file, xx.pos.line, xx.pos.col, xx.pos.modif) >> symmatch(conts, cont, "+:=") >> pluseqlmatch(xx.isplus) >> rest(xx, cont) >> commatch)[setcurloop(curloop, okrule)]))
  BOOST_SPIRIT_OPAQUE_RULE_PARSER(mainrule,(5,(((boostspirit::guard<filepos*>&),theguard),((parserule&),curloop),((parserule&),recovrule),((const boost::filesystem::path&),filename),((bool&),err))),-,
				  !boostspirit::str_p("\xef\xbb\xbf") >>				  
				  commatch >> *(theguard(if_p(boostspirit::end_p)[boostspirit::nothing_p].else_p[curloop])[catcherr(curloop, recovrule, err)]))

  inline bool confvarssort(const varbase& x, const varbase& y) {
    if (x.getline() != y.getline()) return x.getline() < y.getline(); else return x.getcol() < y.getcol();
  }
  inline bool confvarssortp(const varbase* x, const varbase* y) {return confvarssort(*x, *y);}
  void doloadconf(const boost::filesystem::path& fn) {
    boost::filesystem::ifstream f;
    bool err = false;
    try {
      f.exceptions(boost::filesystem::ifstream::eofbit | boost::filesystem::ifstream::failbit | boost::filesystem::ifstream::badbit);
      f.open(fn, boost::filesystem::ifstream::in | boost::filesystem::ifstream::ate | boost::filesystem::ifstream::binary);
      long l = f.tellg();
      f.seekg(0);
      std::vector<char> buf(l + 1);
      f.read(&buf[0], l);
      buf[l] = 0;
      parse_it p(&buf[0], &buf[l]);
      p.set_tabchars(1);
      p.set_position(boostspirit::file_position_base<std::string>(fn.FS_FILE_STRING()));
      boostspirit::symbols<parserule*> conts; // plconts is settings that are followed by '+='
      std::vector<parserule> symrules(vars.size());
      strscratch xx(0);
      std::for_each(vars.begin(), vars.end(),
		    (boost::lambda::bind(&varbase::addsymbol, boost::lambda::bind(&boost::shared_ptr<varbase>::get, boost::lambda::_1), boost::lambda::var(conts), &symrules[0]),
		     boost::lambda::bind(&varbase::addconfrule, boost::lambda::bind(&boost::shared_ptr<varbase>::get, boost::lambda::_1), &symrules[0], boost::lambda::var(xx))));
      parserule cont;
      boostspirit::guard<filepos*> theguard;
      parserule ruleifok(oksymrule(conts, cont, xx));
      parserule curloop(ruleifok);
      parserule ruleifrecov(recovsymrule(conts, cont, curloop, ruleifok, xx));
      parserule rules(mainrule(theguard, curloop, ruleifrecov, fn, err));
      parse(p, parse_it(), rules);
      f.close();
    } catch (const boost::filesystem::ifstream::failure &e) {
      CERR << "error reading `" << fn.FS_FILE_STRING() << '\'' << std::endl;
      throw errbase();
    }
#ifndef NDEBUG  
    catch (const boostspirit::parser_error<const std::string, parse_it > &e) {
      assert(false);
    }
#endif  
    if (err) throw errbase(); 
  }

  extern boost::filesystem::path userconfig;
  extern boost::filesystem::path fomusconfig;
  void loadconf() {
    try {
      doloadconf(fomusconfig);
    } catch (const boost::filesystem::filesystem_error &e) {
      CERR << "error accessing `" << fomusconfig.FS_FILE_STRING() << "' config file" << std::endl;
      throw errbase();
    }
    currsetwhere = info_config;
    try {
      if (exists(userconfig)) doloadconf(userconfig);
    } catch (const boost::filesystem::filesystem_error &e) {
      CERR << "error accessing `.fomus' config file" << std::endl;
      throw errbase();
    }
    currsetwhere = info_global;
  }

  struct repl_str {
    const std::string re;
    const std::string wi;
  };
  std::string stringify(std::string s, const char* inlist) {
    static const repl_str repl[] = {{"\\", "\\\\"}, {"\t", "\\t"}, {"\n", "\\n"}}; // the only escapes allowed in parse.h
    std::for_each(repl, repl + 3, boost::lambda::bind(boost::replace_all<std::string, const std::string, const std::string>, boost::lambda::var(s), boost::lambda::bind(&repl_str::re, boost::lambda::_1), boost::lambda::bind(&repl_str::wi, boost::lambda::_1)));
    if (!s.empty() && s[0] != '"' && s[0] != '\'' && hasnone(s.begin(), s.end(), boost::lambda::_1 == ' ') 
	&& !boost::contains(s, "//") && !boost::contains(s, "//") && (inlist == 0 || s.find_first_of(inlist) == std::string::npos)) return s;
    boost::replace_all(s, "\"", "\\\"");
    return '"' + s + '"';
  };

  struct unnestmapkey:public boost::static_visitor<const std::string&> {
    const std::string& operator()(const numb& i) const {throw typeerr();}
    const std::string& operator()(const std::string& str) const {return str;}
    const std::string& operator()(const boost::shared_ptr<listelvect>& str) const {throw typeerr();}
  };

  listelmap varbase::nestedlisttomap(const listelvect& vect, const filepos& pos) const {
    try {
      listelmap ve;
      for (listelvect_constit i(vect.begin()); i != vect.end();) {
	const std::string x(apply_visitor(unnestmapkey(), *i++));
	if (i == vect.end()) {
	  CERR << "missing map value for setting `" << getname() << '\'';
	  pos.printerr();
	  throw typeerr();
	}
	ve.insert(listelmap_val(x, *i++));
      }
      return ve;
    } catch (const typeerr& e) {
      #warning "check how throw typeerr is used--replace with errbase"
      throwstype(getname(), gettypedoc(), pos);
      throw;
    }  
  }

  std::string mapvarbase::getvalstr(const fomusdata* fd, const char* st) const {
    std::ostringstream s;
    s << '(';
    if (!el.empty()) {
      s << stringify(el.begin()->first, ":=,") << ' ' << el.begin()->second;
      for (listelmap_constit i(boost::next(el.begin())); i != el.end(); ++i) {
	s << ", " << stringify(i->first, ":=,") << ' ' << i->second;
      }
    }
    s << ')';
    return s.str();
  }
  std::string ordmapvartonums::getvalstr(const fomusdata* fd, const char* st) const {
    std::ostringstream s;
    s << '(';
    if (!el.empty()) {
      listelmap_constit i(el.find(listel_getstring(ord.front())));
      assert(i != el.end());
      s << stringify(i->first, ":=,") << ' ' << i->second;
      for (listelvect_constit o(boost::next(ord.begin())); o != ord.end(); ++o) {
	listelmap_constit i(el.find(listel_getstring(*o)));
	assert(i != el.end());
	s << ", " << stringify(i->first, ":=,") << ' ' << i->second;
      }
    }
    s << ')';
    return s.str();
  }
  
  class isavectaux:public boost::static_visitor<bool> {
  public:
    bool operator()(const numb& x) const {return false;}
    bool operator()(const std::string& x) const {return false;}
    bool operator()(const boost::shared_ptr<listelvect>& x) const {return true;} 
  };
  inline bool isavect(const listel& el) {return apply_visitor(isavectaux(), el);}
  std::ostream& operator<<(std::ostream& s, const listelvect& el) {
    bool forcep = (el.size() != 1 || isavect(el[0]));
    if (forcep) s << '(';
    if (!el.empty()) {
      s << el.front();
      std::for_each(boost::next(el.begin()), el.end(), s << boost::lambda::constant(' ') << boost::lambda::_1);
    }
    if (forcep) s << ')';
    return s;
  }

  bool localallowed(const enum module_setting_loc in, const enum module_setting_loc setloc) { // is setloc allowed in `in'?
    switch (setloc) {
    case module_locscore:
      switch (in) {
      case module_locscore:
	return true;
      case module_locpart:
      case module_locpartmap:
      case module_locinst:
      case module_locmeasdef:
      case module_locpercinst:
      case module_locimport:
      case module_locexport:
      case module_locstaff:
      case module_locclef:
      case module_locnote:
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
    case module_locpart:
      switch (in) {
      case module_locscore:
      case module_locinst:
      case module_locpart:
	return true;
      case module_locpartmap:
      case module_locmeasdef:
      case module_locpercinst:
      case module_locimport:
      case module_locexport:
      case module_locstaff:
      case module_locclef:
      case module_locnote:
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
    case module_locpartmap:
      switch (in) {
      case module_locscore:
      case module_locpartmap:
	return true;
      case module_locpart:
      case module_locinst:
      case module_locmeasdef:
      case module_locpercinst:
      case module_locimport:
      case module_locexport:
      case module_locstaff:
      case module_locclef:
      case module_locnote:
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
    case module_locinst:
      switch (in) {
      case module_locscore:
      case module_locinst:
	return true;
      case module_locpart:
      case module_locpartmap:
      case module_locmeasdef:
      case module_locpercinst:
      case module_locimport:
      case module_locexport:
      case module_locstaff:
      case module_locclef:
      case module_locnote:
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
    case module_locpercinst:
      switch (in) {
      case module_locscore:
      case module_locinst:
      case module_locpercinst:
	return true;
      case module_locpart:
      case module_locpartmap:
      case module_locmeasdef:
      case module_locimport:
      case module_locexport:
      case module_locstaff:
      case module_locclef:
      case module_locnote:
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
    case module_locmeasdef:
      switch (in) {
      case module_locscore:
      case module_locinst:
      case module_locpart:
      case module_locmeasdef:
	return true;
      case module_locpartmap:
      case module_locpercinst:
      case module_locimport:
      case module_locexport:
      case module_locstaff:
      case module_locclef:
      case module_locnote:
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
    case module_locimport:
      switch (in) {
      case module_locscore:
      case module_locinst:
      case module_locpercinst:
      case module_locimport:
	return true;
      case module_locpart:
      case module_locpartmap:
      case module_locmeasdef:
      case module_locexport:
      case module_locstaff:
      case module_locclef:
      case module_locnote:
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
    case module_locexport:
      switch (in) {
      case module_locscore:
      case module_locinst:
      case module_locpercinst:
      case module_locexport:
	return true;
      case module_locpart:
      case module_locpartmap:
      case module_locmeasdef:
      case module_locimport:
      case module_locstaff:
      case module_locclef:
      case module_locnote:
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
    case module_locstaff:
      switch (in) {
      case module_locscore:
      case module_locinst:
      case module_locpart:
      case module_locstaff:
	return true;
      case module_locpartmap:
      case module_locmeasdef:
      case module_locpercinst:
      case module_locimport:
      case module_locexport:
      case module_locclef:
      case module_locnote:
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
    case module_locclef:
      switch (in) {
      case module_locscore:
      case module_locinst:
      case module_locpart:
      case module_locstaff:
      case module_locclef:
	return true;
      case module_locpartmap:
      case module_locmeasdef:
      case module_locpercinst:
      case module_locimport:
      case module_locexport:
      case module_locnote: 
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
    case module_locnote:
      switch (in) {
      case module_locscore:
      case module_locpart:
      case module_locinst:
      case module_locmeasdef:
      case module_locstaff:
      case module_locclef:
      case module_locnote:
	return true;
      case module_locpartmap:
      case module_locpercinst:
      case module_locimport:
      case module_locexport:
	return false;
#ifndef NDEBUG
      case module_noloc:
	assert(false);
#endif    
      }
#ifndef NDEBUG
    case module_noloc:
      assert(false);
#endif    
    }
    assert(false);
  }

  // these come from user settings
  printmap note_print;
  printmap acc_print;
  printmap mic_print;
  printmap oct_print;

  void noplus::operator()(const parse_it &s1, const parse_it &s2) const {
    if (fl) {
      CERR << "cannot append to setting `" << var.getname() << '\'';
      throw_(s1, &var.getpos());
    }
  }  
  const instr_str& fomusdata::getdefinstr(const std::string& id) const {
    definstsmap_constit i(default_insts.find(id)); 
    if (i == default_insts.end()) {
      const globinstsvarvect &mp(((instrs_var*)getvarbase(INSTRS_ID))->getmap());
      globinstsvarvect_constit i2(mp.find(id));
      if (i2 == mp.end()) {
	CERR << "instrument `" << id << "' doesn't exist";
	pos.printerr();
	throw errbase();
      }
      assert(i2->second->isvalid());
      return *i2->second;
    }
    assert(i->second->isvalid());
    return *i->second;
  }
  const percinstr_str& fomusdata::getdefpercinstr(const std::string& id) const {
    defpercsmap_constit i(default_percs.find(id));
    if (i == default_percs.end()) {
      const globpercsvarvect &mp(((percinstrs_var*)getvarbase(PERCINSTRS_ID))->getmap());
      globpercsvarvect_constit i2(mp.find(id));
      if (i2 == mp.end()) {
	CERR << "percussion instrument `" << id << "' doesn't exist";
	pos.printerr();
	throw errbase();
      }
      assert(i2->second->isvalid());
      return *i2->second;
    }
    assert(i->second->isvalid());
    return *i->second;
  }

  presetsmap presets;
  std::string presetstype;
  
  int foundpreset(const char *filename, lt_ptr data) {
    try {
      std::string mn(FS_BASENAME(boost::filesystem::path(filename)));
      boost::filesystem::path pa(FS_CHANGE_EXTENSION(boost::filesystem::path(filename), ".fpr"));
      if (!boost::filesystem::exists(pa)) return 0;
      presets.insert(presetsmap::value_type(mn, pa));
    } catch (const boost::filesystem::filesystem_error &e) {
      CERR << "error accessing file `" << filename << '\'' << std::endl;
    }      
    return 0;
  }
  
  // activate presets
  void initpresets() {
    presets.clear();
    char *bp = getenv("FOMUS_BUILTIN_PRESETS_PATH");
#ifdef __MINGW32__
    const char* wsys = getenv("WINDIR");
    std::string pa(std::string(bp ? bp : (wsys ? wsys : "")) + CMD_MACRO(PRESETS_PATH));
#else
    std::string pa(bp ? bp : CMD_MACRO(PRESETS_PATH));
#endif    
    char *p = getenv("FOMUS_PRESETS_PATH");
    if (p != NULL) pa = std::string(p) + LT_PATHSEP_CHAR + pa;
    lt_dlforeachfile(pa.c_str(), foundpreset, 0);
    std::ostringstream x;
    for (presetsmap_constit i(presets.begin()); i != presets.end(); ++i) {
      if (i != presets.begin()) x << '|';
      x << i->first;
    }
    presetstype = x.str();
  }
  
  void var_presets::activate(const symtabs& ta) const {
    for (listelvect_constit i(el.begin()); i != el.end(); ++i) {
      assert(presets.find(listel_getstring(*i)) != presets.end());
      if (ta.fd) {
	modsvect_it i0(std::find_if(mods.begin(), mods.end(), boost::lambda::bind(&modbase::modin_hasext, boost::lambda::_1, "fms")));
	if (i0 == mods.end()) {
	  CERR << "cannot load preset, `.fms' file input module not found" << std::endl;
	  throw errbase();
	}
	const modbase& mo = *i0;
	moddata d(mo, mo.getdata(ta.fd));
	if (mo.loadfile(ta.fd, d.get(), presets.find(listel_getstring(*i))->second.FS_FILE_STRING().c_str(), true)) throw errbase();
      } else doloadconf(presets.find(listel_getstring(*i))->second);
    }
  }

  const char* var_outputs::gettypedoc() const {
    static std::string typstr;
    if (typstr.empty()) {
      std::ostringstream ou;
      ou << "string_ext | (string_ext string_ext ...), string_ext = ";
      bool fi = true;
      assert(!mods.empty());
      for (modsvect_constit i(mods.begin()); i != mods.end(); ++i) i->modout_collext(ou, fi);
      typstr = ou.str();
    }
    return typstr.c_str();
  }

  const char* var_inputmod::gettypedoc() const {
    static std::string typstr;
    if (typstr.empty()) {
      std::ostringstream ou;
      ou << "(string_ext string_modname, string_ext string_modname, ...), string_modname = ";
      bool fi = true;
      assert(!mods.empty());
      for (modsvect_constit i(mods.begin()); i != mods.end(); ++i) {
	if (fi) fi = false; else ou << '|';
	if (i->gettype() == module_modinput) ou << i->getcname();
      }
      typstr = ou.str();
    }
    return typstr.c_str();
  }

  const char* var_outputmod::gettypedoc() const {
    static std::string typstr;
    if (typstr.empty()) {
      std::ostringstream ou;
      ou << "(string_ext string_modname, string_ext string_modname, ...), string_modname = ";
      bool fi = true;
      assert(!mods.empty());
      for (modsvect_constit i(mods.begin()); i != mods.end(); ++i) {
	if (fi) fi = false; else ou << '|';
	if (i->gettype() == module_modoutput) ou << i->getcname();
      }
      typstr = ou.str();
    }
    return typstr.c_str();
  }

  const char* var_chooseclef::gettypedoc() const {
    static std::string typstr;
    if (typstr.empty()) {
      std::ostringstream s;
      s << "string_clef | (string_clef string_clef ...), string_clef = " << clefstypestr;
      typstr = s.str();
    }
    return typstr.c_str();
  }

  const char* var_presets::gettypedoc() const {
    static std::string typstr;
    if (typstr.empty()) {
      std::ostringstream s;
      s << "string_prefix | (string_prefix string_prefix ...), string_prefix = " << presetstype;
      typstr = s.str();
    }
    return typstr.c_str();
  }

  void redokeysigs(const symtabs& ta) { // var_keysigs, KEYSIG_ID contains allsig
    if (ta.fd) {
      assert(ta.fd->isvalid());
      DBG("in redokeysigs the address of var_keysigs is " << &(const var_keysigs&)ta.fd->get_varbase(KEYSIG_ID));
      assert(((const var_keysigs&)ta.fd->get_varbase(KEYSIG_ID)).varisvalid());
      assert(ta.fd->fu());
      var_keysigs *k;
      ta.fd->set_varbase(KEYSIG_ID, k = new var_keysigs((const var_keysigs&)ta.fd->get_varbase(KEYSIG_ID)));
      k->redo(ta.fd);
      var_keysig *x;
      ta.fd->set_varbase(KEYSIGDEF_ID, x = new var_keysig((const var_keysig&)ta.fd->get_varbase(KEYSIGDEF_ID)));
      x->redo(ta.fd);
    } else {
#ifndef NDEBUG      
      ((var_keysigs&)*vars[KEYSIG_ID]).fu();
#endif      
      ((var_keysigs&)*vars[KEYSIG_ID]).redo(ta);
      ((var_keysig&)*vars[KEYSIGDEF_ID]).redo(ta);
    }
  }
  
  varbase* ordmapvartonums::getnewprepend(const listelvect& val, const filepos& p) {
    listelvect z;
    for (listelvect_constit i(ord.begin()); i != ord.end(); ++i) {
      z.push_back(*i);
      std::string str(listel_getstring(*i));
#ifndef NDEBUG
      listelmap_constit j(el.find(str));
      assert(j != el.end());
      z.push_back(j->second);
#else    
      z.push_back(el.find(str)->second);    
#endif    
    }  
    std::copy(val.begin(), val.end(), back_inserter(z));
    return getnew(z, p);
  }

  void var_notesymbols::activate(const symtabs& ta) const {
    fomsymbols<numb> ne;
    for (listelmap_constit i(el.begin()); i != el.end(); ++i) {
      ne.add(i->first.c_str(), listel_getnumb(i->second));
    }
    ta.note = ne;
    numtostring_ins(ta.notepr);
    if (!initing) redokeysigs(ta);
  }

  void var_noteaccs::activate(const symtabs& ta) const {
    fomsymbols<numb> ne;
    for (listelmap_constit i(el.begin()); i != el.end(); ++i) ne.add(i->first.c_str(), listel_getnumb(i->second));
    ta.acc = ne;
    numtostring_ins(ta.accpr);
    if (!initing) redokeysigs(ta);
  }

  void var_notemicrotones::activate(const symtabs& ta) const {
    fomsymbols<numb> ne;
    for (listelmap_constit i(el.begin()); i != el.end(); ++i) ne.add(i->first.c_str(), listel_getnumb(i->second));
    ta.mic = ne;
    numtostring_ins(ta.micpr);
    if (!initing) redokeysigs(ta);
  }

  void var_noteoctaves::activate(const symtabs& ta) const {
    fomsymbols<numb> ne;
    for (listelmap_constit i(el.begin()); i != el.end(); ++i) ne.add(i->first.c_str(), listel_getnumb(i->second));
    ta.oct = ne;
    numtostring_ins(ta.octpr);
    if (!initing) redokeysigs(ta);
  }

  var_defmarktexts::var_defmarktexts():mapvartostrings() {
    assert(getid() == DEFAULTMARKTEXTS_ID);
    el.insert(listelmap_val("pizz", std::string("pizz.")));
    el.insert(listelmap_val("arco", std::string("arco")));
    el.insert(listelmap_val("mute", std::string("con sord.")));
    el.insert(listelmap_val("unmute", std::string("senza sord.")));
    el.insert(listelmap_val("vib", std::string("vib.")));
    el.insert(listelmap_val("moltovib", std::string("molto vib.")));
    el.insert(listelmap_val("nonvib", std::string("non vib.")));
    el.insert(listelmap_val("leg", std::string("legato")));
    el.insert(listelmap_val("moltoleg", std::string("molto legato")));
    el.insert(listelmap_val("nonleg", std::string("non legato")));
    el.insert(listelmap_val("sul", std::string("Sul ")));
    el.insert(listelmap_val("salt", std::string("saltando")));
    el.insert(listelmap_val("ric", std::string("jeté")));
    el.insert(listelmap_val("lv", std::string("l.v.")));
    el.insert(listelmap_val("flt", std::string("flt.")));
    el.insert(listelmap_val("slap", std::string("slap tongued")));
    el.insert(listelmap_val("breath", std::string("breath tone")));
    
    el.insert(listelmap_val("spic", std::string("spiccato")));
    el.insert(listelmap_val("tall", std::string("al tallone")));
    el.insert(listelmap_val("punta", std::string("punta d'arco")));
    el.insert(listelmap_val("pont", std::string("sul pont.")));
    el.insert(listelmap_val("tasto", std::string("sul tasto")));
    el.insert(listelmap_val("legno", std::string("col legno")));    
    el.insert(listelmap_val("flaut", std::string("flautando")));
    el.insert(listelmap_val("etouf", std::string("sons étouffes")));
    el.insert(listelmap_val("table", std::string("près de la table")));    
    el.insert(listelmap_val("cuivre", std::string("cuivré")));
    el.insert(listelmap_val("bellsup", std::string("bells up")));
    el.insert(listelmap_val("ord", std::string("ord.")));
    initmodval();
  }
  
}
