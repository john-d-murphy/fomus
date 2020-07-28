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

#include <cstring>
//#include <queue>
// #include <list>
#include <algorithm>
#include <cassert>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/utility.hpp> // next & prior

#include "ifacesearch.h"
#include "infoapi.h"
#include "module.h"

#include "debugaux.h"

namespace merge {

  template <typename I, typename T>
  inline bool hasall(I first, const I& last, T& pred) {
    while (first != last)
      if (!pred(*first++))
        return false;
    return true;
  }
  template <typename R, typename I, typename F>
  inline R accumulate_results(I first, const I& last, F& fun) {
    R s((R) 0);
    while (first != last)
      s += fun(*first++);
    return s;
  }

  int minnotesid, chordscid, distscid, heapsizeid, enginemodid;

  struct noteobjbase {
    module_noteobj n;
    int voice;
    bool isrest;
    module_markslist marks;
    fomus_rat off, dur;
    fomus_float chordsc, distsc0;
    noteobjbase(const module_noteobj n)
        : n(n), voice(module_voice(n)), isrest(module_isrest(n)),
          marks(module_marks(n)), off(module_time(n)), dur(module_dur(n)),
          chordsc(module_setting_fval(n, chordscid)),
          distsc0(module_setting_fval(n, distscid)) {}
    bool isntvoice_or_isrest(const int v) const {
      return voice != v || isrest;
    }
    bool isvoice(const int v) const {
      return voice == v;
    }
  };
  struct noteobj : public noteobjbase {
    fomus_float val, distsc;
    module_noteobj mergeto;
    noteobj(const noteobjbase& x)
        : noteobjbase(x), val(1), distsc(0), mergeto(0) {}
    fomus_float getsc() const {
      return val + distsc;
    }
    //#warning "put this back!"
    void assign() const {
      DBG("merge: it's " << n << ' ');
      if (mergeto) {
        DBG("MERGE" << std::endl);
        merge_assign_merge(n, mergeto);
      } else {
        DBG("NO MERGE" << std::endl);
        module_skipassign(n);
      }
    }
    // void assign() const {module_skipassign(n);}
  };

  // called from vmergeable(), from newnode()---if mergeable, also sets `val',
  // which is the base score
  bool mergeable(noteobj& n1, noteobj& n2,
                 int& wh) { // for comparing notes in sequence
    if (!n1.isrest) {       // note, rest
      if (n2.isrest) {
        if (wh == 2)
          return false; // conflict = no deal
        wh = 1; // note merges into rest--setting wh means it *must* go in that
                // direction
      }
    }
    if (!n2.isrest) {
      if (n1.isrest) {
        if (wh == 1)
          return false;
        wh = 2;
      }
    }
    if (n1.off != n2.off || n1.dur != n2.dur || n1.marks.n != n2.marks.n)
      return false; // same off and dur
    for (const module_markobj *i1(n1.marks.marks),
         *ie1(n1.marks.marks + n1.marks.n), *i2(n2.marks.marks);
         i1 < ie1; ++i1, ++i2) {
      if (module_markid(*i1) != module_markid(*i2))
        return false;
      const char* x = module_markstring(*i1);
      const char* y = module_markstring(*i2);
      if (std::string(x ? x : "") != std::string(y ? y : ""))
        return false;
      module_value x0(module_marknum(*i1));
      module_value y0(module_marknum(*i2));
      if ((x0.type == module_none) ? (y0.type != module_none)
                                   : (y0.type == module_none || x0 != y0))
        return false;
    }
    if (n1.isrest)
      n1.val = 0; // rests have score base 0
    if (n2.isrest)
      n2.val = 0;
    if (!n1.isrest && !n2.isrest) {
      n1.val = n1.chordsc; // merging two notes
      n2.val = n2.chordsc;
      n1.distsc += n1.distsc0;
    }
    return true;
  }

  typedef boost::ptr_vector<noteobj> noteobjvect;

