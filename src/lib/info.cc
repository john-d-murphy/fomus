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

#include "heads.h"

#include "algext.h"   // for_each_reverse
#include "infoapi.h"  // info api stuff
#include "mods.h"     // modbase
#include "modtypes.h" // module types
#include "vars.h"     // varbase
//#include "fomusapi.h" // fomus_err variable
#include "data.h"  // fomusdata
#include "marks.h" // marks
#include "schedr.h"

namespace fomus {

  boost::filesystem::path userconfig;
  void inituserconfig() {
#if defined(__CYGWIN__)
    const char *c, *h, *hp, *hd;
    userconfig = (boost::filesystem::path(
                      (c = getenv("FOMUS_CONFIG_PATH")) != NULL
                          ? std::string(c)
                          : ((h = getenv("HOME")) != NULL
                                 ? std::string(h)
                                 : ((hp = getenv("HOMEPATH")) != NULL
                                        ? ((hd = getenv("HOMEDRIVE")) != NULL
                                               ? std::string(hd) + hp
                                               : std::string(hp))
                                        : ((hd = getenv("HOMEDRIVE")) != NULL
                                               ? std::string(hd)
                                               : std::string("\\"))))) /
                  ".fomus");
#elif defined(__MINGW32__)
    const char *c, *hp, *hd;
    userconfig = (boost::filesystem::path(
                      (c = getenv("FOMUS_CONFIG_PATH")) != NULL
                          ? std::string(c)
                          : ((hp = getenv("HOMEPATH")) != NULL
                                 ? ((hd = getenv("HOMEDRIVE")) != NULL
                                        ? std::string(hd) + hp
                                        : std::string(hp))
                                 : ((hd = getenv("HOMEDRIVE")) != NULL
                                        ? std::string(hd)
                                        : std::string("\\")))) /
                  ".fomus");
#else
    const char *c, *h;
    userconfig = (boost::filesystem::path(
                      (c = getenv("FOMUS_CONFIG_PATH")) != NULL
                          ? std::string(c)
                          : ((h = getenv("HOME")) != NULL ? std::string(h)
                                                          : std::string("/"))) /
                  ".fomus");
#endif
  }

  boost::filesystem::path fomusconfig;
  void initfomusconfig() {
    char* bp = getenv("FOMUS_BUILTIN_CONFIG_PATH");
#ifdef __MINGW32__
    if (bp)
      fomusconfig = bp;
    else {
      const char* wsys = getenv("WINDIR");
      fomusconfig = std::string(wsys ? wsys : "") + CMD_MACRO(CONFFILE_PATH);
    }
#else
    fomusconfig = (bp ? bp : CMD_MACRO(CONFFILE_PATH));
#endif
  }

  std::string::size_type stringmatch1(const std::string& x,
                                      const std::string& y, const bool sk1) {
    std::string::size_type sc = 0;
    for (std::string::const_iterator i(
             (sk1 && !x.empty()) ? boost::next(x.begin()) : x.begin());
         i < x.end() - sc; ++i) {
      std::string::size_type c = match_count(i, x.end(), y.begin(), y.end());
      if (c > sc)
        sc = c;
    }
    return sc;
  }

