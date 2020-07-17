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

#include <cassert>

#include "intern.h"
#include "ifacedumb.h"

#include "instrs.h"
#include "module.h"
#include "classes.h"
#include "algext.h"
#include "data.h"
#include "schedr.h"

namespace fomus {

#define CASTNOTEEVBASE(xxx) ((noteevbase*)(modobjbase*)(xxx))
#define CASTNOTEEV(xxx) ((noteev*)(modobjbase*)(xxx))
#define CASTRESTEV(xxx) ((restev*)(modobjbase*)(xxx))
#define CASTEVENT(xxx) ((event*)(modobjbase*)(xxx))
#define CASTMARKOBJ(xxx) ((markobj*)(modobjbase*)(xxx))
#define CASTMEASURE(xxx) ((measure*)(modobjbase*)(xxx))
#define CASTPARTORMPART(xxx) ((partormpart_str*)(modobjbase*)(xxx))
#define CASTFOMUSDATA(xxx) ((fomusdata*)(modobjbase*)(xxx))

#ifndef NDEBUG
#define INTREADLOCK(zzz) lcheck zzz000; shlock xxx(CASTEVENT(zzz)->interngetmut())
#define INTUPREADLOCK(zzz) lcheck zzz000; uplock xxx(CASTEVENT(zzz)->interngetmut())
#define INTTOWRITELOCK boost::upgrade_to_unique_lock<boost::shared_mutex> yyy(xxx)
#define INTWRITELOCK(zzz) lcheck zzz000; boost::unique_lock<boost::shared_mutex> yyy(CASTEVENT(zzz)->interngetmut())
#else
#define INTREADLOCK(zzz) shlock xxx(CASTEVENT(zzz)->interngetmut())
#define INTUPREADLOCK(zzz) uplock xxx(CASTEVENT(zzz)->interngetmut())
#define INTTOWRITELOCK boost::upgrade_to_unique_lock<boost::shared_mutex> yyy(xxx)
#define INTWRITELOCK(zzz) boost::unique_lock<boost::shared_mutex> yyy(CASTEVENT(zzz)->interngetmut())
#endif
  
  std::string toromanstr(const int n);
  
  template<typename T>
  inline void int_skipassign(T* note) {
    note->skipassign();
  }
  template<>
  inline void int_skipassign<void>(void* note) {
    ((noteevbase*)(modobjbase*)note)->skipassign();
  }

  const char* internalerr_fun(void* moddata) {return 0;}
  
  // at end of a pass--don't need to assign 
  void postmeasdoit(FOMUS fom, void* moddata) { // BY PART
    while (true) { // eat the notes--this is last in substage section --it's okay
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      int_skipassign(n); // skipassign write-locks the note
    }
    DISABLEMUTCHECK;
    CASTPARTORMPART(stageobj->api_nextpart())->fixmeasures();
    assert(!stageobj->api_nextpart());
  }

