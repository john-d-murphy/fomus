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

#include "algext.h" // renew
#include "data.h"   // class fomusdata
#include "error.h"
#include "fomusapi.h" // types fomus_bool, fomus_float, etc..
#include "instrs.h"   // default_insts
#include "marks.h"    // initmarks()
#include "mods.h"     // initmodules
#include "numbers.h"  // class rat
#include "schedr.h"
#include "vars.h" // initvars

int fomus_api_version() {
  return FOMUS_API_VERSION;
}

namespace fomus {

#define RINGBUF_SIZE 2048

  boost::shared_mutex
      listenermut; // must lock this when doing something important!!!!!
  volatile bool listening = false;

  enum whichbuf { buf_ival, buf_rval, buf_mval, buf_fval, buf_sval, buf_act };
  struct bufobj {
    whichbuf wh;
    FOMUS fom;
    int par;
    int act;
    union {
      struct {
        fomus_int val1;
        fomus_int val2;
        fomus_int val3;
      } val;
      fomus_float fval;
      const char* str;
    } x;
    bufobj() {}
    void set(FOMUS fom0, const int par0, const int act0) volatile {
      fom = fom0;
      wh = buf_act;
      par = par0;
      act = act0;
    }
    void set(FOMUS fom0, const int par0, const int act0,
             const fomus_int val1) volatile {
      fom = fom0;
      wh = buf_ival;
      par = par0;
      act = act0;
      x.val.val1 = val1;
    }
    void set(FOMUS fom0, const int par0, const int act0, const fomus_int val1,
             const fomus_int val2) volatile {
      fom = fom0;
      wh = buf_rval;
      par = par0;
      act = act0;
      x.val.val1 = val1;
      x.val.val2 = val2;
    }
    void set(FOMUS fom0, const int par0, const int act0, const fomus_int val1,
             const fomus_int val2, const fomus_int val3) volatile {
      fom = fom0;
      wh = buf_mval;
      par = par0;
      act = act0;
      x.val.val1 = val1;
      x.val.val2 = val2;
      x.val.val3 = val3;
    }
    void set(FOMUS fom0, const int par0, const int act0,
             const fomus_float val1) volatile {
      fom = fom0;
      wh = buf_fval;
      par = par0;
      act = act0;
      x.fval = val1;
    }
    void set(FOMUS fom0, const int par0, const int act0,
             const char* str0) volatile {
      fom = fom0;
      wh = buf_sval;
      par = par0;
      act = act0;
      x.str = str0;
    }
    void doit() const;
  };
  volatile bufobj* volatile inptr;
  bufobj* volatile outptr;

  extern const char* partostrs[];
  extern const char* acttostrs[];
  inline const char* paramtostr(const int par) {
    return (par >= 0 && par < fomus_par_n) ? partostrs[par] : "?";
  }
  inline const char* actiontostr(const int act) {
    return (act >= 0 && act < fomus_act_n) ? acttostrs[act] : "?";
  }

  boost::ptr_set<fomusdata> data;
  bool operator<(const fomusdata& x, const fomusdata& y) {
    return &x < &y;
  }

  void inituserconfig();
  void initfomusconfig();
  void initboolsyms();
  void initstrenummaps();
  void initaccrules();
  void initcommkeysigsmap();
  void initpresets();

  boost::condition_variable_any listenercond;
  boost::thread inthread;
  void listener();

  struct scoped_ring : std::vector<bufobj> {
    ~scoped_ring() {
      off();
    }
    void off();
  };
  scoped_ring ringbuf;

  inline void catchup() {
    while (outptr != inptr) {
      outptr->doit();
      if (outptr >= &ringbuf[RINGBUF_SIZE - 1])
        outptr = &ringbuf[0];
      else
        ++outptr;
    }
  }

  template <typename T>
  inline std::string numbtostr(const T& val) {
    std::ostringstream str;
    str << val;
    return str.str();
  }

} // namespace fomus

using namespace fomus;

// data
void fomus_init() {
#if defined(HAVE_FENV_H) && !defined(NDEBUG)
  feenableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW);
#endif
  ENTER_API;
  static bool cs = true;
  if (cs) {
    initboolsyms();
    initstrenummaps();
    initaccrules();
    initcommkeysigsmap();
    cs = false;
  }
  clearmodules();
  inituserconfig();  // just the filename!
  initfomusconfig(); // just the filename!
  data.clear();
  initpresets();
  initvars();
  initmarks();
  initmodules(); // gets all settings
  loadconf();
  isinited = true;
  EXIT_API_VOID;
}
FOMUS fomus_new() {
  ENTER_MAINAPI;
  checkinit();
  fomusdata* x;
  data.insert(x = new fomusdata);
  return x;
  EXIT_API_0;
}
FOMUS fomus_copy(FOMUS f) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  fomusdata* x;
  data.insert(x = new fomusdata(*(fomusdata*) f));
  return x;
  EXIT_API_0;
}
void fomus_merge(FOMUS to, FOMUS from) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) to)->isvalid());
  assert(((fomusdata*) from)->isvalid());
  ((fomusdata*) to)->mergeinto(*((fomusdata*) from));
  EXIT_API_VOID;
}
void fomus_free(const FOMUS f) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  if (listening)
    outptr = (fomus::bufobj*) inptr;
  data.erase(data.find(*(fomusdata*) f));
  EXIT_API_VOID;
}

// clear all input (usually it's done automatically)
void fomus_clear(FOMUS f) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  renew(*(fomusdata*) f);
  EXIT_API_VOID;
}

const char* fomus_version() {
  ENTER_MAINAPI;
  return VERSION;
  EXIT_API_0;
}

namespace fomus {
#ifndef NDEBUGOUT
  bool dumping = true;
#else
  bool dumping = false;
#endif

  inline void advlistener() {
    if (inptr >= &ringbuf[RINGBUF_SIZE - 1])
      inptr = &ringbuf[0];
    else
      inptr = (bufobj*) (inptr + 1);
    if (inptr == outptr) {
      boost::unique_lock<boost::shared_mutex> lock(
          listenermut); // if buffer is overrun, then block anyways rather than
                        // lose data
      outptr->doit();
      if (outptr >= &ringbuf[RINGBUF_SIZE - 1])
        outptr = &ringbuf[0];
      else
        ++outptr;
    }
    listenercond.notify_one();
  }

  void scoped_ring::off() {
    if (!listening)
      return;
    {
      {
        boost::unique_lock<boost::shared_mutex> xxx(listenermut);
        listening = false;
      }
      listenercond.notify_one();
    }
    inthread.join();
  }
} // namespace fomus

void fomus_rt(int rt) {
  ENTER_API;
  if (rt) {
    if (!listening) {
      ringbuf.resize(RINGBUF_SIZE);
      inptr = outptr = &ringbuf[0];
      listening = true;
      inthread = boost::thread(listener);
    }
  } else {
    if (listening)
      ringbuf.off();
  }
  EXIT_API_VOID;
}

