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

#include "mods.h"
#include "vars.h" // addmodvar
#include "numbers.h" // type number
#include "algext.h"
//#include "fomusapi.h" // fomus_err variable
#include "data.h"
#include "classes.h" // offbase < offbase
#include "module.h" // modinout api
#include "schedr.h" // stageobj--get the `stage' object for indiv. thread
#include "marks.h" // marksvect
#include "intern.h" // whicheng in internl modules

#include "ifacedumb.h"

#warning "make sure info functions and some modinout functions don't get called after processing has started"

namespace fomus {

  // DATA
  modsvect mods;
  modsmap modsbyname;

  // throws moderr
  lt_ptr getsym(lt_dlhandle ha, const std::string& modname, const char* sym) {
    lt_ptr is(lt_dlsym(ha, sym));
    if (is == NULL) {
#ifndef NDEBUGOUT
      DBG("libltdl: " << lt_dlerror() << std::endl);
#endif    
      CERR << "error loading module `" << modname << '\'' << std::endl;
      throw moderr();
    }
    return is;
  }

  struct foundmoddata {
    std::vector<std::string> bl;
    std::set<std::string> lded;
  };
  int foundmod(const char *filename, lt_ptr dat) { // data is &vector<string>
    try {
      std::string mn(FS_BASENAME(boost::filesystem::path(filename)));
      if (std::binary_search(((foundmoddata*)dat)->bl.begin(), ((foundmoddata*)dat)->bl.end(), mn) || !((foundmoddata*)dat)->lded.insert(mn).second) {
	return 0;
      }
      lt_dladvise adv;
      lt_dlhandle ha = NULL;
      if (!lt_dladvise_init(&adv) && !lt_dladvise_ext(&adv) && !lt_dladvise_local(&adv)) ha = lt_dlopenadvise(filename, adv);
      lt_dladvise_destroy(&adv);
      if (ha == NULL) {
#ifndef NDEBUGOUT
	DBG("libltdl: " << lt_dlerror() << std::endl);
#endif      
	CERR << "error loading module `" << mn << '\'' << std::endl;
	return 0;
      }
      try {
	lt_ptr sy(getsym(ha, mn, "module_init")); // throws moderr
	((modfun_init)sy)(); // call MODFUN_INIT
	modfun_initerr esy(NULL); // modfun_err function
	try {
	  esy = (modfun_initerr)getsym(ha, mn, "module_initerr"); // throws moderr
	  const char* err = esy(); // call MODFUN_ERR (see if INIT produced err)
	  if (err) {
	    CERR << "error `" << err << "' initializing module `" << mn << '\'' << std::endl;
	    throw moderr();
	  }
	  std::auto_ptr<modbase> mb;
	  modfun_type getity((modfun_type)getsym(ha, mn, "module_type"));
	  enum module_type ity = getity();
	  switch (ity) {
	  case module_modengine:
	  case module_modinput:
	  case module_modoutput:
	    {
	      dlmodstufferr st(boost::filesystem::path(filename).FS_BRANCH_PATH().FS_DIRECTORY_STRING().c_str(), mn,
			       (modfun_newdata)getsym(ha, mn, "module_newdata"), //
			       (modfun_freedata)getsym(ha, mn, "module_freedata"), //
			       (modfun_err)getsym(ha, mn, "module_err"), // module_err
			       (modfun_longname)getsym(ha, mn, "module_longname"),
			       (modfun_author)getsym(ha, mn, "module_author"),
			       (modfun_free)getsym(ha, mn, "module_free"),
			       (modfun_doc)getsym(ha, mn, "module_doc"),
			       esy,
			       getity,
			       (modfun_getsetting)getsym(ha, mn, "module_get_setting"),
			       (modfun_ready)getsym(ha, mn, "module_ready"));
	      switch (ity) {
	      case module_modengine:
		mb.reset(new dlmodeng(st,
				      (engauxfun_interfaceid)getsym(ha, mn, "engine_ifaceid"), // provides the interfaceid
				      (engfun_run)getsym(ha, mn, "engine_run"),
				      (engfun_getinterface)getsym(ha, mn, "engine_get_iface")));
		break;
	      case module_modinput:
		mb.reset(new dlmodin(st,
				     (modinoutfun_getext)getsym(ha, mn, "modin_get_extension"),
				     (modinfun_load)getsym(ha, mn, "modin_load"),
				     (modinoutfun_getloadid)getsym(ha, mn, "modin_get_loadid")));
		break;
	      case module_modoutput:
		mb.reset(new dlmodout(st,
				      (modinoutfun_getext)getsym(ha, mn, "modout_get_extension"),
				      (modoutfun_write)getsym(ha, mn, "modout_write"),
				      (modinoutfun_getloadid)getsym(ha, mn, "modout_get_saveid"),
				      (modoutfun_ispre)getsym(ha, mn, "modout_ispre"),
				      (modfun_itertype)getsym(ha, mn, "module_itertype"),
				      (modfun_sameinst)getsym(ha, mn, "module_sameinst")));
		break;
	      default: assert(false);
	      }
	    }
	    break;
	  case module_modaux:
	    mb.reset(new dlmodaux(dlmodstuffinit(boost::filesystem::path(filename).FS_BRANCH_PATH().FS_DIRECTORY_STRING().c_str(), mn,
						 (modfun_longname)getsym(ha, mn, "module_longname"),
						 (modfun_author)getsym(ha, mn, "module_author"),
						 (modfun_free)getsym(ha, mn, "module_free"),
						 (modfun_doc)getsym(ha, mn, "module_doc"),
						 esy,
						 getity,
						 (modfun_getsetting)getsym(ha, mn, "module_get_setting"),
						 (modfun_ready)getsym(ha, mn, "module_ready")),
				  (engauxfun_interfaceid)getsym(ha, mn, "aux_ifaceid"), // provides the interfaceid
				  (auxfun_fillinterface)getsym(ha, mn, "aux_fill_iface")));
	    break;
	  default:
	    mb.reset(new dlmodmod(dlmodstuff(boost::filesystem::path(filename).FS_BRANCH_PATH().FS_DIRECTORY_STRING().c_str(), mn,
					     (modfun_newdata)getsym(ha, mn, "module_newdata"), //
					     (modfun_freedata)getsym(ha, mn, "module_freedata"), //
					     (modfun_longname)getsym(ha, mn, "module_longname"),
					     (modfun_author)getsym(ha, mn, "module_author"),
					     (modfun_free)getsym(ha, mn, "module_free"),
					     (modfun_doc)getsym(ha, mn, "module_doc"),
					     esy,
					     getity,
					     (modfun_getsetting)getsym(ha, mn, "module_get_setting"),
					     (modfun_ready)getsym(ha, mn, "module_ready")),
				  (modfun_interfaceid)getsym(ha, mn, "module_engine_iface"), // what engine type to hook up with
				  (modfun_fillinterface)getsym(ha, mn, "module_fill_iface"),
				  (modfun_itertype)getsym(ha, mn, "module_itertype"),
				  (modfun_priority)getsym(ha, mn, "module_priority"),
				  (modfun_engine)getsym(ha, mn, "module_engine"),
				  (modfun_sameinst)getsym(ha, mn, "module_sameinst")));
	  }
	  modsbyname.insert(modsmap_val(mb->getsname(), mb.get()));
	  mods.push_back(mb.release());
	} catch (const moderr &e) {
	  lt_ptr sy(lt_dlsym(ha, "module_free"));
	  if (sy != NULL) {
	    ((modfun_free)sy)(); // call MODFUN_FREE
	    if (esy != NULL) {
	      const char* err = esy(); // call MODFUN_ERR (see if FREE produced err)
	      if (err != NULL) {
		CERR << "error `" << err << "' freeing module `" << mn << '\'' << std::endl;
	      }	    
	    }
	  }
	  throw;
	}
      } catch (const moderr &e) {
#ifndef NDEBUGOUT
	if (lt_dlclose(ha) != 0) {DBG("libltdl: " << lt_dlerror() << std::endl);}
#else	
	lt_dlclose(ha); // closing error isn't terribly useful after some other error
#endif      
      }
    } catch (const boost::filesystem::filesystem_error &e) {
      CERR << "error accessing file `" << filename << '\'' << std::endl;
    }
    return 0;
  }

  void findconflicts(int strt = 0) {
    bool err = false;
    {
      std::map<const std::string, const varbase*, isiless> names;
      std::map<const std::string, const varbase*, isiless>::const_iterator ne(names.end());
      int c = 0;
      for (varsvect::iterator i(vars.begin()); i != vars.end(); ++i, ++c) {
	const varbase& v = **i;
	const std::string n(v.getname());
	if (c < strt) {
	INSIT:
	  names.insert(std::map<const std::string, const varbase*, isiless>::value_type(n, i->get()));
	  continue;
	}
	std::map<const std::string, const varbase*, isiless>::iterator f(names.find(n));
	if (f != ne) {
	  CERR << "conflicting setting `" << n << "' in modules `" << f->second->getmodsname() << "' and `" << v.getmodsname() << "'\n";
	  err = true;
	} else goto INSIT;
      }
    }
    if (err) {
      ferr.flush();
      throw errbase();
    }
  }

  void findinvalids(int strt = 0) {
    bool err = false;
    for (varsvect_it i(vars.begin() + strt); i != vars.end(); ++i) {
      varbase& v = **i;
      if (!v.isvalid(0)) {
	ferr << " in setting `" << v.getname() << "', module `" << v.getmodsname() << "'\n";
	err = true;
      }
    }
    if (err) {
      ferr.flush();
      throw errbase();
    }
  }
  