  void inbetweenmarksdoit(FOMUS fom, void* moddata) { // by part/voice
    module_noteobj ln = stageobj->api_nextnote();
    bool ass = false;
    if (ln) {
      while (true) {
	module_noteobj n = stageobj->api_nextnote();
	if (!n) break;
	if (CASTNOTEEVBASE(n)->getfulltime() > CASTNOTEEVBASE(ln)->getfulltime()) {
	  module_markslist ml(CASTNOTEEVBASE(n)->getsinglemarks());
	  for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) {
	    switch (CASTMARKOBJ(*m)->getmarkid()) {
	    case mark_gliss_before: {
	      {
		INTWRITELOCK(n);
		CASTNOTEEVBASE(n)->assignmarkrem(mark_gliss_before, 0, numb(module_none));
	      } {
		INTWRITELOCK(ln);
		CASTNOTEEVBASE(ln)->assignmarkins(mark_gliss_after, 0, numb(module_none));
	      }
	      ass = true;
	      break;
	    }
	    case mark_port_before: {
	      {
		INTWRITELOCK(n);
		CASTNOTEEVBASE(n)->assignmarkrem(mark_port_before, 0, numb(module_none));
	      } {
		INTWRITELOCK(ln);
		CASTNOTEEVBASE(ln)->assignmarkins(mark_port_after, 0, numb(module_none));
	      }
	      ass = true;
	      break;
	    }
	    case mark_breath_before: {
	      {
		INTWRITELOCK(n);
		CASTNOTEEVBASE(n)->assignmarkrem(mark_breath_before, 0, numb(module_none));
	      } {
		INTWRITELOCK(ln);
		CASTNOTEEVBASE(ln)->assignmarkins(mark_breath_after, 0, numb(module_none));
	      }
	      ass = true;
	      break;
	    }
	    case mark_break_before: {
	      {
		INTWRITELOCK(n);
		CASTNOTEEVBASE(n)->assignmarkrem(mark_break_before, 0, numb(module_none));
	      } {
		INTWRITELOCK(ln);
		CASTNOTEEVBASE(ln)->assignmarkins(mark_break_after, 0, numb(module_none));
	      }
	      ass = true;
	      break;
	    }
	    }
	  }
	}
	if (ass) {
	  ass = false;
	  INTWRITELOCK(ln);
	  CASTNOTEEVBASE(ln)->recacheassign();
	} else CASTNOTEEVBASE(ln)->post_apisetvalue();
	ln = n;
      }
      assert(!ass);
      CASTNOTEEVBASE(ln)->post_apisetvalue();
    }
  }

  void sysbreakdoit(FOMUS fom, void* moddata) { // by all
    module_measobj me = 0;
    module_noteobj n = stageobj->api_peeknextnote(0);
    std::set<rat> ms;
    std::set<module_noteobj> no;
    while (true) {
      me = stageobj->api_peeknextmeas(CASTMEASURE(me));
      if (!me) break;
      while (n && CASTNOTEEVBASE(n)->getmeasobj() == (modobjbase*)me) {
	module_markslist ml(CASTNOTEEVBASE(n)->getsinglemarks());
	for (const module_markobj *m(ml.marks), *me0(ml.marks + ml.n); m < me0; ++m) {
	  if (CASTMARKOBJ(*m)->getmarkid() == mark_break_after) {
	    CASTNOTEEVBASE(n)->assignmarkrem(mark_break_after, 0, numb(module_none));
	    no.insert(n);
	    ms.insert(numtorat(CASTMEASURE(me)->getendtime()));
	  }
	}
	n = stageobj->api_peeknextnote((modobjbase*)n);
      }
    }
    n = stageobj->api_nextnote();
    while (true) {
      me = stageobj->api_nextmeas();
      if (!me) break;
      bool fi = (ms.find(numtorat(CASTMEASURE(me)->getendtime())) != ms.end());
      while (n && CASTNOTEEVBASE(n)->getmeasobj() == me) {
	if (fi) {
	  CASTNOTEEVBASE(n)->assignmarkins(mark_break_after, 0, numb(module_none));
	  CASTNOTEEVBASE(n)->recacheassign();
	  fi = false;
	} else {
	  if (no.find(n) != no.end()) CASTNOTEEVBASE(n)->recacheassign(); else CASTNOTEEVBASE(n)->post_apisetvalue();
	}
	n = stageobj->api_nextnote();
      }
    }
    assert(!n);
  }
  
  // at end of a pass--don't need to assign 
  void posttquantdoit(FOMUS fom, void* moddata) { // BY ALL
    while (true) { // eat the notes--this is last in substage section
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      int_skipassign(n);
    }
    DISABLEMUTCHECK;
    numb tr((fint)-1); // measures to trim off end
    while (true) {
      module_partobj p = stageobj->api_nextpart();
      if (!p) break;
      numb r(CASTPARTORMPART(p)->fixtimequant());
      if (r > tr) tr = r;
    }
    assert(tr >= (fint)0);
    module_partobj p = 0;
    while (true) {
      p = stageobj->api_peeknextpart((modobjbase*)p);
      if (!p) break;
      CASTPARTORMPART(p)->trimmeasures(numtofrat(tr)); // get rid of extra measures
    }
  }
  
  void posttquantinvdoit(FOMUS fom, void* moddata) { // BY ALL
    while (true) { // eat the notes--this is last in substage section
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      int_skipassign(n);
    }
    DISABLEMUTCHECK;
    //numb tr((fint)-1); // measures to trim off end
    while (true) {
      module_partobj p = stageobj->api_nextpart();
      if (!p) break;
      CASTPARTORMPART(p)->fixtimequantinv();
    }
    // module_partobj p = 0;
    // while (true) {
    //   p = stageobj->api_peeknextpart((modobjbase*)p);
    //   if (!p) break;
    //   CASTPARTORMPART(p)->trimmeasures(numtofrat(tr)); // get rid of extra measures
    // }
  }
  
  void postpquantdoit(FOMUS fom, void* moddata) { // BY MEAS, NORESTS, NOPERCS, FIRSTONLY
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      assert(CASTNOTEEVBASE(n)->getisnote());
      CASTNOTEEV(n)->checkpquant();
    }
  }
  
  void postvoicesdoit(FOMUS fom, void* moddata) { // BY MEAS
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      CASTNOTEEVBASE(n)->checkvoices();
    }
    CASTMEASURE(stageobj->api_nextmeas())->checkrestvoices();
  }
  
  // must be the last in section
  void poststavesdoit_aux(boost::ptr_map<const int, std::vector<int> >& srvs, std::list<noteev*>& objs) {
    for (boost::ptr_map<const int, std::vector<int> >::iterator si(srvs.begin()); si != srvs.end(); ++si) {
      std::vector<int>& clfs = *si->second;
      assert(!clfs.empty());
      std::sort(clfs.begin(), clfs.end());
      int cid = clfs[clfs.size() / 2]; // round up (higher clefs break the tie)
      for (std::list<noteev*>::iterator i(objs.begin()); i != objs.end(); ) {
	INTUPREADLOCK(*i);
	if ((*i)->get1staff() == si->first) {
	  //if ((*i)->maybereplaceclef(cid)) { // if different clef...
#ifndef NDEBUG
	    (*i)->replaceclef(cid, xxx, zzz000);
#else
	    (*i)->replaceclef(cid, xxx);
#endif
	  //}
	  i = objs.erase(i);
	} else ++i;
      }
    }
    assert(objs.empty());
  }
  void poststavesdoit(FOMUS fom, void* moddata) { // BY MEAS, NORESTS, NOPERCS, FIRSTONLY--rests are dealt with later (when measure divs are created)--(use this same fun)
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      CASTNOTEEVBASE(n)->checkstaves();
    } // do not disable mutex checking!  next thread might access events in this thread
    boost::ptr_map<const int, std::vector<int> > srvs; // staff, clefs
    std::list<noteev*> objs;
    offgroff ceo((fomus_int)-1); // last offset, grab all including overlaps
    module_noteobj n = 0;
    while (true) {
      n = stageobj->api_peeknextnote((modobjbase*)n);
      if (!n) {poststavesdoit_aux(srvs, objs); break;}
      if (CASTNOTEEVBASE(n)->checkstaves0()) continue; // if not first tied note
      assert(CASTNOTEEVBASE(n)->getisnote());
      if (ceo < CASTNOTEEV(n)->getfulltime()) {
	poststavesdoit_aux(srvs, objs); // process only chords, make sure they all have same staff/clef
	srvs.clear();
      } 
      objs.push_back(CASTNOTEEV(n)); // gather full chord
      INTREADLOCK(n);
      ceo = CASTNOTEEV(n)->getfulltime_nomut();
      int st = CASTNOTEEV(n)->getstaff_nomut();
      boost::ptr_map<int, std::vector<int> >::iterator i(srvs.find(st));
      if (i == srvs.end()) srvs.insert(st, new std::vector<int>(1, CASTNOTEEV(n)->getclefid()));
      else i->second->push_back(CASTNOTEEV(n)->getclefid());
    }
  }
  // this one is for rests
  void poststavesdoit2(FOMUS fom, void* moddata) { // BY MEAS, RESTSONLY, NOPERCS, FIRSTONLY--rests are dealt with later (when measure divs are created)--(use this same fun)
    DBG("we're at poststavesdoit2 now" << std::endl);
#ifndef NDEBUGOUT
    ((fomusdata*)fom)->dumpall();
#endif    
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      //assert(CASTNOTEEVBASE(n)->getisrest());
      CASTNOTEEVBASE(n)->checkstaves2();
    }    
  }

  void fillnotes1doit(FOMUS fom, void* moddata) { // BY PART, BYVOICE
    offgroff off((fint)0), off0((fint)0);
    std::list<noteevbase*> tmp;
    partormpart_str& p = *CASTPARTORMPART(stageobj->api_nextpart());
    offgroff lmeo(p.getlastmeasendoff());
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      offgroff thisoff(CASTNOTEEVBASE(n)->getfulltime());
      module_noteobj nn0 = stageobj->api_peeknextnote((modobjbase*)n); // next note
      module_noteobj nn = nn0;
      while (nn && CASTNOTEEVBASE(nn)->getfulltime() == thisoff) {nn = stageobj->api_peeknextnote((modobjbase*)nn);} // next offset note
      assert(!nn || !(CASTNOTEEVBASE(nn)->getfulltime() < thisoff));
      offgroff noff(nn ? CASTNOTEEVBASE(nn)->getfulltime() : lmeo);
      {
	INTUPREADLOCK(n);
	pointtype pt = CASTNOTEEVBASE(n)->getpoint_nomut();
	if (pt & (point_left | point_grleft)) {
	  CASTNOTEEVBASE(n)->dopointl(off, tmp, LOCKARG); // an autodur note reduced to a "point"
	} else if (pt & (point_right | point_grright)) {
	  CASTNOTEEVBASE(n)->dopointr(noff, tmp, LOCKARG);
	}
	offgroff eo(CASTNOTEEVBASE(n)->getendtime_nomut()); // should always have one now!
	if (eo > off0) off0 = eo;
      }
      if (!nn0 || CASTNOTEEVBASE(nn0)->getfulltime() > CASTNOTEEVBASE(n)->getfulltime()) off = off0;
      CASTNOTEEVBASE(n)->post_apisetvalue();
    }
    for (std::list<noteevbase*>::iterator i(tmp.begin()); i != tmp.end(); ) {
      assert((*i)->isvalid());
      p.insertnew((*i++)->releaseme()); // put into tmpevs
    }
  }
  
  void fillnotes2doit(FOMUS fom, void* moddata) {
    std::list<noteevbase*> tmp;
    partormpart_str& p = *CASTPARTORMPART(stageobj->api_nextpart());
    offgroff lmeo(CASTMEASURE(stageobj->api_nextmeas())->getendtime() /*p.getlastmeasendoff()*/);
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      {
	pointtype x = CASTNOTEEVBASE(n)->getpoint();
	if (x & (point_auto | point_grauto)) {
	  offgroff thisoff(CASTNOTEEVBASE(n)->getfulltime());
	  module_noteobj nn = n;
	  do {
	    nn = stageobj->api_peeknextnote((modobjbase*)nn);
	    DBG("maybe it's this one: " << (nn ? CASTNOTEEVBASE(nn)->getfulltime() : lmeo).off << "," << (nn ? CASTNOTEEVBASE(nn)->getfulltime() : lmeo).groff << std::endl);
	  } while (nn && (x == point_grauto ? CASTNOTEEVBASE(nn)->getfulltime() <= thisoff : CASTNOTEEVBASE(nn)->gettime() <= thisoff.off));
	  assert(!nn || (CASTNOTEEVBASE(nn)->getfulltime() > thisoff));
	  DBG("doing dopointr at " << thisoff.off << "," << thisoff.groff << " up to " <<
	      (nn ? CASTNOTEEVBASE(nn)->getfulltime() : lmeo).off << "," << (nn ? CASTNOTEEVBASE(nn)->getfulltime() : lmeo).groff <<
	      " " << (x == point_grauto ? "GRauto" : "auto") << std::endl);
	  offgroff x(nn ? CASTNOTEEVBASE(nn)->getfulltime() : lmeo);
	  INTUPREADLOCK(n);
	  CASTNOTEEVBASE(n)->dopointr(x, tmp, LOCKARG);
	}
      }
      CASTNOTEEVBASE(n)->post_apisetvalue();
    }    
    for (std::list<noteevbase*>::iterator i(tmp.begin()); i != tmp.end(); ) {
      assert((*i)->isvalid());
      p.insertnew((*i++)->releaseme()); // put into tmpevs
    }
  }

  void postprunedoit(FOMUS fom, void* moddata) { // BY MEAS, NORESTS, NOPERCS, FIRSTONLY--rests are dealt with later (when measure divs are created)
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      int_skipassign(n);
    }
    DISABLEMUTCHECK;
    module_measobj m = stageobj->api_nextmeas();
    CASTMEASURE(m)->doprune();    
    CASTMEASURE(m)->checkprune();
    DBG("done with postprune" << std::endl);
  }

  void postmarkevsdoit(FOMUS fom, void* moddata) { // BY MEAS, must be last!!!
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      int_skipassign(n);
    }
    DISABLEMUTCHECK;
    CASTMEASURE(stageobj->api_nextmeas())->dopostmarkevs();
  }

  // remove duplicate marks
  void prevspandoit(FOMUS fom, void* moddata) { // by PART, VOICE
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      CASTNOTEEVBASE(n)->remdupmarks();
    }    
  }

  inline void dobarlines(measure& m1, const measure& m2) {
    DBG("dobarlines " << m1.gettime() << " and " << m2.gettime() << std::endl);
    assert(barlines.find(m1.get_sval(BARLINER_ID)) != barlines.end());
    assert(barlines.find(m2.get_sval(BARLINEL_ID)) != barlines.end());
    module_barlines
      l = barlines.find(m1.get_sval(BARLINER_ID))->second,
      r = barlines.find(m2.get_sval(BARLINEL_ID))->second;
    if (l < r) std::swap(l, r); // make sure l is bigger
    DBG("left is `" << m1.get_sval(BARLINEL_ID) << "' and right is `" << m2.get_sval(BARLINER_ID) << "'" << std::endl);
    DBG("left is " << l << " and right is " << r << std::endl);
    if (l == barline_final && r == barline_initial) l = barline_initfinal;
    else if (l == barline_repeatright && r == barline_repeatleft) l = barline_repeatleftright;
    m1.setbarline(l);
  }
  inline void dobarliner(measure& m) {
    m.setbarline(barlines.find(m.get_sval(BARLINER_ID))->second);
  }
  inline void dobarliner2(measure& m) {
    module_barlines r = barlines.find(m.get_sval(BARLINER_ID))->second;
    if (r > barline_normal) m.setbarline(r);
  }
  inline void dobarlinel(measure& m1, measure& m2) {
    module_barlines r = barlines.find(m2.get_sval(BARLINEL_ID))->second;
    m1.setbarline(r);
  }
  void barlinesdoit(FOMUS fom, void* moddata) { // by PART, VOICE
    while (true) { // this can update measure info independent of other stages
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      CASTNOTEEVBASE(n)->post_apisetvalue();
    }
    module_measobj lm = 0;
    bool lnw = false;
    while (true) {
      module_measobj m = stageobj->api_nextmeas();
      if (!m) {
	assert(lm);
	if (CASTFOMUSDATA(fom)->get_ival(FINALBAR_ID)) CASTMEASURE(lm)->setbarline(barline_final);
	if (lnw) dobarliner2(*CASTMEASURE(lm));
	break;
      }
      bool nw = lm && CASTMEASURE(lm)->isntsameblock(*CASTMEASURE(m));
      if (lnw && nw) {
	assert(lm); assert(m);
	dobarlines(*CASTMEASURE(lm), *CASTMEASURE(m));
      } else {
	if (lnw) {assert(lm); dobarliner(*CASTMEASURE(lm));}
	if (nw) {assert(lm); assert(m); dobarlinel(*CASTMEASURE(lm), *CASTMEASURE(m));}
      }
      lm = m;
      lnw = nw;
    }
  }

  enum splittremsenum {
    spltrems_none, spltrems_sing, spltrems_doub
  };

  void splittremsaux(std::vector<noteevbase*>& nos, const numb& spl, const splittremsenum xx, const int era, const bool inv) {
    DBG("START AUX" << std::endl);
    if (xx != spltrems_none) {
      bool t1 = false, t2 = false, t0 = false;
      for (std::vector<noteevbase*>::iterator i(nos.begin()); i != nos.end(); ++i) {
	if (!CASTNOTEEVBASE(*i)->getisnote()) continue;
	module_markslist ml(CASTNOTEEV(*i)->getsinglemarks());
	for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) {
	  switch (CASTMARKOBJ(*m)->getmarkid()) {
	  case mark_trem:
	    switch (xx) {
	    case spltrems_sing: {
	      INTWRITELOCK(*i);
	      CASTNOTEEV(*i)->trem_fix(spl, WRLOCKARG); // spl is incase tremolo doesn't divide properly
	      t0 = true;
	      goto STLOOP;
	    }
	    case spltrems_doub: {
	      if (era == 2) { // only first tremchord (maybe w/ split)
		if (spl.isnull()) { // no split
		  if (inv) CASTNOTEEV(*i)->trem_dur0(module_none);
		} else if (inv) CASTNOTEEV(*i)->trem_incoff0(spl); else CASTNOTEEV(*i)->trem_halfdur0(spl); // inv must also switch trem marks w/ trem2 marks
	      } else if (inv) {
		CASTNOTEEV(*i)->trem_incoff(spl);
		if (!era) CASTNOTEEV(*i)->switchtrems(mark_trem, mark_trem2);
	      } else CASTNOTEEV(*i)->trem_halfdur(spl); // off = 18, 1/8 beams don't fit
	      t1 = true;
	      goto STLOOP;
	    }
	    default: ;
	    }
	    break;
	  case mark_trem2: {
	    if (era == 2) {
	      if (spl.isnull()) {
		if (!inv) CASTNOTEEV(*i)->trem_dur0(module_none);
	      } else if (inv) CASTNOTEEV(*i)->trem_halfdur0(spl); else CASTNOTEEV(*i)->trem_incoff0(spl);
	    } else if (inv) {
	      CASTNOTEEV(*i)->trem_halfdur(spl);
	      if (!era) CASTNOTEEV(*i)->switchtrems(mark_trem2, mark_trem);
	    } else CASTNOTEEV(*i)->trem_incoff(spl);
	    t2 = true;
	    goto STLOOP;
	  }
	  }
	}
      STLOOP: ;
      }
      if (t1 != t2 || (t0 && (t1 || t2))) {
	CERR << "invalid tremolo"; integerr("tremolos");	
      }
    }
    DBG("END AUX 1" << std::endl);
    for (std::vector<noteevbase*>::iterator i(nos.begin()); i != nos.end(); ++i) {
      if (era) {
	INTWRITELOCK(*i);
	CASTNOTEEVBASE(*i)->remtremmarks();
      }
      (*i)->post_apisetvalue();
    }
    DBG("END AUX 2" << std::endl);
  }
  
  void splittremsdoit(FOMUS fom, void* moddata) { // by PART, VOICE
    DBG("START SPLITTREMS" << std::endl);
    offgroff loff((fint)-1);
    numb spl;
    std::vector<noteevbase*> nos;
    splittremsenum xx = spltrems_none;
    measure* lmea = 0;
    rat wrm;
    int era = 0;
    bool inv = false;
    bool minv = false;
    bool notyet = true;
    while (true) { // this can update measure info independent of other stages
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break; // this is a _destr stage, so nothing is waiting on this thread
      assert(CASTNOTEEVBASE(n)->isvalid());
      offgroff ti(CASTNOTEEVBASE(n)->getfulltime());
      if (ti > loff) {
	splittremsaux(nos, spl, xx, era, inv);
	if (xx == spltrems_none) inv = false; else if (minv) inv = !inv;
	nos.clear();
	xx = spltrems_none;
	era = 0;
	minv = false;
	spl.null();
	loff = ti;
	notyet = true;
      }
      measure* mea = CASTNOTEEVBASE(n)->getmeasptr();
      if (mea != lmea) {
	if (lmea) {
	  lmea->reinsert();
	}
	wrm = mea->writtenmultrat();
	lmea = mea;
      }
      assert(lmea);      
      if (notyet) {
	INTREADLOCK(n);
	fomus_rat mlt(rattofrat(wrm * CASTNOTEEVBASE(n)->gettupmult(-1)));
	module_markslist ml(CASTNOTEEVBASE(n)->getsinglemarks_nomut());
	numb du(CASTNOTEEVBASE(n)->getdur_nomut());
	for (const module_markobj *i(ml.marks), *ie(ml.marks + ml.n); i < ie; ++i) {
	  assert(CASTMARKOBJ(*i)->isvalid());
	  switch (CASTMARKOBJ(*i)->getmarkid()) {
	  case mark_trem:
	  case mark_trem2: {
	    numb x(CASTMARKOBJ(*i)->getmarkval());
	    if (!x.isint() || abs_int(x.val.i) < 8 || abs_int(x.val.i) > 32 || !isexpof2(abs_int(x.val.i))) {
	      CERR << "invalid tremolo number"; integerr("tremolos");
	    }
	    fomus_rat dt1((fint)1 / ((x.geti() < 0 ? -x.geti() : x.geti()) * mlt)); // duration of 1st + 2nd part of tremolo
	    if (x < (fint)0) {
	      fomus_rat dt2(dt1 * (fint)2); // duration of 1st + 2nd part of tremolo
	      xx = spltrems_doub;
	      if (dt2 <= du) {
		numb x0(mod(du, dt2)); // used to be dt1, but lilypond doesn't notate it unless both part of alt. trem divide evenly
		if (x0 > (fint)0) spl = du - x0; //else spl.null();
		if (dt2 >= (spl.isnull() ? du : spl)) era = 1;
		if (/*spl.isnull() &&*/ x.geti() > -32 && mod(du, dt2) == dt1) minv = true;
	      } else {
		era = 2; // one half of alternating trem + possible split
		if (dt1 < du) spl = dt1;
		else if (dt1 <= du) minv = true; // exactly 1/2 of alt.
	      }
	    } else {
	      if (dt1 <= du) {
		numb x0(mod(du, dt1));
		if (x0 > (fint)0) spl = du - x0; //else spl.null();
		xx = spltrems_sing;
		if (dt1 >= (spl.isnull() ? du : spl)) era = 1;
	      } else era = 1;
	    }
	    notyet = false;
	  }
	  }
	}
      }
      CASTNOTEEVBASE(n)->resetassdel();
      nos.push_back(CASTNOTEEVBASE(n));
    }
    DBG("END SPLITTREMS" << std::endl);
    splittremsaux(nos, spl, xx, era, inv);
    assert(lmea);
    lmea->reinsert();
    DISABLEMUTCHECK;
    CASTPARTORMPART(stageobj->api_nextpart())->inserttmps();
  }

  bool tremtiesaux(const std::vector<module_noteobj>& x, const std::vector<module_noteobj>& y, bool ch1is /*, const offgroff& ti*/) {
    // if (y.empty()) return false;
    assert(y.empty() || tremties.find(CASTNOTEEV(y.front())->get_sval(TREMTIE_ID)) != tremties.end());
    bool ch2is;
    if (!y.empty() && tremties.find(CASTNOTEEV(y.front())->get_sval(TREMTIE_ID))->second == tiestyle_none) {
      module_markslist ml(CASTNOTEEVBASE(y.front())->getsinglemarks());
      for (const module_markobj *m(ml.marks), *me(ml.marks + ml.n); m < me; ++m) {
	switch (CASTMARKOBJ(*m)->getmarkid()) {
	case mark_trem:
	case mark_trem2:
	  ch2is = true;
	  goto SKIP;
	}
      }
    }
    ch2is = false;
  SKIP:
    //assert(ch1is ? !x.empty() : true);
    if (ch1is || ch2is /*&& x.front()->getfullendtime() >= ti*/) {
      assert(ch1is ? !x.empty() : true);
      for (std::vector<module_noteobj>::const_iterator i(x.begin()); i != x.end(); ++i) CASTNOTEEV(*i)->destroytiedr();
      //for (std::vector<module_noteobj>::const_iterator i(y.begin()); i != y.end(); ++i) CASTNOTEEV(*i)->destroytiedl();
    }
    return ch2is;
  }
  
  inline void tremtiesdone(std::vector<module_noteobj>& x) {
    for (std::vector<module_noteobj>::iterator i(x.begin()); i != x.end(); ++i) CASTNOTEEV(*i)->post_apisetvalue();
  }
  void tremtiesdoit(FOMUS fom, void* moddata) { // by PART, VOICE, NOTES ONLY
    std::auto_ptr<std::vector<module_noteobj> > ch1(new std::vector<module_noteobj>), ch2(new std::vector<module_noteobj>);
    bool ch1is = false;
    offgroff cti((fint)-1);
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      offgroff ti(CASTNOTEEV(n)->getfulltime());
      if (ti > cti) {
	ch1is = tremtiesaux(*ch1, *ch2, ch1is /*, ti*/);
	tremtiesdone(*ch1);
	std::vector<module_noteobj>* x = ch1.release();
	ch1 = ch2;
	ch2.reset(x);
	ch2->clear();
	cti = ti;
      }
      ch2->push_back(n);
    }
    tremtiesaux(*ch1, *ch2, ch1is /*, ti*/);
    tremtiesdone(*ch1);
    tremtiesdone(*ch2);
  }
  
  void fixlyrsdoit(FOMUS fom, void* moddata) { // by PART, VOICE
    std::auto_ptr<std::vector<markobj*> > mks(new std::vector<markobj*>());
    offgroff of((fomus_int)-1);
    bool ddr = false;
    std::vector<noteevbase*> q;
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) {
	for (std::vector<noteevbase*>::iterator i(q.begin()); i != q.end(); ++i) (*i)->post_apisetvalue();
	return;
      }
      module_markslist ml(CASTNOTEEVBASE(n)->getsinglemarks());
      if (CASTNOTEEVBASE(n)->getfulltime() > of) {
	std::auto_ptr<std::vector<markobj*> > mks0(new std::vector<markobj*>());
	bool ddr0 = false;
	for (const module_markobj *i(ml.marks), *ie(ml.marks + ml.n); i < ie; ++i) {
	  if (CASTMARKOBJ(*i)->getmarkid() == mark_vocal_text) {
	    if (boost::algorithm::starts_with(CASTMARKOBJ(*i)->str, "--")) ddr = true;
	    if (boost::algorithm::ends_with(CASTMARKOBJ(*i)->str, "--")) ddr0 = true;
	    mks0->push_back(CASTMARKOBJ(*i));
	  }
	}
	if (ddr) {
	  for (std::vector<markobj*>::iterator i(mks->begin()); i != mks->end(); ++i) (*i)->insureddr();
	  for (std::vector<markobj*>::iterator i(mks0->begin()); i != mks0->end(); ++i) (*i)->insureddl();	  
	}
	mks = mks0; ddr = ddr0;
	for (std::vector<noteevbase*>::iterator i(q.begin()); i != q.end(); ++i) (*i)->post_apisetvalue();
	q.clear();
      } else { // same time
	for (const module_markobj *i(ml.marks), *ie(ml.marks + ml.n); i < ie; ++i) {
	  if (CASTMARKOBJ(*i)->getmarkid() == mark_vocal_text) {
	    mks->push_back(CASTMARKOBJ(*i));
	    if (boost::algorithm::ends_with(CASTMARKOBJ(*i)->str, "--")) ddr = true;
	  }
	}
      }
      q.push_back(CASTNOTEEVBASE(n));
    }        
  }
  
