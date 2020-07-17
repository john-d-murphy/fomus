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

#include <cstring>
#include <vector>
#include <map>
#include <algorithm> // transform, copy
#include <limits>
#include <cassert>
#include <iterator> // back_inserter
#include <string>
#include <cmath>
#include <sstream>
#include <set>

#include <boost/utility.hpp> // next & prior

// #include "module.h"
#include "infoapi.h"
#include "module.h"
#include "ifacesearch.h"
#include "ifacedist.h"

#include "debugaux.h"

namespace voices {

  // setting ids
  int vertmaxid, heapsizeid,
    vertmaxscid, previsgraceid, voiceolapid, voicecrossid, /*balanceid,*/ smoothid, adjsmoothid, exponid,
    octdistid, beatdistid, rangeid, distmodid, enginemodid, connectid, dissimid;
  
  struct voicenode _NONCOPYABLE {
    module_noteobj note;
    int voice;
    fomus_float vertmaxpenalty, previsgracescore, voiceolappenalty, voicecrosspenalty, /*balancepenalty,*/ smoothpenalty, connectpenalty, dissimpenalty, adjsmoothpenalty;
    fomus_float expon;
#ifndef NDEBUG
    int valid;
#endif
    int vmax;
    fomus_rat ti, teti, no;
    bool isgr;
    voicenode(const module_noteobj note, const int voice):
      note(note), voice(voice), 
#ifndef NDEBUG      
      valid(12345), 
#endif
      ti(module_time(note)), teti(module_tiedendtime(note)), no(module_pitch(note)), isgr(module_isgrace(note))
      {initstuff();}
    void initstuff() {
      vertmaxpenalty = module_setting_fval(note, vertmaxscid);
      previsgracescore = module_setting_fval(note, previsgraceid);
      voiceolappenalty = module_setting_fval(note, voiceolapid);
      voicecrosspenalty = module_setting_fval(note, voicecrossid);
      //balancepenalty = module_setting_fval(note, balanceid);
      smoothpenalty = module_setting_fval(note, smoothid);
      adjsmoothpenalty = module_setting_fval(note, adjsmoothid);
      expon = pow(2, -1 / module_setting_fval(note, exponid));
      vmax = module_setting_ival(note, vertmaxid);
      connectpenalty = module_setting_fval(note, connectid);
      dissimpenalty = module_setting_fval(note, dissimid);
    }
  };

  struct scorenode {
    const voicenode* node;
    fomus_float dist;
    scorenode(const voicenode* node, const fomus_float dist):node(node), dist(dist) {}
  };
  
  // voice overlap and cross--overlapping notes in wrong order--should be penalized severely--crossing notes should be penalized gently
  inline fomus_float voiceolap(const voicenode& n1, const voicenode& n2) { 
    if (n1.teti > n2.ti) { // must be overlapping...
      if ((n1.no > n2.no && n1.voice > n2.voice) || (n2.no > n1.no && n2.voice > n1.voice)) return n2.voiceolappenalty;
    }
    return 0;
  }
  inline fomus_float voicecross(const voicenode& n1, const voicenode& n2) { 
    if (n1.teti <= n2.ti) { // must NOT be overlapping...
      if ((n1.no > n2.no && n1.voice > n2.voice) || (n2.no > n1.no && n2.voice > n1.voice)) return n2.voicecrosspenalty;
    }
    return 0;
  }
  inline fomus_float dissim(const voicenode& n1, const voicenode& n2) {
    assert(n1.ti <= n2.ti);
    if (n1.voice == n2.voice && n1.teti > n2.ti && (n1.teti != n2.teti || n1.ti != n2.ti)) return n2.dissimpenalty;
    return 0;
  }  
  inline fomus_float smooth(const voicenode& n1, const voicenode& n2) {
    if (n1.teti < n2.ti) { // must NOT be overlapping or touching...
      return fabs(module_rattofloat(n1.no - n2.no)) * ((double)1 / (double)12) * n2.smoothpenalty;
    }
    return 0;
  }
  inline fomus_float adjsmooth(const voicenode& n1, const voicenode& n2) {
    if (n1.teti >= n2.ti) { // must be overlapping or touching...
      return fabs(module_rattofloat(n1.no - n2.no)) * ((double)1 / (double)12) * n2.smoothpenalty;
    }
    return 0;
  }
  // a grace note is touching the current note--boost score to maintain continuity
  inline fomus_float previsgrace(const voicenode& n1, const voicenode& n2) {
    return (n1.isgr && n1.teti >= n2.ti) ? n2.previsgracescore : 0;
  }

