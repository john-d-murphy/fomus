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

#include <algorithm> // transform, copy
#include <cassert>
#include <cmath>
#include <cstring>
#include <deque>
#include <iterator> // back_inserter
#include <limits>
#include <map>
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

namespace octs {

  const char* ierr = 0;

  // setting ids
  int heapsizeid, properoctid, octchangeid, // diaintid, dianoteid, simqtid,
      exponid, octdistid, beatdistid, rangeid, distmodid, enginemodid, octupid,
      octdownid, upllsid, downllsid, isanoctid;

  struct octsnode _NONCOPYABLE {
    module_noteobj note;
    fomus_float properoctpenalty, octchangepenalty,
        isanoctpenalty; // diaintpenalty, dianotepenalty, simqtpenalty;
    fomus_float expon;
    int oct;
    int midpitch;
    octsnode(
        const module_noteobj note,
        /*const deque<octsnode*>::type::const_iterator& it,*/ const int oct)
        : note(note), /*it(it),*/ oct(oct),
          midpitch(todiatonic(module_clefmidpitch(module_clef(note)))) {
      properoctpenalty = module_setting_fval(note, properoctid);
      octchangepenalty = module_setting_fval(note, octchangeid);
      isanoctpenalty = module_setting_fval(note, isanoctid);
      expon = pow(2, -1 / module_setting_fval(note, exponid));
    }
  };

  struct scorenode {
    const octsnode* node; // pointer to keep boost::pool_allocator from bitching
    fomus_float dist;
    scorenode(const octsnode* node, const fomus_float dist)
        : node(node), dist(dist) {}
  };
  inline fomus_float properoct(const octsnode& n2) {
    fomus_int n = todiatonic(module_writtennote(n2.note)) - (n2.oct * 7);
    // if (n2.oct > 0) {
    int u = n2.midpitch + module_setting_ival(n2.note, upllsid) * 2 + 5;
    if (n > u)
      return ((n - u) / 2) * n2.properoctpenalty;
    //} else {
    int dw = n2.midpitch - (module_setting_ival(n2.note, downllsid) * 2 + 5);
    if (n < dw)
      return ((dw - n) / 2) * n2.properoctpenalty;
    //}
    return 0;
  }
  inline fomus_float isanoct(const octsnode& n2) {
    return n2.oct ? n2.isanoctpenalty : 0;
  }
  inline fomus_float
  octchange(const std::vector<scorenode>::const_iterator& n1,
            const octsnode& n11,
            const octsnode& n2) { // n2 used for calculating distance
    return (n1->node->oct != n11.oct) ? n2.octchangepenalty : 0;
  }

