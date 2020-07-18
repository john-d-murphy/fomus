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

#include <cassert>
#include <list>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp> // next

#include "ifacesearch.h"
#include "module.h"

#ifndef NDEBUG
#define NONCOPYABLE , boost::noncopyable
#define _NONCOPYABLE :boost::noncopyable
#else
#define NONCOPYABLE
#define _NONCOPYABLE
#endif

namespace dynprog {

#define NODE_NULL ((void*) 0)
#define NODE_BEGIN ((void*) &module_init)
#define NODE_END ((void*) &module_free)

  // errors
  struct dynprogerr {
    const char* err;
    dynprogerr() : err(0) {}
    dynprogerr(const char* err) : err(err) {}
  };
  inline void checkerr(const search_iface& iface) {
    const char* err = iface.err(iface.moddata);
    if (err)
      throw dynprogerr(err);
  }
  inline search_node checkerr(const search_iface& iface, const search_node x) {
    if (x)
      return x;
    throw dynprogerr(iface.err(iface.moddata));
  }

  // scopednode
  struct scopednode _NONCOPYABLE {
    const search_iface& iface;
    search_node no;
    scopednode(const search_iface& iface) : iface(iface), no(0) {}
    scopednode(const search_iface& iface, const search_node n)
        : iface(iface), no(n) {}
    bool isbegin() {
      return no == NODE_BEGIN;
    }
    bool isend() {
      return no == NODE_END;
    }
    bool isnull() {
      return no == NODE_NULL;
    }
    bool isntbegin() {
      return no != NODE_BEGIN;
    }
    bool isntend() {
      return no != NODE_END;
    }
    bool isntnull() {
      return no != NODE_NULL;
    }
    search_node get() {
      return no;
    }
    ~scopednode() {
      if (isntnull() && isntbegin() && isntend())
        iface.free_node(iface.moddata, no);
    }
    void reset(const search_node n) {
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

  struct cntr {
    int val;
    std::list<int>::iterator it;
    cntr() : val(0) {}
  };
  struct nodecounter _NONCOPYABLE {
    std::vector<cntr>
        nbrs;           // size = nchoices, number of nodes open for each choice
    std::list<int> nop; // list of all choices that still have a branch--when
                        // there is only 1, that's the correct one
    nodecounter() {}
    nodecounter(const int nchoices) : nbrs(nchoices) {}
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
        return false; // size = 1
      iface.assign(iface.moddata, nop.front());
      return true;
    }
#ifndef NDEBUG
    ~nodecounter() {
      assert(nop.empty());
    }
#endif
  };

  struct node : public scopednode /*NONCOPYABLE*/ {
    search_score score;
    // fomus_int off, /*eoff*/ /*, feoff*/; // fendoff is farthest endoff so far
    fomus_int nnds; //, xnnds;
    boost::shared_ptr<node> prev;
    int choice;
    nodecounter& cnt;

    node(const search_iface& iface, nodecounter& cnt)
        : scopednode(iface, 0), score(iface.min_score), choice(-1), cnt(cnt) {
      assert(isntbegin()); // construct after begin, default choice = -1
    }
    node(const search_iface& iface, const search_node n, nodecounter& cnt)
        : scopednode(iface, n), nnds(0), choice(-1), cnt(cnt) {
      assert(isbegin()); // BEGIN node, gets minimum endoffset possible
    }
    void set(search_node n0, const search_score sc0,
             /*const fomus_int off0,*/ boost::shared_ptr<node>& prev0,
             const fomus_int choice0, /*const module_value& off1,*/
             const fomus_int nnds0 /*, const fomus_int xnnds0*/) {
      reset(n0);
      score = sc0; /*off = off0;*/
      prev = prev0;
      choice = choice0; /*eoff = off1;*/
      nnds = nnds0;     // xnnds = xnnds0;
    }
    search_node get() const {
      return no;
    }
    ~node() {
      if (choice >= 0)
        cnt.dec(choice);
    }
    void assignrest(const fomus_int pos /*, const fomus_int to*/) const {
      if (pos > 0)
        prev->assignrest(pos - 1 /*, to*/);
      iface.assign(iface.moddata, choice);
    }
    //#warning "*** see bfsearch.cc, might need to change this ***"
    bool pushlst(std::vector<search_node>& arr, const fomus_int n,
                 const search_node& last) const {
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
    void inccnt() {
      assert(choice >= -1 && choice < iface.nchoices);
      if (choice >= 0)
        cnt.inc(choice);
    } // delayed inc after construction
  };

