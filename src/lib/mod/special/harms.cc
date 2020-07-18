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

#include <cassert>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>
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

namespace harms {

  const char* ierr = 0;

  int defid, natshowid, artshowid, strtypeid, openstrid, maxartintid, minintid,
      minpitid;

  extern "C" {
  void run_fun(FOMUS fom, void* moddata); // return true when done
  const char* err_fun(void* moddata);
  }

  // inline fomus_rat getmarknum(const module_noteobj n, const module_markobj m)
  // {
  //   module_value x(module_marknum(m));
  //   if (x.type == module_none) return module_note(n);
  //   return GET_R(x);
  // }
  // fomus_int tryints[] = {12, 7, 5, 4, 3};

  struct invalid {};
  struct conflicting {};
  struct notenough {};

  fomus_rat bttos(const fomus_rat& base, const fomus_rat& t, const bool art,
                  const fomus_rat& maxartint,
                  const fomus_rat& minint) { // base+touched to sounding
    fomus_rat d(t - base);
    if ((art && d > maxartint) || d < minint)
      throw invalid();
    if (d == (fomus_int) 12)
      return base + (fomus_int) 12;
    if ((d == (fomus_int) 7) || (!art && d == (fomus_int) 7 + 12))
      return base + ((fomus_int) 12 + 7); // 12
    if ((d == (fomus_int) 5) || (!art && d == (fomus_int) 5 + 7 + 12))
      return base + ((fomus_int) 12 + 7 + 5); // 12 + 7
    if ((d == (fomus_int) 4) ||
        (!art && ((d == (fomus_int) 4 + 5) || (d == (fomus_int) 4 + 5 + 7) ||
                  (d == (fomus_int) 4 + 5 + 7 + 12))))
      return base + ((fomus_int) 12 + 7 + 5 + 4); // 12, 7 + 12, 5 + 7 + 12
    if ((d == (fomus_int) 3) || (!art && d == (fomus_int) 3 + 4 + 5 + 7 + 12))
      return base + ((fomus_int) 12 + 7 + 5 + 4 + 3); // 12 + 7 + 5 + 4
    throw invalid();
  }
  fomus_rat bstot(const fomus_rat& base, const fomus_rat& s,
                  const fomus_rat& pt, const bool art,
                  const fomus_rat& maxartint,
                  const fomus_rat& minint) { // base+sounding to touched (pt =
                                             // possible touched)
    fomus_rat d(s - base);
    static const int h12[] = {12, 0};
    static const int h7[] = {7, 7 + 12, 0};
    static const int h5[] = {5, 5 + 7 + 12, 0};
    static const int h4[] = {4, 4 + 5, 4 + 5 + 7, 4 + 5 + 7 + 12, 0};
    static const int h3[] = {3, 3 + 4 + 5 + 7 + 12, 0};
    const int* r;
    if (d == (fomus_int) 12)
      r = h12;
    else if (d == (fomus_int) 12 + 7)
      r = h7; // TODO: can return multiple solutions!...  pick closest one to
              // given "touched" value
    else if (d == (fomus_int) 12 + 7 + 5)
      r = h5;
    else if (d == (fomus_int) 12 + 7 + 5 + 4)
      r = h4;
    else if (d == (fomus_int) 12 + 7 + 5 + 4 + 3)
      r = h3;
    else
      throw invalid();
    if ((art && (fomus_int) r[0] > maxartint) || (fomus_int) r[0] < minint)
      throw invalid();
    if (art || pt == (fomus_int) 0)
      return base +
             module_makerat(r[0], 1); // 0 = just return default lowest one
    fomus_rat dst = {std::numeric_limits<fomus_int>::max() - 1, 1},
              ret; // possible t is given, find closest
    for (; *r != 0; ++r) {
      fomus_rat ca(base + module_makerat(*r, 1));
      fomus_rat d(diff(pt, ca));
      if (d < dst) {
        dst = d;
        ret = ca;
      }
    }
    return ret;
  }
  fomus_rat tstob(const fomus_rat& t, const fomus_rat& s, const bool art,
                  const std::set<fomus_rat>& open, const fomus_rat& maxartint,
                  const fomus_rat& minint,
                  const fomus_rat& minpit) { // touched+sounding to base
    fomus_rat d(s - t);
    static const int h0[] = {
        12, 12 + 7, 12 + 7 + 5, 12 + 7 + 5 + 4, 12 + 7 + 5 + 4 + 3,
        0}; // possible bases below sounding
    static const int h1[] = {12 + 7, 12 + 7 + 5 + 4, 0};     // 12
    static const int h2[] = {12 + 7 + 5, 12 + 7 + 5 + 4, 0}; // 12 + 7
    static const int h3[] = {12 + 7 + 5 + 4, 0};             // 12 + 7 + 5
    static const int h4[] = {12 + 7 + 5 + 4 + 3, 0};         // 12 + 7 + 5 + 4
    const int* r;
    if (d == (fomus_int) 0)
      r = h0;
    else if (d == (fomus_int) 12)
      r = h1;
    else if (d == (fomus_int) 12 + 7)
      r = h2;
    else if (d == (fomus_int) 12 + 7 + 5)
      r = h3;
    else if (d == (fomus_int) 12 + 7 + 5 + 4)
      r = h4;
    else
      throw invalid();
    for (; *r != 0; ++r) {
      fomus_rat ret(s - (fomus_int) *r);
      fomus_rat d0(t - ret);
      if (ret < minpit || (art && d0 > maxartint) || d0 < minint ||
          (!open.empty() && open.find(ret) == open.end()))
        continue;
      return ret;
    }
    throw invalid();
  }
  const int tryhs[] = {
      12, 12 + 7, 12 + 7 + 5, 12 + 7 + 5 + 4, 12 + 7 + 5 + 4 + 3,
      0}; // possible bases below sounding
  fomus_rat
  stob(const fomus_rat& s, const std::set<fomus_rat>& open,
       const fomus_rat& minpit) { // sounding to base (natural harmonics only!)
    for (const int* r = tryhs; *r != 0; ++r) {
      fomus_rat ret(s - (fomus_int) *r);
      if (ret < minpit || (!open.empty() && open.find(ret) == open.end()))
        continue;
      return ret;
    }
    throw invalid();
  }

