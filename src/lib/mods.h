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

#ifndef FOMUS_MODS_H
#define FOMUS_MODS_H

#ifndef BUILD_LIBFOMUS
#error "mods.h shouldn't be included"
#endif

#include "heads.h"

#include "basetypes.h"
#include "error.h"    // CERR, errbase
#include "modtypes.h" // module_list
#include "module.h"   // modfun_init, etc..

#include "ifacedumb.h"

#include "div/grdiv.h"
#include "dumb.h"
#include "marks/markevs1.h"
#include "marks/markevs2.h"
#include "marks/marks.h"
#include "marks/smarks.h"
#include "marks/vmarks.h"
#include "meas/meas.h"
#include "parts/mparts.h"
#include "special/pnotes.h"
#include "staves/rstaves.h"
#include "voices/prune.h"

namespace fomus {

  // exceptions
  class moderr : public errbase {
  }; // ***** SAVE THIS throw when module reports an error

  // types
  class modbase;
  class fomusdata;
  typedef boost::ptr_vector<modbase> modsvect;
  typedef modsvect::iterator modsvect_it;
  typedef modsvect::const_iterator modsvect_constit;
  extern modsvect mods;

  typedef std::map<const std::string, modbase*> modsmap;
  typedef modsmap::iterator modsmap_it;
  typedef modsmap::const_iterator modsmap_constit;
  typedef modsmap::value_type modsmap_val;
  extern modsmap modsbyname;

  class modbase {
public:
    virtual ~modbase() {}

    virtual const std::string& getsname() const {
      return intname;
    }
    virtual const char* getcname() const {
      return "(internal)";
    }
    virtual const char* getcfilename() const {
      assert(false);
    }
    virtual const char* getlongname() const {
      assert(false);
    }
    virtual const char* getauthor() const {
      assert(false);
    }
    virtual const char* getdoc() const {
      assert(false);
    }
    virtual module_type gettype() const {
      assert(false);
    }
    virtual int getpriority() const {
      return 0;
    }

    virtual void exec(fomusdata* fd, void* data, const char* fn) const {
      return fireup(fd, data);
    }

    virtual const char* getiniterr() const {
      assert(false);
    }

    virtual void* getdata(FOMUS f) const {
      assert(false);
    }
    virtual void freedata(void*) const {
      assert(false);
    }

    virtual bool modin_hasext(const std::string& ext) const {
      return false;
    }
    virtual void modin_collext(std::ostream& ou, bool& fi) const {}
    virtual void modout_collext(std::ostream& ou, bool& fi) const {}
    virtual bool modin_hasloadid(const std::string& id) const {
      return false;
    }
    virtual bool modout_hasext(const std::string& ext) const {
      return false;
    }
    virtual bool modout_hasloadid(const std::string& id) const {
      return false;
    }
    virtual void modout_addext(std::vector<const char*>& v) const {}

    virtual bool loadfile(FOMUS f, void* d, const char* fn,
                          const bool isfile) const {
      assert(false);
    }
    virtual void writefile(FOMUS f, void* d, const char* fn) const {
      assert(false);
    }

    virtual bool ispre() const {
      assert(false);
    }

    virtual int getitertype() const {
      assert(false);
    }
    virtual bool getsameinst(void* a, void* b) const {
      assert(false);
    }

    virtual int getifaceid() const {
      return std::numeric_limits<int>::min();
    }
    virtual void eng_exec(void* d, const modbase& mb) const {
      assert(false);
    }
    virtual void* eng_getiface(void* d) const {
      assert(false);
    }

    virtual void fillinterface(void* moddata, void* iface) const {
      assert(false);
    }
    virtual void fillauxinterface(void* iface) const {
      assert(false);
    }

    virtual int getsetting(int n, struct module_setting* set, int id) const {
      assert(false);
    };

    void fireup(fomusdata* fd, void* data) const;

    virtual const char* whicheng(void* data) const {
      assert(false);
    }
    virtual void isready() const {}
  };