  class data _NONCOPYABLE {
public:
    search_iface iface;

private:
    bool cerr;
    std::stringstream CERR;
    std::string errstr;
    // std::string modname; // TODO--get value
public:
    data() : cerr(false) {
      iface.moddata = 0;
      iface.api.begin = NODE_BEGIN;
      iface.api.end = NODE_END;
    }
    //~data() {if (iface.moddata) iface.free_moddata(iface.moddata);}
    const char* module_err() {
      if (!cerr)
        return 0;
      std::getline(CERR, errstr);
      return errstr.c_str();
    }
    search_iface* getiface() {
      return &iface;
    }
    void run() {
      try {
        if (iface.nchoices <= 0) {
          cerr = true;
          CERR << "no possible choices" << std::endl;
          return;
        }
        nodecounter bcnt;
        boost::ptr_vector<nodecounter> nop;
        boost::shared_ptr<node> beg(
            new node(iface, NODE_BEGIN, bcnt)); // begin "root" node
        std::vector<boost::shared_ptr<node>> nds(iface.nchoices,
                                                 beg); // previous nodes
        std::vector<boost::shared_ptr<node>> nws;      // new nodes
        std::vector<search_node> arr;
        fomus_int ass = 0;
        nodecounter* nwp; // nwp = new counter
        assert(iface.nchoices > 0);
        nop.push_back(nwp = new nodecounter(iface.nchoices));
        for (int i = 0; i < iface.nchoices; ++i)
          nws.push_back(boost::shared_ptr<node>(new node(iface, *nwp)));
        while (true) {
          int ch = 0;     // choice
          bool nf = true; // not found flag
          for (std::vector<boost::shared_ptr<node>>::iterator j(nws.begin());
               j != nws.end();
               ++j, ++ch) { // j points to new node ptr, ch is choice number
            node& sj = **j; // sj = new node
            for (std::vector<boost::shared_ptr<node>>::iterator i(nds.begin());
                 i != nds.end(); ++i) { // i points to old node ptr
              node& si = **i;           // si = old node
              if (si.isntnull()) {
                scopednode n(iface, iface.new_node(iface.moddata, si.get(),
                                                   ch)); // n = fresh new node
                if (n.isend()) {                         // done...
                  node* it;
#ifndef NDEBUG
                  it = 0;
#endif
                  search_score bst = iface.min_score;
                  for (std::vector<boost::shared_ptr<node>>::const_iterator i(
                           nds.begin());
                       i != nds.end(); ++i) { // i points to old node ptr
                    search_score sc = (*i)->score;
                    if (iface.score_lt(iface.moddata, bst, sc)) {
                      bst = sc;
                      it = i->get();
                    }
                  }
                  assert(it);
                  if ((fomus_int) nop.size() - 1 > ass)
                    it->assignrest((nop.size() - 2) - ass);
                  checkerr(iface);
                  return;
                }
                if (n.isntnull()) { // got one (maybe)
                  arr.clear(); // this is supposed to NOT reduce the capacity
                  si.pushlst(arr, si.nnds,
                             n.get()); // gathers list in order by offset
                  // if (arr.size() < si.nnds) si.nnds = arr.size(); that's bad
                  // assert(!arr.empty());
                  arr.push_back(n.get());
                  search_nodes arr0 = {arr.size(), &arr[0]};
                  search_score sc =
                      (si.isbegin()
                           ? iface.get_score(iface.moddata, arr0)
                           : iface.score_add(
                                 iface.moddata, si.score,
                                 iface.get_score(iface.moddata, arr0)));
                  if (iface.score_lt(iface.moddata, sj.score, sc)) {
                    // fomus_rat off(GET_R(iface.get_time(iface.moddata,
                    // n.get()))); bool x = (off > si.off);
                    sj.set(n.release(), sc, /*newo,*/ *i,
                           ch, /*iface.get_endtime(iface.moddata, n.get()),*/
                           /*x ? si.xnnds + 1 : si.nnds + 1,*/
                           /*x ? arr.size() : std::max((fomus_int)arr.size(),
                              si.xnnds + 1)*/
                           arr.size()); // set **j to new node
                    nf = false;
                  }
                }
              }
            }
            sj.inccnt(); // increment counter now (if count >= 0)
          }
          if (nf) { // should never happen--this is the module's fault
            cerr = true;
            CERR << "can't continue--no possible choices" << std::endl;
            return;
          }
          std::vector<boost::shared_ptr<node>>::iterator i(nds.begin());
          nwp = new nodecounter(iface.nchoices);
          for (std::vector<boost::shared_ptr<node>>::iterator j(nws.begin());
               j != nws.end(); ++j, ++i) {
            *i = *j;
            j->reset(new node(iface, *nwp));
          }
          while (ass < (fomus_int) nop.size() && nop[ass].assign(iface))
            ++ass;            // assignments for known solutions
          nop.push_back(nwp); // reset new node ptrs
          checkerr(iface);
        }
      } catch (const dynprogerr& e) {
        if (e.err) {
          CERR << e.err << std::endl;
        } else {
          // CERR << "error in module `" << modname << '\'' << std::endl;
          CERR << "unknown" << std::endl;
        }
        cerr = true;
      }
    }
  };
} // namespace dynprog

using namespace dynprog;

const char* module_longname() {
  return "Dynamic Programming Engine";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "A dynamic programming engine for FOMUS's formatting operations "
         "(faster, lower quality).  "
         "The difference between this engine and the best-first search module "
         "is that there is no backtracking--decisions are made in sequence "
         "from left to right.";
}
enum module_type module_type() {
  return module_modengine;
}
// int module_priority() {return 0;}
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