  extern "C" {
    void repl_modfun_free() {}
    const char* repl_modfun_initerr() {return 0;}
    void* repl_modfun_newdata(FOMUS f) {return 0;}
    void repl_modfun_freedata(void* data) {}
    const char* repl_modfun_longname() {return "(unnamed)";}
    const char* repl_modfun_author() {return "(callback)";}
    const char* repl_modfun_doc() {return "";}
    const char* repl_modfun_err(void* data) {return 0;}
    int repl_modfun_getsetting(int n, struct module_setting* set, int id) {return 0;}
    void repl_modfun_ready() {}
    const char* repl_modinoutfun_getloadid() {return 0;}
    int repl_modoutfun_ispre() {return 0;}
    int repl_modfun_sameinst(module_obj a, module_obj b) {return 1;}
    int repl_modfun_interfaceid() {return ENGINE_INTERFACEID;}
    const char* repl_modfun_engine(void* m) {return "dumb";}
    int repl_modfun_priority() {return 0;}
  }

  void addvars(const modbase& mb);
  
  template <typename T> const T checkcb(const T cb, const char* name, const char* slot) {
    if (cb) return cb;
    CERR << "missing callback function `" << slot << "\' for module `" << name << '\'' << std::endl;
    throw errbase();
  }
  template <typename T> const T checkcb(const T cb, const T repl) {return (cb ? cb : repl);}
  
  bool dlin = false;
  void clearmodules() {
    if (dlin) {
      mods.clear();
      modsbyname.clear();
      dlin = false;
      if (lt_dlexit() != 0) {
	CERR << "error unloading modules" << std::endl;
	return;	
      }
    }
  }
  void initmodules() {
    dlin = true;
    {
      if (lt_dlinit() != 0) {
#ifndef NDEBUGOUT
	DBG("libltdl: " << lt_dlerror() << std::endl);
#endif      
	CERR << "error loading modules" << std::endl;
	return;
      }
      char *bp = getenv("FOMUS_BUILTIN_MODULES_PATH");
#ifdef __MINGW32__
      std::string pa;
      if (bp) pa = bp; else {
	const char* wsys = getenv("WINDIR");
	pa = std::string(wsys ? wsys : "") + CMD_MACRO(MODULE_PATH);
      }
#else      
      std::string pa(bp ? bp : CMD_MACRO(MODULE_PATH));
#endif      
      char *p = getenv("FOMUS_MODULES_PATH");
      if (p != NULL) pa = std::string(p) + LT_PATHSEP_CHAR + pa; //CMD_MACRO(MODULE_PATH);
      p = getenv("FOMUS_MODULES_BLACKLIST");
      foundmoddata xdata;
      if (p != NULL) {
	boost::split(xdata.bl, p, boost::lambda::_1 == LT_PATHSEP_CHAR);
	std::for_each(xdata.bl.begin(), xdata.bl.end(), boost::lambda::bind(&boost::trim<std::string>, boost::lambda::_1, boost::lambda::constant_ref(std::locale())));
      }
      sort(xdata.bl.begin(), xdata.bl.end());
      lt_dlforeachfile(pa.c_str(), foundmod, &xdata);
    }
    {
      modbase* p;
      mods.push_back(p = new stmod_dumb);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_prune);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_vmarks);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_smarks);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_marks);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_meas);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_rstaves);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_mparts);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_pnotes);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_markevs1);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_markevs2);
      modsbyname.insert(modsmap_val(p->getsname(), p));
      mods.push_back(p = new stmod_grdiv);
      modsbyname.insert(modsmap_val(p->getsname(), p));
    }
    std::for_each(mods.begin(), mods.end(), boost::lambda::bind(addvars, boost::lambda::_1));
    findconflicts();
    findinvalids();
    std::for_each(mods.begin(), mods.end(), boost::lambda::bind(&modbase::isready, boost::lambda::_1));
  }

  inline void initsetting(module_setting &set) {
    set.name = 0;
    set.type = module_number;
    set.descdoc = 0;
    set.typedoc = 0;
    initvalue(set.val);
    set.loc = module_locscore;
    set.valid = 0;
    set.uselevel = 0;
  }
  inline void resetsetting(module_setting &set) {
    freevalue(set.val);
  }

  struct scoped_modsetting {
    module_setting set;
    scoped_modsetting() {initsetting(set);}
    ~scoped_modsetting() {resetsetting(set);}
  };
  void addvars(const modbase& mb) {
    for (int i = 0; ; ++i) {
      scoped_modsetting set;
      if (!mb.getsetting(i, &set.set, vars.size())) break;
      addmodvar(mb, set.set);
    }
  }

  // MODULE API
  bool module_valid_intaux(const struct module_value& val, const fomus_int min, const enum module_bound minbnd, const fomus_int max, const enum module_bound maxbnd, const char* printerr) {
    if (val.type != module_int) {
      printexp(printerr);
      return false;
    }
    if ((minbnd == module_incl && val.val.i < min)
	|| (minbnd == module_excl && val.val.i <= min)
	|| (maxbnd == module_incl && val.val.i > max)
	|| (maxbnd == module_excl && val.val.i >= max)) {
      printexp(printerr);
      return false;
    }
    return true;
  }
  inline bool module_valid_intn(const struct module_value& val, const fomus_int min, const enum module_bound minbnd, const fomus_int max, const enum module_bound maxbnd,
  				const valid_listint fun, const int n, const char* printerr) {
    if (!module_valid_intaux(val, min, minbnd, max, maxbnd, printerr)) return false;
    if (fun && !fun(n, val.val.i)) {
      printexp(printerr);
      return false;
    }
    return true;
  }
  inline bool module_valid_intn(const struct module_value& val, const fomus_int min, const enum module_bound minbnd, const fomus_int max, const enum module_bound maxbnd,
  				const valid_mapint fun, const char* s, const int n, const char* printerr) {
    if (!module_valid_intaux(val, min, minbnd, max, maxbnd, printerr)) return false;
    if (fun && !fun(n, s, val.val.i)) {
      printexp(printerr);
      return false;
    }
    return true;
  }

  bool module_valid_rataux(const struct module_value& val, const fomus_rat& min, const enum module_bound minbnd, const fomus_rat& max, const enum module_bound maxbnd, const char* printerr) {
    if (val.type != module_rat && val.type != module_int) {
      printexp(printerr);
      return false;
    }
    rat v(val.type == module_rat ? rat(val.val.r.num, val.val.r.den) : val.val.i);
    rat mi(min.num, min.den);
    rat ma(max.num, max.den);
    if ((minbnd == module_incl && v < mi) || (minbnd == module_excl && v <= mi) || (maxbnd == module_incl && v > ma) || (maxbnd == module_excl && v >= ma)) {
      printexp(printerr);
      return false;
    }
    return true;
  }
  inline bool module_valid_ratn(const struct module_value& val, const fomus_rat& min, const enum module_bound minbnd, const fomus_rat& max, const enum module_bound maxbnd,
  				const valid_listrat fun, const int n, const char* printerr) {
    if (!module_valid_rataux(val, min, minbnd, max, maxbnd, printerr)) return false;
    if (fun && !fun(n, val.val.r)) { 
      printexp(printerr);
      return false;
    }
    return true;
  }
  inline bool module_valid_ratn(const struct module_value& val, const fomus_rat& min, const enum module_bound minbnd, const fomus_rat& max, const enum module_bound maxbnd,
  				const valid_maprat fun, const char* s, const int n, const char* printerr) {
    if (!module_valid_rataux(val, min, minbnd, max, maxbnd, printerr)) return false;
    if (fun && !fun(n, s, val.val.r)) {
      printexp(printerr);
      return false;
    }
    return true;
  }

  bool module_valid_numaux(const struct module_value& val, const struct module_value& min, const enum module_bound minbnd, const struct module_value& max, const enum module_bound maxbnd, const char* printerr) {
    numb v;
    switch (val.type) {
    case module_int: v = val.val.i; break;
    case module_float: v = val.val.f; break;
    case module_rat: v = rat(val.val.r.num, val.val.r.den); break;
    default:
      printexp(printerr);
      return false;
    }
    numb mi;
    switch (min.type) {
    case module_int:  mi = min.val.i; break;
    case module_float: mi = min.val.f; break;
    case module_rat: mi = rat(min.val.r.num, min.val.r.den); break;
    default: return false; 
    }
    numb ma;
    switch (max.type) {
    case module_int: ma = max.val.i; break;
    case module_float: ma = max.val.f; break;
    case module_rat: ma = rat(max.val.r.num, max.val.r.den); break;
    default: return false;
    }
    if ((minbnd == module_incl && v < mi.modval()) || (minbnd == module_excl && v <= mi.modval()) || (maxbnd == module_incl && v > ma.modval()) || (maxbnd == module_excl && v >= ma.modval())) {
      printexp(printerr);
      return false;
    }
    return true;  
  }
  inline bool module_valid_numn(const struct module_value& val, const struct module_value& min, const enum module_bound minbnd, const struct module_value& max, const enum module_bound maxbnd,
  				const valid_listnum fun, const int n, const char* printerr) {
    if (!module_valid_numaux(val, min, minbnd, max, maxbnd, printerr)) return false;
    if (fun && !fun(n, &val)) { 
      printexp(printerr);
      return false;
    }
    return true;  
  }
  inline bool module_valid_numn(const struct module_value& val, const struct module_value& min, const enum module_bound minbnd, const struct module_value& max, const enum module_bound maxbnd,
  				const valid_mapnum fun, const char* s, const int n, const char* printerr) {
    if (!module_valid_numaux(val, min, minbnd, max, maxbnd, printerr)) return false;
    if (fun && !fun(n, s, &val)) {
      printexp(printerr);
      return false;
    }
    return true;  
  }

  bool module_valid_stringaux(const struct module_value& val, const int minlen, const int maxlen, const char* printerr) {
    if (val.type == module_string && val.val.s != NULL) {
      size_t sz = strlen(val.val.s);
      if ((minlen < 0 || sz >= (size_t)minlen) && (maxlen < 0 || sz <= (size_t)maxlen)) return true;  
    }
    printexp(printerr);
    return false;
  }
  inline bool module_valid_stringn(const struct module_value& val, const int minlen, const int maxlen, const valid_liststring fun, const int n, const char* printerr) {
    if (!module_valid_stringaux(val, minlen, maxlen, printerr)) return false;
    if (fun && !fun(n, val.val.s)) {
      printexp(printerr);
      return false;
    }
    return true;  
  }
  inline bool module_valid_stringn(const struct module_value& val, const int minlen, const int maxlen, const valid_mapstring fun, const char* s, const int n, const char* printerr) {
    if (!module_valid_stringaux(val, minlen, maxlen, printerr)) return false;
    if (fun && !fun(n, s, val.val.s)) { 
      printexp(printerr);
      return false;
    }
    return true;  
  }

  inline bool listofvals_isinvalid(int& n, const struct module_value& val, valid_listval fun, const char* printerr) {
    if (!fun(++n, val)) {
      printexp(printerr);
      return false;
    }
    return true;
  }
  
  inline bool maptoints_isinvalid(int& n, const char* &s, const struct module_value& val, const fomus_int min, enum module_bound minbnd, const fomus_int max, enum module_bound maxbnd, valid_mapint fun, const char* printerr) {
    if (++n % 2 == 0) {
      s = val.val.s; 
      return !module_valid_stringaux(val, 1, -1, printerr);
    } else {
      return !module_valid_intn(val, min, minbnd, max, maxbnd, fun, s, n / 2, printerr);
    }    
  }

  inline bool maptorats_isinvalid(int& n, const char* &s, const struct module_value& val, const struct fomus_rat min, enum module_bound minbnd, const struct fomus_rat max, enum module_bound maxbnd, valid_maprat fun, const char* printerr) {
    if (++n % 2 == 0) {
      s = val.val.s; 
      return !module_valid_stringaux(val, 1, -1, printerr);
    } else {
      return !module_valid_ratn(val, min, minbnd, max, maxbnd, fun, s, n / 2, printerr);
    }    
  }

  inline bool maptonums_isinvalid(int& n, const char* &s, const struct module_value& val, const struct module_value& min, enum module_bound minbnd, const struct module_value& max, enum module_bound maxbnd, valid_mapnum fun, const char* printerr) {
    if (++n % 2 == 0) {
      s = val.val.s; 
      return !module_valid_stringaux(val, 1, -1, printerr);
    } else {
      return !module_valid_numn(val, min, minbnd, max, maxbnd, fun, s, n / 2, printerr);
    }    
  }

  inline bool maptostrings_isinvalid(int& n, const char* &s, const struct module_value& val, int minlen, int maxlen, valid_mapstring fun, const char* printerr) {
    if (++n % 2 == 0) {
      s = val.val.s; 
      return !module_valid_stringaux(val, 1, -1, printerr);
    } else {
      return !module_valid_stringn(val, minlen, maxlen, fun, s, n / 2, printerr);
    }  
  }

  inline bool maptovals_isinvalid(int& n, const char* &s, const struct module_value& val, valid_mapval fun, const char* printerr) {
    if (++n % 2 == 0) {
      s = val.val.s; 
      return !module_valid_stringaux(val, 1, -1, printerr);
    } else {
      if (fun && !fun(n / 2, s, val)) { // fun 
  	printexp(printerr);
  	return true;
      }
      return false;
    }  
  }

  bool dlmodinout::modinout_hasext(const std::string& ext) const {
    for (int i = 0; ; ++i) {
      const char* x = getext(i);
      initerrcheck();
      if (x == NULL) return false;
      if (ext == x) return true;
    }
  }
  void dlmodinout::modinout_collext(std::ostream& ou, bool& fi) const {
    for (int i = 0; ; ++i) {
      const char* x = getext(i);
      initerrcheck();
      if (x == NULL) return;
      if (fi) fi = false; else ou << '|';
      ou << x;
    }
  }
  void dlmodout::modout_addext(std::vector<const char*>& v) const {
    for (int i = 0; ; ++i) {
      const char* x = getext(i);
      initerrcheck();
      if (x == NULL) break;
      v.push_back(x);
    }
  }

  void modbase::fireup(fomusdata* fd, void* data) const {
    const char* e = whicheng(fd);
    modsmap_it i(modsbyname.find(e));
    if (i == modsbyname.end()) {
      CERR << "engine`" << e << "' doesn't exist in module `" << getcname() << '\'' << std::endl;
      throw errbase();
    }
    if (i->second->getifaceid() != getifaceid()) {
      CERR << "invalid engine type in module `" << getcname() << '\'' << std::endl;
      throw errbase();
    }
    moddata theeng(*i->second, i->second->getdata(fd)); // engine data
    fillinterface(data, theeng.getmod().eng_getiface(theeng.get()));
    theeng.getmod().eng_exec(theeng.get(), *this);
    assert(stageobj.get());
    if (stageobj->geterr()) throw errbase();
  }

  inline void wrongtypeerr(const wrongtype& e) {
    CERR << "expected " << e.what << " object";
    modprinterr();
  }

}