  // called from newnode() function
  bool
  vmergeable(noteobjvect& vect, const int v1,
             const int v2) { // vect = list of notes, v1 = (upper), v2 = (lower)
    boost::ptr_vector<noteobj>::iterator i1(vect.begin()), i2(vect.begin());
    bool isany = false;
    int wh = 0; // 1 = v1 ---> v2, 2 = v2 <--- v1, 0 = either way is a-okay
    while (true) {
      while (i1 != vect.end() && !i1->isvoice(v1))
        ++i1;
      while (i2 != vect.end() && !i2->isvoice(v2))
        ++i2;
      if (i1 == vect.end() && i2 == vect.end())
        return isany; // if voice doesn't exist, then it isn't mergeable
      if (i1 == vect.end() || i2 == vect.end())
        return false; // voices should be same length
      isany = true;
      if (!mergeable(*i1++, *i2++, wh))
        return false;
    }
    assert(wh != 0);
    i1 = i2 = vect.begin();
    while (true) { // iterate in parallel through voices again and set which
                   // ones merge to which
      while (true) {
        if (i1 == vect.end())
          return true;
        if (i1->isvoice(v1))
          break;
        ++i1;
      }
      while (!i2->isvoice(v2))
        ++i2;
      if (wh == 1)
        i1->mergeto = i2->n;
      else
        i2->mergeto = i1->n;
      ++i1;
      ++i2;
    }
  }

  typedef std::vector<std::pair<int, int>> pairvect;

  struct node;
  typedef boost::ptr_map<int, node> nodemap;

  // node contains merges, # of notes (this is being minimized!, chords count as
  // 1?), # of notes across (to keep track of dist--includes measure boundaries)
  struct node _NONCOPYABLE {
    std::auto_ptr<noteobjvect> vect; // for assigning if picked
    const pairvect& switchs; // which voices merge to where... for assigning
    fomus_float sc;
    fomus_int dist;
    boost::ptr_list<nodemap>::iterator it;
#ifndef NDEBUG
    int valid;
#endif
    node(std::auto_ptr<noteobjvect>& vect0, const pairvect& switchs,
         const bool lastinm, const boost::ptr_list<nodemap>::iterator& it)
        : vect(vect0), switchs(switchs), // vector ot noteobjs have been
                                         // prepared & scored already
          sc(accumulate_results<fomus_float>(
              vect->begin(), vect->end(),
              boost::lambda::bind(&noteobj::getsc, boost::lambda::_1))),
          dist(0), it(it) {
#ifndef NDEBUG
      valid = 12345;
#endif
      assert(!vect->empty());
      std::map<int, fomus_int> cnt;
      for (boost::ptr_vector<noteobj>::const_iterator i(vect->begin());
           i != vect->end(); ++i) {
        fomus_int d = ++cnt[i->voice];
        if (d > dist)
          dist = d;
      }
      if (lastinm)
        ++dist; // add one for measure boundary
    }
#ifndef NDEBUG
    bool isvalid() const {
      return valid == 12345;
    }
#endif
    fomus_float getscore() const {
      return sc;
    }
    void assign() const {
      assert(isvalid());
      std::for_each(vect->begin(), vect->end(),
                    boost::lambda::bind(&noteobj::assign, boost::lambda::_1));
    }
    fomus_int mindist() const {
      assert(isvalid());
      assert(!vect->empty());
      return module_setting_ival(vect->front().n, minnotesid);
    }
  };
  inline bool nodesamemerge(const node& x, const node& y) {
    assert(x.isvalid());
    assert(y.isvalid());
    for (pairvect::const_iterator i(x.switchs.begin()), j(y.switchs.begin());;
         ++i, ++j) {
      if (i == x.switchs.end())
        return j == y.switchs.end();
      if (j == y.switchs.end() || *i != *j)
        return false;
    }
  }

