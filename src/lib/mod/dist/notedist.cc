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

#include <algorithm> // std::swap
#include <cassert>
#include <limits>
#include <map>
#include <set>
#include <utility> // std::pair
#include <vector>

#include <boost/utility.hpp>

#include "ifacedist.h"
#include "module.h"
#include "modutil.h"

namespace notedist {

  struct etnode {
    const module_noteobj note;
    const fomus_rat et;
    etnode(const fomus_rat& et, const module_noteobj& note)
        : note(note), et(et) {}
  };
  inline bool operator<(const etnode& x, const etnode& y) {
    if (x.et != y.et)
      return x.et < y.et;
    return x.note < y.note;
  }

  typedef std::map<std::pair<module_noteobj, module_noteobj>, fomus_float>
      cachemap;
  typedef std::set<etnode> prevmap; // should be a set, not a multiset

  struct distdata {       // TODO: put in pool, will get created every measure
    const bool byendtime; // if true, then distance can = 0
    const fomus_float rng;
    cachemap cache, rcache;
    prevmap prevs;
    distdata(const dist_iface& iface)
        : byendtime(iface.data.byendtime), rng(iface.data.rangemax) {}
    fomus_float dist(const module_noteobj note1, const module_noteobj note2,
                     const bool cr = false);
    bool isoutofmaxrange(
        const module_noteobj note1,
        const module_noteobj
            note2) { // if byendtime, calling module must add notes to beginning
                     // of arr until endtimes are < original first time
      return dist(note1, note2, true) > rng;
    }
  };

  struct scoped_rangeobj {
    modutil_rangesobj o;
    scoped_rangeobj() : o(ranges_init()) {}
    ~scoped_rangeobj() {
      ranges_free(o);
    }
    void insert(const fomus_rat& x1, const fomus_rat& x2) const {
      modutil_range v = {module_makeval(x1), module_makeval(x2)};
      ranges_insert(o, v);
    }
    void remove(const fomus_rat& x1, const fomus_rat& x2) const {
      modutil_range v = {module_makeval(x1), module_makeval(x2)};
      ranges_remove(o, v);
    }
    fomus_int size() const {
      return ranges_size(o);
    }
  };

  // this module assumes note2 doesn't skip ahead!
  fomus_float distdata::dist(const module_noteobj note1,
                             const module_noteobj note2,
                             const bool cr) { // cr = calculating in/outofrange
    assert(!module_isrest(note1));
    assert(!module_isrest(note2));
    if (cr) {
      cachemap::const_iterator c(rcache.find(cachemap::key_type(
          note1,
          note2))); // TODO: find most efficient cache container (hash table??)
      if (c != rcache.end())
        return c->second; // notes should be all notes from note1 to note2
    } else {
      cachemap::const_iterator c(cache.find(cachemap::key_type(note1, note2)));
      if (c != cache.end())
        return c->second;
    }
    std::vector<module_noteobj> nts;
    if (byendtime) {
      fomus_rat n1et(module_tiedendtime(note1));
      for (prevmap::const_iterator i(prevs.lower_bound(etnode(n1et, 0)));
           i != prevs.end(); ++i) { // all end times >= note1's
        if (module_less(i->note, note1))
          nts.push_back(i->note);
      }
      sort(nts.begin(), nts.end(), module_less);
      prevs.insert(etnode(n1et, note1));
    } else {
      fomus_rat t(module_time(note1));
      for (prevmap::const_iterator i(prevs.lower_bound(etnode(t, 0)));
           i->et <= t; ++i) {
        if (module_less(i->note, note1))
          nts.push_back(i->note);
      }
      prevs.insert(prevmap::value_type(t, note1));
    }
    for (module_noteobj n = note1;; n = module_peeknextnote(n)) {
      nts.push_back(n);
      if (n == note2)
        break;
      assert(n);
    }
    fomus_rat t(module_time(note2));
    for (module_noteobj n = module_peeknextnote(note2);
         n && module_time(n) <= t; n = module_peeknextnote(n))
      nts.push_back(n);
    fomus_rat minn;
    fomus_rat maxn;
    if (!cr) {
      minn = module_pitch(note1);
      maxn = module_pitch(note2);
      if (minn > maxn)
        std::swap(minn, maxn);
    }
    fomus_rat mino(byendtime ? module_tiedendtime(note1) : module_time(note1));
    fomus_rat maxo(module_time(note2));
    // holes hls(mino, maxo);
    scoped_rangeobj hls;
    if (maxo > mino)
      hls.insert(mino, maxo);
    fomus_int s = 0;
    module_measobj lm = 0;
    for (std::vector<module_noteobj>::const_iterator i(nts.begin());
         i != nts.end(); ++i) {
      if (*i != note2) { // note2 is the target note
        fomus_rat n(module_pitch(*i));
        fomus_rat o1(module_time(*i));
        fomus_rat o2(module_tiedendtime(*i));
        if ((byendtime ? o2 >= mino : o2 > mino) &&
            o1 <= maxo) { // is in the square
          if (!cr && n >= minn && n <= maxn)
            ++s; // if cr=true, exclude figuring notes into calculation to get
                 // mininum w/o notes
          // hls.remhole(hole(o1, o2)); // "holes" are rests, an element between
          // note1 and note2
          hls.remove(o1, o2);
        }
      }
      module_measobj m = module_meas(*i);
      assert(m);
      if (lm) { // works only if notes are sorted, which they are
        while (lm != m) {
          // std::cout << "*" << std::endl;
          ++s; // add one for each barline
          fomus_rat t(module_endtime(lm));
          // hls.remhole(hole(t, t));
          hls.remove(t, t);
          lm = module_peeknextmeas(lm); // works for measures also
          assert(lm);
        }
      } else
        lm = m;
    }
    // std::cout << s << std::endl;
    s += hls.size();
    if (cr) {
      rcache.insert(cachemap::value_type(cachemap::key_type(note1, note2), s));
    } else {
      cache.insert(cachemap::value_type(cachemap::key_type(note1, note2), s));
    }
    return s;
  }