  enum optsenum {
    opts_play,
    opts_touch,
    opts_sound,
    opts_string,
    opts_circle,
    opts_stringtext,
    opts_none
  };
  typedef std::map<std::string, optsenum, isiless> optstype;
  optstype opts;

  enum strtypeenum {
    /*tstyle_sul,*/ tstyle_let,
    tstyle_rom
  };
  typedef std::map<std::string, strtypeenum, isiless> strtypestype;
  strtypestype strtypes;

  enum whichone { which_none, which_art, which_nat };
  // typedef std::map<std::string, whichone, isiless> whichonetype;
  // whichonetype whichones;

  struct noteholderbase {
    module_noteobj n;
    std::multiset<mark, markless> orig, ins;
    noteholderbase(const module_noteobj n) : n(n) {}
    void skipassign() const {
      module_skipassign(n);
    }
    void insert(const mark& mk) {
      ins.insert(mk);
    }
    void insert(const std::multiset<mark, markless>& ms) {
      ins.insert(ms.begin(), ms.end());
    }
    void insorig(const mark& mk) {
      orig.insert(mk);
    }
    void doerr(const module_noteobj n, const char* str);
    virtual void notdel() {}
    virtual bool isntdel() {
      return true;
    }
  };
  struct noteholder : public noteholderbase {
    bool del;
    noteholder(const module_noteobj n) : noteholderbase(n), del(false) {}
    void assign();
    void delet() {
      del = true;
    }
    void notdel() {
      del = false;
    }
    bool isntdel() {
      return !del;
    }
  };
  void noteholder::assign() {
    if (del)
      special_assign_delete(n);
    else {
      inplace_set_difference<std::multiset<mark, markless>::iterator,
                             std::multiset<mark, markless>::iterator, markless>(
          orig, ins);
      std::for_each(orig.begin(), orig.end(),
                    boost::lambda::bind(&mark::assrem, boost::lambda::_1, n));
      std::for_each(ins.begin(), ins.end(),
                    boost::lambda::bind(&mark::assass, boost::lambda::_1, n));
      marks_assign_done(n);
    }
  }
  inline fomus_int todiatonic2(const fomus_int x) {
    return (x >= 0 ? todiatonic(x) : -todiatonic(-x));
  }
  inline void getaccs(const fomus_rat& frnote, const fomus_rat& facc,
                      const fomus_rat& tonote, fomus_rat& acc1,
                      fomus_rat& acc2) {
    assert((frnote - facc).den == 1);
    assert((tonote - frnote).den == 1);
    fomus_rat ac(tonote - tochromatic(todiatonic((frnote - facc).num) +
                                      todiatonic2((tonote - frnote).num)));
    acc1 = module_makerat(ac.num / ac.den, 1);
    acc2 = ac - acc1;
  }