  // get all combinations--only merge upwards
  void collectmerge(const int* to, const int* fr, const int* end,
                    pairvect& vect, boost::ptr_vector<pairvect>& merges) {
    if (fr >= end) {
      if (!vect.empty())
        merges.push_back(new pairvect(vect));
    } else {
      vect.push_back(std::pair<int, int>(*to, *fr)); // to, from
      collectmerge(to, fr + 1, end, vect, merges);
      collectmerge(fr + 1, fr + 2, end, vect, merges);
      vect.pop_back();
      collectmerge(fr + 1, fr + 2, end, vect, merges);
    }
  }

  struct divpt {
    fomus_rat pt;
    bool end;
    divpt() {}
    divpt(const fomus_rat& pt) : pt(pt), end(false) {}
    void set(const fomus_rat& pt0) {
      pt = pt0;
      end = false;
    }
    divpt(const fomus_rat& pt, int) : pt(pt), end(true) {}
    void set(const fomus_rat& pt0, int x) {
      pt = pt0;
      end = x;
    }
    void set(const divpt& x) {
      pt = x.pt;
      end = x.end;
    }
  };

  struct data {
    search_api api; // engine api--filled by callback
    boost::ptr_list<nodemap> nodes;
    boost::ptr_list<nodemap>::const_iterator ass;
    boost::ptr_vector<pairvect>
        merges; // list of merges (from which voice to which voice)
    std::deque<divpt> divpts;
    module_noteobj nd;
    module_measobj mm;