  struct octsdata _NONCOPYABLE {
    search_api api; // engine api
    module_noteobj ass, getn;
    dist_iface diface; // fill it up with data!
    octsdata() : ass(0), getn(0) {
      diface.moddata = 0;
      diface.data.octdist_setid = octdistid;
      diface.data.beatdist_setid = beatdistid;
      diface.data.byendtime = true;
      module_partobj p = module_peeknextpart(0);
      diface.data.rangemax = module_setting_fval(p, rangeid);
      module_get_auxiface(module_setting_sval(p, distmodid), DIST_INTERFACEID,
                          &diface);
    }
    ~octsdata() {
      if (diface.moddata)
        diface.free_moddata(diface.moddata);
    }
    bool isoutofrange(const search_node n1, const search_node n2) const {
      return diface.is_outofrange(diface.moddata, ((octsnode*) n1)->note,
                                  ((octsnode*) n2)->note);
    }
    // no rests, only notes
    search_score getscore(const search_nodes& nodes)
        const { // nodes ordered by offset, last one is the node being scored
      assert(
          nodes.n >
          0); // nodes are guaranteed to be in range, though needs to be pruned
      std::vector<scorenode> arr;
      octsnode** ie = (octsnode**) nodes.nodes + nodes.n - 1;
      const octsnode& n2 = **ie;
      for (octsnode** i = (octsnode**) nodes.nodes; i != ie; ++i) {
        fomus_float d = diface.dist(diface.moddata, (*i)->note, n2.note);
        if (d <= diface.data.rangemax)
          arr.push_back(scorenode(*i, pow(n2.expon, d)));
      }
      search_score sc;
      assert(properoct(n2) >= 0);
      assert(isanoct(n2) >= 0);
      sc.f =
          properoct(n2) +
          isanoct(n2); // vertmax(arr, n2) * arr.size(); // multiply by arr size
                       // so matches with accumulated calculations in loop
      fomus_rat o2(module_pitch(n2.note));
      if (!arr.empty()) {
        fomus_float mx = 0, ll = 0;
        for (std::vector<scorenode>::const_iterator i(arr.begin()),
             ie(std::prev(arr.end()));
             i != arr.end(); ++i) {
          assert(i->dist > 0);
          ll += i->dist;
          assert(octchange(i, (i == ie ? n2 : *std::next(i)->node), n2) >= 0);
          mx +=
              octchange(i, (i == ie ? n2 : *std::next(i)->node), n2) * i->dist;
        }
        sc.f += mx / ll;
      }
      DBG("OCTAVE SCORE: t=" << module_time(n2.note) << " ");
#ifndef NDEBUGOUT
      for (std::vector<scorenode>::const_iterator i(arr.begin());
           i != arr.end(); ++i) {
        DBG(module_pitch(i->node->note) << "," << i->node->oct << "  ");
      }
#endif
      DBG(module_pitch(n2.note) << "," << n2.oct << "  ");
      DBG("=  " << sc.f << std::endl);
      return sc;
    }
    search_node newnode(const search_node prevnode, const int choice) {
      assert(prevnode);
      module_noteobj n =
          (prevnode == api.begin)
              ? (getn ? module_peeknextnote(0) : (getn = module_nextnote()))
              : ((((octsnode*) prevnode)->note != getn)
                     ? module_peeknextnote(((octsnode*) prevnode)->note)
                     : (getn = module_nextnote()));
      if (!n)
        return api.end;
      DBG("NEWNODE??? t=" << module_time(n) << " oct=" << choice - 2
                          << std::endl);
      DBG("octup=" << module_setting_ival(n, octupid) << " octdown="
                   << module_setting_ival(n, octdownid) << std::endl);
      if (choice - 2 > module_setting_ival(n, octupid) ||
          choice - 2 < -module_setting_ival(n, octdownid))
        return 0;
      int uo = module_octsign(n);
      DBG("uo = " << uo << std::endl);
      if (uo != 0 && uo != choice - 2)
        return 0;
      DBG("NEWNODE t=" << module_time(n) << " oct=" << choice - 2 << std::endl);
      return new octsnode(n, /*boost::prior(vect.end()),*/ choice - 2);
    }
    void assignnext(const int choice) {
      DBG("ASSIGNING OCT " << module_time(module_peeknextnote(ass)) << " --> "
                           << choice - 2 << std::endl);
      //#warning "fixme--put this back and finish octaves module"
      octs_assign(ass = module_peeknextnote(ass), choice - 2);
      // octs_assign(ass = module_peeknextnote(ass), 0);
      assert(ass);
    };
  };

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
  // void search_free_moddata(void* moddata); // free up moddata structure and
  // choices vector when finished
  const char* search_err(void* moddata);
  int search_score_lt(void* moddata, union search_score x,
                      union search_score y); // less is better
  union search_score search_score_add(void* moddata, union search_score x,
                                      union search_score y);
  int search_is_outofrange(void* moddata, search_node node1, search_node node2);
  }

  void search_assign(void* moddata, int choice) {
    ((octsdata*) moddata)->assignnext(choice);
  } // makes a solution assignment & reports it so that other phases of the
    // program can continue
  union search_score search_get_score(void* moddata,
                                      struct search_nodes nodes) {
    return ((octsdata*) moddata)->getscore(nodes);
  }
  search_node search_new_node(void* moddata, search_node prevnode, int choice) {
    return ((octsdata*) moddata)->newnode(prevnode, choice);
  } // return NULL if no valid next node or END if at end of search--`prevnode'
    // might be BEGIN (get the first node)
  void search_free_node(void* moddata, search_node node) {
    delete (octsnode*) node;
  }
  // void search_free_moddata(void* moddata) {delete (octsdata*)moddata;} //
  // free up moddata structure and choices vector when finished
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
    return ((octsdata*) moddata)->isoutofrange(node1, node2);
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
  const char* octsdistmulttype = "real>0";
  int valid_octsdistmult(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int) 0), module_excl,
                            module_makeval((fomus_int) 0), module_nobound, 0,
                            octsdistmulttype);
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

  std::set<std::string> modstrset;
  std::string modstrtype;
  int valid_modstr_aux(const char* str) {
    return modstrset.find(str) != modstrset.end();
  }
  int valid_modstr(const struct module_value val) {
    return module_valid_string(val, 1, -1, valid_modstr_aux,
                               modstrtype.c_str());
  }

  const char* octsupdowntype = "integer0..2";
  int valid_octsupdown(const struct module_value val) {
    return module_valid_int(val, 0, module_incl, 2, module_incl, 0,
                            octsupdowntype);
  }

} // namespace octs

