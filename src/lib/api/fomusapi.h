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

/*
  ALL COMMENTS IN THIS FILE ARE OLD AND HAVEN'T BEEN UPDATED YET!
*/

#ifndef FOMUS_FOMUSAPI_H
#define FOMUS_FOMUSAPI_H

#include "basetypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// API version is incremented with each change to `fomusapi.h'
#define FOMUS_API_VERSION 1

// parameters
enum fomus_param {
  // settings and values
  fomus_par_none,
  fomus_par_entry,
  fomus_par_list,
  fomus_par_setting,
  fomus_par_settingval,

  // source location
  fomus_par_locfile,
  fomus_par_locline,
  fomus_par_loccol,

  // instruments and parts
  fomus_par_import,
  fomus_par_import_settingval,

  fomus_par_export,
  fomus_par_export_settingval,

  fomus_par_clef,
  fomus_par_clef_settingval,

  fomus_par_staff,
  fomus_par_staff_clefs,
  fomus_par_staff_settingval,

  fomus_par_percinst,
  fomus_par_percinst_template,
  fomus_par_percinst_id,
  fomus_par_percinst_imports,
  fomus_par_percinst_export,
  fomus_par_percinst_settingval,

  fomus_par_inst,
  fomus_par_inst_template,
  fomus_par_inst_id,
  fomus_par_inst_staves,
  fomus_par_inst_imports,
  fomus_par_inst_export,
  fomus_par_inst_percinsts,
  fomus_par_inst_settingval,

  fomus_par_part,
  fomus_par_part_id,
  fomus_par_part_inst,
  fomus_par_part_settingval,

  fomus_par_partmap,
  fomus_par_partmap_part,
  fomus_par_partmap_metapart,
  fomus_par_partmap_settingval,

  fomus_par_metapart,
  fomus_par_metapart_id,
  fomus_par_metapart_partmaps,
  fomus_par_metapart_settingval,

  fomus_par_measdef,
  fomus_par_measdef_id,
  fomus_par_measdef_settingval,

  fomus_par_list_percinsts,
  fomus_par_list_insts,
  fomus_par_settingval_percinsts,
  fomus_par_settingval_insts,

  // regions
  fomus_par_region,
  fomus_par_region_voice,
  fomus_par_region_voicelist,
  fomus_par_region_time,
  fomus_par_region_gracetime,
  fomus_par_region_duration,
  fomus_par_region_pitch,
  fomus_par_region_dynlevel,
  fomus_par_region_mark,
  fomus_par_region_settingval,

  // events
  fomus_par_time,
  fomus_par_gracetime,
  fomus_par_duration,
  fomus_par_pitch,
  fomus_par_dynlevel,
  fomus_par_voice,

  fomus_par_note_settingval,

  fomus_par_meas,
  fomus_par_meas_measdef,

  // marks
  fomus_par_markid,
  fomus_par_markval,
  fomus_par_mark,

  // events
  fomus_par_noteevent,
  fomus_par_restevent,
  fomus_par_markevent,
  fomus_par_measevent,

  fomus_par_events,

  // n
  fomus_par_n
};

// actions
enum fomus_action {
  fomus_act_none,

  fomus_act_set,

  fomus_act_inc,
  fomus_act_dec,
  fomus_act_mult,
  fomus_act_div,

  fomus_act_start,
  fomus_act_append,
  fomus_act_add,
  fomus_act_remove,
  fomus_act_end,
  fomus_act_clear,

  fomus_act_queue,
  fomus_act_cancel,
  fomus_act_resume,

  // n
  fomus_act_n
};

// callback function
typedef void (*fomus_output)(const char* str);

#ifndef LIBFOMUS_HIDE

// return API version number that library was compiled with
LIBFOMUS_EXPORT int fomus_api_version();

// check if last function call returned an error
LIBFOMUS_EXPORT int fomus_err();

// version string
LIBFOMUS_EXPORT const char* fomus_version();

// must call before anything, can be called repeatedly, frees and invalidates
// all instances, if fomus_err() reports an error then can't continue
LIBFOMUS_EXPORT void fomus_init();
// get a new instance
LIBFOMUS_EXPORT FOMUS fomus_new();
// destroy an instance
LIBFOMUS_EXPORT void fomus_free(FOMUS f);

// clear all input (destroy and reconstruct instance)
LIBFOMUS_EXPORT void fomus_clear(FOMUS f);

// insert data into instance
LIBFOMUS_EXPORT void fomus_ival(FOMUS f, int par, int act, fomus_int val);
LIBFOMUS_EXPORT void fomus_rval(FOMUS f, int par, int act, fomus_int num,
                                fomus_int den);
LIBFOMUS_EXPORT void fomus_mval(FOMUS f, int par, int act, fomus_int val,
                                fomus_int num, fomus_int den);
LIBFOMUS_EXPORT void fomus_fval(FOMUS f, int par, int act, fomus_float val);
LIBFOMUS_EXPORT void fomus_sval(FOMUS f, int par, int act, const char* str);
LIBFOMUS_EXPORT void fomus_act(FOMUS f, int par, int act);

// turn realtime input mode on/off
LIBFOMUS_EXPORT void fomus_rt(int on);
// flush output
LIBFOMUS_EXPORT void fomus_flush();

// load a `.fms' file
LIBFOMUS_EXPORT void fomus_load(FOMUS f, const char* filename);
// parse and input string as if it were a `.fms' file
LIBFOMUS_EXPORT void fomus_parse(FOMUS f, const char* input);

// copies instance
LIBFOMUS_EXPORT FOMUS fomus_copy(FOMUS f);
// merges `from' instance into `to'
LIBFOMUS_EXPORT void fomus_merge(FOMUS to, FOMUS from);

// runs FOMUS, always destroys instance (copy it first if necessary)
LIBFOMUS_EXPORT void fomus_run(FOMUS f);
// save instance as `.fms' file, always destroys instance (copy it first if
// necessary)
LIBFOMUS_EXPORT void fomus_save(FOMUS f, const char* filename);

// set output callback functions, either/both of these can be NULL, `newline'
// set to 1 means include newline in output
LIBFOMUS_EXPORT void fomus_set_outputs(fomus_output out, fomus_output err,
                                       int newline);

#endif

#ifdef __cplusplus
}
#endif

#endif
