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

#include "classes.h"
#include "algext.h"
#include "schedr.h"
//#include "modnotes.h"

namespace fomus {

#ifndef NDEBUG
  boost::thread_specific_ptr<bool> lockcheck;
#endif
  
  void marksbase::cachessort() { 
    boost::ptr_vector<markobj>::iterator x1(std::lower_bound(RMUT(marks).begin(), RMUT(marks).end(), m_voicebegin,
							     boost::lambda::bind(&markobj::getspantype, boost::lambda::_1) < boost::lambda::_2));
    assert(x1 >= RMUT(marks).begin() && x1 <= RMUT(marks).end());
    boost::ptr_vector<markobj>::const_iterator x2(std::lower_bound(x1, RMUT(marks).end(), m_voiceend,
								   boost::lambda::bind(&markobj::getspantype, boost::lambda::_1) < boost::lambda::_2));
    assert(x2 >= x1 && x2 <= RMUT(marks).end());
    WMUT(s).n = x1 - RMUT(marks).begin();
    WMUT(s).marks = (module_markobj*)RMUT(marks).c_array();
    WMUT(b).n = x2 - x1;
    WMUT(b).marks = (module_markobj*)(RMUT(s).marks + RMUT(s).n);
    WMUT(e).n = RMUT(marks).end() - x2;
    WMUT(e).marks = (module_markobj*)(RMUT(b).marks + RMUT(b).n);
    assert(RMUT(s).n + RMUT(b).n + RMUT(e).n == (int)RMUT(marks).size());
  }

  // get the keysig vector
  const std::vector<std::pair<rat, rat> >& measure::getkeysigvect() const {
    const var_keysig& sigs((const var_keysig&)get_varbase(KEYSIGDEF_ID));
    if (sigs.userempty()) {
      READLOCK;
      return ((const var_keysigs&)get_varbase(KEYSIG_ID)).getkeysigvect(RMUT(newkey).empty() ? get_sval(COMMKEYSIG_ID) : RMUT(newkey));
    } else {
      return sigs.getkeysigvect();
    }
  }

  bool measure::getwritaccaux(const noteevbase& ev, SHLOCKPAR) const {
    assert(ev.isrlocked());
    if (!ev.getisnote() || ev.getistiedleft_nomut()) return false;
    if (RMUT(((const noteev&)ev).cautacc)) return true;
    assert(accrules.find(ev.get_sval_nomut(ACCRULE_ID)) != accrules.end()); // shouldn't ever happen
    accruleenum ru = accrules.find(ev.get_sval_nomut(ACCRULE_ID))->second;
    if (ru == accrule_notenats) return true;
    int no0 = numtoint(((const noteev&)ev).getwrittennote_nomut());
    std::pair<rat, rat> accs;
    {
      UNLOCKREAD;
      accs = getkeysigvect()[todiatonic(no0)]; // get keysig accs
      DBG("The KEYSIG accidental for note " << no0 << " is " << accs.first << std::endl);
      if (ru == accrule_meas) {
	const noteev* invnote = 0;
	for (eventmap_constit e(CMUT(events).begin()); e != CMUT(events).end() && e->second != &ev; ++e) {
	  if (e->second->getisnote() && numtoint(e->second->getwrittennote()) == no0 && !e->second->getistiedleft()) { // same written note
	    std::pair<rat, rat> ba(((noteev*)e->second)->getbothaccs());
	    if (!invnote || ba != invnote->getbothaccs()) accs = ba;
	    if (e->second->isinvisible()) invnote = ((noteev*)e->second); else invnote = 0;
	  }
	}
      }
    }
    return (RMUT(((const noteev&)ev).acc1) != accs.first || RMUT(((const noteev&)ev).acc2) != accs.second);
  }

#ifndef NDEBUG
  void part::resetstage() {
    std::for_each(getmeass().begin(), getmeass().end(), boost::lambda::bind(&measure::resetstage, boost::lambda::bind(&measmap_val::second, boost::lambda::_1)));      
  }
#endif
#ifndef NDEBUGOUT
  void part::showstage() {
    std::for_each(getmeass().begin(), getmeass().end(), boost::lambda::bind(&measure::showstage, boost::lambda::bind(&measmap_val::second, boost::lambda::_1)));      
  }
#endif
  
  void part::collectallvoices(std::set<int>& vv) {
    std::set<int> v;
    for (measmap_it i(CMUT(meass).begin()); i != CMUT(meass).end(); ++i) i->second->collectallvoices(v);
    voicescache.assign(v.begin(), v.end());
    vv.insert(v.begin(), v.end());
  }
  
  void part::collectallstaves(std::set<int>& ss) {
    std::set<int> s;
    for (measmap_it i(CMUT(meass).begin()); i != CMUT(meass).end(); ++i) i->second->collectallstaves(s);
    stavescache.assign(s.begin(), s.end());
    ss.insert(s.begin(), s.end());
  }

  void part::filltmppart(measmapview& m) {
    for (measmap_it i(CMUT(meass).begin()); i != CMUT(meass).end(); ++i) {
      measure* m0 = i->second;
      m0->settmpself(m.insert(offgroff(m0->gettime(), (fint)def->ind), m0));
    }
  }
  
  // this is different from the staff clefsetting !
  void noteevbase::getclefsinit() {
    DBG("XXX " << meas->getpartid() << std::endl);
    const module_value& cc(get_lval(CHOOSECLEF_ID));
    assert(cc.type == module_list);
    struct module_intslist cprt(meas->getpartclefs());
    if (cprt.n <= 0) {
      CERR << "no clefs in part `" << meas->getpartid() << '\'' << std::endl;
      throw errbase(); //clefsiniterr();
    }      
    if (cc.val.l.n > 0) {
      clefscache.reset(new std::vector<int>);
      for (module_value* i = cc.val.l.vals, *ie = cc.val.l.vals + cc.val.l.n; i < ie; ++i) {
	assert(i->type == module_string);
	int cl = strtoclef(i->val.s);
	if (!std::binary_search(cprt.ints, cprt.ints + cprt.n, cl)) {
	  CERR << "invalid clef `" << cleftostr(cl) << '\'';
	  pos.printerr();
	  throw errbase(); //clefsiniterr();
	}
	clefscache->push_back(cl); // should already be valid clef string
      }
      std::sort(clefscache->begin(), clefscache->end());
    } else clefscache.reset(); // ok for clefscache to be 0, this means go to parent measure for clefs
    {
      WRITELOCK;
      WMUT(marks).sort(marksetlt());
      marksbase::cachessort(); // MARKS
      WMUT(staves).clear();
    }
    module_value l(get_lval(USERSTAVES_ID));
    assert(l.type == module_list);
    int ns = meas->getpartnstaves();
    if (ns == 0) {
      CERR << "no staves in part `" << meas->getpartid() << '\'' << std::endl;
      throw errbase(); //clefsiniterr();
    }
    if (l.val.l.n > 0) {
      for (module_value* i = l.val.l.vals, *ie = l.val.l.vals + l.val.l.n; i < ie; ++i) {
	assert(i->type == module_int);
	if (ns > 0 && i->val.i > ns) {
	  CERR << "invalid staff number " << i->val.i;
	  pos.printerr();
	  throw errbase(); //clefsiniterr();
	}
      }
    } else{
      WRITELOCK;
      for (int i = 1; i <= ns; ++i) WMUT(staves).push_back(i);
    }
  }
  
