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
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "ifacedivsearch.h"
#include "module.h"

#include "debugaux.h"

namespace divsearch {

  // algorithms
  template <typename I, typename T>
  inline bool hasall(I first, const I& last, T& pred) {
    while (first != last) {
      if (!pred(*first++))
        return false;
    }
    return true;
  }
  template <typename I1, typename I2, typename F>
  inline F for_each2(I1 first1, const I1& last1, I2 first2, F& fun) {
    while (first1 != last1)
      fun(*first1++, *first2++);
    return fun;
  }

  // error
  struct divsearcherr {
    const char* err;
    divsearcherr(const char* err) : err(err) {}
  };
  inline divsearch_node checkerr(const divsearch_iface& iface,
                                 const divsearch_node x) {
    if (x)
      return x;
    throw divsearcherr(iface.err(iface.moddata));
  }
  inline void checkerr(const divsearch_iface& iface) {
    const char* err = iface.err(iface.moddata);
    if (err)
      throw divsearcherr(err);
  };

  // andnodeex and ornodeex
  typedef std::vector<divsearch_node> andnodeexvect;
  typedef andnodeexvect::const_iterator andnodeexvect_constit;
  typedef andnodeexvect::iterator andnodeexvect_it;
  struct divsearch_andnodeex_nodel _NONCOPYABLE {
    const divsearch_iface& iface;
    andnodeexvect arr;
    divsearch_andnodeex_nodel(const divsearch_iface& iface)
        : iface(iface) {} // needs iface to free nodes
    void setarr(divsearch_andnode& str) {
      str.n = arr.size();
      str.nodes = &arr[0];
    }
    divsearch_andnode getarr() const {
      divsearch_andnode x = {arr.size(), (divsearch_node*) &arr[0]};
      return x;
    }
    void pushnew(const divsearch_node d) {
      arr.push_back(d);
    }
    bool empty() const {
      return arr.empty();
    }
    void clear() {
      arr.clear();
    }
  };
  struct divsearch_andnodeex
      : public divsearch_andnodeex_nodel /*NONCOPYABLE*/ {
    divsearch_andnodeex(const divsearch_iface& iface)
        : divsearch_andnodeex_nodel(iface) {}
    ~divsearch_andnodeex() {
      for (andnodeexvect_constit i(arr.begin()); i != arr.end(); ++i) {
        if (*i)
          iface.free_node(iface.moddata, *i);
      }
    }
  };

  typedef boost::ptr_vector<divsearch_andnodeex> ornodeexvect;
  typedef ornodeexvect::const_iterator ornodeexvect_constit;
  typedef ornodeexvect::iterator ornodeexvect_it;
  struct divsearch_ornodeex _NONCOPYABLE {
    const divsearch_iface& iface;
    std::vector<divsearch_andnode> arr; // array that module sees
    ornodeexvect aex;                   // extra info
    divsearch_ornodeex(const divsearch_iface& iface) : iface(iface) {}
    divsearch_ornode getarr();
    divsearch_andnodeex& getnewandnodeex() {
      divsearch_andnodeex* x;
      aex.push_back(x = new divsearch_andnodeex(iface));
      return *x;
    }
    bool empty() const {
      return aex.empty();
    }
  };

  inline divsearch_ornode divsearch_ornodeex::getarr() {
    divsearch_ornode x;
    x.n = aex.size();
    arr.resize(x.n);
    for_each2(aex.begin(), aex.end(), arr.begin(),
              bind(&divsearch_andnodeex::setarr, boost::lambda::_1,
                   boost::lambda::_2));
    x.ands = &arr[0];
    return x;
  }

  // andnodeholder & ornodeholder
  class andnodeholder;
  struct orsheap_less {
    bool operator()(const andnodeholder* x, const andnodeholder* y) const;
  };
  typedef std::vector<andnodeholder*> orsheap_vect;
  typedef std::priority_queue<andnodeholder*, orsheap_vect, orsheap_less>
      orsheap;

