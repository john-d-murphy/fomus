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

#include "config.h"

#include <set>
#include <map>
#include <cstring>
#include <functional>
#include <algorithm>
#include <limits>
#include <vector>
#include <iterator>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "module.h"
#include "vmarks.h"
#include "smarks.h"
#include "ifacedumb.h"

#include "ftimeaux.h"
using namespace ftimeaux;
#include "debugaux.h"

namespace vsmarks {
  
  template <class I1, class I2, class L, class C1, class C2>
  void inplace_set_difference(C1& cont1, C2& cont2) {
    I1 first1(cont1.begin());
    I2 first2(cont2.begin());
    while (first1 != cont1.end() && first2 != cont2.end()) {
      if (L()(*first1, *first2)) ++first1;
      else if (L()(*first2, *first1)) ++first2;
      else {cont1.erase(first1++); cont2.erase(first2++);}
    }
  }

  struct mark {
    int ty, grp; //, eqty;
    const char* str;
    module_value val;
    //bool cmp; // this mark is for comparison purposes only, all "cmp" marks don't compare the val/str
    mark(const int ty):ty(ty), grp(module_markspangroup(ty)), str(0) {val.type = module_none;}
    mark(const int ty, const char* str, const module_value& val):ty(ty), grp(module_markspangroup(ty)), str(str), val(val) /*, cmp(false)*/ {}
    mark(const int ty, const int grp, const char* str, const module_value& val):ty(ty), grp(grp), str(str), val(val) /*, cmp(false)*/ {}
    mark(const int ty, const int grp):ty(ty), grp(grp), str(0) /*cmp(true)*/ {val.type = module_none;}
    void assass(const module_noteobj n) const {
      DBG("assigning mark " << ty << ' ' << (str ? str : "NIL") << ' ' << val << " at " << module_time(n) << std::endl);
      marks_assign_add(n, ty, str, val);
    }
    void assrem(const module_noteobj n) const {
      DBG("removing mark " << ty << ' ' << (str ? str : "NIL") << ' ' << val << " at " << module_time(n) << std::endl);
      marks_assign_remove(n, ty, str, val);
    }
    mark getplus1(const mark& m) { // m is a begin
      // ty = (m.ty > m.eqty ? eqty + 2 : eqty); make ty match the 
      //assert((m.ty - ty) % 2 == 0);
      ty = m.ty;
      return mark(ty + 1, grp, 0, module_makenullval());
    }
    mark getplus1() const {
      return mark(ty + 1, grp, 0, module_makenullval());
    }
    mark getminus1() const {return mark(module_markbaseid(ty) - 1, grp, str, val);}
#ifndef NDEBUGOUT
    void dump() const {
      DBG("(mark ty=" << ty << " grp=" << grp << ")" << std::endl);
    }
#endif
  };
  inline bool operator<(const mark& x, const mark& y) {
    assert(x.val.type >= module_none && x.val.type <= module_rat);
    assert(y.val.type >= module_none && y.val.type <= module_rat);
    if (x.grp != y.grp || (x.grp == 0 && y.grp == 0)) return x.ty < y.ty; // if in same group, maybe treat as =
    // if (x.cmp || y.cmp) return false;
    // if (x.val.type != module_none && y.val.type != module_none) return x.val < y.val;
    // if ((x.val.type == module_none) != (y.val.type == module_none)) return y.val.type == module_none;
    return false;
  }
  // inline bool operator<(const mark& x, const int y) {return x.eqty < y;}
  // inline bool operator<(const int x, const mark& y) {return x < y.eqty;}

  struct markless:std::binary_function<const mark&, const mark&, bool> { // markless only pays attention to val, not str
    inline bool operator()(const mark& x, const mark& y) const {return x < y;}
  };
  