  struct dlmodstuffinit {
    // lt_dlhandle ha0;
    std::string modfile;
    std::string modname; // filename basename

    modfun_longname longname;
    modfun_author author;
    modfun_free free;
    modfun_doc doc;
    modfun_initerr initerr;
    modfun_type type;
    modfun_getsetting getsetting;
    modfun_ready ready;

    dlmodstuffinit(/*lt_dlhandle ha,*/ const std::string& modfile,
                   const std::string& modname, modfun_longname longname,
                   modfun_author author, modfun_free free, modfun_doc doc,
                   modfun_initerr initerr, modfun_type type,
                   modfun_getsetting getsetting, modfun_ready ready)
        : /*ha0(ha),*/ modfile(modfile), modname(modname), longname(longname),
          author(author), free(free), doc(doc), initerr(initerr), type(type),
          getsetting(getsetting), ready(ready) {}

    void initerrcheck() const {
      const char* e = initerr();
      if (e) {
        CERR << "error \"" << e << "\" in module `" << modname << '\''
             << std::endl;
        throw moderr();
      }
    }
    template <typename T> // just print an error
    const T& initerrwrap(const T& x) const {
      initerrcheck();
      return x;
    }
    module_type gettype() const {
      return initerrwrap(type());
    }
  };
  struct dlmodstuff : public dlmodstuffinit {
    modfun_newdata newdata;
    modfun_freedata freedata;

    dlmodstuff(/*lt_dlhandle ha,*/ const std::string& modfile,
               const std::string& modname, modfun_newdata newdata,
               modfun_freedata freedata, // modfun_err err,
               modfun_longname longname, modfun_author author, modfun_free free,
               modfun_doc doc, modfun_initerr initerr, modfun_type type,
               modfun_getsetting getsetting, modfun_ready ready)
        : dlmodstuffinit(/*ha,*/ modfile, modname, longname, author, free, doc,
                         initerr, type, getsetting, ready),
          newdata(newdata), freedata(freedata) /*, err(err)*/ {}
  };
  struct dlmodstufferr : public dlmodstuff {
    modfun_err err;
    dlmodstufferr(/*lt_dlhandle ha,*/ const std::string& modfile,
                  const std::string& modname, modfun_newdata newdata,
                  modfun_freedata freedata, modfun_err err,
                  modfun_longname longname, modfun_author author,
                  modfun_free free, modfun_doc doc, modfun_initerr initerr,
                  modfun_type type, modfun_getsetting getsetting,
                  modfun_ready ready)
        : dlmodstuff(/*ha,*/ modfile, modname, newdata, freedata, longname,
                     author, free, doc, initerr, type, getsetting, ready),
          err(err) {}
    void errcheck(void* data) const {
      const char* e = err(data);
      if (e) {
        CERR << "error \"" << e << "\" in module `" << modname << '\''
             << std::endl;
        throw moderr();
      }
    }
    void errcheck(void* data, const modbase& nm) const {
      const char* e = err(data);
      if (e) {
        CERR << "error \"" << e << "\" in module `" << nm.getcname() << '\''
             << std::endl;
        throw moderr();
      }
    }
    template <typename T> // just print an error
    const T& errwrap(void* data, const T& x) const {
      errcheck(data);
      return x;
    }
  };

  // dynamically loadable module
  template <typename T>
  class dlmodt1 : public modbase, public T {
public:
    dlmodt1(const T& st) : modbase(), T(st) {}

    const char* getiniterr() const {
      return T::initerr();
    }

    const std::string& getsname() const {
      return T::modname;
    }
    const char* getcname() const {
      return T::modname.c_str();
    }
    const char* getcfilename() const {
      return T::modfile.c_str();
    }
    const char* getlongname() const {
      return T::initerrwrap(T::longname());
    }
    const char* getauthor() const {
      return T::initerrwrap(T::author());
    }
    const char* getdoc() const {
      return T::initerrwrap(T::doc());
    }

    module_type gettype() const {
      return T::gettype();
    }

