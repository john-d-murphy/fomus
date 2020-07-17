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

#include "data.h"
#include "schedr.h"
#include "mods.h"
#include "modtypes.h"
#include "error.h"
#include "intern.h"

namespace fomus {

  inline bool modisl(const modbase* a, const modbase* b) {
    int pa = a->getpriority(), pb = a->getpriority();
    return (pa != pb) ? pa < pb : (a->getitertype() & 0x7) < (b->getitertype() & 0x7);
  }
  
  int fomusdata::getsubstages(const std::string& msg, const int modssetid, stagesvect& sta, syncs& sys, bool& filled, const int endpass, bool& fi, const runpair* fn, const bool invvoicesonly) {
    DBG("stagenum = " << stagenum << std::endl);
    std::vector<std::pair<measure*, stage*> > stageinss;
    bool gotmeasgrpmods = false;
    fint stal = sta.size(); // in case need to insert measgroup mods
    assert(!scoreparts.empty());
    static std::vector<int> nov0(1, 0), nos0(1, 0); // default staves & voices lists
    //bool fi = true;
    bool pag = false;
    {
      std::multimap<modbase*, stage*> mods; // mods in effect
      modobjbase* lno = 0;
      scorepartlist_it pe(scoreparts.end());
      measmap_it mb(scoreparts.front()->getpart().getmeass().begin()), me(scoreparts.back()->getpart().getmeass().end());
      for (scorepartlist_it p(scoreparts.begin()); p != pe; ++p) {
	measmap_it pmb((*p)->getpart().getmeass().begin()), pme((*p)->getpart().getmeass().end());
	for (measmap_it m(pmb); m != pme; ++m) {
	  eventmap_it ne(m->second->getevents().end());
	  for (eventmap_it n(m->second->getevents().begin()); n != ne; ++n) {
	    noteevbase& no(*n->second);
	    std::set<modbase*> mset;
	    std::vector<modbase*> modbs; // for iterating through SORTED note's modules... (sorted by priority)
	    if (modssetid >= 0) { 
	      module_value l(no.get_lval(modssetid));
	      assert(l.type == module_list);
	      for (module_value* md = l.val.l.vals, *mde = l.val.l.vals + l.val.l.n; md < mde; ++md) { // stick them in the set
		assert(md->type == module_string);
#ifndef NDEBUG	    
		modsmap_constit mbi(modsbyname.find(md->val.s));
		assert(mbi != modsbyname.end()); // should already have been validated
		modbase* mb = mbi->second;
#else
		modbase* mb = modsbyname.find(md->val.s)->second;
#endif	    
		if (mb->getitertype() & module_bymeasgroups) gotmeasgrpmods = true; else modbs.push_back(mb);
	      }
	      std::stable_sort(modbs.begin(), modbs.end(), modisl);
	    } else {
	      if (((runpair*)fn)->mb->getitertype() & module_bymeasgroups) gotmeasgrpmods = true;
	      else modbs.push_back(((runpair*)fn)->mb);
	    }
	    std::vector<modbase*>::const_iterator modbs1(endpass >= 0 ? modbs.begin() + std::min(endpass, (int)modbs.size()) : modbs.begin());
	    assert(endpass >= -1);
	    std::vector<modbase*>::const_iterator modbs2(endpass >= 0 ? modbs.begin() + std::min(endpass + 1, (int)modbs.size()) : modbs.end());
	    if (modbs2 > modbs1) {
	      mset.insert(modbs1, modbs2); // for quick find
	      pag = true;
	    }
	    for (std::multimap<modbase*, stage*>::iterator i(mods.begin()); i != mods.end();) { // remove stages w/o module (set end measure and part)
	      if (mset.find(i->first) == mset.end()) { // all voices/staves will be removed
		i->second->setends(boost::next(p), boost::next(m), boost::next(n));
		mods.erase(i++);
	      } else ++i;
	    } // mods now only has current stages--now add any new ones
	    bool repl = false;
	    for (std::vector<modbase*>::const_iterator i(modbs1); i != modbs2; ++i) { // add new stages--iterating through modbs in properly sorted order
	      std::multimap<modbase*, stage*>::iterator oo(mods.find(*i));
	      if (oo != mods.end() && (!lno || oo->first->getsameinst(lno, (modobjbase*)&no))) { // already there
		if (repl) { // we're replacing now--get rid of it and create a new one
		  do {
		    oo->second->setends(boost::next(p), boost::next(m), boost::next(n));
		    mods.erase(oo);
		    oo = mods.find(*i);
		  } while (oo != mods.end());
		  goto NEWONE;
		}
	      } else { // new one
	      NEWONE:
		int tt = (*i)->getitertype();
		bool bp = tt & module_bypart, bm = tt & module_bymeas;
		const std::vector<int>& nov((tt & module_byvoice)
					    ? ((tt & module_bypart) ? (*p)->getvoicescache() : ((tt & module_bymeas) ? (*m)->second->getvoicescache() : voicescache))
					    : nov0);
		const std::vector<int>& nos((tt & module_bystaff)
					    ? ((tt & module_bypart) ? (*p)->getstavescache() : ((tt & module_bymeas) ? (*m)->second->getstavescache() : stavescache))
					    : nos0);
		for (std::vector<int>::const_iterator v(nov.begin()); v != nov.end(); ++v) {
		  for (std::vector<int>::const_iterator s(nos.begin()); s != nos.end(); ++s) {
		    stage* st;
		    sta.push_back(st = new stage(stagenum, msg, sys, **i,
						 (bp || bm) ? p : scoreparts.begin(),
						 (bm ? m : (bp ? pmb : mb)),
						 fi, tt, *v, *s, (modssetid < 0 ? ((runpair*)fn)->fn.c_str() : 0),
						 invvoicesonly)); // p and m might be different depending on iter_types
		    mods.insert(std::multimap<modbase*, stage*>::value_type(*i, st));
		    fi = false;
		    repl = true; // replace the rest!, since the order needs to be preserved
		  }
		}
	      }
	    }
	    for (std::multimap<modbase*, stage*>::iterator i(mods.begin()); i != mods.end(); ++i) {
	      //m->second->getstages().insert(i->second);
	      stageinss.push_back(std::pair<measure*, stage*>(m->second, i->second));
	      DBG("^^inserted stage " << i->second->getid() << " into measure " << m->second << " at " << m->second->gettime_nomut(MUTDBG) << std::endl);
	    }
	    lno = &no;
	  }
	  for (std::multimap<modbase*, stage*>::iterator i(mods.begin()); i != mods.end();) { // remove by_meas stages
	    if (i->first->getitertype() & module_bymeas) {
	      i->second->setends(boost::next(p), boost::next(m), ne);
	      mods.erase(i++);
	    } else ++i;
	  }
	}
	eventmap_it ne(boost::prior(pme)->second->getevents().end()); // there's always at least 1 measure
	for (std::map<modbase*, stage*>::iterator i(mods.begin()); i != mods.end();) { // remove by_parts stages
	  if (i->first->getitertype() & (module_bypart | module_bymeas)) {
	    i->second->setends(boost::next(p), pme, ne);
	    mods.erase(i++);
	  } else ++i;
	}
      }
      eventmap_it ne(boost::prior(me)->second->getevents().end()); // there's always at least 1 measure & 1 part
      std::for_each(mods.begin(), mods.end(), boost::lambda::bind(&stage::setends, boost::lambda::bind(&std::map<modbase*, stage*>::value_type::second, boost::lambda::_1),
								  boost::lambda::constant_ref(pe), boost::lambda::constant_ref(me), boost::lambda::constant_ref(ne)));
    }
    if (gotmeasgrpmods) {
      if (!filled) {filltmppart(); filled = true;}
      fint stagenumadj = sta.size() - stal;
      stagenum -= stagenumadj; // reset stagenum
      stagesvect sta2; // new stages to insert
      for (measmap_it m1(tmpmeass.begin()); m1 != tmpmeass.end();) {
	assert(m1->second->isvalid());
	numb o(m1->first.off), d(m1->second->getdur());
	measmap_it m2(boost::next(m1));
	while (m2 != tmpmeass.end() && m2->first.off == o && m2->second->getdur() == d) ++m2;
	std::vector<modbase*> modbs;
	if (modssetid >= 0) {
	  module_value l(m1->second->get_lval(modssetid));
	  assert(l.type == module_list);
	  for (module_value* md = l.val.l.vals, *mde = l.val.l.vals + l.val.l.n; md < mde; ++md) { // stick them in the set
	    assert(md->type == module_string);
#ifndef NDEBUG	    
	    modsmap_constit mbi(modsbyname.find(md->val.s));
	    assert(mbi != modsbyname.end()); // should already have been validated
	    modbase* mb = mbi->second;
#else
	    modbase* mb = modsbyname.find(md->val.s)->second;
#endif	    
	    if (mb->getitertype() & module_bymeasgroups) modbs.push_back(mb);
	  }
	  std::stable_sort(modbs.begin(), modbs.end(),
			   boost::lambda::bind(&modbase::getpriority, boost::lambda::_1)
			   < boost::lambda::bind(&modbase::getpriority, boost::lambda::_2));
	} else {
	  if (((runpair*)fn)->mb->getitertype() & module_bymeasgroups) modbs.push_back(((runpair*)fn)->mb); 
	}
	std::vector<modbase*>::const_iterator modbs1(endpass >= 0 ? modbs.begin() + std::min(endpass, (int)modbs.size()) : modbs.begin());
	assert(endpass >= -1);
	std::vector<modbase*>::const_iterator modbs2(endpass >= 0 ? modbs.begin() + std::min(endpass + 1, (int)modbs.size()) : modbs.end());
	if (modbs2 > modbs1) pag = true;
	for (std::vector<modbase*>::const_iterator i(modbs1); i != modbs2; ++i) {
	  int tt = (*i)->getitertype();
	  const std::vector<int>& nov((tt & module_byvoice) ? voicescache : nov0);
	  const std::vector<int>& nos((tt & module_bystaff) ? stavescache : nos0);
	  for (std::vector<int>::const_iterator v(nov.begin()); v != nov.end(); ++v) {
	    for (std::vector<int>::const_iterator s(nos.begin()); s != nos.end(); ++s) {
	      stage* st;
	      sta2.push_back(st = new stage(stagenum, msg, sys, **i, m1, m2, fi, tt, *v, *s, (modssetid < 0 ? ((runpair*)fn)->fn.c_str() : 0), invvoicesonly));
	      for (measmap_it m(m1); m != m2; ++m) {
	      	//m->second->getstages().insert(st);
		stageinss.push_back(std::pair<measure*, stage*>(m->second, st));
		DBG("^^..inserted stage " << st->getid() << " into measure "<< m->second << " at " << m->second->gettime_nomut(MUTDBG) << std::endl);
	      }
	      fi = false;
	    }
	  }
	}
	m1 = m2;
      }
      DBG("sta2.size() = " << sta2.size() << std::endl);
      std::for_each(sta.begin() + stal, sta.end(), boost::lambda::bind(&stage::incid, boost::lambda::_1, sta2.size()));
      sta.transfer(sta.begin() + stal, sta2); // stick them in, makes sense to be before everything else
      stagenum += stagenumadj;
    } else if (modssetid == -2 && stal == (fint)sta.size()) {
      sta.push_back(new stage(stagenum, msg, sys, *fn->mb,
			      scoreparts.begin(), scoreparts.front()->getpart().getmeass().begin(),
			      true, fn->mb->getitertype(), 0, 0, ((runpair*)fn)->fn.c_str(),
			      invvoicesonly)); // p and m might be different depending on iter_types
      measmap_it e(scoreparts.back()->getpart().getmeass().end());
      sta.back().setends(scoreparts.end(), e, boost::prior(e)->second->getevents().end());
    }
    for (std::vector<std::pair<measure*, stage*> >::const_iterator i(stageinss.begin()); i != stageinss.end(); ++i) i->first->getstages().insert(i->second);
    return pag ? endpass + 1 : 0;
  }
  
