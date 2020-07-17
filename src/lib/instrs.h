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

#ifndef FOMUS_INSTRS_H
#define FOMUS_INSTRS_H

#ifndef BUILD_LIBFOMUS
#error "strs.h shouldn't be included"
#endif

#include "heads.h"
#include "numbers.h" // fint
#include "algext.h" // renew
#include "userstrs.h" // clefs_enum
#include "vars.h" // class setmap
#include "module.h" // module_ratslist, etc..
#include "infoapi.h"

namespace fomus {

  class partormpart_str;
  typedef std::list<boost::shared_ptr<partormpart_str> > scorepartlist;
  typedef scorepartlist::iterator scorepartlist_it;
  typedef scorepartlist::const_iterator scorepartlist_constit;

  struct offgroff {
    numb off;
    numb groff;
    offgroff(const numb& off, const numb& groff):off(off), groff(groff) {}
    offgroff(const numb& off):off(off), groff(module_none) {}
    offgroff() {}
    bool isgrace() const {return groff.isntnull();}
  };

  class measure;
  typedef boost::ptr_multimap<const offgroff, measure> measmap;
  typedef measmap::iterator measmap_it;
  typedef measmap::const_iterator measmap_constit;
  typedef measmap::value_type measmap_val;
  typedef boost::ptr_multimap<const offgroff, measure, std::less<const offgroff>, boost::view_clone_allocator> measmapview;
  typedef measmapview::iterator measmapview_it;

  inline bool operator<(const offgroff& x, const offgroff& y) {
    if (x.off == y.off) {
      return x.groff.isnull() ? false : (y.groff.isnull() ? true : x.groff < y.groff);
    } else return x.off < y.off;
  }
  inline bool operator>(const offgroff& x, const offgroff& y) {return y < x;}
  inline bool operator==(const offgroff& x, const offgroff& y) { // grace offset might be `none'
    if (x.off == y.off) {
      return (x.groff.isnull() ? y.groff.isnull() : (y.groff.isnull() ? false : x.groff == y.groff));
      // if (x.groff.type() == module_none && y.groff.type() == module_none) return true;
      // if (x.groff.type() == module_none || y.groff.type() == module_none) return false;
      // return x.groff == y.groff;
    } else return false;
  }
  inline bool operator!=(const offgroff& x, const offgroff& y) {
    return !(x == y);
  }
  inline bool operator>=(const offgroff& x, const offgroff& y) {return !(x < y);}
  inline bool operator<=(const offgroff& x, const offgroff& y) {return !(y < x);}

  struct scoped_info_setlist:public info_setlist {
    std::vector<info_setting> arr;
    scoped_info_setlist() {n = 0; sets = 0;}
    void resize(const int sz) {
      resetsetlist();
      if (sz != n) {
	arr.resize(n = sz);
	sets = &arr[0];
      }
    }
    ~scoped_info_setlist() {resetsetlist();}
    void resetsetlist() {
      std::for_each(arr.begin(), arr.end(), boost::lambda::bind(delete_charptr0, boost::lambda::bind(&info_setting::valstr, boost::lambda::_1)));
    }
  };

  const varbase& fom_get_varbase_up(const int id); // only called from module, so stageobj must be valid
  fint fom_get_ival_up(const int id); 
  rat fom_get_rval_up(const int id); 
  ffloat fom_get_fval_up(const int id); 
  const std::string& fom_get_sval_up(const int id); 
  const module_value& fom_get_lval_up(const int id); 

  class event;
  class noteevbase;
  typedef boost::ptr_multimap<offgroff, noteevbase> eventmap;
  typedef eventmap::iterator eventmap_it;
  typedef eventmap::const_iterator eventmap_constit;
  typedef eventmap::value_type eventmap_val;
  
  // any object allowing user to grab a setting value
  struct wrongtype:public errbase {
    const char* what;
    wrongtype(const char* what):what(what) {}
  };
  class objlesserr:public errbase {};

  class part;
  class markobj;
  class noteev;
  class noteevbase;
  class markev;
  class modobjbase {
  public:
#ifndef NDEBUG
    int debugvalid;
    bool isvalid() const {return debugvalid == 12345;}
#endif
    modobjbase() {
#ifndef NDEBUG
      debugvalid = 12345;
#endif    
    }
    modobjbase(const modobjbase& x) {
#ifndef NDEBUG
      debugvalid = 12345;
#endif    
    }
    virtual ~modobjbase() {
#ifndef NDEBUG
      debugvalid = 0;
#endif    
    }
    virtual const varbase& get_varbase(const int id) const {assert(false);};
    virtual fint get_ival(const int id) const {assert(false);};
    virtual rat get_rval(const int id) const {assert(false);};
    virtual ffloat get_fval(const int id) const {assert(false);};
    virtual const std::string& get_sval(const int id) const {assert(false);};
    virtual const module_value& get_lval(const int id) const {assert(false);};
    virtual info_setlist& getsettinginfo() {throw wrongtype("a settings-containing");}
    virtual const char* getcid() const {throw wrongtype("an id-containing");}
    virtual const char* gettype() const = 0;
    virtual const eventmap_it& getnoteselfit() const {throw wrongtype("a note");}
    virtual const measmap_it& getmeasselfit() const {throw wrongtype("a measure");}
    virtual const measmap_it& getmeasselfit(const bool istmp) const {throw wrongtype("a measure");}
    virtual const scorepartlist_it& getpartselfit() const {throw wrongtype("a part");}
    virtual modobjbase* getmeasobj() {throw wrongtype("a note");}
    virtual modobjbase* getpartobj() {throw wrongtype("a note or measure");}
    virtual modobjbase* getinstobj() {throw wrongtype("a note, measure or part");}
    virtual modobjbase* getmeasdef() {throw wrongtype("a measure");}
    virtual const char* getprintstr() const {throw wrongtype("a printable");}
    virtual const numb& getdyn() const {throw wrongtype("a note");}
    virtual const numb& gettime() const {throw wrongtype("a note or measure");}
    virtual const numb& getgracetime() const {throw wrongtype("a grace note/rest");}
    virtual numb getdur() const {throw wrongtype("a note or measure");}
    virtual numb getendtime() const {throw wrongtype("a note or measure");}
    virtual numb getgraceendtime() const {throw wrongtype("a grace note/rest");}
    virtual numb getgracedur() const {throw wrongtype("a grace note/rest");}
    virtual numb gettiedendtime() const {throw wrongtype("a note");}
    virtual const numb& getnote() const {throw wrongtype("a note");}
    virtual numb getwrittennote() const {throw wrongtype("a note");}
    virtual int getvoice() const {throw wrongtype("a note or rest");}
    virtual int getstaff() const {throw wrongtype("a note or rest");}
    virtual struct module_intslist getvoices() {throw wrongtype("a note, measure or part");}
    virtual struct module_intslist getstaves() {throw wrongtype("a note, measure or part");}
    virtual struct module_intslist getclefs() {throw wrongtype("a note, measure or part");} // possible clefs obtained by going down to staves and grabbing them
    virtual struct module_intslist getclefs(const int staff) {throw wrongtype("a note, measure or part");} // possible clefs obtained by going down to staves and grabbing them
    virtual int getoctsign() const {throw wrongtype("a note or rest");}
    virtual fomus_rat getfullwrittenacc() const {throw wrongtype("a note");} // accidental that appears in score (after key signature and measure position)
    virtual fomus_rat getwrittenacc1() const {throw wrongtype("a note");} // accidental that appears in score (after key signature and measure position)
    virtual fomus_rat getwrittenacc2() const {throw wrongtype("a note");} // accidental that appears in score (after key signature and measure position)
    virtual fomus_rat getfullacc() const {throw wrongtype("a note");}
    virtual fomus_rat getacc1() const {throw wrongtype("a note");}
    virtual fomus_rat getacc2() const {throw wrongtype("a note");}
    virtual numb getwrittendur(const int level) const {throw wrongtype("a note or rest");}
    virtual numb getwrittengracedur(const int level) const {throw wrongtype("a note or rest");}
    virtual fomus_rat getbeatstowrittendur(const fomus_rat& dur, const int level) const {throw wrongtype("a note or rest");}
    virtual bool getoctavebegin() {throw wrongtype("a note or rest");}
    virtual bool getoctaveend() {throw wrongtype("a note or rest");}
    virtual bool getistiedleft() const {throw wrongtype("a note");}
    virtual bool getistiedright() const {throw wrongtype("a note");}
    virtual bool gettupletbegin(const int level) const {throw wrongtype("a note or rest");}
    virtual bool gettupletend(const int level) const {throw wrongtype("a note or rest");}
    virtual bool gethasvoice(const int voice) const {throw wrongtype("a note or rest");}
    virtual bool gethasstaff(const int staff) const {throw wrongtype("a note or rest");}
    virtual bool gethasclef(const int clef) {throw wrongtype("a note, measure or part");}
    virtual bool gethasclef(const int clef, const int staff) {throw wrongtype("a note, measure or part");}
    virtual bool getisrest() const {return false;}
    virtual bool getisnote() const {return false;}
    virtual bool getisgrace() const {throw wrongtype("a note or rest");}
    virtual module_ratslist getdivs() const {throw wrongtype("a measure");}
    virtual module_markslist getspannerbegins() {throw wrongtype("a note or rest");} // get all of them
    virtual module_markslist getspannerends() {throw wrongtype("a note or rest");}
    virtual module_markslist getsinglemarks() {throw wrongtype("a note or rest");}
    virtual module_markslist getmarks() {throw wrongtype("a note or rest");}
    virtual const char* getmarkstr() const {throw wrongtype("a mark");}
    virtual const numb& getmarkval() const {throw wrongtype("a mark");}
    virtual int getmarkorder() const {throw wrongtype("a mark");}
    virtual bool getisbeginchord() const {throw wrongtype("a note");}
    virtual bool getisendchord() const {throw wrongtype("a note");}
    virtual bool getischordlow() const {throw wrongtype("a note");}
    virtual bool getischordhigh() const {throw wrongtype("a note");}
    virtual int getmarkid() const {throw wrongtype("a mark");}

