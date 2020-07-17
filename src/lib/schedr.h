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

#ifndef FOMUS_SCHEDR_H
#define FOMUS_SCHEDR_H

#ifndef BUILD_LIBFOMUS
#error "sched.h shouldn't be included"
#endif

#include "heads.h"

#include "data.h" // fomusdata

namespace fomus {

  struct flags {
    const bool iter_nograce, iter_graceonly,
      iter_norests, iter_restsonly,
      iter_firsttied, iter_midtied, iter_lasttied, iter_anytied,
      iter_noperc, iter_perconly,
      iter_byvoice,
      iter_bystaff,
      iter_noinvnotes;
    int voice, staff;
    bool invvoicesonly;
    flags(const int ty, const int voice, const int staff, const bool invvoicesonly):
      iter_nograce(ty & module_nograce),
      iter_graceonly(ty & module_graceonly),
      iter_norests(ty & module_norests),
      iter_restsonly(ty & module_restsonly),
      iter_firsttied(ty & module_firsttied),
      iter_midtied(ty & module_midtied),
      iter_lasttied(ty & module_lasttied),
      iter_anytied(ty & (module_firsttied | module_midtied | module_lasttied)),
      iter_noperc(ty & module_noperc),
      iter_perconly(ty & module_perconly),
      iter_byvoice(ty & module_byvoice),
      iter_bystaff(ty & module_bystaff),
      iter_noinvnotes(ty & module_noinvisible),
      voice(voice), staff(staff), invvoicesonly(invvoicesonly) {}
    bool dontwantnote(const noteevbase& nref) const {
      assert(nref.isntlocked());
      return ((iter_byvoice && voice != nref.getvoice())
	      || (iter_bystaff && staff != nref.getstaff())
	      || (nref.getisgrace() ? iter_nograce : iter_graceonly)
	      || (nref.getisrest() ? iter_norests : iter_restsonly)
	      || (nref.getisperc() ? iter_noperc : iter_perconly)
	      || (iter_anytied
		  && ((!iter_firsttied && nref.isfirsttied()) 
		      || (!iter_midtied && nref.ismidtied()) 
		      || (!iter_lasttied && nref.islasttied())))
	      || ((iter_noinvnotes || nref.getisnote()) && nref.isinvisible())
	      || (invvoicesonly && nref.getvoice() < 1000)
	      );
    }
    bool separate(const flags& x) const { // no conflict
      return ((voice > 0 && x.voice > 0 && voice != x.voice)
	      || (staff > 0 && x.staff > 0 && staff != x.staff)
	      || (iter_nograce && x.iter_graceonly) || (iter_graceonly && x.iter_nograce)
	      || (iter_norests && x.iter_restsonly) || (iter_restsonly && x.iter_norests)
	      || (iter_noperc && x.iter_perconly) || (iter_perconly && x.iter_noperc)
	      || (iter_anytied && x.iter_anytied && (!iter_firsttied || !x.iter_firsttied) && (!iter_midtied || !x.iter_midtied) && (!iter_lasttied || !x.iter_lasttied))
	      );
    }
  };

  struct syncs;
  class stage {
    fomus_int id;
    const std::string msg;
    syncs& sys;
    const modbase& mod;
    scorepartlist_it part, initpart, mitpart, pitpart; // itpart and itmeas used to iterate through measures, initpart & initmeas are the first ones
    measmap_it meas, initmeas, mitmeas;
    eventmap_it note;
    bool firstnote, firstmeas, firstpart, isfirst, done, mitdone, pitdone; // isfirst is for displaying a message, done is for finished getting notes (but not updating)
    fint checkin, checkout; // for notes
    flags fl; 
    scorepartlist_it partend;
    measmap_it measend;
    eventmap_it noteend;
    const char* filename;
    bool istmpmeas;
#ifndef NDEBUG    
    int debugvalid;
#endif
    bool err;
  public:
    boost::condition_variable_any wkup;
  public:
    stage(fomus_int& id0, const std::string& msg, syncs& sys, const modbase& mod, const scorepartlist_it& part, const measmap_it& meas,
	  const bool first, const int ty, const int voice, const int staff, const char* filename, const bool invvoicesonly):
      id(id0++), msg(first ? msg : std::string()), sys(sys), 
      mod(mod), part(part), initpart(part), meas(meas), initmeas(meas), note(meas->second->getevents().begin()),
      firstnote(true), firstmeas(true), firstpart(true), isfirst(first),
      done(false), mitdone(false), pitdone(false), checkin(0), checkout(0),
      fl(ty, voice, staff, invvoicesonly), filename(filename), istmpmeas(false), err(false) {
#ifndef NDEBUG
      debugvalid = 12345;
#endif      
    }
    stage(fomus_int& id0, const std::string& msg, syncs& sys, const modbase& mod, const measmap_it& meas1, // special parts list constructor
	  const measmap_it& meas2, const bool first, const int ty, const int voice, const int staff, const char* filename, const bool invvoicesonly):
      id(id0++), msg(first ? msg : std::string()), sys(sys), 
      mod(mod), meas(meas1), initmeas(meas1), note(meas1->second->getevents().begin()),
      firstnote(true), firstmeas(true), firstpart(true), isfirst(first),
      done(false), mitdone(false), pitdone(true), checkin(0), checkout(0),
      fl(ty, voice, staff, invvoicesonly), measend(meas2), noteend(boost::prior(meas2)->second->getevents().end()), filename(filename), istmpmeas(true), err(false) {
#ifndef NDEBUG
      debugvalid = 12345;
#endif      
    }
#ifndef NDEBUG
    ~stage() {debugvalid = 0;}
#endif    
#ifndef NDEBUG
    bool isvalid() const {return debugvalid == 12345;}
#endif      
    void resetnoteits() { // called from writeout
      assert(isvalid()); 
      note = meas->second->getevents().begin();
      assert(boost::prior(measend)->second->isvalid());
      noteend = boost::prior(measend)->second->getevents().end();
    }
    void incid(const fomus_int i) {id += i;}
    syncs& getsys() const {return sys;}
    modobjbase* api_nextnote();
    modobjbase* api_nextmeas();
    modobjbase* api_nextpart();
    noteevbase* api_peeknextnote(const modobjbase* note);
    modobjbase* api_peeknextmeas(const modobjbase* meas);
    modobjbase* api_peeknextpart(const modobjbase* part);
    void post_apisetvalue(noteevbase& note);
    void exec(fomusdata* fd);
    void printmsg() {
      assert(isvalid()); 
      if (!msg.empty()) {
	boost::lock_guard<boost::mutex> xxx(outlock);
	fout << msg << std::endl;
      }
      isfirst = false;
    }
    const modbase& getmod() const {assert(isvalid()); return mod;}
    void setends(const scorepartlist_it& p, const measmap_it& e, const eventmap_it& ne) {assert(isvalid()); partend = p; measend = e; noteend = ne;}
    bool conflicts(const flags& x) const {assert(isvalid()); return !fl.separate(x);}
    const flags& getflags() const {assert(isvalid()); return fl;}
    fint nextcheckout() {assert(isvalid()); return checkout++;}
    fomus_int getid() const {assert(isvalid()); return id;}
    void seterr() {assert(isvalid()); err = true;}
    void reseterr() {assert(isvalid()); err = false;}
    int geterr() const {assert(isvalid()); return err;}
  };

