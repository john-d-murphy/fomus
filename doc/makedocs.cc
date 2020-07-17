#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <cassert>
#include <vector>
#include <utility>
#include <algorithm>
#include <fstream>
#include <set>
#include <cctype>

#include <boost/ptr_container/ptr_map.hpp>

#include <fomusapi.h>
#include <infoapi.h>

const char* getval(const module_value& x, const std::string& id) {
  assert(x.type == module_list);
  for (const module_value *i(x.val.l.vals), *ie(x.val.l.vals + x.val.l.n); i < ie; i += 2) {
    assert(i->type == module_string);
    if (id == i->val.s) {
      assert((i + 1)->type == module_string);
      return (i + 1)->val.s;
    }
  }
  return 0;
}

std::string stringify(const std::string& str) {
  std::ostringstream ou;
  for (std::string::const_iterator i(str.begin()); i != str.end(); ) {
    if ((unsigned char)*i < 0x80) {
      switch (*i) {
      case '@':
      case '{':
      case '}':
	ou << '@';
      }
      ou << *i++;
    } else {
      unsigned long u = 0;
      std::string::const_iterator i0(i);
      unsigned char x = (unsigned char)*i;
      while (x >= 0x80) {
	x = x << 1;
	if (i == str.end() || (unsigned char)*i < 0x80) {
	  std::cerr << "invalid UTF-8 character" << std::endl;
	  exit(EXIT_FAILURE);
	}
	u = (u * 0x100) + (unsigned char)*i++;
      }
      switch (u) {
      case 50088: ou << "@`e"; break;
      case 50089: ou << "@'e"; break;
      default:
	std::cerr << "makedocs: stringify missing UTF-8 translation for " << u << ' ' << std::string(i0, i) << std::endl;
	exit(EXIT_FAILURE);
      }
    }
  }
  return ou.str();
}

std::string stripnonalpha(const std::string& str) {
  std::ostringstream ou;
  bool ca = true;
  for (std::string::const_iterator i(str.begin()); i != str.end(); ++i) {
    if (isalnum(*i)) {
      ou << (char)(ca ? toupper(*i) : *i);
      ca = false;
    } else ca = true;
  }
  return ou.str();  
}

const char* thecats[][3] = {{"basic", "Basic", "Settings affecting FOMUS's overall operation and performance, plus a few global score parameters."},
			    {"syntax", "Syntax", "Settings specifying the syntax of certain items in `.fms' input files."},
			    {"libs", "Libraries", "Settings that belong in your `.fomus' initialization file and function as libraries."},
			    {"instsparts", "Instrument Definitions/Parts", "Settings that have more or less to do with defining instruments and/or parts."},
			    {"quant", "Quantization", "Settings that determine how time, durations and pitches are quantized."},
			    {"meas", "Measures", "Settings affecting measures and items related to measures."},
			    {"voices", "Voices", "Settings affecting how notes are distributed into voices."},
			    {"staves", "Staves", "Settings affecting how notes are distributed onto staves."},
			    {"notes", "Notes", "Settings that allow you to override certain decisions regarding notes."},
			    {"grnotes", "Grace Notes", "Settings affecting grace notes."},
			    {"marks", "Marks", "Settings affecting the behavior of marks and mark objects."},
			    {"dyns", "Dynamics", "Settings affecting dynamic markings and the translation of dynamic values into text markings."},
			    {"accs", "Accidentals", "Settings affecting accidental spellings."},
			    {"octs", "Octave Signs", "Settings affecting octave sign placement."},
			    {"tuplets", "Tuplets", "Settings affecting how tuplets are created, plus settings to override tuplet placement."},
			    {"beams", "Beams", "Settings affecting placement of beams."},
			    {"special", "Special Notations", "Settings for special notations such as string harmonics, trills and tremolos."},
			    {"rhythmic", "Rhythmic Spelling", "Settings affecting how notes and rests are split and tied in a measure."},
			    {"fmsin", "File Input", "Settings affecting input from FOMUS text files."},
			    {"fmsout", "File Output", "Settings affecting output to FOMUS text files."},
			    {"lilyout", "LilyPond Output", "Settings affecting output to LilyPond input files."},
			    {"xmlout", "MusicXML Output", "Settings affecting output to MusicXML files."},
			    {"midiin", "MIDI Input", "Settings affecting output to MIDI files."},
			    {"midiout", "MIDI Output", "Settings affecting input from MIDI files."},
			    {"mods", "Modules", "Settings for selecting and combining modules, which contain all of FOMUS's decision-making algorithms."},
			    {"other", "Other", "Odds and ends."},
			    {0}};
			    

