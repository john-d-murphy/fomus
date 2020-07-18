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

#include <algorithm> // transform, copy
#include <cassert>
#include <cmath>
#include <cstring>
#include <deque>
#include <iterator> // back_inserter
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/utility.hpp> // next & prior

#include "ifacedist.h"
#include "ifacesearch.h"
#include "infoapi.h"
#include "module.h"
#include "modutil.h"

#include "debugaux.h"

namespace accs {

  const char* ierr = 0;

// nchoices = 15
#define NACC 5
#define NQT 3
  const int choicetoacc[] = {0, -1, 1, -2, 2}; // mod 3
  const fomus_rat choicetoqtacc[] = {{0, 1}, {-1, 2}, {1, 2}};
  const int minaccdiff[] = {0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0};
  const int maxaccdiff[] = {0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1};

  // setting ids
  int heapsizeid, qtsid, daccsid, stepwiseid, diaintid, dianoteid, simqtid,
      exponid, octdistid, beatdistid, rangeid, distmodid, enginemodid, accid,
      wrongdirid; //, inkeyid;

  struct accsnode _NONCOPYABLE {
    module_noteobj note;
    bool daccs, qts; // is this note qt-enabled?
    fomus_float stepwisepenalty, diaintpenalty, dianotepenalty, simqtpenalty,
        /*inkeypenalty,*/ wrongdirpenalty;
    fomus_float expon;
    int acc;
    fomus_rat qtacc;
    // struct module_keysigref keyaccs;
    // bool oacc;
    accsnode(
        const module_noteobj note,
        /*const deque<accsnode*>::type::const_iterator& it,*/ const int acc,
        const fomus_rat& qtacc, /*const dist_iface& diface,*/
        const bool daccs, const bool qts)
        : note(note), /*it(it),*/ daccs(daccs), qts(qts), acc(acc),
          qtacc(qtacc) /*,*/ /*, diface(diface)*/
    /*keyaccs(module_keysigacc(note))*/ {
      stepwisepenalty = module_setting_fval(note, stepwiseid);
      diaintpenalty = module_setting_fval(note, diaintid);
      dianotepenalty = module_setting_fval(note, dianoteid);
      simqtpenalty = module_setting_fval(note, simqtid);
      wrongdirpenalty = module_setting_fval(note, wrongdirid);
      expon = pow(2, -1 / module_setting_fval(note, exponid));
      fomus_rat a((fomus_int) acc + qtacc);
      //       module_measobj m(module_meas(note));
      //       assert((module_note(note) - a).den == 1);
      //       int nn = (module_note(note) - a).num;
      //       assert(iswhite(nn));
      //       keyaccs = module_measkeysigacc(m, nn);
      //       if (a == (fomus_int)0) oacc = false;
      //       else {
      // 	if (a < (fomus_int)0) {
      // 	  struct module_keysigref xx(module_measkeysigacc(m,
      // tochromatic(todiatonic(nn) - 1))); 	  oacc = (xx.acc1 + xx.acc2 >
      // (fomus_int)0); 	} else { 	  struct module_keysigref
      // xx(module_measkeysigacc(m, tochromatic(todiatonic(nn) + 1))); 	  oacc =
      // (xx.acc1 + xx.acc2 < (fomus_int)0);
      // 	}
      //       }
    }
  };

  struct scorenode {
    const accsnode* node; // pointer to keep boost::pool_allocator from bitching
    fomus_float dist;
    scorenode(const accsnode* node, const fomus_float dist)
        : node(node), dist(dist) {}
  };