  double stringmatchmax(const std::vector<std::string>& v,
                        const std::string& s) { // s is user string
    if (s.empty())
      return 0;
    double sc = 0;
    for (std::vector<std::string>::const_iterator i(v.begin());
         i != v.end() && (i->length() / (double) s.length()) > sc; ++i) {
      assert(!i->empty());
      double c =
          std::max(stringmatch1(*i, s, false), stringmatch1(s, *i, true)) /
          (double) std::max(s.length(), i->length());
      assert(c <= 1);
      if (c > sc)
        sc = c;
    }
    return sc;
  }
  // comes up with a score that more or less equals the number of chars
  // matched--non-alphanum chars are separators
  double stringmatch(std::string x, std::string y, const bool isdoc = false,
                     const bool ismark = false) { // y is user's query
    std::vector<std::string> yy;
    boost::to_lower(y);
    if (ismark)
      boost::split(yy, y, boost::algorithm::is_space(),
                   boost::token_compress_on);
    else
      boost::split(yy, y, !boost::algorithm::is_alnum(),
                   boost::token_compress_on);
    std::vector<std::string> xx;
    boost::to_lower(x);
    if (ismark)
      boost::split(xx, x, boost::algorithm::is_space(),
                   boost::token_compress_on);
    else
      boost::split(xx, x, !boost::algorithm::is_alnum(),
                   boost::token_compress_on);
    if (yy.empty() || (!isdoc && xx.empty()))
      return 0;
    std::sort(xx.begin(), xx.end(),
              boost::lambda::bind(&std::string::length, boost::lambda::_1) >
                  boost::lambda::bind(&std::string::length, boost::lambda::_2));
    // if (isdoc) {
    //   return accumulate_results<double>(yy.begin(), yy.end(),
    //   boost::lambda::bind(stringmatchmax, boost::lambda::constant_ref(xx),
    //   boost::lambda::_1))
    // 	/ (double)yy.size();
    // } else {
    //   return accumulate_results<double>(yy.begin(), yy.end(),
    //   boost::lambda::bind(stringmatchmax, boost::lambda::constant_ref(xx),
    //   boost::lambda::_1))
    // 	/ (double)std::max(xx.size(), yy.size());
    // }
    return accumulate_results<double>(
               yy.begin(), yy.end(),
               boost::lambda::bind(stringmatchmax,
                                   boost::lambda::constant_ref(xx),
                                   boost::lambda::_1)) /
           (isdoc ? (double) yy.size()
                  : (double) std::max(xx.size(), yy.size()));
  }

  // class sorterr:public errbase {}; // throw when bad sort arg

  double modmatch(const struct info_modsearch& prox, const modbase& m) {
    double n = 0;
    if (prox.name)
      n += stringmatch(m.getsname(), prox.name);
    if (prox.longname)
      n += stringmatch(m.getlongname(), prox.longname);
    if (prox.author)
      n += stringmatch(m.getauthor(), prox.author);
    if (prox.doc)
      n += stringmatch(m.getdoc(), prox.doc, true);
    return n;
  }
  double setmatch(const struct info_setsearch& prox, const varbase& m) {
    double n = 0;
    if (prox.modname)
      n += stringmatch(m.getmodsname(), prox.modname);
    if (prox.modlongname)
      n += stringmatch(m.getmodlongname(), prox.modlongname);
    if (prox.modauthor)
      n += stringmatch(m.getmodauthor(), prox.modauthor);
    if (prox.moddoc)
      n += stringmatch(m.getmoddoc(), prox.moddoc, true);
    if (prox.name)
      n += stringmatch(m.getname(), prox.name);
    if (prox.doc)
      n += stringmatch(m.getdescdoc(), prox.doc, true);
    return n;
  }
  double markmatch(const struct info_marksearch& prox, const markbase& m) {
    double n = 0;
    if (prox.modname)
      n += stringmatch(m.getmodsname(), prox.modname);
    if (prox.modlongname)
      n += stringmatch(m.getmodlongname(), prox.modlongname);
    if (prox.modauthor)
      n += stringmatch(m.getmodauthor(), prox.modauthor);
    if (prox.moddoc)
      n += stringmatch(m.getmoddoc(), prox.moddoc, true);
    if (prox.name)
      n += stringmatch(m.getname(), prox.name, false, true);
    if (prox.doc)
      n += stringmatch(m.getdoc(), prox.doc, true);
    return n;
  }

  struct matchcont {
    const void* ptr;
    double sc;
    void setm(const struct info_modsearch& prox, const modbase* m) {
      sc = modmatch(prox, *m);
      ptr = m;
    }
    void sets(const struct info_setsearch& prox, const varbase* m) {
      sc = setmatch(prox, *m);
      ptr = m;
    }
    void seta(const struct info_marksearch& prox, const markbase* m) {
      sc = markmatch(prox, *m);
      ptr = m;
    }
    modbase* getm() const {
      return (modbase*) ptr;
    }
    varbase* gets() const {
      return (varbase*) ptr;
    }
    markbase* geta() const {
      return (markbase*) ptr;
    }
    double getsc() const {
      return sc;
    }
  };

  struct dosort {
    const struct info_sortpair& s;
    dosort(const struct info_sortpair& s) : s(s) {}
    dosort(const dosort& x) : s(x.s) {}
    bool operator()(const modbase* x, const modbase* y) const;
    bool operator()(const varbase* x, const varbase* y) const;
    bool operator()(const markbase* x, const markbase* y) const;
  };