  struct markless2:std::binary_function<const mark&, const mark&, bool> {
    inline bool operator()(const mark& x, const mark& y) {
      assert(x.val.type >= module_none && x.val.type <= module_rat);
      assert(y.val.type >= module_none && y.val.type <= module_rat);
      if (x.ty != y.ty) return x.ty < y.ty;
      if (x.val.type != module_none && y.val.type != module_none && x.val != y.val) return x.val < y.val;
      if ((x.val.type == module_none) != (y.val.type == module_none)) return y.val.type == module_none;
      if (x.str && y.str) return strcmp(x.str, y.str) < 0;
      if (x.str || y.str) return x.str;
      return false;
    }
  };
  
  struct node {
    module_noteobj n;
  private:
    std::multiset<mark, markless2> orig; // remove!--(by type)
    std::multiset<mark, markless2> ins; // stored markobjs are the SOURCE of the begin/end mark (which isn't created yet)
  public:    
    ftimewends off, endoff;
    bool isrest;
    node(const module_noteobj n):n(n), off(n), endoff(n, 0), isrest(module_isrest(n) && !module_ismarkrest(n)) {
      DBG("module_ismarkrest(n) = " << module_ismarkrest(n) << std::endl);
    }
    void assign() {
      DBG("orig.size() = " << orig.size() << " ins.size() = " << ins.size() << std::endl);
      inplace_set_difference<std::multiset<mark>::iterator, std::multiset<mark>::iterator, markless2>(orig, ins);
      std::for_each(orig.begin(), orig.end(), boost::lambda::bind(&mark::assrem, boost::lambda::_1, n));
      std::for_each(ins.begin(), ins.end(), boost::lambda::bind(&mark::assass, boost::lambda::_1, n));
      marks_assign_done(n);
    }
    void insert(const mark& mk) {
#ifndef NDEBUGOUT
      DBG("INSERT @ " << module_time(n) << ": ");
      dump();
#endif      
      ins.insert(mk);
    }
    void insorig(const mark& mk) {orig.insert(mk);}
#ifndef NDEBUGOUT
    void dump() const {
      DBG("(node t=" << module_time(n) << " ol=" << orig.size() << " il=" << ins.size() << ")" << std::endl);
    }
#endif
  };
  inline bool operator<(const node& x, const ftimewends& y) {return x.off < y;}
  inline bool operator<(const ftimewends& x, const node& y) {return x < y.off;}

  struct nodebwds:std::binary_function<const node*, const node*, bool> {
    inline bool operator()(const node* x, const node* y) {return x->endoff < y->endoff;}
  };
  inline bool operator<(const node* x, const ftimewends& y) {return x->endoff < y;}
  inline bool operator<(const ftimewends& x, const node* y) {return x < y->endoff;}
  
  node* beginnode(const ftimewends& off, boost::ptr_vector<node>& vect, const bool cantouch, const bool coer) {
    boost::ptr_vector<node>::iterator v(cantouch
					? std::lower_bound(vect.begin(), vect.end(), off)
					: std::upper_bound(vect.begin(), vect.end(), off));
    if (v == vect.end()) return 0;
    if (!coer) {
      while (v->isrest) {
	++v;
	if (v == vect.end()) return 0;
      }
    }
    boost::ptr_vector<node>::iterator bv(v++); // best v
    for (ftimewends o(bv->off); v != vect.end() && v->off <= o; ++v) {
      if ((coer || !v->isrest) && v->endoff < bv->endoff) bv = v;
    }
    return &*bv;
  }
  node* endnode(const ftimewends& off, std::vector<node*>& vect, const bool cantouch, const bool coer) {
    std::vector<node*>::iterator v(cantouch // get one past
				   ? std::upper_bound(vect.begin(), vect.end(), off)
				   : std::lower_bound(vect.begin(), vect.end(), off));
    if (coer) {
      if (v == vect.begin()) return 0;
      --v; // decrement ONE
    } else {
      do { // decrement AT LEAST ONE
	if (v == vect.begin()) return 0;
	--v;
      } while ((*v)->isrest);
    }
    if (v == vect.begin()) return *v;
    std::vector<node*>::iterator bv(v--); // best v
    for (ftimewends o((*bv)->endoff); (*v)->endoff >= o; --v) {
      if ((coer || !(*v)->isrest) && (*v)->off > (*bv)->off) bv = v;
      if (v == vect.begin()) break;
    }
    return *bv;
  }