  void noteevbase::dopointl(const offgroff& o1, std::list<noteevbase*>& tmp, UPLOCKPAR) {
    assert(RMUT(off).groff.isntnull());
    if (RMUT(point) == point_left) {
      if (o1.off < RMUT(off).off) {
      DOLEFT:
	TOWRITELOCK;
	WMUT(dur) = RMUT(off).off - o1.off;
	WMUT(off).off = o1.off;
	WMUT(off).groff.null();
	WMUT(point) = point_none;
      } else {
	numb o(RMUT(off).off);
	int v = get1voice();
	numb r(UNLOCKP(meas)->getlastgrendoff(o, v));
	RELOCK;
	TOWRITELOCK;
	WMUT(dur) = get_rval_nomut(DEFAULTGRACEDUR_ID);
	WMUT(off).groff = r;
	WMUT(point) = point_none;
      }
      tmp.push_back(this);
      return;
    }
    assert(RMUT(point) == point_grleft);
    if (o1.off < RMUT(off).off) goto DOLEFT;
    TOWRITELOCK;
    WMUT(point) = point_none;
    if (o1.groff < RMUT(off).groff) {
      WMUT(dur) = RMUT(off).groff - o1.groff; // off and groff are correct in this case
      tmp.push_back(this);
      return;
    }
    WMUT(dur) = get_rval_nomut(DEFAULTGRACEDUR_ID);
  }
  void noteevbase::dopointr(const offgroff& o2, std::list<noteevbase*>& tmp, UPLOCKPAR) {
    assert(RMUT(off).groff.isntnull());
    if (RMUT(point) == point_right || RMUT(point) == point_auto) {
      if (RMUT(off).off < o2.off) {
      DORIGHT:
	TOWRITELOCK;
	WMUT(dur) = o2.off - RMUT(off).off;
	WMUT(off).groff.null();
	WMUT(point) = point_none;
      } else {
	numb o(RMUT(off).off);
	int v = get1voice();
	numb r(UNLOCKP(meas)->getfirstgroff(o, v));
	RELOCK;
	TOWRITELOCK;
	WMUT(dur) = get_rval_nomut(DEFAULTGRACEDUR_ID);
	WMUT(off).groff = r - RMUT(dur);
	WMUT(point) = point_none;
      }
      tmp.push_back(this);
      return;
    }
    assert(RMUT(point) == point_grright || RMUT(point) == point_grauto);
    if (RMUT(off).off < o2.off) goto DORIGHT;
    TOWRITELOCK;
    WMUT(point) = point_none;
    if (o2.groff.isntnull() && RMUT(off).groff < o2.groff) {
      WMUT(dur) = o2.groff - RMUT(off).groff;
      tmp.push_back(this);
      return;	  
    }
    WMUT(dur) = get_rval_nomut(DEFAULTGRACEDUR_ID);
  }