  bool dosort::operator()(const modbase* x, const modbase* y) const {
    switch (s.sort) {
    case info_ascending:
      switch (s.key) {
      case info_modname:
        return x->getsname() < y->getsname();
      case info_modlongname:
        return std::string(x->getlongname()) < y->getlongname();
      case info_modauthor:
        return std::string(x->getauthor()) < y->getauthor();
      case info_modtype:
        return x->gettype() < y->gettype();
      default:
        return false;
      }
    case info_descending:
      switch (s.key) {
      case info_modname:
        return x->getsname() > y->getsname();
      case info_modlongname:
        return std::string(x->getlongname()) > y->getlongname();
      case info_modauthor:
        return std::string(x->getauthor()) > y->getauthor();
      case info_modtype:
        return x->gettype() > y->gettype();
      default:;
      }
    default:
      return false;
    }
  }

  bool dosort::operator()(const varbase* x, const varbase* y) const {
    switch (s.sort) {
    case info_ascending:
      switch (s.key) {
      case info_modname:
        return x->getmodsname() < y->getmodsname();
      case info_modlongname:
        return std::string(x->getmodlongname()) < y->getmodlongname();
      case info_modauthor:
        return std::string(x->getmodauthor()) < y->getmodauthor();
      case info_modtype:
        return x->getmodtype() < y->getmodtype();
      case info_setname:
        return std::string(x->getname()) < y->getname();
      case info_setloc:
        return x->getloc() < y->getloc();
      case info_setuselevel:
        return x->getuselevel() < y->getuselevel();
      default:
        return false;
      }
    case info_descending:
      switch (s.key) {
      case info_modname:
        return x->getmodsname() > y->getmodsname();
      case info_modlongname:
        return std::string(x->getmodlongname()) > y->getmodlongname();
      case info_modauthor:
        return std::string(x->getmodauthor()) > y->getmodauthor();
      case info_modtype:
        return x->getmodtype() > y->getmodtype();
      case info_setname:
        return std::string(x->getname()) > y->getname();
      case info_setloc:
        return x->getloc() > y->getloc();
      case info_setuselevel:
        return x->getuselevel() > y->getuselevel();
      default:;
      }
    default:
      return false;
    }
  }

  bool dosort::operator()(const markbase* x, const markbase* y) const {
    switch (s.sort) {
    case info_ascending:
      switch (s.key) {
      case info_modname:
        return x->getmodsname() < y->getmodsname();
      case info_modlongname:
        return std::string(x->getmodlongname()) < y->getmodlongname();
      case info_modauthor:
        return std::string(x->getmodauthor()) < y->getmodauthor();
      case info_modtype:
        return x->getmodtype() < y->getmodtype();
      case info_markname:
        return std::string(x->getname()) < y->getname();
      case info_markdoc:
        return x->getdoc() < y->getdoc();
      default:
        return false;
      }
    case info_descending:
      switch (s.key) {
      case info_modname:
        return x->getmodsname() > y->getmodsname();
      case info_modlongname:
        return std::string(x->getmodlongname()) > y->getmodlongname();
      case info_modauthor:
        return std::string(x->getmodauthor()) > y->getmodauthor();
      case info_modtype:
        return x->getmodtype() > y->getmodtype();
      case info_markname:
        return std::string(x->getname()) > y->getname();
      case info_markdoc:
        return x->getdoc() > y->getdoc();
      default:;
      }
    default:
      return false;
    }
  }

  // bool testx(const modbase* x, const modbase* y) {return true;}
  // TODO: CATCH EXCEPTION!
  inline bool ismodfiltmatch(const info_modfilter& f, const modbase& b) {
    return (f.name == NULL || std::string(f.name) == b.getsname()) &&
           (f.longname == NULL || std::string(f.longname) == b.getlongname()) &&
           (f.author == NULL || std::string(f.author) == b.getauthor()) &&
           (f.type == module_nomodtype || f.type == b.gettype()) &&
           (f.ifaceid == 0 || f.ifaceid == b.getifaceid());
  }
  inline bool nomodfiltmatches(const struct info_modfilterlist& filter,
                               const modbase& b) {
    return hasnone(filter.mods, filter.mods + filter.n,
                   boost::lambda::bind(ismodfiltmatch, boost::lambda::_1,
                                       boost::lambda::constant_ref(b)));
  }
  inline void makemodinfo(info_module& m, const modbase& b) {
    m.name = b.getcname();
    m.filename = b.getcfilename();
    m.longname = b.getlongname();
    m.author = b.getauthor();
    m.doc = b.getdoc();
    m.type = b.gettype();
    m.ifaceid = b.getifaceid();
  }