    int getsetting(int n, struct module_setting* set, int id) const {
      return T::getsetting(n, set, id);
    }

    void isready() const {
      T::ready();
      T::initerrcheck();
    }
  };
  template <typename T>
  class dlmodt2 : public dlmodt1<T> {
public:
    dlmodt2(const T& st) : dlmodt1<T>(st) {}
    void* getdata(FOMUS f) const {
      return T::initerrwrap(T::newdata(f));
    }
    void freedata(void* data) const {
      T::freedata(data);
      T::initerrcheck();
    }
  };
  typedef dlmodt1<dlmodstuffinit> dlmodinit; // auxiliary modules
  typedef dlmodt2<dlmodstuff> dlmod;         // most of the other modules
  typedef dlmodt2<dlmodstufferr>
      dlmoderr; // engines, input, output, modules that report errors

  // SPECIFIC INTERFACES
  class dlmodinout : public dlmoderr {
protected:
    modinoutfun_getext getext;
    modinoutfun_getloadid getloadid;

public:
    dlmodinout(const dlmodstufferr& st, modinoutfun_getext getext,
               modinoutfun_getloadid getloadid)
        : dlmoderr(st), getext(getext), getloadid(getloadid) {}
    bool modinout_hasext(const std::string& ext) const;
    void modinout_collext(std::ostream& ou, bool& fi) const;
    bool modinout_hasloadid(const std::string& id) const {
      const char* x = getloadid();
      initerrcheck();
      if (x == NULL)
        return false;
      return id == x;
    }
  };

  class dlmodin : public dlmodinout {
private:
    modinfun_load load;

public:
    dlmodin(const dlmodstufferr& st, modinoutfun_getext getext,
            modinfun_load load, modinoutfun_getloadid getloadid)
        : dlmodinout(st, getext, getloadid), load(load) {}
    bool loadfile(FOMUS f, void* d, const char* fn, const bool isfile) const {
      return errwrap(d, load(f, d, fn, isfile));
    } // errcheck(d);}
    bool modin_hasext(const std::string& ext) const {
      return initerrwrap(dlmodinout::modinout_hasext(ext));
    }
    void modin_collext(std::ostream& ou, bool& fi) const {
      dlmodinout::modinout_collext(ou, fi);
    }
    bool modin_hasloadid(const std::string& id) const {
      return initerrwrap(dlmodinout::modin_hasloadid(id));
    }
  };

  class dlmodout : public dlmodinout {
private:
    modfun_itertype itertype;
    modoutfun_write write;
    modoutfun_ispre isprewr;
    modfun_sameinst sameinst;

public:
    dlmodout(const dlmodstufferr& st, modinoutfun_getext getext,
             modoutfun_write write, modinoutfun_getloadid getloadid,
             modoutfun_ispre ispre, modfun_itertype itertype,
             modfun_sameinst sameinst)
        : dlmodinout(st, getext, getloadid), itertype(itertype), write(write),
          isprewr(ispre), sameinst(sameinst) {}
    void writefile(FOMUS f, void* d, const char* fn) const {
      write(f, d, fn);
      errcheck(d);
    }
    bool modout_hasext(const std::string& ext) const {
      return initerrwrap(dlmodinout::modinout_hasext(ext));
    }
    void modout_collext(std::ostream& ou, bool& fi) const {
      dlmodinout::modinout_collext(ou, fi);
    }
    bool modout_hasloadid(const std::string& id) const {
      return initerrwrap(dlmodinout::modin_hasloadid(id));
    }
    bool ispre() const {
      return initerrwrap(isprewr());
    }
    int getitertype() const {
      return initerrwrap(itertype());
    }
    void exec(fomusdata* fd, void* data, const char* fn) const {
      write((void*) fd, data, fn);
      errcheck(data);
    }
    bool getsameinst(void* a, void* b) const {
      return initerrwrap(sameinst(a, b));
    }
    void modout_addext(std::vector<const char*>& v) const;
  };