  typedef std::map<mark, node*, markless> nodemap;
  
#ifndef NDEBUGOUT
  std::ostream& dumpmarks(std::ostream& ou, const module_markslist& sp) {
    DBG("MARKS=");
    for (const module_markobj *j = sp.marks, *je = sp.marks + sp.n; j != je; ++j) {
      DBG(module_markid(*j) << " ");
    }
    return ou;
  }
  void dumpnodemap(const nodemap& sp) {
    for (nodemap::const_iterator i(sp.begin()); i != sp.end(); ++i) {
      i->first.dump();
      i->second->dump();
    }
  }
#endif  

  // go through end marks
  void doends(node* ipr0, nodemap& spans, nodemap& es, const boost::ptr_vector<node>::iterator& i, const module_markslist& se, const bool pass10,
	      boost::ptr_vector<node>& vect, const bool isvoice, const std::set<mark, markless>& begids, std::set<module_markobj>& endids) {
    for (const module_markobj *j = se.marks, *je = se.marks + se.n; j != je; ++j) {
      int mkid = module_markid(*j);
      if (!(isvoice ? module_markisvoice(mkid) : module_markisstaff(mkid))) continue;
      bool coer = module_markcanendonrests(mkid, i->n);
      node* ipr = (coer ? &*i : ipr0); // current valid end
      mark mk(mkid, module_markstring(*j), module_marknum(*j)); // it's an end--subtract 1 from id to get start
      DBG("MARK0=" << mk.ty << "," << mk.grp << std::endl);
      if (pass10) {i->insorig(mk);}
      --mk.ty; //--mk.eqty; // it's an end--subtract 1 from id to get begin id
      bool pass1 = (pass10 || endids.find(*j) != endids.end()); // passes: 1. markcantouch  2. markcan'ttouch (or saved in endids)
      if (pass1 ? module_markcantouch(mk.ty, i->n) : !module_markcantouch(mk.ty, i->n)) { // ends should be evaluated before the begins in this case
#ifndef NDEBUGOUT
	DBG("SPANS=" << std::endl);
	dumpnodemap(spans);
	DBG("ES=" << std::endl);
	dumpnodemap(es);
	DBG("MARK=" << mk.ty << "," << mk.grp << std::endl);
#endif	
	nodemap::iterator f(spans.find(mk)); // finds it if it's in the same group (spans = all the begins)
	//if (f == spans.end()) f = spans.find(mark(mk.ty, mk.grp)); // search in map of begins
	if (f != spans.end()) { // another begin of same type, have a match
	  if (module_markcanspanone(mk.ty, i->n)
	      ? (f->second->off <= ipr->off && f->second->endoff <= ipr->endoff)
	      : (f->second->off < ipr->off && f->second->endoff < ipr->endoff)) { // if it's valid
	    f->second->insert(f->first); // insert the begin into the first node
	    ipr->insert(mk.getplus1(f->first)); // insert proper end into the second node, also set mk to inserted id, plus1 returns end mark w/o any argument values
	  } else if (f->second->off == ipr->off && f->second->endoff == ipr->endoff && module_markcanreduce(mk.ty)) { // if NOT later offset or able to begin/end on same note (NOT valid)
	    f->second->insert(mk.getminus1()); // convert to single mark (beginid - 1) beginnind mark has the arguments
	  }
	  spans.erase(f); // either case: `begin' is gone or used up, so get rid of it
	} else { // have an `end' without a `begin'
	  // if pass = 1 && markcanspanone, look for begin in same event (?)--if there is, set flag so mark is treated as pass1 during pass 2
	  if (pass1 && module_markcanspanone(mk.ty, i->n) && begids.find(mk) != begids.end()) {
	    endids.insert(*j);
	    continue;
	  }
	  nodemap::const_iterator bi(es.find(mk)); // last end inserted
	  //if (bi == es.end()) bi = es.find(mark(mk.ty, mk.grp));
	  node* b = beginnode(bi == es.end() ? module_makerat(-1, 1) : bi->second->off, vect, pass1, coer); // first nonrest node w/ off just at or after bi->off
	  if (b && ipr) {
	    if (module_markcanspanone(mk.ty, i->n)
		? (b->off <= ipr->off && b->endoff <= ipr->endoff)
		: (b->off < ipr->off && b->endoff < ipr->endoff)) { // got room for a match
	      assert(!b->isrest);
	      b->insert(mk); // b is guaranteed to be a note
	      ipr->insert(mk.getplus1());
	    } else if (b->off == ipr->off && b->endoff == ipr->endoff && module_markcanreduce(mk.ty)) { // if same offset...
	      b->insert(mk.getminus1()); // convert to single mark (beginid - 1) beginnind mark has the arguments
	    }
	  }
	}
	es.erase(mk); es.insert(nodemap::value_type(mk, ipr)); // end in es is replaced no matter what
      }
    }
  }

