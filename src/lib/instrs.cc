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

#include "instrs.h"
#include "data.h" // fomusdata
#include "module.h"
#include "parse.h"

namespace fomus {

  template <typename I, typename F>
  inline void for_each_pr(std::ostream& ou, I first, const I& last,
                          const F& fun) {
    if (first == last)
      return;
    while (true) {
      fun(*first++);
      if (first != last)
        ou << ' ';
      else
        break;
    }
  }

  void copymodval(const struct module_value& from, struct module_value& to) {
    to.type = from.type;
    if (from.type >= module_list) {
      to.val.l.n = from.val.l.n;
      if (from.val.l.vals && to.val.l.n > 0) {
        to.val.l.vals =
            newmodvals(from.val.l.n); // new module_value[from.val.l.n];
        for (module_value *i = from.val.l.vals,
                          *ie = from.val.l.vals + from.val.l.n,
                          *j = to.val.l.vals;
             i < ie; ++i, ++j)
          copymodval(*i, *j);
      } else
        to.val.l.vals = 0;
    } else
      to.val = from.val;
  }

  const instr_str& getaglobinstr(const std::string& id, const filepos& pos) {
    const globinstsmap& xx =
        ((const instrs_var*) vars[INSTRS_ID].get())->getmap();
    globinstsmap_constit i(xx.find(id));
    if (i == xx.end()) {
      CERR << "instrument `" << id << "' doesn't exist";
      pos.printerr();
      throw errbase();
    }
    return *i->second;
  }
  const percinstr_str& getaglobpercinstr(const std::string& id,
                                         const filepos& pos) {
    const globpercsmap& xx =
        ((const percinstrs_var*) vars[PERCINSTRS_ID].get())->getmap();
    globpercsmap_constit i(xx.find(id));
    if (i == xx.end()) {
      CERR << "percussion instrument `" << id << "' doesn't exist";
      pos.printerr();
      throw errbase();
    }
    return *i->second;
  }

  inline void comma(std::ostream& s, bool& sm) {
    if (sm)
      s << ", ";
    else
      sm = true;
  }

  void
  str_base::getmodvals(module_value* x) const { // array must be 2x sets.size
    for (setmap_constit i(sets.begin()); i != sets.end(); ++i) {
      x->type = module_string;
      x->val.s = i->second->getname();
      ++x;
      copymodval(i->second->getmodval(), *x); // must be destroyed, so copy it
      ++x;
    }
  }
  void str_base::printsets(std::ostream& s, const fomusdata* fd,
                           bool& sm) const {
    if (!sets.empty()) {
      comma(s, sm);
      s << stringify(sets.begin()->second->getname(), ":=,") << ' '
        << sets.begin()->second->getvalstr(
               fd, ")>,"); // : el.begin()->first) << ' ' << el.begin()->second;
      for (setmap_constit i(std::next(sets.begin())); i != sets.end(); ++i) {
        s << ", " << stringify(i->second->getname(), ":=,") << ' '
          << i->second->getvalstr(fd, ")>,");
      }
    }
  }
  void str_base::print(std::ostream& s, const fomusdata* fd) const {
    s << '<';
    bool sm = false;
    printsets(s, fd, sm);
    s << '>';
  }

