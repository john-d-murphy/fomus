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

#include "m_pd.h"

//#include <sstream>
#include <cassert>
#include <string>
#include <vector>

#include "config.h"
#include "fomusapi.h"

namespace pdfomus {

#define NTEXTBUFFERS 128

  std::vector<std::string> pdfomus_textbuffers(NTEXTBUFFERS);
  std::string* volatile pdfomus_inptr;
  std::string* volatile pdfomus_outptr;
  volatile bool pdfomus_sync = true;

  inline void pdfomus_flushout() {
    while (pdfomus_inptr != pdfomus_outptr) {
      post("%s", pdfomus_outptr->c_str());
      if (pdfomus_outptr >= &pdfomus_textbuffers[NTEXTBUFFERS - 1])
        pdfomus_outptr = &pdfomus_textbuffers[0];
      else
        ++pdfomus_outptr;
    }
  }

#define CLOCKDELAY 500

  struct pdfomus_scopedsync {
    pdfomus_scopedsync() {
      fomus_flush();
      pdfomus_sync = true;
    }
    ~pdfomus_scopedsync() {
      pdfomus_sync = false;
    }
  };
} // namespace pdfomus

using namespace pdfomus;

extern "C" {

static t_class* fomus_class;

typedef struct _fomus {
  t_object x_obj; // must be first
  int xxxxx; // when compiling in mingw32, need to put an int between x_obj and
             // fom, probably some kind of alignment issue?
  FOMUS fom;
  t_clock* clock;
} t_fomus;

void pdfomus_flushout(t_fomus* x) {
  pdfomus_flushout();
}

void pdfomus_enternote(t_fomus* x) { // bang
  pdfomus_flushout();
  if (!x->fom)
    return;
  fomus_act(x->fom, fomus_par_noteevent, fomus_act_add);
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}
void pdfomus_enterrest(t_fomus* x) { // bang
  pdfomus_flushout();
  if (!x->fom)
    return;
  fomus_act(x->fom, fomus_par_restevent, fomus_act_add);
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}
void pdfomus_entermark(t_fomus* x) { // bang
  pdfomus_flushout();
  if (!x->fom)
    return;
  fomus_act(x->fom, fomus_par_markevent, fomus_act_add);
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}

void pdfomus_new(t_fomus* x) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  pdfomus_scopedsync xxx;
  fomus_free(x->fom);
  x->fom = 0;
  x->fom = fomus_new();
}

void pdfomus_clear(t_fomus* x) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  pdfomus_scopedsync xxx;
  fomus_act(x->fom, fomus_par_events, fomus_act_clear);
}

void pdfomus_load(t_fomus* x, t_symbol* s) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  pdfomus_scopedsync xxx;
  fomus_load(x->fom, s->s_name);
}

void pdfomus_send(t_fomus* x, t_symbol* s, int argc, t_atom* i) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  pdfomus_scopedsync xxx;
  t_binbuf* b = binbuf_new();
  binbuf_add(b, argc, i);
  char* buf;
  int len;
  binbuf_gettext(b, &buf, &len);
  static std::string cpy;
  cpy.assign(buf, buf + len);
  fomus_parse(x->fom, cpy.c_str());
  binbuf_free(b);
}

void pdfomus_run(t_fomus* x, t_symbol* s) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  pdfomus_scopedsync xxx;
  if (s->s_name[0]) {
    fomus_sval(x->fom, fomus_par_setting, fomus_act_set, "filename");
    fomus_sval(x->fom, fomus_par_settingval, fomus_act_set, s->s_name);
  }
  fomus_run(fomus_copy(x->fom));
}

void pdfomus_part(t_fomus* x, t_symbol* s) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  fomus_sval(x->fom, fomus_par_part, fomus_act_set, s->s_name);
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}

void pdfomus_time(t_fomus* x, t_floatarg f) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  fomus_fval(x->fom, fomus_par_time, fomus_act_set, f);
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}

void pdfomus_grtime(t_fomus* x, t_floatarg f) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  fomus_fval(x->fom, fomus_par_gracetime, fomus_act_set, f);
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}

void pdfomus_dur(t_fomus* x, t_floatarg f) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  fomus_fval(x->fom, fomus_par_duration, fomus_act_set, f);
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}

void pdfomus_voice(t_fomus* x, t_symbol* s, int argc,
                   t_atom* i) { // float or list
  pdfomus_flushout();
  if (!x->fom)
    return;
  fomus_act(x->fom, fomus_par_voice, fomus_act_clear);
  t_atom* ie = i + argc;
  for (; i < ie; ++i) {
    assert(i->a_type != A_DEFFLOAT);
    assert(i->a_type != A_DEFSYMBOL);
    fomus_fval(x->fom, fomus_par_voice, fomus_act_add, atom_getint(i));
  }
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}

void pdfomus_pitch(t_fomus* x, t_floatarg f) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  fomus_fval(x->fom, fomus_par_pitch, fomus_act_set, f);
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}