    virtual void assignmeas(const fomus_rat& time, const fomus_rat& dur, const bool rmable) {throw wrongtype("a measure");}
    virtual void assigntquant(struct fomus_rat& time, struct fomus_rat& dur) {throw wrongtype("a note or rest");}
    virtual void assigntquant(struct fomus_rat& time, struct module_value& grtime, struct module_value& dur) {throw wrongtype("a note or rest");}
    virtual void assigntquantdel() {throw wrongtype("a note or rest");}
    virtual void assignpquant(const fomus_rat& p) {throw wrongtype("a note");}
    virtual void skipassign() {throw wrongtype("a note or rest");}
    virtual void assignvoices(const int voice) {throw wrongtype("a note");}
    virtual void assignstaves(const int staff, const int clef) {throw wrongtype("a note");}
    virtual void assignrstaff(const int staff, const int clef) {throw wrongtype("a rest");}
    virtual void assignaccs(const fomus_rat& acc, const fomus_rat& macc) {throw wrongtype("a note");}
    virtual void assigndivs(const module_ratslist& divs0) {throw wrongtype("a measure");}
    virtual void assignsplit(const divide_split& split0) {throw wrongtype("a measure");}
    virtual void assigngracesplit(const divide_gracesplit& split0) {throw wrongtype("a measure");}
    virtual void assigncautacc() {throw wrongtype("a note");}
    virtual void assignbeams(const int bl, const int br) {throw wrongtype("a note");}
    virtual void assignorder(const int ord, const bool tmark) {throw wrongtype("a part");}
    virtual void assignprune(const fomus_rat& time1, const fomus_rat& time2) {throw wrongtype("a note");}
    virtual void assignprunedone() {throw wrongtype("a note");}
    virtual void assigngroupbegin(const parts_grouptype type) {throw wrongtype("a part");}
    virtual void assigngroupend(const parts_grouptype type) {throw wrongtype("a part");}
    virtual void assignocts(const int octs) {throw wrongtype("a note");}
    virtual void assignmarkrem(const int type, const char* arg1, const module_value& arg2) {throw wrongtype("a note or rest");}
    virtual void assignmarkins(const int type, const char* arg1, const module_value& arg2) {throw wrongtype("a note or rest");}
    virtual void assignmpart(const module_partobj part, const int voice, const fomus_rat& pitch) {throw wrongtype("a note, rest or mark event");}
    virtual void assignmpart(std::auto_ptr<noteevbase>& pa) {throw wrongtype("a part");}
    virtual void assignmpartmarkev(std::auto_ptr<markev>& ev) {throw wrongtype("a part");}
    
    virtual bool objless(const modobjbase& y) const {throw objlesserr();}
    virtual bool objless(const markobj& y) const {throw objlesserr();}
    virtual bool objless(const noteevbase& y) const {throw objlesserr();}
    virtual bool objless(const noteev& y) const {throw objlesserr();}
    virtual bool objless(const measure& y) const {throw objlesserr();}
    virtual bool objgreat(const markobj& y) const {throw objlesserr();}
    virtual bool objgreat(const noteevbase& y) const {throw objlesserr();}
    virtual bool objgreat(const noteev& y) const {throw objlesserr();}
    virtual bool objgreat(const measure& y) const {throw objlesserr();}

    virtual noteevbase& getnoteevbase() {throw wrongtype("a note");}

    virtual fomus_rat gettuplet(const int level) const {throw wrongtype("a note or rest");}
    virtual enum parts_grouptype partgroupbegin(const int lvl) const {throw wrongtype("a part");}
    virtual bool partgroupend(const int lvl) const {throw wrongtype("a part");}
    virtual fomus_rat timesig() const {throw wrongtype("a measure");}
    virtual fomus_rat writtenmult() const {throw wrongtype("a measure");}
    virtual int getclef() const {throw wrongtype("a note or rest");}
    virtual int getnstaves() const {throw wrongtype("a part");}
    virtual const char* getdefclef(const int staff) const {throw wrongtype("a part");}
    virtual int getvoiceinstaff() const {throw wrongtype("a note or rest");}
    virtual bool getisfullmeasrest() const {throw wrongtype("a measure");}
    virtual metaparts_partmaps getpartmaps() const {throw wrongtype("a metapart");}
    virtual modobjbase* getmappart() const {throw wrongtype("a partmap");}
    virtual bool ismetapart() const {throw wrongtype("a part");}
    virtual module_percinstobj getpercinst() const {throw wrongtype("a note");}
    virtual void assignpercinst(const int voice, struct fomus_rat& pitch) {throw wrongtype("a note");}
    virtual modobjbase* getpartstaffobj(const int st) const {throw wrongtype("a part");}
    virtual modobjbase* getpartclefobj(const int st, const int cl) const {throw wrongtype("a part");}
    virtual int getbeamsleft() const {throw wrongtype("a note");}
    virtual int getbeamsright() const {throw wrongtype("a note");}
    virtual module_objlist getmarkevslist() {throw wrongtype("a part");}
    virtual const char* getfileposstr() const {throw wrongtype("a note");}
    virtual bool getisperc() const {throw wrongtype("a note");}
    virtual const char* getpercinststr() const {throw wrongtype("a note");}
    virtual struct modout_keysig getkeysig() const {throw wrongtype("a measure");}
    virtual fomus_rat getfulltupdur(const int lvl) const {throw wrongtype("a note or rest");}
    virtual int getconnbeamsleft() const {throw wrongtype("a note");}
    virtual int getconnbeamsright() const {throw wrongtype("a note");}
    virtual struct modin_imports getimports() {throw wrongtype("a part, instrument or percussion instrument");}
    virtual module_obj getexport() {throw wrongtype("a part, instrument or percussion instrument");}
    virtual const char* getimportpercid() const {throw wrongtype("an import object");}
    virtual void assignnewkeysig(const char* str) {throw wrongtype("a measure");}
    virtual const struct module_keysigref* getkeysigref() const {throw wrongtype("a measure");}
    virtual struct module_keysigref getkeysigacc() const {throw wrongtype("a note");}
    virtual struct module_keysigref getkeysigacc(const int note) const {throw wrongtype("a measure");}
    virtual modobjbase* leftmosttied() {throw wrongtype("a note");}
    virtual modobjbase* rightmosttied() {throw wrongtype("a note");}
    virtual void recacheassign() {throw wrongtype("a note or rest");}
    virtual enum module_markpos getmarkpos() const {throw wrongtype("a mark");}
    virtual void setmarkpos(const enum module_markpos p) {throw wrongtype("a mark");}
    virtual void assigninv() {throw wrongtype("a note or rest");}
    virtual void assigndetmark(const numb& off, const int voice, const int type, const char* arg1, const struct module_value& arg2) {throw wrongtype("a part");}
    virtual bool isinvisible() const {throw wrongtype("a note or rest");}
    virtual bool getismarkrest() const {throw wrongtype("a note or rest");}
    virtual module_barlines getbarline() const {throw wrongtype("a measure");}
    virtual void newinvnote(const fomus_rat& note, const fomus_rat& acc1, const fomus_rat& acc2, const special_markslist& marks) {throw wrongtype("a note");}
    //virtual void assignunsplit0(modobjbase* note2) {throw wrongtype("a note");}
    virtual void assignunsplit(/*noteev& note1*/) {throw wrongtype("a note");}
    virtual int getpartialmeas() const {throw wrongtype("a measure");}
    virtual int getpartialbarline() const {throw wrongtype("a measure");}
  };
  class modobjbase_sets:public modobjbase {
  protected:
    scoped_info_setlist setlist;
  };

  // MODULE VALUES AREN'T CACHED IN THESE OBJECTS
  class fomusdata;
  class str_base:public modobjbase_sets {
  public:
    setmap sets;
    str_base() {}
    str_base(const str_base& x):sets(x.sets) {}
    setmap& getsets() {return sets;}
    void setting(const int id, std::auto_ptr<varbase>& set, const fomusdata* fd) {
      set->throwifinvalid(fd);
      sets.insert(setmap_val(id, boost::shared_ptr<const varbase>(set.release())));
    } // varbase* is already stored
    void completesets(const str_base& b) {for (setmap_constit i(b.sets.begin()); i != b.sets.end(); ++i) sets.insert(*i);}
    void getmodvals(module_value* x) const; // array must be 2x sets.size
    void getmodval(module_value& x) const {
      x.type = module_list;
      getmodvals(x.val.l.vals = newmodvals(x.val.l.n = sets.size() * 2) /*new module_value[x.val.l.n = sets.size() * 2]*/);
    }
    void printsets(std::ostream& s, const fomusdata* fd, bool& sm) const; // leave the fomusdata& in there! str's FOM might be 0 while fd is a fomusdata struct
    void print(std::ostream& s, const fomusdata* fd) const;
    void replace(str_base& x) {
      sets = x.sets;
    }
    