  inline fomus_float stepwise(const accsnode& n1, const accsnode& n2,
                              const fomus_rat& o1,
                              const fomus_rat& o2) { // o1 and o2 = notes
    fomus_rat a1((fomus_int) n1.acc + n1.qtacc); // quartertone also considered
    fomus_rat a2((fomus_int) n2.acc + n2.qtacc); // quartertone also considered
    return ((o1 - a1 == o2 - a2) &&
            ((o2 > o1 && (a2 < (fomus_int) 0 || a1 < (fomus_int) 0)) ||
             (o2 < o1 && (a2 > (fomus_int) 0 || a1 > (fomus_int) 0))))
               ? n2.stepwisepenalty
               : 0;
  }
  inline fomus_float dianote(const accsnode& n2,
                             const fomus_rat& o2) { // o2 is note
    // int a = n2.acc;
    return ((n2.acc != 0 && iswhite(o2 - n2.qtacc)) || n2.acc < -1 ||
            n2.acc > 1)
               ? n2.dianotepenalty
               : 0; // acc = semitone-acc
  }
  //   inline fomus_float inkey(const accsnode& n2, const fomus_rat& o2) {
  //     return // ((n2.keyaccs.acc1 > (fomus_int)0  && n2.acc < (fomus_int)0)
  //       // 	    || (n2.keyaccs.acc1 < (fomus_int)0 && n2.acc > (fomus_int)0)
  //       // 	    || (n2.keyaccs.acc2 > (fomus_int)0  && n2.qtacc <
  //       (fomus_int)0)
  //       // 	    || (n2.keyaccs.acc2 < (fomus_int)0 && n2.qtacc >
  //       (fomus_int)0)) n2.oacc ? n2.inkeypenalty : 0;
  //   }
  // increase in sharpness/flatness must match the allowed values for that IC
  inline fomus_float diaint(const accsnode& n1, const accsnode& n2,
                            const fomus_rat& o1, const fomus_rat& o2) {
    assert(diff(o1 - /*n1.acc -*/ n1.qtacc, o2 - /*n2.acc -*/ n2.qtacc).den ==
           1);
    int it(diff(o1 - /*n1.acc -*/ n1.qtacc, o2 - /*n2.acc -*/ n2.qtacc).num %
           12); // the base written interval mod 12
    int id(o1 > o2
               ? n1.acc - n2.acc
               : n2.acc -
                     n1.acc); // the accidental difference (not including qt)
    return (id < minaccdiff[it] || id > maxaccdiff[it]) ? n2.diaintpenalty : 0;
  }
  inline fomus_float simqt(const accsnode& n1, const accsnode& n2) {
    fomus_rat q1(n1.qtacc);
    fomus_rat q2(n2.qtacc);
    return ((n1.acc < (fomus_int) 0 && q2 > (fomus_int) 0) ||
            (n1.acc > (fomus_int) 0 && q2 < (fomus_int) 0) ||
            (q1 < (fomus_int) 0 && q2 > (fomus_int) 0) ||
            (q1 > (fomus_int) 0 && q2 < (fomus_int) 0) ||
            (q1 < (fomus_int) 0 && n2.acc > (fomus_int) 0) ||
            (q1 > (fomus_int) 0 && n2.acc < (fomus_int) 0))
               ? n2.simqtpenalty
               : 0;
  }
  inline fomus_float wrongdir(const accsnode& n1, const accsnode& n2,
                              const fomus_rat& o1, const fomus_rat& o2) {
    fomus_rat a1(o1 - ((fomus_int) n1.acc + n1.qtacc)),
        a2(o2 - ((fomus_int) n2.acc + n2.qtacc));
    return ((o1 > o2 && a1 < a2) || (o1 < o2 && a1 > a2)) ? n2.wrongdirpenalty
                                                          : 0;
  }

