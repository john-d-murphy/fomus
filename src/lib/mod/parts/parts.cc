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

#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <boost/algorithm/string/predicate.hpp>

#include "ifacedumb.h"
#include "module.h"

#include "debugaux.h"
#include "ilessaux.h"
using namespace ilessaux;

namespace parts {

  typedef std::map<const std::string, parts_grouptype, isiless> strtogrouptype;
  typedef strtogrouptype::value_type strtogrouptype_val;
  typedef strtogrouptype::const_iterator strtogrouptype_constit;
  strtogrouptype strtogroupb, strtogroupe;

  int groupsid, groupid, ensemid, groupsingid;

  struct partserr {};

  struct parthold {
    module_partobj p;
    int ind;
    parthold(const module_partobj p, const int ind) : p(p), ind(ind) {}
  };
  inline bool operator<(const parthold& x, const parthold& y) {
    return x.ind < y.ind;
  }

  struct partsdata {
    std::multimap<const std::string, parthold, isiless> pts;
    const module_value* e2;
    int ord;
    bool tmark;

private:
    bool cerr;
    std::stringstream CERR;
    std::string errstr;

public:
    partsdata() : ord(0), tmark(true), cerr(false) {}
    void barf() {
      cerr = true;
      CERR << "invalid score layout" << std::endl;
      throw partserr();
    }
    const char* err_fun() {
      if (!cerr)
        return 0;
      std::getline(CERR, errstr);
      return errstr.c_str();
    }
    void run_fun(FOMUS fom);
    void inner(module_value*& e1, const parts_grouptype g, module_partobj& fip0,
               module_partobj& rep0);
  };

  void partsdata::inner(
      module_value*& e, const parts_grouptype g, module_partobj& fip0,
      module_partobj&
          rep0) { // ord begins at int_min--so assigned parts just go to bottom
    assert(e->type == module_string);
    module_partobj fip = 0; // first part found
    module_partobj rep;     // most recent part found
    static const std::string ast("*");
    while (true) {
      if (e == e2) {
        if (g != parts_nogroup)
          barf();
        return;
      }
      if (e->val.s == ast) {
        tmark = true;
        goto CONT;
      }
      {
        DBG("::: " << e->val.s << std::endl);
        strtogrouptype_constit beg(strtogroupb.find(e->val.s));
        if (beg != strtogroupb.end()) {
          inner(++e, beg->second, fip, rep);
        } else {
          strtogrouptype_constit en(strtogroupe.find(e->val.s));
          if (en != strtogroupe.end()) {
            if (fip) {
              if (en->second == g) {
                if (fip != rep || module_setting_ival(fip, groupsingid))
                  parts_assigngroup(fip, rep, g);
                rep0 = rep;
                if (!fip0)
                  fip0 = fip;
              } else
                barf();
            }
            return;
          }
          std::multimap<const std::string, parthold, isiless>::iterator i(
              pts.lower_bound(e->val.s));
          std::set<parthold> pp;
          while (i != pts.end() &&
                 boost::algorithm::iequals(e->val.s, i->first)) {
            pp.insert(i->second);
            pts.erase(i++);
          }
          for (std::set<parthold>::iterator i(pp.begin()); i != pp.end(); ++i) {
            rep = i->p;
            if (!fip)
              fip = rep;
            parts_assignorder(rep, ord++, tmark);
            tmark = false;
          }
        }
      }
    CONT:
      ++e;
    }
  }

  void partsdata::run_fun(FOMUS fom) {
    while (true) {
      module_noteobj n = module_nextnote();
      if (!n)
        break;
    }
    try {
      std::string ens(module_setting_sval(fom, ensemid));
      int ind = 0;
      while (true) {
        module_partobj p = module_nextpart();
        if (!p)
          break;
        DBG("ID = " << module_id(p) << std::endl);
        DBG("INSTR = " << module_id(module_inst(p)) << std::endl);
        pts.insert(
            std::multimap<const std::string, parthold, isiless>::value_type(
                module_id(module_inst(p)), parthold(p, ind++)));
      }
      struct module_list lst0(GET_L(module_setting_val(fom, groupid)));
      if (lst0.n > 0) {
        e2 = lst0.vals + lst0.n;
        module_partobj tmp;
        module_value* e = lst0.vals;
        inner(e, parts_nogroup, tmp, tmp);
        goto OK;
      }
      {
        struct module_list lst(GET_L(module_setting_val(fom, groupsid)));
        for (module_value *i = lst.vals, *ie = lst.vals + lst.n; i < ie;
             i += 2) {
          if (ens == GET_S(*i)) {
            struct module_list l(GET_L(*(i + 1)));
            e2 = l.vals + l.n;
            module_partobj tmp;
            module_value* e = l.vals;
            inner(e, parts_nogroup, tmp, tmp);
            goto OK;
          }
        }
      }
      {
        int ord = 0;
        module_partobj i = 0;
        while (true) {
          i = module_peeknextpart(i);
          if (!i)
            break;
          parts_assignorder(i, ord++, tmark);
          tmark = false;
        }
      }
    OK:
      module_noteobj n = 0, ln;
      while (true) {
        ln = n;
        n = module_peeknextnote(n);
        if (ln)
          module_skipassign(ln);
        if (!n)
          break;
      }
    } catch (const partserr& e) {}
  }