    bool get_varbase0(const int id, const varbase* &ret) const {setmap_constit i(sets.find(id)); if (i != sets.end()) {ret = i->second.get(); return true;} else return false;}
    bool get_ival0(const int id, fint& ret) const {setmap_constit i(sets.find(id)); if (i != sets.end()) {ret = i->second->getival(); return true;} else return false;}
    bool get_rval0(const int id, rat& ret) const {setmap_constit i(sets.find(id)); if (i != sets.end()) {ret = i->second->getrval(); return true;} else return false;}
    bool get_fval0(const int id, ffloat& ret) const {setmap_constit i(sets.find(id)); if (i != sets.end()) {ret = i->second->getfval(); return true;} else return false;}
    bool get_sval0(const int id, const std::string* &ret) const {setmap_constit i(sets.find(id)); if (i != sets.end()) {ret = &i->second->getsval(); return true;} else return false;}
    bool get_lval0(const int id, const module_value* &ret) const {setmap_constit i(sets.find(id)); if (i != sets.end()) {ret = &i->second->getmodval(); return true;} else return false;}
    info_setlist& getsettinginfo();
    virtual void getlastentry(info_objinfo& str, const fomusdata& fd) const {assert(false);}
  };
  class instrorpercinstr_str:public str_base {
  public:
    instrorpercinstr_str():str_base() {}
    instrorpercinstr_str(const instrorpercinstr_str& x):str_base(x) {}
  };

  template<typename T>
  inline void addstrconfrule(boost::shared_ptr<T>& shthis, boostspirit::symbols<parserule*>& syms, std::vector<parserule> &ret, confscratch& x,
			     const module_setting_loc loc, const char* typestr) { 
    ret.resize(vars.size());
    for (varsvect_it i(vars.begin()); i != vars.end(); ++i) {
      varbase& v = **i;
      v.addsymbol(syms, &ret[0]);
      if (v.isallowed(loc)) {
	v.addconfrulestr(&ret[0], x, shthis);
      } else {
	v.addnotallowedrule(&ret[0], typestr, x.pos);
      }
    } 
  }

  // template<typename V, typename T>
  // inline void sticknewvar<V, T>::operator()(const parse_it &s1, const parse_it &s2) const {
  //   map->getsets().insert(setmap_val(v.getid(), boost::shared_ptr<const varbase>(v.getnew(val, fp))));
  // }
  template<typename T>
  inline void sticknewvar<T>::operator()(const parse_it &s1, const parse_it &s2) const {
    assert(var.get());
    map->getsets().insert(setmap_val(var->getid(), var));
  }
  
  // ------------------------------------------------------------------------------------------------------------------------
  // STORAGE