  struct accsdata _NONCOPYABLE {
    search_api api; // engine api
    module_noteobj ass, getn;
    dist_iface diface;
    accsdata() : ass(0), getn(0) {
      diface.moddata = 0;
      diface.data.octdist_setid = octdistid;
      diface.data.beatdist_setid = beatdistid;
      diface.data.byendtime = true;
      module_partobj p = module_peeknextpart(0);
      diface.data.rangemax = module_setting_fval(p, rangeid);
      module_get_auxiface(module_setting_sval(p, distmodid), DIST_INTERFACEID,
                          &diface);
    }
    ~accsdata() {
      if (diface.moddata)
        diface.free_moddata(diface.moddata);
    }
    bool isoutofrange(const search_node n1, const search_node n2) const {
      return diface.is_outofrange(diface.moddata, ((accsnode*) n1)->note,
                                  ((accsnode*) n2)->note);
    }
    // no rests, only notes
    union search_score getscore(const search_nodes& nodes)
        const { // nodes ordered by offset, last one is the node being scored
      assert(
          nodes.n >
          0); // nodes are guaranteed to be in range, though needs to be pruned
      std::vector<scorenode> arr;
      accsnode** ie = (accsnode**) nodes.nodes + nodes.n - 1;
      const accsnode& n2 = **ie; // the last one (current one) in the list
      for (accsnode** i = (accsnode**) nodes.nodes; i != ie; ++i) {
        fomus_float d = diface.dist(diface.moddata, (*i)->note, n2.note);
        if (d <= diface.data.rangemax)
          arr.push_back(scorenode(*i, pow(n2.expon, d)));
      }
      union search_score sc;
      fomus_rat o2(module_pitch(n2.note));
      sc.f = dianote(
          n2, o2) /*+ inkey(n2, o2)*/; // vertmax(arr, n2) * arr.size(); //
                                       // multiply by arr size so matches with
                                       // accumulated calculations in loop
      if (!arr.empty()) {
        fomus_float mx = 0, ll = 0;
        for (std::vector<scorenode>::const_iterator i(arr.begin());
             i != arr.end(); ++i) {
          fomus_rat o1(module_pitch(i->node->note));
          DBG("  dist " << module_time(i->node->note) << " to "
                        << module_time(n2.note) << " = " << i->dist
                        << std::endl);
          assert(i->dist > 0);
          ll += i->dist;
          mx += (stepwise(*i->node, n2, o1, o2) + diaint(*i->node, n2, o1, o2) +
                 simqt(*i->node, n2) + wrongdir(*i->node, n2, o1, o2)) *
                i->dist;
        }
        sc.f += mx / ll;
      }
      DBG("ACC SCORE: ");
#ifndef NDEBUGOUT
      for (std::vector<scorenode>::const_iterator i(arr.begin());
           i != arr.end(); ++i) {
        DBG(module_pitch(i->node->note) << ',' << i->node->acc << ",  ");
      }
#endif
      DBG(module_pitch(n2.note) << ',' << n2.acc << ",  ");
      DBG("=  " << sc.f << std::endl);
      return sc;
    }
    search_node newnode(const search_node prevnode, const int choice) {
      assert(prevnode);
      module_noteobj n =
          (prevnode == api.begin)
              ? (getn ? module_peeknextnote(0) : (getn = module_nextnote()))
              : ((((accsnode*) prevnode)->note != getn)
                     ? module_peeknextnote(((accsnode*) prevnode)->note)
                     : (getn = module_nextnote()));
      if (!n)
        return api.end;
      bool daccs = module_setting_ival(n, daccsid); // double accs allowed?
      if (!daccs && choice >= (3 * NQT))
        return 0;
      bool qts = module_setting_ival(n, qtsid);
      fomus_int theacc1 = choicetoacc[choice / NQT];
      fomus_rat theacc2 = choicetoqtacc[choice % NQT];
      if ((!qts && (choice % NQT) > 0) ||
          (!iswhite(module_pitch(n) - (fomus_int) theacc1 - theacc2)))
        return 0;
      fomus_rat a1(module_acc1(n));
      if (a1.num != std::numeric_limits<fomus_int>::max()) {
        if (a1 != theacc1)
          return 0;
        fomus_rat a2(module_acc2(n));
        if (a2.num != std::numeric_limits<fomus_int>::max() && a2 != theacc2)
          return 0;
      }
      module_value x(module_setting_val(n, accid)); // what the user wants
      assert(x.type == module_list);
      if (x.val.l.n > 0) {
        bool gotval = false;
        for (const module_value *i = x.val.l.vals,
                                *ie = x.val.l.vals + x.val.l.n;
             i < ie; ++i) {
          module_noteparts parts;
          parts.acc1 = parts.acc2 = module_makerat(0, 1);
          fomus_rat ret(module_strtoacc(i->val.s, &parts));
          if (parts.acc1 == theacc1 && parts.acc2 == theacc2)
            goto ITSOK;
          if (ret.num != std::numeric_limits<fomus_int>::max() &&
              (qts || (choice % NQT) <= 0) &&
              (iswhite(module_pitch(n) - parts.acc1 - parts.acc2)))
            gotval = true;
        }
        if (gotval)
          return 0; // there's a valid one in there, don't make choice for
                    // user--if there isn't a valid one, go ahead and make
                    // choice for user
      }
    ITSOK:
      DBG("  considering acc: " << module_pitch(n) << ' ' << theacc1 << ' '
                                << theacc2 << std::endl);
      return new accsnode(n, /*boost::prior(vect.end()),*/ theacc1, theacc2,
                          /*getdiface(n),*/ daccs, qts);
    }
    void assignnext(const int choice) {
      DBG("ASSIGNING ACC = " << module_pitch(module_peeknextnote(ass))
                             << " --> " << choicetoacc[choice / NQT] << ' '
                             << choicetoqtacc[choice % NQT] << std::endl);
      accs_assign(ass = module_peeknextnote(ass),
                  module_makerat(choicetoacc[choice / NQT], 1),
                  choicetoqtacc[choice % NQT]);
      assert(ass);
    };
  };

