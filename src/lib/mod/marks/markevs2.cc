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

#include <list>
#include <map>
#include <set>
#include <limits>
#include <cassert>
#include <algorithm>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_set.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include "module.h"
#include "markevs2.h"
#include "ifacedumb.h"

#include "debugaux.h"
#include "ilessaux.h"
using namespace ilessaux;
#include "marksaux.h"
using namespace marksaux;

namespace markevs2 {

  const char* ierr = 0;
  
  int leftid, rightid, detachid;
  
  extern "C" {
    void markevs2run_fun(FOMUS f, void* moddata); // return true when done
    const char* markevs2err_fun(void* moddata);
  }

  enum markincl { // leftmost is most inclusive, rightmost is least inclusive
    mark_musttouch, mark_mustolap, mark_mustbeincltouch //, mark_mustbeincl // default should be mustolap
  };
  typedef std::map<std::string, markincl, isiless> itypemap;
  itypemap itypes;

  struct offgroff {
    module_value off, groff;
    offgroff(const module_markobj m):off(module_vtime(m)), groff(module_isgrace(m) ? module_vgracetime(m) : module_makenullval()) {}
    offgroff(const module_markobj m, int):off(module_vendtime(m)), groff(module_isgrace(m) ? module_vgracetime(m) : module_makenullval()) {}
  };
  inline bool operator<(const offgroff& x, const offgroff& y) {
    if (x.off == y.off) {
      return x.groff.type == module_none ? false
	: (y.groff.type == module_none ? true : x.groff < y.groff);
    } else return x.off < y.off;    
  }
  inline bool operator>(const offgroff& x, const offgroff& y) {return y < x;}
  inline bool operator<=(const offgroff& x, const offgroff& y) {return !(x > y);}
  inline bool operator>=(const offgroff& x, const offgroff& y) {return !(x < y);}
  
  struct markassnode {
    const module_noteobj n;
    fomus_int ass;
    std::multiset<mark, markless> orig; // remove!--(by type)
    std::multiset<mark, markless> ins; 
#ifndef NDEBUG
    int valid;
#endif    
    markassnode(const module_noteobj n):n(n), ass(0)
#ifndef NDEBUG
				       , valid(12345)
#endif				     
    {}
#ifndef NDEBUG
    bool isvalid() const {return valid == 12345;}
    ~markassnode() {valid = 0;}
#endif				     
    void inc() {assert(isvalid()); ++ass;}
    void dec() {assert(isvalid()); --ass;}
    void insert(const module_markobj m) {assert(isvalid()); ins.insert(mark(m));}
    void assign() {
      assert(isvalid()); 
      assert(isready());
      inplace_set_difference<std::multiset<mark>::iterator, std::multiset<mark>::iterator, markless>(orig, ins);
      std::for_each(ins.begin(), ins.end(), boost::lambda::bind(&mark::assass, boost::lambda::_1, n));
      marks_assign_done(n);
    }
    bool isready() const {assert(isvalid()); return !ass;}
  };
  struct marknode {
    const module_markobj m;
    std::list<markassnode>::iterator vn1, vn2;
    markincl lincl, rincl;
    const offgroff st, et;
    bool got, detall; // detach allowed
    std::set<int> vcs;
#ifndef NDEBUG
    int valid;
#endif    
    marknode(const module_markobj m):m(m), //vn1(0), //, vn2(0),
				     lincl(itypes.find(module_setting_sval(m, leftid))->second),
				     rincl(itypes.find(module_setting_sval(m, rightid))->second),
				     st(m), et(m, 0), got(false), detall(module_setting_ival(m, detachid))
#ifndef NDEBUG
				    , valid(12345)
#endif				     
    {
      module_intslist v(module_voices(m));
      vcs.insert(v.ints, v.ints + v.n);
    }
#ifndef NDEBUG
    bool isvalid() const {return valid == 12345;}
#endif				     
    ~marknode() {
      assert(isvalid()); 
#ifndef NDEBUG
      valid = 0;
#endif				     
      if (got) vn1->dec();
    }
    bool stick(const std::list<markassnode>::iterator& x, const offgroff& o1, const offgroff& o2, bool& done, const int v);
    void gotit();
  };
  bool marknode::stick(const std::list<markassnode>::iterator& x, const offgroff& o1, const offgroff& o2, bool& done, const int v) {
    assert(isvalid()); 
    if (o2 < st) return true;
    if (o1 > et) done = true;
    if (vcs.find(v) == vcs.end()) return false;
    switch (rincl) {
    case mark_musttouch: if (o1 <= et) break; else return false;
    case mark_mustolap: if (st >= et ? o1 <= et : o1 < et) break; else return false;
    case mark_mustbeincltouch: if (o2 <= et) break; else return false;
    }
    switch (lincl) {
    case mark_musttouch: if (o2 >= st) break; else return false;
    case mark_mustolap: if (o2 > st) break; else return false;
    case mark_mustbeincltouch: if (o1 >= st) break; else return false;
    }
    if (!got) {vn1 = x; x->inc(); got = true;}
    vn2 = x;
    return false;
  }
  void marknode::gotit() {
    assert(isvalid()); 
    if (got) {
      assert(vn1->isvalid());
      assert(vn2->isvalid());
      {
	module_markslist ma(module_singlemarks(m));
	for (const module_markobj *a(ma.marks), *ae(ma.marks + ma.n); a != ae; ++a) {
	  int id = module_markid(*a);
	  if (detall && module_markisdetachable(id)) continue;
	  std::list<markassnode>::iterator x0(vn1);
	  while (true) {
	    x0->insert(*a);
	    if (x0 == vn2) break;
	    ++x0;
	  }
	}
      }
      {
	module_markslist ma(module_spannerbegins(m));
	for (const module_markobj *a(ma.marks), *ae(ma.marks + ma.n); a != ae; ++a) {
	  int id = module_markid(*a);
	  if (detall && module_markisdetachable(id)) continue;
	  vn1->insert(*a);
	}
      }
      {
	module_markslist ma(module_spannerends(m));
	for (const module_markobj *a(ma.marks), *ae(ma.marks + ma.n); a != ae; ++a) {
	  int id = module_markid(*a);
	  if (detall && module_markisdetachable(id)) continue;
	  vn2->insert(*a);
	}
      }
    }
  }
  struct marknodeless:public std::binary_function<const marknode&, const marknode&, bool> {
    bool operator()(const marknode& x, const marknode& y) const {return x.st < y.st;}
  };
  