  void staff_str::getmodval(module_value& x) const {
    int xx;
    if (!clefs.empty())
      xx = 2;
    else
      xx = 0;
    x.type = module_list;
    int ss = sets.size() * 2;
    x.val.l.vals = newmodvals(
        x.val.l.n = ss + xx); // new module_value[x.val.l.n = ss + xx];
    getmodvals(x.val.l.vals);
    xx = ss;
    if (!clefs.empty()) {
      module_value& y = x.val.l.vals[xx++];
      y.type = module_string;
      y.val.s = "clefs";
      module_value& z = x.val.l.vals[xx];
      z.type = module_list;
      z.val.l.vals = newmodvals(
          z.val.l.n =
              clefs.size()); // new module_value[z.val.l.n = clefs.size()];
      for_each2(clefs.begin(), clefs.end(), z.val.l.vals,
                boost::lambda::bind(
                    &clef_str::getmodval,
                    boost::lambda::bind(&boost::shared_ptr<clef_str>::get,
                                        boost::lambda::_1),
                    boost::lambda::_2));
    }
  }
  void staff_str::print(std::ostream& s, const fomusdata* fd) const {
    s << '<';
    bool sm = false;
    printsets(s, fd, sm);
    if (!clefs.empty()) {
      comma(s, sm);
      s << "clefs ";
      if (clefs.size() > 1)
        s << '(';
      for_each_pr(s, clefs.begin(), clefs.end(),
                  boost::lambda::bind(
                      &clef_str::print,
                      boost::lambda::bind(&boost::shared_ptr<clef_str>::get,
                                          boost::lambda::_1),
                      boost::lambda::var(s), boost::lambda::constant_ref(fd)));
      if (clefs.size() > 1)
        s << ')';
    }
    s << '>';
  }