  inline bool issetfiltmatch(const info_setfilter& f, const varbase& b) {
    return (f.modname == NULL || std::string(f.modname) == b.getmodsname()) &&
           (f.modlongname == NULL ||
            std::string(f.modlongname) == b.getmodlongname()) &&
           (f.modauthor == NULL ||
            std::string(f.modauthor) == b.getmodauthor()) &&
           (f.modtype == module_nomodtype || f.modtype == b.getmodtype()) &&
           (f.name == NULL || std::string(f.name) == b.getname()) &&
           (f.loc == module_noloc || localallowed(f.loc, b.getloc())) &&
           (f.uselevel < 0 || f.uselevel >= b.getuselevel()) &&
           (f.where == info_none || b.getmodif() >= f.where);
  }
  inline bool nosetfiltmatches(const struct info_setfilterlist& filter,
                               const varbase& b) {
    return hasnone(filter.sets, filter.sets + filter.n,
                   boost::lambda::bind(issetfiltmatch, boost::lambda::_1,
                                       boost::lambda::constant_ref(b)));
  }

  inline bool ismarkfiltmatch(const info_markfilter& f, const markbase& b) {
    return (f.modname == NULL || std::string(f.modname) == b.getmodsname()) &&
           (f.modlongname == NULL ||
            std::string(f.modlongname) == b.getmodlongname()) &&
           (f.modauthor == NULL ||
            std::string(f.modauthor) == b.getmodauthor()) &&
           (f.modtype == module_nomodtype || f.modtype == b.getmodtype()) &&
           (f.name == NULL || std::string(f.name) == b.getname());
  }
  inline bool nomarkfiltmatches(const struct info_markfilterlist& filter,
                                const markbase& b) {
    return hasnone(filter.marks, filter.marks + filter.n,
                   boost::lambda::bind(ismarkfiltmatch, boost::lambda::_1,
                                       boost::lambda::constant_ref(b)));
  }

} // namespace fomus

// ------------------------------------------------------------------------------------------------------------------------
// END NAMESPACE

using namespace fomus;

const struct info_modlist info_list_modules(struct info_modfilterlist* filter,
                                            struct info_modsearch* prox,
                                            struct info_sortlist* sortlst,
                                            int limit) {
  ENTER_API;
  std::list<const modbase*> mds;
  copy_ptrs(mods.begin(), mods.end(), std::back_inserter(mds));
  if (filter && filter->n > 0) {
    mds.remove_if(bind(nomodfiltmatches, boost::lambda::constant_ref(*filter),
                       *boost::lambda::_1));
  }
  std::vector<info_sortpair> lst;
  if (sortlst)
    std::copy(sortlst->keys, sortlst->keys + sortlst->n,
              std::back_inserter(lst)); // push_back_into(lst, sortlst->keys,
                                        // sortlst->keys + sortlst->n);
  if (sortlst == 0 ||
      hasnone(sortlst->keys, sortlst->keys + sortlst->n,
              boost::lambda::bind(&info_sortpair::key, boost::lambda::_1) ==
                  info_modname)) {
    info_sortpair x = {info_modname, info_ascending};
    lst.push_back(x);
  }
  std::vector<const modbase*> mdsv(mds.begin(), mds.end());
  for (std::vector<info_sortpair>::reverse_iterator i(lst.rbegin());
       i != lst.rend(); ++i)
    std::stable_sort(mdsv.begin(), mdsv.end(), dosort(*i));
  if (prox != NULL) {
    std::vector<matchcont> cnts(mdsv.size());
    for_each2(mdsv.begin(), mdsv.end(), cnts.begin(),
              boost::lambda::bind(&matchcont::setm, boost::lambda::_2,
                                  boost::lambda::constant_ref(*prox),
                                  boost::lambda::_1));
    std::stable_sort(
        cnts.begin(), cnts.end(),
        boost::lambda::bind(&matchcont::getsc, boost::lambda::_1) >
            boost::lambda::bind(&matchcont::getsc, boost::lambda::_2));
    std::transform(cnts.begin(), cnts.end(), mdsv.begin(),
                   boost::lambda::bind(&matchcont::getm, boost::lambda::_1));
  }
  static struct info_modlist rt = {0, 0}; // STATIC!
  if (limit >= 1)
    rt.n = std::min(limit, (int) mdsv.size());
  else
    rt.n = mdsv.size();
  static std::vector<info_module> arr;
  arr.resize(rt.n);
  for_each2(
      arr.begin(), arr.end(), mdsv.begin(),
      boost::lambda::bind(makemodinfo, boost::lambda::_1, *boost::lambda::_2));
  rt.mods = &arr[0];
  return rt;
  EXIT_API_FMODLIST0;
}

