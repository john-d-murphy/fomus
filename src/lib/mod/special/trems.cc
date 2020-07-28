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

#include <cassert>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/ptr_container/ptr_set.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "ifacedumb.h"
#include "module.h"
#include "modutil.h"

#include "debugaux.h"
#include "marksaux.h"
using namespace marksaux;
#include "ilessaux.h"
using namespace ilessaux;
#include "ferraux.h"
using namespace ferraux;

namespace trems {

  int trilltotremid, tremtotrillid, trillshowauxid;

  extern "C" {
  void run_fun(FOMUS fom, void* moddata); // return true when done
  const char* err_fun(void* moddata);
  }

  enum whatitis { what_nothing, what_trill, what_trem };

  struct noteholder {
    module_noteobj n;
    std::multiset<mark, markless> orig, ins;
    bool inv, del;
    noteholder(const module_noteobj n) : n(n), inv(false), del(false) {}
    void skipassign() const {
      module_skipassign(n);
    }
    void assign() {
      if (inv)
        special_assign_makeinv(n);
      else if (del)
        special_assign_delete(n);
      else {
        inplace_set_difference<std::multiset<mark, markless>::iterator,
                               std::multiset<mark, markless>::iterator,
                               markless>(orig, ins);
        std::for_each(orig.begin(), orig.end(),
                      boost::lambda::bind(&mark::assrem, boost::lambda::_1, n));
        std::for_each(ins.begin(), ins.end(),
                      boost::lambda::bind(&mark::assass, boost::lambda::_1, n));
        marks_assign_done(n);
      }
    }
    void insert(const mark& mk) {
      ins.insert(mk);
    }
    void insorig(const mark& mk) {
      orig.insert(mk);
    }
    void doerr(const module_noteobj n, const char* str);
    void remove() {
      inv = true;
    }
    void delet() {
      del = true;
    }
  };

  inline bool hassamepitches(const std::vector<noteholder*>& nos1,
                             const std::vector<noteholder*>& nos2) {
    if (nos1.size() != nos2.size())
      return false;
    std::multiset<fomus_rat> p1, p2;
    for (std::vector<noteholder*>::const_iterator i(nos1.begin());
         i != nos1.end(); ++i)
      p1.insert(module_pitch((*i)->n));
    for (std::vector<noteholder*>::const_iterator i(nos2.begin());
         i != nos2.end(); ++i)
      p2.insert(module_pitch((*i)->n));
    return std::equal(p1.begin(), p1.end(), p2.begin());
  }

  struct tremsdata {
    bool cerr;
    std::stringstream MERR;
    std::string errstr;
    int cnt;

    tremsdata() : cerr(false), cnt(0) {}

    const char* err() {
      if (!cerr)
        return 0;
      std::getline(MERR, errstr);
      return errstr.c_str();
    }

    void doerr(const module_noteobj n, const char* str);

    inline module_value qtrmaux(const module_noteobj n, const fomus_rat& trm,
                                const fomus_rat& wrm, const bool trem2) {
      return (trm < module_adjdur(n, -1) * wrm
                  ? module_makeval(trem2 ? -trm.den : trm.den)
                  : module_makenullval());
    }
    module_value qtrm(const module_noteobj n, const module_value& trm,
                      const fomus_rat& wrm, const bool trem2 = false) {
      if (trm.type == module_none)
        return qtrmaux(n, module_makerat(1, 32), wrm, trem2);
      module_value x(trm * wrm);
      if (x >= module_makerat(3, 16))
        return module_makenullval();
      if (x >= module_makerat(3, 32))
        return qtrmaux(n, module_makerat(1, 8), wrm, trem2);
      if (x >= module_makerat(3, 64))
        return qtrmaux(n, module_makerat(1, 16), wrm, trem2);
      return qtrmaux(n, module_makerat(1, 32), wrm, trem2);
    }

    inline void gettheaccs(const module_value& tri, module_value& acc1,
                           module_value& acc2) {
      if (tri.type == module_none) {
        acc1.type = acc2.type = module_none;
      } else {
        acc1 =
            module_makeval(tri >= (fomus_int) 0 ? mfloor(tri) : -mfloor(-tri));
        acc2 = tri - acc1;
      }
    }

