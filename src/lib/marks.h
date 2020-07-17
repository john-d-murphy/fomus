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

#ifndef FOMUS_MARKS_H
#define FOMUS_MARKS_H

#ifndef BUILD_LIBFOMUS
#error "marks.h shouldn't be included"
#endif

#include "heads.h"
#include "numbers.h" // numb
#include "modtypes.h" // module_value_type
#include "mods.h"// modbase
#include "algext.h"

#include "module.h"

namespace fomus {

  extern std::set<std::pair<enum module_markids, enum module_markids> > spec;
  inline bool markisspecialpair(const int t1, const int t2) {
    return (spec.find(std::set<std::pair<enum module_markids, enum module_markids> >::value_type((module_markids)t1, (module_markids)t2)) != spec.end()
	    || spec.find(std::set<std::pair<enum module_markids, enum module_markids> >::value_type((module_markids)t2, (module_markids)t1)) != spec.end());
  }
  
  class markbase;
  typedef boost::ptr_vector<markbase> marksvect;
  typedef marksvect::iterator marksvect_it;
  typedef marksvect::const_iterator marksvect_constit;
  
  extern marksvect markdefs;
  
  extern std::map<std::string, markbase*, isiless> marksbyname;

  enum s_type {
    m_single = 0x1,
    m_voicebegin = 0x2,
    m_staffbegin = 0x4,
    m_voiceend = 0x8,
    m_staffend = 0x10
  };
  enum mark_movetype {
    move_right, move_leftright, move_left
  };

  enum mark_props {
    spr_none = 0x0,
    
    spr_cantouch = 0x01,
    spr_cannottouch = 0x02,
    
    spr_canspanone = 0x04,
    spr_cannotspanone = 0x08,
    
    spr_isreduc = 0x10,
    
    spr_canspanrests = 0x20,
    spr_cannotspanrests = 0x40,
    
    spr_canendonrests = 0x80,
    spr_dontspread = 0x100,

    sin_isdetach = 0x200,
    sin_mustdetach = 0x400,
    sin_ispartgroupmark = 0x800
  };

  class noteevbase;
  struct filepos;
  class markbase _NONCOPYABLE {
    friend bool operator<(const markbase& x, const markbase& y);
  protected:
    int id;
    const char* name;
    const char* doc;
    module_value_type vartype;
    mark_movetype mv;
    enum module_markpos pos;
    const char* typedoc;
    int props;
    const markbase* orig;
#ifndef NDEBUG
    int debugvalid;
#endif    
  public:
    markbase(const char* name, const char* doc, const module_value_type vartype, const mark_movetype mv, const enum module_markpos pos, const char* typedoc,
	     const int props = 0, const markbase* orig = 0):
      id(markdefs.size()), name(name), doc(doc), vartype(vartype), mv(mv), pos(pos), typedoc(typedoc), props(props), orig(orig) {
      assert(typedoc);
#ifndef NDEBUG
      debugvalid = 12345;
#endif      
    }
    virtual ~markbase() {
#ifndef NDEBUG
      debugvalid = 0;
#endif      
    }
#ifndef NDEBUG
    bool isvalid() const {return debugvalid == 12345;}
#endif          
    module_value_type gettype() const {return vartype;}
    const std::string getmodsname() const {return getmodcname();}		
    const char* getmodcname() const {return "(built-in)";}		
    const char* getmodlongname() const {return "(built-in)";}	
    const char* getmodauthor() const {return "(built-in)";}		
    const char* getmoddoc() const {assert(false);}			
    enum module_type getmodtype() const {return module_nomodtype;}
    enum module_markpos getpos() const {return pos;}
    int getprops() const {return props;}
    virtual bool iscont() const {return false;}
    
    virtual s_type getspantype() const {return m_single;}
    virtual bool getisvoice() const {return false;}
    virtual bool getisstaff() const {return false;}
    virtual bool getisbegin() const {return false;}
    virtual bool getisend() const {return false;}
    virtual bool getcanreduce() const {return false;}
    virtual bool getcanspanrests(const noteevbase& ev) const {return false;}
    virtual bool getcantouch(const noteevbase& ev) const {return false;}
    virtual bool getcanspanone(const noteevbase& ev) const {return false;}
    virtual bool getcanendonrests(const noteevbase& ev) const {return false;}
    virtual bool getcantouch_nomut(const noteevbase& ev) const {return false;}
    virtual bool getcanspanone_nomut(const noteevbase& ev) const {return false;}
    virtual bool getcanspanrests_nomut(const noteevbase& ev) const {return false;}
    bool getdontspread() const {return props & spr_dontspread;}
    int isdetach() const {return (props & sin_mustdetach) ? 2 : ((props & sin_isdetach) ? 1 : 0);}
    bool ispgroupmark() const {return props & sin_ispartgroupmark;}
    