void pdfomus_dyn(t_fomus* x, t_floatarg f) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  fomus_fval(x->fom, fomus_par_dynlevel, fomus_act_set, f);
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}

void pdfomus_mark(t_fomus* x, t_symbol* s, int argc, t_atom* i) { // list
  pdfomus_flushout();
  if (!x->fom)
    return;
  bool fi = true;
  t_atom* ie = i + argc;
  for (; i < ie; ++i) {
    assert(i->a_type != A_DEFFLOAT);
    assert(i->a_type != A_DEFSYMBOL);
    switch (i->a_type) {
    case A_FLOAT:
      fomus_fval(x->fom, fomus_par_markval, fomus_act_add, atom_getfloat(i));
      break;
    case A_SYMBOL:
      if (fi) {
        fomus_sval(x->fom, fomus_par_markid, fomus_act_set,
                   atom_getsymbol(i)->s_name);
        fi = false;
      } else
        fomus_sval(x->fom, fomus_par_markval, fomus_act_add,
                   atom_getsymbol(i)->s_name);
      break;
    default:;
    }
  }
  fomus_act(x->fom, fomus_par_mark, fomus_act_add);
  clock_set(x->clock, clock_getsystimeafter(CLOCKDELAY));
}

void* pdfomus_init_new() {
  pdfomus_flushout();
  pdfomus_scopedsync xxx;
  t_fomus* x = (t_fomus*) pd_new(fomus_class);
  x->fom = 0;
  x->fom = fomus_new();
  // inlets
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("time"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("grtime"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("dur"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float,
            gensym("voice")); // a list has to go in leftmost inlet
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("pitch"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("dyn"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_list, gensym("mark"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_symbol, gensym("part"));
  x->clock = clock_new(&x->x_obj, (t_method) pdfomus_flushout);
  return (void*) x;
}

void pdfomus_init_free(t_fomus* x) {
  pdfomus_flushout();
  if (!x->fom)
    return;
  pdfomus_scopedsync xxx;
  clock_free(x->clock);
  fomus_free(x->fom);
}

void pdfomus_output(const char* str) {
  if (pdfomus_sync) {
    pdfomus_flushout();
    post("%s", str);
  } else {
    std::string* ptr = (pdfomus_inptr >= &pdfomus_textbuffers[NTEXTBUFFERS - 1]
                            ? &pdfomus_textbuffers[0]
                            : pdfomus_inptr + 1);
    if (ptr == pdfomus_outptr)
      return; // don't run into outptr, discard output
    pdfomus_inptr->assign(str);
    pdfomus_inptr = ptr;
  }
}

FOMUSMOD_EXPORT void fomus_setup() {
  // init fomus
  pdfomus_inptr = pdfomus_outptr = &pdfomus_textbuffers[0];
  fomus_set_outputs(pdfomus_output, pdfomus_output, 0);
  fomus_init();
  fomus_rt(1);
  post("%s %s", "FOMUS", fomus_version());
  post("%s",
       "Copyright (c) 2009, 2010, 2011 David Psenicka, All Rights Reserved");
  // pd stuff
  fomus_class = class_new(gensym("fomus"), (t_newmethod) pdfomus_init_new,
                          (t_method) pdfomus_init_free, sizeof(t_fomus),
                          CLASS_DEFAULT, (t_atomtype) 0); // no arguments
  // methods
  class_addmethod(fomus_class, (t_method) pdfomus_new, gensym("new"),
                  (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_clear, gensym("clear"),
                  (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_load, gensym("load"),
                  A_DEFSYMBOL, (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_send, gensym("send"), A_GIMME,
                  (t_atomtype) 0); // call fomus_parse
  class_addmethod(fomus_class, (t_method) pdfomus_run, gensym("run"),
                  A_DEFSYMBOL, (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_flushout, gensym("flush"),
                  (t_atomtype) 0);
  //
  class_addmethod(fomus_class, (t_method) pdfomus_part, gensym("part"),
                  A_SYMBOL, (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_time, gensym("time"),
                  A_DEFFLOAT, (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_grtime, gensym("grtime"),
                  A_DEFFLOAT, (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_dur, gensym("dur"),
                  A_DEFFLOAT, (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_voice, gensym("voice"),
                  A_GIMME, (t_atomtype) 0); // only a float goes in inlet
  class_addmethod(fomus_class, (t_method) pdfomus_pitch, gensym("pitch"),
                  A_DEFFLOAT, (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_dyn, gensym("dyn"),
                  A_DEFFLOAT, (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_mark, gensym("mark"), A_GIMME,
                  (t_atomtype) 0);
  //
  class_addmethod(fomus_class, (t_method) pdfomus_enternote, gensym("note*"),
                  (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_enterrest, gensym("rest*"),
                  (t_atomtype) 0);
  class_addmethod(fomus_class, (t_method) pdfomus_entermark, gensym("mark*"),
                  (t_atomtype) 0);
  class_addbang(fomus_class, (t_method) pdfomus_enternote); // enter
  pdfomus_sync = false;
}
}