  extern "C" {
  fomus_float dist_dist(void* moddata, module_noteobj note1,
                        module_noteobj note2);
  void free_moddata(void* moddata);
  int is_outofrange(void* moddata, module_noteobj note1, module_noteobj note2);
  }

  fomus_float dist_dist(void* moddata, module_noteobj note1,
                        module_noteobj note2) {
    return ((distdata*) moddata)->dist(note1, note2, false);
  }
  void free_moddata(void* moddata) {
    delete (distdata*) moddata;
  }
  int is_outofrange(void* moddata, module_noteobj note1, module_noteobj note2) {
    return ((distdata*) moddata)->isoutofmaxrange(note1, note2);
  }
}; // namespace notedist

using namespace notedist;

void aux_fill_iface(void* iface) {
  ((dist_iface*) iface)->moddata = new distdata(*(dist_iface*) iface);
  ((dist_iface*) iface)->dist = dist_dist;
  ((dist_iface*) iface)->is_outofrange = is_outofrange;
  ((dist_iface*) iface)->free_moddata = free_moddata;
}
int aux_ifaceid() {
  return DIST_INTERFACEID;
} // divrules.cc must provide a unique one in header divrules.h file

const char* module_longname() {
  return "Note Distance";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Calculates distance between two objects by counting the number of "
         "notes (and other objects such as measure barlines) that appear "
         "between them on the score.  "
         "Slower than other distance measurements based on temporal difference "
         "between events, but causes decisions to be based more on how "
         "notational objects are actually arranged on the score.";
}
enum module_type module_type() {
  return module_modaux;
}
// int module_priority() {return 0;}
int module_get_setting(int n, module_setting* set, int id) {
  return 0;
}
void module_ready() {}
void module_init() {
  // #ifndef NDEBUG
  //   std::cout << "DEBUGGING HOLES\n";
  //   holes x(module_makerat(0, 1), module_makerat(11, 1));
  //   for (holes::const_iterator i(x.begin()); i != x.end(); ++i) std::cout <<
  //   *i; std::cout << '\n'; x.remhole(hole(module_makerat(-1, 1),
  //   module_makerat(1, 1))); for (holes::const_iterator i(x.begin()); i !=
  //   x.end(); ++i) std::cout << *i; std::cout << '\n';
  //   x.remhole(hole(module_makerat(10, 1), module_makerat(11, 1)));
  //   for (holes::const_iterator i(x.begin()); i != x.end(); ++i) std::cout <<
  //   *i; std::cout << '\n'; x.remhole(hole(module_makerat(4, 1),
  //   module_makerat(6, 1))); for (holes::const_iterator i(x.begin()); i !=
  //   x.end(); ++i) std::cout << *i; std::cout << '\n';
  //   x.remhole(hole(module_makerat(3, 1), module_makerat(5, 1)));
  //   for (holes::const_iterator i(x.begin()); i != x.end(); ++i) std::cout <<
  //   *i; std::cout << '\n'; x.remhole(hole(module_makerat(8, 1),
  //   module_makerat(9, 1))); for (holes::const_iterator i(x.begin()); i !=
  //   x.end(); ++i) std::cout << *i; std::cout << '\n';
  //   x.remhole(hole(module_makerat(2, 1), module_makerat(9, 1)));
  //   for (holes::const_iterator i(x.begin()); i != x.end(); ++i) std::cout <<
  //   *i; std::cout << '\n'; x.remhole(hole(module_makerat(2, 1),
  //   module_makerat(10, 1))); for (holes::const_iterator i(x.begin()); i !=
  //   x.end(); ++i) std::cout << *i; std::cout << '\n';
  //   x.remhole(hole(module_makerat(1, 1), module_makerat(10, 1)));
  //   for (holes::const_iterator i(x.begin()); i != x.end(); ++i) std::cout <<
  //   *i; std::cout << '\n'; exit(1);
  // #endif
}
void module_free() { /*assert(newcount == 0);*/
}
// void* module_newdata(FOMUS f) {return new data;}
// void module_freedata(void* dat) {delete (data*)dat;}
// const char* module_err(void* dat) {return ((data*)dat)->module_err();}
const char* module_initerr() {
  return 0;
}
// const char* module_err(void* dat) {return 0;}