// ------------------------------------------------------------------------------------------------------------------------
// END NAMESPACE

using namespace fomus;

void module_register(const char* name, const struct module_callbacks* callbacks) {
  ENTER_API;
  checkinit();
  if (modsbyname.find(name) != modsbyname.end()) {
    CERR << "module `" << name << "' already exists" << std::endl;
    throw errbase();
  }
  if (callbacks->init_fun) callbacks->init_fun();
  modfun_initerr esy = checkcb(callbacks->initerr_fun, repl_modfun_initerr);
  if (esy) {
    const char* err = esy();
    if (err) {
      CERR << "error `" << err << "' initializing module `" << name << '\'' << std::endl;
      throw errbase();
    }
  }
  std::auto_ptr<modbase> mb;
  modfun_type getity(checkcb(callbacks->type_fun, name, "type_fun"));
  enum module_type ity = getity();
  switch (ity) {
  case module_modengine:
  case module_modinput:
  case module_modoutput:
    {
      dlmodstufferr st("(callback)", name,
		       checkcb(callbacks->newdata_fun, repl_modfun_newdata),
		       checkcb(callbacks->freedata_fun, repl_modfun_freedata),
		       checkcb(callbacks->err_fun, repl_modfun_err),
		       checkcb(callbacks->longname_fun, repl_modfun_longname),
		       checkcb(callbacks->author_fun, repl_modfun_author),
		       checkcb(callbacks->free_fun, repl_modfun_free),
		       checkcb(callbacks->doc_fun, repl_modfun_doc),
		       esy,
		       getity,
		       checkcb(callbacks->getsetting_fun, repl_modfun_getsetting),
		       checkcb(callbacks->ready_fun, repl_modfun_ready));
      switch (ity) {
      case module_modengine:
	mb.reset(new dlmodeng(st,
			      checkcb(callbacks->engaux_interfaceid_fun, name, "engaux_interfaceid_fun"),
			      checkcb(callbacks->run_fun, name, "run_fun"),
			      checkcb(callbacks->getinterface_fun, name, "getinterface_fun")));
	break;
      case module_modinput:
	mb.reset(new dlmodin(st,
			     checkcb(callbacks->getext_fun, name, "getext_fun"),
			     checkcb(callbacks->load_fun, name, "load_fun"),
			     checkcb(callbacks->getloadid_fun, repl_modinoutfun_getloadid)));
	break;
      case module_modoutput:
	mb.reset(new dlmodout(st,
			      checkcb(callbacks->getext_fun, name, "getext_fun"),
			      checkcb(callbacks->write_fun, name, "write_fun"),
			      checkcb(callbacks->getloadid_fun, repl_modinoutfun_getloadid),
			      checkcb(callbacks->ispre_fun, repl_modoutfun_ispre),
			      checkcb(callbacks->itertype_fun, name, "itertype_fun"),
			      checkcb(callbacks->sameinst_fun, repl_modfun_sameinst)));
	break;
      default: assert(false);
      }
    }
    break;
  case module_modaux:
    mb.reset(new dlmodaux(dlmodstuffinit("(callback)", name,
					 checkcb(callbacks->longname_fun, repl_modfun_longname),
					 checkcb(callbacks->author_fun, repl_modfun_author),
					 checkcb(callbacks->free_fun, repl_modfun_free),
					 checkcb(callbacks->doc_fun, repl_modfun_doc),
					 esy,
					 getity,
					 checkcb(callbacks->getsetting_fun, repl_modfun_getsetting),
					 checkcb(callbacks->ready_fun, repl_modfun_ready)),
			  checkcb(callbacks->engaux_interfaceid_fun, name, "engaux_interfaceid_fun"),
			  checkcb(callbacks->aux_fillinterface_fun, name, "aux_fillinterface_fun")));
    break;
  default:
    mb.reset(new dlmodmod(dlmodstuff("(callback)", name,
				     checkcb(callbacks->newdata_fun, repl_modfun_newdata),
				     checkcb(callbacks->freedata_fun, repl_modfun_freedata),
				     checkcb(callbacks->longname_fun, repl_modfun_longname),
				     checkcb(callbacks->author_fun, repl_modfun_author),
				     checkcb(callbacks->free_fun, repl_modfun_free),
				     checkcb(callbacks->doc_fun, repl_modfun_doc),
				     esy,
				     getity,
				     checkcb(callbacks->getsetting_fun, repl_modfun_getsetting),
				     checkcb(callbacks->ready_fun, repl_modfun_ready)),
			  checkcb(callbacks->mod_interfaceid_fun, repl_modfun_interfaceid),
			  checkcb(callbacks->mod_fillinterface_fun, name, "mod_fillinterface_fun"),
			  checkcb(callbacks->itertype_fun, name, "itertype_fun"),
			  checkcb(callbacks->priority_fun, repl_modfun_priority),
			  checkcb(callbacks->engine_fun, repl_modfun_engine),
			  checkcb(callbacks->sameinst_fun, repl_modfun_sameinst)));
  }
  modsbyname.insert(modsmap_val(mb->getsname(), mb.get()));
  modbase& x(*mb);
  mods.push_back(mb.release());
  int n = vars.size();
  addvars(x);
  if (n < (int)vars.size()) findconflicts(n);
  findinvalids(n);
  x.isready();
  EXIT_API_VOID;
}