  struct syncs _NONCOPYABLE {
    boost::mutex symut;
    boost::condition_variable sync; // synchronization object for all threads
    volatile fint fin;
    volatile int alv;
    volatile bool abt; // abort--mutex isn't used for this
    stagesvect sta;
    stagesvect_it i; // iterator for launching/recycling threads
    boost::shared_mutex stamut;
    const int verb;
    fomusdata& fd;
    syncs(fomusdata& fd, const int nt, const int verb, const std::vector<runpair>::iterator& b1, const std::vector<runpair>::iterator& b2, const int pa, int& endpass, bool& efix):abt(false), verb(verb), fd(fd) {
      endpass = fd.getstages(sta, *this, b1, b2, pa, endpass, efix);
      fin = sta.size();
      alv = nt > 0 ? std::min((stagesvect::size_type)nt, sta.size()) : std::numeric_limits<int>::max(); // number alive at one time
      i = sta.begin(); // set iterator to first one
    }
    void notify() {sync.notify_one();}
    stage* getstage() {
      boost::upgrade_lock<boost::shared_mutex> xxx(stamut);
      if (i == sta.end()) return 0;
      boost::upgrade_to_unique_lock<boost::shared_mutex> yyy(xxx);
      return &*i++;
    }
    void decalv() {
      {
	boost::unique_lock<boost::mutex> xxx(symut);
	--alv;
	if (alv <= 0) {xxx.unlock(); notify();}
      }
    }
  };

