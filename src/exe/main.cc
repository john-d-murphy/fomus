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

#include <cctype>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
// #include <algorithm>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/program_options.hpp>

#include "config.h"
#include "fomusapi.h"
#include "infoapi.h"
#include "modtypes.h"
#ifdef HAVE_TERM_H
#include <cstdio>
#include <term.h>
#endif

#define CONSOLE_WIDTH 80

#define CERR std::cerr << "fomus: "

struct err {};
#define CHECK_ERR                                                              \
  if (fomus_err())                                                             \
  throw err()

struct fieldspec {
  std::string fi;
  std::string va;
  fieldspec(const std::string& fi, const std::string& va) : fi(fi), va(va) {}
  fieldspec(const fieldspec& x) : fi(x.fi), va(x.va) {}
  void operator=(const fieldspec& x) {
    fi = x.fi;
    va = x.va;
  }
};
struct fielderr {};
fieldspec check_fieldspec(const std::string& x) { // throws fielderr
  std::string::size_type i = x.find_first_of("=:");
  if (i == std::string::npos)
    throw fielderr();
  return fieldspec(x.substr(0, i), x.substr(i + 1));
}
inline void
fill_fieldspecs(std::vector<fieldspec>& v,
                const std::vector<std::string>& in) { // throws fielderr
  std::transform(in.begin(), in.end(), std::back_inserter(v),
                 boost::lambda::bind(check_fieldspec, boost::lambda::_1));
}

enum modfilts { modfilt_name, modfilt_longname, modfilt_author, modfilt_type };
enum modsims { modsim_name, modsim_longname, modsim_author, modsim_doc };

enum setfilts {
  setfilt_modname,
  setfilt_modlongname,
  setfilt_modauthor,
  setfilt_modtype,
  setfilt_name,
  setfilt_loc,
  setfilt_uselevel /*, setfilt_where*/
};
enum setsims {
  setsim_modname,
  setsim_modlongname,
  setsim_modauthor,
  setsim_moddoc,
  setsim_name,
  setsim_doc
};

enum markfilts {
  markfilt_modname,
  markfilt_modlongname,
  markfilt_modauthor,
  markfilt_modtype,
  markfilt_name
};
enum marksims {
  marksim_modname,
  marksim_modlongname,
  marksim_modauthor,
  marksim_moddoc,
  marksim_name,
  marksim_doc
};

#ifdef HAVE_TERM_H
inline int putfunc(int x) {
  std::cout.put(x);
  return x;
}
struct attr {
  const char* str;
  attr(const char* str) : str(str) {}
};
inline std::ostream& operator<<(std::ostream& os, const attr& attr) {
  if (attr.str != 0)
    tputs(attr.str, 1, putfunc);
  return os;
}
#define BOLD(xxx) attr(bold) << xxx << attr(nobold)
#define ULINE(xxx) attr(uline) << xxx << attr(nouline)
#else
#define BOLD(xxx) xxx
#define ULINE(xxx) xxx
#endif

// va = is a value (false for documentation strings, etc..)
void cout_justify(std::string s, const int start = 0, bool va = false) {
  // const int first = fi ? 0 : start;
  boost::replace_all(s, "\t", std::string(8, ' '));
  std::string::size_type i = 0;
  std::string::size_type je = s.length();
  const std::string exc("\"'`{}[]():;,.!?_"); // part of a word--anything else
                                              // will get split AFTER
  for (std::string::size_type j = (CONSOLE_WIDTH - 1) - start; j < je;
       j += (CONSOLE_WIDTH - 1) - start) {
    std::string::size_type js = i;
    while (js < j && s[js] != '\n')
      ++js;       // js is either at j or at next '\n'
    if (js < j) { // must be '\n'
      std::cout << s.substr(i, (++js) - i) << std::string(start, ' ');
      i = j = js;
    } else {
      js = j; // save j
      if (j < je && s[j] == ' ')
        ++j;
      while (j > i && (va ? s[j] == ' '
                          : !(isalnum(s[j]) || exc.find(s[j]) < exc.length())))
        --j; // dec if space or splittable char
      while (j > i &&
             (va ? s[j - 1] != ' '
                 : isalnum(s[j - 1]) || exc.find(s[j - 1]) < exc.length()))
        --j;       // dec if prev is not space or splittable char
      if (j > i) { // got some
        std::cout << s.substr(i, std::min(j, js) - i) << '\n'
                  << std::string(start, ' ');
        while (j < je && s[j] == ' ')
          ++j;
        i = j;
      } else { // didn't get any
        std::cout << s.substr(i, js - i) << '\n' << std::string(start, ' ');
        i = j = js;
      }
    }
  }
  std::cout << s.substr(i);
}