int module_valid_printiferr(int valid, const char* printerr) {
  ENTER_API;
  if (!valid) printexp(printerr);
  return valid;
  EXIT_API_0;
}

struct module_list module_new_list(int n) { // api fun
  ENTER_API;
  struct module_list arr = {n, newmodvals(n)};
  std::for_each(arr.vals, arr.vals + n, boost::lambda::bind(initvalue, boost::lambda::_1));
  return arr;
  EXIT_API_MODLIST0;
}
void module_free_list(struct module_list arr) { // api fun
  ENTER_API;
  if (arr.vals) {
    std::for_each(arr.vals, arr.vals + arr.n, bind(freevalue, boost::lambda::_1));
    deletemodvals(arr.vals);
  }
  EXIT_API_VOID;
}

void module_setval_int(struct module_value* val, fomus_int i) {
  ENTER_API;
  freevalue(*val);
  val->type = module_int;
  val->val.i = i;
  EXIT_API_VOID;
}
void module_setval_float(struct module_value* val, fomus_float f) {
  ENTER_API;
  freevalue(*val);
  val->type = module_float;
  val->val.f = f;
  EXIT_API_VOID;
}
void module_setval_rat(struct module_value* val, struct fomus_rat r) {
  ENTER_API;
  freevalue(*val);
  val->type = module_rat;
  val->val.r = r;
  EXIT_API_VOID;
}
void module_setval_string(struct module_value* val, const char* s) {
  ENTER_API;
  assert(s);
  freevalue(*val);
  val->type = module_string;
  val->val.s = s;
  EXIT_API_VOID;
}
void module_setval_list(struct module_value* val, int n) {
  ENTER_API;
  freevalue(*val);
  val->type = module_list;
  struct module_list& l(val->val.l);
  l.n = n;
  l.vals = newmodvals(n);
  std::for_each(l.vals, l.vals + n, boost::lambda::bind(initvalue, boost::lambda::_1));
  EXIT_API_VOID;
}

fomus_int module_getval_int(struct module_value val) {
  ENTER_API;
  switch (val.type) {
  case module_int: return val.val.i;
  case module_float: return lround(val.val.f);
  case module_rat: return rattoint(rat(val.val.r.num, val.val.r.den));
  default:
    CERR << "invalid value type" << std::endl;
    throw errbase();
  }
  EXIT_API_0;
}
fomus_float module_getval_float(struct module_value val) {
  ENTER_API;
  switch (val.type) {
  case module_int: return val.val.i;
  case module_float: return val.val.f;
  case module_rat: return boost::rational_cast<fomus_float>(rat(val.val.r.num, val.val.r.den));
  default:
    CERR << "invalid value type" << std::endl;
    throw errbase();
  }
  EXIT_API_0;
}
struct fomus_rat module_getval_rat(struct module_value val) {
  ENTER_API;
  switch (val.type) {
  case module_int:
    return tofrat(val.val.i);
  case module_float:
    return tofrat(floattorat(val.val.f));
  case module_rat:
    return tofrat(rat(val.val.r.num, val.val.r.den));
  default:
    CERR << "invalid value type" << std::endl;
    throw errbase();
  }
  EXIT_API_RAT0;
}
const char* module_getval_string(struct module_value val) {
  ENTER_API;
  switch (val.type) {
  case module_string:
    return val.val.s;
  default:
    CERR << "invalid value type" << std::endl;
    throw errbase();
  }
  EXIT_API_0;
}
struct module_list module_getval_list(struct module_value val) {
  ENTER_API;
  switch (val.type) {
  case module_list:
    return val.val.l;
  default:
    CERR << "invalid value type" << std::endl;
    throw errbase();
  }
  EXIT_API_MODLIST0;
}

void module_ratreduce(struct fomus_rat* x) {
  ENTER_API;
  rat r1(x->num, x->den);
  x->num = r1.numerator();
  x->den = r1.denominator();
  EXIT_API_VOID;
}
fomus_int module_rattoint(struct fomus_rat x) {
  ENTER_API;
  return rattoint(rat(x.num, x.den));
  EXIT_API_0;
}
fomus_float module_rattofloat(struct fomus_rat x) {
  ENTER_API;
  return boost::rational_cast<ffloat>(rat(x.num, x.den));
  EXIT_API_0;
}
struct fomus_rat module_inttorat(fomus_int x) {
  ENTER_API;
  return tofrat(x);
  EXIT_API_RAT0;
}
struct fomus_rat module_floattorat(fomus_int x) {
  ENTER_API;
  return tofrat(floattorat(x));
  EXIT_API_RAT0;
}
int module_rateq(struct fomus_rat a, struct fomus_rat b) {
  ENTER_API;
  return rat(a.num, a.den) == rat(b.num, b.den);
  EXIT_API_0;
}
int module_ratneq(struct fomus_rat a, struct fomus_rat b) {
  ENTER_API;
  return rat(a.num, a.den) != rat(b.num, b.den);
  EXIT_API_0;
}
int module_ratlt(struct fomus_rat a, struct fomus_rat b) {
  ENTER_API;
  return rat(a.num, a.den) < rat(b.num, b.den);
  EXIT_API_0;
}
int module_ratlteq(struct fomus_rat a, struct fomus_rat b) {
  ENTER_API;
  return rat(a.num, a.den) <= rat(b.num, b.den);
  EXIT_API_0;
}
int module_ratgt(struct fomus_rat a, struct fomus_rat b) {
  ENTER_API;
  return rat(a.num, a.den) > rat(b.num, b.den);
  EXIT_API_0;
}
int module_ratgteq(struct fomus_rat a, struct fomus_rat b) {
  ENTER_API;
  return rat(a.num, a.den) >= rat(b.num, b.den);
  EXIT_API_0;
}
struct fomus_rat module_ratplus(struct fomus_rat a, struct fomus_rat b) {
  ENTER_API;
  return tofrat(rat(a.num, a.den) + rat(b.num, b.den));
  EXIT_API_RAT0;
}
struct fomus_rat module_ratminus(struct fomus_rat a, struct fomus_rat b) {
  ENTER_API;
  return tofrat(rat(a.num, a.den) - rat(b.num, b.den));
  EXIT_API_RAT0;
}
struct fomus_rat module_ratmult(struct fomus_rat a, struct fomus_rat b) {
  ENTER_API;
  return tofrat(rat(a.num, a.den) * rat(b.num, b.den));
  EXIT_API_RAT0;
}
struct fomus_rat module_ratdiv(struct fomus_rat a, struct fomus_rat b) {
  ENTER_API;
  return tofrat(rat(a.num, a.den) / rat(b.num, b.den));
  EXIT_API_RAT0;
}
struct fomus_rat module_ratneg(struct fomus_rat x0) {
  ENTER_API;
  return tofrat(-rat(x0.num, x0.den));
  EXIT_API_RAT0;
}
struct fomus_rat module_ratinv(struct fomus_rat x0) {
  ENTER_API;
  return tofrat(1 / rat(x0.num, x0.den));
  EXIT_API_RAT0;
}

int module_valid_int(struct module_value val, fomus_int min, enum module_bound minbnd, fomus_int max, enum module_bound maxbnd, valid_int fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_intaux(val, min, minbnd, max, maxbnd, printerr)) return false;
  if (fun && !fun(val.val.i)) { // user must supply own error
    printexp(printerr);
    return false;
  }
  return true;
  EXIT_API_0;
}

int module_valid_rat(struct module_value val, fomus_rat min, enum module_bound minbnd, fomus_rat max, enum module_bound maxbnd, valid_rat fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_rataux(val, min, minbnd, max, maxbnd, printerr)) return false;
  if (fun && !fun(val.val.r)) { // user must supply own error
    printexp(printerr);
    return false;
  }
  return true;
  EXIT_API_0;
}

int module_valid_num(struct module_value val, struct module_value min, enum module_bound minbnd, struct module_value max, enum module_bound maxbnd, valid_num fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_numaux(val, min, minbnd, max, maxbnd, printerr)) return false;
  if (fun && !fun(val)) { // user must supply own error
    printexp(printerr);
    return false;
  }
  return true;
  EXIT_API_0;
}