  void fomusdata::writeout(stagesvect& sta, syncs& sys, std::vector<runpair>::iterator b1, const std::vector<runpair>::iterator& b2, const bool pre) {
    if (pre) insfills();
    for (; b1 != b2; ++b1) {
      std::ostringstream msg;
      if (sys.verb >= 1) msg << "writing output file `" << b1->pfn << "'..."; // << std::endl;
      bool filled = false, fix;
      getsubstages(msg.str(), -1, sta, sys, filled, -1, fix = true, &*b1);
    }
    if (pre) {
      delfills();
      std::for_each(sta.begin(), sta.end(), boost::lambda::bind(&stage::resetnoteits, boost::lambda::_1));
    }
  }

  int fomusdata::getstages(stagesvect& sta, syncs& sys, const std::vector<runpair>::iterator& b1, const std::vector<runpair>::iterator& b2, const int pa, int endpass, bool& efix) {
#ifndef NDEBUG
    for (scorepartlist_it i(scoreparts.begin()); i != scoreparts.end(); ++i) (*i)->resetstage();
#endif    
    bool filled = false, fix;
#ifndef NDEBUGOUT
    dumpall();
#endif
    DBG(" getting substages" << std::endl);
    DBG("()()()()()()()() it's pass #" << pa << ", here we go... ()()()()()()()()" << std::endl);
    switch (pa) {
    case 0: { // don't need `prepare' for pa < 1
      if (sys.verb >= 1) fout << "processing..." << std::endl;
      preprocess();
      insfills();
      getsubstages(sys.verb >= 2 ? "  creating measures..." : "", MEASMOD_ID, sta, sys, filled, -1, fix = true);
      runpair x(&imod_postmeas_destr); // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;
    }
    case 1: {
      if (endpass <= 0) {
	delfills();
	//postmeas();
      }
      prepare();
      endpass = getsubstages(sys.verb >= 2 ? "  quantizing time values..." : "", TQUANTMOD_ID, sta, sys, filled, endpass, efix);
      runpair x(&imod_posttquant_destr); // *** note events are split at measure boundaries here (but not in metaparts)
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x); 
      break;
    }
    case 2: {
      prepare();
      getsubstages(sys.verb >= 2 ? "  quantizing pitches..." : "", PQUANTMOD_ID, sta, sys, filled, -1, fix = true);
      runpair x(&imod_postpquant);
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      getsubstages(sys.verb >= 2 ? "  checking data..." : "", CHECKMOD_ID, sta, sys, filled, -1, fix = true); // before tposing--should check raw (quantized) data
      getsubstages(sys.verb >= 2 ? "  transposing parts..." : "", TPOSEMOD_ID, sta, sys, filled, -1, fix = true);
      x.mb = &imod_postpquant;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      getsubstages(sys.verb >= 2 ? "  choosing voices..." : "", VOICESMOD_ID, sta, sys, filled, -1, fix = true);
      x.mb = &imod_postvoices_destr; // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;
    }
    case 3: { // fake grace notes become full durations
      prepare();
      runpair x(&imod_fillnotes1_destr); // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;
    }
    case 4: {
      fillnotes1();
      prepare();
      getsubstages(sys.verb >= 2 ? "  distributing metapart events..." : "", METAPARTSMOD_ID, sta, sys, filled, -1, fix = true);
      break;
    }
    case 5: {
      postmparts();
      prepare();
      runpair x(&imod_redoties_destr); // *** uses willbetied
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;
    }
    case 6: {
      prepare();
      getsubstages(sys.verb >= 2 ? "  processing percussion events..." : "", PERCNOTESMOD_ID, sta, sys, filled, -1, fix = true);
      runpair x(&imod_pnotes_destr); // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;
    }
    case 7: { // pruning must come early, before processing spanner marks
      prepare();
      endpass = getsubstages(sys.verb >= 2 ? "  pruning overlapping pitches..." : "", PRUNEMOD_ID, sta, sys, filled, endpass, efix); // destroys notes & rests--must start a new group of stages--don't need check, fomus internal
      runpair x(&imod_postprune_destr); // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;
    }
    case 8: {
      prepare();
      getsubstages(sys.verb >= 2 ? "  distributing mark events..." : "", MARKEVS1MOD_ID, sta, sys, filled, -1, fix = true);
      runpair x(&imod_postmarkevs_destr); // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;
    }
    case 9: {
      fillholes1(); // insert temporary events
      prepare();
      getsubstages(sys.verb >= 2 ? "  quantizing time values of marks..." : "", TQUANTMOD_ID, sta, sys, filled, -1, fix = true, 0, true);
      runpair x(&imod_posttquantinv_destr); // *** note events are split at measure boundaries here (but not in metaparts)
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x, true); 
      break;
    }
    case 10: {
      prepare();
      runpair x(&imod_inbetweenmarks); // removes duplicate marks
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      x.mb = &imod_prevspan;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      getsubstages(sys.verb >= 2 ? "  creating dynamic markings..." : "", DYNSMOD_ID, sta, sys, filled, -1, fix = true);
      x.mb = &imod_contmarksv;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      getsubstages(sys.verb >= 2 ? "  processing voice spanner marks..." : "", VMARKSMOD_ID, sta, sys, filled, -1, fix = true);
      x.mb = &imod_postvspan;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      x.mb = &imod_fixlyrs;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      x.mb = &imod_finalmarksv;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      getsubstages(sys.verb >= 2 ? "  choosing accidental spellings..." : "", ACCSMOD_ID, sta, sys, filled, -1, fix = true); // before staves--no. of ledger lines depends on spellings
      x.mb = &imod_postacc1;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x); // assigns temporary spellings if written_note test fails
      getsubstages(sys.verb >= 2 ? "  choosing staves..." : "", STAVESMOD_ID, sta, sys, filled, -1, fix = true);
      x.mb = &imod_poststaves_destr; // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;
    }
    case 11: {
      prepare();
      if (endpass <= 0) {
	runpair x(&imod_contmarkss);
	getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
	getsubstages(sys.verb >= 2 ? "  processing staff spanner marks..." : "", SMARKSMOD_ID, sta, sys, filled, -1, fix = true); // BUILT-IN
	x.mb = &imod_postsspan;
	getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
	x.mb = &imod_finalmarkss;
	getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);      
	getsubstages(sys.verb >= 2 ? "  creating octave signs..." : "", OCTSMOD_ID, sta, sys, filled, -1, fix = true); // spanners might contain octave signs
	x.mb = &imod_postoct;
	getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
	getsubstages(sys.verb >= 2 ? "  processing marks..." : "", PMARKSMOD_ID, sta, sys, filled, -1, fix = true); // pizz/arco, mute/ord. etc.., also grace note slurs, other additions...
	x.mb = &imod_postvspan;
	getsubstages("", -1, sta, sys, filled, -1, fix = true, &x); // check the mark pairs as spanners!
	x.mb = &imod_postsspan;
	getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      }
      endpass = getsubstages(sys.verb >= 2 ? "  processing special notations..." : "", SPECIALMOD_ID, sta, sys, filled, endpass, efix); // tremlos, harmonics, etc..
      runpair x(&imod_postspecial_destr); // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x); // will split notes, must be last
      break;
    }
    case 12: {
      prepare();
      runpair x(&imod_fillholes_destr); // *** fill in holes with rests here
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x); // need rests before processing marks
      break;
    }
    case 13: {
      DBG("gonna fill some holes" << std::endl);
      fillholes1(); // must follow fillholes
      DBG("gonna fill some more holes" << std::endl);
      fillholes2(); // full measure rests
      DBG("filled all them holes" << std::endl);
      prepare();
      getsubstages(sys.verb >= 2 ? "  creating cautionary accidentals..." : "", CAUTACCSMOD_ID, sta, sys, filled, -1, fix = true); // depends on octave signs, special notations
      getsubstages(sys.verb >= 2 ? "  determining layout of parts..." : "", PARTSMOD_ID, sta, sys, filled, -1, fix = true); // must be before divide and after metaparts->parts
      break;
    }
    case 14: {
      if (endpass <= 0) postparts(); // must follow parts layout
      prepare();
      endpass = getsubstages(sys.verb >= 2 ? "  dividing/tying notes..." : "", DIVMOD_ID, sta, sys, filled, endpass, efix);
      runpair x(&imod_posttie_destr); // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x); // activate all of the ties/splits
      break;
    }
    case 15: {
      prepare();
      runpair x(&imod_splittrems_destr); // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x); // will split notes, must be last
      break;
    }
    case 16: {
      prepare();
      runpair x(&imod_fillnotes2_destr); // *** deal with "point" durations
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;      
    }
    case 17: { // the rest can't really be checked--they alter flags and settings...
      fillnotes1(); // reinserts temporary events
      prepare();
      getsubstages(sys.verb >= 2 ? "  choosing rest staves..." : "", RESTSTAVESMOD_ID, sta, sys, filled, -1, fix = true);
      runpair x(&imod_poststaves2_destr); // *** deal with where rests go
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;
    }
    case 18: {
      prepare();
      endpass = getsubstages(sys.verb >= 2 ? "  merging parts..." : "", MERGEMOD_ID, sta, sys, filled, endpass, efix); // single rests, voices as chords--if in same voice/staff--make rests invisible!
      runpair x(&imod_postmerge_destr); // ***
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x); // insert rests and add a2/I/II
      break;
    }
    case 19: {
      prepare();
      getsubstages(sys.verb >= 2 ? "  beaming notes..." : "", BEAMMOD_ID, sta, sys, filled, -1, fix = true);
      runpair x(&imod_postbeams);
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x); // determine tuplet brackets appearances
      getsubstages(sys.verb >= 2 ? "  determining mark locations..." : "", MARKSMOD_ID, sta, sys, filled, -1, fix = true); // nitpicky stuff related to LAYOUT of marks
      x.mb = &imod_spreadmarks;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);      
      x.mb = &imod_finalmarksv;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      x.mb = &imod_finalmarkss;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      x.mb = &imod_markhelpers;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      x.mb = &imod_tremties;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      x.mb = &imod_barlines;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      x.mb = &imod_sysbreak;
      getsubstages("", -1, sta, sys, filled, -1, fix = true, &x);
      break;
    }
    case 20:
      fillholes3();
      prepare();