  void percinstr_str::getmodval(module_value& x, const bool justid) const {
    if (justid && !id.empty()) {
      x.type = module_string;
      x.val.s = id.c_str();
    } else {
      int xx;
      if (!id.empty())
        xx = 2;
      else
        xx = 0;
      if (!ims.empty())
        xx += 2;
      if (ex.get() != 0)
        xx += 2;
      x.type = module_list;
      int ss = sets.size() * 2;
      x.val.l.vals = newmodvals(x.val.l.n = ss + xx);
      xx = 0;
      if (!id.empty()) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "id";
        module_value& z = x.val.l.vals[xx++];
        z.type = module_string;
        z.val.s = id.c_str();
      }
      getmodvals(x.val.l.vals + xx);
      xx += ss;
      if (!ims.empty()) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "imports";
        module_value& z = x.val.l.vals[xx++];
        z.type = module_list;
        z.val.l.vals = newmodvals(z.val.l.n = ims.size());
        for_each2(ims.begin(), ims.end(), z.val.l.vals,
                  boost::lambda::bind(
                      &import_str::getmodval,
                      boost::lambda::bind(&boost::shared_ptr<import_str>::get,
                                          boost::lambda::_1),
                      boost::lambda::_2));
      }
      if (ex.get() != 0) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "export";
        ex->getmodval(x.val.l.vals[xx]);
      }
    }
  }
  void percinstr_str::print(std::ostream& s, const fomusdata* fd,
                            const bool justid) const {
    if (justid && !id.empty()) {
      s << stringify(id, ")>,");
    } else {
      s << '<';
      bool sm = false;
      if (!id.empty()) {
        comma(s, sm);
        s << "id " << stringify(id, ")>,");
      }
      printsets(s, fd, sm);
      if (!ims.empty()) {
        comma(s, sm);
        s << "imports ";
        if (ims.size() > 1)
          s << '(';
        for_each_pr(s, ims.begin(), ims.end(),
                    boost::lambda::bind(
                        &import_str::print,
                        boost::lambda::bind(&boost::shared_ptr<import_str>::get,
                                            boost::lambda::_1),
                        boost::lambda::var(s),
                        boost::lambda::constant_ref(fd)));
        if (ims.size() > 1)
          s << ')';
      }
      if (ex.get() != 0) {
        comma(s, sm);
        s << "export ";
        ex->print(s, fd);
      }
      s << '>';
    }
  }

  struct percinstr_getmodval : public boost::static_visitor<void> {
    module_value& yy;
    percinstr_getmodval(module_value& yy) : yy(yy) {}
    void operator()(const boost::shared_ptr<percinstr_str>& x) const {
      x->getmodval(yy, true);
    }
    void operator()(const std::string& x) const {
      assert(false);
    }
  };
  void instr_str::getmodval(module_value& x, const bool justid) const {
    if (justid && !id.empty()) {
      x.type = module_string;
      x.val.s = id.c_str();
    } else {
      int xx;
      if (!id.empty())
        xx = 2;
      else
        xx = 0;
      if (!staves.empty())
        xx += 2;
      if (!ims.empty())
        xx += 2;
      if (ex.get() != 0)
        xx += 2;
      if (!percs.empty())
        xx += 2;
      x.type = module_list;
      int ss = sets.size() * 2;
      x.val.l.vals = newmodvals(x.val.l.n = ss + xx);
      xx = 0;
      if (!id.empty()) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "id";
        module_value& z = x.val.l.vals[xx++];
        z.type = module_string;
        z.val.s = id.c_str();
      }
      getmodvals(x.val.l.vals + xx);
      xx += ss;
      if (!staves.empty()) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "staves";
        module_value& z = x.val.l.vals[xx++];
        z.type = module_list;
        z.val.l.vals = newmodvals(z.val.l.n = staves.size());
        for_each2(staves.begin(), staves.end(), z.val.l.vals,
                  boost::lambda::bind(
                      &staff_str::getmodval,
                      boost::lambda::bind(&boost::shared_ptr<staff_str>::get,
                                          boost::lambda::_1),
                      boost::lambda::_2));
      }
      if (!ims.empty()) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "imports";
        module_value& z = x.val.l.vals[xx++];
        z.type = module_list;
        z.val.l.vals = newmodvals(z.val.l.n = ims.size());
        for_each2(ims.begin(), ims.end(), z.val.l.vals,
                  boost::lambda::bind(
                      &import_str::getmodval,
                      boost::lambda::bind(&boost::shared_ptr<import_str>::get,
                                          boost::lambda::_1),
                      boost::lambda::_2));
      }
      if (ex.get() != 0) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "export";
        ex->getmodval(x.val.l.vals[xx++]);
      }
      if (!percs.empty()) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "percinsts";
        module_value& z = x.val.l.vals[xx]; // last one no inc
        z.type = module_list;
        module_value* yy = z.val.l.vals = newmodvals(z.val.l.n = percs.size());
        for (std::vector<boost::variant<boost::shared_ptr<percinstr_str>,
                                        std::string>>::const_iterator
                 i(percs.begin());
             i != percs.end(); ++i, ++yy) {
          apply_visitor(percinstr_getmodval(*yy), *i);
        }
      }
    }
  }
  struct percinstr_print : public boost::static_visitor<void> {
    std::ostream& s;
    const fomusdata* fd;
    percinstr_print(std::ostream& s, const fomusdata* fd) : s(s), fd(fd) {}
    void operator()(const boost::shared_ptr<percinstr_str>& x) const {
      x->print(s, fd, true);
    }
    void operator()(const std::string& x) const {
      assert(false);
    }
  };
  void instr_str::print(std::ostream& s, const fomusdata* fd,
                        const bool justid) const {
    if (justid && !id.empty()) {
      s << stringify(id, ")>,");
    } else {
      s << '<';
      bool sm = false;
      if (!id.empty()) {
        comma(s, sm);
        s << "id " << stringify(id, ")>,");
      }
      printsets(s, fd, sm);
      if (!staves.empty()) {
        comma(s, sm);
        s << "staves ";
        if (staves.size() > 1)
          s << '(';
        for_each_pr(s, staves.begin(), staves.end(),
                    boost::lambda::bind(
                        &staff_str::print,
                        boost::lambda::bind(&boost::shared_ptr<staff_str>::get,
                                            boost::lambda::_1),
                        boost::lambda::var(s),
                        boost::lambda::constant_ref(fd)));
        if (staves.size() > 1)
          s << ')';
      }
      if (!ims.empty()) {
        comma(s, sm);
        s << "imports ";
        if (ims.size() > 1)
          s << '(';
        for_each_pr(s, ims.begin(), ims.end(),
                    boost::lambda::bind(
                        &import_str::print,
                        boost::lambda::bind(&boost::shared_ptr<import_str>::get,
                                            boost::lambda::_1),
                        boost::lambda::var(s),
                        boost::lambda::constant_ref(fd)));
        if (ims.size() > 1)
          s << ')';
      }
      if (ex.get() != 0) {
        comma(s, sm);
        s << "export ";
        ex->print(s, fd);
      }
      if (!percs.empty()) {
        comma(s, sm);
        s << "percinsts ";
        if (percs.size() > 1)
          s << '(';
        for (std::vector<boost::variant<boost::shared_ptr<percinstr_str>,
                                        std::string>>::const_iterator
                 i(percs.begin());
             i != percs.end(); ++i) {
          if (i != percs.begin())
            s << ' ';
          boost::apply_visitor(percinstr_print(s, fd), *i);
        }
        if (percs.size() > 1)
          s << ')';
      }
      s << '>';
    }
  }

  struct instr_isfull : public boost::static_visitor<bool> {
    bool operator()(const boost::shared_ptr<instr_str>& x) const {
      return x.get();
    }
    bool operator()(const std::string& x) const {
      assert(false);
    }
  };
  struct instr_getmodval : public boost::static_visitor<void> {
    module_value& val;
    instr_getmodval(module_value& val) : val(val) {}
    void operator()(const boost::shared_ptr<instr_str>& x) const {
      x->getmodval(val, true);
    }
    void operator()(const std::string& x) const {
      assert(false);
    }
  };
  void part_str::getmodval(module_value& x, const bool justid) const {
    if (justid && !id.empty()) {
      x.type = module_string;
      x.val.s = id.c_str();
    } else {
      int xx;
      if (!id.empty())
        xx = 2;
      else
        xx = 0;
      if (apply_visitor(instr_isfull(), instr))
        xx += 2;
      x.type = module_list;
      int ss = sets.size() * 2;
      x.val.l.vals = newmodvals(x.val.l.n = ss + xx);
      xx = 0;
      if (!id.empty()) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "id";
        module_value& z = x.val.l.vals[xx++];
        z.type = module_string;
        z.val.s = id.c_str();
      }
      getmodvals(x.val.l.vals + xx);
      xx += ss;
      if (apply_visitor(instr_isfull(), instr)) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "inst";
        apply_visitor(instr_getmodval(x.val.l.vals[xx]), instr);
      }
    }
  }
  struct instr_print : public boost::static_visitor<void> {
    std::ostream& s;
    const fomusdata* fd;
    instr_print(std::ostream& s, const fomusdata* fd) : s(s), fd(fd) {}
    void operator()(const boost::shared_ptr<instr_str>& x) const {
      x->print(s, fd, true);
    }
    void operator()(const std::string& x) const {
      assert(false);
    }
  };
  void part_str::print(std::ostream& s, const fomusdata* fd,
                       const bool justid) const {
    if (justid && !id.empty()) {
      s << stringify(id, ")>,");
    } else {
      s << '<';
      bool sm = false;
      if (!id.empty()) {
        comma(s, sm);
        s << "id " << stringify(id, ")>,");
      }
      printsets(s, fd, sm);
      if (apply_visitor(instr_isfull(), instr)) {
        comma(s, sm);
        s << "inst ";
        apply_visitor(instr_print(s, fd), instr);
      }
      s << '>';
    }
  }

  struct part_print : public boost::static_visitor<void> {
    std::ostream& s;
    const fomusdata* fd;
    part_print(std::ostream& s, const fomusdata* fd) : s(s), fd(fd) {}
    void operator()(const boost::shared_ptr<part_str>& x) const {
      s << "part ";
      x->print(s, fd, true);
    }
    void operator()(const boost::shared_ptr<mpart_str>& x) const {
      s << "metapart ";
      x->print(s, fd, true);
    }
    void operator()(const std::string& x) const {
      assert(false);
    }
  };
  struct part_getmodval : public boost::static_visitor<void> {
    module_value& val;
    const char*& what;
    part_getmodval(module_value& val, const char*& what)
        : val(val), what(what) {}
    void operator()(const boost::shared_ptr<part_str>& x) const {
      what = "part";
      x->getmodval(val, true);
    }
    void operator()(const boost::shared_ptr<mpart_str>& x) const {
      what = "metapart";
      x->getmodval(val, true);
    }
    void operator()(const std::string& x) const {
      assert(false);
    }
  };
  void partmap_str::getmodval(module_value& x) const {
    x.type = module_list;
    int ss = sets.size() * 2;
    x.val.l.vals = newmodvals(x.val.l.n = ss + 2);
    getmodvals(x.val.l.vals);
    module_value& y = x.val.l.vals[ss /*- 2*/];
    y.type = module_string;
    apply_visitor(part_getmodval(x.val.l.vals[ss /*-*/ + 1], y.val.s), part);
  }
  void partmap_str::print(std::ostream& s, const fomusdata* fd) const {
    s << '<';
    bool sm = false;
    printsets(s, fd, sm);
    comma(s, sm);
    apply_visitor(part_print(s, fd), part);
    s << '>';
  }

  void mpart_str::getmodval(module_value& x, const bool justid) const {
    if (justid && !id.empty()) {
      x.type = module_string;
      x.val.s = id.c_str();
    } else {
      int xx;
      if (!id.empty())
        xx = 2;
      else
        xx = 0;
      if (!parts.empty())
        xx += 2;
      x.type = module_list;
      int ss = sets.size() * 2;
      x.val.l.vals = newmodvals(x.val.l.n = ss + xx);
      xx = 0;
      if (!id.empty()) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "id";
        module_value& z = x.val.l.vals[xx++];
        z.type = module_string;
        z.val.s = id.c_str();
      }
      getmodvals(x.val.l.vals + xx);
      xx += ss;
      if (!parts.empty()) {
        module_value& y = x.val.l.vals[xx++];
        y.type = module_string;
        y.val.s = "parts";
        module_value& z = x.val.l.vals[xx];
        z.type = module_list;
        z.val.l.vals = newmodvals(z.val.l.n = parts.size());
        for_each2(parts.begin(), parts.end(), z.val.l.vals,
                  boost::lambda::bind(
                      &partmap_str::getmodval,
                      boost::lambda::bind(&boost::shared_ptr<partmap_str>::get,
                                          boost::lambda::_1),
                      boost::lambda::_2));
      }
    }
  }
  void mpart_str::print(std::ostream& s, const fomusdata* fd,
                        const bool justid) const {
    if (justid && !id.empty()) {
      s << stringify(id, ")>,");
    } else {
      s << '<';
      bool sm = false;
      if (!id.empty()) {
        comma(s, sm);
        s << "id " << stringify(id, ")>,");
      }
      printsets(s, fd, sm);
      if (!parts.empty()) {
        comma(s, sm);
        s << "parts ";
        if (parts.size() > 1)
          s << '(';
        for_each_pr(
            s, parts.begin(), parts.end(),
            boost::lambda::bind(
                &partmap_str::print,
                boost::lambda::bind(&boost::shared_ptr<partmap_str>::get,
                                    boost::lambda::_1),
                boost::lambda::var(s), boost::lambda::constant_ref(fd)));
        if (parts.size() > 1)
          s << ')';
      }
      s << '>';
    }
  }

  void measdef_str::getmodval(module_value& x) const {
    int xx = sets.size() * 2;
    if (!id.empty())
      xx += 2;
    x.type = module_list;
    x.val.l.vals = newmodvals(x.val.l.n = xx);
    xx = 0;
    if (!id.empty()) {
      module_value& y = x.val.l.vals[xx++];
      y.type = module_string;
      y.val.s = "id";
      module_value& z = x.val.l.vals[xx++];
      z.type = module_string;
      z.val.s = id.c_str();
    }
    getmodvals(x.val.l.vals + xx);
  }
  void measdef_str::print(std::ostream& s, const fomusdata* fd,
                          const bool meas) const {
    s << (meas ? '|' : '<');
    bool sm = false;
    if (!id.empty()) {
      comma(s, sm);
      s << "id " << stringify(id, (meas ? "|," : ")>,"));
    } // was ")>,"
    printsets(s, fd, sm);
    s << (meas ? '|' : '>');
  }

  inline const boost::ptr_vector<userkeysigent>*
  measure::getkeysig_aux(const std::vector<std::pair<rat, rat>>*& vect,
                         std::string& ksname) const {
    try {
      const var_keysig& uks((var_keysig&) get_varbase(KEYSIGDEF_ID));
      if (uks.userempty()) {
        const var_keysigs& ukss((var_keysigs&) get_varbase(KEYSIG_ID));
        vect = &ukss.getkeysigvect(ksname = (RMUT(newkey).empty()
                                                 ? get_sval(COMMKEYSIG_ID)
                                                 : RMUT(newkey)));
        return &ukss.getuserdef(ksname);
      } else {
        vect = &uks.getsig();
        return &uks.getuserdef();
      }
    } catch (const keysigerr& e) {
      CERR << std::endl;
      throw;
    }
  }
  inline bool isconsist(const std::vector<std::pair<rat, rat>>& arr,
                        const int n) {
    assert(n >= 0 && n < 7);
    std::pair<rat, rat> acc(arr[n]);
    for (int i = n + 7; i < 75; i += 7) {
      DBG("comparing " << acc.first << "," << acc.second << " to "
                       << arr[i].first << "," << arr[i].second << std::endl);
      if (arr[i] != acc)
        return false;
    }
    return true;
  }

  struct commkeysig {
    int dianote;
    int acc;
    void set(modout_keysig& x) const {
      x.dianote = dianote;
      x.acc = acc;
    }
    commkeysig(const int d, const int a) : dianote(d), acc(a) {}
  };
  std::map<int, std::pair<commkeysig, commkeysig>>
      commkeysigsmap; // pair = maj, min
  int getkeyid(std::vector<int>& vect) {
    int keyid = 0;
    for (std::vector<int>::const_iterator i(vect.begin()); i != vect.end();
         ++i) {
      keyid *= 4;
      switch (*i) {
      case -1:
        keyid += 0x2;
        break;
      case 0:
        break;
      case 1:
        keyid += 0x1;
        break;
      default:
        assert(false);
      }
    }
    return keyid;
  }