    void run() {
      whatitis tr = what_nothing;
      boost::ptr_vector<noteholder> nos;
      std::vector<noteholder*> nos1, nos2;
      fomus_rat wrm(module_writtenmult(module_nextmeas()));
      module_value tri, trm;
      tri.type = module_none;
      trm.type = module_none;
      bool typ = false;
      while (true) {
        module_noteobj n0 = module_nextnote();
        if (!n0)
          break;
        noteholder* n;
        nos.push_back(n = new noteholder(n0));
        module_markslist m(module_singlemarks(n0));
        for (const module_markobj *i(m.marks), *ie(m.marks + m.n); i != ie;
             ++i) {
          switch (module_markid(*i)) {
          case mark_longtrill: {
            nos1.push_back(n);
            if (tr == what_nothing)
              tr = what_trill;
            const char* x = module_markstring(*i);
            if (x) {
              // module_noteparts np;
              fomus_rat a(module_strtoacc(x, 0));
              assert(a.num != std::numeric_limits<fomus_int>::max());
              tri = module_makeval(a);
            } else
              tri.type = module_none;
            n->insorig(mark(mark_longtrill, x ? x : ""));
            break;
          }
          case mark_longtrill2: {
            nos2.push_back(n);
            if (tr == what_nothing)
              tr = what_trill;
            typ = module_setting_ival(n0, trillshowauxid); // acc
            n->insorig(mark(mark_longtrill2));
            break;
          }
          // case mark_longtrill2_note: {
          //   nos2.push_back(n);
          //   if (tr == what_nothing) tr = what_trill;
          //   typ = true; // acc
          //   n->insorig(mark(mark_longtrill2_note));
          //   break;
          // }
          case mark_trem: {
            nos1.push_back(n);
            if (tr == what_nothing)
              tr = what_trem;
            trm = module_marknum(*i);
            n->insorig(mark(mark_trem, trm));
            break;
          }
          case mark_trem2: {
            nos2.push_back(n);
            if (tr == what_nothing)
              tr = what_trem;
            n->insorig(mark(mark_trem2));
            break;
          }
          default:; // nos2.push_back(n);
          }
        }
        if (tr != what_nothing &&
            module_isendchord(n0)) { // figure out proper notation
          if (nos1.empty()) {
            doerr(n, "invalid trill/tremolo marking");
            std::for_each(
                nos.begin(), nos.end(),
                boost::lambda::bind(module_skipassign,
                                    &boost::lambda::_1)); // SKIP ASSIGNS
            goto POSTASSIGN;
          }
          {
            bool hsp = hassamepitches(nos1, nos2);
            if (hsp || nos2.empty() ||
                (tr == what_trill &&
                 tri.type !=
                     module_none)) { // SINGLE-trem OR trill with accidental
                                     // argument given (ignore longtr2 mark) OR
                                     // SAME NOTES in double something
              if (hsp && tr != what_trem) {
                tr = what_trem;
                trm.type = module_none;
              } // if 1 note only (same pitches), single trem
              if (tr == what_trill) {
                module_value acc1, acc2;
                gettheaccs(tri, acc1, acc2); // acc2 should be a rat
                std::for_each(nos1.begin(), nos1.end(),
                              boost::lambda::bind(&noteholder::insert,
                                                  boost::lambda::_1,
                                                  mark(mark_longtrill, acc1)));
                std::for_each(nos1.begin(), nos1.end(),
                              boost::lambda::bind(&noteholder::insert,
                                                  boost::lambda::_1,
                                                  mark(mark_longtrill2, acc2)));
                std::for_each(nos2.begin(), nos2.end(),
                              boost::lambda::bind(&noteholder::remove,
                                                  boost::lambda::_1));
              } else {
                module_value trm0(qtrm(n0, trm, wrm));
                if (trm0.type != module_none) { // single note/chord tremelo w/
                                                // slashes through stems
                  std::for_each(
                      nos1.begin(), nos1.end(),
                      boost::lambda::bind(
                          &noteholder::insert, boost::lambda::_1,
                          mark(mark_trem, trm0))); // > 0 = single trem
                  std::for_each(nos2.begin(), nos2.end(),
                                boost::lambda::bind(&noteholder::delet,
                                                    boost::lambda::_1));
                }
              }
              goto ASSIGN;
            }
          }
          switch (tr) { // DOUBLE MARK trem OR DOUBLE MARK trill w/ second note
                        // given
          case what_trill: {
            if (nos1.size() != 1 ||
                nos2.size() != 1 // impossible for a trill, make it a tremolo
                || (todiatonic(module_writtennote(nos2[0]->n)) -
                            todiatonic(module_writtennote(nos1[0]->n)) !=
                        (fomus_int) 1 // not a step, so make it a tremolo
                    && module_setting_ival(nos1[0]->n, trilltotremid))) {
              tr = what_trem;
              trm.type = module_none;
            }
            break;
          }
          case what_trem: {
            if (nos1.size() == 1 && nos2.size() == 1 &&
                trm.type == module_none &&
                todiatonic(module_writtennote(nos2[0]->n)) -
                        todiatonic(module_writtennote(nos1[0]->n)) ==
                    (fomus_int) 1 // it's a step, make it a trill
                && module_setting_ival(nos1[0]->n, tremtotrillid)) {
              tr = what_trill;
              tri.type = module_none;
            }
            break;
          }
          default:
            assert(false);
          }
          assert(!nos1.empty());
          assert(!nos2.empty());
          switch (tr) {
          case what_trill: {
            assert(nos1.size() == 1);
            assert(nos2.size() == 1);
            assert(tri.type == module_none);
            // if (tri.type == module_none) { // not user-supplied, find the
            // accidental #warning "should this be comparing pitch classes?  i.e.
            //module_note(nos2[0]->n) - module_note(nos1[0]->n) < (fomus_int)3"
            // no because this should have been sorted out already
            if (!typ && todiatonic(module_writtennote(nos2[0]->n)) -
                                todiatonic(module_writtennote(nos1[0]->n)) ==
                            1) { // 1 step, a "proper" trill
              fomus_rat a(module_fullwrittenacc(nos2[0]->n));
              assert(tri.type == module_none);
              //#warning "should also show acc if it's not in keysig"
              if (a.num != std::numeric_limits<fomus_int>::max())
                tri = module_makeval(a);
              // nos2[0]->remove(); // make aux. note invisible (lily and xml
              // should skip them), but still influences other notes for accs,
              // etc.. assert(false);
              module_value acc1, acc2;
              gettheaccs(tri, acc1, acc2);
              nos1[0]->insert(mark(mark_longtrill, acc1));  // = acc1
              nos1[0]->insert(mark(mark_longtrill2, acc2)); // = acc2
            } else { // need a little note in ()
              nos1[0]->insert(mark(mark_longtrill,
                                   module_pitch(nos2[0]->n))); // NOTE IN PARENS
              nos1[0]->insert(mark(mark_longtrill, tri));      // = acc1
              nos1[0]->insert(mark(mark_longtrill2, tri));     // = acc2
            }
            // } else { // user gave a number, it's an ACCIDENTAL
            //   module_value acc1, acc2;
            //   gettheaccs(tri, acc1, acc2);
            //   nos1[0]->insert(mark(mark_longtrill, acc1)); // = acc1
            //   nos1[0]->insert(mark(mark_longtrill2_acc, acc2)); // = acc2
            // }
            nos2[0]->remove();
            // std::for_each(nos1.begin(), nos1.end(),
            // boost::lambda::bind(&noteholder::insert, *boost::lambda::_1,
            // mark(mark_longtrill, tri))); std::for_each(nos2.begin(),
            // nos2.end(), boost::lambda::bind(&noteholder::insert,
            // *boost::lambda::_1, mark(mark_longtrill2)));
            break;
          }
          case what_trem: {
            module_value trm0(qtrm(n0, trm, wrm, true));
            if (trm0.type == module_none)
              break;
            std::for_each(nos1.begin(), nos1.end(),
                          boost::lambda::bind(&noteholder::insert,
                                              *boost::lambda::_1,
                                              mark(mark_trem, trm0)));
            std::for_each(nos2.begin(), nos2.end(),
                          boost::lambda::bind(&noteholder::insert,
                                              *boost::lambda::_1,
                                              mark(mark_trem2, trm0)));
            break;
          }
          default:;
          }
        ASSIGN:
          std::for_each(
              nos.begin(), nos.end(),
              boost::lambda::bind(&noteholder::assign, boost::lambda::_1));
        POSTASSIGN:
          tr = what_nothing;
          nos.clear();
          nos1.clear();
          nos2.clear();
          tri.type = module_none;
          trm.type = module_none;
          typ = false;
        }
      }
      std::for_each(
          nos.begin(), nos.end(),
          boost::lambda::bind(&noteholder::assign, boost::lambda::_1));
      if (cnt) {
        MERR << "invalid trills/tremelos" << std::endl;
        cerr = true;
      }
    }
  };