  struct voicesdata _NONCOPYABLE {
    std::vector<int> voices; // individual voices, size is the nchoices
    search_api api; // engine api
    module_noteobj ass, getn;
    dist_iface diface; // fill it up with data!
    voicesdata():ass(0), getn(0) {
      diface.moddata = 0;
      diface.data.octdist_setid = octdistid;
      diface.data.beatdist_setid = beatdistid;
      diface.data.byendtime = true;
      module_partobj p = module_peeknextpart(0);
      diface.data.rangemax = module_setting_fval(p, rangeid);
      module_get_auxiface(module_setting_sval(p, distmodid), DIST_INTERFACEID, &diface);
    }
    ~voicesdata() {if (diface.moddata) diface.free_moddata(diface.moddata);}
    bool isoutofrange(const search_node n1, const search_node n2) const {return diface.is_outofrange(diface.moddata, ((voicenode*)n1)->note, ((voicenode*)n2)->note);}
    int getlistofvoices(const module_partobj p) {
      module_intslist vl(module_voices(p)); // gets them all
      voices.assign(vl.ints, vl.ints + vl.n);
      return voices.size();
    }
    
    search_score getscore(const search_nodes& nodes) const { // nodes ordered by offset, last one is the node being scored
      assert(nodes.n > 0); // nodes are guaranteed to be in range, though needs to be pruned
      std::vector<scorenode> arr;
      voicenode** ie = (voicenode**)nodes.nodes + nodes.n - 1;
      const voicenode& n2 = **ie;
      for (voicenode** i = (voicenode**)nodes.nodes; i != ie; ++i) {
	fomus_float d = diface.dist(diface.moddata, (*i)->note, n2.note);
	if (d <= diface.data.rangemax) arr.push_back(scorenode(*i, pow(n2.expon, d)));
      }
      search_score sc;
      sc.f = vertmax(arr, n2);
      if (!arr.empty()) {
	fomus_float mx = 0, ll = 0;
	for (std::vector<scorenode>::const_iterator i(arr.begin()); i != arr.end(); ++i) {
	  assert(i->dist > 0);
	  ll += i->dist;
	  mx += (previsgrace(*i->node, n2) + voiceolap(*i->node, n2) + voicecross(*i->node, n2)
		 + dissim(*i->node, n2) + smooth(*i->node, n2) + adjsmooth(*i->node, n2)) * i->dist;
	}
	sc.f += mx / ll;
      }
#ifndef NDEBUGOUT
      for (std::vector<scorenode>::const_iterator i(arr.begin()); i != arr.end(); ++i) {
	DBG("no " << module_pitch(i->node->note) << " vo " << i->node->voice << ", ");
      }
#endif      
      DBG("no " << module_pitch(n2.note) << " vo " << n2.voice << ": ");
      DBG("::: sc " << sc.f << std::endl);
      return sc;
    }
    fomus_float vertmax(const std::vector<scorenode>& arr, const voicenode& n2) const {
      std::map<int, int> cnt; // map of voice -> note-count
      cnt.insert(std::map<int, int>::value_type(n2.voice, 1));
      fomus_rat clos = {std::numeric_limits<fomus_int>::max(), 1};
      bool issam = true; // don't penalize by default
      for (std::vector<scorenode>::const_iterator i(arr.begin()); i != arr.end(); ++i) {
	fomus_rat et(i->node->teti);
	if (et > n2.ti) { // overlapping notes only
	  std::map<int, int>::iterator c(cnt.find(i->node->voice)); // TODO: account for all possible voices
	  if (c == cnt.end()) cnt.insert(std::map<int, int>::value_type(i->node->voice, 1)); else ++c->second;
	} else { // not overlapping
	  fomus_rat d(n2.ti - et);
	  if (d < clos) {
	    clos = d;
	    issam = (i->node->voice == n2.voice);
	  } else if (d <= clos) {
	    issam = issam || (i->node->voice == n2.voice);
	  }
	}
      }
      fomus_float sc = (issam ? 0 : n2.connectpenalty);
      assert(voices.size() > 0);
      for (std::vector<int>::const_iterator i(voices.begin()); i != voices.end(); ++i) { // TODO: only those present in simult. notes
	std::map<int, int>::iterator c(cnt.find(*i));
	int n = (c == cnt.end() ? 0 : c->second);
	if (n > n2.vmax) sc += (n - n2.vmax) * n2.vertmaxpenalty;
      }
      return sc;
    } 
    search_node newnode(const search_node prevnode, const int choice) {
      assert(prevnode);
      module_noteobj n = (prevnode == api.begin)
	? (getn ? module_peeknextnote(0) : (getn = module_nextnote()))
	: ((((voicenode*)prevnode)->note != getn)
	   ? module_peeknextnote(((voicenode*)prevnode)->note)
	   : (getn = module_nextnote()));
      if (!n) return api.end;
      assert(choice >= 0 && choice < (int)voices.size());
      if (!module_hasvoice(n, voices[choice])) return 0;
      return new voicenode(n, voices[choice]);
    }
    void assignnext(const int choice) {
      DBG("ASSIGNING VOICE = " << voices[choice] << std::endl);
      voices_assign(ass = module_peeknextnote(ass), voices[choice]);
      assert(ass);
    };
  };