int module_valid_string(struct module_value val, int minlen, int maxlen, valid_string fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_stringaux(val, minlen, maxlen, printerr)) return false;
  if (fun && !fun(val.val.s)) { // user must supply own error
    printexp(printerr);
    return false;
  }
  return true;
  EXIT_API_0;
}

int module_valid_listofints(struct module_value val, int minlen, int maxlen, fomus_int min, enum module_bound minbnd, fomus_int max, enum module_bound maxbnd, valid_listint fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_listaux(val, minlen, maxlen, printerr)) return false;
  int n = 0;
  if (hassome(val.val.l.vals, val.val.l.vals + val.val.l.n, !boost::lambda::bind(module_valid_intn, boost::lambda::_1, min, minbnd, max, maxbnd, fun, boost::lambda::var(n)++, printerr))) return false;
  return true;
  EXIT_API_0;
}

int module_valid_listofrats(struct module_value val, int minlen, int maxlen, fomus_rat min, enum module_bound minbnd, fomus_rat max, enum module_bound maxbnd, valid_listrat fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_listaux(val, minlen, maxlen, printerr)) return false;
  int n = 0;
  if (hassome(val.val.l.vals, val.val.l.vals + val.val.l.n, !boost::lambda::bind(module_valid_ratn, boost::lambda::_1, boost::lambda::constant_ref(min), minbnd, boost::lambda::constant_ref(max), maxbnd, fun, boost::lambda::var(n)++, printerr))) return false;
  return true;
  EXIT_API_0;
}

int module_valid_listofnums(struct module_value val, int minlen, int maxlen, struct module_value min, enum module_bound minbnd, struct module_value max, enum module_bound maxbnd, valid_listnum fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_listaux(val, minlen, maxlen, printerr)) return false;
  int n = 0;
  if (hassome(val.val.l.vals, val.val.l.vals + val.val.l.n, !boost::lambda::bind(module_valid_numn, boost::lambda::_1, boost::lambda::constant_ref(min), minbnd, boost::lambda::constant_ref(max), maxbnd, fun, boost::lambda::var(n)++, printerr))) return false;
  return true;
  EXIT_API_0;
}

int module_valid_listofstrings(struct module_value val, int minlen0, int maxlen0, int minlen, int maxlen, valid_liststring fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_listaux(val, minlen0, maxlen0, printerr)) return false;
  int n = 0;
  if (hassome(val.val.l.vals, val.val.l.vals + val.val.l.n, !boost::lambda::bind(module_valid_stringn, boost::lambda::_1, minlen, maxlen, fun, boost::lambda::var(n)++, printerr))) return false;
  return true;
  EXIT_API_0;
}

int module_valid_listofvals(struct module_value val, int minlen, int maxlen, valid_listval fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_listaux(val, minlen, maxlen, printerr)) return false;
  int n = -1;
  if (fun && hassome(val.val.l.vals, val.val.l.vals + val.val.l.n, !boost::lambda::bind(listofvals_isinvalid, boost::lambda::var(n), boost::lambda::_1, fun, printerr))) return false;
  return true;
  EXIT_API_0;
}

int module_valid_maptoints(struct module_value val, int minlen, int maxlen, fomus_int min, enum module_bound minbnd, fomus_int max, enum module_bound maxbnd, valid_mapint fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_listaux(val, minlen * 2, maxlen * 2, printerr)) return false;
  int n = -1;
  const char* s;
  if (hassome(val.val.l.vals, val.val.l.vals + val.val.l.n, boost::lambda::bind(maptoints_isinvalid, boost::lambda::var(n), boost::lambda::var(s),
										boost::lambda::_1, min, minbnd, max, maxbnd, fun, printerr))) return false;
  if (n % 2 == 0) {
    CERR << "missing map value";
    return false;    
  }
  return true;
  EXIT_API_0;
}

int module_valid_maptorats(struct module_value val, int minlen, int maxlen, fomus_rat min, enum module_bound minbnd, fomus_rat max, enum module_bound maxbnd, valid_maprat fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_listaux(val, minlen * 2, maxlen * 2, printerr)) return false;  
  int n = -1;
  const char* s;
  if (hassome(val.val.l.vals, val.val.l.vals + val.val.l.n, boost::lambda::bind(maptorats_isinvalid, boost::lambda::var(n), boost::lambda::var(s),
										boost::lambda::_1, boost::lambda::constant_ref(min), minbnd, boost::lambda::constant_ref(max), maxbnd, fun, printerr))) return false;
  if (n % 2 == 0) {
    CERR << "missing map value";
    return false;    
  }
  return true;
  EXIT_API_0;
}

int module_valid_maptonums(struct module_value val, int minlen, int maxlen, struct module_value min, enum module_bound minbnd, struct module_value max, enum module_bound maxbnd, valid_mapnum fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_listaux(val, minlen * 2, maxlen * 2, printerr)) return false;  
  int n = -1;
  const char* s;
  if (hassome(val.val.l.vals, val.val.l.vals + val.val.l.n, boost::lambda::bind(maptonums_isinvalid, boost::lambda::var(n), boost::lambda::var(s),
										boost::lambda::_1, boost::lambda::constant_ref(min), minbnd, boost::lambda::constant_ref(max), maxbnd, fun, printerr))) return false;
  if (n % 2 == 0) {
    CERR << "missing map value";
    return false;    
  }
  return true;
  EXIT_API_0;
}

int module_valid_maptostrings(struct module_value val, int minlen0, int maxlen0, int minlen, int maxlen, valid_mapstring fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_listaux(val, minlen0 * 2, maxlen0 * 2, printerr)) return false; 
  int n = -1;
  const char* s;
  if (hassome(val.val.l.vals, val.val.l.vals + val.val.l.n, boost::lambda::bind(maptostrings_isinvalid, boost::lambda::var(n), boost::lambda::var(s),
										boost::lambda::_1, minlen, maxlen, fun, printerr))) return false;
  if (n % 2 == 0) {
    CERR << "missing map value";
    return false;    
  }
  return true;
  EXIT_API_0;
}

int module_valid_maptovals(struct module_value val, int minlen, int maxlen, valid_mapval fun, const char* printerr) {
  ENTER_API;
  if (!module_valid_listaux(val, minlen * 2, maxlen * 2, printerr)) return false; 
  int n = -1;
  const char* s;
  if (hassome(val.val.l.vals, val.val.l.vals + val.val.l.n, boost::lambda::bind(maptovals_isinvalid, boost::lambda::var(n), boost::lambda::var(s),
										boost::lambda::_1, fun, printerr))) return false;
  if (n % 2 == 0) {
    CERR << "missing map value";
    return false;    
  }
  return true;
  EXIT_API_0;
}

int module_setting_allowed(const enum module_setting_loc allowedin, const enum module_setting_loc setloc) {
  ENTER_API;
  return localallowed(allowedin, setloc);
  EXIT_API_0;
}

int module_settingid(const char* set) {
  ENTER_API;
  assert(varslookup.size() == vars.size());
  varsmap_constit i(varslookup.find(set));
  return i == varslookup.end() ? -1 : i->second->getid();
  EXIT_API_0;
}

fomus_int module_setting_ival(module_obj f, int id) {
  ENTER_API;
  try {
    if (f) {
      if (id < 0 || id > (int)vars.size()) {
	CERR << "invalid setting id " << id; modprinterr(); throw errbase();}
      return ((modobjbase*)f)->get_ival(id);
    } else {
      return getdefaultival(id);
    }
  } catch (const badset& e) {
    modprinterr(); throw errbase();
  }
  EXIT_API_0;
}
struct fomus_rat module_setting_rval(module_obj f, int id) {
  ENTER_API;
  try {
    if (f) {
      if (id < 0 || id > (int)vars.size()) {
	CERR << "invalid setting id " << id; modprinterr(); throw errbase();}
      return tofrat(((modobjbase*)f)->get_rval(id));
    } else {
      return tofrat(getdefaultrval(id));
    }
  } catch (const badset& e) {
    modprinterr(); throw errbase();
  }
  EXIT_API_RAT0;
}
fomus_float module_setting_fval(module_obj f, int id) {
  ENTER_API;
  try {
    if (f) {
      if (id < 0 || id > (int)vars.size()) {
	CERR << "invalid setting id " << id; modprinterr(); throw errbase();}
      return ((modobjbase*)f)->get_fval(id);
    } else {
      return getdefaultfval(id);
    }
  } catch (const badset& e) {
    modprinterr(); throw errbase();
  }
  EXIT_API_0;
}
const char* module_setting_sval(module_obj f, int id) {
  ENTER_API;
  try {
    if (f) {
      if (id < 0 || id > (int)vars.size()) {
	CERR << "invalid setting id " << id; modprinterr(); throw errbase();}
      return ((modobjbase*)f)->get_sval(id).c_str();
    } else {
      return getdefaultsval(id).c_str();
    }
  } catch (const badset& e) {
    modprinterr(); throw errbase();
  }
  EXIT_API_0;
}
struct module_value module_setting_val(module_obj f, int id) {
  ENTER_API;
  try {
    if (f) {
      if (id < 0 || id > (int)vars.size()) {
	CERR << "invalid setting id " << id; modprinterr(); throw errbase();}
      return ((modobjbase*)f)->get_lval(id);
    } else {
      return getdefaultmodval(id);
    }
  } catch (const badset& e) {
    modprinterr(); throw errbase();
  }
  EXIT_API_MODVAL0;
}