  void measure::deletefiller() {
    assert(isvalid());
    DBG("maybe deleting filler in measure " << this << std::endl);
    for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end(); ++i) {
      if (i->second->isfill()) {
	DBG("  deleted filler in measure " << this << std::endl);
	XMUT(events).erase(i);
	return;
      }
    }
  }
  
  void part::fixmeasures() {
    if (RMUT(newmeass).empty()) {CERR << "no measures"; integerr("measure");}
    if (RMUT(newmeass).begin()->second->gettime() != (fint)0) {CERR << "invalid time/duration"; integerr("measure");}
    for (measmap_constit m(RMUT(newmeass).begin()), me(boost::prior(RMUT(newmeass).end())); m != me; ++m) {
      measmap_constit nm(boost::next(m));
      const measure& mea(*m->second);
      assert(mea.gettime().israt());
      assert(mea.getdur().israt());
      if (mea.gettime() < (fint)0 || mea.getdur() <= (fint)0 || !mea.fixmeascheck()) {
	CERR << "invalid time/duration"; integerr("measure");
      }
      if (mea.getendtime() != nm->second->gettime()) {
	CERR << "unaligned measures"; integerr("measure");
      }
    }
    for (measmap::iterator i(CMUT(meass).begin()); i != CMUT(meass).end(); ++i) { // pickup measures
      rat p(i->second->get_rval(PICKUP_ID));
      if (p != 0) i->second->dosplit(*this, p);
    }
    module_value en(boost::prior(RMUT(newmeass).end())->second->getendtime());
    for (measmap_it m(CMUT(meass).begin()); m != CMUT(meass).end(); ++m) {
      measure& mea(*m->second);
      for (eventmap_it n(mea.getevents().begin()), ne(mea.getevents().end()); n != ne;) { // noteevent
	const numb& o(n->second->gettime_nomut());
	if (o < (fomus_int)0 || n->second->getendtime_nomut() > en) { // en is very last measure endoffset
	  CERR << "invalid note time/duration"; integerr("measure");
	}
#ifndef NDEBUG	
	measmap_it i(boost::prior(RMUT(newmeass).upper_bound(o)));
	assert(i != RMUT(newmeass).end());
	i->second->insertnew(mea.getevents().release(n++).release());
#else
	boost::prior(newmeass.upper_bound(o))->second->insertnew(mea.getevents().release(n++).release());
#endif	
      }
      assert(mea.getevents().empty());
    }
    XMUT(meass).clear();
    for (measmap::iterator i(RMUT(newmeass).begin()); i != RMUT(newmeass).end();) {
      DBG("INSERTING MEASURE " << numb(i->second->gettime()) << std::endl);
      measure* m = WMUT(newmeass).release(i++).release();
      m->setself(XMUT(meass).insert(offgroff(m->gettime()), m));
      m->getkeysig_init();
    }
    assert(RMUT(newmeass).empty());
  }

  void noteevbase::fixtimequant(numb& mx) {
    if (!RMUT(off).off.israt() || !(RMUT(off).groff.israt() || RMUT(off).groff.isnull()) || !RMUT(dur).israt()) {
      CERR << "non-rational time/duration"; integerr("time quantization");
    }
    if (RMUT(off).off < (fint)0 || RMUT(dur) < (fint)0) {
      CERR << "invalid time/duration"; integerr("time quantization");
    }
    if (RMUT(dur) == (fint)0 && RMUT(off).groff.isnull()) { // becomes a grace note
      WMUT(off).groff = meas->getlastgrendoff(RMUT(off).off, get1voice());
      WMUT(dur) = get_rval(DEFAULTGRACEDUR_ID); // default gracedur setting...
    }
    if (getendtime() > mx) mx = getendtime();
  }

  //#warning "check this, the inserted note event should receive the same tuplet information as concurrent note events"
  void part::reinsert(std::auto_ptr<noteevbase>& e, const char* what) {
    DISABLEMUTCHECK;
    measmap_it i(CMUT(meass).upper_bound(offgroff(e->gettime_nomut())));
    if (i == CMUT(meass).begin() || e->getendtime_nomut() > boost::prior(CMUT(meass).end())->second->getendtime()) {
      DBG("i == meass.begin() = " << (i == CMUT(meass).begin()) << std::endl);
      DBG("e->getendtime() = " << e->getendtime() << "  boost::prior(meass.end())->second->getendtime() = " << boost::prior(CMUT(meass).end())->second->getendtime() << std::endl);
      CERR << "invalid time/duration"; integerr(what);
    }
    measure& m = *boost::prior(i)->second;
    assert(m.gettime() <= e->gettime_nomut() && m.getendtime() >= e->gettime_nomut());
    if (!def->ismetapart() && m.getendtime() < e->getendtime_nomut()) {
      std::auto_ptr<noteevbase> e2(e->getsplitat(m.getendtime()));
      reinsert(e2, what);
    }
    DBG("inserting into measure w/ ptr " << &m << std::endl);
    m.removerest(*e);
    m.insertnew(e.release());
  }
  
  void measure::fixtimequant(numb& mx, const bool inv) {
    DBG("fixtimequant: part=" << prt->getid() << " measure=" << gettime() << std::endl);
    boost::ptr_list<noteevbase> tmp;
    for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end(); ) {
      if (inv && i->second->getvoice_nomut() < 1000) ++i; else tmp.push_back(XMUT(events).release(i++).release());
    }
    noteevbase* lne = 0;
    for (boost::ptr_list<noteevbase>::iterator e(tmp.begin()); e != tmp.end();) {
      std::auto_ptr<noteevbase> ne(tmp.release(e++).release());
      if (inv) ne->resetassdel(); else {
	if (ne->isassdel()) continue;
      }
      ne->fixtimequant(mx);
      DBG("we're in measure w/ ptr " << this << std::endl);
      if (inv && lne && lne->getfulltime_nomut() == ne->getfulltime_nomut()) {
	lne->xfermarks(*ne);
	continue;
      }
      DISABLEMUTCHECK;
      lne = ne.get();
      prt->reinsert(ne, "time quantization");
    }
  }

  numb part::fixtimequant() { // return number of empty measures that can be trimmed off the end
    numb mx((fint)-1);
    for (measmap_it m(CMUT(meass).begin()); m != CMUT(meass).end(); ++m) m->second->fixtimequant(mx, false);
    assert(!CMUT(meass).empty());
    numb o(CMUT(meass).rbegin()->second->getendtime());
    for (measmap::const_reverse_iterator i(CMUT(meass).rbegin()); i != CMUT(meass).rend(); ++i) {
      if (i->second->canremove() && mx <= i->first.off) o = i->first.off; else break;
    }
    assert(o.israt());
    return o;
  }

  // new split is to the right 
  marksbase::marksbase(MUTPARAM_ marksbase& x):MUTINITP_(marks) MUTINITP_(nmarks) newm(false _MUTP) _MUTINITP(s) _MUTINITP(b) _MUTINITP(e) { // don't care about oldmarks, isinv = true if splitting rests in voice >1000
    WMUT(x.marks).sort(markslr()); // sort by left, leftright, right
    while (!RMUT(x.marks).empty() && RMUT(x.marks).back().getmove() == move_right) CMUT(marks).push_back(WMUT(x.marks).pop_back().release());
    for (boost::ptr_vector<markobj>::iterator i(RMUT(x.marks).begin()); i != RMUT(x.marks).end(); ++i) {
      assert(i->getmove() != move_right);
      if (i->getmove() == move_leftright) CMUT(marks).push_back(new markobj(*i));
    }    
    WMUT(x.marks).sort(marksetlt()); x.cachessort();
    CMUT(marks).sort(marksetlt()); DISABLEMUTCHECK; cachessort();
  }

  void marksbase::unsplitmarks(marksbase& x) {
    WMUT(x.marks).sort(markslr()); // sort by left, leftright, right
    while (!RMUT(x.marks).empty() && RMUT(x.marks).back().getmove() < move_left) WMUT(marks).push_back(WMUT(x.marks).pop_back().release());
    WMUT(marks).sort(marksetlt()); /*DISABLEMUTCHECK;*/ cachessort();    
  }

  void restev::checkrestvoices(boost::ptr_list<noteevbase>& tmp) { // duplicate 1 rest for ea. voice
    UPREADLOCK;
    assert(RMUT(voices).size() > 0);
    for (std::vector<int>::iterator i(boost::next(RMUT(voices).begin())); i != RMUT(voices).end(); ++i) {
      tmp.push_back(new restev(*this, *i));
    }
    if (RMUT(voices).size() > 1) {TOWRITELOCK; WMUT(voices).resize(1);}
  }

  bool isspecial(const marksbase& x, const marksbase& y) {
    for (boost::ptr_vector<markobj>::const_iterator i(x.getmarkslst().begin()); i != x.getmarkslst().end(); ++i) {
      for (boost::ptr_vector<markobj>::const_iterator j(y.getmarkslst().begin()); j != y.getmarkslst().end(); ++j) {
	if (markisspecialpair(i->getmarkid(), j->getmarkid())) return true;
      }
    }
    return false;
  }

  inline bool checkpruneaux(noteevbase& x, const noteevbase& y) {
    boost::shared_lock<boost::shared_mutex> xxx(x.interngetmut());
    if (isspecial(x, y)) return false;
    return
      x.get1voice() == y.get1voice()
      && (x.getisrest() || y.getisrest() || x.getnote() == y.getnote())
      && ((x.isgrace() && y.isgrace())
	  ? y.getfulltime_nomut() < x.getfullendtime() && x.getfulltime_nomut() < y.getfullendtime()
	  : y.gettime() < x.getendtime() && x.gettime() < y.getendtime());
  }
  void measure::checkprune() {
    DBG("PRUNE CHECK" << std::endl);
#ifndef NDEBUGOUT
    dumpall();
#endif    
    for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end(); ++i) {
      boost::shared_lock<boost::shared_mutex> xxx(i->second->interngetmut());
      i->second->checkprune();
      for (eventmap_it j(CMUT(events).begin()); j != i; ++j) {
	if (checkpruneaux(*j->second, *i->second)) {
	  CERR << "overlapping pitches"; integerr("prune");
	}
      }
    }
  }

  void measure::fixties() {
    boost::ptr_list<noteevbase> tmp;
    for (eventmap_it n(CMUT(events).begin()); n != CMUT(events).end(); ) {
      noteevbase* ev;
      tmp.push_back(ev = XMUT(events).release(n++).release());
      boost::ptr_map<const rat, boost::nullable<divsplitstr> >& spl = ev->getsplits();
      boost::ptr_map<const rat, boost::nullable<divsplitstr> >::const_iterator i(spl.begin());
      if (i != spl.end()) { // ev doesn't need to be locked here
	rat et(numtorat(ev->isgrace(MUTDBG) ? ev->getgraceendtimenochk(MUTDBG) : ev->getendtime_nomut(MUTDBG)));
	if (i->first != numtorat(ev->isgrace(MUTDBG) ? ev->getgracetime_nomut(MUTDBG) : ev->gettime_nomut(MUTDBG))) {CERR << "bad split"; integerr("note division");} 
	while (true) {
	  assert(i->first == numtorat(ev->isgrace(MUTDBG) ? ev->getgracetime_nomut(MUTDBG) : ev->gettime_nomut(MUTDBG)));
	  if (i->second) ev->filltups(i->second->tups _MUTDBG);
	  DBG((ev->gettupsvect(MUTDBG).empty() ? false : ev->gettupsvect(MUTDBG)[0].beg) << '/' << (ev->gettupsvect(MUTDBG).empty() ? false : ev->gettupsvect(MUTDBG)[0].end) << ' ');
	  if (++i == spl.end()) break;
	  if (i->first >= et) {CERR << "bad split"; integerr("note division");}
	  DBG("**splitting at " << i->first << std::endl);
	  DISABLEMUTCHECK;
	  tmp.push_back(ev = ev->getsplitat0(i->first)); // getsplitat0 doesn't split the tuplets
	}
	spl.clear();
      }
    }
    DBG(std::endl);
    for (boost::ptr_list<noteevbase>::iterator i(tmp.begin()); i != tmp.end();) {
      boost::ptr_list<noteevbase>::iterator j(boost::next(i));
      if (j == tmp.end()) break;
      if (j->unsplit(*i)) {
	DBG("fixties UNSPLITTING " << j->gettime_nomut());
	tmp.erase(j);
      } else ++i;
    }
#ifndef NDEBUGOUT
    for (boost::ptr_list<noteevbase>::iterator i(tmp.begin()); i != tmp.end(); ++i) {
      DBG((i->gettupsvect(MUTDBG).empty() ? false : i->gettupsvect(MUTDBG)[0].beg) << '/' << (i->gettupsvect(MUTDBG).empty() ? false : i->gettupsvect(MUTDBG)[0].end) << ' ');
    }
#endif    
    DBG(std::endl);
    for (boost::ptr_list<noteevbase>::iterator i(tmp.begin()); i != tmp.end();) insertnew(tmp.release(i++).release());
  }

  void measure::doprune() {
    boost::ptr_list<noteevbase> tmp;
    for (eventmap_it n(CMUT(events).begin()); n != CMUT(events).end(); ) {
      if (n->second->getclips().get()) {
	std::auto_ptr<std::set<clippair> > clips(n->second->getclips()); // steal it first--clips represent begins/ends of NEW notes
        std::auto_ptr<noteevbase> no(XMUT(events).release(n++).release());
	if (!clips->empty()) {
	  std::set<clippair>::const_iterator ie(boost::prior(clips->end()));
	  if (no->isgrace(MUTDBG)) {
	    if (ie->o2 != no->getgraceendtimenochk(MUTDBG)) no->adjustdur(ie->o2 - no->getgracetime_nomut(MUTDBG));	    
	  } else {
	    if (ie->o2 != no->getendtime_nomut(MUTDBG)) no->adjustdur(ie->o2 - no->gettime() _MUTDBG);
	  }
	  std::set<clippair>::const_iterator i(clips->begin());
	  DBG("i->o1 = " << numb(i->o1) << std::endl);
	  while (true) {
	    if (i->o1 < (no->isgrace(MUTDBG) ? no->getgracetime_nomut(MUTDBG) : no->gettime_nomut(MUTDBG)) || i->o1 >= i->o2) {CERR << "bad prune times"; integerr("prune");} // is in range?
	    if (no->isgrace(MUTDBG)) {
	      if (i->o1 > no->getgracetime_nomut(MUTDBG)) no->adjustgracetime(i->o1 _MUTDBG);	      
	    } else {
	      if (i->o1 > no->gettime_nomut(MUTDBG)) no->adjusttime(i->o1 _MUTDBG);
	    }
	    if (i == ie) break;
	    noteevbase* no0 = no.get();
	    tmp.push_back(no);
	    assert(no0->isvalid());
	    no.reset(no0->getsplitat(i->o2));
	    assert(no0->isvalid());
            assert(no->isvalid());
	    ++i;
   	  }
	  tmp.push_back(no);
          assert(!no.get());
	}
      } else ++n;
    }
    for (boost::ptr_list<noteevbase>::iterator i(tmp.begin()); i != tmp.end();) insertnew(tmp.release(i++).release());
  }

  void measure::getkeysig_init() {
    const std::vector<std::pair<rat, rat> >& x(getkeysig_init_aux());
    keysigref.resize(75);
    assert(x.size() == 75);
    std::vector<struct module_keysigref>::iterator j(keysigref.begin());
    for (std::vector<std::pair<rat, rat> >::const_iterator i(x.begin()); i != x.end(); ++i, ++j) {
      j->acc1.num = i->first.numerator();
      j->acc1.den = i->first.denominator();
      j->acc2.num = i->second.numerator();
      j->acc2.den = i->second.denominator();
    }
  }

  void marksbase::recachemarksaux() {
    DBG("recaching... " << RMUT(nmarks).size() << " new marks" << std::endl);
    WMUT(marks).clear();
    WMUT(marks).transfer(RMUT(marks).end(), RMUT(nmarks));
    assert(RMUT(nmarks).empty());
    DBG("new sz = " << RMUT(marks).size() << std::endl);
    WMUT(newm) = false;
  }
  
  void marksbase::spreadreplmarks(const boost::ptr_set<markobj, marksetlt>& mrks) {
    assert(!RMUT(newm));
    for (boost::ptr_set<markobj, marksetlt>::const_iterator i(mrks.begin()); i != mrks.end(); ++i) WMUT(marks).push_back(new markobj(*i));
    WMUT(marks).sort(marksetlt());
    cachessort();
    for (boost::ptr_vector<markobj>::const_iterator i(RMUT(marks).begin()); i != RMUT(marks).end(); ++i) i->checkpos();
  }
  
  void marksbase::assignmarkrem(const int type, const char* arg1, const module_value& arg2) {
    if (type < 0 || type >= (int)markdefs.size()) throw assmarkerr();
    movetoold(); // actually copies to new
    boost::ptr_set<markobj>::iterator i(RMUT(nmarks).find(markobj(type, arg1, arg2)));
    if (i != RMUT(nmarks).end()) WMUT(nmarks).erase(i); else throw assmarkerr();
  }
  
  struct haspart:public std::unary_function<const module_value&, bool> {
    const std::string& id;
    haspart(const std::string& id):id(id) {}
    bool operator()(const module_value& x) const {assert(x.type == module_string); return boost::algorithm::iequals(id, x.val.s);}
  };  
  void part::postinput(std::vector<makemeas>& makemeass, const numb& trun1, const numb& trun2) { // notes need to go to measures for .fms output (and any other preproc output)
    assert(!CMUT(meass).empty());
    for (std::vector<makemeas>::iterator i(makemeass.begin()); i != makemeass.end(); ++i) {
      if (i->off < trun1 || (trun2 > (fint)0 && i->off >= trun2)) continue;
      numb of(i->off - trun1);
      std::auto_ptr<measure> m(new measure(of, i->dur, i->measdef, *def));
      module_value pl(m->get_lval(PARTS_ID));
      assert(pl.type == module_list);
      if (pl.val.l.n <= 0 || hassome(pl.val.l.vals, pl.val.l.vals + pl.val.l.n, haspart(getid()))) {
	DBG("INSERTING MEASURE at " << of << " dur " << i->dur << " INTO PART " << getid() << std::endl);
	XMUT(meass).erase(of);
	m->setself(XMUT(meass).insert(of, m.get()));
	m.release();
      }
    }
    assert(!CMUT(meass).empty());
    def->cachesinit();
    std::for_each(getmeass().begin(), getmeass().end(), boost::lambda::bind(&measure::getkeysig_init, boost::lambda::bind(&measmap_val::second, boost::lambda::_1)));
    postinput2(trun1, trun2);
  }

  void part::postinput2(const numb& trun1, const numb& trun2) { // is also called after distributing mparts
    assert(isvalid());
    DBG("POSTINPUT ..." << this << std::endl);
#ifndef NDEBUGOUT
    dumpall();
#endif
    {
      WRITELOCK;
      for (boost::ptr_list<noteevbase>::iterator i(RMUT(tmpevs).begin()); i != RMUT(tmpevs).end();) {
	if ((i->gettime() < trun1) || (trun2 > (fint)0 && i->gettime() >= trun2)) {
	  i = WMUT(tmpevs).erase(i);
	  continue;
	}
	if (trun1 > (fint)0) {
	  DISABLEMUTCHECK;
	  i->dectime(trun1);
	}
	measmap_it j(CMUT(meass).upper_bound(offgroff(i->gettime_nomut())));
	assert(j != CMUT(meass).begin());
	boost::prior(j)->second->insertnew(WMUT(tmpevs).release(i++).release());
      }
    }
    std::for_each(getmeass().begin(), getmeass().end(), boost::lambda::bind(&measure::initclefscaches, boost::lambda::bind(&measmap_val::second, boost::lambda::_1)));
  }

  void part::postinput3() { // is also called after distributing mparts
    assert(isvalid());
    DBG("POSTINPUT3 ..." << this << std::endl);
#ifndef NDEBUGOUT
    dumpall();
#endif
    {
      for (boost::ptr_list<noteevbase>::iterator i(RMUT(tmpevs).begin()); i != RMUT(tmpevs).end();) {
	std::auto_ptr<fomus::noteevbase> ne(WMUT(tmpevs).release(i++).release());
	reinsert(ne, "metaparts");
      }
    }
    std::for_each(getmeass().begin(), getmeass().end(), boost::lambda::bind(&measure::initclefscaches, boost::lambda::bind(&measmap_val::second, boost::lambda::_1)));
    XMUT(markevs).transfer(CMUT(markevs).end(), WMUT(tmpmarkevs));
    assert(RMUT(tmpmarkevs).empty());
  }
  
  void part::inserttmps() {
    for (boost::ptr_list<noteevbase>::iterator i(RMUT(tmpevs).begin()); i != RMUT(tmpevs).end();) {
      measmap_it j(CMUT(meass).upper_bound(offgroff(i->gettime_nomut(MUTDBG))));
      assert(j != CMUT(meass).begin());
      boost::prior(j)->second->insertnew(WMUT(tmpevs).release(i++).release());
    }
  }
  
  void part::reinserttmps() { // use the reinsert method
    for (boost::ptr_list<noteevbase>::iterator i(RMUT(tmpevs).begin()); i != RMUT(tmpevs).end();) {
      std::auto_ptr<noteevbase> x(WMUT(tmpevs).release(i++).release());
      reinsert(x, "internal");
    }
  }

  void measure::dosplit(part& prt, rat p) {
    int i;
    if (p < 0) {
      p += numtorat(getdur());
      i = 0;
    } else i = 2; // right-barline applies to the extra barline
    if (p > 0 && p < getdur()) {
      WMUT(icp) = i + 2; // 2 or 4 = incomplete, right
      numb t(gettime() + p);
      prt.insertnewmeas_nomut(t, new measure(*this, prt.getdef(), t, getdur() - p, (2 - i) + 1));
      WMUT(dur) = numb(p);
    }
  }
  
  void measure::removerest(noteevbase& n) {
    offgroff t(n.getfulltime_nomut());
    bool fi = true;
    for (eventmap_it i(CMUT(events).lower_bound(t)); i != CMUT(events).end() && i->second->getfulltime_nomut() <= t; ) {
      if (i->second->get1voice() == n.get1voice()) {
	if (fi) {n.replacetups(*i->second); fi = false;}
	if (i->second->getisrest()) {
	  assert(n.getfullendtime() == i->second->getfullendtime());
	  CMUT(events).erase(i++);
	  continue;
	}
      }
      ++i;
    }
  }
  
  void noteev::estabpercinst() {
    {
      UPREADLOCK;
      if (getisperc_nomut()) {
	voicesbase::checkvoices("percussion notes");
	checkpitch("percussion notes");
	if (RMUT(percname)) {
	  TOWRITELOCK;
	  WMUT(perc) = meas->findpercinst(RMUT(percname));
	  assert(!RMUT(perc) || RMUT(perc)->isvalid()); 
	}
      }
    }
    post_apisetvalue();
  }
  
  void measure::reinsert() {
    std::vector<eventmap_it> tmp;
    for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end(); ) {
      if (i->second->isassdel()) {
	if (i->second->is0dur(MUTDBG)) {CMUT(events).erase(i++); continue;}
	tmp.push_back(i);
	i->second->resetassdel();
      }
      ++i;
    }
    DISABLEMUTCHECK; // no chance anything will conflit here
    for (std::vector<eventmap_it>::iterator i(tmp.begin()); i != tmp.end(); ++i) {
      insertnew(XMUT(events).release(*i).release());
    }
  }
    
  std::pair<int, int> noteev::postassignstaves() {
    std::pair<int, int> stcl;
    UPREADLOCK;
    if (RMUT(tiedl)) {
      stcl = (UNLOCKP((noteev*)RMUT(tiedl)))->postassignstaves();
      RELOCK;
      TOWRITELOCK;    
      stavesbase::assign(stcl.first, stcl.second);
      WMUT(stf) = meas->getstaffptr(stcl.first);
      WMUT(clf) = meas->getclefptr(stcl.first, stcl.second);
      assert(RMUT(clf) != 0);
    } else { // first one in tie chain
      stavesbase::checkstaves(); // checks the staff
      if (!meas->gethasclef(RMUT(clef))) {CERR << "invalid clef"; integerr("staves/clefs");}
      stcl.first = get1staff();
      stcl.second = RMUT(clef);
      TOWRITELOCK;    
      WMUT(stf) = meas->getstaffptr(stcl.first);
      WMUT(clf) = meas->getclefptr(stcl.first, stcl.second);
      assert(RMUT(clf) != 0);
    }
    return stcl;
  }

  int noteev::postassignvoices() {
    UPREADLOCK;
    int c;
    if (RMUT(tiedl)) {
      c = (UNLOCKP((noteev*)RMUT(tiedl)))->postassignvoices();
      RELOCK;
      TOWRITELOCK;
      voicesbase::assign(c);
    } else { // first in tie chain
      voicesbase::checkvoices("voices");
      c = get1voice();
    }
    return c;
  }
  
  void noteevbase::domerge() {
    assert(isntlocked());
    {
      UPREADLOCK;
      if (RMUT(mergeto)) {
	checkmerge(*RMUT(mergeto));
	int v = get1voice();
	{
	  TOWRITELOCK;
	  WMUT(voices) = RMUT(RMUT(mergeto)->voices);
	}
	meas->insertnew(new restev(gettime(), getgracetime(), getdur(), v, get1staff(), pos, RMUT(marksbase::marks)));
      }
    }
    assert(isntlocked());
    post_apisetvalue();
  }

  void part::mergefrom(part& x, const numb& shift) {
    for (measmap_it i(CMUT(x.meass).begin()); i != CMUT(x.meass).end(); ++i) {
      measure *m = i->second->fomclone(def, shift);
      m->setself(CMUT(meass).insert(i->first, m));
    }
    for (boost::ptr_list<noteevbase>::iterator i(RMUT(x.tmpevs).begin()); i != RMUT(x.tmpevs).end(); ++i) CMUT(tmpevs).push_back(i->fomclone(shift));
    for (boost::ptr_vector<modobjbase>::iterator i(CMUT(x.markevs).begin()); i != CMUT(x.markevs).end(); ++i) CMUT(markevs).push_back(((markev&)*i).fomclone(shift));      
  }
  
  bool measure::getisbeginchord(eventmap_it i, const offgroff& ti, const int v) const { // beginning in arbitrary eventmap ordering only!
    while(i != CMUT(events).begin()) {
      --i;
      bool bl;
      if (i->second->beginchordaux(bl, ti, v)) return bl;
    };
    return true;
  }
  bool measure::getisendchord(eventmap_it i, const offgroff& ti, const int v) const { // end in arbitrary eventmap ordering only!
    ++i;
    for (; i != CMUT(events).end(); ++i) {
      bool bl;
      if (i->second->endchordaux(bl, ti, v)) return bl;
    };
    return true;
  }
  bool measure::getischordlow(const eventmap_it& i0, const offgroff& ti, const int v, const rat& n) const { // is it the lowest note?
    eventmap_it i(i0);
    while(i != CMUT(events).begin()) {
      --i;
      bool bl;
      if (i->second->lowchordaux1(bl, ti, v, n)) {if (bl) break; else return false;}
    };
    for (eventmap_it i(boost::next(i0)); i != CMUT(events).end(); ++i) {
      bool bl;
      if (i->second->lowchordaux2(bl, ti, v, n)) return bl;
    };
    return true;
  }
  bool measure::getischordhigh(const eventmap_it& i0, const offgroff& ti, const int v, const rat& n) const { // is it the highest note?
    eventmap_it i(i0);
    while(i != CMUT(events).begin()) {
      --i;
      bool bl;
      if (i->second->highchordaux1(bl, ti, v, n)) {if (bl) break; else return false;}
    };
    for (eventmap_it i(boost::next(i0)); i != CMUT(events).end(); ++i) {
      bool bl;
      if (i->second->highchordaux2(bl, ti, v, n)) return bl;
    };
    return true;
  }
  
  bool measure::getoctavebegin(eventmap_it ev, const offgroff& ti, const int st) const {
    while (true) {
      if (ev == CMUT(events).begin()) return prt->getoctavebegin(self, ti, st);
      --ev;
      if (!(ev->first < ti)) continue;
      bool bl;
      if (ev->second->getoctaux(bl, st)) return bl;
    }
  }
  bool part::getoctavebegin(measmap_it me, const offgroff& ti, const int st) const {
    do {
      if (me == CMUT(meass).begin()) return true;
      --me;
    } while (me->second->getevents().empty());
    return me->second->getoctavebegin(boost::prior(me->second->getevents().end()), ti, st);
  }
  bool measure::getoctaveend(eventmap_it ev, const offgroff& ti, const int st) const {
    while (true) {
      ++ev;
      if (ev == CMUT(events).end()) return prt->getoctaveend(self, ti, st);
      if (!(ev->first > ti)) continue;
      bool bl;
      if (ev->second->getoctaux(bl, st)) return bl;
    }
  }
  bool part::getoctaveend(measmap_it me, const offgroff& ti, const int st) const {
    do {
      ++me;
      if (me == CMUT(meass).end()) return true;
    } while (me->second->getevents().empty());
    return me->second->getoctavebegin(me->second->getevents().begin(), ti, st);
  }

  int measure::getvoiceinstaff(const noteevbase& no, const offgroff& ti, const int vo, const int st) const { // is it the highest note?    
    bool vgr = false;
    eventmap_it i(no.getself());
    std::set<int> bf;
    while(i != CMUT(events).begin()) {
      --i;
      bool bl;
      if (i->second->voiceinstaffaux1(bl, ti, vo, st, bf, vgr)) {if (bl) break; else continue;}
    };
    for (eventmap_it i(boost::next(no.getself())); i != CMUT(events).end(); ++i) {
      bool bl;
      if (i->second->voiceinstaffaux2(bl, ti, vo, st, bf, vgr)) {if (bl) break; else continue;}
    };
    return (vgr || !bf.empty()) ? bf.size() + 1 : 0;
  }

  const module_value& event::get_lval0(const int id, const bool nomut) const {
    setmap_constit i(sets.find(id));
    if (i != sets.end()) return i->second->getmodval();
    if (!meas) return fom_get_lval_up(id);
    const module_value* ret;
    if (meas->get_lval0(id, ret)) return *ret;
    if (nomut) {
      if (RMUT(clf) && RMUT(clf)->get_lval0(id, ret)) return *ret;
      if (RMUT(stf) && RMUT(stf)->get_lval0(id, ret)) return *ret;
      return meas->get_lval_up(id, *this);
    } else {
      {
	READLOCK;
	if (RMUT(clf) && UNLOCKP(RMUT(clf))->get_lval0(id, ret)) return *ret;
      } {
	READLOCK;
	if (RMUT(stf) && UNLOCKP(RMUT(stf))->get_lval0(id, ret)) return *ret;
      } {
	READLOCK;
	return meas->get_lval_up(id, *this);
      }
    }
  }
  const std::string& event::get_sval0(const int id, const bool nomut) const {
    setmap_constit i(sets.find(id));
    if (i != sets.end()) return i->second->getsval();
    if (!meas) return fom_get_sval_up(id);
    const std::string* ret;
    if (meas->get_sval0(id, ret)) return *ret;
    if (nomut) {
      if (RMUT(clf) && RMUT(clf)->get_sval0(id, ret)) return *ret;
      if (RMUT(stf) && RMUT(stf)->get_sval0(id, ret)) return *ret;
      return meas->get_sval_up(id, *this);
    } else {
      {
	READLOCK;
	if (RMUT(clf) && UNLOCKP(RMUT(clf))->get_sval0(id, ret)) return *ret;
      } {
	READLOCK;
	if (RMUT(stf) && UNLOCKP(RMUT(stf))->get_sval0(id, ret)) return *ret;
      } {
	READLOCK;
	return meas->get_sval_up(id, *this);
      }
    }
  }
  ffloat event::get_fval0(const int id, const bool nomut) const {
    setmap_constit i(sets.find(id));
    if (i != sets.end()) return i->second->getfval();
    if (!meas) return fom_get_fval_up(id);
    ffloat ret;
    if (meas->get_fval0(id, ret)) return ret;
    if (nomut) {
      if (RMUT(clf) && RMUT(clf)->get_fval0(id, ret)) return ret;
      if (RMUT(stf) && RMUT(stf)->get_fval0(id, ret)) return ret;
      return meas->get_fval_up(id, *this);
    } else {
      {
	READLOCK;
	if (RMUT(clf) && UNLOCKP(RMUT(clf))->get_fval0(id, ret)) return ret;
      } {
	READLOCK;
	if (RMUT(stf) && UNLOCKP(RMUT(stf))->get_fval0(id, ret)) return ret;
      } {
	READLOCK;
	return meas->get_fval_up(id, *this);
      }
    }
  }
  rat event::get_rval0(const int id, const bool nomut) const {
    setmap_constit i(sets.find(id));
    if (i != sets.end()) return i->second->getrval();
    if (!meas) return fom_get_rval_up(id);
    rat ret;
    if (meas->get_rval0(id, ret)) return ret;
    if (nomut) {
      if (RMUT(clf) && RMUT(clf)->get_rval0(id, ret)) return ret;
      if (RMUT(stf) && RMUT(stf)->get_rval0(id, ret)) return ret;
      return meas->get_rval_up(id, *this);
    } else {
      {
	READLOCK;
	if (RMUT(clf) && UNLOCKP(RMUT(clf))->get_rval0(id, ret)) return ret;
      } {
	READLOCK;
	if (RMUT(stf) && UNLOCKP(RMUT(stf))->get_rval0(id, ret)) return ret;
      } {
	READLOCK;
	return meas->get_rval_up(id, *this);
      }
    }
  }
  fint event::get_ival0(const int id, const bool nomut) const {
    setmap_constit i(sets.find(id));
    if (i != sets.end()) return i->second->getival();
    if (!meas) return fom_get_ival_up(id);
    fint ret;
    if (meas->get_ival0(id, ret)) return ret; // measure first
    if (nomut) {
      if (RMUT(clf) && RMUT(clf)->get_ival0(id, ret)) return ret;
      if (RMUT(stf) && RMUT(stf)->get_ival0(id, ret)) return ret;
      return meas->get_ival_up(id, *this); // go up to part
    } else {
      {
	READLOCK;
	if (RMUT(clf) && UNLOCKP(RMUT(clf))->get_ival0(id, ret)) return ret;
      } {
	READLOCK;
	if (RMUT(stf) && UNLOCKP(RMUT(stf))->get_ival0(id, ret)) return ret;
      } {
	READLOCK;
	return meas->get_ival_up(id, *this); // go up to part
      }
    }
  }
  const varbase& event::get_varbase0(const int id, const bool nomut) const {
    setmap_constit i(sets.find(id));
    if (i != sets.end()) return *i->second;
    if (!meas) return fom_get_varbase_up(id);
    const varbase* ret;
    if (meas->get_varbase0(id, ret)) return *ret; // measure first
    if (nomut) {
      if (RMUT(clf) && RMUT(clf)->get_varbase0(id, ret)) return *ret;
      if (RMUT(stf) && RMUT(stf)->get_varbase0(id, ret)) return *ret;
      return meas->get_varbase_up(id, *this); // go up to part    
    } else {
      {
	READLOCK;
	if (RMUT(clf) && UNLOCKP(RMUT(clf))->get_varbase0(id, ret)) return *ret;
      } {
	READLOCK;
	if (RMUT(stf) && UNLOCKP(RMUT(stf))->get_varbase0(id, ret)) return *ret;
      } {
	READLOCK;
	return meas->get_varbase_up(id, *this); // go up to part
      }
    }
  }
  
  int measure::getconnbeamsleft(eventmap_constit i, const offgroff& ti, const int vo, const int bl) const {
    while (i != CMUT(events).begin()) {
      --i;
      int nb;
      if (i->first < ti && i->second->connbeamsleftaux(nb, vo)) return (nb < bl ? nb : bl);
    }
    return bl;
  }

  int measure::getconnbeamsright(eventmap_constit i, const offgroff& ti, const int vo, const int br) const {
    ++i;
    while (i != CMUT(events).end()) {
      int nb;
      if (i->first > ti && i->second->connbeamsrightaux(nb, vo)) return (nb < br ? nb : br);
      ++i;
    }
    return br;      
  }

  fomus_rat measure::timesig() const {
    rat du(numtorat(RMUT(dur)));
    rat b(get_rval(BEAT_ID));
    bool c = get_ival(COMP_ID);
    if (c) b = rat(b.numerator() * 3, b.denominator() * 2);
    numb ts(get_lval(TIMESIG_ID));
    assert(ts.islist());
    if (ts.val.l.vals->val.i > 0) { // see if it's valid, if so return it
      rat tsr(ts.val.l.vals->val.i, ts.val.l.vals[1].val.i);
      if (du == tsr / b) { // the timesig is ok
	fomus_rat re = {ts.val.l.vals->val.i, ts.val.l.vals[1].val.i};
	return re;
      }
    }
    numb tss(get_lval(TIMESIGS_ID));
    assert(tss.islist());
    for (const module_value* i(tss.val.l.vals), *ie(tss.val.l.vals + tss.val.l.n); i < ie; ++i) {
      rat tsr(i->val.l.vals->val.i, i->val.l.vals[1].val.i);
      if (du == tsr / b) { // the timesig is ok
	fomus_rat re = {i->val.l.vals->val.i, i->val.l.vals[1].val.i};
	return re;
      }      
    }
    int d = get_ival(TIMESIGDEN_ID);
    assert(d > 0);
    du *= b; // * d;  3/2
    fomus_rat re = {du.numerator(), du.denominator()};
    fomus_int de(get_ival(TIMESIGDEN_ID));
    if (c) de *= 2;
    while (re.den < de) {
      re.num *= 2;
      re.den *= 2;
    }
    return re;
  }
  
  void measure::assigndetmark0(numb off0, const int voice, const int type, const char* arg1, const struct module_value& arg2) {
    DBG("**off0=" << off0 << " measoff=" << RMUT(off).off << std::endl);
    if (off0 < RMUT(off).off) off0 = RMUT(off).off;
    numb eo(getendoff());
    if (off0 > eo) off0 = eo;
    UPREADLOCK;
    for (boost::ptr_multimap<const numb, noteevbase>::iterator i(RMUT(newevents).lower_bound(off0)); i != RMUT(newevents).end() && i->first <= off0; ++i) {
      DISABLEMUTCHECK;
      if (i->second->get1voice() == voice) {
	i->second->assignmarkins(type, arg1, arg2);
	return;
      }
    }
    restev* x;
    TOWRITELOCK;
    if (off0 >= eo) {
      DBG("**sticking in grace note at offset " << eo << ", dur 0, voice " << voice << std::endl);
      WMUT(newevents).insert(eo, x = new restev(eo, (fomus_int)2000, get_rval(DEFAULTGRACEDUR_ID), voice, filepos("(internal)"))); // grace note... don't use max/2 here to avoid later math errors
      assert(x->isvalid());
    } else { // normal note
      DBG("**sticking in non-grace note at offset " << off0 << ", dur 0, voice " << voice << std::endl);
      WMUT(newevents).insert(off0, x = new restev(off0, std::numeric_limits<fint>::max() / 2 /*module_none*/, (fomus_int)0, voice, filepos("(internal)")));
      assert(x->isvalid());
      //assert(x->ispoint_nomut());
    }
    DISABLEMUTCHECK;
    x->assignmarkins(type, arg1, arg2);
    assert(x->isvalid());
    DBG("created invisible mark REST at " << x << std::endl);
  }

  void measure::dopostmarkevs() {
    for (boost::ptr_multimap<const numb, noteevbase>::iterator i(RMUT(newevents).begin()); i != RMUT(newevents).end(); ) {
      noteevbase* e;
      insertnew(e = WMUT(newevents).release(i++).release());
      DBG("moving mark REST event at " << e << std::endl);
      if (!e->getisgrace_nomut(MUTDBG)) {
	boost::ptr_multimap<const numb, noteevbase>::const_iterator j(RMUT(newevents).upper_bound(e->gettime_nomut(MUTDBG)));
	e->setdur((j == RMUT(newevents).end() ? getendoff() : j->first) - e->gettime_nomut(MUTDBG) _MUTDBG);
      }
      e->recachemarks0();
      e->getclefsinit();
    }
  }

  void restev::checkstaves2() {
    {
      UPREADLOCK;
      if (getismarkrest_nomut()) {
	meas->getrestmarkstaffclef(*this, LOCKARG);
      }
      DBG("checkstaves2, voice = " << get1voice() << "  staff = " << get1staff() << std::endl);
      stavesbase::checkstaves2();
      {
	TOWRITELOCK;
	int st = RMUT(staves)[0];
	WMUT(stf) = meas->getstaffptr(st);
	WMUT(clf) = meas->getclefptr(st, RMUT(clef));
      }
    }
    post_apisetvalue();
  }

  void measure::getrestmarkstaffclef(restev& ev, UPLOCKPAR) { 
    int v = ev.get1voice();
    if (v >= 3000) {TOWRITELOCK; ((stavesbase&)ev).assign(prt->getnstaves());}
    else if (v >= 2000) {TOWRITELOCK; ((stavesbase&)ev).assign(1);}
    else {
      v %= 1000;
      assert(v > 0 && v <= 128);
      offgroff ti(ev.getfulltime_nomut()), et(ev.getfullendtime());
      int st, cl;
      {
	UNLOCKUPREAD;
	for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end() && (*i)->first < et; ++i) {
	  if (i->second->matchesvoiceetime(v, ti, st, cl)) goto GOTIT;
	}
	return;
      }
    GOTIT:
      TOWRITELOCK;
      ((stavesbase&)ev).assign(st, cl);
    }
  }

  inline bool noteevbase::lastgrendoffaux(const offgroff& off, numb& ret, const int v) const {
    READLOCK;
    if (get1voice() != v) return false;
    if (isgrace() && gettime_nomut() == off) {
      if (!ispoint()) {
	module_value x(getgraceendtimenochk());
	if (x > ret) ret = x;
      }      
      return false;
    }
    return true; 
  }
  
  numb measure::getlastgrendoff(const numb& off, const int v) const {
    numb ret(-std::numeric_limits<ffloat>::max()); 
    for (eventmap_constit i(CMUT(events).lower_bound(off)); i != CMUT(events).end(); ++i) {
      if (i->second->lastgrendoffaux(off, ret, v)) break;
    }
    return (ret <= -std::numeric_limits<ffloat>::max()) ? (fint)0 : ret;
  }

  inline bool noteevbase::firstgrendoffaux(const offgroff& off, numb& ret, const int v) const {
    READLOCK;
    if (get1voice() != v) return false;
    if (isgrace() && gettime() == off) {
      if (!ispoint()) {
	ret = getgracetime();
	return true;
      }
      return false;
    }
    return true;
  }
  
  numb measure::getfirstgroff(const numb& off, const int v) const {
    numb ret((fint)0);
    for (eventmap_constit i(CMUT(events).lower_bound(off)); i != CMUT(events).end(); ++i) {
      if (i->second->firstgrendoffaux(off, ret, v)) break;
    }
    return ret;
  }

  void measure::collectallvoices(std::set<int>& vv) {
    std::set<int> v;
    for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end(); ++i) i->second->collectallvoices(v);
    voicescache.assign(v.begin(), v.end());
    vv.insert(v.begin(), v.end());
  }

  void measure::collectallstaves(std::set<int>& ss) {
    std::set<int> s;
    for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end(); ++i) i->second->collectallstaves(s);
    stavescache.assign(s.begin(), s.end());
    ss.insert(s.begin(), s.end());
  }

  void measure::checkoct(eventmap_it e, const offgroff& ti, const int st, const int os) {
    for (; e != CMUT(events).end() && e->first > ti; ++e) {
      noteevbase& v = *e->second;
      uplock xxx(v.interngetmut());
      if (st == v.get1staff()) {
	TOWRITELOCK;
	v.checkoct(os);
      }
    }
  }
  
  void measure::checkrestvoices() {
    boost::ptr_list<noteevbase> tmp;
    for (eventmap_it i(CMUT(events).begin()); i != CMUT(events).end(); ++i) i->second->checkrestvoices(tmp);
    for (boost::ptr_list<noteevbase>::iterator i(tmp.begin()); i != tmp.end(); ) insertnew(tmp.release(i++).release());
  }

  void measure::postspecial() {
    for (eventmap::iterator i(CMUT(events).begin()); i != CMUT(events).end(); ) {
      if (i->second->getisnote() && i->second->isassdel()) XMUT(events).erase(i++); else ++i;
    }
  }

  void part::trimmeasures(const fomus_rat& n) {
    measmap_it i(boost::prior(CMUT(meass).end()));
    while (i->first.off >= n) XMUT(meass).erase(i--);
    while (true) {
      numb et(i->second->getendtime());
      if (et > n) {
	i->second->setdur(n - i->first.off);
	break;
      } else if (et >= n) break;
      measure *m = new measure(*i->second, def, et);
      m->setself(i = XMUT(meass).insert(offgroff(et), m));
    }
  }
  
  void marksbase::spreadgetmarks(boost::ptr_set<markobj, marksetlt>& mrks) {
    assert(RMUT(nmarks).empty());
    WMUT(marks).sort(dontspreadless()); // spreads are later
    while (!RMUT(marks).empty() && !RMUT(marks).back().getdontspread()) {
      DBG("spread id=" << RMUT(marks).back().getmarkid() << " dontspread=" << RMUT(marks).back().getdontspread() << std::endl);
      mrks.insert(WMUT(marks).pop_back().release());
    }
  }  