  // import
  struct import_str:public str_base NONCOPYABLE {
  private:
    const instrorpercinstr_str* par;
  public:
    import_str():str_base(), par((instrorpercinstr_str*)0) {}
    void complete(const fomusdata& fd) {}
    void complete(const filepos& pos) {}
    import_str(const import_str& x, const instrorpercinstr_str* par):str_base(x), par(par) {}
    boost::shared_ptr<import_str> copy(const instrorpercinstr_str* newpar) const {return boost::shared_ptr<import_str>(new import_str(*this, newpar));}
    void setpar(const instrorpercinstr_str* ptr) {par = ptr;}
    const varbase& get_varbase(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : get_varbase_up(id);}
    fint get_ival(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : get_ival_up(id);}
    rat get_rval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : get_rval_up(id);}
    ffloat get_fval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : get_fval_up(id);}
    const std::string& get_sval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : get_sval_up(id);}
    const module_value& get_lval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : get_lval_up(id);}    
    const varbase& get_varbase_up(const int id) const {return par->get_varbase(id);}
    fint get_ival_up(const int id) const {return par->get_ival(id);}
    rat get_rval_up(const int id) const {return par->get_rval(id);}
    ffloat get_fval_up(const int id) const {return par->get_fval(id);}
    const std::string& get_sval_up(const int id) const {return par->get_sval(id);}
    const module_value& get_lval_up(const int id) const {return par->get_lval(id);}
    const char* gettype() const {return "an import object";}
    const char* getimportpercid() const {return par->getimportpercid();}
  };
  inline void import_addstrconfrule(boost::shared_ptr<import_str>& shthis, boostspirit::symbols<parserule*>& syms, std::vector<parserule> &ret, confscratch& x) {
    addstrconfrule(shthis, syms, ret, x, module_locimport, "an import object");
  }
  inline void copyimps(const std::vector<boost::shared_ptr<import_str> >& from, std::vector<boost::shared_ptr<import_str> >& to, const instrorpercinstr_str* newpar) {
    assert(to.empty());
    std::transform(from.begin(), from.end(), std::back_inserter(to), boost::lambda::bind(&import_str::copy, boost::lambda::bind(&boost::shared_ptr<import_str>::get, boost::lambda::_1), newpar));
  }

  // export
  struct export_str:public str_base {
  private:
    const instrorpercinstr_str* par;
  public:
    export_str():str_base(), par((instrorpercinstr_str*)0) {}
    void complete(const fomusdata& fd) {}
    void complete(const filepos& pos) {}
    export_str(const export_str& x, const instrorpercinstr_str* par):str_base(x), par(par) {}
    boost::shared_ptr<export_str> copy(const instrorpercinstr_str* newpar) const {return boost::shared_ptr<export_str>(new export_str(*this, newpar));}
    void setpar(const instrorpercinstr_str* ptr) {par = ptr; /*DBG("xpar=" << par << std::endl);*/}
    const varbase& get_varbase(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : get_varbase_up(id);}
    fint get_ival(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : get_ival_up(id);}
    rat get_rval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : get_rval_up(id);}
    ffloat get_fval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : get_fval_up(id);}
    const std::string& get_sval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : get_sval_up(id);}
    const module_value& get_lval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : get_lval_up(id);}    
    const varbase& get_varbase_up(const int id) const {return par->get_varbase(id);}
    fint get_ival_up(const int id) const {return par->get_ival(id);}
    rat get_rval_up(const int id) const {return par->get_rval(id);}
    ffloat get_fval_up(const int id) const {return par->get_fval(id);}
    const std::string& get_sval_up(const int id) const {return par->get_sval(id);}
    const module_value& get_lval_up(const int id) const {return par->get_lval(id);}
    const char* gettype() const {return "an export object";}
  };
  inline void export_addstrconfrule(boost::shared_ptr<export_str>& shthis, boostspirit::symbols<parserule*>& syms, std::vector<parserule> &ret, confscratch& x) {
    addstrconfrule(shthis, syms, ret, x, module_locexport, "an export object");
  }

  // clefs
  struct staff_str;
  struct clef_str:public str_base NONCOPYABLE { // input holder
  private:
    const staff_str* par;
  public:
    clef_str():str_base(), par(0) {}
    clef_str(const std::string& cl):str_base(), par(0) {
      str_base::sets.insert(setmap_val(CLEF_ID, boost::shared_ptr<varbase>(vars[CLEF_ID]->getnewstr(0, cl, filepos(info_global))))); // set when fomusdata is created, so it's global
    }
    clef_str(const clef_str& x, const staff_str* par):str_base(x), par(par) {}
    boost::shared_ptr<clef_str> copy(const staff_str* newpar) const {return boost::shared_ptr<clef_str>(new clef_str(*this, newpar));}
    void complete(const fomusdata& fd) {}
    void complete(const filepos& pos) {}
    void setpar(const staff_str* ptr) {par = ptr;}
    const varbase& get_varbase(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : get_varbase_up(id);}
    fint get_ival(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : get_ival_up(id);}
    rat get_rval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : get_rval_up(id);}
    ffloat get_fval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : get_fval_up(id);}
    const std::string& get_sval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : get_sval_up(id);}
    const module_value& get_lval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : get_lval_up(id);}    
    const varbase& get_varbase_up(const int id) const;
    fint get_ival_up(const int id) const;
    rat get_rval_up(const int id) const;
    ffloat get_fval_up(const int id) const;
    const std::string& get_sval_up(const int id) const;
    const module_value& get_lval_up(const int id) const;
    const char* gettype() const {return "a clef";}
    void getclefsaux(std::set<int>& st) const {st.insert(strtoclef(get_sval(CLEF_ID)));} // collect clef
    bool getclefptr(const int clef) const {return strtoclef(get_sval(CLEF_ID)) == clef;}
    const char* getdefclef() const {return get_sval(CLEF_ID).c_str();}
  };
  inline void clef_addstrconfrule(boost::shared_ptr<clef_str>& shthis, boostspirit::symbols<parserule*>& syms, std::vector<parserule> &ret, confscratch& x) {
    addstrconfrule(shthis, syms, ret, x, module_locclef, "a clef");
  }

  struct clefvalidid:public errbase {
  };

  // staves
  struct staff_str:public str_base NONCOPYABLE { // input holdergetclefsget
  private:
    const instr_str* par;
    std::vector<boost::shared_ptr<clef_str> > clefs;
    std::vector<int> clefscache;
  public:
    staff_str():str_base(), par(0) {}
    staff_str(const std::string& cl):str_base(), par(0) {
      clefs.push_back(boost::shared_ptr<clef_str>(new clef_str(cl)));
    }
    void insclef(boost::shared_ptr<clef_str>& x) {x->setpar(this); clefs.push_back(boost::shared_ptr<clef_str>(x)); x.reset(new clef_str());}
    void complete(const fomusdata& fd) {}
    void complete(const filepos& pos) {}
    void setpar(const instr_str* ptr) {par = ptr;}
    void getmodval(module_value& x) const;
    void getwholemodval(module_value& x) const {getmodval(x);}
    const char* getprintstr() const;
    void print(std::ostream& s, const fomusdata* fd) const;
    const varbase& get_varbase(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : get_varbase_up(id);}
    fint get_ival(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : get_ival_up(id);}
    rat get_rval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : get_rval_up(id);}
    ffloat get_fval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : get_fval_up(id);}
    const std::string& get_sval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : get_sval_up(id);}
    const module_value& get_lval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : get_lval_up(id);}    
    const varbase& get_varbase_up(const int id) const;
    fint get_ival_up(const int id) const;
    rat get_rval_up(const int id) const;
    ffloat get_fval_up(const int id) const;
    const std::string& get_sval_up(const int id) const;
    const module_value& get_lval_up(const int id) const;
    const char* gettype() const {return "a staff";}
    void cachesinit(std::set<int>& st);
    struct module_intslist getclefs() { // get clefs from this structure
      module_intslist l;
      l.n = clefscache.size();
      l.ints = &clefscache[0];
      return l;
    }
    bool gethasclef(const int clef) {
      return std::binary_search(clefscache.begin(), clefscache.end(), clef);
    }
    clef_str* getclefptr(const int clef) const;
    const char* getdefclef() const {
      assert(!clefs.empty());
      return clefs.front()->getdefclef();
    }
    modobjbase* getpartclefobj(const int clef) const {
      clef_str* r = getclefptr(clef);
      if (r) return r;
      throw clefvalidid();
    }
  };
  inline void staff_addstrconfrule(boost::shared_ptr<staff_str>& shthis, boostspirit::symbols<parserule*>& syms, std::vector<parserule> &ret, confscratch& x) {
    addstrconfrule(shthis, syms, ret, x, module_locstaff, "a staff");
  }
  inline const varbase& clef_str::get_varbase_up(const int id) const {return par->get_varbase(id);}
  inline fint clef_str::get_ival_up(const int id) const {return par->get_ival(id);}
  inline rat clef_str::get_rval_up(const int id) const {return par->get_rval(id);}
  inline ffloat clef_str::get_fval_up(const int id) const {return par->get_fval(id);}
  inline const std::string& clef_str::get_sval_up(const int id) const {return par->get_sval(id);}
  inline const module_value& clef_str::get_lval_up(const int id) const {return par->get_lval(id);}

  // percussion strument
  class fomusdata;
  struct percinstr_str:public instrorpercinstr_str NONCOPYABLE { // + marks?
  private:
    const instr_str* par;
    std::string basedon;
    std::string id; 
    std::vector<boost::shared_ptr<import_str> > ims; bool imsmod;
    boost::shared_ptr<export_str> ex; // mod if ptr != 0
    std::vector<modobjbase*> impscache;
  public:
    percinstr_str():instrorpercinstr_str(), par(0), /*namemod(false), abbrmod(false),*/ imsmod(false) {}
    percinstr_str(const percinstr_str& x, const instr_str* par):instrorpercinstr_str(x), par(par), id(x.id), 
								ims(x.ims), imsmod(x.imsmod), ex(x.ex) {}
    boost::shared_ptr<percinstr_str> copy(const instr_str* newpar) const {return boost::shared_ptr<percinstr_str>(new percinstr_str(*this, newpar));}
    void replace(percinstr_str& x) {
      id = x.id;
      ims.assign(x.ims.begin(), x.ims.end()); imsmod = x.imsmod;
      ex = x.ex;
      str_base::replace(x);
    }
    void cacheimports() {
      std::transform(ims.begin(), ims.end(), std::back_inserter(impscache), boost::lambda::bind(&boost::shared_ptr<import_str>::get, boost::lambda::_1));
    }
    void grabimports(std::vector<modobjbase*>& imps) {
      if (impscache.empty()) cacheimports();
      std::transform(impscache.begin(), impscache.end(), std::back_inserter(imps), boost::lambda::_1);
    }
    struct modin_imports getimports() {
      if (impscache.empty()) cacheimports();
      struct modin_imports r = {impscache.size(), (module_obj*)&impscache[0]};
      return r;
    }
    module_obj getexport() {return ex.get();}
    void setbasedon(const std::string& str) {basedon = str;}
    void setid(const std::string& str) {id = str;}
    void insimport(boost::shared_ptr<import_str>& x) {x->setpar(this); ims.push_back(boost::shared_ptr<import_str>(x)); imsmod = true; x.reset(new import_str());}
    void setexport(boost::shared_ptr<export_str>& x) {x->setpar(this); ex = x; x.reset(new export_str());}
    const std::string& getid() const {return id;}
    const char* getcid() const {return id.c_str();}
    void completeaux(const percinstr_str& b);
    void complete(fomusdata& fd);
    void complete(const filepos& pos);
    void setpar(const instr_str* ptr) {par = ptr; DBG("par=" << par << std::endl);}
    void getmodval(module_value& x, const bool justid) const;
    void getwholemodval(module_value& x) const {getmodval(x, false);}
    const char* getprintstr() const;
    void print(std::ostream& s, const fomusdata* fd, const bool justid) const;
    std::string structtypestr() const {return "percussion instrument";}
    const varbase& get_varbase(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : get_varbase_up(id);}
    fint get_ival(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : get_ival_up(id);}
    rat get_rval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : get_rval_up(id);}
    ffloat get_fval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : get_fval_up(id);}
    const std::string& get_sval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : get_sval_up(id);}
    const module_value& get_lval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : get_lval_up(id);}    
    const varbase& get_varbase_up(const int id) const;
    fint get_ival_up(const int id) const;
    rat get_rval_up(const int id) const;
    ffloat get_fval_up(const int id) const;
    const std::string& get_sval_up(const int id) const;
    const module_value& get_lval_up(const int id) const;
    const char* gettype() const {return "a percussion instrument";}
    percinstr_str* findpercinsts(const char* na) {return boost::algorithm::iequals(id, na) ? this : 0;}
    void getlastentry(info_objinfo& str, const fomusdata& fd) const;
    void clearid() {id.clear();}
    const char* getimportpercid() const {assert(isvalid()); return id.c_str();}
  };
  inline void percinstr_addstrconfrule(boost::shared_ptr<percinstr_str>& shthis, boostspirit::symbols<parserule*>& syms, std::vector<parserule> &ret, confscratch& x) {
    addstrconfrule(shthis, syms, ret, x, module_locpercinst, "a percussion instrument");
  }

  struct grabimports:public boost::static_visitor<void> {
    std::vector<modobjbase*>& imps;
    grabimports(std::vector<modobjbase*>& imps):imps(imps) {}
    void operator()(const boost::shared_ptr<percinstr_str>& x) const {return x->grabimports(imps);}
    void operator()(const std::string& x) const {assert(false);}
  };

  //#warning "make sure all error structs are based on errbase"
  struct staffrange:public errbase {
  };
  
  // instruments
  struct copypercinstr:public boost::static_visitor<boost::shared_ptr<percinstr_str> > {
    const instr_str* newpar;
    copypercinstr(const instr_str* newpar):newpar(newpar) {}
    boost::shared_ptr<percinstr_str> operator()(const boost::shared_ptr<percinstr_str>& x) const {return x->copy(newpar);}
    boost::shared_ptr<percinstr_str> operator()(const std::string& x) const {assert(false);}
  };
  
  struct part_str;
  class event;
  class measdef_str;
  struct instr_str:public instrorpercinstr_str NONCOPYABLE {
  private:
    const part_str* par;
    std::string basedon;
    std::string id;
    std::vector<boost::shared_ptr<staff_str> > staves; bool stavesmod;
    std::vector<boost::shared_ptr<import_str> > ims; bool imsmod;
    boost::shared_ptr<export_str> ex; 
    std::vector<boost::variant<boost::shared_ptr<percinstr_str>, std::string> > percs; bool percsmod;
    std::vector<modobjbase*> impscache;
  public:
    instr_str():instrorpercinstr_str(), par(0), stavesmod(false), imsmod(false), percsmod(false) {}
    instr_str(const std::string& id):instrorpercinstr_str(), par(0), id(id), stavesmod(false), imsmod(false), percsmod(false) {
      staves.push_back(boost::shared_ptr<staff_str>(new staff_str("treble")));
      staves.push_back(boost::shared_ptr<staff_str>(new staff_str("bass")));
    }
    instr_str(const instr_str& x, const part_str* par):instrorpercinstr_str(x), par(par), id(x.id), 
						       staves(x.staves), stavesmod(x.stavesmod), ims(x.ims), imsmod(x.imsmod),
						       ex(x.ex), percsmod(x.percsmod) {
      for (std::vector<boost::variant<boost::shared_ptr<percinstr_str>, std::string> >::const_iterator i(x.percs.begin()); i != x.percs.end(); ++i) {
	percs.push_back(boost::apply_visitor(copypercinstr(this), *i));
      }
    }
    boost::shared_ptr<instr_str> copy(const part_str* newpar) const {return boost::shared_ptr<instr_str>(new instr_str(*this, newpar));}
    void replace(instr_str& x);
    void setbasedon(const std::string& str) {basedon = str;}
    void setid(const std::string& str) {id = str;}
    void insstaff(boost::shared_ptr<staff_str>& x) {x->setpar(this); staves.push_back(boost::shared_ptr<staff_str>(x)); stavesmod = true; x.reset(new staff_str());}
    void insimport(boost::shared_ptr<import_str>& x) {x->setpar(this); ims.push_back(boost::shared_ptr<import_str>(x)); imsmod = true; x.reset(new import_str());}
    void setexport(boost::shared_ptr<export_str>& x) {x->setpar(this); ex = x; x.reset(new export_str());}
    void inspercinstrid(const std::string& str) {percs.push_back(str); percsmod = true;}
    void inspercinstrstr(boost::shared_ptr<percinstr_str>& x) {x->setpar(this); percs.push_back(boost::shared_ptr<percinstr_str>(x)); percsmod = true; x.reset(new percinstr_str(/*fom*/));}
    const std::string& getid() const {return id;}
    const char* getcid() const {return id.c_str();}
    void completeaux(const instr_str& b);
    void complete(fomusdata& fd);
    void complete(const filepos& pos);
    void setpar(const part_str* ptr) {par = ptr;}
    void getmodval(module_value& x, const bool justid) const;
    void getwholemodval(module_value& x) const {getmodval(x, false);}
    const char* getprintstr() const;
    void print(std::ostream& s, const fomusdata* fd, const bool justid) const;
    std::string structtypestr() const {return "instrument";}
    const varbase& get_varbase(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : fom_get_varbase_up(id);}
    fint get_ival(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : fom_get_ival_up(id);}
    rat get_rval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : fom_get_rval_up(id);}
    ffloat get_fval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : fom_get_fval_up(id);}
    const std::string& get_sval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : fom_get_sval_up(id);}
    const module_value& get_lval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : fom_get_lval_up(id);}
    const varbase& get_varbase(const int id, const event& ev) const;
    fint get_ival(const int id, const event& ev) const;
    rat get_rval(const int id, const event& ev) const;
    ffloat get_fval(const int id, const event& ev) const;
    const std::string& get_sval(const int id, const event& ev) const;
    const module_value& get_lval(const int id, const event& ev) const;
    const char* gettype() const {return "an instrument";}
    struct module_intslist getclefs(int st) {
      if (st < 1 || st > (int)staves.size()) throw staffrange();
      return staves[st - 1]->getclefs();
    }
    bool gethasclef(const int clef, const int st) {
      if (st < 1 || st > (int)staves.size()) throw staffrange();
      return staves[st - 1]->gethasclef(clef);
    }
    staff_str* getstaffptr(const int staff) const {
      assert(!(staff < 1 || staff > (int)staves.size()));
      return staves[staff - 1].get();
    }
    modobjbase* getpartstaffobj(const int staff) const {
      if (staff < 1 || staff > (int)staves.size()) throw staffrange();
      return staves[staff - 1].get();
    }
    modobjbase* getpartclefobj(const int staff, const int clef) const {
      if (staff < 1 || staff > (int)staves.size()) throw staffrange();
      return staves[staff - 1]->getpartclefobj(clef);
    }
    clef_str* getclefptr(const int staff, const int clef) const {
      assert(!(staff < 1 || staff > (int)staves.size()));
      return staves[staff - 1]->getclefptr(clef);
    }
    void cachesinit(std::set<int>& st) {
      //DBG("CACHESINIT" << std::endl);
      std::for_each(staves.begin(), staves.end(),
		    boost::lambda::bind(&staff_str::cachesinit, boost::lambda::bind(&boost::shared_ptr<staff_str>::get, boost::lambda::_1), boost::lambda::var(st)));
      if (impscache.empty()) cacheimports();      
    }
    void cacheimports() {
      for (std::vector<boost::variant<boost::shared_ptr<percinstr_str>, std::string> >::const_iterator i(percs.begin()); i != percs.end(); ++i) {
	boost::apply_visitor(grabimports(impscache), *i);
      }
      std::transform(ims.begin(), ims.end(), std::back_inserter(impscache), boost::lambda::bind(&boost::shared_ptr<import_str>::get, boost::lambda::_1));
    }
    struct modin_imports getimports() {
      if (impscache.empty()) cacheimports();      
      struct modin_imports r = {impscache.size(), (module_obj*)&impscache[0]};
      return r;
    }
    module_obj getexport() {return ex.get();}
    int getnstaves() const {return staves.size();}
    const char* getdefclef(const int staff) const {
      if (staff < 1 || staff > (int)staves.size()) throw staffrange();
      return staves[staff - 1]->getdefclef();
    }
    percinstr_str* findpercinsts(const char* na) const;
    void getlastentry(info_objinfo& str, const fomusdata& fd) const;
    void clearid() {id.clear();}
    const char* getimportpercid() const {assert(isvalid()); return 0;}
  };

  struct percinstr_isid:public boost::static_visitor<percinstr_str*> {
    const char* na;
    percinstr_isid(const char* na):na(na) {}
    percinstr_str* operator()(const boost::shared_ptr<percinstr_str>& x) const {return x->findpercinsts(na);}
    percinstr_str* operator()(const std::string& x) const {assert(false);}
  };  
  
  inline void instr_addstrconfrule(boost::shared_ptr<instr_str>& shthis, boostspirit::symbols<parserule*>& syms, std::vector<parserule> &ret, confscratch& x) {
    addstrconfrule(shthis, syms, ret, x, module_locinst, "an instrument");
  }
  inline const varbase& staff_str::get_varbase_up(const int id) const {return par->get_varbase(id);}
  inline fint staff_str::get_ival_up(const int id) const {return par->get_ival(id);}
  inline rat staff_str::get_rval_up(const int id) const {return par->get_rval(id);}
  inline ffloat staff_str::get_fval_up(const int id) const {return par->get_fval(id);}
  inline const std::string& staff_str::get_sval_up(const int id) const {return par->get_sval(id);}
  inline const module_value& staff_str::get_lval_up(const int id) const {return par->get_lval(id);}
  inline const varbase& percinstr_str::get_varbase_up(const int id) const {return par->get_varbase(id);}
  inline fint percinstr_str::get_ival_up(const int id) const {return par->get_ival(id);}
  inline rat percinstr_str::get_rval_up(const int id) const {return par->get_rval(id);}
  inline ffloat percinstr_str::get_fval_up(const int id) const {return par->get_fval(id);}
  inline const std::string& percinstr_str::get_sval_up(const int id) const {return par->get_sval(id);}
  inline const module_value& percinstr_str::get_lval_up(const int id) const {return par->get_lval(id);}

  // parts
  class part;
  struct copyinstr:public boost::static_visitor<boost::shared_ptr<instr_str> > {
    const part_str* newpar;
    copyinstr(const part_str* newpar):newpar(newpar) {}
    boost::shared_ptr<instr_str> operator()(const boost::shared_ptr<instr_str>& x) const {return x->copy(/*fom,*/ newpar);}
    boost::shared_ptr<instr_str> operator()(const std::string& x) const {assert(false);}
  };
  struct instr_get_varbase:public boost::static_visitor<const varbase&> {
    const int id;
    instr_get_varbase(const int id):id(id) {}
    const varbase& operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_varbase(id);}
    const varbase& operator()(const std::string& x) const {assert(false);}
  };
  struct instr_get_ival:public boost::static_visitor<fint> {
    const int id;
    instr_get_ival(const int id):id(id) {}
    fint operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_ival(id);}
    fint operator()(const std::string& x) const {assert(false);}
  };
  struct instr_get_rval:public boost::static_visitor<rat> {
    const int id;
    instr_get_rval(const int id):id(id) {}
    rat operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_rval(id);}
    rat operator()(const std::string& x) const {assert(false);}
  };
  struct instr_get_fval:public boost::static_visitor<ffloat> {
    const int id;
    instr_get_fval(const int id):id(id) {}
    ffloat operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_fval(id);}
    ffloat operator()(const std::string& x) const {assert(false);}
  };
  struct instr_get_sval:public boost::static_visitor<const std::string&> {
    const int id;
    instr_get_sval(const int id):id(id) {}
    const std::string& operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_sval(id);}
    const std::string& operator()(const std::string& x) const {assert(false);}
  };
  struct instr_get_lval:public boost::static_visitor<const module_value&> {
    const int id;
    instr_get_lval(const int id):id(id) {}
    const module_value& operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_lval(id);}
    const module_value& operator()(const std::string& x) const {assert(false);}
  };
  struct instr_get_ptr:public boost::static_visitor<instr_str*> {
    instr_str* operator()(const boost::shared_ptr<instr_str>& x) const {return x.get();}
    instr_str* operator()(const std::string& x) const {assert(false);}
  };
  struct instr_get_nstaves:public boost::static_visitor<int> {
    int operator()(const boost::shared_ptr<instr_str>& x) const {return x->getnstaves();}
    int operator()(const std::string& x) const {assert(false);}
  };

  struct instr_evget_varbase:public boost::static_visitor<const varbase&> {
    const int id;
    const event& ev;
    instr_evget_varbase(const int id, const event& ev):id(id), ev(ev) {}
    const varbase& operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_varbase(id, ev);}
    const varbase& operator()(const std::string& x) const {assert(false);}
  };
  struct instr_evget_ival:public boost::static_visitor<fint> {
    const int id;
    const event& ev;
    instr_evget_ival(const int id, const event& ev):id(id), ev(ev) {}
    fint operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_ival(id, ev);}
    fint operator()(const std::string& x) const {assert(false);}
  };
  struct instr_evget_rval:public boost::static_visitor<rat> {
    const int id;
    const event& ev;
    instr_evget_rval(const int id, const event& ev):id(id), ev(ev) {}
    rat operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_rval(id, ev);}
    rat operator()(const std::string& x) const {assert(false);}
  };
  struct instr_evget_fval:public boost::static_visitor<ffloat> {
    const int id;
    const event& ev;
    instr_evget_fval(const int id, const event& ev):id(id), ev(ev) {}
    ffloat operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_fval(id, ev);}
    ffloat operator()(const std::string& x) const {assert(false);}
  };
  struct instr_evget_sval:public boost::static_visitor<const std::string&> {
    const int id;
    const event& ev;
    instr_evget_sval(const int id, const event& ev):id(id), ev(ev) {}
    const std::string& operator()(const boost::shared_ptr<instr_str>& x) const {return x->get_sval(id, ev);}
    const std::string& operator()(const std::string& x) const {assert(false);}
  };
  struct instr_evget_lval:public boost::static_visitor<const module_value&> {
    const int id;
    const event& ev;
    instr_evget_lval(const int id, const event& ev):id(id), ev(ev) {}
    const module_value& operator()(const boost::shared_ptr<instr_str>& x) const {
      return x->get_lval(id, ev);
    }
    const module_value& operator()(const std::string& x) const {assert(false);}
  };

  struct instr_cachesinit:public boost::static_visitor<void> {
    std::set<int>& c;
    instr_cachesinit(std::set<int>& c):c(c) {}
    void operator()(const boost::shared_ptr<instr_str>& x) const {assert(x.get()); return x->cachesinit(c);}
    void operator()(const std::string& x) const {assert(false);}
  };
  struct instr_getclefs1:public boost::static_visitor<module_intslist> {
    const int c;
    instr_getclefs1(const int c):c(c) {}
    module_intslist operator()(const boost::shared_ptr<instr_str>& x) const {return x->getclefs(c);}
    module_intslist operator()(const std::string& x) const {assert(false);}
  };
  struct instr_gethasclef2:public boost::static_visitor<bool> {
    const int c, s;
    instr_gethasclef2(const int c, const int s):c(c), s(s) {}
    bool operator()(const boost::shared_ptr<instr_str>& x) const {return x->gethasclef(c, s);}
    bool operator()(const std::string& x) const {assert(false);}
  };
  struct instr_getstaffptr:public boost::static_visitor<staff_str*> {
    const int s;
    instr_getstaffptr(const int s):s(s) {}
    staff_str* operator()(const boost::shared_ptr<instr_str>& x) const {return x->getstaffptr(s);}
    staff_str* operator()(const std::string& x) const {assert(false);}
  };
  struct instr_getpartstaffobj:public boost::static_visitor<modobjbase*> {
    const int s;
    instr_getpartstaffobj(const int s):s(s) {}
    modobjbase* operator()(const boost::shared_ptr<instr_str>& x) const {return x->getpartstaffobj(s);}
    modobjbase* operator()(const std::string& x) const {assert(false);}
  };
  struct instr_getpartclefobj:public boost::static_visitor<modobjbase*> {
    const int s, c;
    instr_getpartclefobj(const int s, const int c):s(s), c(c) {}
    modobjbase* operator()(const boost::shared_ptr<instr_str>& x) const {return x->getpartclefobj(s, c);}
    modobjbase* operator()(const std::string& x) const {assert(false);}
  };
  struct instr_getclefptr:public boost::static_visitor<clef_str*> {
    const int s, c;
    instr_getclefptr(const int s, const int c):s(s), c(c) {}
    clef_str* operator()(const boost::shared_ptr<instr_str>& x) const {return x->getclefptr(s, c);}
    clef_str* operator()(const std::string& x) const {assert(false);}
  };
  struct instr_getdefclef:public boost::static_visitor<const char*> {
    const int s;
    instr_getdefclef(const int s):s(s) {}
    const char* operator()(const boost::shared_ptr<instr_str>& x) const {return x->getdefclef(s);}
    const char* operator()(const std::string& x) const {assert(false);}
  };
  struct instr_findpercinst:public boost::static_visitor<percinstr_str*> {
    const char* na;
    instr_findpercinst(const char* na):na(na) {}
    percinstr_str* operator()(const boost::shared_ptr<instr_str>& x) const {return x->findpercinsts(na);}
    percinstr_str* operator()(const std::string& x) const {assert(false);}
  };
  struct instr_getimports:public boost::static_visitor<struct modin_imports> {
    struct modin_imports operator()(const boost::shared_ptr<instr_str>& x) const {return x->getimports();}
    struct modin_imports operator()(const std::string& x) const {assert(false);}
  };
  struct instr_getexport:public boost::static_visitor<module_obj> {
    module_obj operator()(const boost::shared_ptr<instr_str>& x) const {return x->getexport();}
    module_obj operator()(const std::string& x) const {assert(false);}
  };
  
  struct makemeas {
    filepos pos;
    numb off, dur;
    boost::shared_ptr<measdef_str> measdef;
    makemeas(const filepos& pos, const numb& off, const numb& dur, boost::shared_ptr<measdef_str>& measdef):pos(pos), off(off), dur(dur), measdef(measdef) {}
    const measdef_str& getmeasdef() const {return *measdef.get();}
  };

  class markev;
  class partormpart_str:public str_base {
  public:
    scorepartlist_it self;
    boost::shared_ptr<part> prt; // make new ones as user selects them
    int ind;
    int insord;
    partormpart_str();
    partormpart_str(const partormpart_str& x);
    partormpart_str(const partormpart_str& x, const numb& shift); // this one's for fomcloning
    virtual ~partormpart_str() {}
    void assigngroupbegin(const parts_grouptype type);
    void assigngroupend(const parts_grouptype type);
    void assignorder(const int ord, const bool tmark) {ind = ord; settmark(tmark);}    
    void settmark(const bool tmark);
    void settmark();
    const part& getpart() const {return *prt;}
    part& getpart() {return *prt;}
    part* getpartptr() {return prt.get();}
    void setself(const scorepartlist_it& s, const fint ind0) {self = s; ind = ind0;}
    void setself(const scorepartlist_it& s) {self = s;}
    const scorepartlist_it& getpartselfit() const {return self;}
    void fixmeasures();
    numb fixtimequant();
    void fixtimequantinv();
    void reinsert(std::auto_ptr<noteevbase>& e, const char* what);
    void trimmeasures(const fomus_rat& n);
    bool haspart() const {return prt.get();}
    virtual const varbase& get_varbase(const int id) const {assert(false);} // redeclarations
    virtual fint get_ival(const int id) const {assert(false);} // redeclarations
    virtual rat get_rval(const int id) const {assert(false);}
    virtual ffloat get_fval(const int id) const {assert(false);}
    virtual const std::string& get_sval(const int id) const {assert(false);}
    virtual const module_value& get_lval(const int id) const {assert(false);}
    virtual const varbase& get_varbase(const int id, const event& ev) const {assert(false);}
    virtual fint get_ival(const int id, const event& ev) const {assert(false);}
    virtual rat get_rval(const int id, const event& ev) const {assert(false);}
    virtual ffloat get_fval(const int id, const event& ev) const {assert(false);}
    virtual const std::string& get_sval(const int id, const event& ev) const {assert(false);}
    virtual const module_value& get_lval(const int id, const event& ev) const {assert(false);}
    const char* gettype() const {return "a part";}
    modobjbase* getpartobj() {return this;}
    modobjbase* getinstobj() {return 0;} // if no instrument, return 0
    fint index() const {return ind;}
    virtual staff_str* getstaffptr(const int staff) const {assert(false);}
    virtual clef_str* getclefptr(const int staff, const int clef) const {assert(false);}
    void postinput(std::vector<makemeas>& makemeass, const int ord, const numb& trun1, const numb& trun2);
    void collectallvoices(std::set<int>& v);
    void collectallstaves(std::set<int>& s);
#ifndef NDEBUG    
    void resetstage();
#endif
#ifndef NDEBUGOUT
    void showstage();
#endif    
    struct module_intslist getvoices();
    struct module_intslist getstaves();
    virtual void cachesinit() {assert(false);}
    virtual const std::string& getid() const {assert(false);}
    virtual int getnstaves() const {return -1;}
    void filltmppart(measmapview& m) const;
    enum parts_grouptype partgroupbegin(const int lvl) const;
    bool partgroupend(const int lvl) const;
    void sortord(fomus_int& i);
    void fillholes1();
    void fillholes2();
    void fillholes3();
    const std::vector<int>& getvoicescache() const;
    const std::vector<int>& getstavescache() const;
    void assignmpart(std::auto_ptr<noteevbase>& pa);
    void assignmpartmarkev(std::auto_ptr<markev>& ev);
    void postinput3();
#ifndef NDEBUGOUT
    void dumpall() const;
#endif
    bool ismetapart() const {return false;}
    virtual percinstr_str* findpercinst(const char* name) const {return 0;}
    void insbgroups(const int lvl, const parts_grouptype typ);
    void insegroups(const int lvl);
    numb getlastmeasendoff() const;
    module_objlist getmarkevslist();
    void /*eventmap_it*/ insertfiller();
    void /*eventmap_it*/ preprocess(const numb& te, const std::string& ts);
    void deletefiller(/*eventmap_it& it*/);
    void insertnew(noteevbase* ev); // OVERRIDE THIS FOR PARTSREF!!?
    void insertnewmarkev(markev* ev);
    void clearallnotes();
    void assigndetmark(const numb& off, const int voice, const int type, const char* arg1, const struct module_value& arg2);
    void inserttmps();
    bool getoctavebegin(const measmap_it& me, const offgroff& ti, const int st) const;
    bool getoctaveend(const measmap_it& me, const offgroff& ti, const int st) const;
    bool gettmark() const;
    void insertnewmeas(const offgroff& o, measure* m);
    void reinserttmps();
    void mergefrom(part& x, const numb& shift);
    void insdefmeas(boost::shared_ptr<measdef_str>& ms);
    //void postmeas();
  };
  inline bool operator<(const boost::shared_ptr<partormpart_str>& x, const boost::shared_ptr<partormpart_str>& y) {return x->ind < y->ind;}
  inline bool operator<(const partormpart_str& x, const partormpart_str& y) {return x.ind < y.ind;}
  