struct isiless
    : std::binary_function<const std::string&, const std::string&, bool> {
  bool operator()(const std::string& x, const std::string& y) const {
    return boost::algorithm::ilexicographical_compare(x, y);
  }
};

void doinfo(const boost::program_options::variables_map& vm, const bool bigl,
            const bool littlel, const bool littlem) {
  if (bigl + littlel + littlem > 1) {
    CERR << "conflicting list types" << std::endl;
    throw err();
  }
  bool brf = vm.count("brief");
#ifdef HAVE_TERM_H
  const char* bold;
  const char* uline;
  const char* nobold;
  const char* nouline;
  if (vm.count("noattrs") || brf) {
    bold = 0;
    uline = 0;
    nobold = 0;
    nouline = 0;
  } else {
    setupterm(0, fileno(stdout), 0);
    bold = tigetstr("bold");
    if (bold == (char*) -1)
      bold = 0; // just in case
    nobold = tigetstr("sgr0");
    if (nobold == (char*) -1)
      nobold = 0;
    uline = tigetstr("smul"); // smacs
    if (uline == (char*) -1)
      uline = 0;
    nouline = tigetstr("rmul"); // rmacs
    if (nouline == (char*) -1)
      nouline = 0;
    if (nobold == 0)
      bold = 0;
    if (nouline == 0)
      uline = 0;
  }
#endif
  std::vector<fieldspec> vect_search;
  std::vector<fieldspec> vect_sort;
  std::vector<fieldspec> vect_filter;
  try {
    if (vm.count("search"))
      fill_fieldspecs(vect_search, vm["search"].as<std::vector<std::string>>());
  } catch (const fielderr& v) {
    CERR << "expected `:' or `=' in `--search' option" << std::endl;
    throw err();
  }
  if (vm.count("in")) {
    bool sdocs = vm.count("search-docs");
    bool snams = (!sdocs || vm.count("search-names"));
    const std::vector<std::string>& ids(
        vm["in"].as<std::vector<std::string>>());
    for (std::vector<std::string>::const_iterator i(ids.begin());
         i != ids.end(); ++i) {
      if (snams)
        vect_search.push_back(fieldspec("name", *i));
      if (sdocs)
        vect_search.push_back(fieldspec("doc", *i));
    }
  }
  try {
    if (vm.count("sort"))
      fill_fieldspecs(vect_sort, vm["sort"].as<std::vector<std::string>>());
  } catch (const fielderr& v) {
    CERR << "expected `:' or `=' in `--sort' option" << std::endl;
    throw err();
  }
  try {
    if (vm.count("filter"))
      fill_fieldspecs(vect_filter, vm["filter"].as<std::vector<std::string>>());
  } catch (const fielderr& v) {
    CERR << "expected `:' or `=' in `--filter' option" << std::endl;
    throw err();
  }
  if (littlel && vm.count("uselevel")) {
    int ul(vm["uselevel"].as<int>());
    if (ul < 0 || ul > 3) {
      CERR << "expected use level between 0 and 3" << std::endl;
      throw err();
    }
    std::ostringstream s;
    s << ul;
    vect_filter.push_back(fieldspec("uselevel", s.str()));
  }
  int lim;
  if (brf)
    lim = 0;
  else {
    if (vm.count("select")) {
      lim = vm["select"].as<int>();
      if (lim <= 0) {
        CERR << "invalid number of selections `" << lim << '\'' << std::endl;
        throw err();
      }
    } else {
      lim = vect_search.empty() ? 0 : 1;
    }
  }
  std::map<const std::string, info_sort, isiless> dirs;
  dirs.insert(std::map<const std::string, info_sort>::value_type(
      "asc", info_ascending));
  dirs.insert(std::map<const std::string, info_sort>::value_type(
      "desc", info_descending));
  if (bigl) {
    std::map<const std::string, modfilts, isiless> filts;
    filts.insert(std::map<const std::string, modfilts>::value_type(
        "name", modfilt_name));
    filts.insert(std::map<const std::string, modfilts>::value_type(
        "longname", modfilt_longname));
    filts.insert(std::map<const std::string, modfilts>::value_type(
        "author", modfilt_author));
    filts.insert(std::map<const std::string, modfilts>::value_type(
        "type", modfilt_type));
    std::map<const std::string, modsims, isiless> sims;
    sims.insert(
        std::map<const std::string, modsims>::value_type("name", modsim_name));
    sims.insert(std::map<const std::string, modsims>::value_type(
        "longname", modsim_longname));
    sims.insert(std::map<const std::string, modsims>::value_type(
        "author", modsim_author));
    sims.insert(
        std::map<const std::string, modsims>::value_type("doc", modsim_doc));
    std::map<const std::string, info_key, isiless> sorts;
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "name", info_modname));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "longname", info_modlongname));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "author", info_modauthor));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "type", info_modtype));

    info_modsearch sim = {0, 0, 0, 0};
    struct info_sortlist sort; // = {, };
    info_sortpair* x = sort.keys;
    info_modfilterlist filt; // = {, };
    // info_modfilter ky = {0, 0, 0, module_nomodtype, -1};
    if (!brf) {
      sort.n = vect_sort.size();
      sort.keys = new info_sortpair[vect_sort.size()];
      filt.n = vect_filter.size();
      filt.mods = new info_modfilter[vect_filter.size()];
      for (std::vector<fieldspec>::iterator i = vect_search.begin(),
                                            e = vect_search.end();
           i != e; ++i) {
        std::map<const std::string, modsims, isiless>::iterator s(
            sims.find(i->fi));
        if (s == sims.end()) {
          CERR << "invalid search key `" << i->fi << '\'' << std::endl;
          throw err();
        }
        switch (s->second) {
        case modsim_name:
          sim.name = i->va.c_str();
          break;
        case modsim_longname:
          sim.longname = i->va.c_str();
          break;
        case modsim_author:
          sim.author = i->va.c_str();
          break;
        case modsim_doc:
          sim.doc = i->va.c_str();
        }
      }
      for (std::vector<fieldspec>::iterator i = vect_sort.begin(),
                                            ie = vect_sort.end();
           i != ie; ++i, ++x) {
        std::map<const std::string, info_key, isiless>::iterator f(
            sorts.find(i->fi));
        if (f == sorts.end()) {
          CERR << "invalid sort key `" << i->fi << '\'' << std::endl;
          throw err();
        }
        x->key = f->second;
        std::map<const std::string, info_sort, isiless>::iterator d(
            dirs.find(i->va));
        if (d == dirs.end()) {
          CERR << "invalid sort direction `" << i->va << '\'' << std::endl;
          throw err();
        }
        x->sort = d->second;
      }
      {
        info_modfilter* x = filt.mods;
        for (std::vector<fieldspec>::iterator i = vect_filter.begin(),
                                              ie = vect_filter.end();
             i != ie; ++i, ++x) {
          x->name = 0;
          x->longname = 0;
          x->author = 0;
          x->type = module_nomodtype; // input, output, engine, notespelling...
          x->ifaceid = -1;
          std::map<const std::string, modfilts, isiless>::iterator f(
              filts.find(i->fi));
          if (f == filts.end()) {
            CERR << "invalid filter key `" << i->fi << '\'' << std::endl;
            throw err();
          }
          switch (f->second) {
          case modfilt_name:
            x->name = i->va.c_str();
            break;
          case modfilt_longname:
            x->longname = i->va.c_str();
            break;
          case modfilt_author:
            x->author = i->va.c_str();
            break;
          case modfilt_type: {
            x->type = info_str_to_modtype(i->va.c_str());
            if (x->type) {
              CERR << "invalid module type `" << i->va << '\'' << std::endl;
              throw err();
            }
          }
          }
        }
      }
    } else {
      sort.n = 0;
      sort.keys = 0;
      filt.n = 0;
      filt.mods = 0;
    }
    const struct info_modlist lst = info_list_modules(&filt, &sim, &sort, lim);
    CHECK_ERR;
    delete[] sort.keys;
    delete[] filt.mods;
    for (struct info_module *i = lst.mods, *e = lst.mods + lst.n; i != e; ++i) {
      if (brf) {
        std::cout << i->name << '\n';
      } else {
        std::cout << ULINE("name:") << ' ' << BOLD(i->name) << '\n'
                  << ULINE("long name:") << " \"" << i->longname << "\"  "
                  << ULINE("author:") << ' ' << i->author << "  "
                  << ULINE("type:") << ' ' << info_modtype_to_str(i->type)
                  << '\n'
                  << ULINE("directory:") << ' ';
        cout_justify(i->filename, 11);
        std::cout << '\n';
        cout_justify(i->doc);
        std::cout << "\n\n";
      }
    }
  } else if (littlel) {
    std::map<const std::string, setfilts, isiless> filts;
    filts.insert(std::map<const std::string, setfilts>::value_type(
        "modname", setfilt_modname));
    filts.insert(std::map<const std::string, setfilts>::value_type(
        "modlongname", setfilt_modlongname));
    filts.insert(std::map<const std::string, setfilts>::value_type(
        "modauthor", setfilt_modauthor));
    filts.insert(std::map<const std::string, setfilts>::value_type(
        "modtype", setfilt_modtype));
    filts.insert(std::map<const std::string, setfilts>::value_type(
        "name", setfilt_name));
    filts.insert(
        std::map<const std::string, setfilts>::value_type("loc", setfilt_loc));
    filts.insert(std::map<const std::string, setfilts>::value_type(
        "uselevel", setfilt_uselevel));
    // filts.insert(std::map<const std::string, setfilts>::value_type("where",
    // setfilt_where));
    std::map<const std::string, setsims, isiless> sims;
    sims.insert(std::map<const std::string, setsims>::value_type(
        "modname", setsim_modname));
    sims.insert(std::map<const std::string, setsims>::value_type(
        "modlongname", setsim_modlongname));
    sims.insert(std::map<const std::string, setsims>::value_type(
        "modauthor", setsim_modauthor));
    sims.insert(std::map<const std::string, setsims>::value_type(
        "moddoc", setsim_moddoc));
    sims.insert(
        std::map<const std::string, setsims>::value_type("name", setsim_name));
    sims.insert(
        std::map<const std::string, setsims>::value_type("doc", setsim_doc));
    std::map<const std::string, info_key, isiless> sorts;
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "modname", info_modname));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "modlongname", info_modlongname));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "modauthor", info_modauthor));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "modtype", info_modtype));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "name", info_setname));
    sorts.insert(
        std::map<const std::string, info_key>::value_type("loc", info_setloc));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "uselevel", info_setuselevel));