#ifndef NDEBUG
  bool noteevbase::isntlocked() const {
    if (threadfd.get()->get_ival(NTHREADS_ID) > 1) return true;
    if (!lockcheck.get()) return true;
    if (((boost::shared_mutex&)mut).try_lock()) {
      ((boost::shared_mutex&)mut).unlock();
      return true;
    }
    return false;
  }
#endif

  void marksbase::remtremmarks() {
    movetoold(); // actually copies to new
    boost::ptr_set<markobj>::iterator i(RMUT(nmarks).lower_bound(markobj(mark_trem, 0, numb(-std::numeric_limits<ffloat>::max()))));
    if (i != RMUT(nmarks).end() && i->getmarkid() == mark_trem) WMUT(nmarks).erase(i);
    i = RMUT(nmarks).lower_bound(markobj(mark_trem2, 0, numb(-std::numeric_limits<ffloat>::max())));
    if (i != RMUT(nmarks).end() && i->getmarkid() == mark_trem2) WMUT(nmarks).erase(i);
    recachemarks();
  }
  void marksbase::switchtrems(const module_markids from, const module_markids to) {
    movetoold(); // actually copies to new
    boost::ptr_set<markobj>::iterator i(RMUT(nmarks).lower_bound(markobj(from, 0, numb(-std::numeric_limits<ffloat>::max()))));
    assert(i != RMUT(nmarks).end());
    markobj* x = WMUT(nmarks).release(i).release();
    x->def = &markdefs[to];
    WMUT(nmarks).insert(x);
    recachemarks();
  }

  void noteev::trem_fix(const numb& chop, WRLOCKPAR) {
    assert(!isgrace());
    // destroytiedl();
    // destroytiedr();
    if (chop.isntnull()) {
      noteevbase* s = getsplitatt(gettime_nomut() + chop); // use getsplitat (no t) to retain ties
      {
	DISABLEMUTCHECK;
	s->remtremmarks();
      }
      UNLOCKWRITE;
      meas->inserttmp(s);
    }
  }
  
  void noteev::checkacc1(const fomusdata* fom) {
    {
      UPREADLOCK;
      if (RMUT(acc1) < (fint)-128 || RMUT(acc1) > (fint)128 || RMUT(acc2) < (fint)-128 || RMUT(acc2) > (fint)128) {
	const listelvect& v(get_varbase_nomut(ACC_ID).getvectval());
	if (v.size() == 1) {
	  module_noteparts parts;
	  parts.acc1 = parts.acc2 = module_makerat(0, 1);
	  numb val0(doparseacc(fom, listel_getstring(v.front()).c_str(), true, &parts));
	  if (val0.isntnull()) {
	    TOWRITELOCK;
	    WMUT(acc1).assign(parts.acc1.num, parts.acc1.den);
	    WMUT(acc2).assign(parts.acc2.num, parts.acc2.den);
	  } else goto CHECKERR;
	} else {
	CHECKERR:
	  CERR << "invalid accidental"; integerr("accidentals"); // found during accidental integrity check" << std::endl;
	}
      }
      numb w(getwrittennote_nomut());
      if (!w.isint() || isblack(w.geti())) {
	CERR << "invalid accidental"; integerr("accidentals"); // found during accidental integrity check" << std::endl;
      }
      if (RMUT(tiedr)) {const rat& a1(RMUT(acc1)); const rat& a2(RMUT(acc2)); (UNLOCKP((noteev*)RMUT(tiedr)))->postassignacc(a1, a2); READLOCK;}
    }
    post_apisetvalue();
  }

  void part::preprocess(const numb& te, const std::string& ts) {
    assert(isvalid());
    for (measmap_it i(CMUT(meass).begin()); i != CMUT(meass).end(); ++i) i->second->preprocess();
    if (te > (fint)0) {
      boost::ptr_set<markobj> mm;
      mm.insert(new markobj(mark_tempo, ts, te));
      setmap ss;
      ss.insert(setmap_val(DETACH_ID, boost::shared_ptr<varbase>(new var_detach(1))));
      insertnewmarkev(new markev((fint)0, (fint)0, (fint)0, std::set<int>(), filepos("(internal)"), mm, point_none, ss));
    }
  }