module_objlist module_get_percinsts(FOMUS f, int all) { // TODO: use vectors<>
  ENTER_API;
  return all ? ((fomusdata*)f)->get_allpercinstslist() : ((fomusdata*)f)->get_percinstslist();
  EXIT_API_MODOBJLIST0;
}
module_objlist module_get_insts(FOMUS f, int all) {
  ENTER_API;
  return all ? ((fomusdata*)f)->get_allinstslist() : ((fomusdata*)f)->get_instslist();
  EXIT_API_MODOBJLIST0;
}
module_objlist module_get_parts(FOMUS f) {
  ENTER_API;
  return ((fomusdata*)f)->get_partslist();
  EXIT_API_MODOBJLIST0;
}

const struct info_setlist info_get_settings(module_obj o) { // modinouts don't run in parallel
  ENTER_API;
  try {
    return ((modobjbase*)o)->getsettinginfo();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_SETLIST0;
}

const struct info_marklist info_get_marks() {
  ENTER_API;
  marklist.resize(markdefs.size());
  for_each2(marklist.marks, marklist.marks + marklist.n, markdefs.begin(), boost::lambda::bind(get_markinfo, boost::lambda::_1, boost::lambda::_2));
  return marklist;
  EXIT_API_MARKLIST0;
}

const struct info_objinfo_list info_get_percinsts(FOMUS f) {
  ENTER_API;
  return ((fomusdata*)f)->getpercinstsinfo();
  EXIT_API_INOUTOBJLIST0;
}
const struct info_objinfo_list info_get_insts(FOMUS f) {
  ENTER_API;
  return ((fomusdata*)f)->getinstsinfo();
  EXIT_API_INOUTOBJLIST0;
}
const struct info_objinfo_list info_get_parts(FOMUS f) {
  ENTER_API;
  return ((fomusdata*)f)->getpartsinfo();
  EXIT_API_INOUTOBJLIST0;
}
const struct info_objinfo_list info_get_metaparts(FOMUS f) {
  ENTER_API;
  return ((fomusdata*)f)->getmpartsinfo();
  EXIT_API_INOUTOBJLIST0;
}
const struct info_objinfo_list info_get_measdefs(FOMUS f) {
  ENTER_API;
  return ((fomusdata*)f)->getmeasdefinfo();
  EXIT_API_INOUTOBJLIST0;
}

struct info_objinfo info_getlastentry(FOMUS f) {
  ENTER_API;
  return ((fomusdata*)f)->getlastentry();
  EXIT_API_INFOEXTOBJINFO0;
}

const char* module_rattostr(struct fomus_rat x) {
  ENTER_API;
  std::ostringstream str;
  str << rat(x.num, x.den);
  return make_charptr(str);
  EXIT_API_0;
}
const char* module_valuetostr(struct module_value x) {
  ENTER_API;
  std::ostringstream str;
  str << numb(x);
  return make_charptr(str);
  EXIT_API_0;
}

module_noteobj module_nextnote() {
  ENTER_API;
  return stageobj->api_nextnote();
  EXIT_API_0;  
}
// for modules working with entire parts--this isn't synchronized with `nextnote', these are just iterators
module_measobj module_nextmeas() {
  ENTER_API;
  return stageobj->api_nextmeas();
  EXIT_API_0;  
}
module_partobj module_nextpart() {
  ENTER_API;
  return stageobj->api_nextpart();  
  EXIT_API_0;  
}

module_noteobj module_peeknextnote(module_noteobj note) {
  ENTER_API;
  assert(!note || ((modobjbase*)note)->isvalid());
  return stageobj->api_peeknextnote((modobjbase*)note);  
  EXIT_API_0;  
}
module_measobj module_peeknextmeas(module_measobj meas) {
  ENTER_API;
  assert(!meas || ((modobjbase*)meas)->isvalid());
  return stageobj->api_peeknextmeas((modobjbase*)meas);    
  EXIT_API_0;  
}
module_partobj module_peeknextpart(module_partobj part) {
  ENTER_API;
  assert(!part || ((modobjbase*)part)->isvalid());
  return stageobj->api_peeknextpart((modobjbase*)part);    
  EXIT_API_0;  
}

module_measobj module_meas(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getmeasobj();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
module_partobj module_part(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getpartobj();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
module_instobj module_inst(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getinstobj();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;
}
module_staffobj staves_staff(module_partobj part, int staff) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    return ((modobjbase*)part)->getpartstaffobj(staff);
  } catch (const staffrange& e) {
    CERR << "staff out of range";
    modprinterr();
    throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;
}
module_clefobj staves_clef(module_partobj part, int staff, int clef) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    return ((modobjbase*)part)->getpartclefobj(staff, clef);
  } catch (const staffrange& e) {
    CERR << "staff out of range";
    modprinterr();
    throw errbase();
  } catch (const clefvalidid& e) {
    CERR << "invalid clef id";
    modprinterr();
    throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;
}


fomus_rat retrat(const module_value& x) {
  switch (x.type) {
  case module_int: {
    fomus_rat r = {x.val.i, 1};
    return r;
  }
  case module_rat: return x.val.r;
  default:
    CERR << "return value not of type `rational'";
    modprinterr();
    throw errbase();
  }
}
const fomus_int& retint(const module_value& x) {
  switch (x.type) {
  case module_int: return x.val.i;
  case module_rat: if (x.val.r.den == 1) return x.val.r.num;
  default:
    CERR << "return value not of type `integer'";
    modprinterr();
    throw errbase();
  }
}

module_value module_vtime(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->gettime();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODVAL0;  
}
fomus_rat module_time(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return retrat(((modobjbase*)obj)->gettime());
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
module_value module_vgracetime(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getgracetime();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODVAL0;  
}
fomus_rat module_gracetime(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return retrat(((modobjbase*)obj)->getgracetime());
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
module_value module_vdur(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getdur();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODVAL0;  
}
fomus_rat module_dur(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return retrat(((modobjbase*)obj)->getdur());
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
module_value module_vendtime(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getendtime();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODVAL0;  
}
fomus_rat module_endtime(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return retrat(((modobjbase*)obj)->getendtime());
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
module_value module_vgraceendtime(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getgraceendtime();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODVAL0;  
}
fomus_rat module_graceendtime(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return retrat(((modobjbase*)obj)->getgraceendtime());
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
module_value module_vgracedur(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getgracedur();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODVAL0;  
}
fomus_rat module_gracedur(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return retrat(((modobjbase*)obj)->getgracedur());
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
module_value module_vtiedendtime(module_noteobj note){
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->gettiedendtime();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODVAL0;  
}
fomus_rat module_tiedendtime(module_noteobj note){
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return retrat(((modobjbase*)note)->gettiedendtime());
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
module_value module_vpitch(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getnote();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODVAL0;  
}
fomus_rat module_pitch(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return retrat(((modobjbase*)note)->getnote());
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
fomus_int module_writtennote(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return retint(((modobjbase*)note)->getwrittennote());
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_voice(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getvoice();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_staff(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getstaff(); 
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
struct module_intslist module_voices(module_obj obj) {
  ENTER_API;
  try {
    return ((modobjbase*)obj)->getvoices();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_INTSLIST0;  
}
struct module_intslist module_staves(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getstaves();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_INTSLIST0;  
}
struct module_intslist module_clefs(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getclefs();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_CLEFSLIST0;  
}
int module_totalnstaves(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getnstaves();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
struct module_intslist module_clefsinstaff(module_obj obj, int staff) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getclefs(staff);
  } catch (const staffrange& e) {
    CERR << "staff out of range";
    modprinterr();
    throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_CLEFSLIST0;  
}
int module_octsign(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getoctsign();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
fomus_rat module_fullwrittenacc(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getfullwrittenacc();
  } catch (const keysigerr& e) {
    modprinterr(); throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
fomus_rat module_writtenacc1(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getwrittenacc1();
  } catch (const keysigerr& e) {
    modprinterr(); throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
fomus_rat module_writtenacc2(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getwrittenacc2();
  } catch (const keysigerr& e) {
    modprinterr(); throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
fomus_rat module_fullacc(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getfullacc();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
fomus_rat module_acc1(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getacc1();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
fomus_rat module_acc2(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getacc2();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
fomus_rat module_adjdur(module_obj obj, int level) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return retrat(((modobjbase*)obj)->getwrittendur(level));
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
fomus_rat module_adjgracedur(module_obj obj, int level) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return retrat(((modobjbase*)obj)->getwrittengracedur(level));
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
fomus_rat module_beatstoadjdur(module_obj obj, fomus_rat dur, int level) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getbeatstowrittendur(dur, level);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}

int module_octavebegin(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getoctavebegin();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_octaveend(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getoctaveend();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}

int module_istiedleft(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getistiedleft();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_istiedright(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getistiedright();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}

int module_tupletbegin(module_noteobj note, int level) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->gettupletbegin(level);
  } catch (const badtuplvl& e) {
    CERR << "invalid tuplet level";
    modprinterr();
    throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_tupletend(module_noteobj note, int level) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->gettupletend(level);
  } catch (const badtuplvl& e) {
    CERR << "invalid tuplet level";
    modprinterr();
    throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
  
int module_hasvoice(module_noteobj note, int voice) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->gethasvoice(voice);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;
}
int module_hasstaff(module_noteobj note, int staff) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->gethasstaff(staff);
  } catch (const staffrange& e) {
    CERR << "staff out of range";
    modprinterr();
    throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_hasclef(module_obj obj, int clef) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->gethasclef(clef);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_hasstaffclef(module_obj obj, int staff, int clef) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->gethasclef(clef, staff);
  } catch (const staffrange& e) {
    CERR << "staff out of range";
    modprinterr();
    throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
    
int module_isrest(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getisrest();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_ismarkrest(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getismarkrest();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_isnote(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getisnote();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_isgrace(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getisgrace();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}

int module_markisspecialpair(int type1, int type2) {
  ENTER_API;
  return markisspecialpair(type1, type2);
  EXIT_API_0;  
}

fomus_rat module_timesig(module_measobj meas) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    return ((modobjbase*)meas)->timesig();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}
fomus_rat module_writtenmult(module_measobj meas) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    return ((modobjbase*)meas)->writtenmult();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RAT0;  
}

module_barlines modout_rightbarline(module_measobj meas) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    return ((modobjbase*)meas)->getbarline();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODBARLINES;  
}

int module_less(module_obj obj1, module_obj obj2) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj1)->isvalid());
    assert(((modobjbase*)obj2)->isvalid());
    return ((modobjbase*)obj1)->objless(*(modobjbase*)obj2);
  } catch (const objlesserr& e) {
    CERR << "cannot determine order of " << ((modobjbase*)obj1)->gettype() << " object and " << ((modobjbase*)obj2)->gettype() << " object";
    modprinterr();
    throw errbase();
  }
  EXIT_API_0;  
}
  
module_ratslist module_divs(module_measobj meas) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    return ((modobjbase*)meas)->getdivs();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_RATSLIST0;  
}

int module_clefmidpitch(int clef) {
  ENTER_API;
  return clefmidpitches[clef];
  EXIT_API_0;  
}

int module_clef(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getclef();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }    
  EXIT_API_0;  
}

module_markslist module_singlemarks(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getsinglemarks();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }    
  EXIT_API_MARKSLIST0;  
}
module_markslist module_marks(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getmarks();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }    
  EXIT_API_MARKSLIST0;  
}
module_markslist module_spannerbegins(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getspannerbegins();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }    
  EXIT_API_MARKSLIST0;  
}
module_markslist module_spannerends(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getspannerends();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MARKSLIST0;  
}
int module_markid(module_markobj mark) {
  ENTER_API;
  try {
    assert(((modobjbase*)mark)->isvalid());
    return ((modobjbase*)mark)->getmarkid();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_markbaseid(int type) {
  ENTER_API;
  try {
    if (type < 0 || type >= (int)markdefs.size()) {
      CERR << "bad mark id";
      modprinterr();
      throw errbase();
    }
    return markdefs[type].getbaseid();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
const char* module_markstring(module_markobj mark) {
  ENTER_API;
  try {
    assert(((modobjbase*)mark)->isvalid());
    return ((modobjbase*)mark)->getmarkstr();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
module_value module_marknum(module_markobj mark) {
  ENTER_API;
  try {
    assert(((modobjbase*)mark)->isvalid());
    return ((modobjbase*)mark)->getmarkval();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODVAL0;  
}
int modout_markorder(module_markobj mark) {
  ENTER_API;
  try {
    assert(((modobjbase*)mark)->isvalid());
    return ((modobjbase*)mark)->getmarkorder();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;
}
enum module_markpos module_markpos(module_markobj mark) {
  ENTER_API;
  try {
    assert(((modobjbase*)mark)->isvalid());
    return ((modobjbase*)mark)->getmarkpos();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0MARKPOS;
}
int module_markcantouch(int type, module_noteobj note) {
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  assert(((modobjbase*)note)->isvalid());
  return ((modobjbase*)note)->getismarkrest() || markdefs[type].getcantouch(((modobjbase*)note)->getnoteevbase());
  EXIT_API_0;  
}
int module_markcanspanone(int type, module_noteobj note) {
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  assert(((modobjbase*)note)->isvalid());
  return ((modobjbase*)note)->getismarkrest() || markdefs[type].getcanspanone(((modobjbase*)note)->getnoteevbase());
  EXIT_API_0;  
}
int module_markisdetachable(int type) {
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  return markdefs[type].isdetach();
  EXIT_API_0;  
}
int module_markcanreduce(int type) { 
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  return markdefs[type].getcanreduce();
  EXIT_API_0;  
}
int module_markcanspanrests(int type, module_noteobj note) { 
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  assert(((modobjbase*)note)->isvalid());
  return ((modobjbase*)note)->getismarkrest() || markdefs[type].getcanspanrests(((modobjbase*)note)->getnoteevbase());
  EXIT_API_0;  
}
int module_markcanendonrests(int type, module_noteobj note) { 
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  assert(((modobjbase*)note)->isvalid());
  return ((modobjbase*)note)->getismarkrest() || markdefs[type].getcanendonrests(((modobjbase*)note)->getnoteevbase());
  EXIT_API_0;  
}
int module_markspangroup(int type) {
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  return markdefs[type].getgroup();
  EXIT_API_0;  
}
int module_markisvoice(int type) {
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  return markdefs[type].getisvoice();
  EXIT_API_0;  
}
int module_markisstaff(int type) {
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  return markdefs[type].getisstaff();
  EXIT_API_0;  
}
int module_markisspannerbegin(int type) {
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  return markdefs[type].getisbegin();
  EXIT_API_0;  
}
int module_markisspannerend(int type) {
  ENTER_API;
  if (type < 0 || type >= (int)markdefs.size()) {
    CERR << "bad mark id";
    modprinterr();
    throw errbase();
  }
  return markdefs[type].getisend();
  EXIT_API_0;  
}

const char* module_pitchtostr(struct module_value pitch) {
  ENTER_API;
  return notenumtostring(threadfd.get(), pitch, "").c_str();
  EXIT_API_0;  
}

const char* module_pitchnametostr(struct module_value pitch, const int forceacc) {
  ENTER_API;
  try {
    return notenumtostring(threadfd.get(), pitch, "", false, forceacc).c_str();
  } catch (const badforceacc& e) {
    CERR << "invalid accidental value";
    modprinterr();
    throw errbase();    
  }
  EXIT_API_0;  
}

const char* module_id(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getcid();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
  
int module_isbeginchord(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getisbeginchord();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_isendchord(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getisendchord();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_ischordlow(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getischordlow();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}
int module_ischordhigh(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getischordhigh();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}

module_value module_dyn(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getdyn();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_MODVAL0;
}

int module_isfullmeasrest(module_measobj meas) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    return ((modobjbase*)meas)->getisfullmeasrest();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;    
}

int module_partialmeas(module_measobj meas) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    return ((modobjbase*)meas)->getpartialmeas();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;    
}

int module_partialbarline(module_measobj meas) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    return ((modobjbase*)meas)->getpartialbarline();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;    
}

const char* info_valstr(module_obj obj) {
  ENTER_API;
  try {
    assert(((modobjbase*)obj)->isvalid());
    return ((modobjbase*)obj)->getprintstr();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;    
}

fomus_rat module_tuplet(module_noteobj note, int level) {
  ENTER_API;
  try {
    return ((modobjbase*)note)->gettuplet(level);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  } catch (const badtuplvl& e) {
    CERR << "invalid tuplet level";
    modprinterr();
    throw errbase();
  }
  EXIT_API_RAT0;  
}

fomus_rat module_fulltupdur(module_noteobj note, int level) {
  ENTER_API;
  try {
    return ((modobjbase*)note)->getfulltupdur(level);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  } catch (const badtuplvl& e) {
    CERR << "invalid tuplet level";
    modprinterr();
    throw errbase();
  }
  EXIT_API_RAT0;  
}

const char* module_staffclef(module_partobj part, int staff) {
  ENTER_API;
  try {
    return ((modobjbase*)part)->getdefclef(staff);
  } catch (const staffrange& e) {
    CERR << "staff out of range";
    modprinterr();
    throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;
}

int module_staffvoice(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getvoiceinstaff();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;
}

metaparts_partmaps metaparts_getpartmaps(module_partobj part) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    return ((modobjbase*)part)->getpartmaps();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_PARTMAPS0;  
}
module_partobj metaparts_partmappart(metaparts_partmapobj partmap) {
  ENTER_API;
  try {
    assert(((modobjbase*)partmap)->isvalid());
    return ((modobjbase*)partmap)->getmappart();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;  
}
int module_ismetapart(module_partobj part) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    return ((modobjbase*)part)->ismetapart();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;  
}

module_percinstobj module_percinst(module_noteobj note) { // return NULL if no perc inst
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getpercinst();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;      
}
const char* module_percinststr(module_noteobj note) { // return NULL if no perc inst
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getpercinststr();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;      
}
int module_isperc(module_noteobj note) { // this is different than above!
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getisperc();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;      
}

int modout_isinvisible(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->isinvisible();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;      
}

int modout_beamsleft(module_noteobj note) { // return NULL if no perc inst
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getbeamsleft();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;      
}

int modout_beamsright(module_noteobj note) { // return NULL if no perc inst
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getbeamsright();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;      
}
int modout_connbeamsleft(module_noteobj note) { // return NULL if no perc inst
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getconnbeamsleft();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;      
}

int modout_connbeamsright(module_noteobj note) { // return NULL if no perc inst
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getconnbeamsright();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;      
}

module_objlist module_getmarkevlist(module_partobj part) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    return ((modobjbase*)part)->getmarkevslist();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_MARKEVS0;      
}

const char* module_getposstring(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getfileposstr();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;      
}

struct module_keysigref module_keysigacc(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->getkeysigacc();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_KEYSIGREF0;      
}

struct module_keysigref module_measkeysigacc(module_measobj meas, int note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)meas)->getkeysigacc(note);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_KEYSIGREF0;      
}

// return types trad, indiv, full-indi
struct modout_keysig modout_keysigdef(module_measobj meas) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    return ((modobjbase*)meas)->getkeysig();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  } 
  EXIT_API_0KEYSIGSTR;  
}

struct modin_imports modin_imports(module_partobj part) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    return ((modobjbase*)part)->getimports();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0IMPORTS;
}
module_obj modout_export(module_partobj part) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    return ((modobjbase*)part)->getexport();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;
}

const char* modin_importpercid(module_obj imp) {
  ENTER_API;
  try {
    assert(((modobjbase*)imp)->isvalid());
    return ((modobjbase*)imp)->getimportpercid();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_0;
}

module_noteobj module_leftmosttiednote(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->leftmosttied(); 
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;    
}

module_noteobj module_rightmosttiednote(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    return ((modobjbase*)note)->rightmosttied(); 
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;    
}

void meas_assign(module_measobj meas, fomus_rat time, fomus_rat dur, int rmable) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    ((modobjbase*)meas)->assignmeas(time, dur, rmable); // meas must be a meausure when assignmeas is called
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void merge_assign_merge(module_noteobj note, module_noteobj tonote) {
  ENTER_API;
  #warning "FINISH ME!!!"
  EXIT_API_VOID;
}

void tquant_assign_time(module_noteobj note, struct fomus_rat time, struct fomus_rat dur) { // fomus protects grace notes from having durations overwritten
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assigntquant(time, dur);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}
void tquant_assign_gracetime(module_noteobj note, struct fomus_rat time, struct module_value grtime, struct module_value dur) { // fomus protects grace notes from having durations overwritten
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assigntquant(time, grtime, dur);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void tquant_delete(module_noteobj note) { // user might want notes deleted if their durations quantize to 0
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assigntquantdel();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void special_assign_makeinv(module_noteobj note) { // user might want notes deleted if their durations quantize to 0
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assigninv();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void special_assign_delete(module_noteobj note) { // user might want notes deleted if their durations quantize to 0
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assigntquantdel();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void special_assign_newnote(module_noteobj note, fomus_rat pitch, fomus_rat acc1, fomus_rat acc2, special_markslist marks) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->newinvnote(pitch, acc1, acc2, marks);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void pquant_assign(module_noteobj note, struct fomus_rat pitch) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignpquant(pitch);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void tpose_assign(module_noteobj note, struct fomus_rat pitch) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignpquant(pitch);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void module_skipassign(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->skipassign();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void voices_assign(module_noteobj note, int voice) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignvoices(voice);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void accs_assign(module_noteobj note, fomus_rat acc, fomus_rat micacc) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignaccs(acc, micacc);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

void staves_assign(module_noteobj note, int staff, int clef) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignstaves(staff, clef);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void divide_assign_initdivs(module_measobj meas, module_ratslist initdivs) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    ((modobjbase*)meas)->assigndivs(initdivs);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void divide_assign_split(module_measobj meas, divide_split split) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    ((modobjbase*)meas)->assignsplit(split);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void divide_assign_gracesplit(module_measobj meas, struct divide_gracesplit split) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    ((modobjbase*)meas)->assigngracesplit(split);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

void divide_assign_unsplit(module_noteobj note2) {
  ENTER_API;
  try {
    assert(((modobjbase*)note2)->isvalid());
    ((modobjbase*)note2)->assignunsplit();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;
}

void cautaccs_assign(module_noteobj note) { // assign a cautionary accidental
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assigncautacc();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

void beams_assign_beams(module_noteobj note, int leftbeams, int rightbeams) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignbeams(leftbeams, rightbeams);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

void parts_assigngroup(module_partobj begin, module_partobj end, parts_grouptype type) {
  ENTER_API;
  try {
    assert(((modobjbase*)begin)->isvalid());
    assert(((modobjbase*)end)->isvalid());
    ((modobjbase*)begin)->assigngroupbegin(type);
    ((modobjbase*)end)->assigngroupend(type);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

enum parts_grouptype module_partgroupbegin(module_partobj part, int lvl) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    return ((modobjbase*)part)->partgroupbegin(lvl);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase(); 
  }
  EXIT_API_PART0;  
}
int module_partgroupend(module_partobj part, int lvl) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    return ((modobjbase*)part)->partgroupend(lvl);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase(); 
  }
  EXIT_API_0;  
}

void parts_assignorder(module_partobj part, int ord, int tempomarks) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    ((modobjbase*)part)->assignorder(ord, tempomarks);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

void prune_assign(module_noteobj note, fomus_rat time1, fomus_rat time2) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignprune(time1, time2);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}
  
void prune_assign_done(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignprunedone();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

void octs_assign(module_noteobj note, int octs) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignocts(octs);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

const int whitenotes[12] = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
const int blacknotes[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
const int diatonicnotes[12] = {0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6};
const int chromaticnotes[7] = {0, 2, 4, 5, 7, 9, 11};

void module_get_auxiface(const char* modname, int ifaceid, void* iface) {
  ENTER_API;
  modsmap_constit i(modsbyname.find(modname));
  if (i == modsbyname.end()) {
    CERR << "invalid module name `" << modname << '\'';
    modprinterr();
    throw errbase();
  }
  if (i->second->getifaceid() != ifaceid) {
    CERR << "invalid module `" << modname << '\'';
    modprinterr();
    throw errbase();
  }
  i->second->fillauxinterface(iface);
  EXIT_API_VOID;  
}

void marks_assign_add(module_noteobj note, int type, const char* arg1, module_value arg2) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignmarkins(type, arg1, arg2);
  } catch (const assmarkerr& e) {
    CERR << "invalid mark"; modprinterr(); throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;      
}

void marks_assign_remove(module_noteobj note, int type, const char* arg1, module_value arg2) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignmarkrem(type, arg1, arg2);
  } catch (const assmarkerr& e) {
    CERR << "mark not found"; modprinterr(); throw errbase();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;    
}

void marks_assign_done(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->recacheassign();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

void rstaves_assign(module_noteobj note, int staff, int clef) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignrstaff(staff, clef);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

void module_stdout(const char* str, unsigned long n) {
  ENTER_API;
  {
    boost::lock_guard<boost::mutex> xxx(outlock);
    if (n > 0) fout.write(str, n); else fout << str;
    fout.flush();
  }
  EXIT_API_VOID;  
}
void module_stderr(const char* str, unsigned long n) {
  ENTER_API;
  {
    boost::lock_guard<boost::mutex> xxx(outlock);
    if (n > 0) ferr.write(str, n); else ferr << str;
    ferr.flush();
  }
  EXIT_API_VOID;  
}

void metaparts_assign(module_noteobj note, module_partobj part, int voice, fomus_rat pitch) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignmpart(part, voice, pitch);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}
void metaparts_assign_done(module_noteobj note) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->skipassign();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

void tpose_assign_keysig(module_measobj meas, const char* str) { // new value for "keysig"
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    ((modobjbase*)meas)->assignnewkeysig(str);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;  
}

void percnotes_assign(module_noteobj note, int voice, struct fomus_rat pitch) {
  ENTER_API;
  try {
    assert(((modobjbase*)note)->isvalid());
    ((modobjbase*)note)->assignpercinst(voice, pitch);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;   
}

void markpos_assign(module_markobj mark, enum module_markpos pos) {
  ENTER_API;
  try {
    if (pos < 0 || pos > markpos_below) {
      CERR << "bad mark position";
      modprinterr();
      throw errbase();
    }
    assert(((modobjbase*)mark)->isvalid());
    return ((modobjbase*)mark)->setmarkpos(pos);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }  
  EXIT_API_VOID;
}

void markevs_assign_add(module_partobj part, int voice, struct module_value off, int type, const char* arg1, struct module_value arg2) {
  ENTER_API;
  try {
    assert(((modobjbase*)part)->isvalid());
    ((modobjbase*)part)->assigndetmark(off, voice, type, arg1, arg2);
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_VOID;     
}

struct fomus_rat module_makerat_reduce(fomus_int n, fomus_int d) {
  ENTER_API;
  rat r1(n, d);
  fomus_rat r = {r1.numerator(), r1.denominator()};
  return r;
  EXIT_API_RAT0;
}

fomus_rat module_strtonote(const char* str, module_noteparts* parts) {
  ENTER_API;
  numb val0(doparsenote(threadfd.get(), str, true, parts));
  return val0.isnull() ? makerat(std::numeric_limits<fint>::max(), 1) : numtofrat(val0);
  EXIT_API_RAT0;   
}

fomus_rat module_strtoacc(const char* str, module_noteparts* parts) {
  ENTER_API;
  numb val0(doparseacc(threadfd.get(), str, true, parts));
  return val0.isnull() ? makerat(std::numeric_limits<fint>::max(), 1) : numtofrat(val0);
  EXIT_API_RAT0;   
}

const struct module_keysigref* module_keysigref(module_measobj meas) {
  ENTER_API;
  try {
    assert(((modobjbase*)meas)->isvalid());
    return ((modobjbase*)meas)->getkeysigref();
  } catch (const wrongtype& e) {
    wrongtypeerr(e); throw errbase();
  }
  EXIT_API_0;  
}

struct fomus_rat module_makerat(fomus_int n, fomus_int d) {ENTER_API; struct fomus_rat r = {n, d}; return r; EXIT_API_RAT0;}
struct module_value module_vgracetiedendtime(module_obj obj) {ENTER_API; return module_vgraceendtime(obj); EXIT_API_MODVAL0;}
struct fomus_rat module_gracetiedendtime(module_obj obj) {ENTER_API; return module_graceendtime(obj); EXIT_API_RAT0;}