#ifndef NDEBUG
    {
      enum module_setting_loc xxx = module_noloc;
      switch (xxx) {
      case module_noloc:
      case module_locscore:
      case module_locpart:
      case module_locinst:
      case module_locpercinst:
      case module_locpartmap:
      case module_locmeasdef:
      case module_locimport:
      case module_locexport:
      case module_locstaff:
      case module_locclef:
      case module_locnote:;
      }
    }
#endif

    info_setsearch sim = {0, 0, 0, 0, 0, 0};
    struct info_sortlist sort; // = {, };
    info_sortpair* x = sort.keys;
    info_setfilterlist filt; // = {vect_filter.size(), };
    info_setfilter ky;
    if (!brf) {
      sort.n = vect_sort.size();
      sort.keys = new info_sortpair[vect_sort.size()];
      filt.n = vect_filter.size();
      filt.sets = new info_setfilter[vect_filter.size()];
      for (std::vector<fieldspec>::iterator i = vect_search.begin(),
                                            e = vect_search.end();
           i != e; ++i) {
        std::map<const std::string, setsims, isiless>::iterator s(
            sims.find(i->fi));
        if (s == sims.end()) {
          CERR << "invalid search key `" << i->fi << '\'' << std::endl;
          throw err();
        }
        switch (s->second) {
        case setsim_modname:
          sim.modname = i->va.c_str();
          break;
        case setsim_modlongname:
          sim.modlongname = i->va.c_str();
          break;
        case setsim_modauthor:
          sim.modauthor = i->va.c_str();
          break;
        case setsim_moddoc:
          sim.moddoc = i->va.c_str();
          break;
        case setsim_name:
          sim.name = i->va.c_str();
          break;
        case setsim_doc:
          sim.doc = i->va.c_str();
        }
      }
      for (std::vector<fieldspec>::iterator i = vect_sort.begin(),
                                            ie = vect_sort.end();
           i != ie; ++i, ++x) {
        std::map<const std::string, info_key, isiless>::iterator f(
            sorts.find(i->fi));
        if (f == sorts.end()) {
          CERR << "invalid sort key `" << i->fi << '\'' << std::endl;
          throw err();
        }
        x->key = f->second;
        std::map<const std::string, info_sort, isiless>::iterator d(
            dirs.find(i->va));
        if (d == dirs.end()) {
          CERR << "invalid sort direction `" << i->va << '\'' << std::endl;
          throw err();
        }
        x->sort = d->second;
      }
      {
        info_setfilter* x = filt.sets;
        for (std::vector<fieldspec>::iterator i = vect_filter.begin(),
                                              ie = vect_filter.end();
             i != ie; ++i, ++x) {
          x->modname = 0;
          x->modlongname = 0;
          x->modauthor = 0;
          x->modtype =
              module_nomodtype; // input, output, engine, notespelling...
          x->name = 0;
          x->loc = module_noloc;
          x->uselevel = -1;
          x->where = info_none;
          std::map<const std::string, setfilts, isiless>::iterator f(
              filts.find(i->fi));
          if (f == filts.end()) {
            CERR << "invalid filter key `" << i->fi << '\'' << std::endl;
            throw err();
          }
          switch (f->second) {
          case setfilt_modname:
            x->modname = i->va.c_str();
            break;
          case setfilt_modlongname:
            x->modlongname = i->va.c_str();
            break;
          case setfilt_modauthor:
            x->modauthor = i->va.c_str();
            break;
          case setfilt_modtype: {
            x->modtype = info_str_to_modtype(i->va.c_str());
            if (!x->modtype) {
              CERR << "invalid module type `" << i->va << '\'' << std::endl;
              throw err();
            }
          } break;
          case setfilt_name:
            x->name = i->va.c_str();
            break;
          case setfilt_loc: {
            x->loc = info_str_to_settingloc(i->va.c_str());
            if (!x->loc) {
              CERR << "invalid location `" << i->va << '\'' << std::endl;
              throw err();
            }
          } break;
          case setfilt_uselevel: {
            std::istringstream s(i->va);
            int v;
            try {
              s >> v;
            } catch (const std::istringstream::failure& e) {
              CERR << "invalid use level `" << i->va << '\'' << std::endl;
              throw err();
            }
            x->uselevel = v;
          }
          }
        }
      }
    } else {
      sort.n = 0;
      sort.keys = 0;
      filt.n = 1;
      ky.modname = 0;
      ky.modlongname = 0;
      ky.modauthor = 0;
      ky.modtype = module_nomodtype; // input, output, engine, notespelling...
      ky.name = 0;
      ky.loc = module_noloc;
      ky.uselevel = 3;
      ky.where = info_none;
      filt.sets = &ky;
    }
    const struct info_setlist lst =
        info_list_settings(0, &filt, &sim, &sort, lim);
    CHECK_ERR;
    delete[] sort.keys;
    if (!brf)
      delete[] filt.sets;
    for (struct info_setting *i = lst.sets, *e = lst.sets + lst.n; i != e;
         ++i) {
      if (brf) {
        std::cout << i->name << '\n';
      } else {
        std::cout << ULINE("name:") << ' ' << BOLD(i->name) << '\n'
                  << ULINE("module:") << ' '
                  << i->modname
                  //<< "  " <<
                  << "  " << ULINE("use level:") << ' ';
        switch (i->uselevel) {
        case 0:
          std::cout << "0 beginner";
          break;
        case 1:
          std::cout << "1 intermediate";
          break;
        case 2:
          std::cout << "2 advanced";
          break;
        case 3:
          std::cout << "3 guru";
        }
        std::cout << '\n'
                  << ULINE("location:") << ' '
                  << info_settingloc_to_extstr(i->loc);
        std::cout << '\n' << ULINE("type:") << ' ';
        cout_justify(i->typedoc, 6, true);
        std::cout << '\n' << ULINE("default value:") << ' ';
        cout_justify(i->valstr, 15, true);
        std::cout << '\n';
        cout_justify(i->descdoc);
        std::cout << "\n\n";
      }
    }
  }
  if (littlem) {
    std::map<const std::string, markfilts, isiless> filts;
    filts.insert(std::map<const std::string, markfilts>::value_type(
        "name", markfilt_name));
    filts.insert(std::map<const std::string, markfilts>::value_type(
        "modname", markfilt_modname));
    filts.insert(std::map<const std::string, markfilts>::value_type(
        "modlongname", markfilt_modlongname));
    filts.insert(std::map<const std::string, markfilts>::value_type(
        "modauthor", markfilt_modauthor));
    filts.insert(std::map<const std::string, markfilts>::value_type(
        "modtype", markfilt_modtype));
    std::map<const std::string, marksims, isiless> sims;
    sims.insert(std::map<const std::string, marksims>::value_type(
        "name", marksim_name));
    sims.insert(
        std::map<const std::string, marksims>::value_type("doc", marksim_doc));
    sims.insert(std::map<const std::string, marksims>::value_type(
        "modname", marksim_modname));
    sims.insert(std::map<const std::string, marksims>::value_type(
        "modlongname", marksim_modlongname));
    sims.insert(std::map<const std::string, marksims>::value_type(
        "modauthor", marksim_modauthor));
    sims.insert(std::map<const std::string, marksims>::value_type(
        "moddoc", marksim_moddoc));
    std::map<const std::string, info_key, isiless> sorts;
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "modname", info_modname));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "modlongname", info_modlongname));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "modauthor", info_modauthor));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "modtype", info_modtype));
    sorts.insert(std::map<const std::string, info_key>::value_type(
        "name", info_markname));
    sorts.insert(
        std::map<const std::string, info_key>::value_type("doc", info_markdoc));

    info_marksearch sim = {0, 0, 0, 0, 0, 0};
    struct info_sortlist sort; // = {, };
    info_sortpair* x = sort.keys;
    info_markfilterlist filt; // = {, };
    struct info_marklist lst;
    if (!brf) {
      sort.n = vect_sort.size();
      sort.keys = new info_sortpair[vect_sort.size()];
      filt.n = vect_filter.size();
      filt.marks = new info_markfilter[vect_filter.size()];
      for (std::vector<fieldspec>::iterator i = vect_search.begin(),
                                            e = vect_search.end();
           i != e; ++i) {
        std::map<const std::string, marksims, isiless>::iterator s(
            sims.find(i->fi));
        if (s == sims.end()) {
          CERR << "invalid search key `" << i->fi << '\'' << std::endl;
          throw err();
        }
        switch (s->second) {
        case marksim_modname:
          sim.modname = i->va.c_str();
          break;
        case marksim_modlongname:
          sim.modlongname = i->va.c_str();
          break;
        case marksim_modauthor:
          sim.modauthor = i->va.c_str();
          break;
        case marksim_moddoc:
          sim.moddoc = i->va.c_str();
          break;
        case marksim_name:
          sim.name = i->va.c_str();
          break;
        case marksim_doc:
          sim.doc = i->va.c_str();
        }
      }
      for (std::vector<fieldspec>::iterator i = vect_sort.begin(),
                                            ie = vect_sort.end();
           i != ie; ++i, ++x) {
        std::map<const std::string, info_key, isiless>::iterator f(
            sorts.find(i->fi));
        if (f == sorts.end()) {
          CERR << "invalid sort key `" << i->fi << '\'' << std::endl;
          throw err();
        }
        x->key = f->second;
        std::map<const std::string, info_sort, isiless>::iterator d(
            dirs.find(i->va));
        if (d == dirs.end()) {
          CERR << "invalid sort direction `" << i->va << '\'' << std::endl;
          throw err();
        }
        x->sort = d->second;
      }
      {
        info_markfilter* x = filt.marks;
        for (std::vector<fieldspec>::iterator i = vect_filter.begin(),
                                              ie = vect_filter.end();
             i != ie; ++i, ++x) {
          x->modname = 0;
          x->modlongname = 0;
          x->modauthor = 0;
          x->modtype =
              module_nomodtype; // input, output, engine, notespelling...
          x->name = 0;
          std::map<const std::string, markfilts, isiless>::iterator f(
              filts.find(i->fi));
          if (f == filts.end()) {
            CERR << "invalid filter key `" << i->fi << '\'' << std::endl;
            throw err();
          }
          switch (f->second) {
          case markfilt_modname:
            x->modname = i->va.c_str();
            break;
          case markfilt_modlongname:
            x->modlongname = i->va.c_str();
            break;
          case markfilt_modauthor:
            x->modauthor = i->va.c_str();
            break;
          case markfilt_modtype: {
            x->modtype = info_str_to_modtype(i->va.c_str());
            if (x->modtype) {
              CERR << "invalid module type `" << i->va << '\'' << std::endl;
              throw err();
            }
          } break;
          case markfilt_name:
            x->name = i->va.c_str();
          }
        }
      }
      lst = info_list_marks(0, &filt, &sim, &sort, lim);
    } else {
      lst = info_get_marks();
      sort.n = 0;
      sort.keys = 0;
      filt.n = 0;
      filt.marks = 0;
    }
    CHECK_ERR;
    delete[] sort.keys;
    delete[] filt.marks;
    for (struct info_mark *i = lst.marks, *e = lst.marks + lst.n; i != e; ++i) {
      if (brf) {
        std::cout << i->name << '\n';
      } else {
        std::cout << ULINE("name:") << ' ' << BOLD(i->name) << '\n'
                  << ULINE("module:") << ' ' << i->modname << '\n'
                  << ULINE("arguments:") << ' ';
        cout_justify(i->argsdoc, 6, true);
        std::cout << '\n';
        cout_justify(i->descdoc);
        std::cout << "\n\n";
      }
    }
  }
}