    data() : nd(0) {
#ifndef NDEBUG
      mm = 0;
#endif
    }
    union search_score getscore(struct search_nodes& nodes) {
      union search_score ret;
      assert(nodes.n > 0);
      const node& l = **(node**) (nodes.nodes + nodes.n - 1);
      if (nodes.n > 1) {
        fomus_int d = 0;
        fomus_int mindist = (*(node**) (nodes.nodes + nodes.n - 2))
                                ->mindist(); // it's a setting
        for (node** i = (node**) (nodes.nodes + nodes.n - 2);
             i >= (node**) nodes.nodes; --i) {
          if (nodesamemerge(**i, l))
            d += (*i)->dist;
          else
            break;
          if (d >= mindist) {
            ret.f = l.sc;
            return ret;
          }
        }
      }
      ret.f = 1000 + l.sc; // horrible score
      return ret;
    }
    // an init function
    int getlistofmerges(const module_partobj p) {
      module_intslist vl(module_voices(p)); // gets them all
      if (vl.n >= 2) {
        pairvect vect;
        collectmerge(vl.ints, vl.ints + 1, vl.ints + vl.n, vect, merges);
      }
      merges.push_back(
          new pairvect(1, std::pair<int, int>(0, 0))); // "no merge"
      return merges.size();
    }
    void assignnext(const int choice) {
      DBG((ass == nodes.begin() ? "ASSIGNING FIRST!" : "ASSIGNING!")
          << std::endl);
      assert(ass != nodes.end());
      nodemap::const_iterator i(ass->find(choice));
      assert(i != ass->end()); // should be there!
      i->second->assign();
      ++ass;
    }
    void* newnode(node* prev, const int choice) { // might be api.begin
      if (!nodes.empty()) {                       // called more than once!
        boost::ptr_list<nodemap>::iterator pl(
            prev == api.begin ? nodes.begin() : boost::next(prev->it));
        if (pl != nodes.end()) { // nodes is a list
          nodemap::iterator i(
              pl->find(choice)); // in the map--choice should be there
          return (i == pl->end()) ? 0 : i->second;
        }
      }
      nodemap* nv = new nodemap;
      if (nodes.empty()) {
        nodes.push_back(nv);
        ass = nodes.begin();
        assert(ass != nodes.end());
      } else {
        nodes.push_back(nv); // nodes is a ptr_list
        if (ass == nodes.end())
          ass = boost::prior(nodes.end());
      }
      boost::ptr_vector<noteobjbase> nobs;
      divpt nxdv;
      do {
        if (divpts.empty()) {
          mm = module_nextmeas();
          if (!mm)
            return api.end;
          module_ratslist d(module_divs(mm));
          assert(d.n > 0); // <---no divs!!!!!!!!
          fomus_rat o = module_time(mm);
          const fomus_rat* r = d.rats;
          for (const fomus_rat* re = d.rats + d.n - 1; r < re; ++r) {
            o = o + *r;
            DBG("merge: pushing back offset " << o << std::endl);
            divpts.push_back(divpt(o));
          }
          DBG("merge: pushing back offset " << (o + *r) << std::endl);
          divpts.push_back(divpt(o + *r, 1));
        }
        assert(mm);
        nxdv.set(divpts.front());
        assert(!divpts.empty());
        divpts.pop_front();
        bool isl = !module_peeknextmeas(mm);
        if (!nd)
          goto STARTHERE;
        while (isl || module_time(nd) < nxdv.pt) {
          nobs.push_back(new noteobjbase(nd));
          DBG("pushed back t=" << module_time(nd) << " onto nobs ending at "
                               << nxdv.pt << std::endl);
        STARTHERE:
          nd = module_nextnote();
          DBG("merge: nextnote = " << nd << std::endl);
          if (!nd) {
            if (nobs.empty())
              return api.end;
            break;
          }
        }
      } while (nobs.empty());
      assert(nobs.size() !=
             0); // node is a group of notes in all voices that belong to a
                 // measure division (ea. node has a mapping from/to spec)
      int n = 0;
      assert(!nodes.empty());
      boost::ptr_list<nodemap>::iterator ee(boost::prior(nodes.end()));
      for (boost::ptr_vector<pairvect>::const_iterator i(merges.begin());
           i != merges.end(); ++i) { // complete list of merges
        std::auto_ptr<noteobjvect> x(new noteobjvect);
        for (boost::ptr_vector<noteobjbase>::const_iterator j(nobs.begin());
             j != nobs.end(); ++j)
          x->push_back(new noteobj(*j));
        if (i->front().first > 0) { // no-merge--always allow this
          for (pairvect::const_iterator pp(i->begin()); pp != i->end();
               ++pp) { // go through sequence of merges, make sure every one is
                       // mergeable
            if (!vmergeable(*x.get(), pp->first, pp->second))
              goto SKIPINS;
          }
        }
        nv->insert(n, new node(x, *i, nxdv.end, ee));
      SKIPINS:
        ++n;
      }
      nodemap::iterator i(nv->find(choice));
      return i == nv->end() ? 0 : i->second;
    }
    bool isoutofrange(const node* node1, const node* node2) {
      fomus_int mindist = node2->mindist();
      fomus_int d = 0;
      for (boost::ptr_list<nodemap>::const_iterator i(node1->it), ie(node2->it);
           i != ie; ++i) { // yes, I want <node2, not <=node2
        assert(!i->empty());
        d += i->begin()->second->dist;
        if (d >= mindist)
          return true;
      }
      return false;
    }
  };

  const char* mergeminnotestype = "integer>=0";
  int valid_mergeminnotes(const struct module_value val) {
    return module_valid_int(val, 0, module_incl, 0, module_nobound, 0,
                            mergeminnotestype);
  }
  const char* mergechordsscoretype = "real>=0";
  int valid_mergechordsscore(const struct module_value val) {
    //     module_value z;
    //     z.type = module_int;
    //     z.val.i = 0;
    return module_valid_num(val, module_makeval((fomus_int) 0), module_incl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            mergechordsscoretype);
  }
  const char* mergedistscoretype = "real>=0";
  int valid_mergedistscore(const struct module_value val) {
    //     module_value z;
    //     z.type = module_int;
    //     z.val.i = 0;
    return module_valid_num(val, module_makeval((fomus_int) 0), module_incl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            mergedistscoretype);
  }
  const char* heapsizetype = "integer>=10";
  int valid_heapsize(const struct module_value val) {
    return module_valid_int(val, 10, module_incl, 0, module_nobound, 0,
                            heapsizetype);
  }