  struct newnote : public noteholderbase {
    fomus_rat note, acc1, acc2;
    newnote(const module_noteobj n, const fomus_rat& note,
            const fomus_rat& frnote, const fomus_rat& facc)
        : noteholderbase(n), note(note) {
      getaccs(frnote, facc, note, acc1, acc2);
      assert(iswhite(note - acc1 - acc2));
      module_markslist ml(module_marks(n));
      for (const module_markobj *i(ml.marks), *ie(ml.marks + ml.n); i < ie;
           ++i) {
        int id(module_markid(*i));
        switch (id) {
        case mark_natharm_sounding:
        case mark_artharm_sounding:
        case mark_natharm_touched:
        case mark_artharm_touched:
        case mark_artharm_base:
        case mark_natharm_string:
          break;
        default: {
          const char* str = module_markstring(*i);
          insert(mark(module_markid(*i), str ? str : "", module_marknum(*i)));
        }
        }
      }
    }
    void assign();
  };
  void newnote::assign() {
    inplace_set_difference<std::multiset<mark, markless>::iterator,
                           std::multiset<mark, markless>::iterator, markless>(
        orig, ins);
    std::vector<special_markspec> x;
    for (std::multiset<mark, markless>::const_iterator i(ins.begin());
         i != ins.end(); ++i) {
      special_markspec y = {i->ty, i->str.c_str(), i->val};
      x.push_back(y);
    }
    special_markslist ml = {x.size(), &x[0]};
    special_assign_newnote(n, note, acc1, acc2, ml);
  }

  struct harmsdata {
    bool cerr;
    std::stringstream MERR;
    std::string errstr;
    int cnt;

    harmsdata() : cerr(false), cnt(0) {}

    const char* err() {
      if (!cerr)
        return 0;
      std::getline(MERR, errstr);
      return errstr.c_str();
    }

    void doerr(const module_noteobj n, const char* str);