inline void eachfile(const std::string& ve, FOMUS fom, const int verb) {
  if (verb >= 0) {
    fomus_sval(fom, fomus_par_setting, fomus_act_set, "verbose");
    CHECK_ERR;
    fomus_ival(fom, fomus_par_settingval, fomus_act_set, verb);
    CHECK_ERR;
  }
  fomus_load(fom, ve.c_str());
  CHECK_ERR;
}
void dofile(const boost::program_options::variables_map& vm) {
  if (vm.count("in")) {
    FOMUS fom = fomus_new();
    CHECK_ERR;
    int verb;
    if (vm.count("quiet")) {
      verb = 0;
    } else if ((verb = vm.count("verbose")) != 0) {
      if (verb > 2)
        verb = 2;
    } else
      verb = -1;
    const std::vector<std::string>& ve(vm["in"].as<std::vector<std::string>>());
    if (vm.count("preset")) {
      fomus_sval(fom, fomus_par_setting, fomus_act_set, "presets");
      CHECK_ERR;
      fomus_act(fom, fomus_par_list, fomus_act_start);
      CHECK_ERR;
      const std::vector<std::string>& pr =
          vm["preset"].as<std::vector<std::string>>();
      for (std::vector<std::string>::const_iterator i(pr.begin());
           i != pr.end(); ++i) {
        fomus_sval(fom, fomus_par_list, fomus_act_add, i->c_str());
        CHECK_ERR;
      }
      fomus_act(fom, fomus_par_list, fomus_act_end);
      CHECK_ERR;
      fomus_act(fom, fomus_par_settingval, fomus_act_set);
      CHECK_ERR;
    }
    std::for_each(ve.begin(), ve.end(),
                  boost::lambda::bind(eachfile, boost::lambda::_1, fom,
                                      verb)); // load up each file
    if (verb >= 0) {                          // reset verbosity again
      fomus_sval(fom, fomus_par_setting, fomus_act_set, "verbose");
      CHECK_ERR;
      fomus_ival(fom, fomus_par_settingval, fomus_act_set, verb);
      CHECK_ERR;
    }
    if (vm.count("out")) {
      fomus_sval(fom, fomus_par_setting, fomus_act_set, "filename");
      CHECK_ERR;
      fomus_sval(fom, fomus_par_settingval, fomus_act_set,
                 vm["out"].as<std::string>().c_str());
      CHECK_ERR;
      fomus_sval(fom, fomus_par_setting, fomus_act_set,
                 "output"); // override file's output formats--expected behavior
                            // for a CMD line prog
      CHECK_ERR;
      fomus_act(fom, fomus_par_list,
                fomus_act_start); // 0-length list = let fomus figure it out
      CHECK_ERR;
      fomus_act(fom, fomus_par_list, fomus_act_end);
      CHECK_ERR;
      fomus_act(fom, fomus_par_settingval, fomus_act_set);
      CHECK_ERR;
    }
    //#warning "get rid of this--for testing only"
#ifndef NDEBUG
    FOMUS x = fomus_copy(fom);
    CHECK_ERR;
    fomus_run(x);
#else
    fomus_run(fom);
#endif
    CHECK_ERR;
  } else {
    CERR << "missing input filename" << std::endl;
    throw err();
  }
}