  class dlmodeng : public dlmoderr {
private:
    engauxfun_interfaceid ifaceid;
    engfun_run run;
    engfun_getinterface getiface;

public:
    dlmodeng(const dlmodstufferr& st, engauxfun_interfaceid ifaceid,
             engfun_run run, engfun_getinterface getiface)
        : dlmoderr(st), ifaceid(ifaceid), run(run), getiface(getiface) {}
    int getifaceid() const {
      return initerrwrap(ifaceid());
    }
    void eng_exec(void* d, const modbase& mb) const {
      run(d);
      errcheck(d, mb);
    }
    void* eng_getiface(void* d) const {
      return errwrap(d, getiface(d));
    }
  };

  class dlmodaux : public dlmodinit {
private:
    engauxfun_interfaceid ifaceid;
    auxfun_fillinterface filliface;

public:
    dlmodaux(const dlmodstuffinit& st, engauxfun_interfaceid ifaceid,
             auxfun_fillinterface filliface)
        : dlmodinit(st), ifaceid(ifaceid), filliface(filliface) {}
    int getifaceid() const {
      return initerrwrap(ifaceid());
    }
    void fillauxinterface(void* iface) const {
      filliface(iface);
      initerrcheck();
    }
  };

  class dlmodmod : public dlmod {
private:
    modfun_interfaceid engifaceid; // what engine type to hook up with
    modfun_fillinterface filliface;
    modfun_itertype itertype;
    modfun_priority priority;
    modfun_engine engi;
    modfun_sameinst sameinst;

public:
    dlmodmod(const dlmodstuff& st, modfun_interfaceid engifaceid,
             modfun_fillinterface filliface, modfun_itertype itertype,
             modfun_priority priority, modfun_engine eng,
             modfun_sameinst sameinst)
        : dlmod(st), engifaceid(engifaceid), filliface(filliface),
          itertype(itertype), priority(priority), engi(eng),
          sameinst(sameinst) {}
    int getpriority() const {
      return initerrwrap(priority());
    }
    int getitertype() const {
      return initerrwrap(itertype());
    } // returns module_iter_type as int
    bool getsameinst(void* a, void* b) const {
      return initerrwrap(sameinst(a, b));
    }
    void fillinterface(void* moddata, void* iface) const {
      filliface(moddata, iface); /*errcheck(moddata);*/
    }                            // up to engine to check this error
    const char* whicheng(void* data) const {
      return initerrwrap(engi(data));
    }
    int getifaceid() const {
      return initerrwrap(engifaceid());
    }
  };

  inline void printexp(const char* printerr) {
    if (printerr) {
      CERR << "expected value of type `" << printerr << '\'';
    }
  }
  inline bool validerrwrap(const bool err, const char* printerr) {
    if (!err)
      printexp(printerr);
    return err;
  }

  inline bool module_valid_listaux(const struct module_value& val, const int mi,
                                   const int ma, const char* printerr) {
    if (val.type != module_list || val.val.l.n < mi ||
        (ma >= 0 && val.val.l.n > ma)) {
      printexp(printerr);
      return false;
    }
    return true;
  }

  // MODULE DATA HOLDER
  class moddata {
private:
    const modbase& mod;
    void* data;

public:
    moddata(const modbase& mod, void* data) : mod(mod), data(data) {}
    moddata(moddata& x) : mod(x.mod), data(x.data) {
      x.data = 0;
    }
    ~moddata() {
      mod.freedata(data);
    }
    void* get() {
      return data;
    }
    const modbase& getmod() {
      return mod;
    }
  };

  // load all dlmods and query for variables
  // langmods have to explicitly register themselves
  void initmodules();
  void clearmodules();

  // used to execute backend modules
  class fomusdata;
  struct runpair {
    modbase* mb;
    std::string fn, pfn;
    runpair(modbase* mb, const std::string& fn, const std::string& pfn)
        : mb(mb), fn(fn), pfn(pfn) {}
    runpair(modbase* mb) : mb(mb) {}
    int prenum() const {
      return mb->ispre() ? 0 : 1;
    }
    void write(fomusdata* fd) const {
      moddata d(*mb, mb->getdata(fd));
      try {
        mb->writefile(fd, d.get(), fn.c_str());
      } catch (const errbase& e) {} // don't puke if backend doesn't cooperate
    }
  };

