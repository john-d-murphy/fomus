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

#include "ifacedumb.h"
#include "markevs1.h"
#include "module.h"

#include "debugaux.h"

// TAKES CARE OF DETACHED MARKS

namespace markevs1 {

  const char* ierr = 0;

  int detachid;

  extern "C" {
  void markevs1run_fun(FOMUS f, void* moddata); // return true when done
  const char* markevs1err_fun(void* moddata);
  }

  void markevs1run_fun(FOMUS fom, void* moddata) {
    while (
        true) { // this has to be the last module, since it adds/deletes notes!
      module_noteobj n = module_nextnote();
      if (!n)
        break;
      module_skipassign(n);
    }
    module_partobj prt = module_nextpart();
    module_objlist mm0(module_getmarkevlist(prt));
    for (const module_markobj *i(mm0.objs), *ie(mm0.objs + mm0.n); i != ie;
         ++i) {
      int det = module_setting_ival(*i, detachid);
      module_markslist ma(module_marks(*i));
      module_value ti(module_vtime(*i)), eti(module_vendtime(*i));
      for (const module_markobj *a(ma.marks), *ae(ma.marks + ma.n); a != ae;
           ++a) {
        int id = module_markid(*a);
        int mdet = module_markisdetachable(id);
        if (mdet >= 2 || (mdet && det)) {
          module_intslist vs(module_voices(*i));
          // for (const int* v = vs.ints, *ve = vs.ints + vs.n; v < ve; ++v) {
          if (vs.n > 0) {
            int vo = vs.ints[0];
            switch (module_markpos(*a)) {
            case markpos_above:
              vo += 2000;
              break;
            case markpos_below:
              vo += 3000;
              break;
            default:
              vo += 1000;
            }
            markevs_assign_add(
                prt, vo, // pedals and similar marks w/ markpos_below go on
                         // lowest staff w/ voice = 3000+
                (module_markisspannerend(module_markbaseid(id)) ? eti : ti), id,
                module_markstring(*a), module_marknum(*a));
          }
        }
      }
    }
  }

  const char* markevs1err_fun(void* moddata) {
    return 0;
  }

  void fill_iface(void* moddata, void* iface) {
    ((dumb_iface*) iface)->moddata = 0;
    ((dumb_iface*) iface)->run = markevs1run_fun;
    ((dumb_iface*) iface)->err = markevs1err_fun;
  };

  // ------------------------------------------------------------------------------------------------------------------------

  int get_setting(int n, module_setting* set, int id) {
    // switch (n) {
    // case 0:
    //   {
    // 	set->name = "detach"; // doc**scat{marks}
    // 	set->type = module_bool;
    // 	set->descdoc = //"Most marks (e.g., articulation) must be attached to
    // note events.  " 	  "Some marks such as crescendo wedges often need to
    // appear detached from notes events.  " 	  "Setting `detach' to `yes'
    // insures that these marks appear in the location given by the mark event's
    // time
    // attribute, " 	  "regardless of whether or not any note events are at
    // that location.  Setting `detach' to no forces all " 	  "marks to be
    // attached to note events.";
    // 	//set->typedoc = markevtype;

    // 	module_setval_int(&set->val, 1);

    // 	set->loc = module_locnote;
    // 	//set->valid = valid_markev; // no range
    // 	//set->validdeps = validdeps_maxp;
    // 	set->uselevel = 2;
    // 	detachid = id;
    // 	break;
    //   }
    // default:
    //   return 0;
    // }
    return 0;
  }

  void init() {
    detachid = module_settingid("detach");
    if (detachid < 0) {
      ierr = "missing required setting `detach'";
      return;
    }
  }

  // const char* module_initerr() {return ierr;}

} // namespace markevs1