  extern "C" {
  void
  search_assign(void* moddata,
                int choice); // makes a solution assignment & reports it so
                             // that other phases of the program can continue
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
    ((accsdata*) moddata)->assignnext(choice);
  } // makes a solution assignment & reports it so that other phases of the
    // program can continue
  union search_score search_get_score(void* moddata,
                                      struct search_nodes nodes) {
    return ((accsdata*) moddata)->getscore(nodes);
  }
  search_node search_new_node(void* moddata, search_node prevnode, int choice) {
    return ((accsdata*) moddata)->newnode(prevnode, choice);
  } // return NULL if no valid next node or END if at end of search--`prevnode'
    // might be BEGIN (get the first node)
  void search_free_node(void* moddata, search_node node) {
    delete (accsnode*) node;
  }
  const char* search_err(void* moddata) {
    return 0;
  }
  int search_score_lt(void* moddata, union search_score x,
                      union search_score y) {
    DBG("  comparing " << x.f << ">" << y.f << std::endl);
    return x.f > y.f;
  }
  union search_score search_score_add(void* moddata, union search_score x,
                                      union search_score y) {
    union search_score r;
    r.f = x.f + y.f;
    return r;
  }
  int search_is_outofrange(void* moddata, search_node node1,
                           search_node node2) {
    return ((accsdata*) moddata)->isoutofrange(node1, node2);
  } // node2 has the proper diface

  // ------------------------------------------------------------------------------------------------------------------------

  const char* heapsizetype = "integer>=10";
  int valid_heapsize(const struct module_value val) {
    return module_valid_int(val, 10, module_incl, 0, module_nobound, 0,
                            heapsizetype);
  }
  const char* scoretype = "real>=0";
  int valid_score(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) 0), module_incl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            scoretype);
  }
  const char* accsdistmulttype = "real>0";
  int valid_accsdistmult(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) 0), module_excl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            accsdistmulttype);
  }

  std::set<std::string> modstrset;
  std::string modstrtype;
  int valid_modstr_aux(const char* str) {
    return modstrset.find(str) != modstrset.end();
  }
  int valid_modstr(const struct module_value val) {
    return module_valid_string(val, 1, -1, valid_modstr_aux,
                               modstrtype.c_str());
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

} // namespace accs

using namespace accs;