  class ornodeholder _NONCOPYABLE {
    const divsearch_iface& iface;
    bool sol;            // this and-combination is a valid solution
    divsearch_node data; // *** is data before it's expanded by module
    orsheap heap;
#ifndef NDEBUG
    int valid;
#endif
public:
    ornodeholder(const divsearch_iface& iface, const divsearch_node data)
        : iface(iface), sol(iface.is_leaf(iface.moddata, data)), data(data) //,
    /*heap(orsheap_less(), orsheap_vect())*/ {
#ifndef NDEBUG
      valid = 12345;
#endif
    } // heap not created until proc called (not needed yet...)
    ~ornodeholder() {
      if (data)
        iface.free_node(iface.moddata, data);
      clearheap();
    }
    void clearheap();
    divsearch_node proc(fomus_int& id, const fomus_int de);
    bool issol() const {
      assert(isvalid());
      return sol;
    }
    divsearch_node getdata() const {
      return data;
    }
#ifndef NDEBUG
    bool isvalid() const {
      return valid == 12345;
    }
#endif
  };

  typedef boost::ptr_vector<ornodeholder> andsvect;
  typedef andsvect::const_iterator andsvect_constit;
  typedef andsvect::iterator andsvect_it;

  class andnodeholder _NONCOPYABLE {
    friend bool orsheap_less::operator()(const andnodeholder* x,
                                         const andnodeholder* y) const;
    const divsearch_iface& iface;
    fomus_int id;
    andsvect ands;
    divsearch_node data; // assembled data
    divsearch_score score;
    fomus_int depth;
#ifndef NDEBUG
    int valid;
#endif
public:
    andnodeholder(const divsearch_iface& iface, fomus_int& id,
                  divsearch_andnodeex& node, const divsearch_node data)
        : iface(iface), id(id++), data(data),
          score(iface.score(iface.moddata, data)), depth(0) {
      for (andnodeexvect_constit i(node.arr.begin()); i != node.arr.end(); ++i)
        ands.push_back(new ornodeholder(iface, *i));
      node.clear(); // nodes have been transfered
#ifndef NDEBUG
      valid = 12345;
#endif
    }
    ~andnodeholder() {
      if (data)
        iface.free_node(iface.moddata, data);
    }
    bool operator<(const andnodeholder& y) const {
      if (iface.score_lt(iface.moddata, score, y.score))
        return true; // want to pull out the lowest number
      if (iface.score_lt(iface.moddata, y.score, score))
        return false;
      return id < y.id; // earlier ids first
    }
#ifndef NDEBUG
    bool andsempty() const {
      return ands.empty();
    }
#endif
    bool isallsol() const {
      return hasall(
          ands.begin(), ands.end(),
          boost::lambda::bind(&ornodeholder::issol, boost::lambda::_1));
    }
    fomus_int getdepth() const {
      return depth;
    }
    bool proc(fomus_int& id);
    divsearch_node getassem() const {
      return data;
    }
    divsearch_node releaseassem() {
      divsearch_node r = data;
      data = 0;
      return r;
    }
#ifndef NDEBUG
    bool isvalid() const {
      return valid == 12345;
    }
#endif
  };

  bool orsheap_less::operator()(const andnodeholder* x,
                                const andnodeholder* y) const {
    return *x < *y;
  }

  inline void ornodeholder::clearheap() {
    while (!heap.empty()) {
      delete heap.top();
      heap.pop();
    }
  }

  // scoped stuff
  struct scoped_data _NONCOPYABLE {
    const divsearch_iface& iface;
    divsearch_node data;
    scoped_data(const divsearch_iface& iface, const divsearch_node& data)
        : iface(iface), data(data) {}
    ~scoped_data() {
      if (data)
        iface.free_node(iface.moddata, data);
    }
  };

  // returns stored value (don't free it!) or 0
  divsearch_node ornodeholder::proc(fomus_int& id,
                                    const fomus_int de) { // gets called first
    assert(!sol);
    if (data) { // make initial ors heap
      assert(heap.empty());
      divsearch_ornodeex li(iface); // needs to be scoped--checkerr might throw
                                    // in the middle of transfering nodes
      iface.expand(iface.moddata, &li,
                   data); // li = or node (list of and-nodes)
      iface.free_node(iface.moddata, data);
      data = 0;
      for (ornodeexvect_it e(li.aex.begin()); e != li.aex.end();
           ++e) { // *e is an and-node
        if (!e->empty()) {
          heap.push(new andnodeholder(
              iface, id, *e,
              checkerr(
                  iface,
                  iface.assemble(
                      iface.moddata,
                      e->getarr())))); // *e is cleared, nodes are transfered
        }
      }
    }
    while (!heap.empty()) {
      andnodeholder* n(heap.top());
      assert(n->isvalid());
      if (n->isallsol()) { // se of and-nodes finished, so or-node is finished
                           // and this and-node is the best one (other nodes
                           // can't do better)
        assert(!n->andsempty());
        sol = true;               // set this or-node to = finished
        assert(!data);            // iface.free(iface.moddata, data);
        data = n->releaseassem(); // must save it--assembled data in and-node is
                                  // pre-calculated
        assert(data);
        clearheap(); // don't need anymore
        return data; // must put together an `and' node from list
      }
      if (de <= n->getdepth())
        return n->getassem(); // de not large enough yet to descend here
      heap.pop();
      std::auto_ptr<andnodeholder> h(n); // insure that n gets deleted
      if (n->proc(id))
        heap.push(h.release()); // else delete n;
    }
    return 0;
  }