const struct info_setlist info_list_settings(FOMUS fom,
                                             struct info_setfilterlist* filter,
                                             struct info_setsearch* prox,
                                             struct info_sortlist* sortlst,
                                             int limit) {
  ENTER_API;
  assert(!fom || ((fomusdata*) fom)->isvalid());
  std::list<varbase*> vas;
  if (fom) {
    for (varcopiesvect_constit i(((fomusdata*) fom)->getinvars().begin()),
         ie(((fomusdata*) fom)->getinvars().end());
         i != ie; ++i) {
      vas.push_back(i->get());
    }
  } else
    std::transform(
        vars.begin(), vars.end(), std::back_inserter(vas),
        boost::lambda::bind(&boost::shared_ptr<varbase>::get,
                            boost::lambda::_1)); // push_back_ptrs(vas,
                                                 // vars.begin(), vars.end());
  if (filter == 0 || hasnone(filter->sets, filter->sets + filter->n,
                             boost::lambda::bind(&info_setfilter::uselevel,
                                                 boost::lambda::_1) >= 0)) {
    fint lv = getdefaultival(USELEVEL_ID);
    vas.remove_if(
        boost::lambda::bind(&varbase::getuselevel, boost::lambda::_1) > lv);
  }
  if (filter && filter->n > 0) {
    vas.remove_if(boost::lambda::bind(nosetfiltmatches,
                                      boost::lambda::constant_ref(*filter),
                                      *boost::lambda::_1));
  }
  std::vector<info_sortpair> lst;
  if (sortlst)
    std::copy(sortlst->keys, sortlst->keys + sortlst->n,
              std::back_inserter(lst)); // push_back_into(lst, sortlst->keys,
                                        // sortlst->keys + sortlst->n);
  if (sortlst == 0 ||
      hasnone(sortlst->keys, sortlst->keys + sortlst->n,
              boost::lambda::bind(&info_sortpair::key, boost::lambda::_1) ==
                  info_setname)) {
    info_sortpair x = {info_setname, info_ascending};
    lst.push_back(x);
  }
  std::vector<varbase*> vasv(vas.begin(), vas.end());
  for (std::vector<info_sortpair>::reverse_iterator i(lst.rbegin());
       i != lst.rend(); ++i)
    std::stable_sort(vasv.begin(), vasv.end(), dosort(*i));
  if (prox != NULL) {
    std::vector<matchcont> cnts(vasv.size());
    for_each2(vasv.begin(), vasv.end(), cnts.begin(),
              boost::lambda::bind(&matchcont::sets, boost::lambda::_2,
                                  boost::lambda::constant_ref(*prox),
                                  boost::lambda::_1));
    std::stable_sort(
        cnts.begin(), cnts.end(),
        boost::lambda::bind(&matchcont::getsc, boost::lambda::_1) >
            boost::lambda::bind(&matchcont::getsc, boost::lambda::_2));
    std::transform(cnts.begin(), cnts.end(), vasv.begin(),
                   boost::lambda::bind(&matchcont::gets, boost::lambda::_1));
  }
  struct scoped_info_setlist& rt =
      fom ? ((fomusdata*) fom)->getinfosetlist() : globsetlist;
  rt.resize((limit >= 1) ? std::min(limit, (int) vasv.size()) : vasv.size());
  for_each2(rt.sets, rt.sets + rt.n, vasv.begin(),
            boost::lambda::bind(&fomusdata::get_settinginfo, (fomusdata*) fom,
                                boost::lambda::_1, *boost::lambda::_2));
  return rt;
  EXIT_API_SETLIST0;
}