  struct stmod : public modbase {
    std::string sname;
    stmod(const char* name) : modbase(), sname(name) {}
    const char* getcfilename() const {
      return "(built-in)";
    }
    const char* getcname() const {
      return sname.c_str();
    }
    const std::string& getsname() const {
      return sname;
    }
    int getifaceid() const {
      return ENGINE_INTERFACEID;
    }
    const char* whicheng(void* data) const {
      return "dumb";
    }
  };

  struct stmod_dumb : public stmod {
    stmod_dumb() : stmod(dumb::name()) {
      dumb::init();
    }
    ~stmod_dumb() {
      dumb::free();
    }

    void errcheck(void* data) const {
      const char* e = dumb::err(data);
      if (e) {
        CERR << "error \"" << e << "\" in module `" << getcname() << '\''
             << std::endl;
        throw moderr();
      }
    }
    void errcheck(void* data, const modbase& nm) const {
      const char* e = dumb::err(data);
      if (e) {
        CERR << "error \"" << e << "\" in module `" << nm.getcname() << '\''
             << std::endl;
        throw moderr();
      }
    }
    template <typename T> // just print an error
    const T& errwrap(void* data, const T& x) const {
      errcheck(data);
      return x;
    }

    const char* getcname() const {
      return dumb::name();
    }
    const char* getlongname() const {
      return dumb::prettyname();
    }
    const char* getauthor() const {
      return dumb::author();
    }
    const char* getdoc() const {
      return dumb::doc();
    }
    void* getdata(FOMUS f) const {
      return dumb::newdata(f);
    }
    void freedata(void* dat) const {
      dumb::freedata(dat);
    }
    // int getpriority() const {return dumb::priority();}
    enum module_type gettype() const {
      return dumb::modtype();
    }
    const char* getiniterr() const {
      return dumb::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return dumb::get_setting(n, set, id);
    }
    // int getitertype() const {return dumb::itertype();}
    // bool getsameinst(void* a, void* b) const {return dumb::sameinst();}
    //     const char* whicheng(void* data) const {return dumb::engine();}
    // void fillinterface(void* moddata, void* iface) const
    // {dumb::fill_iface(moddata, iface);}
    int getifaceid() const {
      return ENGINE_INTERFACEID;
    }
    void eng_exec(void* d, const modbase& mb) const {
      dumb::run(d);
      errcheck(d, mb);
    }
    void* eng_getiface(void* d) const {
      return errwrap(d, dumb::get_iface(d));
    }
  };

  // staticly linked modules
  struct stmod_prune : public stmod NONCOPYABLE {
    stmod_prune() : stmod(prune::name()) {
      prune::init();
    }
    ~stmod_prune() {
      prune::free();
    }

    const char* getlongname() const {
      return prune::prettyname();
    }
    const char* getauthor() const {
      return prune::author();
    }
    const char* getdoc() const {
      return prune::doc();
    }
    void* getdata(FOMUS f) const {
      return prune::newdata(f);
    }
    void freedata(void* dat) const {
      prune::freedata(dat);
    }
    int getpriority() const {
      return prune::priority();
    }
    enum module_type gettype() const {
      return prune::modtype();
    }
    const char* getiniterr() const {
      return prune::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return prune::get_setting(n, set, id);
    }
    int getitertype() const {
      return prune::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return prune::sameinst();
    }
    //     const char* whicheng(void* data) const {return prune::engine();}
    void fillinterface(void* moddata, void* iface) const {
      prune::fill_iface(moddata, iface);
    }
  };
  struct stmod_vmarks : public stmod NONCOPYABLE {
    stmod_vmarks() : stmod(vmarks::name()) {
      vmarks::init();
    }
    ~stmod_vmarks() {
      vmarks::free();
    }