  extern "C" {
  void run_partfun(FOMUS f, void* moddata); // return true when done
  const char* err_partfun(void* moddata);
  }

  const char* err_partfun(void* moddata) {
    return ((partsdata*) moddata)->err_fun();
  }
  void run_partfun(FOMUS f, void* moddata) {
    ((partsdata*) moddata)->run_fun(f);
  } // return true when done

  const char* ensemtype = "string";

  // const char* valid_def_type = "";
  // int valid_def(struct module_value val) {module_valid_listofstrings(val, -1,
  // -1, -1, -1, 0, valid_def_type);}

} // namespace parts

using namespace parts;

void module_fill_iface(void* moddata, void* iface) {
  ((dumb_iface*) iface)->moddata = moddata;
  ((dumb_iface*) iface)->run = run_partfun;
  ((dumb_iface*) iface)->err = err_partfun;
};
const char* module_longname() {
  return "Parts";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Groups staves and arranges the order of parts in the score.";
}
void* module_newdata(FOMUS f) {
  return new partsdata;
}
void module_freedata(void* dat) {
  delete (partsdata*) dat;
}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modparts;
}
const char* module_initerr() {
  return 0;
}
void module_init() {
  strtogroupb.insert(strtogrouptype_val("[", parts_group));
  strtogroupb.insert(strtogrouptype_val("b-", parts_group));
  strtogroupb.insert(strtogrouptype_val("br-", parts_group));
  strtogroupb.insert(strtogrouptype_val("bra-", parts_group));
  strtogroupb.insert(strtogrouptype_val("bracket-", parts_group));

  strtogroupb.insert(strtogrouptype_val("{", parts_grandstaff));
  strtogroupb.insert(strtogrouptype_val("g-", parts_grandstaff));
  strtogroupb.insert(strtogrouptype_val("gr-", parts_grandstaff));
  strtogroupb.insert(strtogrouptype_val("gra-", parts_grandstaff));
  strtogroupb.insert(strtogrouptype_val("grandstaff-", parts_grandstaff));

  strtogroupb.insert(strtogrouptype_val("<", parts_choirgroup));
  strtogroupb.insert(strtogrouptype_val("c-", parts_choirgroup));
  strtogroupb.insert(strtogrouptype_val("ch-", parts_choirgroup));
  strtogroupb.insert(strtogrouptype_val("cho-", parts_choirgroup));
  strtogroupb.insert(strtogrouptype_val("choir-", parts_choirgroup));

  strtogroupe.insert(strtogrouptype_val("]", parts_group));
  strtogroupe.insert(strtogrouptype_val("-b", parts_group));
  strtogroupe.insert(strtogrouptype_val("-br", parts_group));
  strtogroupe.insert(strtogrouptype_val("-bra", parts_group));
  strtogroupe.insert(strtogrouptype_val("-bracket", parts_group));

  strtogroupe.insert(strtogrouptype_val("}", parts_grandstaff));
  strtogroupe.insert(strtogrouptype_val("-g", parts_grandstaff));
  strtogroupe.insert(strtogrouptype_val("-gr", parts_grandstaff));
  strtogroupe.insert(strtogrouptype_val("-gra", parts_grandstaff));
  strtogroupe.insert(strtogrouptype_val("-grandstaff", parts_grandstaff));

  strtogroupe.insert(strtogrouptype_val(">", parts_choirgroup));
  strtogroupe.insert(strtogrouptype_val("-c", parts_choirgroup));
  strtogroupe.insert(strtogrouptype_val("-ch", parts_choirgroup));
  strtogroupe.insert(strtogrouptype_val("-cho", parts_choirgroup));
  strtogroupe.insert(strtogrouptype_val("-choir", parts_choirgroup));
}
void module_free() { /*assert(newcount == 0);*/
}
int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
int module_itertype() {
  return module_all;
}