  extern "C" {
  void
  search_assign(void* moddata,
                int choice); // makes a solution assignment & reports it so that
                             // other phases of the program can continue
  union search_score search_get_score(void* moddata, struct search_nodes nodes);
  search_node search_new_node(
      void* moddata, search_node prevnode,
      int choice); // return NULL if no valid next node or END if at end of
                   // search--`prevnode' might be BEGIN (get the first node)
  void search_free_node(void* moddata, search_node node);
  const char* search_err(void* moddata);
  int search_score_lt(void* moddata, union search_score x,
                      union search_score y); // less is better
  union search_score search_score_add(void* moddata, union search_score x,
                                      union search_score y);
  int search_is_outofrange(void* moddata, search_node node1, search_node node2);
  }

  void search_assign(void* moddata, int choice) {
    ((data*) moddata)->assignnext(choice);
  } // makes a solution assignment & reports it so that other phases of the
    // program can continue
  union search_score search_get_score(void* moddata,
                                      struct search_nodes nodes) {
    return ((data*) moddata)->getscore(nodes);
  }
  search_node search_new_node(void* moddata, search_node prevnode, int choice) {
    return ((data*) moddata)->newnode((node*) prevnode, choice);
  } // return NULL if no valid next node or END if at end of search--`prevnode'
    // might be BEGIN (get the first node)
  void search_free_node(void* moddata, search_node nod) { /*intentional NOP*/
  }
  const char* search_err(void* moddata) {
    return 0;
  }
  int search_score_lt(void* moddata, union search_score x,
                      union search_score y) {
    return x.f > y.f;
  }
  union search_score search_score_add(void* moddata, union search_score x,
                                      union search_score y) {
    search_score r;
    r.f = x.f + y.f;
    return r;
  }
  int search_is_outofrange(void* moddata, search_node node1,
                           search_node node2) {
    return ((data*) moddata)->isoutofrange((node*) node1, (node*) node2);
  }

  std::string enginemodstype;
  std::set<std::string> enginemodsset;
  int valid_enginemod_aux(const char* str) {
    return enginemodsset.find(str) != enginemodsset.end();
  }
  int valid_enginemod(const struct module_value val) {
    return module_valid_string(val, -1, -1, valid_enginemod_aux,
                               enginemodstype.c_str());
  }

} // namespace merge

using namespace merge;

int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
void module_fill_iface(void* moddata, void* iface) { // module
  ((data*) moddata)->api = ((search_iface*) iface)->api;
  ((search_iface*) iface)->moddata = moddata;
  ((search_iface*) iface)->assign = search_assign;
  ((search_iface*) iface)->get_score = search_get_score;
  ((search_iface*) iface)->new_node = search_new_node;
  ((search_iface*) iface)->free_node = search_free_node;
  ((search_iface*) iface)->err = search_err;
  ((search_iface*) iface)->score_lt = search_score_lt;
  ((search_iface*) iface)->score_add = search_score_add;
  ((search_iface*) iface)->is_outofrange = search_is_outofrange;
  module_partobj p = module_peeknextpart(0);
  ((search_iface*) iface)->nchoices = ((data*) moddata)->getlistofmerges(p);
  union search_score ms;
  ms.f = std::numeric_limits<fomus_float>::max();
  ((search_iface*) iface)->min_score = ms;
  ((search_iface*) iface)->heapsize = module_setting_ival(p, heapsizeid);
}

const char* module_longname() {
  return "Merge Voices";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Merges separate voices with identical rhythms.  Also adds text "
         "markings such as \"a2,\" \"I\" and \"II\" where appropriate.";
}
void* module_newdata(FOMUS f) {
  return new data;
}
void module_freedata(void* dat) {
  delete (data*) dat;
}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modmerge;
}
int module_itertype() {
  return module_bypart;
}