#ifndef NDEBUGOUT
  void noteev::dumpall() const {
    DBG(this);
    DBG("    off=" << CMUT(off).off << "  groff=" << CMUT(off).groff << "  dur=" << CMUT(dur) << "  note=");
    if (CMUT(percname)) DBG(CMUT(percname)); else {
      DBG(CMUT(note) << ' ' << CMUT(acc1) << '/' << CMUT(acc2));
    }
    DBG("  dyn=" << CMUT(dyn) << "  voices=(");
    if (!CMUT(voices).empty()) {
      for (std::vector<int>::const_iterator i(CMUT(voices).begin()), ie(boost::prior(CMUT(voices).end())); i != CMUT(voices).end(); ++i) {
	DBG(*i);
	if (i != ie) DBG(' ');
      }
    }
    DBG(")  tups=(");
    if (!CMUT(tups).empty()) {
      for (std::vector<tupstruct>::const_iterator i(CMUT(tups).begin()), ie(boost::prior(CMUT(tups).end())); i != CMUT(tups).end(); ++i) {
	DBG("b=" << i->beg << ",e=" << i->end << ",u=" << i->tup.num << '/' << i->tup.den);
	if (i != ie) DBG(' ');
      }
    }
    DBG(")  ");
    for (boost::ptr_vector<markobj>::const_iterator i(CMUT(marks).begin()); i != CMUT(marks).end(); ++i) {
      DBG('[' << i->def->getname() << ',' << i->str << ',' << i->val << ']');
    }
    DBG("  staves=(");
    if (!CMUT(staves).empty()) {
      for (std::vector<int>::const_iterator i(CMUT(staves).begin()), ie(boost::prior(CMUT(staves).end())); i != CMUT(staves).end(); ++i) {
	DBG(*i);
	if (i != ie) DBG(' ');
      }
    }
    DBG(")  clef=" << CMUT(clef) << (CMUT(clf) ? "*" : ""));
    if (CMUT(beaml) > 0) DBG(" bl:" << CMUT(beaml));
    if (CMUT(beamr) > 0) DBG(" br:" << CMUT(beamr));
    DBG(" tiel=" << CMUT(tiedl) << " tier=" << CMUT(tiedr));
    DBG(std::endl);
  }