    void run() {
      std::set<fomus_rat> open;
      module_value op(module_setting_val(module_nextpart(), openstrid));
      assert(op.type == module_list);
      for (const module_value *i(op.val.l.vals),
           *ie(op.val.l.vals + op.val.l.n);
           i < ie; ++i)
        open.insert(GET_R(*i));
      boost::ptr_vector<noteholder> notevect;
      bool so = false, to = false, pl = false, st = false;
      noteholderbase *sonh = 0, *tonh = 0, *plnh = 0, *stnh = 0;
      fomus_rat sov, tov, plv, stv; // mark value
      whichone wh = which_none;
      fomus_rat nomin = {std::numeric_limits<fomus_int>::max() - 1, 1};
      module_noteobj n0;
      // fomus_rat acc1 = {0, 1}, acc2 = {0, 1};
      while (true) {
        module_noteobj n = module_nextnote();
        if (!n)
          break;
        noteholder* nh;
        notevect.push_back(nh = new noteholder(n));
        fomus_rat no(module_pitch(n));
        module_markslist m(module_singlemarks(n));
        try {
          for (const module_markobj *i(m.marks), *ie(m.marks + m.n); i != ie;
               ++i) {
            switch (module_markid(*i)) {
            case mark_natharm_sounding: {
              if (so || (wh != which_none && wh != which_nat))
                throw conflicting();
              so = true;
              sonh = nh;
              wh = which_nat;
              sov = no; // getmarknum(n, *i); // returns the note if the marknum
                        // is NONE
              nh->insorig(*i);
              break;
            }
            case mark_artharm_sounding: {
              if (so || (wh != which_none && wh != which_art))
                throw conflicting();
              so = true;
              sonh = nh;
              wh = which_art;
              sov = no; // getmarknum(n, *i);
              nh->insorig(*i);
              break;
            }
            case mark_natharm_touched: {
              if (to || (wh != which_none && wh != which_nat))
                throw conflicting();
              to = true;
              tonh = nh;
              wh = which_nat;
              tov = no; // getmarknum(n, *i);
              nh->insorig(*i);
              break;
            }
            case mark_artharm_touched: {
              if (to || (wh != which_none && wh != which_art))
                throw conflicting();
              to = true;
              tonh = nh;
              wh = which_art;
              tov = no; // getmarknum(n, *i);
              nh->insorig(*i);
              break;
            }
            case mark_artharm_base: {
              if (pl || (wh != which_none && wh != which_art))
                throw conflicting();
              pl = true;
              plnh = nh;
              wh = which_art;
              plv = no; // getmarknum(n, *i);
              nh->insorig(*i);
              break;
            }
            case mark_natharm_string: {
              if (st || (wh != which_none && wh != which_nat))
                throw conflicting();
              st = true;
              stnh = nh;
              wh = which_nat;
              stv = no; // getmarknum(n, *i);
              nh->insorig(*i);
              break;
            }
            default:
              goto SKIPIT;
            }
            // {
            //   fomus_rat acc10 = module_acc1(n), acc20 = module_acc2(n);
            //   if (acc1 == (fomus_int)0) acc1 = acc10; else if ((acc1 <
            //   (fomus_int)0 && acc10 > (fomus_int)0) || (acc1 > (fomus_int)0
            //   && acc10 < (fomus_int)0))
            //     throw invalid();
            //   if (acc2 == (fomus_int)0) acc2 = acc20; else if ((acc2 <
            //   (fomus_int)0 && acc20 > (fomus_int)0) || (acc2 > (fomus_int)0
            //   && acc20 < (fomus_int)0))
            //     throw invalid();
            // }
          SKIPIT:;
          }
          if (no < nomin) {
            nomin = no;
            n0 = n;
          }
          if (!module_isendchord(n))
            continue;
          if (so || to || pl || st) { // user wants a harmonic, figure it out
            fomus_rat maxartint = module_setting_rval(n0, maxartintid);
            fomus_rat minint = module_setting_rval(n0, minintid);
            fomus_rat minpit = module_setting_rval(n0, minpitid);
            if (pl) { // it's ARTIFICIAL--assume (!st)
              if (st)
                throw conflicting(); // THROWS
              if (to && so) {
                tov = bstot(plv, sov, tov, true, maxartint,
                            minint); // readjust tov!
              } else if (to && !so) {
                sov = bttos(plv, tov, true, maxartint, minint);
              } else if (so && !to) {
                tov = bstot(plv, sov, module_makerat(0, 1), true, maxartint,
                            minint);
              } else
                throw notenough();
            } else if (st) { // (and !pl) it's natural or artificial
              if (to && so) {
                tov = bstot(stv, sov, tov, false, maxartint, minint);
              } else if (to && !so) {
                sov = bttos(stv, tov, false, maxartint, minint);
              } else if (so && !to) {
                tov = bstot(stv, sov, module_makerat(0, 1), false, maxartint,
                            minint);
              } else
                throw notenough();
            } else if (to && so) { // no played pitch or string
              switch (wh) {
              case which_none:
              case which_nat:
                try {
                  stv = tstob(tov, sov, false, open, maxartint, minint, minpit);
                  st = true;
                  goto GOTIT;
                } catch (const invalid& e) {} // user screwed up, try artificial
              case which_art:
                plv = tstob(tov, sov, true, open, maxartint, minint, minpit);
                pl = true;
                break;
              }
            } else if (so) { // do
              switch (wh) {
              case which_none:
              case which_nat:
                try {
                  stv = stob(sov, open, minpit);
                  tov = bstot(stv, sov, module_makerat(0, 1), false, maxartint,
                              minint);
                  st = true;
                  goto GOTIT;
                } catch (const invalid& e) {} // try artificial
              case which_art:
                for (const int* i = tryhs; *i != 0; ++i) {
                  try {
                    plv = sov - (fomus_int) *i;
                    tov = bstot(plv, sov, module_makerat(0, 1), true, maxartint,
                                minint);
                    pl = true;
                    goto GOTIT;
                  } catch (const invalid& e) {}
                }
                throw invalid();
              }
            } else
              notenough(); // not enough information
          GOTIT: // now check it, either pl and st are true, indicating type of
                 // harmonic
            assert(pl != st);
            // if (st && !open.empty() && open.find(stv) == open.end())
            //   throw invalid();
            // if (ttos(pl ? plv : stv, tov) != sov)
            //   throw invalid();
            struct module_list shw(
                module_setting_val(n0, pl ? artshowid : natshowid).val.l);
            std::multiset<mark, markless> ins;
            std::for_each(
                notevect.begin(), notevect.end(),
                boost::lambda::bind(&noteholder::delet, boost::lambda::_1));
            boost::ptr_vector<newnote> newnotes;
            fomus_rat fra(module_fullacc(n0));
            for (const module_value *i(shw.vals), *ie(shw.vals + shw.n);
                 i != ie; ++i) {
              assert(i->type == module_string);
              switch (opts.find(i->val.s)->second) {
              case opts_play: {
                if (plnh)
                  plnh->notdel();
                else
                  newnotes.push_back(
                      (newnote*) (plnh = new newnote(n0, plv, nomin, fra)));
                plnh->insert(mark(mark_artharm_base));
              } break;
              case opts_string: {
                if (stnh)
                  stnh->notdel();
                else
                  newnotes.push_back(
                      (newnote*) (stnh = new newnote(n0, stv, nomin, fra)));
                stnh->insert(mark(mark_natharm_string));
              } break;
              case opts_touch: {
                if (tonh)
                  tonh->notdel();
                else
                  newnotes.push_back(
                      (newnote*) (tonh = new newnote(n0, tov, nomin, fra)));
                tonh->insert(
                    mark(pl ? mark_artharm_touched : mark_natharm_touched));
              } break; // THIS IS FOR BOTH TYPES OF HARMONICS
              case opts_sound: {
                if (sonh)
                  sonh->notdel();
                else
                  newnotes.push_back(
                      (newnote*) (sonh = new newnote(n0, sov, nomin, fra)));
                sonh->insert(
                    mark(pl ? mark_artharm_sounding : mark_natharm_sounding));
              } break;
              case opts_circle: {
                ins.insert(mark(mark_harm));
              } break;
              case opts_stringtext: {
                switch (
                    strtypes.find(module_setting_sval(n0, strtypeid))->second) {
                case tstyle_let:
                  ins.insert(mark(
                      mark_sul,
                      stv.num / stv.den)); // specify the string, which is in a
                                           // diferent font ("/" converts to int
                  break;
                case tstyle_rom: {
                  std::set<fomus_rat>::const_iterator i(open.find(stv));
                  if (i == open.end())
                    break;
                  int n = 0;
                  do {
                    ++n;
                    ++i;
                  } while (i != open.end());
                  ins.insert(mark(mark_sul, n)); // specify "roman" ???
                  break;
                }
                default:
                  assert(false);
                }
                break;
              }
              default:
                assert(false);
              }
            }
            if (stnh && stnh->isntdel())
              stnh->insert(ins);
            else if (plnh && plnh->isntdel())
              plnh->insert(ins);
            else if (tonh && tonh->isntdel())
              tonh->insert(ins);
            else if (sonh && sonh->isntdel())
              sonh->insert(ins);
            std::for_each(
                newnotes.begin(), newnotes.end(),
                boost::lambda::bind(&newnote::assign, boost::lambda::_1));
            std::for_each(
                notevect.begin(), notevect.end(),
                boost::lambda::bind(&noteholder::assign, boost::lambda::_1));
            notevect.clear();
            so = to = pl = st = false;
            sonh = tonh = plnh = stnh = 0;
            wh = which_none;
            nomin.num = std::numeric_limits<fomus_int>::max() - 1;
            // acc1.num = acc2.num = 0;
            // acc1.den = acc2.den =
            nomin.den = 1;
            // continue;
          }
        } catch (const invalid& e) {
          doerr(n, "invalid harmonic pitches");
        } catch (const conflicting& e) {
          doerr(n, "conflicting harmonic type");
        } catch (const notenough& e) {
          doerr(n, "not enough harmonic information");
        }
        std::for_each(
            notevect.begin(), notevect.end(),
            boost::lambda::bind(&noteholder::skipassign, boost::lambda::_1));
        notevect.clear();
        so = to = pl = st = false;
        sonh = tonh = plnh = stnh = 0;
        wh = which_none;
        nomin.num = std::numeric_limits<fomus_int>::max() - 1;
        // acc1.num = acc2.num = 0;
        // acc1.den = acc2.den =
        nomin.den = 1;
      }
      if (cnt) {
        MERR << "invalid harmonics" << std::endl;
        cerr = true;
      }
    }
  };