    virtual int getgroup() const {return 0;}
    
    bool hasaval() const {return vartype == module_number || vartype == module_stringnum;}
    bool hasastr() const {return vartype == module_string || vartype == module_stringnum;}
    
    const char* getname() const {return name;}
    const char* getdoc() const {return doc;}
    int getid() const {return id;}
    int getbaseid() const {
      assert(isvalid());
      assert(!orig || orig->isvalid());
      return (orig ? orig->getid() : id);
    }
    const char* gettypedoc() const {
      return typedoc;
    }

    mark_movetype getmove() const {return mv;}

    marks_which getwhich() const {
      switch (getspantype()) {
      case m_single: return marks_sing;
      case m_voicebegin: 
      case m_staffbegin: return marks_begin;
      case m_voiceend:
      case m_staffend: return marks_end;
      default: assert(false);
      }
    }
    bool hasid(const int i) const {return i == id;}
    virtual void checkargs(const fomusdata* fom, const std::string& str, const numb& val, const filepos& pos) const;
  };
  inline bool operator<(const markbase& x, const markbase& y) {
    if (x.getspantype() != y.getspantype()) return (x.getspantype() < y.getspantype());
    return x.id < y.id;
  }

  struct contmark:public markbase {
    bool spantype; // true if voice, false if staff
    contmark(const char* name, const char* doc, const bool spantype, const module_value_type vartype, const mark_movetype mv, const enum module_markpos pos, const char* typedoc,
	     const int props = 0, const markbase* orig = 0):markbase(name, doc, vartype, mv, pos, typedoc, props, orig), spantype(spantype) {}
    contmark(const char* name0, const int props0, const contmark& x):markbase(name0, x.doc, x.vartype, x.mv, x.pos, x.typedoc, props0, x.orig), spantype(x.spantype) {}
    contmark* clone(const char* name, const int props) const {return new contmark(name, props, *this);}
    bool iscont() const {return true;}
    bool getisvoice() const {return spantype;}
    bool getisstaff() const {return !spantype;}
  };
  
  struct longtrmark:public markbase {
    longtrmark(const char* name, const char* doc, const module_value_type vartype, const mark_movetype mv, const enum module_markpos pos, const char* typedoc,
	       const int props = 0, const markbase* orig = 0):markbase(name, doc, vartype, mv, pos, typedoc, props, orig) {}
    void checkargs(const fomusdata* fom, const std::string& str, const numb& val, const filepos& pos) const;
  };
  struct tremmark:public markbase {
    tremmark(const char* name, const char* doc, const module_value_type vartype, const mark_movetype mv, const enum module_markpos pos, const char* typedoc,
	     const int props = 0, const markbase* orig = 0):markbase(name, doc, vartype, mv, pos, typedoc, props, orig) {}
    void checkargs(const fomusdata* fom, const std::string& str, const numb& val, const filepos& pos) const;
  };
  struct sulmark:public markbase {
    sulmark(const char* name, const char* doc, const module_value_type vartype, const mark_movetype mv, const enum module_markpos pos, const char* typedoc,
	    const int props = 0, const markbase* orig = 0):markbase(name, doc, vartype, mv, pos, typedoc, props, orig) {}
    void checkargs(const fomusdata* fom, const std::string& str, const numb& val, const filepos& pos) const;
  };
  struct tupletmark:public markbase {
    tupletmark(const char* name, const char* doc, const module_value_type vartype, const mark_movetype mv, const enum module_markpos pos, const char* typedoc,
	       const int props = 0, const markbase* orig = 0):markbase(name, doc, vartype, mv, pos, typedoc, props, orig) {}
    void checkargs(const fomusdata* fom, const std::string& str, const numb& val, const filepos& pos) const;
  };
  struct textmark:public markbase {
    textmark(const char* name, const char* doc, const module_value_type vartype, const mark_movetype mv, const enum module_markpos pos, const char* typedoc,
	     const int props = 0, const markbase* orig = 0):markbase(name, doc, vartype, mv, pos, typedoc, props, orig) {}
    void checkargs(const fomusdata* fom, const std::string& str, const numb& val, const filepos& pos) const;
  };
  struct textcontmark:public contmark {
    textcontmark(const char* name, const char* doc, const bool spantype, const module_value_type vartype, const mark_movetype mv, const enum module_markpos pos, const char* typedoc,
		 const int props = 0, const markbase* orig = 0):contmark(name, doc, spantype, vartype, mv, pos, typedoc, props, orig) {}
    void checkargs(const fomusdata* fom, const std::string& str, const numb& val, const filepos& pos) const;
  };
  struct tempomark:public markbase {
    tempomark(const char* name, const char* doc, const module_value_type vartype, const mark_movetype mv, const enum module_markpos pos, const char* typedoc,
	      const int props = 0, const markbase* orig = 0):markbase(name, doc, vartype, mv, pos, typedoc, props, orig) {}
    void checkargs(const fomusdata* fom, const std::string& str, const numb& val, const filepos& pos) const;
  };