    const char* getlongname() const {
      return vmarks::prettyname();
    }
    const char* getauthor() const {
      return vmarks::author();
    }
    const char* getdoc() const {
      return vmarks::doc();
    }
    void* getdata(FOMUS f) const {
      return vmarks::newdata(f);
    }
    void freedata(void* dat) const {
      vmarks::freedata(dat);
    }
    int getpriority() const {
      return vmarks::priority();
    }
    enum module_type gettype() const {
      return vmarks::modtype();
    }
    const char* getiniterr() const {
      return vmarks::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return vmarks::get_setting(n, set, id);
    }
    int getitertype() const {
      return vmarks::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return vmarks::sameinst();
    }
    //     const char* whicheng(void* data) const {return vmarks::engine();}
    void fillinterface(void* moddata, void* iface) const {
      vmarks::fill_iface(moddata, iface);
    }
  };
  struct stmod_smarks : public stmod NONCOPYABLE {
    stmod_smarks() : stmod(smarks::name()) {
      smarks::init();
    }
    ~stmod_smarks() {
      smarks::free();
    }

    const char* getlongname() const {
      return smarks::prettyname();
    }
    const char* getauthor() const {
      return smarks::author();
    }
    const char* getdoc() const {
      return smarks::doc();
    }
    void* getdata(FOMUS f) const {
      return smarks::newdata(f);
    }
    void freedata(void* dat) const {
      smarks::freedata(dat);
    }
    int getpriority() const {
      return smarks::priority();
    }
    enum module_type gettype() const {
      return smarks::modtype();
    }
    const char* getiniterr() const {
      return smarks::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return smarks::get_setting(n, set, id);
    }
    int getitertype() const {
      return smarks::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return smarks::sameinst();
    }
    //     const char* whicheng(void* data) const {return smarks::engine();}
    void fillinterface(void* moddata, void* iface) const {
      smarks::fill_iface(moddata, iface);
    }
  };
  //   struct stmod_pmarks:public stmod NONCOPYABLE {
  //     stmod_pmarks():stmod(pmarks::name()) {pmarks::init();}
  //     ~stmod_pmarks() {pmarks::free();}

  //     const char* getlongname() const {return pmarks::prettyname();}
  //     const char* getauthor() const {return pmarks::author();}
  //     const char* getdoc() const {return pmarks::doc();}
  //     void* getdata(FOMUS f) const {return pmarks::newdata(f);}
  //     void freedata(void* dat) const {pmarks::freedata(dat);}
  //     int getpriority() const {return pmarks::priority();}
  //     enum module_type gettype() const {return pmarks::modtype();}
  //     const char* getiniterr() const {return pmarks::initerr();}
  //     int getsetting(int n, struct module_setting* set, int id) const {return
  //     pmarks::get_setting(n, set, id);} int getitertype() const {return
  //     pmarks::itertype();} int sameinstyvoid* a, void* bpe() const {return
  //     pmarks::sameinst();}
  // //     const char* whicheng(void* data) const {return pmarks::engine();}
  //     void fillinterface(void* moddata, void* iface) const
  //     {pmarks::fill_iface(moddata, iface);}
  //   };
  struct stmod_marks : public stmod NONCOPYABLE {
    stmod_marks() : stmod(marks::name()) {
      marks::init();
    }
    ~stmod_marks() {
      marks::free();
    }

    const char* getlongname() const {
      return marks::prettyname();
    }
    const char* getauthor() const {
      return marks::author();
    }
    const char* getdoc() const {
      return marks::doc();
    }
    void* getdata(FOMUS f) const {
      return marks::newdata(f);
    }
    void freedata(void* dat) const {
      marks::freedata(dat);
    }
    int getpriority() const {
      return marks::priority();
    }
    enum module_type gettype() const {
      return marks::modtype();
    }
    const char* getiniterr() const {
      return marks::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return marks::get_setting(n, set, id);
    }
    int getitertype() const {
      return marks::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return marks::sameinst();
    }
    //     const char* whicheng(void* data) const {return marks::engine();}
    void fillinterface(void* moddata, void* iface) const {
      marks::fill_iface(moddata, iface);
    }
  };