#ifndef NDEBUG
  inline bool debugmarksinorder(const module_markslist& x) {
    if (x.n >= 2) {
      for (const module_markobj* m(x.marks), *me(x.marks + x.n - 1); m < me; ++m) {
	if (CASTMARKOBJ(*m)->getmarkid() > (CASTMARKOBJ(*(m + 1)))->getmarkid()) return false;
      }
    }
    return true;
  }
#endif
  
  // template<typename T>
  // inline void erase1(T& cont, const typename T::key_type& key) {
  //   typename T::const_iterator i(cont.find(key));
  //   if (i != cont.end()) cont.erase(i);
  // }
  void postspandoit(FOMUS fom, void* moddata) { // BY PART, (VOICE or STAFF)
    std::multiset<int> st;
    std::multiset<int> st0; // start ids w/ ends so far (st0 is a tmp set)
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      {
	module_markslist b(CASTNOTEEVBASE(n)->getspannerbegins());
	module_markslist e(CASTNOTEEVBASE(n)->getspannerends());
	assert(debugmarksinorder(b));
	assert(debugmarksinorder(e));
#ifndef NDEBUGOUT
	CASTNOTEEVBASE(n)->dumpall();
#endif      
	std::set<int> rec; // matching ends found
	const module_markobj* me(e.marks + e.n); // ends
	INTUPREADLOCK(n);
	for (const module_markobj* m(e.marks); m < me; ++m) {
	  int id = CASTMARKOBJ(*m)->getmarkid() - 1; // get begin mark
	  if (moddata ? markdefs[id].getisstaff() : markdefs[id].getisvoice()) {
	    bool cso = markdefs[id].getcanspanone_nomut(*CASTNOTEEVBASE(n));
	    if (markdefs[id].getcantouch_nomut(*CASTNOTEEVBASE(n)) && cso) { // ambiguous end/begin order
	      std::multiset<int>::const_iterator i(st.find(id));
	      if (i != st.end()) {
		st.erase(i);
		rec.insert(id); // ends founds w/ matches
	      }
	    } else if (!cso) { // can-span-one end marks are ignored here...
	      std::multiset<int>::const_iterator i(st.find(id));
	      if (i == st.end()) {
		CERR << "invalid end mark"; integerr("mark spanners");
	      }
	      st.erase(i);
	      rec.insert(id); // ends founds w/ matches
	    }
	  }
	}
	assert(st0.empty());
	const module_markobj* mbe(b.marks + b.n); // loop through begin marks
	for (const module_markobj* m(b.marks); m < mbe; ++m) {
	  int id = CASTMARKOBJ(*m)->getmarkid();
	  if (moddata ? markdefs[id].getisstaff() : markdefs[id].getisvoice()) {
	    if (st.find(id) != st.end() // already another of same one missing an end
		|| (!markdefs[id].getcantouch_nomut(*CASTNOTEEVBASE(n)) // can't touch (so can't be a new beginning on same note)
		    && rec.find(id) != rec.end())) { // corresponding end w/ beginning already found
	      CERR << "invalid begin mark"; integerr("mark spanners");
	    }
	    st0.insert(id); // defer adding them to st, in case there is more than one begin
	  }
	}
	st.insert(st0.begin(), st0.end());
	st0.clear();
	for (const module_markobj* m(e.marks); m < me; ++m) {
	  int id = CASTMARKOBJ(*m)->getmarkid() - 1;
	  if (rec.find(id) != rec.end()) continue; // end mark has already been matched
	  if (moddata ? markdefs[id].getisstaff() : markdefs[id].getisvoice()) {
	    bool cso = markdefs[id].getcanspanone_nomut(*CASTNOTEEVBASE(n));
	    if (markdefs[id].getcantouch_nomut(*CASTNOTEEVBASE(n)) && cso) { // ambiguous end/begin order
	      std::multiset<int>::const_iterator i(st.find(id));
	      if (i != st.end()) st.erase(i);
	    } else if (cso) { // finish up what was ignored above, now that starts were iterated through...
	      std::multiset<int>::const_iterator i(st.find(id));
	      if (i == st.end()) {
		CERR << "invalid end mark"; integerr("mark spanners");
	      }
	      st.erase(i);
	    }
	  }
	}
      }
      CASTNOTEEVBASE(n)->post_apisetvalue();
    }
    if (!st.empty()) {
      CERR << "invalid begin mark"; integerr("mark spanners");      
    }
  }

  inline bool isconflict(const markobj& x, const module_markobj y) { // x is always end mark, y is always begin mark
    assert(x.getwhich() == marks_end);
    assert(CASTMARKOBJ(y)->getwhich() == marks_begin);
    //return (x.getisconflict() ? x.getmarkid() - 3 : x.getmarkid() - 1) == (CASTMARKOBJ(y)->getisconflict() ? CASTMARKOBJ(y)->getmarkid() - 2 : CASTMARKOBJ(y)->getmarkid());
    return (x.getgroup() != 0 && x.getgroup() == CASTMARKOBJ(y)->getgroup());
  }
  void finalmarksdoit(FOMUS fom, void* moddata) { // BY PART, BY VOICE
    std::set<int> mks;
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      module_markslist b(CASTNOTEEVBASE(n)->getspannerbegins());
      module_markslist e(CASTNOTEEVBASE(n)->getspannerends());
      assert(debugmarksinorder(b));
      assert(debugmarksinorder(e));
      const module_markobj* be(b.marks + b.n);
      const module_markobj* me(e.marks + e.n);
      std::set<int> skp;
      for (const module_markobj* m(e.marks); m < me; ++m) {
	int id = CASTMARKOBJ(*m)->getmarkid() - 1; // the begin mark
	if (moddata ? markdefs[id].getisstaff() : markdefs[id].getisvoice()) { // only if voice/staff
	  std::set<int>::iterator i(mks.find(id));
	  if (i != mks.end()) {
	    mks.erase(i);
	    CASTMARKOBJ(*m)->sort = (hassome(b.marks, be, boost::lambda::bind(isconflict, boost::lambda::constant_ref(*CASTMARKOBJ(*m)), boost::lambda::_1)) ? 0 : 2);
	    DBG("(a) MARK @ " << numb(CASTNOTEEVBASE(n)->gettime()) << " ENDMARK order = " << CASTMARKOBJ(*m)->sort << std::endl);
	    skp.insert(id); // skip this next time
	  }
	}
      }
      for (const module_markobj* m(b.marks); m < be; ++m) {
	int id = CASTMARKOBJ(*m)->getmarkid(); // the begin mark
	if (moddata ? markdefs[id].getisstaff() : markdefs[id].getisvoice()) { // only if voice/staff
	  std::set<int>::iterator i(mks.find(id));
	  if (i == mks.end()) { // not there
	    mks.insert(id);
	    assert(CASTMARKOBJ(*m)->sort == 1);
	  }
	}
      }
      for (const module_markobj* m(e.marks); m < me; ++m) {
	int id = CASTMARKOBJ(*m)->getmarkid() - 1; // the begin mark
	if ((moddata ? markdefs[id].getisstaff() : markdefs[id].getisvoice()) && skp.find(id) == skp.end()) { // only if voice/staff and not skipping
	  std::set<int>::iterator i(mks.find(id));
	  if (i != mks.end()) {
	    mks.erase(id);
	    CASTMARKOBJ(*m)->sort = 2;
	    DBG("(b) MARK @ " << numb(CASTNOTEEVBASE(n)->gettime()) << " ENDMARK order = " << CASTMARKOBJ(*m)->sort << std::endl);
	  }
	}
      }
      int_skipassign(n);
    }    
  }
  
  void spreadmarksdoit(FOMUS fom, void* moddata) { // BY MEAS, BY VOICE
    boost::ptr_set<markobj, marksetlt> mrks; // unique ordered mark set
    module_noteobj n0 = 0; //, stn = 0;
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      DBG("spreading " << numb(CASTNOTEEVBASE(n)->gettime()) << " voice " << CASTNOTEEVBASE(n)->getvoice() << std::endl);
      CASTNOTEEVBASE(n)->spreadgetmarks(mrks); // note is emptied
      module_noteobj nx = stageobj->api_peeknextnote((modobjbase*)n);
      if (!n0) n0 = n;
      if (!nx || CASTNOTEEVBASE(n)->getfulltime() != CASTNOTEEVBASE(nx)->getfulltime()) { // next note isn't in chord
	while (true) {
	  DBG("!!!" << std::endl);
	  CASTNOTEEVBASE(n0)->spreadreplmarks(mrks);
	  CASTNOTEEVBASE(n0)->post_apisetvalue();
	  if (n0 == n) break;
	  n0 = stageobj->api_peeknextnote((modobjbase*)n0);
	}
	mrks.clear();
	n0 = 0;
      } 
    }    
  }

  void postoctdoit(FOMUS fom, void* moddata) { // MEAS
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      CASTNOTEEVBASE(n)->checkoct();
    }
  }  

  void postacc1doit(FOMUS fom, void* moddata) { // MEAS, NOTEONLY
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      assert(CASTNOTEEVBASE(n)->getisnote());
      CASTNOTEEV(n)->checkacc1(CASTFOMUSDATA(fom));
    }    
  }

  void posttiedoit(FOMUS fom, void* moddata) {
    while (true) { // eat the notes--this is last in substage section
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      int_skipassign(n);
    }
    DISABLEMUTCHECK;
    CASTMEASURE(stageobj->api_nextmeas())->fixties();
    assert(!stageobj->api_nextmeas());
  }  

  void postbeamsdoit(FOMUS fom, void* moddata) { // BYMEAS, VOICE
    module_noteobj b = 0;
    int bl = -1, br;
    offgroff t;
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      {
	INTUPREADLOCK(n);
	CASTNOTEEVBASE(n)->checkbeam();
	if (bl >= 0 && CASTNOTEEVBASE(n)->getfulltime_nomut() == t) CASTNOTEEVBASE(n)->assignbeams2(bl, br, LOCKARG); else {
	  if (CASTNOTEEVBASE(n)->getisnote() && !CASTNOTEEVBASE(n)->getisperc_nomut()) {
	    CASTNOTEEV(n)->getblbr(bl, br);
	    t = CASTNOTEEVBASE(n)->getfulltime_nomut();
	  } else bl = -1;
	}
	if (CASTNOTEEVBASE(n)->isbeambeg() && CASTNOTEEVBASE(n)->isinnerbeg()) b = n;
	else if (b && CASTNOTEEVBASE(n)->isbeamend() && CASTNOTEEVBASE(n)->isinnerend()) {
	  UNLOCKUPREAD;
	  module_noteobj j = b;
	  do {
	    CASTNOTEEVBASE(j)->setinnerinvis(); // write locks
	    j = stageobj->api_peeknextnote((modobjbase*)j);
	  } while (j != n);
	  b = 0;
	} else if (!CASTNOTEEVBASE(n)->isbeammid()) b = 0;
      }
      CASTNOTEEVBASE(n)->post_apisetvalue();
    }
  }  

  void postmergedoit(FOMUS fom, void* moddata) { // BY MEAS
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      CASTNOTEEVBASE(n)->domerge(); // write locks
    }
  }  

  void fillholesdoit(FOMUS fom, void* moddata) { // BY MEAS, BY VOICE
    DBG("FILLHOLESDOIT!!!" << std::endl);
    measure& m = *CASTMEASURE(stageobj->api_nextmeas());
    DBG("the measure is: " << &m << " @ t=" << (const numb&)m.gettime() << " - t=" << (const numb&)m.getendtime() << std::endl);
    ranges holes(range(m.gettime(), m.getendtime()));
    bool fi = true;
    int v;
#ifndef NDEBUG
    v = 0;
#endif    
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      if (fi) {
	v = CASTNOTEEVBASE(n)->getvoice();
	fi = false;
      }
    AGAIN:
      numb ti(CASTNOTEEVBASE(n)->gettime());
      holes.remove_range(range(ti, CASTNOTEEVBASE(n)->getendtime()));
      if (CASTNOTEEVBASE(n)->getisgrace()) {
	numb e(std::numeric_limits<fomus_int>::min() + 1);
	std::vector<module_noteobj> nov;
	do {
	  nov.push_back(n);
	  numb et(CASTNOTEEVBASE(n)->getgraceendtime());
	  if (et > e) e = et;
	  int_skipassign(n);
	  n = stageobj->api_nextnote();
	} while (n && CASTNOTEEVBASE(n)->getisgrace() && CASTNOTEEVBASE(n)->gettime() <= ti);
	assert(!nov.empty());
	assert(e > std::numeric_limits<fomus_int>::min() + 1);
	ranges grholes(range(CASTNOTEEVBASE(nov.front())->getgracetime(), e));
	for (std::vector<module_noteobj>::const_iterator i(nov.begin()); i != nov.end(); ++i) {
	  grholes.remove_range(range(CASTNOTEEVBASE(*i)->getgracetime(), CASTNOTEEVBASE(*i)->getgraceendtime()));
	}
	for (ranges::const_iterator i(grholes.begin()); i != grholes.end(); ++i) m.fillgrhole(ti, i->x1, i->x2, v);
	if (!n) break; else goto AGAIN;
      }
      int_skipassign(n);
    } // notes are eaten
    DISABLEMUTCHECK;
    assert(v);
    for (ranges::const_iterator i(holes.begin()); i != holes.end(); ++i) m.fillhole(i->x1, i->x2, v);
  }

  struct groupstr {
    int ind1;
    parts_grouptype type;
    scorepartlist::iterator p1, p2;
    groupstr(const int ind,  const parts_grouptype type, const scorepartlist::iterator p1, const scorepartlist::iterator p2):ind1(ind), type(type), p1(p1), p2(p2) {}
    void set(const scorepartlist::iterator p, const int ind) {
      assert(ind > ind1);
      p2 = p;
      ind1 = ind - ind1;
    }
  };
  inline bool operator<(const groupstr& x, const groupstr& y) {
    return x.ind1 != y.ind1 ? x.ind1 > y.ind1 : x.type < y.type; 
  }
  struct splless:public std::binary_function<const scorepartlist_it&, const scorepartlist_it&, bool> {
    bool operator()(const scorepartlist_it& x, const scorepartlist_it& y) const {
      return x->get() < y->get();
    }
  };
  
  void fomusdata::postparts() {
    scoreparts.sort();
    scoreparts.front()->settmark();
    std::list<groupstr> grps;
    {
      std::map<int, groupstr*> grps0;
      for (scorepartlist_it i(scoreparts.begin()); i != scoreparts.end(); ++i) {
	part& p = (*i)->getpart();
	for (std::map<int, parts_grouptype>::const_iterator j(p.getbgroups().begin()), je(p.getbgroups().end()); j != je; ++j) {
	  std::map<int, groupstr*>::iterator t(grps0.find(j->first));
	  if (t == grps0.end()) {
	    grps.push_back(groupstr((*i)->index(), j->second, i, scoreparts.end()));
	    grps0.insert(std::map<int, groupstr*>::value_type(j->first, &grps.back()));
	  } else {
	    if (t->second->p2 != scoreparts.end()) {CERR << "invalid part group"; integerr("parts");}
	    t->second->set(i, (*i)->index());
	  }
	}
	p.getbgroups().clear();
	assert(p.getegroups().empty());
      }
    }
    grps.sort(); // biggest groups first
    std::map<scorepartlist_it, int, splless> grplvls;
    for (std::list<groupstr>::const_iterator i(grps.begin()); i != grps.end(); ++i) {
      if (i->p2 == scoreparts.end()) {CERR << "invalid part group"; integerr("parts");}
      std::map<scorepartlist_it, int, splless>::iterator l1 = grplvls.find(i->p1);
      int lvl;
      if (l1 == grplvls.end()) {
	grplvls.insert(std::map<scorepartlist_it, int, splless>::value_type(i->p1, 1)).first;
	lvl = 1;
      } else {
	lvl = ++l1->second;
      }
      scorepartlist_it i0(i->p1);
      do {
	++i0;
	assert(i0 != scoreparts.end());
	std::map<scorepartlist_it, int, splless>::iterator l2 = grplvls.find(i0);
	int lvl0;
	if (l2 == grplvls.end()) {
	  grplvls.insert(std::map<scorepartlist_it, int, splless>::value_type(i0, 1)).first;
	  lvl0 = 1;
	} else {
	  lvl0 = ++l2->second;
	}
	if (lvl != lvl0) {CERR << "overlapping parts group"; integerr("parts");}
      } while (i0 != i->p2);
      (*i->p1)->insbgroups(lvl, i->type);
      (*i0)->insegroups(lvl);
      DBG("LAYER " << lvl << " BRACKET AROUND: " << (*i->p1)->getid() << " and " << (*i0)->getid() << std::endl);
    }
  }
  
  void pnotesdoit(FOMUS fom, void* moddata) { // by meas and perconly
    DBG("doing pnotes" << std::endl);
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      CASTNOTEEVBASE(n)->estabpercinst();
    }    
  }

  void redotiesdoit(FOMUS fom, void* moddata) { // by PART, by VOICE
    std::list<noteev*> evs;
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      offgroff ti;
      numb no;
      bool will;
      {
	INTREADLOCK(n);
	assert(CASTNOTEEVBASE(n)->getisnote());
	ti = CASTNOTEEV(n)->getfulltime_nomut();
	no = CASTNOTEEV(n)->getnote_nomut();
	will = CASTNOTEEV(n)->getwillbetiedl();
      }
      for (std::list<noteev*>::iterator i(evs.begin()); i != evs.end(); ) {
	INTUPREADLOCK(*i);
	if ((*i)->getfullendtime() < ti) {
	  i = evs.erase(i);
	  continue;
	} else if ((*i)->getfullendtime() == ti) {
	  (*i)->tiewillbetieds(no, will, CASTNOTEEV(n), LOCKARG);
	}
	++i;
      }
      evs.push_back(CASTNOTEEV(n));
      int_skipassign(n);
    }    
  }
  
  void postspecialdoit(FOMUS fom, void* moddata) { // bymeas--this must be last in stage!!!!
    DBG("POSTSPECIALDOIT!" << std::endl);
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      int_skipassign(n); 
    }
    DISABLEMUTCHECK;
    DBG("POSTSPECIALDOIT!  ready to do it" << std::endl);
    while (true) {
      module_measobj m = stageobj->api_nextmeas();
      if (!m) break;
      CASTMEASURE(m)->postspecial(); // deletes notes that are assdel
    }
    CASTPARTORMPART(stageobj->api_nextpart())->inserttmps();    
    DBG("POSTSPECIALDOIT!  done it" << std::endl);
  }
  
  void markhelpersdoit(FOMUS fom, void* moddata) { // by meas
    const partormpart_str& pmp(*CASTPARTORMPART(stageobj->api_nextpart()));
    assert(pmp.isvalid());
    bool npgr = !pmp.gettmark();
    const listelmap&
      map1(pmp.get_varbase(MARKTEXTS_ID).getmapval()),
      map2(pmp.get_varbase(DEFAULTMARKTEXTS_ID).getmapval()),
      smap(pmp.get_varbase(MARKALIASES_ID).getmapval());
    const std::string qu("?");
    listelmap_constit ii(map1.find("sul"));
    if (ii == map1.end()) ii = map2.find("sul");
    const std::string sul(ii != map2.end() ? listel_getstring(ii->second) : qu);
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) break;
      module_markslist ma(CASTNOTEEVBASE(n)->getsinglemarks());
      bool rca = false;
      for (const module_markobj *m(ma.marks), *me(ma.marks + ma.n); m != me; ++m) {
	int id = CASTMARKOBJ(*m)->getmarkid();
	switch (markdefs[id].getbaseid()) {
	case mark_sul: {
	  assert(sulstyles.find(CASTNOTEEVBASE(n)->get_sval(SULSTYLE_ID)) != sulstyles.end());
	  assert(CASTMARKOBJ(*m)->val.type() == module_int);
	  static const char lett[7] = {'C', 'D', 'E', 'F', 'G', 'A', 'B'};
	  switch (sulstyles.find(CASTNOTEEVBASE(n)->get_sval(SULSTYLE_ID))->second) {
	  case ss_letter: CASTMARKOBJ(*m)->str = lett[diatonicnotes[CASTMARKOBJ(*m)->val.geti() % 12]]; break;
          case ss_sulletter: CASTMARKOBJ(*m)->str = sul + lett[diatonicnotes[CASTMARKOBJ(*m)->val.geti() % 12]]; break;
	  case ss_roman: CASTMARKOBJ(*m)->str = toromanstr(CASTMARKOBJ(*m)->val.geti()); break;
	  case ss_sulroman: CASTMARKOBJ(*m)->str = sul + toromanstr(CASTMARKOBJ(*m)->val.geti()); break;
	  }
	}
	  break;
	case mark_pizz: 
	case mark_arco:
	case mark_mute:
	case mark_unmute:
	case mark_vib:
	case mark_moltovib:
	case mark_nonvib:
	case mark_legato:
	case mark_moltolegato:
	case mark_nonlegato:
	  //case mark_sul:
	case mark_salt:
	case mark_ric:
	case mark_lv:
	case mark_flt:
	case mark_slap:
	case mark_breath:    
	case mark_spic:
	case mark_tall:
	case mark_punta:
	case mark_pont:
	case mark_tasto:
	case mark_legno:
	case mark_flaut:
	case mark_etouf:
	case mark_table:
	case mark_cuivre:
	case mark_bellsup:
	case mark_ord:
	  {
	    const std::string na(markdefs[id].getname());
	    listelmap_constit i(map1.find(na));
	    if (i == map1.end()) i = map2.find(na);
	    CASTMARKOBJ(*m)->str = (i != map2.end() ? listel_getstring(i->second) : qu);
	  }
	  break;
	case mark_text:
	case mark_italictextabove:
	case mark_italictextabove_begin:
	case mark_italictextbelow:
	case mark_italictextbelow_begin:
	case mark_stafftext:
	case mark_stafftext_begin:
	  {
	    listelmap_constit i(smap.find(CASTMARKOBJ(*m)->str));
	    if (i != smap.end()) CASTMARKOBJ(*m)->str = listel_getstring(i->second);
	  }
	  break;
	}
	if (npgr && markdefs[id].ispgroupmark()) {
	  CASTNOTEEVBASE(n)->assignmarkrem(id, CASTMARKOBJ(*m)->str.c_str(), CASTMARKOBJ(*m)->val);	  
	  rca = true;
	}
      }
      if (rca) CASTNOTEEVBASE(n)->recacheassign(); else CASTNOTEEVBASE(n)->post_apisetvalue();
    }    
  }

  struct mhelpstr {
    module_noteobj n;
    bool m;
    mhelpstr(const module_noteobj n):n(n), m(false) {}
    void ass() const {if (m) CASTNOTEEVBASE(n)->recacheassign(); else CASTNOTEEVBASE(n)->post_apisetvalue();}
    void ins(const int i, const std::string& str) {/*INTWRITELOCK(n);*/ CASTNOTEEVBASE(n)->assignmarkins(i, str, numb(module_none)); m = true;}
    void rem(const int i) {CASTNOTEEVBASE(n)->assignmarkrem(i, 0, numb(module_none)); m = true;}
  };
  void contmarksaux1(const std::map<int, std::string>& s1, const std::map<int, std::string>& s2, std::vector<mhelpstr>& objs2) {
    for (std::map<int, std::string>::const_iterator i(s2.begin()); i != s2.end(); ++i) {
      if (i->first < 0) continue;
      int id = i->first - 2; // id of begin mark
      if (s1.find(i->first) == s1.end() && s1.find(-id) == s1.end()) { // insert a begin mark
	std::for_each(objs2.begin(), objs2.end(), boost::lambda::bind(&mhelpstr::ins, boost::lambda::_1, id, boost::lambda::constant_ref(i->second)));	
      }
    }
  }
  void contmarksaux2(const std::map<int, std::string>& s1, const std::map<int, std::string>& s2, std::vector<mhelpstr>& objs1) {
    for (std::map<int, std::string>::const_iterator i(s1.begin()); i != s1.end(); ++i) {
      if (i->first < 0) continue;
      int id = i->first - 1; // id of end mark
      if (s2.find(i->first) == s2.end() && s2.find(-id) == s2.end()) { // insert an end mark
	std::for_each(objs1.begin(), objs1.end(), boost::lambda::bind(&mhelpstr::ins, boost::lambda::_1, id, boost::lambda::constant_ref(i->second)));	
      }
    }
    std::for_each(objs1.begin(), objs1.end(), boost::lambda::bind(&mhelpstr::ass, boost::lambda::_1));
  }
  
  void contmarksdoit(FOMUS fom, void* moddata) {
    DBG("contmarksdoit" << std::endl);
    std::auto_ptr<std::map<int, std::string> > s1(new std::map<int, std::string>), s2(new std::map<int, std::string>);
    offgroff o((fomus_int)0);
    std::auto_ptr<std::vector<mhelpstr> > objs1(new std::vector<mhelpstr>), objs2(new std::vector<mhelpstr>);
    while (true) {
      module_noteobj n = stageobj->api_nextnote();
      if (!n) {
	contmarksaux1(*s1, *s2, *objs2);
	contmarksaux2(*s1, *s2, *objs1);
	contmarksaux2(*s2, std::map<int, std::string>(), *objs2);
	break;
      }
      if (CASTNOTEEVBASE(n)->getfulltime() > o) {
	contmarksaux1(*s1, *s2, *objs2);
	contmarksaux2(*s1, *s2, *objs1);
	std::vector<mhelpstr>* x1 = objs1.release();
	x1->clear();
	objs1 = objs2;
	objs2.reset(x1);
	std::map<int, std::string>* x2 = s1.release();
	x2->clear();
	s1 = s2;
	s2.reset(x2);
      }
      objs2->push_back(n);
      module_markslist ma(CASTNOTEEVBASE(n)->getsinglemarks());
      for (const module_markobj *m(ma.marks), *me(ma.marks + ma.n); m != me; ++m) {
	int id = CASTMARKOBJ(*m)->getmarkid();
	if (markdefs[id].iscont() && (moddata ? markdefs[id].getisstaff() : markdefs[id].getisvoice())) {
	  s2->insert(std::map<int, std::string>::value_type(id, CASTMARKOBJ(*m)->str));
	  objs2->back().rem(id);
	}
      } {
	module_markslist ma(CASTNOTEEVBASE(n)->getspannerends());
	for (const module_markobj *m(ma.marks), *me(ma.marks + ma.n); m != me; ++m) {
	  int id = CASTMARKOBJ(*m)->getmarkid();
	  if (moddata ? markdefs[id].getisstaff() : markdefs[id].getisvoice()) s2->insert(std::map<int, std::string>::value_type(-id, std::string()));
	}
      } {
	module_markslist ma(CASTNOTEEVBASE(n)->getspannerbegins());
	for (const module_markobj *m(ma.marks), *me(ma.marks + ma.n); m != me; ++m) {
	  int id = CASTMARKOBJ(*m)->getmarkid();
	  if (moddata ? markdefs[id].getisstaff() : markdefs[id].getisvoice()) s2->insert(std::map<int, std::string>::value_type(-id, std::string()));
	}
      }
    }   
  }
  
  intmod_postmeas imod_postmeas_destr;
  intmod_posttquant imod_posttquant_destr;
  intmod_posttquantinv imod_posttquantinv_destr;
  intmod_postpquant imod_postpquant;
  intmod_postvoices imod_postvoices_destr;
  intmod_poststaves imod_poststaves_destr;
  intmod_poststaves2 imod_poststaves2_destr;
  intmod_postprune imod_postprune_destr;
  intmod_postsspan imod_postsspan;
  intmod_prevspan imod_prevspan;
  intmod_postvspan imod_postvspan;
  intmod_postoct imod_postoct;
  intmod_postacc1 imod_postacc1;
  intmod_posttie imod_posttie_destr;
  intmod_postbeams imod_postbeams;
  intmod_fillholes imod_fillholes_destr;
  //intmod_fillholesa imod_fillholesa_destr;
  intmod_postmerge imod_postmerge_destr;
  //intmod_postparts imod_postparts_destr;
  intmod_spreadmarks imod_spreadmarks;
  //intmod_mparts imod_mparts_destr;
  intmod_pnotes imod_pnotes_destr;
  intmod_fillnotes1 imod_fillnotes1_destr;
  //intmod_fillnotes1a imod_fillnotes1a_destr;
  intmod_fillnotes2 imod_fillnotes2_destr;
  intmod_postmarkevs imod_postmarkevs_destr;
  intmod_redoties imod_redoties_destr;
  intmod_finalmarksv imod_finalmarksv;
  intmod_finalmarkss imod_finalmarkss;
  intmod_postspecial imod_postspecial_destr;
  intmod_fixlyrs imod_fixlyrs;
  intmod_barlines imod_barlines;
  intmod_splittrems imod_splittrems_destr;
  intmod_markhelpers imod_markhelpers;
  intmod_inbetweenmarks imod_inbetweenmarks;
  intmod_sysbreak imod_sysbreak;
  intmod_contmarksv imod_contmarksv;
  intmod_contmarkss imod_contmarkss;
  //intmod_eatnotes_bymeas imod_eatnotes_bymeas;
  intmod_tremties imod_tremties;
  
}
  