#define KEY_C 0
#define KEY_D 1
#define KEY_E 2
#define KEY_F 3
#define KEY_G 4
#define KEY_A 5
#define KEY_B 6
  void initcommkeysigsmap() {
    {
      std::vector<int> x(7, 0);
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_C, 0), commkeysig(KEY_A, 0))));
      x[KEY_F] = 1; // f+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_G, 0), commkeysig(KEY_E, 0))));
      x[KEY_C] = 1; // c+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_D, 0), commkeysig(KEY_B, 0))));
      x[KEY_G] = 1; // g+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_A, 0), commkeysig(KEY_F, 1))));
      x[KEY_D] = 1; // d+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_E, 0), commkeysig(KEY_C, 1))));
      x[KEY_A] = 1; // a+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_B, 0), commkeysig(KEY_G, 1))));
      x[KEY_E] = 1; // e+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_F, 1), commkeysig(KEY_D, 1))));
      x[KEY_B] = 1; // b+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_C, 1), commkeysig(KEY_A, 1))));
    }
    {
      std::vector<int> x(7, 0);
      x[KEY_B] = -1; // f+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_F, 0), commkeysig(KEY_D, 0))));
      x[KEY_E] = -1; // c+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_B, -1), commkeysig(KEY_G, 0))));
      x[KEY_A] = -1; // g+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_E, -1), commkeysig(KEY_C, 0))));
      x[KEY_D] = -1; // d+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_A, -1), commkeysig(KEY_F, 0))));
      x[KEY_G] = -1; // a+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_D, -1), commkeysig(KEY_B, -1))));
      x[KEY_C] = -1; // e+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_G, -1), commkeysig(KEY_E, -1))));
      x[KEY_F] = -1; // b+
      commkeysigsmap.insert(
          std::map<int, std::pair<commkeysig, commkeysig>>::value_type(
              getkeyid(x), std::pair<commkeysig, commkeysig>(
                               commkeysig(KEY_C, -1), commkeysig(KEY_A, -1))));
    }
  }

  inline bool measure::isthemode(const std::string& mode, const int id) const {
    const listelvect& vect(((listvarofstrings&) get_varbase(id)).getvectval());
    for (listelvect_constit i(vect.begin()); i != vect.end(); ++i) {
      if (boost::algorithm::ends_with(mode, listel_getstring(*i)))
        return true;
    }
    return false;
  }

  const std::vector<std::pair<rat, rat>>&
  measure::getkeysig_init_aux() { // throws keysigerr!!
    std::string ksname;
    const std::vector<std::pair<rat, rat>>* ksig;
    const boost::ptr_vector<userkeysigent>* ord = getkeysig_aux(ksig, ksname);
    DBG("keysig = " << ksname << ' ');
    assert(ksig->size() == 75);
    keysigcache.dianote = -1;
    keysigcache.acc = 0;
    for (boost::ptr_vector<userkeysigent>::const_iterator i(ord->begin());
         i != ord->end(); ++i) {
      modout_keysig_indiv x = {
          ((i->n == std::numeric_limits<fint>::min() + 1 ||
            i->n.denominator() != 1)
               ? (fomus_int) -1
               : i->n.numerator()),
          (i->a == std::numeric_limits<fint>::min() + 1 ? makerat(0, 1)
                                                        : tofrat(i->a)),
          (i->m == std::numeric_limits<fint>::min() + 1 ? makerat(0, 1)
                                                        : tofrat(i->m))};
      kscache.push_back(x);
    }
    keysigcache.n = kscache.size();
    keysigcache.indiv = &kscache[0];
    for (int i = 0; i < 7; ++i) { // rule out fullindiv
      if (!isconsist(*ksig, i)) {
        keysigcache.mode = keysig_fullindiv;
        return *ksig;
      }
    }
    if (isthemode(ksname, MAJMODE_ID)) {
      keysigcache.mode = keysig_common_maj;
    } else if (isthemode(ksname, MINMODE_ID)) {
      keysigcache.mode = keysig_common_min;
    } else {
      keysigcache.mode = keysig_indiv;
      return *ksig;
    }
    int keyid = 0; // 14 bits
    for (std::vector<std::pair<rat, rat>>::const_iterator i(ksig->begin()),
         ie(ksig->begin() + 7);
         i < ie; ++i) {
      if (i->second == 0 && i->first.denominator() == 1) {
        keyid *= 4;
        switch (i->first.numerator()) {
        case -1:
          keyid += 0x2;
          break;
        case 0:
          break;
        case 1:
          keyid += 0x1;
          break;
        default:
          keysigcache.mode = keysig_indiv;
          return *ksig;
        }
      } else {
        keysigcache.mode = keysig_indiv;
        return *ksig;
      }
    }
    std::map<int, std::pair<commkeysig, commkeysig>>::const_iterator i(
        commkeysigsmap.find(keyid));
    if (i == commkeysigsmap.end())
      keysigcache.mode = keysig_indiv;
    if (keysigcache.mode == keysig_common_maj)
      i->second.first.set(keysigcache);
    else
      i->second.second.set(keysigcache);
    return *ksig;
  }

  partmap_str*
  clonepartmpart::operator()(const boost::shared_ptr<mpart_str>& x0) const {
    DBG("cloning partmap2" << std::endl);
    std::map<mpart_str*, boost::shared_ptr<mpart_str>>::iterator z(
        mcl.find(x0.get()));
    if (z != mcl.end())
      return new partmap_str(x, z->second);
    else {
      boost::shared_ptr<mpart_str> y(x0->fomclone(&x, cl, mcl, shift));
      mcl.insert(std::map<mpart_str*, boost::shared_ptr<mpart_str>>::value_type(
          x0.get(), y));
      return new partmap_str(x, y);
    }
  }
  partmap_str*
  clonepartmpart::operator()(const boost::shared_ptr<part_str>& x0) const {
    DBG("cloning partmap3" << std::endl);
    DBG("old part_str in partmap = " << (partormpart_str*) x0.get()
                                     << std::endl);
    std::map<part_str*, boost::shared_ptr<part_str>>::iterator i(
        cl.find(x0.get()));
    if (i != cl.end()) {
      DBG("replacement is " << i->second.get() << " id = " << i->second->getid()
                            << std::endl);
      return new partmap_str(x, i->second);
    } else {
      boost::shared_ptr<part_str> y(x0->fomclone(shift));
      cl.insert(std::map<part_str*, boost::shared_ptr<part_str>>::value_type(
          x0.get(), y));
      return new partmap_str(x, y);
    }
  }

  void staff_str::cachesinit(std::set<int>& st) {
    if (clefscache.empty()) { // might be called from multiple parts
      std::set<int> c;
      std::for_each(clefs.begin(), clefs.end(),
                    boost::lambda::bind(
                        &clef_str::getclefsaux,
                        boost::lambda::bind(&boost::shared_ptr<clef_str>::get,
                                            boost::lambda::_1),
                        boost::lambda::var(c)));
      clefscache.assign(c.begin(), c.end());
    }
    st.insert(clefscache.begin(), clefscache.end());
  }

  clef_str* staff_str::getclefptr(const int clef)
      const { // noteevbase calls this to cache the pointer to its clef
    std::vector<boost::shared_ptr<clef_str>>::const_iterator i(clefs.begin());
    while (true) {
      assert(i != clefs.end());
      if ((*i)->getclefptr(clef))
        return i->get();
      ++i;
    }
  }

  void instr_str::replace(instr_str& x) {
    id = x.id;
    staves.assign(x.staves.begin(), x.staves.end());
    stavesmod = x.stavesmod;
    ims.assign(x.ims.begin(), x.ims.end());
    imsmod = x.imsmod;
    ex = x.ex;
    percs.assign(x.percs.begin(), x.percs.end());
    percsmod = x.percsmod;
    str_base::replace(x);
  }

  percinstr_str* instr_str::findpercinsts(const char* na) const {
    for (std::vector<boost::variant<boost::shared_ptr<percinstr_str>,
                                    std::string>>::const_iterator
             i(percs.begin());
         i != percs.end(); ++i) {
      percinstr_str* r = boost::apply_visitor(percinstr_isid(na), *i);
      if (r)
        return r;
    }
    return 0;
  }

  void mpart_str::cachesinit() {
    if (partscache.empty()) {
      std::transform(parts.begin(), parts.end(), std::back_inserter(partscache),
                     boost::lambda::bind(&boost::shared_ptr<partmap_str>::get,
                                         boost::lambda::_1));
    }
    std::set<int> c;
    cachesinitformpart(c);
    clefscache.assign(c.begin(), c.end());
  }

  void mpart_str::insertallparts(scorepartlist& prts) {
    DBG("calling insertallparts");
    if (parts.empty()) {
      for (scorepartlist_it i(prts.begin()); i != prts.end(); ++i) {
        if (!(*i)->ismetapart()) {
          parts.push_back(boost::shared_ptr<partmap_str>(
              new partmap_str((boost::shared_ptr<fomus::part_str>&) *i)));
        }
      }
    }
  }

} // namespace fomus