const struct info_marklist info_list_marks(FOMUS fom,
                                           struct info_markfilterlist* filter,
                                           struct info_marksearch* prox,
                                           struct info_sortlist* sortlst,
                                           int limit) {
  ENTER_API;
  assert(!fom || ((fomusdata*) fom)->isvalid());
  std::list<markbase*> mks;
  int cnt = 0;
  for (marksvect_it i(markdefs.begin()); cnt < mark_nmarks; ++i, ++cnt)
    mks.push_back(&*i);
  // std::copy(markdocs.begin(), markdocs.end(), std::back_inserter(mks));
  // //push_back_ptrs(vas, vars.begin(), vars.end());
  if (filter && filter->n > 0) {
    mks.remove_if(boost::lambda::bind(nomarkfiltmatches,
                                      boost::lambda::constant_ref(*filter),
                                      *boost::lambda::_1));
  }
  std::vector<info_sortpair> lst;
  if (sortlst)
    std::copy(sortlst->keys, sortlst->keys + sortlst->n,
              std::back_inserter(lst)); // push_back_into(lst, sortlst->keys,
                                        // sortlst->keys + sortlst->n);
  if (sortlst == 0 ||
      hasnone(sortlst->keys, sortlst->keys + sortlst->n,
              boost::lambda::bind(&info_sortpair::key, boost::lambda::_1) ==
                  info_markname)) {
    info_sortpair x = {info_markname, info_ascending};
    lst.push_back(x);
  }
  std::vector<markbase*> mksv(mks.begin(), mks.end());
  for (std::vector<info_sortpair>::reverse_iterator i(lst.rbegin());
       i != lst.rend(); ++i)
    std::stable_sort(mksv.begin(), mksv.end(), dosort(*i));
  if (prox != NULL) {
    std::vector<matchcont> cnts(mksv.size());
    for_each2(mksv.begin(), mksv.end(), cnts.begin(),
              boost::lambda::bind(&matchcont::seta, boost::lambda::_2,
                                  boost::lambda::constant_ref(*prox),
                                  boost::lambda::_1));
    std::stable_sort(
        cnts.begin(), cnts.end(),
        boost::lambda::bind(&matchcont::getsc, boost::lambda::_1) >
            boost::lambda::bind(&matchcont::getsc, boost::lambda::_2));
    std::transform(cnts.begin(), cnts.end(), mksv.begin(),
                   boost::lambda::bind(&matchcont::geta, boost::lambda::_1));
  }
  marklist.resize((limit >= 1) ? std::min(limit, (int) mksv.size())
                               : mksv.size());
  for_each2(
      marklist.marks, marklist.marks + marklist.n, mksv.begin(),
      boost::lambda::bind(get_markinfo, boost::lambda::_1, *boost::lambda::_2));
  return marklist;
  EXIT_API_MARKLIST0;
}

const char* info_get_userconfigfile() {
  ENTER_API;
  return userconfig.FS_FILE_STRING().c_str();
  EXIT_API_0;
}

const char* info_get_fomusconfigfile() {
  ENTER_API;
  return fomusconfig.FS_FILE_STRING().c_str();
  EXIT_API_0;
}

const char* info_modtype_to_str(enum module_type type) {
  ENTER_API;
  return modtypetostr(type)
      .c_str(); // assuming this is in range, since this is an enum
  EXIT_API_0;
}
enum module_type info_str_to_modtype(const char* str) {
  ENTER_API;
  try {
    return strtomodtype(str);
  } catch (const badstr& e) { return module_nomodtype; }
  EXIT_API_0MODTYPE;
}

const char* info_settingloc_to_str(enum module_setting_loc loc) {
  ENTER_API;
  return setloctostr(loc).c_str();
  EXIT_API_0;
}
const char* info_settingloc_to_extstr(enum module_setting_loc loc) {
  ENTER_API;
  static std::string x;
  x = setloctostrex(loc);
  return x.c_str();
  EXIT_API_0;
}
enum module_setting_loc info_str_to_settingloc(const char* str) {
  ENTER_API;
  try {
    return strtosetloc(str);
  } catch (const badstr& e) { return module_noloc; }
  EXIT_API_0MODSETLOC;
}

const struct info_extslist info_list_exts() {
  ENTER_API;
  static std::vector<const char*> v;
  if (v.empty()) {
    std::for_each(mods.begin(), mods.end(),
                  boost::lambda::bind(&modbase::modout_addext,
                                      boost::lambda::_1,
                                      boost::lambda::var(v)));
  }
  info_extslist r = {v.size(), &v[0]};
  return r;
  EXIT_API_0EXTSLIST;
}

int info_infoapi_version() {
  return FOMUS_INFOAPI_VERSION;
}
