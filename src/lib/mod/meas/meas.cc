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
#include <limits>

#include "ifacedumb.h"
#include "meas.h"
#include "module.h"
#include "modutil.h"

#include "debugaux.h"

namespace meas {

  const char* ierr = 0;

  int mindurid, nbeatsid, measdivid, tsigid; //, beatid;

  extern "C" {
  void meas_run_fun(FOMUS f, void* moddata); // return true when done
  const char* meas_err_fun(void* moddata);
  }

  void meas_run_fun(FOMUS fom, void* moddata) {
    module_measobj lm = module_nextmeas();
    assert(lm);
    fomus_rat ldiv((fomus_int) 1 / module_setting_rval(lm, measdivid));
    struct module_value en(
        module_vendtime(lm)); // en is endtime of last note object
    module_noteobj n = 0;
    while (true) {
      n = module_peeknextnote(n);
      if (!n)
        break;
      struct module_value e(module_vendtime(n));
      if (e > en)
        en = e;
    }
    // bool fi = true;
    module_noteobj an = module_nextnote();
    while (true) {
      // bool assone = false;
      const module_partobj p = module_nextpart();
      if (!p)
        break;
      fomus_rat mn(module_setting_rval(p, mindurid));
      fomus_rat be = {0, 1}; // measure times should be rationals
      assert(lm);
      fomus_rat d, lad = {0, 1};
      module_measobj m;
#ifndef NDEBUG
      fomus_rat div = {-1, 1};
#else
      fomus_rat div;
#endif
      bool assone2;
      while (true) {
        assone2 = false;
        d = module_setting_rval(lm, nbeatsid);
        if (d <= (fomus_int) 0) {
          module_value ts(module_setting_val(
              lm, tsigid)); // is user explicitly specifying timesig?
          assert(ts.type == module_list && ts.val.l.n == 2);
          assert(ts.val.l.vals->type == module_int);
          if (ts.val.l.vals->val.i <= 0) {
            assert(ldiv > (fomus_int) 0);
            d = roundto(module_vdur(lm), ldiv);
          } else {
            fomus_rat nude;
            nude.num = ts.val.l.vals->val.i;
            nude.den = (ts.val.l.vals + 1)->val.i;
            assert(nude.num > 0);
            d = nude /
                module_writtenmult(lm); // module_setting_rval(lm, beatid);
          }
        }
        if (d > (fomus_int) 0)
          lad = d;
        // check for consistency
        m = module_nextmeas();
        if (m)
          div = ((fomus_int) 1 /
                 module_setting_rval(
                     m, measdivid)); // was before break statement above
        if (!m || module_part(m) != p)
          break;
        fomus_rat e(roundto(module_vtime(m), div));
        if (d <= (fomus_int) 0) {
          d = e - be;
          if (d > (fomus_int) 0)
            lad = d;
        }
        bool rm = false;
        while (true) { // from be to e    insert UP TO m
          fomus_rat di(e - be);
          if (di <= (fomus_int) 0)
            break;
          if ((di - d) >= mn) {
            DBG("ASSIGNING MEASURE1: ti:" << be << " du:" << d << " rm:" << rm
                                          << std::endl);
            meas_assign(lm, be, d, rm);
            assone2 = true;
            be = be + d;
          } else {
            DBG("ASSIGNING MEASURE2: ti:" << be << " du:" << di << " rm:" << rm
                                          << std::endl);
            meas_assign(lm, be, di, rm);
            assone2 = true;
            be = e;
            break;
          }
          rm = true;
        }
        lm = m;
      }
      if (d <= (fomus_int) 0) {
        if (lad <= (fomus_int) 0)
          d = module_makerat(4, 1);
        else
          d = lad; // default 4/1 should only happen if all measures are dur 0
      }
      // bool rm = assone2;
      while (be < en || !assone2) {
        DBG("ASSIGNING MEASURE3: ti:" << be << " du:" << d << " rm:" << assone2
                                      << std::endl);
        meas_assign(lm, be, d, assone2);
        assone2 = true;
        be = be + d;
      }
      // if (fi) {
      // 	fi = false;
      // 	an = module_nextnote();
      // }
      while (an && module_part(an) == p) {
        module_skipassign(an);
        an = module_nextnote();
      }
      lm = m;
      ldiv = div;
    }
  }

  const char* meas_err_fun(void* moddata) {
    return 0;
  }

  // ------------------------------------------------------------------------------------------------------------------------
  const char* mindurtype = "rational>=0";
  int valid_mindur(const struct module_value val) {
    return module_valid_rat(val, module_makerat(0, 1), module_incl,
                            module_makerat(0, 1), module_nobound, 0,
                            mindurtype);
  }

  void fill_iface(void* moddata, void* iface) {
    ((dumb_iface*) iface)->moddata = 0;
    ((dumb_iface*) iface)->run = meas_run_fun;
    ((dumb_iface*) iface)->err = meas_err_fun;
  };

  const char* beatdivtype = "rational>=1";
  int valid_beatdiv(const struct module_value val) {
    return module_valid_rat(val, module_makerat(1, 1), module_incl,
                            module_makerat(0, 1), module_nobound, 0,
                            beatdivtype);
  }

  // ------------------------------------------------------------------------------------------------------------------------

  int get_setting(int n, module_setting* set, int id) {
    switch (n) {
    case 0: {
      set->name = "min-measdur"; // docscat{meas}
      set->type = module_number;
      set->descdoc =
          "The minimum duration a measure created by FOMUS (i.e., not "
          "explicitly specified by the user) is allowed to be.  "
          "Measures smaller than this duration are merged with other "
          "(previous) measures to form larger measures."
          "  FOMUS might need to create a small measure to accommodate a "
          "user-defined measure that doesn't align well with previous measures."
          "  If you like tiny measures and don't want FOMUS to merge them with "
          "other measures, set the value of this setting to 0.";
      set->typedoc = mindurtype;

      module_setval_int(&set->val, 2);

      set->loc = module_locmeasdef; // module_locpercinst;
      set->valid = valid_mindur;
      set->uselevel = 2;
      mindurid = id;
      break;
    }
    case 1: {
      set->name = "meas-beatdiv"; // docscat{quant} // division of beat
      set->type = module_rat;
      set->descdoc =
          "The number of divisions per beat, used when quantizing note events. "
          " "
          "If `timesig-den' is 4 (meaning a beat is equal to one quarter "
          "note), a `meas-beatdiv' of 4 allows measure times and "
          "durations to be quantized down to the size of a sixteenth note.  "
          "A `meas-beatdiv' of 8 would allow precision at the level of "
          "thirty-second notes, etc..";
      set->typedoc = beatdivtype;

      module_setval_int(&set->val, 4);

      set->loc = module_locmeasdef; // module_locpercinst;
      set->valid = valid_beatdiv;
      set->uselevel = 2;
      measdivid = id;
      break;
    }
    default:
      return 0;
    };
    return 1;
  }

  void ready() {
    nbeatsid = module_settingid("measdur");
    if (nbeatsid < 0) {
      ierr = "midding setting `measdur'";
      return;
    }
    tsigid = module_settingid("timesig");
    if (tsigid < 0) {
      ierr = "midding setting `timesig'";
      return;
    }
    // beatid = module_settingid("beat");
    // if (beatid < 0) {ierr = "missing required setting `beat'"; return;}
  }

} // namespace meas