#ifndef NDEBUG  
  class instr_assert:public boost::static_visitor<void> {
  public:
    void operator()(const std::string& x) const {assert(false);}
    void operator()(const boost::shared_ptr<instr_str>& x) const {assert(x->isvalid()); DBG("ADDR=" << x.get() << std::endl);} 
  };
#endif  

  struct partmap_str;
  struct part_str:public partormpart_str {
  private:
    std::string basedon;
    std::string id;
    boost::variant<boost::shared_ptr<instr_str>, std::string> instr; // modified if pointer != 0
    std::vector<int> clefscache;
  public:
    part_str():partormpart_str() {}
    part_str(const std::string& id):partormpart_str(), id(id) {}
    part_str(const part_str& x, const partmap_str* par):partormpart_str(x), id(x.id), instr(boost::apply_visitor(copyinstr(this), x.instr)) {}
    part_str(const part_str& x, const numb& shift):partormpart_str(x, shift), id(x.id), instr(x.instr) {} // should only be called from fomclone
    boost::shared_ptr<part_str> copy(const partmap_str* newpar) const {return boost::shared_ptr<part_str>(new part_str(*this, newpar));}
    void replace(part_str& x) {
      id = x.id;
      instr = x.instr;
      str_base::replace(x);
    }
    part_str* fomclone(const numb& shift = module_none) {assert(isvalid()); return new part_str(*this, shift);}
    void setbasedon(const std::string& str) {basedon = str;}  
    void setid(const std::string& str) {id = str;}
    void setinstrid(const std::string& str) {instr = str;}
    void setinstrstr(boost::shared_ptr<instr_str>& x) {x->setpar(this); instr = boost::shared_ptr<instr_str>(x); x.reset(new instr_str());}
    const std::string& getid() const {return id;}
    const char* getcid() const {return id.c_str();}
    void complete(fomusdata& fd);
    void getmodval(module_value& x, const bool justid) const;
    void getwholemodval(module_value& x) const {getmodval(x, false);}
    const char* getprintstr() const;
    void print(std::ostream& s, const fomusdata* fd, const bool justid) const;

    staff_str* getstaffptr(const int staff) const {return boost::apply_visitor(instr_getstaffptr(staff), instr);} // this doesn't do a bounds check
    modobjbase* getpartstaffobj(const int staff) const {return boost::apply_visitor(instr_getpartstaffobj(staff), instr);} // this does a bounds check
    modobjbase* getpartclefobj(const int staff, const int clef) const {return boost::apply_visitor(instr_getpartclefobj(staff, clef), instr);} // this does a bounds check
    clef_str* getclefptr(const int staff, const int clef) const {return boost::apply_visitor(instr_getclefptr(staff, clef), instr);}
    const char* getdefclef(const int staff) const {return boost::apply_visitor(instr_getdefclef(staff), instr);}

    partormpart_str* getbasethis() {return this;}
    const partormpart_str* getbasethis() const {return this;}
    
    const varbase& get_varbase(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : boost::apply_visitor(instr_evget_varbase(id, ev), instr);}
    fint get_ival(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : boost::apply_visitor(instr_evget_ival(id, ev), instr);}
    rat get_rval(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : boost::apply_visitor(instr_evget_rval(id, ev), instr);}
    ffloat get_fval(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : boost::apply_visitor(instr_evget_fval(id, ev), instr);}
    const std::string& get_sval(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : boost::apply_visitor(instr_evget_sval(id, ev), instr);}
    const module_value& get_lval(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : boost::apply_visitor(instr_evget_lval(id, ev), instr);}
    const varbase& get_varbase(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : get_varbase_up(id);}
    fint get_ival(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : get_ival_up(id);}
    rat get_rval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : get_rval_up(id);}
    ffloat get_fval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : get_fval_up(id);}
    const std::string& get_sval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : get_sval_up(id);}
    const module_value& get_lval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : get_lval_up(id);}    
    const varbase& get_varbase_up(const int id) const {return boost::apply_visitor(instr_get_varbase(id), instr);}
    fint get_ival_up(const int id) const {return boost::apply_visitor(instr_get_ival(id), instr);}
    rat get_rval_up(const int id) const {return boost::apply_visitor(instr_get_rval(id), instr);}
    ffloat get_fval_up(const int id) const {return boost::apply_visitor(instr_get_fval(id), instr);}
    const std::string& get_sval_up(const int id) const {return boost::apply_visitor(instr_get_sval(id), instr);}
    const module_value& get_lval_up(const int id) const {return boost::apply_visitor(instr_get_lval(id), instr);}
    modobjbase* getinstobj() {return boost::apply_visitor(instr_get_ptr(), instr);}    
    void cachesinit() {
#ifndef NDEBUG
      boost::apply_visitor(instr_assert(), instr);
#endif    
      std::set<int> c;
      boost::apply_visitor(instr_cachesinit(c), instr);
      clefscache.assign(c.begin(), c.end());
    }
    struct modin_imports getimports() {return boost::apply_visitor(instr_getimports(), instr);}
    module_obj getexport() {return boost::apply_visitor(instr_getexport(), instr);}
    void cachesinitformpart(std::set<int>& c) {boost::apply_visitor(instr_cachesinit(c), instr);}
    struct module_intslist getclefs() {
      module_intslist l;
      l.n = clefscache.size();
      l.ints = &clefscache[0];
      return l;
    }
    struct module_intslist getclefs(int st) {return boost::apply_visitor(instr_getclefs1(st), instr);}
    bool gethasclef(const int clef);
    bool gethasclef(const int clef, const int staff) {return boost::apply_visitor(instr_gethasclef2(clef, staff), instr);}
    int getnstaves() const {return boost::apply_visitor(instr_get_nstaves(), instr);}