#ifndef NDEBUGOUT
      dumpall();
#endif      
    case -1:
      writeout(sta, sys, b1, b2, pa < 0);
    }
#ifndef NDEBUGOUT
    for (scorepartlist_it i(scoreparts.begin()); i != scoreparts.end(); ++i) (*i)->showstage();
#endif
    return endpass;
  }
  
#define LASTPASS 20
  
  typedef std::multimap<stage*, boost::condition_variable_any*> wakeupcondmap;
  typedef wakeupcondmap::iterator wakeupcondmap_it;
  typedef wakeupcondmap::const_iterator wakeupcondmap_constit;  
  typedef wakeupcondmap::value_type wakeupcondmap_val;  

  noteevbase* stage::api_peeknextnote(const modobjbase* note) {
    try {
      assert(isvalid());
      assert(!note || ((event*)note)->isvalid());
      eventmap_it no(note ? boost::next(note->getnoteselfit()) : initmeas->second->getevents().begin()); // throw wrongtype if it's not an `event' object
      measmap_it me(note ? ((event*)note)->getmeas().getmeasselfit(istmpmeas) : initmeas); 
      while (true) {
	if (no == noteend) return 0;
	while (no == me->second->getevents().end()) { // while note is invalid...
	  if (++me == measend) return 0;
	  if (!istmpmeas) {
	    scorepartlist_it pa(boost::prior(me)->second->getpart().getpartselfit());
	    while (me == (*pa)->getpart().getmeass().end()) { // measure is at end()...
	      if (++pa == partend) return 0;
	      me = (*pa)->getpart().getmeass().begin();
	      if (me == measend) return 0;
	    }
	  }
	  me->second->measisready();
	  no = me->second->getevents().begin();
	  if (no == noteend) return 0;
	}
	if (!fl.dontwantnote(*no->second)) break;
	++no;
      }
      assert(no->second->isvalid());
      return no->second;
    } catch (const wrongtype& e) {
      CERR << "expected " << e.what << " object in module `" << mod.getsname() << '\'' << std::endl;
      throw errbase();
    } catch (const boost::thread_interrupted& e) {
      done = true;
      throw errbase();
    }    
  }
  modobjbase* stage::api_peeknextmeas(const modobjbase* meas) {
    try { 
      assert(isvalid());
      measmap_it me(meas ? boost::next(meas->getmeasselfit(istmpmeas)) : initmeas); // must be a `measure' if it doesn't throw
      if (me == measend) return 0;
      if (!istmpmeas) {
	scorepartlist_it pa(meas ? ((measure*)meas)->getpart().getpartselfit() : initpart);
	while (me == (*pa)->getpart().getmeass().end()) {
	  if (++pa == partend) return 0;
	  me = (*pa)->getpart().getmeass().begin();
	  if (me == measend) return 0;
	}
      }
      //// me->second->measisready();
      assert(me->second->isvalid());
      return me->second;    
    } catch (const wrongtype& e) {
      CERR << "expected " << e.what << " object in module `" << mod.getsname() << '\'' << std::endl;
      throw errbase();      
    }
  }
  modobjbase* stage::api_peeknextpart(const modobjbase* part0) {
    try {
      assert(isvalid());
      if (istmpmeas) return 0;
      scorepartlist_it pa(part0 ? boost::next(part0->getpartselfit()) : initpart); // must be part if doesn't throw
      if (pa == partend) return 0;
      assert(pa->get()->isvalid());
      return pa->get();
    } catch (const wrongtype& e) {
      CERR << "expected " << e.what << " object in module `" << mod.getsname() << '\'' << std::endl;
      throw errbase();      
    }    
  }

  modobjbase* stage::api_nextmeas() {
    assert(isvalid());
    if (mitdone) return 0;
    if (firstmeas) {
      if (isfirst) printmsg();
      assert(initmeas != measend);
      mitmeas = initmeas;
      //// mitmeas->second->measisready();
      if (!istmpmeas) mitpart = initpart;
      firstmeas = false;
    } else {
      if (++mitmeas == measend) {mitdone = true; return 0;}
      if (!istmpmeas) {
	while (mitmeas == (*mitpart)->getpart().getmeass().end()) { // past end of part
	  if (++mitpart == partend) {mitdone = true; return 0;}
	  mitmeas = (*mitpart)->getpart().getmeass().begin();
	}
      }
      //// mitmeas->second->measisready(); // block until measure is accessible      
    }
    assert(mitmeas->second->isvalid());
    return mitmeas->second;
  }

  modobjbase* stage::api_nextpart() {
    assert(isvalid());
    if (pitdone || istmpmeas) return 0;
    if (firstpart) {
      if (isfirst) printmsg();
      pitpart = initpart;
      firstpart = false;
      assert(pitpart->get()->isvalid());
    } else {
      if (++pitpart == partend) {pitdone = true; return 0;}
      assert(pitpart->get()->isvalid());
    }
    return pitpart->get();
  }

  // MODULE IS ASKING FOR NEXT NOTE--BLOCK IF IT ISN'T READY YET
  modobjbase* stage::api_nextnote() {
    assert(isvalid());
    if (done) return 0;
    try {
      noteevbase *lnote, *ret; // need *ret because must look ahead and mark the last note in the measure that we're grabbing
      if (firstnote) {
	if (isfirst) printmsg();
	lnote = ret = 0;
	meas->second->measisready();
	firstnote = false;
	goto SKIPFIRST;
      } else lnote = ret = note->second;
      while(true) { // get the next note (and meas)
	++note;
      SKIPFIRST:
	if (note == noteend) {
	  if (lnote) {
	    DBG("setting note @ " << lnote->gettime_nomut(MUTDBG) << " for end of measure" << std::endl);
	    lnote->setasendofmeas();
	  } else {
	    meas->second->post_apisetvalue();
	  }	
	  done = true; break;
	}
	while (note == meas->second->getevents().end()) { // past end of measure
	  if (lnote) {
	    DBG("setting note @ " << lnote->gettime_nomut(MUTDBG) << " for end of measure" << std::endl);
	    lnote->setasendofmeas();
	    lnote = 0;
	  } else {
	    DBG("stage (" << stageobj->getid() << ") at meas (" << meas->second->gettime_nomut(MUTDBG) << ") is FINISHED" << std::endl);
	    meas->second->post_apisetvalue();
	  }
	  if (++meas == measend) {done = true; goto BREAK;}
	  if (!istmpmeas) {
	    while (meas == (*part)->getpart().getmeass().end()) { // past end of part
	      if (++part == partend) {done = true; goto BREAK;}
	      meas = (*part)->getpart().getmeass().begin();
	    }
	  }
	  meas->second->measisready(); // block until measure is accessible
	  note = meas->second->getevents().begin();
	  if (note == noteend) {done = true; goto BREAK;}
	} // got any note now
	if (fl.dontwantnote(*note->second)) continue; // no good
	if (ret) break;
	lnote = ret = note->second;
      }
    BREAK:
      if (ret) {
	assert(ret->getstageptr() == 0);
	ret->setcheck(checkin++); // check to make sure module isn't being bad and updating out of order
#ifndef NDEBUG
	ret->setstageptr(this);
#endif      
	DBG("next note is @ " << ret->gettime_nomut(MUTDBG) << std::endl);
	assert(ret->isvalid());
      }
      return ret;
    } catch (const boost::thread_interrupted& e) {
      done = true;
      throw errbase();
    }
  }
  
  void measure::measisready() {
    assert(isvalid());
    stage& thisstage = *stageobj;
    fomus_int id = thisstage.getid();
  AGAIN:
    boost::shared_lock<boost::shared_mutex> xxx0(wakeupsmut);
#ifndef NDEBUGOUT
    DBG("--stage (" << stageobj->getid() << ")--MEAS IS READY? {{");
    for (std::set<stage*, stageless>::iterator s(stages.begin()); s != stages.end(); ++s) {
      DBG((*s)->getid() << " ");
    }
    DBG("}}" << std::endl);
#endif
    for (std::set<stage*, stageless>::iterator s(stages.begin()); s != stages.end() && (*s)->getid() < id; ++s) {
      DBG("--stage (" << thisstage.getid() << ") is looking to see if it conflicts with stage (" << (*s)->getid() << ")" << std::endl);
      if (thisstage.conflicts((*s)->getflags())) {
	stage* ss = *s;
	xxx0.unlock();
	boost::unique_lock<boost::shared_mutex> xxx(wakeupsmut); // don't want an upgrade-to-unique lock here, multiple upgradables are exclusive to each other
	if (stages.find(ss) == stages.end()) goto AGAIN;
	wakeups.insert(wakeupcondmap_val(ss, &thisstage.wkup)); // store a pointer!
	thisstage.getsys().decalv(); // decrement # alive count--main thread will report deadlock
	DBG("stage (" << thisstage.getid() << ") at meas (" << CMUT(off).off << ") is SLEEPING and WAITING on stage (" << ss->getid() << ")" << std::endl);
	assert(stages.find(ss) != stages.end());
#ifndef NDEBUG
	stage* x0x0 = ss;
#endif
	thisstage.wkup.wait(xxx); // unlock xxx only after locking wkup's mutex--signaling thread later re-increments alv and removes cond from the meas
	assert(stages.find(x0x0) == stages.end());
	DBG("stage (" << thisstage.getid() << ") at meas (" << CMUT(off).off << ") is AWAKE" << std::endl);
	goto AGAIN;
      }
    }
  }
  
  // MODULE IS SETTING A VALUE
  void noteevbase::post_apisetvalue() {
    assert(isvalid());
    assert(isntlocked());
    fint co = stageobj->nextcheckout();
    if (/*stageobj.get() != stageptr ||*/ co != stagechk) { // both stageptr and stagechk in noteevbase are used to check validity of update
      CERR << "value update out of sequence";
      modprinterr();
      throw errbase();
    }
#ifndef NDEBUG
    assert(getstageptr() == stageobj.get());
    stageptr0 = 0;
#endif    
    DBG("assigned note is @ " << gettime_nomut(MUTDBG) << std::endl);
    if (isendofmeas()) meas->post_apisetvalue();
  }
  
  void measure::post_apisetvalue() {
    assert(isvalid());
    DBG("--stage (" << stageobj->getid() << ") at meas (" << CMUT(off).off << ") is FINISHING..." << std::endl);
    std::vector<boost::condition_variable_any*> bla;
    int n = 0;
    {
      boost::upgrade_lock<boost::shared_mutex> xxx(wakeupsmut);
      wakeupcondmap_it i(wakeups.lower_bound(stageobj.get()));
      boost::upgrade_to_unique_lock<boost::shared_mutex> yyy(xxx); // WRITE LOCK
      stagedone_nomut();
      while (i != wakeups.end() && i->first == stageobj.get()) {
	++n;
	bla.push_back(i->second);
	DBG("----stage (" << stageobj->getid() << ") at meas (" << CMUT(off).off << ") is telling another stage to wake up" << std::endl);
	wakeups.erase(i++);
      }
    } {
      syncs& sys(stageobj->getsys());
      boost::lock_guard<boost::mutex> xxx(sys.symut);
      sys.alv += n;
    }
    DBG("stage (" << stageobj->getid() << ") at meas (" << CMUT(off).off << ") is FINISHED" << std::endl);
    for (std::vector<boost::condition_variable_any*>::iterator x(bla.begin()); x != bla.end(); ++x) (*x)->notify_one();
  }

  struct scopedmoddata {
    const modbase& mod;
    void* data;
    scopedmoddata(const modbase& mod, void* moddata):mod(mod), data(moddata) {}
    ~scopedmoddata() {mod.freedata(data);}
  };
  
  void stage::exec(fomusdata* fd) { // aclled from thread function
    assert(isvalid());
    scopedmoddata dat(mod, mod.getdata((void*)fd));
    DBG("************************************************************************************************************************" << std::endl);
#ifndef NDEBUG
    lockcheck.reset(new bool(false));
    DBG("%%%% I reset the lockcheck %%%%" << std::endl);
#endif    
    DBG("%%%% now I'm getting ready to exec %%%%" << std::endl);
    mod.exec(fd, dat.data, filename); // API
    DBG("%%%% done with exec %%%%" << std::endl);
#ifndef NDEBUG
    DBG("%%%% getting ready to reset the lockcheck again %%%%" << std::endl);
    lockcheck.reset();
    DBG("%%%% did it! %%%%" << std::endl);
#endif    
    if (sys.abt) throw boost::thread_interrupted();
    if (!(done && checkout >= checkin)) {
      CERR << "incomplete value updates in module `" << mod.getcname() << '\'' << std::endl;
      throw errbase();
    }
  }

  boost::thread_specific_ptr<stage> stageobj(delstageobj);
  boost::thread_specific_ptr<fomusdata> threadfd(delfomusdata0);
  boost::thread_specific_ptr<char> threadcharptr(delthreadcharptr);
  
  // structure for boost's thread API
  struct exec_thread {
    syncs& sys;
    exec_thread(syncs& sys):sys(sys) {}
    void operator()();
  };
  
  void exec_thread::operator()() { // thread enter
    DBG(" thread is running" << std::endl);
    try {
      threadfd.reset(&sys.fd);
      while(true) {
	stageobj.reset(sys.getstage());
	if (!stageobj.get()) { // no more stages--reached the end
	  boost::unique_lock<boost::mutex> xxx(sys.symut);
	  --sys.alv; // thread is dead
	  DBG("Thread is dead... Down to sys.alv = [[ " << sys.alv << " ]] " << std::endl);
	  if (sys.fin <= 0 || sys.alv <= 0) {
	    xxx.unlock(); // this is really ok
	    sys.notify(); 
	  }
	  return; // exit thread
	}
	DBG("! EXECUTING STAGE {{{" << stageobj.get()->getid() << "}}}" << std::endl);
	stageobj->exec(&sys.fd);
	boost::unique_lock<boost::mutex> xxx(sys.symut);
	--sys.fin;
	DBG("Done executing... Down to sys.fin = [[ " << sys.fin << " ]]" << std::endl);
      }
    } catch (const boost::thread_interrupted& e) { // thread intrrupt
      DBG("thread has been interrupted" << std::endl);
    } catch (const errbase &e) {
      DBG("an error occurred" << std::endl);
      sys.abt = true;
    }
    {
      boost::unique_lock<boost::mutex> xxx(sys.symut);
      sys.alv = 0;
    }
    sys.notify();
  }
  
  inline void fomusdata::singlethread(const int v, const std::vector<runpair>::iterator& b1, const std::vector<runpair>::iterator& b2, const int pa, int& endpass, bool& efix) {
    syncs sys(*this, 0, v, b1, b2, pa, endpass, efix);
    assert(!stageobj.get());
    (exec_thread(sys))();
    stageobj.reset();
    assert(!stageobj.get());
    if (sys.abt) throw errbase();
  }
  
  void fomusdata::multithread(const int n, const int v, const std::vector<runpair>::iterator& b1, const std::vector<runpair>::iterator& b2, const int pa, int& endpass, bool& efix) {
    syncs sys(*this, n, v, b1, b2, pa, endpass, efix);
    if (sys.alv <= 0) return;
    boost::thread_group threads;
    {
      boost::unique_lock<boost::mutex> sylock(sys.symut); // locks the main cond mutex
      DBG("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl);
      DBG("[[ " << sys.fin << " ]] stages ready to go" << std::endl);
#ifndef NDEBUGOUT
      for (stagesvect_it i(sys.sta.begin()); i != sys.sta.end(); ++i) DBG(i->getid() << " ");
      DBG(std::endl);
#endif      
      DBG("[[ " << sys.alv << " ]] threads ready to go" << std::endl);
      DBG("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl);
      for (int i = 0; i < sys.alv; ++i) threads.create_thread(exec_thread(sys));
      while (true) {
	sys.sync.wait(sylock); // wait for threads to finish or sleep
	if (sys.fin <= 0) break;
	if (sys.alv <= 0) {
	  sylock.unlock();
	  DBG("MODULE THREADS ARE DEADLOCKED, CAN'T CONTINUE" << std::endl);
	  sys.abt = true;
	  threads.interrupt_all();
	  threads.join_all();
	  throw errbase();
	}
      }
    }
    threads.join_all();
    assert(sys.fin == 0);
    assert(sys.alv == 0);
  }

  void fomusdata::runfomus(std::vector<runpair>::iterator b1, const std::vector<runpair>::iterator& b2) {
    DBG("################# RUNNING RUNNING RUNNING" << std::endl);
    DBG("makemeass.size() = " << makemeass.size() << std::endl);
    assert(!scoreparts.empty());
    assert(scoreparts.front()->haspart());
    if (theallmpartdef->getpart().alltmpevsempty()) {
      DBG("theallmpartdef is empty" << std::endl);
#ifndef NDEBUG
      int s = scoreparts.size();
#endif      
      scoreparts.remove(theallmpartdef);
      assert((int)scoreparts.size() < s);
    } else theallmpartdef->insertallparts(scoreparts);
    if (scoreparts.size() > 1 && scoreparts.front() == thedefpartdef && scoreparts.front()->getpart().tmpevsempty()) scoreparts.pop_front();
    int srt = 0;
    const module_value& trun(get_lval(CLIP_ID));
    assert(trun.type == module_list);
    assert(trun.val.l.n == 2);
    for_each(scoreparts.begin(), scoreparts.end(), boost::lambda::bind(&partormpart_str::postinput, // also initializes caches in structures
								       boost::lambda::bind(&boost::shared_ptr<partormpart_str>::get, boost::lambda::_1),
								       boost::lambda::var(makemeass), boost::lambda::var(srt)++,
								       boost::lambda::constant_ref(trun.val.l.vals[0]), boost::lambda::constant_ref(trun.val.l.vals[1])));
    assert(!stageobj.get());
    if (fomerr.get()) throw errbase();
    collectallvoices();
    collectallstaves();
    fint n = std::min(get_ival(NTHREADS_ID), (fint)std::numeric_limits<int>::max());
    int v = get_ival(VERBOSE_ID);
    sortorder(); // reset sort indexes so module_less() works
    for (int pa = -1; pa < LASTPASS;) {
      int endpass = 0;
      bool efix = true;
      if (b1->mb->ispre()) {
	if (n <= 0) {
	  do singlethread(v, b1, boost::next(b1), pa, endpass, efix); while (endpass > 0);
	} else {
	  do multithread(n, v, b1, boost::next(b1), pa, endpass, efix); while (endpass > 0);
	}
	++b1;
	if (b1 == b2) break;
      } else {
	++pa;
	if (n <= 0) {
	  do singlethread(v, b1, b2, pa, endpass, efix); while (endpass > 0);
	} else {
	  do multithread(n, v, b1, b2, pa, endpass, efix); while (endpass > 0);
	}
      }
      assert(!stageobj.get());
      if (fomerr.get()) throw errbase();
    }
    fout << "done" << std::endl;
  }

  void getsettinginfo_aux(const setmap& sets, scoped_info_setlist& setlist) { // only called from modinout
    setlist.resize(sets.size());
    setmap_constit s(sets.begin());
    for (std::vector<info_setting>::iterator i(setlist.sets), ie(setlist.sets + setlist.n); i < ie; ++i) {
      threadfd->get_settinginfo(*i, *((s++)->second));
    }
  }

#ifndef NDEBUGOUT
  void measure::showstage() {
    DBG("@@@ MEASURE AT " << gettime_nomut(MUTDBG) << " has stages {{");
    for (std::set<stage*, stageless>::const_iterator i(stages.begin()); i != stages.end(); ++i) {
      DBG((*i)->getid() << " ");
    }
    DBG("}}" << std::endl);
  }
#endif

  const varbase& fom_get_varbase_up(const int id) {return threadfd->get_varbase(id);} // only called from module, so stageobj must be valid
  fint fom_get_ival_up(const int id) {return threadfd->get_ival(id);}
  rat fom_get_rval_up(const int id) {return threadfd->get_rval(id);}
  ffloat fom_get_fval_up(const int id) {return threadfd->get_fval(id);}
  const std::string& fom_get_sval_up(const int id) {return threadfd->get_sval(id);}
  const module_value& fom_get_lval_up(const int id) {return threadfd->get_lval(id);}
  
}