void fomus_flush() {
  ENTER_MAINAPIENT;
  checkinit();
  EXIT_API_VOID;
}

namespace fomus {
  void fomus_ivalaux(FOMUS f, int par, int act, fomus_int val) {
    assert(((fomusdata*) f)->isvalid());
    if (dumping)
      fout << "    - " << paramtostr(par) << ' ' << actiontostr(act) << "  "
           << val << std::endl;
    if (((fomusdata*) f)->queueing())
      ((fomusdata*) f)->store(new apiqueue_i(par, act, val));
    else {
      switch (par) {
      case fomus_par_events:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setmerge(val);
          return;
        default:;
        }
        break;
      case fomus_par_list:
        switch (act) {
        case fomus_act_start:
          ((fomusdata*) f)->startlist(); // go to next one--insert the val
        case fomus_act_add:
          ((fomusdata*) f)->inserti(val);
          return;
        default:;
        }
        break;
      case fomus_par_setting: // integer only!
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->param_settingid(val);
          return; // identify setting by id
        default:;
        }
        break;
      case fomus_par_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setiset(val);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setisetapp(val);
          return;
        default:;
        }
        break;
      case fomus_par_import_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_import_setival(val);
          return;
        default:;
        }
        break;
      case fomus_par_export_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_export_setival(val);
          return;
        default:;
        }
        break;
      case fomus_par_clef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_clef_setival(val);
          return;
        default:;
        }
        break;
      case fomus_par_staff_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_staff_setival(val);
          return;
        default:;
        }
        break;
      case fomus_par_percinst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_setival(val);
          return;
        default:;
        }
        break;
      case fomus_par_inst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_setival(val);
          return;
        default:;
        }
        break;
      case fomus_par_part_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_setival(val);
          return;
        default:;
        }
        break;
      case fomus_par_partmap_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_setival(val);
          return;
        default:;
        }
        break;
      case fomus_par_metapart_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_mpart_setival(val);
          return;
        default:;
        }
        break;
      case fomus_par_measdef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef_setival(val);
          return;
        default:;
        }
        break;
      case fomus_par_region: // integer only!
        switch (act) {
        case fomus_act_start:
          ((fomusdata*) f)->endsetregion(val);
          return;
        case fomus_act_end:
          ((fomusdata*) f)->endregion(val);
          return;
        default:;
        }
        break;
      case fomus_par_locline: // integer only!
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->param_line(val);
          return;
        default:;
        }
        break;
      case fomus_par_loccol: // integer only!
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->param_col(val);
          return;
        default:;
        }
        break;
      case fomus_par_time:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setoffs(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incoffs(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decoffs(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multoffs(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divoffs(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_time:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setoff(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incoff(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decoff(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multoff(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divoff(val);
          return;
        default:;
        }
        break;
      case fomus_par_gracetime:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setgroff(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incgroff(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decgroff(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multgroff(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divgroff(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_gracetime:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setgroff(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incgroff(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decgroff(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multgroff(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divgroff(val);
          return;
        default:;
        }
        break;
      case fomus_par_duration:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setdur(val);
          return;
        case fomus_act_end:
          ((fomusdata*) f)->setendtime(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incdur(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decdur(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multdur(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divdur(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_duration:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setdur(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incdur(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decdur(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multdur(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divdur(val);
          return;
        default:;
        }
        break;
      case fomus_par_pitch:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setpitch(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incpitch(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decpitch(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multpitch(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divpitch(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_pitch:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setpitch(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incpitch(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decpitch(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multpitch(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divpitch(val);
          return;
        default:;
        }
        break;
      case fomus_par_dynlevel:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setdyn(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incdyn(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decdyn(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multdyn(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divdyn(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_dynlevel:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setdyn(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incdyn(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decdyn(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multdyn(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divdyn(val);
          return;
        default:;
        }
        break;
      case fomus_par_voice:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setvoices(val);
          return;
        case fomus_act_add:
          ((fomusdata*) f)->addvoices(val);
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->decvoices(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_voice:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setvoices(val);
          return;
        case fomus_act_add:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->addvoices(val);
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decvoices(val);
          return;
        default:;
        }
        break;
      case fomus_par_markval:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->setmarkval(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setiset_note(val);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setisetapp_note(val);
          return;
        default:;
        }
        break;
      case fomus_par_note_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setiset_note(val);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setisetapp_note(val);
          return;
        default:;
        }
        break;
      case fomus_par_meas_measdef:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef(numbtostr(val));
          return; // enter the note
        default:;
        }
        break;
        break;
      case fomus_par_percinst_template:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_template(numbtostr(val));
          return;
        default:;
        }
        break;
      case fomus_par_percinst_id:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_id(numbtostr(val));
          return;
        default:;
        }
        break;
      case fomus_par_inst_template:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_template(numbtostr(val));
          return;
        default:;
        }
        break;
      case fomus_par_inst_id:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_id(numbtostr(val));
          return;
        default:;
        }
        break;
      case fomus_par_inst_percinsts:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_instr_percinstr(numbtostr(val));
          return; // (add id to list)
        default:;
        }
        break;
      case fomus_par_part_id:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_id(numbtostr(val));
          return;
        default:;
        }
        break;
      case fomus_par_part_inst:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_instr(numbtostr(val));
          return;
        default:;
        }
        break;
      case fomus_par_partmap_part:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_part(numbtostr(val));
          return;
        default:;
        }
        break;
      case fomus_par_partmap_metapart:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_metapart(numbtostr(val));
          return;
        default:;
        }
        break;
      case fomus_par_metapart_id:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_mpart_id(numbtostr(val));
          return;
        default:;
        }
        break;
      case fomus_par_metapart_partmaps:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_mpart_partmap(numbtostr(val).c_str());
          return;
        default:;
        }
        break;
      case fomus_par_measdef_id:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef_id(numbtostr(val));
          return;
        default:;
        }
        break;
      case fomus_par_part: // set the current part
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setpart(numbtostr(val));
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->rem_part(numbtostr(val));
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_metapart: // set the current part
        switch (act) {
        case fomus_act_remove:
          ((fomusdata*) f)->rem_metapart(numbtostr(val));
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_percinst:
        switch (act) {
        case fomus_act_remove:
          ((fomusdata*) f)->rem_percinst(numbtostr(val));
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_inst:
        switch (act) {
        case fomus_act_remove:
          ((fomusdata*) f)->rem_inst(numbtostr(val));
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_measdef:
        switch (act) {
        case fomus_act_remove:
          ((fomusdata*) f)->rem_measdef(numbtostr(val));
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_markid:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->addmark(numbtostr(val));
          return;
        default:;
        }
        break;
      default:;
      }
      throwinvalid(((fomusdata*) f)->getpos());
    }
  }
} // namespace fomus
void fomus_ival(FOMUS f, int par, int act, fomus_int val) {
  assert(((fomusdata*) f)->isvalid());
  if (listening) {
    resetfomuserr();
    inptr->set(f, par, act, val);
    advlistener();
    return;
  }
  ENTER_MAINAPIENT;
  checkinit();
  fomus_ivalaux(f, par, act, val);
  EXIT_API_VOID;
}

namespace fomus {
  void fomus_rvalaux(FOMUS f, int par, int act, fomus_int num, fomus_int den) {
    assert(((fomusdata*) f)->isvalid());
    if (dumping)
      fout << "    - " << paramtostr(par) << ' ' << actiontostr(act) << "  "
           << num << '/' << den << std::endl;
    if (((fomusdata*) f)->queueing())
      ((fomusdata*) f)->store(new apiqueue_r(par, act, num, den));
    else {
      if (den == 0)
        ((fomusdata*) f)->throwfpe(); // check for den = 0
      switch (par) {
      case fomus_par_events:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setmerge(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_list:
        switch (act) {
        case fomus_act_start:
          ((fomusdata*) f)->startlist(); // go to next one--insert the val
        case fomus_act_add:
          ((fomusdata*) f)->insertr(num, den);
          return;
        default:;
        }
        break;
      case fomus_par_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setrset(num, den);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setrsetapp(num, den);
          return;
        default:;
        }
        break;
      case fomus_par_import_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_import_setrval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_export_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_export_setrval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_clef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_clef_setrval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_staff_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_staff_setrval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_percinst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_setrval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_inst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_setrval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_part_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_setrval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_partmap_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_setrval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_metapart_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_mpart_setrval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_measdef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef_setrval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_time:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setoffs(rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incoffs(rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decoffs(rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multoffs(rat(num, den));
          return;
        case fomus_act_div:
          if (num == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divoffs(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_region_time:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setoff(rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incoff(rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decoff(rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multoff(rat(num, den));
          return;
        case fomus_act_div:
          if (num == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divoff(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_gracetime:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setgroff(rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incgroff(rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decgroff(rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multgroff(rat(num, den));
          return;
        case fomus_act_div:
          if (num == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divgroff(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_region_gracetime:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setgroff(rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incgroff(rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decgroff(rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multgroff(rat(num, den));
          return;
        case fomus_act_div:
          if (num == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divgroff(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_duration:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setdur(rat(num, den));
          return;
        case fomus_act_end:
          ((fomusdata*) f)->setendtime(rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incdur(rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decdur(rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multdur(rat(num, den));
          return;
        case fomus_act_div:
          if (num == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divdur(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_region_duration:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setdur(rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incdur(rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decdur(rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multdur(rat(num, den));
          return;
        case fomus_act_div:
          if (num == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divdur(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_pitch:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setpitch(rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incpitch(rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decpitch(rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multpitch(rat(num, den));
          return;
        case fomus_act_div:
          if (num == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divpitch(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_region_pitch:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setpitch(rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incpitch(rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decpitch(rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multpitch(rat(num, den));
          return;
        case fomus_act_div:
          if (num == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divpitch(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_dynlevel:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setdyn(rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incdyn(rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decdyn(rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multdyn(rat(num, den));
          return;
        case fomus_act_div:
          if (num == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divdyn(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_region_dynlevel:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setdyn(rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incdyn(rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decdyn(rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multdyn(rat(num, den));
          return;
        case fomus_act_div:
          if (num == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divdyn(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_voice:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setvoices(rat(num, den));
          return;
        case fomus_act_add:
          ((fomusdata*) f)->addvoices(rat(num, den));
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->decvoices(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_region_voice:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setvoices(rat(num, den));
          return;
        case fomus_act_add:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->addvoices(rat(num, den));
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decvoices(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_markval:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->setmarkval(rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_region_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setrset_note(num, den);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setrsetapp_note(num, den);
          return;
        default:;
        }
        break;
      case fomus_par_note_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setrset_note(num, den);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setrsetapp_note(num, den);
          return;
        default:;
        }
        break;
      default:;
      }
      throwinvalid(((fomusdata*) f)->getpos());
    }
  }
} // namespace fomus
void fomus_rval(FOMUS f, int par, int act, fomus_int num, fomus_int den) {
  assert(((fomusdata*) f)->isvalid());
  if (listening) {
    resetfomuserr();
    inptr->set(f, par, act, num, den);
    advlistener();
    return;
  }
  ENTER_MAINAPIENT;
  checkinit();
  fomus_rvalaux(f, par, act, num, den);
  EXIT_API_VOID;
}

namespace fomus {
  void fomus_mvalaux(FOMUS f, int par, int act, fomus_int val, fomus_int num,
                     fomus_int den) {
    assert(((fomusdata*) f)->isvalid());
    if (dumping) {
      fout << "    - " << paramtostr(par) << ' ' << actiontostr(act) << "  "
           << val;
      if (num >= 0)
        fout << '+' << num;
      else
        fout << '-' << -num;
      fout << '/' << den << std::endl;
    }
    if (((fomusdata*) f)->queueing())
      ((fomusdata*) f)->store(new apiqueue_m(par, act, val, num, den));
    else {
      if (den == 0)
        ((fomusdata*) f)->throwfpe();
      switch (par) {
      case fomus_par_events:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setmerge(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_list:
        switch (act) {
        case fomus_act_start:
          ((fomusdata*) f)->startlist(); // go to next one--insert the val
        case fomus_act_add:
          ((fomusdata*) f)->insertm(val, num, den);
          return;
        default:;
        }
        break;
      case fomus_par_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setmset(val, num, den);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setmsetapp(val, num, den);
          return;
        default:;
        }
        break;
      case fomus_par_import_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_import_setrval(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_export_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_export_setrval(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_clef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_clef_setrval(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_staff_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_staff_setrval(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_percinst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_setrval(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_inst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_setrval(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_part_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_setrval(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_partmap_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_setrval(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_metapart_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_mpart_setrval(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_measdef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef_setrval(val + rat(num, den));
          return;
        default:;
        }
        break;
        // events
      case fomus_par_time:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setoffs(val + rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incoffs(val + rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decoffs(val + rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multoffs(val + rat(num, den));
          return;
        case fomus_act_div: {
          numb v(val + rat(num, den));
          if (v.getnum() == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divoffs(v);
          return;
        }
        default:;
        }
        break;
      case fomus_par_region_time:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setoff(val + rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incoff(val + rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decoff(val + rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multoff(val + rat(num, den));
          return;
        case fomus_act_div: {
          numb v(val + rat(num, den));
          if (v.getnum() == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divoff(v);
          return;
        }
        default:;
        }
        break;
      case fomus_par_gracetime:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setgroff(val + rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incgroff(val + rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decgroff(val + rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multgroff(val + rat(num, den));
          return;
        case fomus_act_div: {
          numb v(val + rat(num, den));
          if (v.getnum() == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divgroff(v);
          return;
        }
        default:;
        }
        break;
      case fomus_par_region_gracetime:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setgroff(val + rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incgroff(val + rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decgroff(val + rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multgroff(val + rat(num, den));
          return;
        case fomus_act_div: {
          numb v(val + rat(num, den));
          if (v.getnum() == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divgroff(v);
          return;
        }
        default:;
        }
        break;
      case fomus_par_duration:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setdur(val + rat(num, den));
          return;
        case fomus_act_end:
          ((fomusdata*) f)->setendtime(val + rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incdur(val + rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decdur(val + rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multdur(val + rat(num, den));
          return;
        case fomus_act_div: {
          numb v(val + rat(num, den));
          if (v.getnum() == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divdur(v);
          return;
        }
        default:;
        }
        break;
      case fomus_par_region_duration:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setdur(val + rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incdur(val + rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decdur(val + rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multdur(val + rat(num, den));
          return;
        case fomus_act_div: {
          numb v(val + rat(num, den));
          if (v.getnum() == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divdur(v);
          return;
        }
        default:;
        }
        break;
      case fomus_par_pitch:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setpitch(val + rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incpitch(val + rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decpitch(val + rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multpitch(val + rat(num, den));
          return;
        case fomus_act_div: {
          numb v(val + rat(num, den));
          if (v.getnum() == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divpitch(v);
          return;
        }
        default:;
        }
        break;
      case fomus_par_region_pitch:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setpitch(val + rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incpitch(val + rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decpitch(val + rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multpitch(val + rat(num, den));
          return;
        case fomus_act_div: {
          numb v(val + rat(num, den));
          if (v.getnum() == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divpitch(v);
          return;
        }
        default:;
        }
        break;
      case fomus_par_dynlevel:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setdyn(val + rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incdyn(val + rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decdyn(val + rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multdyn(val + rat(num, den));
          return;
        case fomus_act_div: {
          numb v(val + rat(num, den));
          if (v.getnum() == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divdyn(v);
          return;
        }
        default:;
        }
        break;
      case fomus_par_region_dynlevel:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setdyn(val + rat(num, den));
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incdyn(val + rat(num, den));
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decdyn(val + rat(num, den));
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multdyn(val + rat(num, den));
          return;
        case fomus_act_div: {
          numb v(val + rat(num, den));
          if (v.getnum() == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divdyn(v);
          return;
        }
        default:;
        }
        break;
      case fomus_par_voice:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setvoices(val + rat(num, den));
          return;
        case fomus_act_add:
          ((fomusdata*) f)->addvoices(val + rat(num, den));
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->decvoices(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_region_voice:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setvoices(val + rat(num, den));
          return;
        case fomus_act_add:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->addvoices(val + rat(num, den));
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decvoices(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_markval:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->setmarkval(val + rat(num, den));
          return;
        default:;
        }
        break;
      case fomus_par_region_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setmset_note(val, num, den);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setmsetapp_note(val, num, den);
          return;
        default:;
        }
        break;
      case fomus_par_note_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setmset_note(val, num, den);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setmsetapp_note(val, num, den);
          return;
        default:;
        }
        break;
      default:;
      }
      throwinvalid(((fomusdata*) f)->getpos());
    }
  }
} // namespace fomus
void fomus_mval(FOMUS f, int par, int act, fomus_int val, fomus_int num,
                fomus_int den) {
  assert(((fomusdata*) f)->isvalid());
  if (listening) {
    resetfomuserr();
    inptr->set(f, par, act, val, num, den);
    advlistener();
    return;
  }
  ENTER_MAINAPIENT;
  checkinit();
  fomus_mvalaux(f, par, act, val, num, den);
  EXIT_API_VOID;
}

namespace fomus {
  void fomus_fvalaux(FOMUS f, int par, int act, fomus_float val) {
    assert(((fomusdata*) f)->isvalid());
    if (dumping)
      fout << "    - " << paramtostr(par) << ' ' << actiontostr(act) << "  "
           << val << std::endl;
    if (((fomusdata*) f)->queueing())
      ((fomusdata*) f)->store(new apiqueue_f(par, act, val));
    else {
      switch (par) {
      case fomus_par_events:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setmerge(val);
          return;
        default:;
        }
        break;
      case fomus_par_list:
        switch (act) {
        case fomus_act_start:
          ((fomusdata*) f)->startlist(); // go to next one--insert the val
        case fomus_act_add:
          ((fomusdata*) f)->insertf(val);
          return;
        default:;
        }
        break;
      case fomus_par_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setfset(val);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setfsetapp(val);
          return;
        default:;
        }
        break;
      case fomus_par_import_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_import_setfval(val);
          return;
        default:;
        }
        break;
      case fomus_par_export_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_export_setfval(val);
          return;
        default:;
        }
        break;
      case fomus_par_clef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_clef_setfval(val);
          return;
        default:;
        }
        break;
      case fomus_par_staff_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_staff_setfval(val);
          return;
        default:;
        }
        break;
      case fomus_par_percinst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_setfval(val);
          return;
        default:;
        }
        break;
      case fomus_par_inst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_setfval(val);
          return;
        default:;
        }
        break;
      case fomus_par_part_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_setfval(val);
          return;
        default:;
        }
        break;
      case fomus_par_partmap_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_setfval(val);
          return;
        default:;
        }
        break;
      case fomus_par_metapart_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_mpart_setfval(val);
          return;
        default:;
        }
        break;
      case fomus_par_measdef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef_setfval(val);
          return;
        default:;
        }
        break;
      case fomus_par_time:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setoffs(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incoffs(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decoffs(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multoffs(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divoffs(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_time:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setoff(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incoff(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decoff(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multoff(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divoff(val);
          return;
        default:;
        }
        break;
      case fomus_par_gracetime:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setgroff(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incgroff(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decgroff(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multgroff(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divgroff(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_gracetime:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setgroff(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incgroff(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decgroff(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multgroff(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divgroff(val);
          return;
        default:;
        }
        break;
      case fomus_par_duration:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setdur(val);
          return;
        case fomus_act_end:
          ((fomusdata*) f)->setendtime(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incdur(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decdur(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multdur(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divdur(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_duration:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setdur(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incdur(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decdur(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multdur(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divdur(val);
          return;
        default:;
        }
        break;
      case fomus_par_pitch:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setpitch(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incpitch(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decpitch(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multpitch(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divpitch(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_pitch:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setpitch(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incpitch(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decpitch(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multpitch(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divpitch(val);
          return;
        default:;
        }
        break;
      case fomus_par_dynlevel:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setdyn(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->incdyn(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->decdyn(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->multdyn(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->divdyn(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_dynlevel:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setdyn(val);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->incdyn(val);
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decdyn(val);
          return;
        case fomus_act_mult:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->multdyn(val);
          return;
        case fomus_act_div:
          if (val == 0)
            ((fomusdata*) f)->throwfpe();
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->divdyn(val);
          return;
        default:;
        }
        break;
      case fomus_par_voice:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setvoices(val);
          return;
        case fomus_act_add:
          ((fomusdata*) f)->addvoices(val);
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->decvoices(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_voice:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setvoices(val);
          return;
        case fomus_act_add:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->addvoices(val);
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decvoices(val);
          return;
        default:;
        }
        break;
      case fomus_par_markval:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->setmarkval(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setfset_note(val);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setfsetapp_note(val);
          return;
        default:;
        }
        break;
      case fomus_par_note_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setfset_note(val);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setfsetapp_note(val);
          return;
        default:;
        }
        break;
      default:;
      }
      throwinvalid(((fomusdata*) f)->getpos());
    }
  }
} // namespace fomus
void fomus_fval(FOMUS f, int par, int act, fomus_float val) {
  assert(((fomusdata*) f)->isvalid());
  if (listening) {
    resetfomuserr();
    inptr->set(f, par, act, val);
    advlistener();
    return;
  }
  ENTER_MAINAPIENT;
  checkinit();
  fomus_fvalaux(f, par, act, val);
  EXIT_API_VOID;
}

namespace fomus {
  void fomus_svalaux(FOMUS f, int par, int act, const char* val) {
    assert(((fomusdata*) f)->isvalid());
    if (dumping)
      fout << "    - " << paramtostr(par) << ' ' << actiontostr(act) << "  \""
           << val << '"' << std::endl;
    if (((fomusdata*) f)->queueing())
      ((fomusdata*) f)->store(new apiqueue_s(par, act, val));
    else {
      switch (par) {
      case fomus_par_pitch:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setpitch(val);
          return;
        default:;
        }
        break;
      case fomus_par_duration:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setdur(val);
          return;
        default:;
        }
        break;
      case fomus_par_list:
        switch (act) {
        case fomus_act_start:
          ((fomusdata*) f)->startlist(); // go to next one--insert the val
        case fomus_act_add:
          ((fomusdata*) f)->inserts(val);
          return;
        default:;
        }
        break;
      case fomus_par_setting:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->param_settingid(val);
          return;
        default:;
        }
        break;
      case fomus_par_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setsset(val);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setssetapp(val);
          return;
        default:;
        }
        break;
      case fomus_par_meas_measdef:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef(val);
          return; // enter the note
        default:;
        }
        break;
        break;
      case fomus_par_import_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_import_setsval(val);
          return;
        default:;
        }
        break;
      case fomus_par_export_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_export_setsval(val);
          return;
        default:;
        }
        break;
      case fomus_par_clef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_clef_setsval(val);
          return;
        default:;
        }
        break;
      case fomus_par_staff_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_staff_setsval(val);
          return;
        default:;
        }
        break;
      case fomus_par_percinst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_setsval(val);
          return;
        default:;
        }
        break;
      case fomus_par_inst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_setsval(val);
          return;
        default:;
        }
        break;
      case fomus_par_part_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_setsval(val);
          return;
        default:;
        }
        break;
      case fomus_par_partmap_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_setsval(val);
          return;
        default:;
        }
        break;
      case fomus_par_metapart_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_mpart_setsval(val);
          return;
        default:;
        }
        break;
      case fomus_par_measdef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef_setsval(val);
          return;
        default:;
        }
        break;
      case fomus_par_percinst_template:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_template(val);
          return;
        default:;
        }
        break;
      case fomus_par_percinst_id:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_id(val);
          return;
        default:;
        }
        break;
      case fomus_par_inst_template:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_template(val);
          return;
        default:;
        }
        break;
      case fomus_par_inst_id:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_id(val);
          return;
        default:;
        }
        break;
      case fomus_par_inst_percinsts:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_instr_percinstr(val);
          return; // (add id to list)
        default:;
        }
        break;
      case fomus_par_part_id:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_id(val);
          return;
        default:;
        }
        break;
      case fomus_par_part_inst:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_instr(val);
          return;
        default:;
        }
        break;
      case fomus_par_partmap_part:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_part(val);
          return;
        default:;
        }
        break;
      case fomus_par_partmap_metapart:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_metapart(val);
          return;
        default:;
        }
        break;
      case fomus_par_metapart_id:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_mpart_id(val);
          return;
        default:;
        }
        break;
      case fomus_par_metapart_partmaps:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_mpart_partmap(val);
          return;
        default:;
        }
        break;
      case fomus_par_measdef_id:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef_id(val);
          return;
        default:;
        }
        break;
      case fomus_par_locfile:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->param_file(val);
          return;
        default:;
        }
        break;
      case fomus_par_part: // set the current part
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setpart(val);
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->rem_part(val);
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_metapart: // set the current part
        switch (act) {
        case fomus_act_remove:
          ((fomusdata*) f)->rem_metapart(val);
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_percinst:
        switch (act) {
        case fomus_act_remove:
          ((fomusdata*) f)->rem_percinst(val);
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_inst:
        switch (act) {
        case fomus_act_remove:
          ((fomusdata*) f)->rem_inst(val);
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_measdef:
        switch (act) {
        case fomus_act_remove:
          ((fomusdata*) f)->rem_measdef(val);
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_markid:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->addmark(val);
          return;
        default:;
        }
        break;
      case fomus_par_markval:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->setmarkval(val);
          return;
        default:;
        }
        break;
      case fomus_par_region_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setsset_note(val);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setssetapp_note(val);
          return;
        default:;
        }
        break;
      case fomus_par_note_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setsset_note(val);
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setssetapp_note(val);
          return;
        default:;
        }
        break;
      default:;
      }
      throwinvalid(((fomusdata*) f)->getpos());
    }
  }
} // namespace fomus
void fomus_sval(FOMUS f, int par, int act, const char* val) {
  assert(((fomusdata*) f)->isvalid());
  if (listening) {
    resetfomuserr();
    inptr->set(f, par, act, val);
    advlistener();
    return;
  }
  ENTER_MAINAPIENT;
  checkinit();
  fomus_svalaux(f, par, act, val);
  EXIT_API_VOID;
}

namespace fomus {
  void fomus_actaux(FOMUS f, int par, int act) {
    assert(((fomusdata*) f)->isvalid());
    assert(paramtostr(fomus_par_markevent) == std::string("markevent"));
    assert(actiontostr(fomus_act_resume) == std::string("resume"));
    if (dumping)
      fout << "    - " << paramtostr(par) << ' ' << actiontostr(act)
           << std::endl;
    if (par == fomus_par_entry) {
      switch (act) {
      case fomus_act_queue:
        ((fomusdata*) f)->startqueue();
        return;
      case fomus_act_cancel:
        ((fomusdata*) f)->cancelqueue();
        return;
      case fomus_act_resume:
        ((fomusdata*) f)->resumequeue();
        return;
      default:;
      }
    }
    if (((fomusdata*) f)->queueing())
      ((fomusdata*) f)->store(new apiqueuebase(par, act));
    else {
      switch (par) {
      case fomus_par_events:
        switch (act) {
        case fomus_act_clear:
          ((fomusdata*) f)->clearallnotes();
          return; // enter the note
        default:;
        }
        break;
      case fomus_par_time:
        switch (act) {
        case fomus_act_inc:
          ((fomusdata*) f)->incoffbyldur();
          return; // enter the note
        default:;
        }
        break;
      case fomus_par_gracetime:
        switch (act) {
        case fomus_act_inc:
          ((fomusdata*) f)->incgroffbyldur();
          return; // enter the note
        default:;
        }
        break;
      case fomus_par_duration:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setdur((fint) 0);
          return;
        case fomus_act_inc:
          ((fomusdata*) f)->setpointrt();
          return;
        case fomus_act_dec:
          ((fomusdata*) f)->setpointlt();
          return;
        default:;
        }
        break;
      case fomus_par_noteevent:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setblastnote(fomus_par_noteevent);
          return; // enter the note
        case fomus_act_add:
          ((fomusdata*) f)->blastnote();
          return; // enter the note
        default:;
        }
        break;
      case fomus_par_restevent:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setblastnote(fomus_par_restevent);
          return; // enter the note
        case fomus_act_add:
          ((fomusdata*) f)->setblastnote(fomus_par_restevent);
          ((fomusdata*) f)->blastnote();
          return; // enter the note
        default:;
        }
        break;
      case fomus_par_markevent:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setblastnote(fomus_par_markevent);
          return; // enter the note
        case fomus_act_add:
          ((fomusdata*) f)->setblastnote(fomus_par_markevent);
          ((fomusdata*) f)->blastnote();
          return; // enter the note
        default:;
        }
        break;
      case fomus_par_measevent:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setblastnote(fomus_par_measevent);
          return; // enter the note
        case fomus_act_add:
          ((fomusdata*) f)->setblastnote(fomus_par_measevent);
          ((fomusdata*) f)->blastnote();
          return; // enter the note
        default:;
        }
        break;
      case fomus_par_meas:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->setblastnote(fomus_par_meas);
          ((fomusdata*) f)->blastnote();
          return; // enter the note
        default:;
        }
        break;
        break;
      case fomus_par_voice:
        switch (act) {
        case fomus_act_clear:
          ((fomusdata*) f)->clearvoiceslist();
          return;
        case fomus_act_set:
          ((fomusdata*) f)->setvoiceslist();
          return;
        case fomus_act_add:
          ((fomusdata*) f)->addvoiceslist();
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->decvoiceslist();
          return;
        default:;
        }
        break;
      case fomus_par_region_voice:
        switch (act) {
        case fomus_act_clear:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->clearvoiceslist();
          return;
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setvoiceslist();
          return;
        case fomus_act_add:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->addvoiceslist();
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decvoiceslist();
          return;
        default:;
        }
        break;
      case fomus_par_region_voicelist:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setvoiceslisttype(fomus_act_set);
          return;
        case fomus_act_add:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setvoiceslisttype(fomus_act_add);
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setvoiceslisttype(fomus_act_remove);
          return;
        default:;
        }
        break;
      case fomus_par_meas_measdef:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef(); /*((fomusdata*)f)->blastmeas();*/
          return;                          // enter the note
        default:;
        }
        break;
        break;
      case fomus_par_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setlset();
          return; // set the list
        case fomus_act_append:
          ((fomusdata*) f)->appendlset();
          return; // set the list
        case fomus_act_clear:
          ((fomusdata*) f)->reset_set();
          return;
        default:;
        }
        break;
      case fomus_par_list:
        switch (act) {
        case fomus_act_start:
          ((fomusdata*) f)->startlist();
          return;
        case fomus_act_end:
          ((fomusdata*) f)->endlist(true);
          return;
        case fomus_act_add:
          ((fomusdata*) f)->endlist(false);
          return;
        default:;
        }
        break;
      case fomus_par_list_percinsts:
        switch (act) {
        case fomus_act_start:
          ((fomusdata*) f)->listclearpercinst();
          return;
        case fomus_act_add:
          ((fomusdata*) f)->listaddpercinst();
          return;
        case fomus_act_end:
          return; // don't need to do anything with this list
        default:;
        }
        break;
      case fomus_par_settingval_percinsts:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->listsetpercinst();
          return; // set the list
        case fomus_act_append:
          ((fomusdata*) f)->listappendpercinst();
          return; // set the list
        default:;
        }
        break;
      case fomus_par_list_insts:
        switch (act) {
        case fomus_act_start:
          ((fomusdata*) f)->listclearinst();
          return;
        case fomus_act_add:
          ((fomusdata*) f)->listaddinst();
          return;
        case fomus_act_end:
          return;
        default:;
        }
        break;
      case fomus_par_settingval_insts:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->listsetinst();
          return; // insert a list
        case fomus_act_append:
          ((fomusdata*) f)->listappendinst();
          return; // insert a list
        default:;
        }
        break;
      case fomus_par_import_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_import_setlval();
          return;
        default:;
        }
        break;
      case fomus_par_export_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_export_setlval();
          return;
        default:;
        }
        break;
      case fomus_par_clef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_clef_setlval();
          return;
        default:;
        }
        break;
      case fomus_par_staff_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_staff_setlval();
          return;
        default:;
        }
        break;
      case fomus_par_percinst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_setlval();
          return;
        default:;
        }
        break;
      case fomus_par_inst_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_setlval();
          return;
        default:;
        }
        break;
      case fomus_par_part_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_setlval();
          return;
        default:;
        }
        break;
      case fomus_par_partmap:
        switch (act) {
        case fomus_act_start:
          ((fomusdata*) f)->set_partmap_start();
          return;
        default:;
        }
        break;
      case fomus_par_partmap_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_setlval();
          return;
        default:;
        }
        break;
      case fomus_par_metapart_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_mpart_setlval();
          return;
        default:;
        }
        break;
      case fomus_par_measdef_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_measdef_setlval();
          return;
        default:;
        }
        break;
      case fomus_par_staff_clefs:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_staff_clefs();
          return;
        default:;
        }
        break;
      case fomus_par_percinst_imports:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_percinstr_imports();
          return;
        default:;
        }
        break;
      case fomus_par_percinst_export:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_percinstr_export();
          return;
        default:;
        }
        break;
      case fomus_par_inst_staves:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_instr_staff();
          return;
        default:;
        }
        break;
      case fomus_par_inst_imports:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_instr_imports();
          return;
        default:;
        }
        break;
      case fomus_par_inst_export:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_instr_export();
          return;
        default:;
        }
        break;
      case fomus_par_inst_percinsts:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_instr_percinstr();
          return;
        default:;
        }
        break;
      case fomus_par_part_inst:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_part_instr();
          return;
        default:;
        }
        break;
      case fomus_par_partmap_part:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_part();
          return;
        default:;
        }
        break;
      case fomus_par_partmap_metapart:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->set_partmap_metapart();
          return;
        default:;
        }
        break;
      case fomus_par_metapart_partmaps:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_mpart_partmap();
          return;
        default:;
        }
        break;
      case fomus_par_part:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_part();
          return; // <<<<<<<<<<<<
        default:;
        }
        break;
      case fomus_par_metapart:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_metapart();
          return;
        default:;
        }
        break;
      case fomus_par_percinst:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_percinst();
          return;
        default:;
        }
        break;
      case fomus_par_inst:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_inst();
          return;
        default:;
        }
        break;
      case fomus_par_measdef:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->ins_measdef();
          return;
        default:;
        }
        break;
      case fomus_par_mark:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->setmarklist();
          return;
        default:;
        }
        break;
      case fomus_par_region_mark:
        switch (act) {
        case fomus_act_add:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setmarklist();
          return;
        case fomus_act_remove:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->decmarklist();
          return;
        default:;
        }
        break;
      case fomus_par_region_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->setlset_note();
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->appendlset_note();
          return;
        default:;
        }
        break;
      case fomus_par_note_settingval:
        switch (act) {
        case fomus_act_set:
          ((fomusdata*) f)->setlset_note();
          return;
        case fomus_act_append:
          ((fomusdata*) f)->setregion();
          ((fomusdata*) f)->appendlset_note();
          return;
        default:;
        }
        break;
      default:;
      }
      throwinvalid(((fomusdata*) f)->getpos());
    }
  }
} // namespace fomus
void fomus_act(FOMUS f, int par, int act) {
  assert(((fomusdata*) f)->isvalid());
  if (listening) {
    resetfomuserr();
    inptr->set(f, par, act);
    advlistener();
    return;
  }
  ENTER_MAINAPIENT;
  checkinit();
  fomus_actaux(f, par, act);
  EXIT_API_VOID;
}

fomus_int fomus_get_ival(FOMUS f, const char* set) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  return f ? ((fomusdata*) f)->get_ival(set) : getdefaultival(set);
  EXIT_API_0;
}
struct fomus_rat fomus_get_rval(FOMUS f, const char* set) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  return tofrat(f ? ((fomusdata*) f)->get_rval(set) : getdefaultrval(set));
  EXIT_API_RAT0;
}
fomus_float fomus_get_fval(FOMUS f, const char* set) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  return f ? ((fomusdata*) f)->get_fval(set) : getdefaultfval(set);
  EXIT_API_0;
}
const char* fomus_get_sval(FOMUS f, const char* set) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  return f ? ((fomusdata*) f)->get_sval(set).c_str()
           : getdefaultsval(set).c_str();
  EXIT_API_0;
}

void fomus_parse(FOMUS f, const char* input) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  threadfd.reset((fomusdata*) f);
  modsvect_it i(std::find_if(
      mods.begin(), mods.end(),
      boost::lambda::bind(&modbase::modin_hasext, boost::lambda::_1, "fms")));
  if (i == mods.end()) {
    CERR << "cannot parse input, module `fmsin' not found" << std::endl;
    throw errbase();
  }
  const modbase& mo = *i;
  moddata d(mo, mo.getdata(f));
  if (mo.loadfile(f, d.get(), input, false))
    throw errbase(); // throw badfile();
  EXIT_API_VOID;
}

void fomus_load(FOMUS f, const char* filename) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  threadfd.reset((fomusdata*) f);
  boost::filesystem::path cur(boost::filesystem::current_path());
  assert(cur.FS_IS_COMPLETE());
  modsvect_constit it(
      std::find_if(mods.begin(), mods.end(),
                   boost::lambda::bind(&modbase::modin_hasloadid,
                                       boost::lambda::_1, filename)));
  if (it != mods.end()) { // not a filename, it's a load id
    const modbase& mo = *it;
    moddata d(mo, mo.getdata(f));
    if (getdefaultival(VERBOSE_ID) >= 1)
      fout << "loading `" << filename << "'..." << std::endl;
    mo.loadfile(f, d.get(), filename, true);
  } else {
    try {
      DBG("filename = " << filename << " is_complete = "
                        << boost::filesystem::path(filename).is_complete()
                        << " has root name = "
                        << boost::filesystem::path(filename).has_root_name()
                        << std::endl);
      boost::filesystem::path fn(FS_COMPLETE(filename, cur));
      const modbase* mo;
      {
        if (!boost::filesystem::exists(fn)) {
          CERR << "input file `" << fn.FS_FILE_STRING() << "' doesn't exist"
               << std::endl;
          throw errbase();
        }
        std::string ext(FS_EXTENSION(fn));
        boost::trim_left_if(ext, boost::lambda::_1 == '.');
        const listelmap& exts(((fomusdata*) f)->get_map(INPUTMOD_ID));
        listelmap_constit x(exts.find(ext));
        if (x == exts.end()) {
          x = exts.find(boost::to_lower_copy(ext));
        } else
          goto SKIPIF;
        if (x != exts.end()) { // user specifies a module
        SKIPIF:
          modsmap_constit mbi(modsbyname.find(listel_getstring(x->second)));
          if (mbi == modsbyname.end()) {
            CERR << "invalid module name `" << x->second
                 << "' in setting `mod-input'" << std::endl;
            throw errbase();
          }
          mo = mbi->second;
        } else { // search for module
          modsvect_it i(std::find_if(
              mods.begin(), mods.end(),
              boost::lambda::bind(&modbase::modin_hasext, boost::lambda::_1,
                                  boost::lambda::constant_ref(ext))));
          if (i == mods.end()) {
            i = std::find_if(
                mods.begin(), mods.end(),
                boost::lambda::bind(
                    &modbase::modin_hasext, boost::lambda::_1,
                    boost::lambda::constant_ref(boost::to_lower_copy(ext))));
            if (i == mods.end()) {
              CERR << "cannot load file of type `." << ext << '\'' << std::endl;
              throw errbase();
            }
          }
          mo = &*i;
        }
      }
      moddata d(*mo, mo->getdata(f));
      ((fomusdata*) f)->setfilename(filename);
      if (getdefaultival(VERBOSE_ID) >= 1)
        fout << "loading input file `"
             << boost::filesystem::path(filename).FS_FILE_STRING() << "'..."
             << std::endl;
      if (mo->loadfile(f, d.get(), fn.FS_FILE_STRING().c_str(), true))
        throw errbase(); // throw badfile();
    } catch (const boost::filesystem::filesystem_error& e) {
      CERR << "invalid filename `" << filename << '\'' << std::endl;
      throw errbase();
    }
  }
  EXIT_API_VOID;
}

void fomus_run(FOMUS f) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  threadfd.reset((fomusdata*) f);
  try {
    boost::filesystem::path cur(boost::filesystem::current_path());
    boost::filesystem::path fn;
    std::vector<runpair> mds;
    std::string ou(
        ((fomusdata*) f)
            ->get_sval(
                FILENAME_ID)); // std:: so boost::filesystem can deal with it
    {
      listelvect exts(((fomusdata*) f)->get_vect(OUTPUTS_ID));
      if (ou.empty()) {
        ou = ((fomusdata*) f)
                 ->getfilename(); // no output file, make it = input file + ext
        if (ou.empty()) {
          CERR << "no output filename given" << std::endl;
          throw errbase();
        }
        fn = FS_COMPLETE(ou /*.c_str()*/, cur); // the first input filename used
      } else {
        fn = FS_COMPLETE(ou /*.c_str()*/, cur);
        std::string ext(
            FS_EXTENSION(fn) /*.c_str()*/); // if output file has ext, append it
                                            // to the list of exts
        boost::trim_left_if(ext, boost::lambda::_1 == '.');
        if (!ext.empty())
          exts.insert(exts.begin(), ext);
      }
      const listelmap& xm(((fomusdata*) f)->get_map(OUTPUTMOD_ID));
      std::set<std::string> exts0;
      for (listelvect_constit i(exts.begin()); i != exts.end(); ++i) {
        if (!exts0.insert(listel_getstring(*i)).second)
          continue;
        const std::string ii(listel_getstring(*i));
        listelmap_constit x(xm.find(ii));
        if (x == xm.end()) {
          x = xm.find(boost::to_lower_copy(ii));
        } else
          goto SKIPIF;
        if (x != xm.end()) {
        SKIPIF:
          modsmap_constit mbi(modsbyname.find(listel_getstring(x->second)));
          if (mbi == modsbyname.end()) {
            CERR << "invalid module name `" << x->second
                 << "' in setting `mod-output'" << std::endl;
            throw errbase();
          }
          std::string ex('.' + ii);
          mds.push_back(
              runpair(mbi->second, FS_CHANGE_EXTENSION(fn, ex).FS_FILE_STRING(),
                      FS_CHANGE_EXTENSION(boost::filesystem::path(ou), ex)
                          .FS_FILE_STRING()));
        } else {
          modsvect_it i(std::find_if(
              mods.begin(), mods.end(),
              boost::lambda::bind(&modbase::modout_hasext, boost::lambda::_1,
                                  boost::lambda::constant_ref(ii))));
          if (i == mods.end()) {
            i = find_if(
                mods.begin(), mods.end(),
                bind(&modbase::modout_hasext, boost::lambda::_1,
                     boost::lambda::constant_ref(boost::to_lower_copy(ii))));
            if (i == mods.end()) {
              CERR << "cannot write file of type `." << ii << '\'' << std::endl;
              throw errbase();
            }
          }
          std::string ex('.' + ii);
          mds.push_back(
              runpair(&*i, FS_CHANGE_EXTENSION(fn, ex).FS_FILE_STRING(),
                      FS_CHANGE_EXTENSION(boost::filesystem::path(ou), ex)
                          .FS_FILE_STRING()));
        }
      }
      stable_sort(
          mds.begin(), mds.end(),
          boost::lambda::bind<int>(&runpair::prenum, boost::lambda::_1) <
              boost::lambda::bind<int>(&runpair::prenum, boost::lambda::_2));
    }
    if (mds.empty()) {
      CERR << "no output format specified" << std::endl;
      throw errbase();
    }
    ((fomusdata*) f)->runfomus(mds.begin(), mds.end());
  } catch (const errbase& e) {
    data.erase(data.find(*(fomusdata*) f));
    throw;
  }
  data.erase(data.find(*(fomusdata*) f));
  EXIT_API_VOID;
}

void fomus_save(FOMUS f, const char* filename) {
  ENTER_MAINAPI;
  checkinit();
  assert(((fomusdata*) f)->isvalid());
  threadfd.reset((fomusdata*) f);
  try {
    boost::filesystem::path cur(boost::filesystem::current_path());
    std::vector<runpair> mds;
    std::string ou(filename); // std:: so boost::filesystem can deal with it
    {
      if (ou.empty()) {
        ou = ((fomusdata*) f)
                 ->getfilename(); // no output file, make it = input file + ext
        if (ou.empty()) {
          CERR << "no output filename given" << std::endl;
          throw errbase();
        }
      }
      boost::filesystem::path fn(FS_COMPLETE(ou /*.c_str()*/, cur));
      const std::string ii("fms");
      modsvect_it i(std::find_if(
          mods.begin(), mods.end(),
          boost::lambda::bind(&modbase::modout_hasext, boost::lambda::_1,
                              boost::lambda::constant_ref(ii))));
      if (i == mods.end()) {
        CERR << "cannot write file, module `fmsin' not found" << std::endl;
        throw errbase();
      }
      mds.push_back(runpair(&*i, fn.FS_FILE_STRING(),
                            boost::filesystem::path(ou).FS_FILE_STRING()));
    }
    assert(!mds.empty());
    ((fomusdata*) f)->runfomus(mds.begin(), mds.end());
  } catch (const errbase& e) {
    data.erase(data.find(*(fomusdata*) f));
    throw;
  }
  data.erase(data.find(*(fomusdata*) f));
  EXIT_API_VOID;
}

namespace fomus {

  inline void bufobj::doit() const {
    ENTER_APIRT;
    switch (wh) {
    case buf_ival:
      fomus_ivalaux(fom, par, act, x.val.val1);
      break;
    case buf_rval:
      fomus_rvalaux(fom, par, act, x.val.val1, x.val.val2);
      break;
    case buf_mval:
      fomus_mvalaux(fom, par, act, x.val.val1, x.val.val2, x.val.val3);
      break;
    case buf_fval:
      fomus_fvalaux(fom, par, act, x.fval);
      break;
    case buf_sval:
      fomus_svalaux(fom, par, act, x.str);
      break;
    case buf_act:
      fomus_actaux(fom, par, act);
      break;
    }
    EXIT_API_VOID;
  }

  void listener() {
    boost::unique_lock<boost::shared_mutex> lock(listenermut);
    DBG("realtime listener launched" << std::endl);
    while (true) {
      listenercond.wait(lock);
      if (!listening)
        return;
      catchup();
    }
  }

} // namespace fomus