  void harmsdata::doerr(const module_noteobj n, const char* str) {
    if (cnt < 8) {
      CERR << str;
      ferr << module_getposstring(n) << std::endl;
    } else if (cnt == 8) {
      CERR << "more errors..." << std::endl;
    }
    ++cnt;
  }

  void run_fun(FOMUS fom, void* moddata) {
    ((harmsdata*) moddata)->run();
  }
  const char* err_fun(void* moddata) {
    return ((harmsdata*) moddata)->err();
  }

  const char* ashowtypes = "(string_show string_show ...), string_show = "
                           "base|touched|sounding|circle";
  int valid_ashowtypes_aux(int x, const char* val) {
    const static bool arr[] = {true, true, true, false, true, false};
    optstype::const_iterator i(opts.find(val));
    return (i != opts.end() && arr[i->second]);
  }
  int valid_ashowtypes(const struct module_value val) {
    return module_valid_listofstrings(val, -1, -1, 1, -1, valid_ashowtypes_aux,
                                      ashowtypes);
  }
  const char* nshowtypes = "(string_show string_show ...), string_show = "
                           "string|touched|sounding|circle|stringtext";
  int valid_nshowtypes_aux(int x, const char* val) {
    const static bool arr[] = {false, true, true, true, true, true};
    optstype::const_iterator i(opts.find(val));
    return (i != opts.end() && arr[i->second]);
  }
  int valid_nshowtypes(const struct module_value val) {
    return module_valid_listofstrings(val, -1, -1, 1, -1, valid_nshowtypes_aux,
                                      nshowtypes);
  }
  const char* deftypes = "string|base|touched|sounding";
  int valid_deftypes_aux(const char* val) {
    const static bool arr[] = {true, true, true, true, false, false};
    optstype::const_iterator i(opts.find(val));
    return (i != opts.end() && arr[i->second]);
  }
  int valid_deftypes(const struct module_value val) {
    return module_valid_string(val, 1, -1, valid_deftypes_aux, deftypes);
  }
  // const char* strtypetypes = "letter|roman";
  // int valid_strtypetypes_aux(const char* val) {return (strtypes.find(val) !=
  // strtypes.end());} int valid_strtypetypes(const struct module_value val)
  // {return module_valid_string(val, 1, -1, valid_strtypetypes_aux,
  // strtypetypes);}
  const char* openstrtype = "(note|rational0..128 note|rational0..128 ...)";
  int valid_openstrtype(const struct module_value val) {
    return module_valid_listofrats(val, -1, -1, module_makerat(0, 1),
                                   module_incl, module_makerat(128, 1),
                                   module_incl, 0, openstrtype);
  }