const char* module_engine(void* f) {
  return module_setting_sval(module_peeknextpart(0), enginemodid);
}
int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
void module_fill_iface(void* moddata, void* iface) { // module
  ((accsdata*) moddata)->api = ((search_iface*) iface)->api;
  ((search_iface*) iface)->moddata = moddata;
  ((search_iface*) iface)->assign = search_assign;
  ((search_iface*) iface)->get_score = search_get_score;
  ((search_iface*) iface)->new_node = search_new_node;
  ((search_iface*) iface)->free_node = search_free_node;
  ((search_iface*) iface)->err = search_err;
  ((search_iface*) iface)->score_lt = search_score_lt;
  ((search_iface*) iface)->score_add = search_score_add;
  ((search_iface*) iface)->is_outofrange = search_is_outofrange;
  ((search_iface*) iface)->nchoices = (NACC * NQT);
  union search_score ms;
  ms.f = std::numeric_limits<fomus_float>::max();
  ((search_iface*) iface)->min_score = ms;
  ((search_iface*) iface)->heapsize =
      module_setting_ival(module_peeknextpart(0), heapsizeid);
}
const char* module_err(void* data) {
  return 0;
}
const char* module_longname() {
  return "Accidentals";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Determines accidental spellings.";
}
void* module_newdata(FOMUS f) {
  return new accsdata;
}
void module_freedata(void* dat) {
  delete (accsdata*) dat;
}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modaccs;
}
int module_itertype() {
  return module_bypart | module_byvoice | module_firsttied | module_norests;
}
const char* module_initerr() {
  return ierr;
}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}