//     percnotes_percinsts getpercinsts() const {return boost::apply_visitor(instr_getpercinsts(), instr);}
    percinstr_str* findpercinst(const char* name) const {return boost::apply_visitor(instr_findpercinst(name), instr);}
    void getlastentry(info_objinfo& str, const fomusdata& fd) const;
    void clearid() {id.clear();}
  };

  struct mpart_str;  
  struct cachesinitformpartvis:public boost::static_visitor<void> {
    std::set<int>& c;
    cachesinitformpartvis(std::set<int>& c):c(c) {}
    void operator()(const boost::shared_ptr<part_str>& x) const {return x->cachesinitformpart(c);} // already completed mparts can't include the current one
    void operator()(const boost::shared_ptr<mpart_str>& x) const;
    void operator()(const std::string& x) const {assert(false);}
  };
  
  // (metapart def)
  class contains_mpart:public boost::static_visitor<bool> {
    const mpart_str* str;
  public:
    contains_mpart(const mpart_str* str):str(str) {}
    bool operator()(const boost::shared_ptr<part_str>& x) const {return false;} // already completed mparts can't include the current one
    bool operator()(const boost::shared_ptr<mpart_str>& x) const;
    bool operator()(const std::string& x) const {assert(false);}
  };
  class getthepart:public boost::static_visitor<modobjbase*> {
  public:
    modobjbase* operator()(const boost::shared_ptr<part_str>& x) const {return x.get();} // already completed mparts can't include the current one
    modobjbase* operator()(const boost::shared_ptr<mpart_str>& x) const {return (partormpart_str*)x.get();}
    modobjbase* operator()(const std::string& x) const {assert(false);}
  };
  
  class clonepartmpart:public boost::static_visitor<partmap_str*> {
    partmap_str& x;
    std::map<part_str*, boost::shared_ptr<part_str> >& cl;
    std::map<mpart_str*, boost::shared_ptr<mpart_str> >& mcl;
    const numb& shift;
  public:
    clonepartmpart(partmap_str& x, std::map<part_str*, boost::shared_ptr<part_str> >& cl, std::map<mpart_str*, boost::shared_ptr<mpart_str> >& mcl,
		   const numb& shift):x(x), cl(cl), mcl(mcl), shift(shift) {}
    partmap_str* operator()(const boost::shared_ptr<part_str>& x) const;
    partmap_str* operator()(const boost::shared_ptr<mpart_str>& x) const;
    partmap_str* operator()(const std::string& x) const {assert(false);}
  };

  struct partmap_str:public str_base NONCOPYABLE { // no templates
  private:
    boost::variant<boost::shared_ptr<part_str>, boost::shared_ptr<mpart_str>, std::string> part;
  public:
    partmap_str():str_base() /*, par(0)*/ {}
    partmap_str(boost::shared_ptr<part_str>& np):str_base(), part(np) {} // for ALL mpart
    partmap_str(partmap_str& x):str_base(x), part(x.part) {} // fomcloning
    partmap_str(partmap_str& x, boost::shared_ptr<part_str>& np):str_base(x), part(np) {} // fomcloning
    //partmap_str(partmap_str& x, mpart_str* p):str_base(x), part(boost::shared_ptr<mpart_str>(p)) {}
    partmap_str(partmap_str& x, boost::shared_ptr<fomus::mpart_str>& p):str_base(x), part(p) {}
    void setpartid(const std::string& str) {part = str;}
    void setpartstr(boost::shared_ptr<part_str>& x) {/*x->setpar(this);*/ part = boost::shared_ptr<part_str>(x); x.reset(new part_str(/*fom*/));}
    void setmpartid(const std::string& str) {part = str;}
    void setmpartstr(boost::shared_ptr<mpart_str>& x);
    partmap_str* fomclone(std::map<part_str*, boost::shared_ptr<part_str> >& cl, std::map<mpart_str*, boost::shared_ptr<mpart_str> >& mcl,
			  const numb& shift) {
      assert(isvalid());
      DBG("cloning partmap" << std::endl);
      return apply_visitor(clonepartmpart(*this, cl, mcl, shift), part);
    }
    void complete(fomusdata& fd);
    bool contains(const mpart_str* ct) const {return apply_visitor(contains_mpart(ct), part);}
    //void setpar(const mpart_str* ptr) {par = ptr;}
    void getmodval(module_value& x) const;
    void getwholemodval(module_value& x) const {getmodval(x);}
    const char* getprintstr() const;
    void print(std::ostream& s, const fomusdata* fd) const;
    const char* gettype() const {return "a partmap";}
    modobjbase* getmappart() const {return apply_visitor(getthepart(), part);}
    void cachesinitformpart(std::set<int>& c) {return apply_visitor(cachesinitformpartvis(c), part);}
    const varbase& get_varbase(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : fom_get_varbase_up(id);}
    fint get_ival(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : fom_get_ival_up(id);}
    rat get_rval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : fom_get_rval_up(id);}
    ffloat get_fval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : fom_get_fval_up(id);}
    const std::string& get_sval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : fom_get_sval_up(id);}
    const module_value& get_lval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : fom_get_lval_up(id);}
  };
  
  // metaparts (voices/staves get distributed to child parts)
  // can be nested
  struct mpart_str:public partormpart_str NONCOPYABLE { // settings override child part settings until notes are distributed
  private:
    const partmap_str* par;
    std::string basedon;
    std::string id; // NO NAME OR ABBR
    std::vector<boost::shared_ptr<partmap_str> > parts; bool partsmod;
    std::vector<modobjbase*> partscache;
    std::vector<int> clefscache;
  public:
    mpart_str():partormpart_str(), par(0), partsmod(false) {}
    mpart_str(const std::string& id):partormpart_str(), par(0), id(id), partsmod(false) {}
    void setid(const std::string& str) {id = str;}
    void inspartmap(boost::shared_ptr<partmap_str>& x) {
      parts.push_back(boost::shared_ptr<partmap_str>(x)); partsmod = true; x.reset(new partmap_str(/*fom*/));
    }
    mpart_str(mpart_str& x, const partmap_str* par, std::map<part_str*, boost::shared_ptr<part_str> >& cl, std::map<mpart_str*, boost::shared_ptr<mpart_str> >& mcl,
	      const numb& shift):partormpart_str(x, shift), par(par), id(x.id), partsmod(x.partsmod) {
      DBG("--mpart_str" << std::endl);
      for (std::vector<boost::shared_ptr<partmap_str> >::iterator i(x.parts.begin()); i != x.parts.end(); ++i) {
	parts.push_back(boost::shared_ptr<partmap_str>((*i)->fomclone(cl, mcl, shift)));
      }
    } // fomclone
    mpart_str* fomclone(const partmap_str* par, std::map<part_str*, boost::shared_ptr<part_str> >& cl, std::map<mpart_str*, boost::shared_ptr<mpart_str> >& mcl,
			const numb& shift = module_none) {
      assert(isvalid());
      DBG("--fomclone" << std::endl);
      return new mpart_str(*this, par, cl, mcl, shift);
    }
    void replace(mpart_str& x) {
      id = x.id;
      parts.assign(x.parts.begin(), x.parts.end()); partsmod = x.partsmod;
      str_base::replace(x);
    }
    void insertallparts(scorepartlist& prts);
    const std::string& getid() const {return id;}
    const char* getcid() const {return id.c_str();}
    void complete(fomusdata& fd);
    bool contains(const mpart_str* ct) const {
#ifndef NDEBUGOUT
      bool ret = hassome(parts.begin(), parts.end(), boost::lambda::bind(&partmap_str::contains,
									 boost::lambda::bind(&boost::shared_ptr<partmap_str>::get,
											     boost::lambda::_1), ct));
      DBG("mpart " << id << ' ' << (ret ? "CONTAINS" : "doesn't CONTAIN") << ' ' << ct->id << std::endl);
      return ret;
#else      
      return hassome(parts.begin(), parts.end(), boost::lambda::bind(&partmap_str::contains,
								     boost::lambda::bind(&boost::shared_ptr<partmap_str>::get,
											 boost::lambda::_1), ct));
#endif      
    }
    bool containsaux(const mpart_str* ct) const {return (id == ct->id) || contains(ct);}
    void getmodval(module_value& x, const bool justid) const;
    void getwholemodval(module_value& x) const {getmodval(x, false);}
    const char* getprintstr() const;
    void print(std::ostream& s, const fomusdata* fd, const bool justid) const;
    void setpar(const partmap_str* ptr) {par = ptr;}

    partormpart_str* getbasethis() {return this;}
    const partormpart_str* getbasethis() const {return this;}

    // optional instrument--get setting
    const varbase& get_varbase(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : fom_get_varbase_up(id);}
    fint get_ival(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : fom_get_ival_up(id);}
    rat get_rval(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : fom_get_rval_up(id);}
    ffloat get_fval(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : fom_get_fval_up(id);}
    const std::string& get_sval(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : fom_get_sval_up(id);}
    const module_value& get_lval(const int id, const event& ev) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : fom_get_lval_up(id);}
    const varbase& get_varbase(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? *i->second : get_varbase_up(id);}
    fint get_ival(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getival() : get_ival_up(id);}
    rat get_rval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getrval() : get_rval_up(id);}
    ffloat get_fval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getfval() : get_fval_up(id);}
    const std::string& get_sval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getsval() : get_sval_up(id);}
    const module_value& get_lval(const int id) const {setmap_constit i(sets.find(id)); return (i != sets.end()) ? i->second->getmodval() : get_lval_up(id);}    
    const varbase& get_varbase_up(const int id) const {return fom_get_varbase_up(id);}
    fint get_ival_up(const int id) const {return fom_get_ival_up(id);}
    rat get_rval_up(const int id) const {return fom_get_rval_up(id);}
    ffloat get_fval_up(const int id) const {return fom_get_fval_up(id);}
    const std::string& get_sval_up(const int id) const {return fom_get_sval_up(id);}
    const module_value& get_lval_up(const int id) const {return fom_get_lval_up(id);}
    metaparts_partmaps getpartmaps() const {
      metaparts_partmaps r = {partscache.size(), (const metaparts_partmapobj*)&partscache[0]};
      return r;
    }
    void cachesinit();
    void cachesinitformpart(std::set<int>& c) {
      for (std::vector<boost::shared_ptr<partmap_str> >::iterator i(parts.begin()); i != parts.end(); ++i) {
	(*i)->cachesinitformpart(c);
      }
    }
    struct module_intslist getclefs() {
      module_intslist l;
      l.n = clefscache.size();
      l.ints = &clefscache[0];
      return l;
    }
    bool ismetapart() const {return true;}
    void getlastentry(info_objinfo& str, const fomusdata& fd) const;
    void clearid() {id.clear();}
  };
  inline void partmap_str::setmpartstr(boost::shared_ptr<mpart_str>& x) {x->setpar(this); part = boost::shared_ptr<mpart_str>(x); x.reset(new mpart_str(/*fom*/));}
  inline bool contains_mpart::operator()(const boost::shared_ptr<mpart_str>& x) const {return x->containsaux(str);}

  inline void cachesinitformpartvis::operator()(const boost::shared_ptr<mpart_str>& x) const {return x->cachesinitformpart(c);}
  
  struct measdef_str:public str_base NONCOPYABLE {
  private:
    std::string basedon;
    std::string id;
  public:
    measdef_str():str_base() {}
    measdef_str(int):str_base(), id("default") {}
    void setid(const std::string& str) {id = str;}
    void replace(measdef_str& x) {
      id = x.id;
      str_base::replace(x);
    }
    const std::string& getid() const {return id;}
    const char* getcid() const {return id.c_str();}
    void complete(fomusdata& fd);
    void getmodval(module_value& x) const;
    void getwholemodval(module_value& x) const {getmodval(x);}
    const char* getprintstr() const;
    const char* getmeasprintstr() const {return (id.empty() ? getprintstr() : make_charptr('|' + (boost::algorithm::iequals(id, "default") ? std::string() : id) + '|'));}
    void print(std::ostream& s, const fomusdata* fd, const bool meas = false) const;
    const char* gettype() const {return "a measure definition";}
    void getlastentry(info_objinfo& str, const fomusdata& fd) const;
    void clearid() {id.clear();}
  };

  typedef std::map<const std::string, boost::shared_ptr<instr_str>, isiless> globinstsmap;
  typedef globinstsmap::iterator globinstsmap_it;
  typedef globinstsmap::const_iterator globinstsmap_constit;
  typedef globinstsmap::value_type globinstsmap_val;

  typedef std::map<const std::string, boost::shared_ptr<percinstr_str>, isiless> globpercsmap;
  typedef globpercsmap::iterator globpercsmap_it;
  typedef globpercsmap::const_iterator globpercsmap_constit;
  typedef globpercsmap::value_type globpercsmap_val;

  const instr_str& getaglobinstr(const std::string& id, const filepos& pos);
  const percinstr_str& getaglobpercinstr(const std::string& id, const filepos& pos);

}
#endif