using namespace octs;

int module_engine_iface() { // what engine does the module want?
  return ENGINE_INTERFACEID;
}
void module_fill_iface(void* moddata, void* iface) { // module
  ((octsdata*) moddata)->api = ((search_iface*) iface)->api;
  ((search_iface*) iface)->moddata = moddata;
  ((search_iface*) iface)->assign = search_assign;
  ((search_iface*) iface)->get_score = search_get_score;
  ((search_iface*) iface)->new_node = search_new_node;
  ((search_iface*) iface)->free_node = search_free_node;
  //((search_iface*)iface)->free_moddata = search_free_moddata;
  ((search_iface*) iface)->err = search_err;
  ((search_iface*) iface)->score_lt = search_score_lt;
  ((search_iface*) iface)->score_add = search_score_add;
  ((search_iface*) iface)->is_outofrange = search_is_outofrange;
  ((search_iface*) iface)->nchoices = 5;
  union search_score ms;
  ms.f = std::numeric_limits<fomus_float>::max();
  ((search_iface*) iface)->min_score = ms;
  ((search_iface*) iface)->heapsize =
      module_setting_ival(module_peeknextpart(0), heapsizeid);
}

const char* module_longname() {
  return "Octave Signs";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Places octave change signs in the score at appropriate points.";
}
void* module_newdata(FOMUS f) {
  return new octsdata;
}
void module_freedata(void* dat) {
  delete (octsdata*) dat;
}
const char* module_err(void* dat) {
  return 0;
}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modocts;
}
int module_itertype() {
  return module_bypart | module_bystaff | module_norests /*| module_firsttied*/;
}
const char* module_initerr() {
  return ierr;
}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "octs-searchdepth"; // docscat{octs}
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
    // THIS GOES IN NOTE QUANTIZER
  case 1: {
    set->name = "octs-distmult"; // docscat{octs}
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
    set->typedoc = octsdistmulttype; // uses the same string/valid fun

    module_setval_int(&set->val, 3); // true

    set->loc = module_locnote;
    set->valid = valid_octsdistmult;
    set->uselevel = 3;
    exponid = id;
  } break;
  case 2: {
    set->name = "octs-engine"; // docscat{octs}
    set->type = module_string;
    set->descdoc = "Engines provide different types of search functionality to "
                   "the rest of FOMUS's modules and are interchangeable."
                   "  For example, two of FOMUS's default engines `dynprog' "
                   "and `bfsearch' execute two different search algorithms, "
                   "each with different benefits."
                   "  Set this to the name of an engine module to change the "
                   "search algorithm used for finding octave changes.";
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
  case 3: {
    set->name = "octs-instaff-score"; // docscat{octs}
    set->type = module_number;
    set->descdoc = "Score when a note is inside the staff (or within a certain "
                   "number of ledger lines)."
                   "  Decreasing this value increases the likelyhood of "
                   "inserting an octave sign.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 5); // TODO: reasonable default

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    properoctid = id;
  } break;
  case 4: {
    set->name = "octs-change-score"; // docscat{octs}
    set->type = module_number;
    set->descdoc = "Score that discourages octave signs changes.  "
                   "Increasing this value decreases the chance of switching "
                   "often between octave change signs.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1); // TODO: reasonable default

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    octchangeid = id;
  } break;
  case 5: {
    set->name = "octs-octavedist"; // docscat{octs}
    set->type = module_number;
    set->descdoc = "If the `cartdist' distance or similar module is selected, "
                   "this is the distance of one octave"
                   " (i.e., two notes an octave apart have a distance "
                   "equivalent to this value)."
                   "  A larger distance translates to a smaller weight in "
                   "decisions regarding octave change signs and vice versa.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1); // TODO: reasonable default

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    octdistid = id;
  } break;
  case 6: {
    set->name = "octs-beatdist"; // docscat{octs}
    set->type = module_number;
    set->descdoc = "If the `cartdist' distance or similar module is selected, "
                   "this is the distance of one beat in a measure."
                   "  A larger distance translates to a smaller weight in "
                   "decisions regarding octave change signs and vice versa.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1); // TODO: reasonable default

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    beatdistid = id;
  } break;
  case 7: {
    set->name = "octs-maxdist"; // docscat{octs}
    set->type = module_number;
    set->descdoc = "The maximum distance allowed for octave sign calculations."
                   "  Notes that are more than this distance apart from each "
                   "other are considered unrelated to each other and are not "
                   "included in FOMUS's calculations.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1); // TODO: reasonable default

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    rangeid = id;
  } break;
  case 8: {
    set->name = "octs-distmod"; // docscat{octs}
    set->type = module_string;
    set->descdoc =
        "Module that is used to calculate the \"distance\" between notes."
        "  The closer two notes are to each other, the more important their "
        "relationship is in terms of note spelling, staff choice, voice "
        "assignments, etc.."
        //"  Since distance can be calculate in many different ways, there are
        //several interchangeable modules for this."
        "  Set this to change the algorithm used for calculating distance when "
        "making decisions regarding the use of octave signs.";

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
  case 9: {
    set->name = "octs-up"; // docscat{octs}
    set->type = module_number;
    set->descdoc =
        "Value indicating which octave signs are allowed above a staff.  "
        "Set this in a staff definition to indicate which octave changes are "
        "allowed for an instrument.  "
        "0 means no octave changes allowed, 1 means a one-octave (8va) change "
        "sign is allowed and 2 means a two-octave (15va) change sign is "
        "allowed.";
    set->typedoc = octsupdowntype; // uses the same string/valid fun

    module_setval_int(&set->val, 2); // true

    set->loc = module_locnote;
    set->valid = valid_octsupdown;
    set->uselevel = 2;
    octupid = id;
  } break;
  case 10: {
    set->name = "octs-down"; // docscat{octs}
    set->type = module_number;
    set->descdoc =
        "Value indicating which octave signs are allowed below a staff.  "
        "Set this in a staff definition to indicate which octave changes are "
        "allowed for an instrument.  "
        "0 means no octave changes allowed, 1 means a one-octave (8vb) change "
        "sign is allowed and 2 means a two-octave (15vb) change sign is "
        "allowed.";
    set->typedoc = octsupdowntype; // uses the same string/valid fun

    module_setval_int(&set->val, 1); // true

    set->loc = module_locnote;
    set->valid = valid_octsupdown;
    set->uselevel = 2;
    octdownid = id;
  } break;
  case 11: {
    set->name = "octs-sign-score"; // docscat{octs}
    set->type = module_number;
    set->descdoc = "Score discouraging the use of octave signs.  "
                   "Increasing this descreases the change of octave change "
                   "signs occurring.";
    set->typedoc = scoretype;

    module_setval_float(&set->val, 1); // TODO: reasonable default

    set->loc = module_locnote;
    set->valid = valid_score;
    set->uselevel = 3;
    isanoctid = id;
  } break;
  default:
    return 0;
  }
  return 1;
}

const char* module_engine(void* f) {
  return module_setting_sval(module_peeknextpart(0), enginemodid);
}
void module_ready() {
  upllsid = module_settingid("ledgers-up");
  if (upllsid < 0) {
    ierr = "missing required setting `ledgers-up'";
    return;
  }
  downllsid = module_settingid("ledgers-down");
  if (downllsid < 0) {
    ierr = "missing required setting `ledgers-down'";
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