  extern "C" {
    void search_assign(void* moddata, int choice); // makes a solution assignment & reports it so that other phases of the program can continue
    union search_score search_get_score(void* moddata, struct search_nodes nodes);
    search_node search_new_node(void* moddata, search_node prevnode, int choice); // return NULL if no valid next node or END if at end of search--`prevnode' might be BEGIN (get the first node)
    void search_free_node(void* moddata, search_node node);
    const char* search_err(void* moddata);
    int search_score_lt(void* moddata, union search_score x, union search_score y); // less is better
    union search_score search_score_add(void* moddata, union search_score x, union search_score y);
    int search_is_outofrange(void* moddata, search_node node1, search_node node2);
  }

  void search_assign(void* moddata, int choice) {((voicesdata*)moddata)->assignnext(choice);} // makes a solution assignment & reports it so that other phases of the program can continue
  union search_score search_get_score(void* moddata, struct search_nodes nodes) {return ((voicesdata*)moddata)->getscore(nodes);}
  search_node search_new_node(void* moddata, search_node prevnode, int choice) {return ((voicesdata*)moddata)->newnode(prevnode, choice);} // return NULL if no valid next node or END if at end of search--`prevnode' might be BEGIN (get the first node)
  void search_free_node(void* moddata, search_node node) {delete (voicenode*)node;}
  const char* search_err(void* moddata) {return 0;}
  int search_score_lt(void* moddata, union search_score x, union search_score y) {return x.f > y.f;} // less is better
  union search_score search_score_add(void* moddata, union search_score x, union search_score y) {search_score r; r.f = x.f + y.f; return r;}
  int search_is_outofrange(void* moddata, search_node node1, search_node node2) {return ((voicesdata*)moddata)->isoutofrange(node1, node2);}

  // ------------------------------------------------------------------------------------------------------------------------
  