  const char* minrattype = "rational3..7";
  int valid_minrattype(const struct module_value val) {
    return module_valid_rat(val, module_makerat(3, 1), module_incl,
                            module_makerat(7, 1), module_incl, 0, minrattype);
  }
  const char* maxrattype = "rational>=3";
  int valid_maxrattype(const struct module_value val) {
    return module_valid_rat(val, module_makerat(3, 1), module_incl,
                            module_makerat(0, 1), module_nobound, 0,
                            maxrattype);
  }

} // namespace harms

using namespace harms;

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
  return "Harmonics";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Facilitates and checks notation of string harmonics.";
}
void* module_newdata(FOMUS f) {
  return new harmsdata;
} // new accsdata;}
void module_freedata(void* dat) {
  delete (harmsdata*) dat;
} // delete (accsdata*)dat;}
int module_priority() {
  return 0;
}
enum module_type module_type() {
  return module_modspecial;
}
int module_itertype() {
  return module_bymeas | module_byvoice | module_norests;
}
const char* module_initerr() {
  return ierr;
}
void module_init() {
  opts.insert(optstype::value_type("base", opts_play));
  opts.insert(optstype::value_type("touched", opts_touch));
  opts.insert(optstype::value_type("sounding", opts_sound));
  opts.insert(optstype::value_type("string", opts_string));
  opts.insert(optstype::value_type("circle", opts_circle));
  opts.insert(optstype::value_type("stringtext", opts_stringtext));
  strtypes.insert(strtypestype::value_type("letter", tstyle_let));
  strtypes.insert(strtypestype::value_type("roman", tstyle_rom));
  strtypes.insert(strtypestype::value_type("sulletter", tstyle_let));
  strtypes.insert(strtypestype::value_type("sulroman", tstyle_rom));
}
void module_free() {}
int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {
  case 0: {
    set->name = "harms-artificial-show"; // docscat{special}
    set->type = module_list_strings;
    set->descdoc =
        "This is a list of strings specifying which notes and symbols of an "
        "artificial harmonic are to appear in the score."
        "  Set this to determine how artificial harmonics are notated."
        "  The choices are `base', `touched' and `sounding' and `circle'.  "
        "`circle' places a harmonic symbol in the score above the note.";
    set->typedoc = ashowtypes; // same valid fun

    module_setval_list(&set->val, 2);
    struct module_list& x = set->val.val.l;
    module_setval_string(x.vals, "base");
    module_setval_string(x.vals + 1, "touched");

    set->loc = module_locnote;
    set->valid = valid_ashowtypes; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2; // technical
    artshowid = id;
    break;
  }
  case 1: {
    set->name = "harms-natural-show"; // docscat{special}
    set->type = module_list_strings;
    set->descdoc =
        "This is a list of strings specifying which notes and symbols of an "
        "natural harmonic are to appear in the score."
        "  Set this to determine how natural harmonics are notated."
        "  The choices are `string', `touched' and `sounding', `circle' and "
        "`stringtext'.  "
        "`circle' places a harmonic symbol in the score above the note while "
        "`stringtext' inserts a \"Sul\" text mark with the proper string name.";
    set->typedoc = nshowtypes; // same valid fun

    module_setval_list(&set->val, 3);
    struct module_list& x = set->val.val.l;
    module_setval_string(x.vals, "sounding");
    module_setval_string(x.vals + 1, "circle");
    module_setval_string(x.vals + 2, "stringtext");

    set->loc = module_locnote;
    set->valid = valid_nshowtypes; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2; // technical
    natshowid = id;
    break;
  }
  case 2: {
    set->name = "open-strings"; // docscat{special}
    set->type = module_list_nums;
    set->descdoc =
        "Specifies the open strings of a string instrument or part."
        "  Set this in the definition of a string instrument so that natural "
        "and artificial harmonics may be calculated correctly.";
    set->typedoc = openstrtype; // same valid fun

    module_setval_list(&set->val, 0);

    set->loc = module_locpart;
    set->valid = valid_openstrtype; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2; // technical
    openstrid = id;
    break;
  }
  case 3: {
    set->name = "harms-maxartificial"; // docscat{special}
    set->type = module_rat;
    set->descdoc =
        "The maximum interval (in semitones) allowed between the base and "
        "touched pitches of an artificial harmonic.  "
        "Set this in the definition of a string instrument to the widest "
        "interval a player can be expected to stretch.  "
        "Instruments with longer strings might require a smaller value here.";
    set->typedoc = maxrattype; // same valid fun

    module_setval_int(&set->val, 5);

    set->loc = module_locnote;
    set->valid = valid_maxrattype; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2; // technical
    maxartintid = id;
    break;
  }
  case 4: {
    set->name = "harms-min"; // docscat{special}
    set->type = module_rat;
    set->descdoc =
        "The minimum interval (in semitones) allowed between the base and "
        "touched pitches of an artificial harmonic or the "
        "open string and touched pitches of a natural harmonic.  "
        "Set this to place a limit on how difficult the harmonic is to play.";
    set->typedoc = minrattype; // same valid fun

    module_setval_int(&set->val, 3);

    set->loc = module_locnote;
    set->valid = valid_minrattype; // no range
    // set->validdeps = validdeps_maxp;
    set->uselevel = 2; // technical
    minintid = id;
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
void module_ready() {
  strtypeid = module_settingid("sul-style");
  if (strtypeid < 0) {
    ierr = "missing required setting `sul-style'";
    return;
  }
  minpitid = module_settingid("min-pitch");
  if (minpitid < 0) {
    ierr = "missing required setting `min-pitch'";
    return;
  }
}
int module_sameinst(module_obj a, module_obj b) {
  return true;
}