int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "accs-searchdepth"; // docscat{accs}
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
  case 1: {
    set->name = "accs-stepwise-score"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "Score when successive stepwise notes contain sharps in "
                   "upwards motion or flats in downwards motion.  "
                   "Increasing this value increases the change of stepwise "
                   "ascending sharps and descending flats.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 13);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    stepwiseid = id;
  } break;
  case 2: {
    set->name = "accs-diatonic-interval-score"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "Score when intervals between notes are diatonic (these are "
                   "presumably easier to read).  "
                   "Increasing this score increases the chance of diatonic "
                   "intervals occurring.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 3);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    diaintid = id;
  } break;
  case 3: {
    set->name = "accs-diatonic-note-score"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "Score when notes are typical diatonic spellings (i.e., no "
                   "F-flats, B-sharps, C-doublesharps, etc.).  "
                   "Increasing this value increases the chance of diatonic "
                   "spellings occurring.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 8);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    dianoteid = id;
  } break;
  case 4: {
    set->name = "accs-similar-quartone-score"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "Score when quartertone accidentals on nearby notes are "
                   "similar to each other (i.e., modifying the written pitch "
                   "in the same direction).  "
                   "Increasing this value increases the chances of similar "
                   "quartertone adjustments on adjacent notes.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 5); // TODO: reasonable default

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    simqtid = id;
  } break;
  case 5: {
    set->name = "accs-distmult"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "FOMUS calculates the distance between note events so that "
                   "it can weigh scores and penalties according to how close "
                   "they are to each other."
                   "  An exponential curve is usually applied before actually "
                   "calculating the weight."
                   "  This value is the distance at which a score calculated "
                   "between two notes is decreased to 1/2 it's full value"
                   " (at twice this distance the score is decreased to 1/4 "
                   "it's full value, etc.).";
    set->typedoc = accsdistmulttype; // uses the same string/valid fun

    // module_setval_rat(&set->val, module_makerat(3, 2)); // true
    // module_setval_int(&set->val, 1);
    module_setval_rat(&set->val, module_makerat(2, 3)); // true

    set->loc = module_locnote;
    set->valid = valid_accsdistmult;
    set->uselevel = 3;
    exponid = id;
  } break;
  case 6: {
    set->name = "accs-engine"; // docscat{accs}
    set->type = module_string;
    set->descdoc = "Engines provide different types of search functionality to "
                   "the rest of FOMUS's modules and are interchangeable."
                   "  For example, two of FOMUS's default engines `dynprog' "
                   "and `bfsearch' execute two different search algorithms, "
                   "each with different benefits."
                   "  Set this to the name of an engine module to change the "
                   "search algorithm used for finding accidental spellings.";

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
  case 7: {
    set->name = "accs-distmod"; // docscat{accs}
    set->type = module_string;
    set->descdoc =
        "Module that is used to calculate the \"distance\" between notes."
        "  The closer two notes are to each other, the more important their "
        "relationship is in terms of note spelling, staff choice, voice "
        "assignments, etc.."
        //"  Since distance can be calculate in many different ways, there are
        // several interchangeable modules for this."
        "  Set this to change the algorithm used for calculating distance when "
        "making decisions regarding accidentals.";

    struct info_modfilter fi0 = {0, 0, 0, module_modaux, DIST_INTERFACEID};
    struct info_modfilterlist fi = {1, &fi0};
    const struct info_modlist ml(info_list_modules(&fi, 0, 0, -1));
    std::ostringstream s;
    for (const info_module *i = ml.mods, *ie = ml.mods + ml.n; i != ie; ++i) {
      modstrset.insert(i->name);
      if (i != ml.mods)
        s << '|';
      s << i->name;
    }
    modstrtype = s.str();
    set->typedoc = modstrtype.c_str();

    module_setval_string(&set->val, "notedist");

    set->loc = module_locnote; // !!! --create dist and select on the fly?
    set->valid = valid_modstr;
    set->uselevel = 3;
    distmodid = id;
  } break;
  case 8: {
    set->name = "double-accs"; // docscat{accs}
    set->type = module_bool;
    set->descdoc = "Whether or not double accidentals (i.e., double flats or "
                   "sharps) are allowed.  "
                   "Set this to `yes' if you want double flats or sharps.";

    module_setval_int(&set->val, 1);

    set->loc = module_locnote;
    set->uselevel = 2;
    daccsid = id;
  } break;
  case 9: {
    set->name = "accs-octavedist"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "If the `cartdist' distance or similar module is selected, "
                   "this is the distance of one octave"
                   " (i.e., two notes an octave apart have a distance "
                   "equivalent to this value)."
                   "  A larger distance translates to a smaller weight in "
                   "decisions regarding accidentals and vice versa.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    octdistid = id;
  } break;
  case 10: {
    set->name = "accs-beatdist"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "If the `cartdist' distance or similar module is selected, "
                   "this is the distance of one beat in a measure."
                   "  A larger distance translates to a smaller weight in "
                   "decisions regarding accidentals and vice versa.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    beatdistid = id;
  } break;
  case 11: {
    set->name = "accs-maxdist"; // docscat{accs}
    set->type = module_number;
    set->descdoc =
        "The maximum distance allowed for accidental spelling calculations."
        "  Notes that are more than this distance apart from each other are "
        "considered unrelated to each other and are not included in FOMUS's "
        "calculations.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 3);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    rangeid = id;
  } break;
  case 12: {
    set->name = "accs-wrongdir-score"; // docscat{accs}
    set->type = module_number;
    set->descdoc = "Score for using higher written notes to spell higher "
                   "pitches and vice versa.  "
                   "Setting this to a high value insures that notes higher up "
                   "on the staff are actually higher pitches.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 13);

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    wrongdirid = id;
  } break;
  default:
    return 0;
  }
  return 1;
}
void module_ready() {
  qtsid = module_settingid("quartertones");
  if (qtsid < 0) {
    ierr = "missing required setting `quartertones'";
    return;
  }
  accid = module_settingid("acc");
  if (accid < 0) {
    ierr = "missing required setting `acc'";
    return;
  }
}
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
  return scmp(a, b, enginemodid) && icmp(a, b, heapsizeid) &&
         scmp(a, b, distmodid) && vcmp(a, b, octdistid) &&
         vcmp(a, b, beatdistid) && vcmp(a, b, rangeid);
}