  const char* vertmaxtype = "integer>=1";
  int valid_vertmax(const struct module_value val) {return module_valid_int(val, 1, module_incl, 0, module_nobound, 0, vertmaxtype);}
  const char* heapsizetype = "integer>=10";
  int valid_heapsize(const struct module_value val) {return module_valid_int(val, 10, module_incl, 0, module_nobound, 0, heapsizetype);}
  const char* scoretype = "real>=0";
  int valid_score(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int)0), module_incl, module_makeval((fomus_int)0), module_nobound, 0, scoretype);
  }

  // distance module setting stuff
  std::set<std::string> modstrset;
  std::string modstrtype;
  int valid_modstr_aux(const char* str) {return modstrset.find(str) != modstrset.end();}  
  int valid_modstr(const struct module_value val) {return module_valid_string(val, 1, -1, valid_modstr_aux, modstrtype.c_str());}
  
  const char* voicesdistmulttype = "real>0";
  int valid_voicesdistmult(const struct module_value val) {
    return module_valid_num(val, module_makeval((fomus_int)0), module_excl, module_makeval((fomus_int)0), module_nobound, 0, voicesdistmulttype);
  }
  std::string enginemodstype;
  std::set<std::string> enginemodsset;
  int valid_enginemod_aux(const char* str) {return enginemodsset.find(str) != enginemodsset.end();}
  int valid_enginemod(const struct module_value val) {return module_valid_string(val, -1, -1, valid_enginemod_aux, enginemodstype.c_str());}
  
}

using namespace voices;

int module_engine_iface() {return ENGINE_INTERFACEID;}
void module_fill_iface(void* moddata, void* iface) { // module
  ((voicesdata*)moddata)->api = ((search_iface*)iface)->api;
  ((search_iface*)iface)->moddata = moddata;
  ((search_iface*)iface)->assign = search_assign;
  ((search_iface*)iface)->get_score = search_get_score;
  ((search_iface*)iface)->new_node = search_new_node;
  ((search_iface*)iface)->free_node = search_free_node;
  ((search_iface*)iface)->err = search_err;
  ((search_iface*)iface)->score_lt = search_score_lt;
  ((search_iface*)iface)->score_add = search_score_add;
  ((search_iface*)iface)->is_outofrange = search_is_outofrange;
  module_partobj p = module_peeknextpart(0);
  ((search_iface*)iface)->nchoices = ((voicesdata*)moddata)->getlistofvoices(p);
  union search_score ms;
  ms.f = std::numeric_limits<fomus_float>::max();
  ((search_iface*)iface)->min_score = ms;
  ((search_iface*)iface)->heapsize = module_setting_ival(p, heapsizeid);
}

const char* module_longname() {return "Voices";}
const char* module_author() {return "(fomus)";}
const char* module_doc() {return "Separates notes into voices";}
void* module_newdata(FOMUS f) {return new voicesdata;}
void module_freedata(void* dat) {delete (voicesdata*)dat;}
int module_priority() {return 0;}
enum module_type module_type() {return module_modvoices;}
int module_itertype() {return module_bypart | module_norests | module_firsttied | module_noperc;}

