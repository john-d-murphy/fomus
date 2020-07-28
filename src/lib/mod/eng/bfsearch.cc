// -*- c++ -*-

/*
    Copyright (C) 2009, 2010, 2011  David Psenicka
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

#include <cassert>
#include <list>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp> // next

#include "ifacesearch.h"
#include "module.h"

#include "debugaux.h"

namespace bfsearch {

#define NODE_NULL ((void*) 0)
#define NODE_BEGIN ((void*) &module_init)
#define NODE_END ((void*) &module_free)

  // errors
  struct bfsearcherr {
    const char* err;
    bfsearcherr() : err(0) {}
    bfsearcherr(const char* err) : err(err) {}
  };
  inline search_node checkerr(const search_iface& iface, const search_node x) {
    if (x)
      return x;
    throw bfsearcherr(iface.err(iface.moddata));
  }
  inline void checkerr(const search_iface& iface) {
    const char* err = iface.err(iface.moddata);
    if (err)
      throw bfsearcherr(err);
  }

  // scoednode
  struct scopednode _NONCOPYABLE {
    const search_iface& iface;
    search_node no;
    scopednode(const search_iface& iface) : iface(iface), no(0) {}
    scopednode(const search_iface& iface, const search_node n)
        : iface(iface), no(n) {}
    bool isbegin() const {
      return no == NODE_BEGIN;
    }
    bool isend() const {
      return no == NODE_END;
    }
    bool isnull() const {
      return no == NODE_NULL;
    }
    bool isntbegin() const {
      return no != NODE_BEGIN;
    }
    bool isntend() const {
      return no != NODE_END;
    }
    bool isntnull() const {
      return no != NODE_NULL;
    }
    search_node get() const {
      return no;
    }
    inline ~scopednode() {
      if (isntnull() && isntbegin() && isntend())
        iface.free_node(iface.moddata, no);
    }
    inline void reset(search_node n) {
      if (isntnull() && isntbegin() && isntend()) {
        iface.free_node(iface.moddata, no);
      }
      no = n;
    }
    search_node release() {
      search_node r = no;
      no = 0;
      return r;
    }
  };

  // node counter
  struct cntr {
    int val;
    std::list<int>::iterator it;
    cntr() : val(0) {}
  };
  struct nodecounter _NONCOPYABLE {
    fomus_int seqn;
    std::vector<cntr> nbrs; // size = nchoices
    std::list<int> nop;
    nodecounter() : seqn(-1) {} // base nodecounter gets seqn = -1
    nodecounter(const fomus_int seqn, const int nchoices)
        : seqn(seqn), nbrs(nchoices) {}
    void inc(const int choice) {
      if (nbrs[choice].val++ <= 0) {
        nop.push_front(choice);
        nbrs[choice].it = nop.begin();
      }
    }
    void dec(const int choice) {
      assert(nbrs[choice].val > 0);
      if (--nbrs[choice].val <= 0) {
        nop.erase(nbrs[choice].it);
      }
    }
    bool assign(const search_iface& iface) {
      assert(!nop.empty());
      if (nop.end() != boost::next(nop.begin()))
        return false; // ie. size = 1
      iface.assign(iface.moddata, nop.front());
      return true;
    }
  };

  // node
  struct node : public scopednode /*NONCOPYABLE*/ {
    fomus_int id;
    int choice;
    union search_score score;
    fomus_int nnds; // how far to go back collecting nodes, xnnds is maximum
    boost::shared_ptr<node> prev;
    nodecounter& cnt;
    bool dead;
    fomus_int brs;
    node(const search_iface& iface, const fomus_int id, search_node dat,
         const int choice, const union search_score score0,
         boost::shared_ptr<node>& prev, nodecounter& cnt, const fomus_int nnds)
        : scopednode(iface, dat), id(id), choice(choice), score(score0),
          nnds(nnds), prev(prev), cnt(cnt), dead(false), brs(1) {
      DBG("BFSEARCH GOT SCORE = " << score.f << std::endl);
      assert(isntbegin()); // after begin
      cnt.inc(choice);
      assert(prev->isbegin() || prev->brs > 0);
      if (!isbegin())
        ++(prev->brs);
    }
    node(const search_iface& iface, const search_node n, nodecounter& cnt)
        : scopednode(iface, n), nnds(0), cnt(cnt) {
      assert(isbegin()); // BEGIN node, gets minimum offset possible
    }
    ~node() {
      assert(isbegin() || brs >= 0);
    }
    void
    assignrest(const fomus_int pos) const { // pos is initially the number of
                                            // assignments left, and must be > 0
      assert(!isbegin());
      if (pos > 1)
        prev->assignrest(pos - 1);
      iface.assign(iface.moddata, choice);
    }
    bool pushlst(std::vector<search_node>& arr, const fomus_int n,
                 const search_node last) const {
      if ((n > 1 && prev->pushlst(arr, n - 1, last)) ||
          (no != NODE_BEGIN &&
           !iface.is_outofrange(
               iface.moddata, no,
               last))) { // true = adding now, include out-of-range ones as well
        arr.push_back(no);
        return true;
      } else
        return false;
    }
    bool opless(const node& y) const {
#ifndef NDEBUGOUT
      int s1, s2;
      s1 = iface.score_lt(iface.moddata, score, y.score);
      s2 = iface.score_lt(iface.moddata, y.score, score);
      DBG(" yes " << s1 << " no " << s2 << std::endl);
      if (s1)
        return true; // highest score on top
      if (s2)
        return false;
#else
      if (iface.score_lt(iface.moddata, score, y.score))
        return true; // highest score on top
      if (iface.score_lt(iface.moddata, y.score, score))
        return false;
#endif
      return id > y.id; // oldest on top
    }
    bool order(const node& y) const {
      if (cnt.seqn != y.cnt.seqn)
        return cnt.seqn > y.cnt.seqn;
      return y.opless(*this);
    }
    bool isdead() const {
      return dead;
    }
    void setdead() {
      assert(isbegin() || !dead);
      dead = true; /*if (!isbegin()) cnt.dec(choice);*/
      decbrs();
    }
    void decbrs() {
      if (!isbegin()) {
        --brs;
        assert(brs >= 0);
        if (brs <= 0) {
          cnt.dec(choice);
          prev->decbrs();
        }
      }
    }
  };

  // heap
  typedef boost::shared_ptr<node> heapshptr;
  struct heapfrontless {
    bool operator()(const heapshptr& x, const heapshptr& y) const {
      return x->opless(*y);
    }
  };
  struct heapbackless {
    bool operator()(const heapshptr& x, const heapshptr& y) const {
      return x->order(*y);
    }
  };
  // boost::pool_allocator<heapshptr> causes segfaults
  typedef std::priority_queue<heapshptr, std::vector<heapshptr>, heapfrontless>
      heapfront;
  typedef std::priority_queue<heapshptr, std::vector<heapshptr>, heapbackless>
      heapback;
  template <typename H>
  inline heapshptr popheap(H& h) { //
    while (true) {
      heapshptr nh(h.top());
      h.pop();
      if (!nh->isdead()) {
        return nh; // still in other one--must reset it after using
      }
    }
  }

  // data
  class data _NONCOPYABLE {
public:
    search_iface iface;

private:
    bool cerr;
    std::stringstream CERR;
    std::string errstr;

public:
    data() : cerr(false) {
      iface.moddata = 0;
      // #ifndef NDEBUG
      //       iface.nchoices = 0;
      //       iface.min_score.ptr = 0;
      //       iface.heapsize = 0;
      //       iface.assign = 0;
      //       iface.get_score = 0;
      //       iface.new_node = 0;
      //       iface.free_node = 0;
      //       iface.err = 0;
      //       iface.score_lt = 0;
      //       iface.score_add = 0;
      //       iface.is_outofrange = 0;
      // #endif
      iface.api.begin = NODE_BEGIN;
      iface.api.end = NODE_END;
    }
    const char* module_err() {
      if (!cerr)
        return 0;
      std::getline(CERR, errstr);
      return errstr.c_str();
    }
    void run() {
      assert(NODE_NULL != NODE_BEGIN);
      assert(NODE_NULL != NODE_END);
      assert(NODE_END != NODE_BEGIN);
      try {
        if (iface.nchoices <= 0) {
          cerr = true;
          CERR << "no possible choices" << std::endl;
          return;
        }
        nodecounter bcnt;
        boost::ptr_vector<nodecounter>
            nop; // number of unique nodes open for each position (when a
                 // counter is 1, can assign it)
        heapfront h;
        heapback rh; // heaps & nodes are destroyed before nodecounters
        fomus_int id = 0;
        fomus_int hs = 0; // doubleheap size
        std::vector<search_node> arr;
        fomus_int ass = 0; // next to assign
        heapshptr np(new node(iface, NODE_BEGIN, bcnt));
        while (true) {
          node& n = *np;                 // n = the popped node
          fomus_int in = n.cnt.seqn + 1; // in = new seq index
          if (in >= (fomus_int) nop.size())
            nop.push_back(new nodecounter(nop.size(), iface.nchoices));
          assert(in < (fomus_int) nop.size());
          for (int ch = 0; ch < iface.nchoices; ++ch) { // ch = choice#
            scopednode i(
                iface, iface.new_node(
                           iface.moddata, n.get(),
                           ch)); // i = new node from module--invalid choice = 0
            if (i.get()) {
              if (i.get() == NODE_END) {
                assert(
                    in ==
                    (fomus_int) nop.size() -
                        1); // since node at in is supposedly invalid (at the
                            // end), actual size = in (one less than nop.size())
                DBG("ASSIGN REMAINING = " << in - ass << std::endl);
                if (in > ass)
                  n.assignrest(in - ass); // go backwards to get optimal path
                checkerr(iface);
                return;
              }
              arr.clear();
              n.pushlst(arr, n.nnds,
                        i.get()); // gathers list in order by offset
              arr.push_back(i.get());
              search_nodes arr0 = {arr.size(), &arr[0]};
              // #ifndef NDEBUGOUT
              // 	      union search_score score =
              // iface.get_score(iface.moddata, arr0); 	      DBG("BFSEARCH GOT SCORE =
              // " << score.f << " and adding to " << n.score.f << std::endl);
              // 	      union search_score scoreadd =
              // iface.score_add(iface.moddata, n.score, score); 	      DBG("RESULT = "
              // << scoreadd.f << std::endl); 	      heapshptr o(new node(iface, id++,
              // i.release(), ch, 				   scoreadd, //off
              // /*iface.get_time(iface.moddata, i.get())*/, 				   np, nop[in],
              // /*iface.get_endtime(iface.moddata, i.get()),*/ 				   arr.size()));
              // #else
              heapshptr o(new node(
                  iface, id++, i.release(), ch,
                  (n.isbegin()
                       ? iface.get_score(iface.moddata, arr0)
                       : iface.score_add(
                             iface.moddata, n.score,
                             iface.get_score(
                                 iface.moddata,
                                 arr0))), // off /*iface.get_time(iface.moddata,
                                          // i.get())*/,
                  np, nop[in], /*iface.get_endtime(iface.moddata, i.get()),*/
                  arr.size()));
              // #endif
              h.push(o);
              rh.push(o);
              ++hs;
            }
          }
          np->setdead(); // popped node is fully removed--it's dead
          while (hs > iface.heapsize) {
            popheap(rh)->setdead();
            --hs;
          }
          while (ass < (fomus_int) nop.size() && nop[ass].assign(iface))
            ++ass;
          if (hs <= 0) {
            cerr = true;
            CERR << "can't continue--no possible choices"
                 << std::endl; // TODO: need module name
            return;
          }
          np = popheap(h);
          --hs;
          checkerr(iface);
        }
      } catch (const bfsearcherr& e) {
        if (e.err) {
          CERR << e.err << std::endl;
        } else {
          CERR << "unknown" << std::endl;
        }
        cerr = true;
      }
    }
    search_iface* getiface() {
      return &iface;
    }
  };
} // namespace bfsearch

using namespace bfsearch;

//#warning "*** check for fomus err also (err from other threads), exit if there
//is one ***"

const char* module_longname() {
  return "Best-First Search Engine";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "A best-first search engine for FOMUS's formatting operations "
         "(slower, higher quality).  "
         "The difference between this engine and the dynamic programming "
         "module is that backtracking occurs, causing the engine to consider "
         "more options.";
}
enum module_type module_type() {
  return module_modengine;
}
int module_get_setting(int n, module_setting* set, int id) {
  return 0;
}
void module_ready() {}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}
void* module_newdata(FOMUS f) {
  return new data;
}
void module_freedata(void* dat) {
  delete (data*) dat;
}
const char* module_err(void* dat) {
  return ((data*) dat)->module_err();
}
const char* module_initerr() {
  return 0;
}

int engine_ifaceid() {
  return ENGINE_INTERFACEID;
}
void* engine_get_iface(void* dat) {
  return ((data*) dat)->getiface();
}
void engine_run(void* dat) {
  ((data*) dat)->run();
}