int main(int ac, char** av) {
  boost::program_options::variables_map vm;
  {
    boost::program_options::options_description gdesc("General Options",
                                                      CONSOLE_WIDTH);
    gdesc.add_options()("help,h", "Print help message")("version",
                                                        "Print version")(
        "verbose,v", "Verbose output (may be specified once or twice)")(
        "quiet,q", "Suppress output")(
        "brief", "Brief output (newline-delimited list suitable for other "
                 "programs to read and parse)")(
        "noattrs,t", "Turn off terminal attributes (bold and underline)");
    boost::program_options::options_description fdesc("File I/O Options",
                                                      CONSOLE_WIDTH);
    fdesc.add_options()(
        "in,i", boost::program_options::value<std::vector<std::string>>(),
        "Input filename (may be specified more than once)")(
        "out,o", boost::program_options::value<std::string>(),
        "Output filename (defaults to path/basename of input filename or value "
        "of `filename' setting)")
        //("format,f", value<vector<string> >(), "Output format (defaults to
        //value of `format' setting--may be specified more than once)") // multi
        //2-option

        ("preset,p", boost::program_options::value<std::vector<std::string>>(),
         "Apply a preset before inputting data (may be specified more than "
         "once)");
    boost::program_options::options_description ldesc("Search Options",
                                                      CONSOLE_WIDTH);
    ldesc.add_options()("list-modules,O", "List/search modules")(
        "list-settings,S", "List/search settings")("list-marks,M",
                                                   "List/search marks")

        ("search-docs,d",
         "Search documentation texts (may be used together with `-a')") // new
        ("search-names,a", "Search names (default behavior, may be used "
                           "together with `-d')") // new

        ("uselevel,u", boost::program_options::value<int>(),
         "Use level (for settings searches, ranges from 0 to 3)")(
            "select,n", boost::program_options::value<int>(),
            "Number of selections to print");
    boost::program_options::options_description idesc("Advanced Search Options",
                                                      CONSOLE_WIDTH);
    idesc.add_options()("search,s",
                        boost::program_options::value<
                            std::vector<std::string>>(),
                        "Search string (`-s KEY:STRING' or `-s KEY=STRING', "
                        "may be specified more than once)\n"
                        "Module keys = name, longname, author, doc\n"
                        "Setting keys = name, doc, modname, modlongname, "
                        "modauthor, moddoc\n"
                        "Mark keys = name, doc, modname, modlongname, "
                        "modauthor, moddoc") // multi 2-option!!!

        ("sort,r", boost::program_options::value<std::vector<std::string>>(),
         "Sort key (`-r KEY:asc|desc' or `-r KEY=asc|desc', may be specified "
         "more than once)\n"
         "Module keys = name, longname, author, type\n"
         "Setting keys = name, loc, uselevel, modname, modlongname, modauthor, "
         "modtype\n"
         "Mark keys = name, doc, modname, modlongname, modauthor, modtype") // multi 2-option

        ("filter,f", boost::program_options::value<std::vector<std::string>>(),
         "Selection filter (`-f KEY:VALUE' or `-f KEY=VALUE', may be specified "
         "more than once)\n"
         "Module keys = name, longname, author, type\n"
         "Setting keys = name, loc, uselevel, modname, modlongname, modauthor, "
         "modtype\n" // don't need where
         "Mark keys = name, modname, modlongname, modauthor, modtype");
    boost::program_options::options_description desc(CONSOLE_WIDTH); // gather
    desc.add(gdesc).add(fdesc).add(ldesc).add(idesc);
    boost::program_options::positional_options_description
        pos; // in and out files
    pos.add("in", -1);
    // pos.add("out", 1);
    try {
      boost::program_options::parsed_options os(
          boost::program_options::command_line_parser(ac, av)
              .options(desc)
              .positional(pos)
              .allow_unregistered()
              .run());
      std::vector<std::string> unr =
          boost::program_options::collect_unrecognized(
              os.options, boost::program_options::exclude_positional);
      if (!unr.empty()) {
        CERR << "unrecognized option `" << unr.front() << '\'' << std::endl;
        return EXIT_FAILURE;
      }
      boost::program_options::store(os, vm);
      // notify(vm);
    } catch (
        const boost::program_options::too_many_positional_options_error& e) {
      CERR << "too many options specified" << std::endl;
      return EXIT_FAILURE;
    } catch (const boost::program_options::error& e) {
      CERR << "error parsing command line arguments" << std::endl;
      return EXIT_FAILURE;
    }

    if (vm.count("help")) {
      std::cout << "Usage:\n  fomus [OPTION]... INFILE...\n  fomus -O|S|M "
                   "[OPTION]... [SEARCHTEXT]...\n";
      std::cout << desc;
      std::cout << "\nReport bugs to: <" << PACKAGE_BUGREPORT << '>'
                << std::endl;
      return EXIT_SUCCESS;
    }
    if (vm.count("version")) {
#if !defined(NDEBUGOUT)
#ifdef NDEBUGASS
      std::cout << PACKAGE_STRING
                << " (debug + output enabled, assertions disabled)"
                << std::endl;
#else
      std::cout << PACKAGE_STRING << " (debug + output enabled)" << std::endl;
#endif
#elif !defined(NDEBUG)
#ifdef NDEBUGASS
      std::cout << PACKAGE_STRING
                << " (compiled with debug flags, assertions disabled)"
                << std::endl;
#else
      std::cout << PACKAGE_STRING << " (debug enabled)" << std::endl;
#endif
#else
#ifdef NDEBUGASS
      std::cout << PACKAGE_STRING << " (assertions disabled)" << std::endl;
#else
      std::cout << PACKAGE_STRING << std::endl;
#endif
#endif
      return EXIT_SUCCESS;
    }
  }

  try {
    // while(true) {
    fomus_init();
    CHECK_ERR;

    bool bigl = vm.count("list-modules");
    bool littlel = vm.count("list-settings");
    bool littlem = vm.count("list-marks");
    if (bigl || littlel || littlem)
      doinfo(vm, bigl, littlel, littlem);
    else
      dofile(vm);
    //}
  } catch (const err& e) { return EXIT_FAILURE; }
  return EXIT_SUCCESS;
}