#endif    
  
#ifndef NDEBUGOUT
  void restev::dumpall() const {
    DBG("    off=" << CMUT(off).off << "  groff=" << CMUT(off).groff << "  dur=" << CMUT(dur) << "  REST  voices=(");
    if (!CMUT(voices).empty()) {
      for (std::vector<int>::const_iterator i(CMUT(voices).begin()), ie(boost::prior(CMUT(voices).end())); i != CMUT(voices).end(); ++i) {
	DBG(*i);
	if (i != ie) DBG(' ');
      }
    }
    DBG(")  tups=(");
    if (!CMUT(tups).empty()) {
      for (std::vector<tupstruct>::const_iterator i(CMUT(tups).begin()), ie(boost::prior(CMUT(tups).end())); i != CMUT(tups).end(); ++i) {
	DBG("b=" << i->beg << ",e=" << i->end << ",u=" << i->tup.num << '/' << i->tup.den);
	if (i != ie) DBG(' ');
      }
    }
    DBG(")  ");
    for (boost::ptr_vector<markobj>::const_iterator i(CMUT(marks).begin()); i != CMUT(marks).end(); ++i) {
      DBG('[' << i->def->getname() << ']');
    }
    DBG("  clef=" << CMUT(clef) << std::endl);
  }
#endif