// ------------------------------------------------------------------------------------------------------------------------

int module_get_setting(int n, module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "layout-defs"; // docscat{libs}
    set->type = module_symmap_stringlists;
    set->descdoc =
        "A mapping from ids to part layout definitions."
        "  Layout definitions inform FOMUS on how to order and group the parts "
        "in a score."
        "  Each one consists of instrument IDs specified in the order that "
        "they should appear in the score."
        "  \"[\" and \"]\" strings should surround instruments that are "
        "grouped together (i.e., with a bracket) and can be nested to any "
        "depth."
        "  \"{\" and \"}\" are used to group instruments with a brace (usually "
        "this isn't needed--parts with multiple staves automatically get a "
        "brace)."
        "  \"<\"  and \">\" are used for groups that aren't connected with a "
        "vertical bar line at each measure boundary."
        "  A \"*\" before a top-level group (i.e., before a family of "
        "instruments such as woodwinds or strings) indicates a location where "
        "system-wide markings such as Tempo should appear."
        "  Layout definitions should contain any instruments that you might "
        "use--instruments that appear in the definition but aren't in a score "
        "are ignored."
        "  A good place to alter this setting is in your `.fomus' "
        "configuration file."
        "  To define a layout specifically for a score, use `layout-def' "
        "instead.";
    // set->typedoc = groupstype;

    module_setval_list(&set->val, 0); // this is set in the config file

    set->loc = module_locscore;
    // set->valid = valid_groups;
    set->uselevel = 2;
    groupsid = id;
    break;
  }
  case 1: {
    set->name = "layout"; // docscat{basic}
    set->type = module_string;
    set->descdoc =
        "A string identifying a part layout."
        "  Set this to affect how parts are ordered and grouped in the score."
        "  The value of this setting must be the ID of one of the part layouts "
        "defined in `layout-defs'."
        //"  FOMUS has two predefined layouts that you can use: `small-ensemble'
        // and `orchestra'."
        ;
    // set->typedoc = ensemtype;

    module_setval_string(&set->val, "default");

    set->loc = module_locscore;
    // set->valid = valid_ensem;
    set->uselevel = 1;
    ensemid = id;
    break;
  }
  case 2: {
    set->name = "layout-def"; // docscat{instsparts}
    set->type = module_list_strings;
    set->descdoc =
        "A single part layout definition.  "
        //"If this is not a zero-length list, this definition is used instead of
        // any predefined ones in setting `layout-defs'"
        "Set this to define a layout and override the `layout' setting."
        "  Part layout definitions inform FOMUS on how to order and group the "
        "parts in a score."
        "  It consists of a list of instrument IDs specified in the order that "
        "they should appear in the score."
        "  \"[\" and \"]\" strings should surround instruments that are "
        "grouped together (i.e., with a bracket) and can be nested to any "
        "depth."
        "  \"{\" and \"}\" are used to group instruments with a brace (usually "
        "this isn't needed--parts with multiple staves automatically get a "
        "brace)."
        "  \"<\"  and \">\" are used for groups that aren't connected with a "
        "vertical bar line at each measure boundary."
        "  A \"*\" before a top-level group (i.e., before a family of "
        "instruments such as woodwinds or strings) indicates a location where "
        "system-wide markings such as Tempo should appear.";
    // set->typedoc = ensemtype;

    module_setval_list(&set->val, 0);

    set->loc = module_locscore;
    // set->valid = valid_ensem;
    set->uselevel = 2;
    groupid = id;
    break;
  }
  case 3: {
    set->name = "group-single"; // docscat{instsparts}
    set->type = module_bool;
    set->descdoc =
        "Indicates whether or not an instrument or part can be the only one in "
        "a group with a bracket around it.  "
        "Set this to `yes' in an instrument or part to cause a bracket to "
        "always appear, even around a single part (that is, if the parts "
        "layout indicates that there should be a bracket).";
    // set->typedoc = ensemtype;

    module_setval_int(&set->val, 0);

    set->loc = module_locpart;
    // set->valid = valid_ensem;
    set->uselevel = 2;
    groupsingid = id;
    break;
  }
  default:
    return 0;
  }
  return 1;
}
void module_ready() {}
const char* module_engine(void* d) {
  return "dumb";
}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