int main(int ac, char** av) {
  std::string cmd(av[1]);
  fomus_init();
  if (cmd == "sets1") {
    std::set<std::string> val; // valid cats w/ documentation
    for (int i = 0; thecats[i][0] != 0; ++i) val.insert(thecats[i][0]);
    boost::ptr_map<const std::string, std::set<std::string> > cats; // category, vect-of-sets
    std::ifstream f("sets_cats.in", std::ifstream::in);
    int nn = 0;
    while (!f.eof()) {
      std::string c, s;
      f >> c >> std::ws >> s >> std::ws;
      if (val.find(c) == val.end()) {
	std::cerr << "makedocs: invalid category `" << c << "\'\n";
	return EXIT_FAILURE;
      }
      cats[c].insert(s);
      ++nn;
    }
    f.close();
    info_setfilter fi = {0, 0, 0, module_nomodtype, 0, module_noloc, 3, info_none};
    info_setfilterlist fl = {1, &fi};
    const struct info_setlist lst(info_list_settings(0, &fl, 0, 0, 0));
    if (nn < lst.n) { // <
      std::cerr << "makedocs: missing a setting category somewhere\n";
      return EXIT_FAILURE;
    }
    std::cout << "@table @code\n";
    for (int i0 = 0; thecats[i0][0] != 0; ++i0) {
      boost::ptr_map<const std::string, std::set<std::string> >::const_iterator i(cats.find(thecats[i0][0]));
      assert(i != cats.end());
      std::cout << "@item " << stringify(thecats[i0][1]) << '\n' << stringify(thecats[i0][2]) << "\n@table @code\n";
      const std::set<std::string>& sets = *i->second;
      for (int ul = 0; ul <= 3; ++ul) {
	bool fi = true;
	for (struct info_setting *i(lst.sets), *ie(lst.sets + lst.n); i < ie; ++i) {
	  if (i->uselevel == ul && sets.find(i->name) != sets.end()) {
	    if (fi) {
	      static const char* ulvls[4] = {"Beginner", "Intermediate", "Advanced", "Guru"};
	      std::cout << "@item " << stringify(ulvls[ul]) << "\n"; // different kind of list?
	      fi = false;
	    }
	    std::cout << "@ref{set" << stripnonalpha(i->name) << ",," << stringify(i->name) << "}  ";
	  }
	}
	if (!fi) std::cout << "\n";
      }
      std::cout << "@end table\n";
    }
    std::cout << "@end table\n";
  } else if (cmd == "sets2") {
    const std::string nthr("n-threads");
    info_setfilter fi = {0, 0, 0, module_nomodtype, 0, module_noloc, 3, info_none};
    info_setfilterlist fl = {1, &fi};
    const struct info_setlist lst(info_list_settings(0, &fl, 0, 0, 0));    
    std::cout << "@table @code\n";
    for (struct info_setting *i(lst.sets), *ie(lst.sets + lst.n); i < ie; ++i) {
      const char* def = (i->name == nthr ? "(machine dependent)" : i->valstr);
      std::cout << "@anchor{set" << stripnonalpha(i->name) << "}\n@item " << stringify(i->name)
		<< "\ntype: @code{" << stringify(i->typedoc)
		<< "}\n\ndefault value: @code{" << stringify(def)
		<< "}\n\nlocation: @code{" << stringify(info_settingloc_to_extstr(i->loc))
		<< "}\n\n" << stringify(i->descdoc) << "\n";      
    }
    std::cout << "@end table\n";
  } else if (cmd == "marks") {
    std::cout << "@table @code\n";
    const struct info_marklist lst(info_list_marks(0, 0, 0, 0, 0));
    for (struct info_mark *i(lst.marks), *ie(lst.marks + lst.n); i < ie; ++i) {
      std::cout << "@item " << stringify(i->name)
		<< "\narguments: @code{" << stringify(i->argsdoc)
		<< "}\n\n" << stringify(i->descdoc) << "\n";
    }    
    std::cout << "@end table\n";
  } else if (cmd == "insts") {
    std::cout << "@itemize @bullet\n";
    info_setfilter fi = {0, 0, 0, module_nomodtype, "inst-defs", module_noloc, 3, info_none};
    info_setfilterlist fl = {1, &fi};
    const struct info_setlist lst(info_list_settings(0, &fl, 0, 0, 0));
    assert(lst.n == 1);
    struct module_value &v = lst.sets->val;
    assert(v.type == module_list);
    std::vector<std::pair<std::string, std::string> > nms;
    for (module_value *i(v.val.l.vals), *ie(v.val.l.vals + v.val.l.n); i < ie; ++i) {
      assert(getval(*i, "id"));
      const char* na = getval(*i, "name");
      if (!na) na = "...";
      nms.push_back(std::pair<std::string, std::string>(na, getval(*i, "id")));
    }
    std::sort(nms.begin(), nms.end());
    for (std::vector<std::pair<std::string, std::string> >::const_iterator i(nms.begin()); i != nms.end(); ++i) {
      std::cout << "@item\n" << stringify(i->first) << " (id: @code{" << stringify(i->second) << "})\n";
    }
    std::cout << "@end itemize\n";  
  } else if (cmd == "percs") {
    std::cout << "@itemize @bullet\n";
    info_setfilter fi = {0, 0, 0, module_nomodtype, "percinst-defs", module_noloc, 3, info_none};
    info_setfilterlist fl = {1, &fi};
    const struct info_setlist lst(info_list_settings(0, &fl, 0, 0, 0));
    assert(lst.n == 1);
    struct module_value &v = lst.sets->val;
    assert(v.type == module_list);
    for (module_value *i(v.val.l.vals), *ie(v.val.l.vals + v.val.l.n); i < ie; ++i) {
      assert(getval(*i, "id"));
      std::cout << "@item\n@code{" << stringify(getval(*i, "id")) << "}\n";
    }
    std::cout << "@end itemize\n";  
  } else if (cmd == "layouts") {
    // std::cout << "@itemize @bullet\n";
    // info_setfilter fi = {0, 0, 0, 0, "percinst-defs", 0, 0, 0};
    // info_setfilterlist fl = {1, &fi};
    // const struct info_setlist lst(info_list_settings(0, &fl, 0, 0, 0));
    // info_setting &i = *lst.sets;
    
    // std::cout << "@end itemize\n";    
  } else assert(false);
  return EXIT_SUCCESS;
}