  struct stmod_meas : public stmod NONCOPYABLE {
    stmod_meas() : stmod(meas::name()) {
      meas::init();
    }
    ~stmod_meas() {
      meas::free();
    }

    const char* getlongname() const {
      return meas::prettyname();
    }
    const char* getauthor() const {
      return meas::author();
    }
    const char* getdoc() const {
      return meas::doc();
    }
    void* getdata(FOMUS f) const {
      return meas::newdata(f);
    }
    void freedata(void* dat) const {
      meas::freedata(dat);
    }
    int getpriority() const {
      return meas::priority();
    }
    enum module_type gettype() const {
      return meas::modtype();
    }
    const char* getiniterr() const {
      return meas::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return meas::get_setting(n, set, id);
    }
    int getitertype() const {
      return meas::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return meas::sameinst();
    }
    //     const char* whicheng(void* data) const {return meas::engine();}
    void fillinterface(void* moddata, void* iface) const {
      meas::fill_iface(moddata, iface);
    }
    void isready() const {
      meas::ready();
    }
  };
  struct stmod_rstaves : public stmod NONCOPYABLE {
    stmod_rstaves() : stmod(rstaves::name()) {
      rstaves::init();
    }
    ~stmod_rstaves() {
      rstaves::free();
    }

    const char* getlongname() const {
      return rstaves::prettyname();
    }
    const char* getauthor() const {
      return rstaves::author();
    }
    const char* getdoc() const {
      return rstaves::doc();
    }
    void* getdata(FOMUS f) const {
      return rstaves::newdata(f);
    }
    void freedata(void* dat) const {
      rstaves::freedata(dat);
    }
    int getpriority() const {
      return rstaves::priority();
    }
    enum module_type gettype() const {
      return rstaves::modtype();
    }
    const char* getiniterr() const {
      return rstaves::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return rstaves::get_setting(n, set, id);
    }
    int getitertype() const {
      return rstaves::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return rstaves::sameinst();
    }
    //     const char* whicheng(void* data) const {return rstaves::engine();}
    void fillinterface(void* moddata, void* iface) const {
      rstaves::fill_iface(moddata, iface);
    }
  };
  struct stmod_mparts : public stmod NONCOPYABLE {
    stmod_mparts() : stmod(mparts::name()) {
      mparts::init();
    }
    ~stmod_mparts() {
      mparts::free();
    }

    const char* getlongname() const {
      return mparts::prettyname();
    }
    const char* getauthor() const {
      return mparts::author();
    }
    const char* getdoc() const {
      return mparts::doc();
    }
    void* getdata(FOMUS f) const {
      return mparts::newdata(f);
    }
    void freedata(void* dat) const {
      mparts::freedata(dat);
    }
    int getpriority() const {
      return mparts::priority();
    }
    enum module_type gettype() const {
      return mparts::modtype();
    }
    const char* getiniterr() const {
      return mparts::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return mparts::get_setting(n, set, id);
    }
    int getitertype() const {
      return mparts::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return mparts::sameinst();
    }
    //     const char* whicheng(void* data) const {return mparts::engine();}
    void fillinterface(void* moddata, void* iface) const {
      mparts::fill_iface(moddata, iface);
    }
  };
  struct stmod_pnotes : public stmod NONCOPYABLE {
    stmod_pnotes() : stmod(pnotes::name()) {
      pnotes::init();
    }
    ~stmod_pnotes() {
      pnotes::free();
    }