  void tremsdata::doerr(const module_noteobj n, const char* str) {
    if (cnt < 8) {
      CERR << str;
      ferr << module_getposstring(n) << std::endl;
    } else if (cnt == 8) {
      CERR << "more errors..." << std::endl;
    }
    ++cnt;
  }

  void run_fun(FOMUS fom, void* moddata) {
    ((tremsdata*) moddata)->run();
  }
  const char* err_fun(void* moddata) {
    return ((tremsdata*) moddata)->err();
  }

} // namespace trems

using namespace trems;

int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
void module_fill_iface(void* moddata, void* iface) { // module
  ((dumb_iface*) iface)->moddata = moddata;
  ((dumb_iface*) iface)->run = run_fun;
  ((dumb_iface*) iface)->err = err_fun;
}
const char* module_err(void* data) {
  return 0;
}
const char* module_longname() {
  return "Trills/Tremolos";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Interprets trill and tremelo marks.";
}
void* module_newdata(FOMUS f) {
  return new tremsdata;
} // new accsdata;}
void module_freedata(void* dat) {
  delete (tremsdata*) dat;
} // delete (accsdata*)dat;}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modspecial;
}
int module_itertype() {
  return module_bymeas | module_byvoice;
}
const char* module_initerr() {
  return 0;
}
void module_init() {}
void module_free() { /*assert(newcount == 0);*/
}
int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "trills-to-trems"; // docscat{special}
    set->type = module_bool;
    set->descdoc =
        "Whether or not some trills are converted to tremolos.  "
        "If set to `yes', indicates that trills that aren't the standard "
        "interval of a 2nd are converted to unmeasured tremolos.";
    // set->typedoc = ashowtypes; // same valid fun

    module_setval_int(&set->val, 0);

    set->loc = module_locnote;
    // set->valid = valid_ashowtypes; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2; // technical
    trilltotremid = id;
    break;
  }
  case 1: {
    set->name = "trems-to-trills"; // docscat{special}
    set->type = module_bool;
    set->descdoc =
        "Whether or not some tremolos are converted to trills.  "
        "If set to `yes', indicates that unmeasured tremolos that consist of "
        "only two notes and are only 2nd apart are converted to trills.";
    // set->typedoc = ashowtypes; // same valid fun

    module_setval_int(&set->val, 0);

    set->loc = module_locnote;
    // set->valid = valid_ashowtypes; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2; // technical
    tremtotrillid = id;
    break;
  }
  case 2: {
    set->name = "show-trillnote"; // docscat{special}
    set->type = module_bool;
    set->descdoc =
        "Indicates how trills should be notated.  "
        "If set to `yes', indicates that long trills are shown with a small "
        "note in parentheses indicating the auxiliary pitch.  "
        "Setting this to `no' indicates that trills are shown with an "
        "accidental above the trill sign.";
    // set->typedoc = ashowtypes; // same valid fun

    module_setval_int(&set->val, 0);

    set->loc = module_locnote;
    // set->valid = valid_ashowtypes; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2; // technical
    trillshowauxid = id;
    break;
  }
  default:;
    return 0;
  }
  return 1;
}
const char* module_engine(void*) {
  return "dumb";
}
void module_ready() {}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