  inline bool stageless::operator()(const stage* x, const stage* y) const {return x->getid() < y->getid();}
  
  inline void measure::stagedone() {boost::unique_lock<boost::shared_mutex> xxx(wakeupsmut); stages.erase(stageobj.get());}
  inline void measure::stagedone_nomut() {stages.erase(stageobj.get());}

  inline void modprinterr() {ferr << " in module `" << stageobj->getmod().getsname() << '\'' << std::endl;}

  typedef boost::ptr_vector<stage> stagesvect;
  typedef stagesvect::iterator stagesvect_it;
  typedef stagesvect::const_iterator stagesvect_constit;

  inline numb noteev::gettiedendtime() const {READLOCK; return RMUT(tiedr) ? (UNLOCKP((noteev*)RMUT(tiedr)))->gettiedendtime(meas) : getendtime_nomut();}
  inline numb noteev::gettiedendtime(measure* m) const {
    READLOCK;
    return RMUT(tiedr) ? (UNLOCKP((noteev*)RMUT(tiedr)))->gettiedendtime(meas) : getendtime_nomut();
  }
  inline noteevbase* noteevbase::getnextnoteevbase() const {
    return stageobj->api_peeknextnote(this);
  }

  inline const char* staff_str::getprintstr() const { // stageobj must be valid, called from modinout
    std::ostringstream ss;
    print(ss, threadfd.get());
    return make_charptr(ss);
  }
  inline const char* percinstr_str::getprintstr() const {
    std::ostringstream ss;
    print(ss, threadfd.get(), true);
    return make_charptr(ss);
  }
  inline const char* instr_str::getprintstr() const {
    std::ostringstream ss;
    print(ss, threadfd.get(), true);
    return make_charptr(ss);
  }
  inline const char* part_str::getprintstr() const {
    std::ostringstream ss;
    print(ss, threadfd.get(), true);
    return make_charptr(ss);
  }
  inline const char* partmap_str::getprintstr() const {
    std::ostringstream ss;
    print(ss, threadfd.get());
    return make_charptr(ss);
  }
  inline const char* mpart_str::getprintstr() const {
    std::ostringstream ss;
    print(ss, threadfd.get(), true);
    return make_charptr(ss);
  }
  inline const char* measdef_str::getprintstr() const {
    std::ostringstream ss;
    print(ss, threadfd.get(), true);
    return make_charptr(ss);
  }

  void getsettinginfo_aux(const setmap& sets, scoped_info_setlist& setlist);

  inline info_setlist& str_base::getsettinginfo() { // only called from modinout
    getsettinginfo_aux(sets, setlist);
    return setlist;
  }

  inline info_setlist& event::getsettinginfo() { // only called from modinout
    getsettinginfo_aux(sets, setlist);
    return setlist;
  }

  inline bool part_str::gethasclef(const int clef) {
    return std::binary_search(clefscache.begin(), clefscache.end(), clef);
  }
  inline void partormpart_str::assigngroupbegin(const parts_grouptype type) {prt->assigngroupbegin(threadfd->grpcnt, type);}
  inline void partormpart_str::assigngroupend(const parts_grouptype type) {prt->assigngroupend(threadfd->grpcnt++, type);}
  
  inline void measure::assigndivs(const module_ratslist& divs0) {
    WRITELOCK;
    if (!RMUT(divs).empty()) {
      CERR << "value update out of sequence";
      modprinterr();
      throw errbase();
    }
    std::copy(divs0.rats, divs0.rats + divs0.n, std::back_inserter(WMUT(divs)));
  }

  const varbase& fom_get_varbase_up(const int id); // only called from module, so stageobj must be valid
  fint fom_get_ival_up(const int id);
  rat fom_get_rval_up(const int id);
  ffloat fom_get_fval_up(const int id);
  const std::string& fom_get_sval_up(const int id);
  const module_value& fom_get_lval_up(const int id);

}

#endif