    const char* getlongname() const {
      return pnotes::prettyname();
    }
    const char* getauthor() const {
      return pnotes::author();
    }
    const char* getdoc() const {
      return pnotes::doc();
    }
    void* getdata(FOMUS f) const {
      return pnotes::newdata(f);
    }
    void freedata(void* dat) const {
      pnotes::freedata(dat);
    }
    int getpriority() const {
      return pnotes::priority();
    }
    enum module_type gettype() const {
      return pnotes::modtype();
    }
    const char* getiniterr() const {
      return pnotes::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return pnotes::get_setting(n, set, id);
    }
    int getitertype() const {
      return pnotes::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return pnotes::sameinst();
    }
    //     const char* whicheng(void* data) const {return pnotes::engine();}
    void fillinterface(void* moddata, void* iface) const {
      pnotes::fill_iface(moddata, iface);
    }
  };
  struct stmod_markevs1 : public stmod NONCOPYABLE {
    stmod_markevs1() : stmod(markevs1::name()) {
      markevs1::init();
    }
    ~stmod_markevs1() {
      markevs1::free();
    }

    const char* getlongname() const {
      return markevs1::prettyname();
    }
    const char* getauthor() const {
      return markevs1::author();
    }
    const char* getdoc() const {
      return markevs1::doc();
    }
    void* getdata(FOMUS f) const {
      return markevs1::newdata(f);
    }
    void freedata(void* dat) const {
      markevs1::freedata(dat);
    }
    int getpriority() const {
      return markevs1::priority();
    }
    enum module_type gettype() const {
      return markevs1::modtype();
    }
    const char* getiniterr() const {
      return markevs1::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return markevs1::get_setting(n, set, id);
    }
    int getitertype() const {
      return markevs1::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return markevs1::sameinst();
    }
    //     const char* whicheng(void* data) const {return markevs1::engine();}
    void fillinterface(void* moddata, void* iface) const {
      markevs1::fill_iface(moddata, iface);
    }
  };
  struct stmod_markevs2 : public stmod NONCOPYABLE {
    stmod_markevs2() : stmod(markevs2::name()) {
      markevs2::init();
    }
    ~stmod_markevs2() {
      markevs2::free();
    }

    const char* getlongname() const {
      return markevs2::prettyname();
    }
    const char* getauthor() const {
      return markevs2::author();
    }
    const char* getdoc() const {
      return markevs2::doc();
    }
    void* getdata(FOMUS f) const {
      return markevs2::newdata(f);
    }
    void freedata(void* dat) const {
      markevs2::freedata(dat);
    }
    int getpriority() const {
      return markevs2::priority();
    }
    enum module_type gettype() const {
      return markevs2::modtype();
    }
    const char* getiniterr() const {
      return markevs2::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return markevs2::get_setting(n, set, id);
    }
    int getitertype() const {
      return markevs2::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return markevs2::sameinst();
    }
    //     const char* whicheng(void* data) const {return markevs2::engine();}
    void fillinterface(void* moddata, void* iface) const {
      markevs2::fill_iface(moddata, iface);
    }
    void isready() const {
      markevs2::ready();
    }
  };
  struct stmod_grdiv : public stmod NONCOPYABLE {
    stmod_grdiv() : stmod(grdiv::name()) {
      grdiv::init();
    }
    ~stmod_grdiv() {
      grdiv::free();
    }

    const char* getlongname() const {
      return grdiv::prettyname();
    }
    const char* getauthor() const {
      return grdiv::author();
    }
    const char* getdoc() const {
      return grdiv::doc();
    }
    void* getdata(FOMUS f) const {
      return grdiv::newdata(f);
    }
    void freedata(void* dat) const {
      grdiv::freedata(dat);
    }
    int getpriority() const {
      return grdiv::priority();
    }
    enum module_type gettype() const {
      return grdiv::modtype();
    }
    const char* getiniterr() const {
      return grdiv::initerr();
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return grdiv::get_setting(n, set, id);
    }
    int getitertype() const {
      return grdiv::itertype();
    }
    bool getsameinst(void* a, void* b) const {
      return grdiv::sameinst();
    }
    //     const char* whicheng(void* data) const {return grdiv::engine();}
    void fillinterface(void* moddata, void* iface) const {
      grdiv::fill_iface(moddata, iface);
    }
  };

  // DON'T FORGET TO ADD A LINE IN MODS.CC (AROUND LINE 230)

} // namespace fomus

#endif