  //a begin mark
  void dobegs(const mark& mk, nodemap::iterator& f, nodemap& es, const ftimewends& ioff, std::vector<node*>& vect, const bool coer, const module_noteobj nn) {
    node* e = endnode(ioff, vect, module_markcantouch(mk.ty, nn), coer); // last node w/ offset just before or at ioff
    if (e) {
      DBG("f->second->off = " << f->second->off.time << std::endl);
      if (module_markcanspanone(mk.ty, nn)
	  ? (f->second->off <= e->off && f->second->endoff <= e->endoff)
	  : (f->second->off < e->off && f->second->endoff < e->endoff)) { // room for an end
	assert(!f->second->isrest);
	f->second->insert(mk);
	assert(!e->isrest);
	e->insert(mk.getplus1()); // insert an end, e is guaranteed to be a note
      } else if (f->second->off == e->off && module_markcanreduce(mk.ty)) {
	assert(!f->second->isrest);
	f->second->insert(mk.getminus1()); // reduced version
      }
      es.erase(mk); es.insert(nodemap::value_type(mk, e)); // inserted an end, so put that in here
    }
  }

  extern "C" {
    //void vsmarks_run_fun(FOMUS f, void* moddata); // return true when done
    void vsmarks_err_fun(FOMUS f, void* mdd);
  }
  