  void markevs2run_fun(FOMUS fom, void* moddata) {
    boost::ptr_set<marknode, marknodeless> mks;
    std::list<markassnode> ass; // assignments, one for each note object
    module_objlist mm0(module_getmarkevlist(module_nextpart()));
    for (const module_markobj *i(mm0.objs), *ie(mm0.objs + mm0.n); i != ie; ++i) mks.insert(new marknode(*i));
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n) break;
      offgroff o1(n);
      offgroff o2(n, 0);
      int v = module_voice(n);
      ass.push_back(markassnode(n));
      std::list<markassnode>::iterator x = boost::prior(ass.end());
      for (boost::ptr_set<marknode, marknodeless>::iterator i(mks.begin()); i != mks.end(); ) {
	bool done = false;
	if (i->stick(x, o1, o2, done, v)) break; // returns true if o2 is before beginning of mark obj
	if (done) {i->gotit(); mks.erase(i++);} else ++i; // done is true if the right one was assigned
      }
      while (!ass.empty() && ass.front().isready()) {
	ass.front().assign();
	ass.pop_front();
      }
    }
    for (boost::ptr_set<marknode>::iterator i(mks.begin()); i != mks.end(); ++i) i->gotit();
    mks.clear();
    while (!ass.empty()) {
      ass.front().assign();
      ass.pop_front();
    }
  }

  const char* markevs2err_fun(void* moddata) {return 0;}
  
  void fill_iface(void* moddata, void* iface) {
    ((dumb_iface*)iface)->moddata = 0;
    ((dumb_iface*)iface)->run = markevs2run_fun;
    ((dumb_iface*)iface)->err = markevs2err_fun;
  };
  void init() {
    itypes.insert(itypemap::value_type("touch", mark_musttouch));
    itypes.insert(itypemap::value_type("overlap", mark_mustolap));
    itypes.insert(itypemap::value_type("include", mark_mustbeincltouch));
    //itypes.insert(itypemap::value_type("", mark_mustbeincl));
  }
  
  // ------------------------------------------------------------------------------------------------------------------------
  
  const char* markevtype = "touch|overlap|include";
  int valid_markevsaux(const char* val) {return itypes.find(val) != itypes.end();}
  int valid_markev(const struct module_value val) {return module_valid_string(val, -1, -1, valid_markevsaux, markevtype);}
  
  int get_setting(int n, module_setting* set, int id) {
    switch (n) {  
    case 0:
      {
	set->name = "left"; // docscat{marks}
	set->type = module_string;
	set->descdoc = "Determines along with `right' how the time of a mark event matches against note events to determine which ones receive the marks."
	  "  Choices are `touch', `overlap' and `include'."
	  "  `touch' specifies that the end time of a note event be greater or equal to the mark event's time to match (the note need only touch the mark event to receive the marks)."
	  "  `overlap' specifies that the end time of a note event be greater than the mark event's time (the note must overlap with the mark event to receive the marks)."
	  "  `include' specifies that the time of a note event be greater or equal to to the mark event's time (the note must be completely within the mark event to receive the marks).";
	set->typedoc = markevtype;
	
	module_setval_string(&set->val, "overlap");
	
	set->loc = module_locnote;
	set->valid = valid_markev; // no range
	//set->validdeps = validdeps_maxp;
	set->uselevel = 2;
	leftid = id;
	break;
      }
    case 1:
      {
	set->name = "right"; // docscat{marks}
	set->type = module_string;
	set->descdoc = "Determines along with `left' how the end time of a mark event matches against note events to determine which ones receive the marks."
	  "  Choices are `touch', `overlap' and `include'."
	  "  `touch' specifies that the time of a note event be less or equal to the mark event's end time to match (the note need only touch the mark event to receive the marks)."
	  "  `overlap' specifies that the time of a note event be less than the mark event's end time (the note must overlap with the mark event to receive the marks)."
	  "  `include' specifies that the end time of a note event be less or equal to the mark event's end time (the note must be completely within the mark event to receive the marks).";
	set->typedoc = markevtype;
	
	module_setval_string(&set->val, "overlap");
	
	set->loc = module_locnote;
	set->valid = valid_markev; // no range
	//set->validdeps = validdeps_maxp;
	set->uselevel = 2;
	rightid = id;
	break;
      }
    default:
      return 0;
    }
    return 1;
  }

  void ready() {
    detachid = module_settingid("detach");
    if (detachid < 0) {ierr = "missing required setting `detach'"; return;}
  }  
  
}