#ifndef NDEBUGOUT
  void measure::dumpall() const {
    DBG("  PART=" << prt->getid() << "  MEASURE=" << CMUT(off).off << "  DURATION=" << CMUT(dur) << std::endl);
    DBG("           KEYSIG= " << keysigcache.mode << ' ' << keysigcache.dianote << ' ' << keysigcache.acc << ' ' << keysigcache.n << std::endl);
    DBG("           DIVS= ");
    for (std::vector<fomus_rat>::const_iterator i(CMUT(divs).begin()); i != CMUT(divs).end(); ++i) DBG(numb(*i) << " ");
    DBG(std::endl);
    for (eventmap_constit i(CMUT(events).begin()); i != CMUT(events).end(); ++i) i->second->dumpall();
  }
#endif
  
#ifndef NDEBUGOUT
  void part::dumpall() const {
    DBG("bgroups: ");
    for (std::multimap<int, parts_grouptype>::const_iterator i(CMUT(bgroups).begin()); i != CMUT(bgroups).end(); ++i) {
      DBG(i->first << ' ');
      switch (i->second) {
      case parts_nogroup: DBG("none "); break;
      case parts_group: DBG("group "); break;
      case parts_choirgroup: DBG("choir "); break;
      case parts_grandstaff: DBG("grand "); break;
      }
    }
    DBG(std::endl << "egroups: ");
    for (std::set<int>::const_iterator i(CMUT(egroups).begin()); i != CMUT(egroups).end(); ++i) {
      DBG(*i << ' ');
    }
    DBG(std::endl);
    for (measmap_constit i(CMUT(meass).begin()); i != CMUT(meass).end(); ++i) {
      i->second->dumpall();
    }
  }
#endif

  void durbase::unsplittups(const durbase& rt) {
    if (RMUT(tups).size() != RMUT(rt.tups).size()) goto UNSPLITERR;
    {
      std::vector<tupstruct>::const_iterator j(RMUT(rt.tups).begin());
      for (std::vector<tupstruct>::iterator i(RMUT(tups).begin()); i != RMUT(tups).end(); ++i, ++j) {
	if (i->end || j->beg) goto UNSPLITERR;
	i->end = j->end;
      }
      return;
    }
  UNSPLITERR:
    CERR << "invalid unsplit"; integerr("note division");
  }

}