const char* module_initerr() {
  return 0;
}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "merge-minnotes"; // docscat{voices}
    set->type = module_number;
    set->descdoc = "The minimum number of note events required between changes "
                   "in how voices are \"merged\" "
                   "(i.e., separate voices are notated as chords or as "
                   "monophonic lines marked \"a2,\" etc.).  "
                   "Set this to a higher value to cause merges to occur over "
                   "larger spans of notes and vice versa.";
    set->typedoc = mergeminnotestype; // uses the same string/valid fun

    module_setval_int(&set->val, 5);

    set->loc = module_locnote;
    set->valid = valid_mergeminnotes;
    set->uselevel = 2;
    minnotesid = id;
  } break;
  case 1: {
    set->name = "merge-chordscore"; // docscat{voices}
    set->type = module_number;
    set->descdoc =
        "Score encouraging separate voices to be merged together into chords.  "
        "Increasing this increases the chance of separate voices merging "
        "together into chords.";
    set->typedoc = mergechordsscoretype; // uses the same string/valid fun

    module_setval_float(&set->val, 0.5);

    set->loc = module_locnote;
    set->valid = valid_mergechordsscore;
    set->uselevel = 3;
    chordscid = id;
  } break;
  case 2: {
    set->name = "merge-distscore"; // docscat{voices}
    set->type = module_number;
    set->descdoc =
        "Score encouraging merged notes to be close together pitchwise.  The "
        "higher this value is, the greater the penalty for merging "
        "voices with large intervals that might appear awkward in the score.";
    set->typedoc = mergedistscoretype; // uses the same string/valid fun

    module_setval_int(&set->val, 1);

    set->loc = module_locnote;
    set->valid = valid_mergedistscore;
    set->uselevel = 3;
    distscid = id;
  } break;
  case 3: {
    set->name = "merge-searchdepth"; // docscat{voices}
    set->type = module_int;
    set->descdoc =
        "The search depth used when an appropriate engine module (such as "
        "`bfsearch') is selected."
        "  A larger search depth increases both computation time and quality."
        // "  FOMUS looks ahead and considers this many events before making a
        // decision."
        ;
    set->typedoc = heapsizetype;

    module_setval_int(&set->val, 25);

    set->loc = module_locpart;
    set->valid = valid_heapsize;
    set->uselevel = 3;
    heapsizeid = id;
  } break;
  case 4: {
    set->name = "merge-engine"; // docscat{voices}
    set->type = module_string;
    set->descdoc = "Engines provide different types of search functionality to "
                   "the rest of FOMUS's modules and are interchangeable."
                   "  For example, two of FOMUS's default engines `dynprog' "
                   "and `bfsearch' execute two different search algorithms, "
                   "each with different benefits."
                   "  Set this to the name of an engine module to change the "
                   "search algorithm used for merging voices.";
    struct info_modfilter fi0 = {0, 0, 0, module_modengine, ENGINE_INTERFACEID};
    struct info_modfilterlist fi = {1, &fi0};
    const struct info_modlist ml(info_list_modules(&fi, 0, 0, -1));
    std::ostringstream s;
    for (const info_module *i = ml.mods, *ie = ml.mods + ml.n; i != ie; ++i) {
      enginemodsset.insert(i->name);
      if (i != ml.mods)
        s << '|';
      s << i->name;
    }
    enginemodstype = s.str();
    set->typedoc = enginemodstype.c_str();

    module_setval_string(&set->val, "bfsearch"); // true

    set->loc = module_locpart;
    set->valid = valid_enginemod;
    set->uselevel = 3;
    enginemodid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}

const char* module_engine(void* f) {
  return module_setting_sval(module_peeknextpart(0), enginemodid);
}
void module_ready() {}
inline bool icmp(const module_obj a, const module_obj b, const int id) {
  return module_setting_ival(a, id) == module_setting_ival(a, id);
}
inline bool vcmp(const module_obj a, const module_obj b, const int id) {
  return module_setting_val(a, id) == module_setting_val(a, id);
}
inline bool scmp(const module_obj a, const module_obj b, const int id) {
  return strcmp(module_setting_sval(a, id), module_setting_sval(a, id)) == 0;
}
int module_sameinst(module_obj a, module_obj b) {
  return scmp(a, b, enginemodid) && icmp(a, b, heapsizeid);
}