  // by voice
  // spanners are just begin + end--module removes multiple begins and ends, adds new ones where necessary
  inline void vsmarks_run_fun(FOMUS f, const bool isvoice) {
    boost::ptr_vector<node> vect;
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n) break;
      vect.push_back(new node(n));
    }
    std::vector<node*> vectb;
    for (boost::ptr_vector<node>::iterator i(vect.begin()); i != vect.end(); ++i) vectb.push_back(&*i);
    std::sort(vectb.begin(), vectb.end(), nodebwds());
    nodemap spans; // map of begins
    nodemap es; // map of last ends
    node* ipr = 0, *lipr = 0; // previous non-rest node
    ftimewends ien(module_makerat(-1, 1));
    for (boost::ptr_vector<node>::iterator i(vect.begin()); i != vect.end(); ++i) {
      DBG(i->off.time << std::endl);
      if (!i->isrest) {
	lipr = ipr; // lipr = the last ipr
	if (lipr && lipr->endoff > ien) ien = lipr->endoff; // ien = max endoff of notes up to ipr
	ipr = &*i; // this note or previous if it's a rest
      }
      module_markslist se(module_spannerends(i->n)); // list of END marks
      module_markslist sp(module_spannerbegins(i->n)); // list of BEGIN marks
      std::set<mark, markless> sp00; // set of begin ids
      //std::transform(sp.marks, sp.marks + sp.n, std::inserter(sp00), boost::lambda::bind(module_markid, boost::lambda::_1));
      for (const module_markobj *j = sp.marks, *je = sp.marks + sp.n; j != je; ++j) sp00.insert(mark(module_markid(*j)));
      std::set<module_markobj> en00; // store ends for pass 2
      doends(ipr, spans, es, i, se, true, vect, isvoice, sp00, en00);
      boost::ptr_vector<node>::iterator inx0(i); // BEGINS
      while (inx0 != vect.end() && inx0->isrest) ++inx0; // inx0 = current or next one that isn't a rest
      std::set<mark, markless> spans0;
      if (i->isrest || (lipr && i->off > ien)) { // rest or skip
	for (nodemap::iterator f(spans.begin()); f != spans.end(); ) {
	  if (module_markcanspanrests(f->first.ty, i->n)) {++f; continue;} // if can't span a rest, must end it and start a new span
	  dobegs(f->first, f, es, i->off, vectb, false /*, true*/, i->n);
	  spans0.insert(f->first);
	  spans.erase(f++);
	}
      }
      for (const module_markobj *j = sp.marks, *je = sp.marks + sp.n; j != je; ++j) { 
	int mkid = module_markid(*j);
	if (!(isvoice ? module_markisvoice(mkid) : module_markisstaff(mkid))) continue;
	bool coer = module_markcanendonrests(mkid, i->n); // can end on rest
	node* inx = (coer ? &*i : (inx0 == vect.end() ? 0 : &*inx0));	
	mark mk(mkid, module_markstring(*j), module_marknum(*j)); 
	i->insorig(mk);
	nodemap::iterator f(spans.find(mk));
	//if (f == spans.end()) f = spans.find(mark(mk.ty, mk.grp));
	if (f != spans.end()) { // another begin of same type, f -> "old" begin
	  dobegs(f->first, f, es, i->endoff, vectb, coer /*, false*/, i->n);
	  spans.erase(f);
	}
	if (inx) {
	  spans.insert(nodemap::value_type(mk, inx)); // new beginning
	  spans0.erase(mk);
	}
      }
      if (inx0 != vect.end()) {
	for (std::set<mark, markless>::const_iterator it(spans0.begin()); it != spans0.end(); ++it) {
	  spans.insert(nodemap::value_type(*it, &*inx0));
	}
      }
      doends(ipr, spans, es, i, se, false, vect, isvoice, sp00, en00);
    }
    for (nodemap::iterator f(spans.begin()); f != spans.end(); ++f) {
      dobegs(f->first, f, es, module_makerat(std::numeric_limits<fomus_int>::max(), 1), vectb, false /*, true*/, vect.back().n);
    }
    for (boost::ptr_vector<node>::iterator i(vect.begin()); i != vect.end(); ++i) i->assign();
  }

  const char* vsmarks_err_fun(void* moddata) {return 0;}
  
}

namespace vmarks {
  extern "C" {
    void vmarks_run_fun(FOMUS f, void* moddata);
  }
  void vmarks_run_fun(FOMUS f, void* moddata) {vsmarks::vsmarks_run_fun(f, true);}
  void fill_iface(void* moddata, void* iface) { 
    ((dumb_iface*)iface)->moddata = 0;
    ((dumb_iface*)iface)->run = vmarks_run_fun;
    ((dumb_iface*)iface)->err = vsmarks::vsmarks_err_fun;
  };
  int get_setting(int n, module_setting* set, int id) {return 0;}
}

namespace smarks {
  extern "C" {
    void smarks_run_fun(FOMUS f, void* moddata);
  }
  void smarks_run_fun(FOMUS f, void* moddata) {vsmarks::vsmarks_run_fun(f, false);}
  void fill_iface(void* moddata, void* iface) { 
    ((dumb_iface*)iface)->moddata = 0;
    ((dumb_iface*)iface)->run = smarks_run_fun;
    ((dumb_iface*)iface)->err = vsmarks::vsmarks_err_fun;
  };
  int get_setting(int n, module_setting* set, int id) {return 0;}
}