  class spanmark:public markbase {
    friend bool operator<(const spanmark& x, const spanmark& y);
  protected:
    s_type spantype;
    int group;
    int cantouchdef, canspanonedef, canspanrestsdef;
  public:
    spanmark(const char* name, const char* doc, const module_value_type vartype, const s_type spantype, const int props, const mark_movetype mv, const enum module_markpos pos,
	     const char* typedoc, const int group = 0, const int cantouchdef = 0, const int canspanonedef = 0, const int canspanrestsdef = 0):
      markbase(name, doc, vartype, mv, pos, typedoc, props), spantype(spantype), group(group), cantouchdef(cantouchdef), canspanonedef(canspanonedef),
      canspanrestsdef(canspanrestsdef) {}
    spanmark(const char* name, const int props, const spanmark& x):markbase(name, x.doc, x.vartype, x.mv, x.pos, x.typedoc, props, &x),
								   spantype(x.spantype), group(x.group),
								   cantouchdef(x.cantouchdef), canspanonedef(x.canspanonedef),
								   canspanrestsdef(x.canspanrestsdef) {}
    virtual spanmark* clone(const char* name, const int props) const {return new spanmark(name, props, *this);}
    s_type getspantype() const {return spantype;}
    bool getcantouch(const noteevbase& ev) const;
    bool getcanspanone(const noteevbase& ev) const;
    bool getcanspanrests(const noteevbase& ev) const;
    bool getcantouch_nomut(const noteevbase& ev) const;
    bool getcanspanone_nomut(const noteevbase& ev) const;
    bool getcanspanrests_nomut(const noteevbase& ev) const;
    bool getcanreduce() const {return props & spr_isreduc;}
    bool getcanendonrests(const noteevbase& ev) const {return getcanspanrests(ev) && (props & spr_canendonrests);}
    bool getisvoice() const {return spantype & (m_voicebegin | m_voiceend);}
    bool getisstaff() const {return spantype & (m_staffbegin | m_staffend);}
    bool getisbegin() const {return spantype & (m_voicebegin | m_staffbegin);}
    bool getisend() const {return spantype & (m_voiceend | m_staffend);}

    int getgroup() const {return group;}
  };
  inline bool operator<(const spanmark& x, const spanmark& y) {
    if (x.spantype != y.spantype) return (x.spantype < y.spantype);
    return x.id < y.id;
  }
  inline bool operator<(const spanmark& x, const markbase& y) {return false;}
  inline bool operator<(const markbase& x, const spanmark& y) {return true;}

  struct textspanner:public spanmark {
    textspanner(const char* name, const char* doc, const module_value_type vartype, const s_type spantype, const int props, const mark_movetype mv, const enum module_markpos pos,
		const char* typedoc, const int group = 0, const int cantouchdef = 0, const int canspanonedef = 0, const int canspanrestsdef = 0):
      spanmark(name, doc, vartype, spantype, props, mv, pos, typedoc, group, cantouchdef, canspanonedef, canspanrestsdef) {}
    textspanner(const char* name, const int props, const spanmark& x):spanmark(name, props, x) {}
    virtual spanmark* clone(const char* name, const int props) const {return new textspanner(name, props, *this);}
    void checkargs(const fomusdata* fom, const std::string& str, const numb& val, const filepos& pos) const;
  };
  
  void initmarks();
  
}

#endif