  bool andnodeholder::proc(
      fomus_int& id) { // return true to reinsert into heap (or-choices)
    divsearch_andnodeex_nodel d(iface); // new one for assemble
    for (andsvect_it e(ands.begin()); e != ands.end(); ++e) {
      if (e->issol())
        d.pushnew(e->getdata());
      else {
        divsearch_node x =
            e->proc(id, depth); // not new data!--also, proc() can return 0
        if (x) {
          d.pushnew(x); // d.del.push_back(x);
        } else {        // x is 0
          return false; // destroy this ornode
        }
      }
    }
    ++depth; // current depth for this node
    assert(data);
    iface.free_node(iface.moddata, data);
    data =
        checkerr(iface, iface.assemble(iface.moddata,
                                       d.getarr())); // reassemble and re-score
    score = iface.score(iface.moddata, data);
    return true; // reinsert into heap
  }

  // api
  extern "C" {
  divsearch_andnode_ptr divsearch_newandnode(divsearch_ornode_ptr ornode);
  void divsearch_pushback(divsearch_andnode_ptr andnode, divsearch_node node);
  struct divsearch_ornode divsearch_get_ornode(divsearch_ornode_ptr ornode);
  struct divsearch_andnode divsearch_get_andnode(divsearch_andnode_ptr andnode);
  }
  divsearch_andnode_ptr divsearch_newandnode(divsearch_ornode_ptr ornode) {
    return &((divsearch_ornodeex*) ornode)->getnewandnodeex();
  }
  void divsearch_pushback(divsearch_andnode_ptr andnode, divsearch_node node) {
    ((divsearch_andnodeex*) andnode)->pushnew(node);
  }
  struct divsearch_ornode divsearch_get_ornode(divsearch_ornode_ptr ornode) {
    return ((divsearch_ornodeex*) ornode)->getarr();
  }
  struct divsearch_andnode
  divsearch_get_andnode(divsearch_andnode_ptr andnode) {
    return ((divsearch_andnodeex*) andnode)->getarr();
  }

  class data _NONCOPYABLE {
    divsearch_iface iface;
    bool cerr;
    std::stringstream CERR;
    std::string errstr;
    // std::string modname; // TODO: get this
public:
    data() : cerr(false) {
      iface.moddata = 0;
      iface.api.new_andnode = divsearch_newandnode;
      iface.api.push_back = divsearch_pushback;
      iface.api.get_ornode = divsearch_get_ornode;
      iface.api.get_andnode = divsearch_get_andnode;
    }
    //~data() {if (iface.moddata) iface.free_moddata(iface.moddata);}
    const char* module_err() {
      if (!cerr)
        return 0;
      std::getline(CERR, errstr);
      return errstr.c_str();
    }
    divsearch_iface* getiface() {
      return &iface;
    }
    void run() {
      try {
        while (true) {
          fomus_int id = 0;
          divsearch_node o = iface.get_root(iface.moddata);
          if (!o)
            break;
          ornodeholder top(iface, o);
          fomus_int de = 0;
          while (!top.issol()) {
            divsearch_node r = top.proc(id, de); // node shouldn't be deleted
            if (!r)
              throw divsearcherr("no solution");
            ++de;
          }
          iface.solution(iface.moddata, top.getdata());
        }
        checkerr(iface);
      } catch (const divsearcherr& e) {
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
} // namespace divsearch

using namespace divsearch;

const char* module_longname() {
  return "Iterative Depth-First/A* Search Engine";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "A hybrid iterative depth-first/A* search engine for quantizing, "
         "dividing measures and tying notes.  "
         "This engine was designed especially for use with FOMUS's measure "
         "dividing and time quantization modules.";
}
enum module_type module_type() {
  return module_modengine;
}
// int module_priority() {return 0;}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}
int module_get_setting(int n, module_setting* set, int id) {
  return 0;
}
void module_ready() {}
// callbacks
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