const char* module_initerr() {return 0;}
void module_init() {}
void module_free() {/*assert(newcount == 0);*/}

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {  
  case 0:
    {
      set->name = "vertmax"; // docscat{instsparts}
      set->type = module_int;
      set->descdoc = "The maximum number of notes an instrument is able to play simultaneously in a single voice."
	"  Set this in an instrument or part to the maximum number of notes that can appear in a chord."
	"  For example, most instruments can only play one note at a time but keyboard instruments could have a value of 5 or more."
	"  String instruments might also require larger values here for chords or double stops, etc..";
      set->typedoc = vertmaxtype; 

      module_setval_int(&set->val, 1);
      
      set->loc = module_locnote;
      set->valid = valid_vertmax;
      set->uselevel = 2; 
      vertmaxid = id;
    }
    break;
  case 1:
    {
      set->name = "voices-searchdepth"; // docscat{voices}
      set->type = module_int;
      set->descdoc = "The search depth used when an appropriate engine module (such as `bfsearch') is selected."
	"  A larger search depth increases both computation time and quality."
	// "  FOMUS looks ahead and considers this many events before making a decision."
	;
      set->typedoc = heapsizetype; 

      module_setval_int(&set->val, 25);
      
      set->loc = module_locpart;
      set->valid = valid_heapsize;
      set->uselevel = 3; 
      heapsizeid = id;
    }
    break;
  case 2:
    {
      set->name = "voices-vertmax-score"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "Score when the number of overlapping notes in one voice doesn't exceed the limit for the instrument.  "
	"Increasing this increases the likelihood that chords exceeding the `vertmax' limit are split into separate voices (if possible).";
      set->typedoc = scoretype; 

      module_setval_float(&set->val, 1000);
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      vertmaxscid = id;
    }
    break;
  case 3:
    {
      set->name = "voices-prevgrace-score"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "Score when the previous note in the same voice is a grace note (to insure continuity).  "
	"Increasing this value causes grace notes to stick together.";
      set->typedoc = scoretype; 

      module_setval_float(&set->val, 5);
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      previsgraceid = id;
    }
    break;
  case 4:
    {
      set->name = "voices-voiceoverlap-score"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "Score when overlapping notes are ordered as expected (i.e., overlapping higher notes are in upper voices and vice versa).  "
	"Increasing this value increases the chance of overlapping notes being ordered correctly.";
      set->typedoc = scoretype; 

      module_setval_float(&set->val, 21);
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      voiceolapid = id;
    }
    break;
  case 5:
    {
      set->name = "voices-smoothness-score"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "Score when notes in close proximity to each other are also close to each other in pitch.  "
	"Increasing this value improves the voice leading in each voice.";
      set->typedoc = scoretype; 

      module_setval_float(&set->val, 2); // 5
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      smoothid = id;
    }
    break;
  case 6:
    {
      set->name = "voices-dissimilar-score"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "Score encouraging dissimilar rhythms in separate voices."
	"  This also encourages notes containing the same rhythms to be appropriately grouped into chords.  "
	"Increasing this value decreases the chance of multiple rhythms being notated in the same voice with a lot of chords and ties.";
      set->typedoc = scoretype; 
      
      module_setval_float(&set->val, 55);
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      dissimid = id;
    }
    break;
  case 7:
    {
      set->name = "voices-octavedist"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "If the `cartdist' distance or similar module is selected, this is the distance of one octave"
	" (i.e., two notes an octave apart have a distance equivalent to this value)."
	"  A larger distance translates to a smaller weight in decisions regarding voice assignments and vice versa.";
      set->typedoc = scoretype; 

      module_setval_float(&set->val, 1);
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      octdistid = id;
    }
    break;
  case 8:
    {
      set->name = "voices-beatdist"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "If the `cartdist' distance or similar module is selected, this is the distance of one beat in a measure."
	"  A larger distance translates to a smaller weight in decisions regarding voice assignments and vice versa.";
      set->typedoc = scoretype; 

      module_setval_float(&set->val, 1);
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      beatdistid = id;
    }
    break;
  case 9:
    {
      set->name = "voices-maxdist"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "The maximum distance allowed for voice assignment calculations."
	"  Notes that are more than this distance apart from each other are considered unrelated to each other and are not included in FOMUS's calculations.";
      set->typedoc = scoretype; 

      module_setval_float(&set->val, 5);
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      rangeid = id;
    }
    break;
  case 10:
    {
      set->name = "voices-distmod"; // docscat{voices}
      set->type = module_string;
      set->descdoc = "Module that is used to calculate the \"distance\" between notes."
	"  The closer two notes are to each other, the more important their relationship is in terms of note spelling, staff choice, voice assignments, etc.."
	//"  Since distance can be calculate in many different ways, there are several interchangeable modules for this."
	"  Set this to change the algorithm used for calculating distance when making decisions regarding voice assignments."
	;

      struct info_modfilter fi0 = {0, 0, 0, module_modaux, DIST_INTERFACEID};
      struct info_modfilterlist fi = {1, &fi0};
      const struct info_modlist ml(info_list_modules(&fi, 0, 0, -1));
      std::ostringstream s;
      for (const info_module* i = ml.mods, *ie = ml.mods + ml.n; i != ie; ++i) {
	modstrset.insert(i->name);
	if (i != ml.mods) s << '|';
	s << i->name;
      }
      modstrtype = s.str();
      set->typedoc = modstrtype.c_str();

      module_setval_string(&set->val, "notedist");

      set->loc = module_locnote;
      set->valid = valid_modstr;
      set->uselevel = 3; 
      distmodid = id;
    }
    break;
  case 11:
    {
      set->name = "voices-distmult"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "FOMUS calculates the distance between note events so that it can weigh scores and penalties according to how close they are to each other."
	"  An exponential curve is usually applied before actually calculating the weight."
	"  This value is the distance at which a score calculated between two notes is decreased to 1/2 it's full value"
	" (at twice this distance the score is decreased to 1/4 it's full value, etc.).";
      set->typedoc = voicesdistmulttype; // uses the same string/valid fun

      module_setval_int(&set->val, 1);
      
      set->loc = module_locnote;
      set->valid = valid_voicesdistmult;
      set->uselevel = 3;
      exponid = id;
    }
    break;
  case 12:
    {
      set->name = "voices-engine"; // docscat{voices}
      set->type = module_string;
      set->descdoc = "Engines provide different types of search functionality to the rest of FOMUS's modules and are interchangeable."
	"  For example, two of FOMUS's default engines `dynprog' and `bfsearch' execute two different search algorithms, each with different benefits."
	"  Set this to the name of an engine module to change the search algorithm used for finding voice assignments.";

      struct info_modfilter fi0 = {0, 0, 0, module_modengine, ENGINE_INTERFACEID};
      struct info_modfilterlist fi = {1, &fi0};
      const struct info_modlist ml(info_list_modules(&fi, 0, 0, -1));
      std::ostringstream s;
      for (const info_module* i = ml.mods, *ie = ml.mods + ml.n; i != ie; ++i) {
	enginemodsset.insert(i->name);
	if (i != ml.mods) s << '|';
	s << i->name;
      }
      enginemodstype = s.str();
      set->typedoc = enginemodstype.c_str();

      module_setval_string(&set->val, "bfsearch");
      
      set->loc = module_locpart;
      set->valid = valid_enginemod;
      set->uselevel = 3;
      enginemodid = id;
      break;
    }
  case 13:
    {
      set->name = "voices-voicecross-score"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "Score when notes crossing in separate voices are ordered as expected (i.e., higher notes are in upper voices and vice versa).  "
	"Increasing this increases the chance of crossing notes being ordered correctly.";
      set->typedoc = scoretype; 

      module_setval_float(&set->val, 2);
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      voicecrossid = id;
    }
    break;
  case 14:
    {
      set->name = "voices-connect-score"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "Encourages neighboring notes to be in a single voice (prevents hockets between voices).  "
	"Increasing this helps insure that continuity exists in each voice.";
      set->typedoc = scoretype; 
      
      module_setval_float(&set->val, 5);
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      connectid = id;
    }
    break;
  case 15:
    {
      set->name = "voices-voiceleading-score"; // docscat{voices}
      set->type = module_number;
      set->descdoc = "Score when adjacent notes are smoothly connected (i.e., smaller intervals are preferred over larger ones).  "
	"Increasing this value improves the voice leading in each voice.";
      set->typedoc = scoretype; 

      module_setval_float(&set->val, 13); // 5
      
      set->loc = module_locnote;
      set->valid = valid_score;
      set->uselevel = 3; 
      smoothid = id;
    }
    break;
  default:
    return 0;
  }
  return 1;
}

const char* module_engine(void* f) {return module_setting_sval(module_peeknextpart(0), enginemodid);}
void module_ready() {}
inline bool icmp(const module_obj a, const module_obj b, const int id) {return module_setting_ival(a, id) == module_setting_ival(a, id);}
inline bool vcmp(const module_obj a, const module_obj b, const int id) {return module_setting_val(a, id) == module_setting_val(a, id);}
inline bool scmp(const module_obj a, const module_obj b, const int id) {return strcmp(module_setting_sval(a, id), module_setting_sval(a, id)) == 0;}
int module_sameinst(module_obj a, module_obj b) {
  return
    scmp(a, b, enginemodid) && icmp(a, b, heapsizeid) &&
    scmp(a, b, distmodid) && vcmp(a, b, octdistid) && vcmp(a, b, beatdistid) && vcmp(a, b, rangeid);
}

