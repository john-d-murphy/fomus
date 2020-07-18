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

/*
  ALL COMMENTS IN THIS FILE ARE OLD AND HAVEN'T BEEN UPDATED YET!
*/

#ifndef FOMUS_MODULE_H
#define FOMUS_MODULE_H

#include "modtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FOMUS_MODAPI_VERSION 1

LIBFOMUS_EXPORT void
module_stdout(const char* str,
              unsigned long n); // if n = 0, then str must be zero terminated
LIBFOMUS_EXPORT void
module_stderr(const char* str,
              unsigned long n); // if n = 0, then str must be zero terminated

LIBFOMUS_EXPORT struct module_list module_new_list(
    int n); // it's actually a vector, but in the config files it's a list
LIBFOMUS_EXPORT void module_free_list(struct module_list list);
// LIBFOMUS_EXPORT void module_free_string(const char* str);

LIBFOMUS_EXPORT void module_setval_int(struct module_value* val, fomus_int i);
LIBFOMUS_EXPORT void module_setval_float(struct module_value* val,
                                         fomus_float f);
LIBFOMUS_EXPORT void module_setval_rat(struct module_value* val,
                                       struct fomus_rat r);
LIBFOMUS_EXPORT void module_setval_string(
    struct module_value* val,
    const char* s); // module must keep track of (new/delete) its own strings!
LIBFOMUS_EXPORT void module_setval_list(struct module_value* val, int n); //

#define SET_I module_setval_int
#define SET_F module_setval_float
#define SET_R module_setval_rat
#define SET_S module_setval_string
#define SET_L module_setval_list

LIBFOMUS_EXPORT fomus_int module_getval_int(struct module_value val);
LIBFOMUS_EXPORT fomus_float module_getval_float(struct module_value val);
LIBFOMUS_EXPORT struct fomus_rat module_getval_rat(struct module_value val);
LIBFOMUS_EXPORT const char* module_getval_string(struct module_value val);
LIBFOMUS_EXPORT struct module_list module_getval_list(struct module_value val);

#define GET_I module_getval_int
#define GET_F module_getval_float
#define GET_R module_getval_rat
#define GET_S module_getval_string
#define GET_L module_getval_list

LIBFOMUS_EXPORT const char* module_valuetostr(struct module_value x);

LIBFOMUS_EXPORT void module_ratreduce(struct fomus_rat* x);
LIBFOMUS_EXPORT struct fomus_rat module_makerat_reduce(fomus_int n,
                                                       fomus_int d);
LIBFOMUS_EXPORT fomus_int module_rattoint(struct fomus_rat x);
LIBFOMUS_EXPORT fomus_float module_rattofloat(struct fomus_rat x);
LIBFOMUS_EXPORT struct fomus_rat module_inttorat(fomus_int x);
LIBFOMUS_EXPORT struct fomus_rat module_floattorat(fomus_int x);
LIBFOMUS_EXPORT int module_rateq(struct fomus_rat a, struct fomus_rat b);
LIBFOMUS_EXPORT int module_ratneq(struct fomus_rat a, struct fomus_rat b);
LIBFOMUS_EXPORT int module_ratlt(struct fomus_rat a, struct fomus_rat b);
LIBFOMUS_EXPORT int module_ratlteq(struct fomus_rat a, struct fomus_rat b);
LIBFOMUS_EXPORT int module_ratgt(struct fomus_rat a, struct fomus_rat b);
LIBFOMUS_EXPORT int module_ratgteq(struct fomus_rat a, struct fomus_rat b);
LIBFOMUS_EXPORT struct fomus_rat module_ratplus(struct fomus_rat a,
                                                struct fomus_rat b);
LIBFOMUS_EXPORT struct fomus_rat module_ratminus(struct fomus_rat a,
                                                 struct fomus_rat b);
LIBFOMUS_EXPORT struct fomus_rat module_ratmult(struct fomus_rat a,
                                                struct fomus_rat b);
LIBFOMUS_EXPORT struct fomus_rat module_ratdiv(struct fomus_rat a,
                                               struct fomus_rat b);
LIBFOMUS_EXPORT struct fomus_rat module_ratinv(struct fomus_rat x);
LIBFOMUS_EXPORT struct fomus_rat module_ratneg(struct fomus_rat x);
LIBFOMUS_EXPORT const char* module_rattostr(struct fomus_rat x);
#ifdef NO_INLINE
LIBFOMUS_EXPORT struct fomus_rat module_makerat(fomus_int n, fomus_int d);
#else
inline struct fomus_rat module_makerat(fomus_int n, fomus_int d) {
  struct fomus_rat r = {n, d};
  return r;
}
#endif

LIBFOMUS_EXPORT int module_valid_int(struct module_value val, fomus_int min,
                                     enum module_bound minbnd, fomus_int max,
                                     enum module_bound maxbnd, valid_int fun,
                                     const char* printerr);
LIBFOMUS_EXPORT int
module_valid_rat(struct module_value val, struct fomus_rat min,
                 enum module_bound minbnd, struct fomus_rat max,
                 enum module_bound maxbnd, valid_rat fun, const char* printerr);
LIBFOMUS_EXPORT int
module_valid_num(struct module_value val, struct module_value min,
                 enum module_bound minbnd, struct module_value max,
                 enum module_bound maxbnd, valid_num fun, const char* printerr);
LIBFOMUS_EXPORT int module_valid_string(struct module_value val, int minlen,
                                        int maxlen, valid_string fun,
                                        const char* printerr);
LIBFOMUS_EXPORT int
module_valid_listofints(struct module_value val, int minlen, int maxlen,
                        fomus_int min, enum module_bound minbnd, fomus_int max,
                        enum module_bound maxbnd, valid_listint fun,
                        const char* printerr);
LIBFOMUS_EXPORT int
module_valid_listofrats(struct module_value val, int minlen, int maxlen,
                        struct fomus_rat min, enum module_bound minbnd,
                        struct fomus_rat max, enum module_bound maxbnd,
                        valid_listrat fun, const char* printerr);
LIBFOMUS_EXPORT int
module_valid_listofnums(struct module_value val, int minlen, int maxlen,
                        struct module_value min, enum module_bound minbnd,
                        struct module_value max, enum module_bound maxbnd,
                        valid_listnum fun, const char* printerr);
LIBFOMUS_EXPORT int module_valid_listofstrings(struct module_value val,
                                               int minlen, int maxlen,
                                               int minstrlen, int maxstrlen,
                                               valid_liststring fun,
                                               const char* printerr);
LIBFOMUS_EXPORT int module_valid_listofvals(
    struct module_value val, int minlen, int maxlen, valid_listval fun,
    const char* printerr); // list of anything (or list of lists)
LIBFOMUS_EXPORT int
module_valid_maptoints(struct module_value val, int minlen, int maxlen,
                       fomus_int min, enum module_bound minbnd, fomus_int max,
                       enum module_bound maxbnd, valid_mapint fun,
                       const char* printerr);
LIBFOMUS_EXPORT int
module_valid_maptorats(struct module_value val, int minlen, int maxlen,
                       struct fomus_rat min, enum module_bound minbnd,
                       struct fomus_rat max, enum module_bound maxbnd,
                       valid_maprat fun, const char* printerr);
LIBFOMUS_EXPORT int
module_valid_maptonums(struct module_value val, int minlen, int maxlen,
                       struct module_value min, enum module_bound minbnd,
                       struct module_value max, enum module_bound maxbnd,
                       valid_mapnum fun, const char* printerr);
LIBFOMUS_EXPORT int module_valid_maptostrings(struct module_value val,
                                              int minlen, int maxlen,
                                              int minstrlen, int maxstrlen,
                                              valid_mapstring fun,
                                              const char* printerr);
LIBFOMUS_EXPORT int module_valid_maptovals(struct module_value val, int minlen,
                                           int maxlen, valid_mapval fun,
                                           const char* printerr);
LIBFOMUS_EXPORT int module_valid_printiferr(int valid, const char* printerr);

LIBFOMUS_EXPORT fomus_float module_setting_fval(module_obj o, int id);
LIBFOMUS_EXPORT struct fomus_rat module_setting_rval(module_obj o, int id);
LIBFOMUS_EXPORT fomus_int module_setting_ival(module_obj o, int id);
LIBFOMUS_EXPORT const char* module_setting_sval(module_obj o, int id);
LIBFOMUS_EXPORT struct module_value module_setting_val(module_obj o, int id);

LIBFOMUS_EXPORT int
module_settingid(const char* set); // if getting setting value multiple times,
                                   // store the integer ID for faster lookup
LIBFOMUS_EXPORT int
module_setting_allowed(const enum module_setting_loc allowedin,
                       const enum module_setting_loc setloc);

// LIBFOMUS_EXPORT fomus_float module_setting_minfval(module_obj o, int id);
// LIBFOMUS_EXPORT struct fomus_rat module_setting_minrval(module_obj o, int
// id); LIBFOMUS_EXPORT fomus_int module_setting_minival(module_obj o, int id);
// LIBFOMUS_EXPORT fomus_float module_setting_maxfval(module_obj o, int id);
// LIBFOMUS_EXPORT struct fomus_rat module_setting_maxrval(module_obj o, int
// id); LIBFOMUS_EXPORT fomus_int module_setting_maxival(module_obj o, int id);

LIBFOMUS_EXPORT struct module_objlist module_get_percinsts(FOMUS f, int all);
LIBFOMUS_EXPORT struct module_objlist module_get_insts(FOMUS f, int all);
LIBFOMUS_EXPORT struct module_objlist module_get_parts(FOMUS f);

// get the next note, or NULL if at end
LIBFOMUS_EXPORT module_noteobj module_nextnote();
LIBFOMUS_EXPORT module_measobj
module_nextmeas(); // for modules working with entire parts--this isn't
                   // synchronized with `nextnote', these are just iterators
LIBFOMUS_EXPORT module_partobj module_nextpart();

// can send NULL to these to get first one
LIBFOMUS_EXPORT module_noteobj module_peeknextnote(
    module_noteobj note); // next measure or note--gets measures/notes out of
                          // sequence from above
LIBFOMUS_EXPORT module_measobj module_peeknextmeas(
    module_measobj meas); // next measure or note--gets measures/notes out of
                          // sequence from above
LIBFOMUS_EXPORT module_partobj module_peeknextpart(module_partobj part);

LIBFOMUS_EXPORT module_measobj
module_meas(module_noteobj note);                           // get parent object
LIBFOMUS_EXPORT module_partobj module_part(module_obj obj); // get parent object

LIBFOMUS_EXPORT const char*
module_id(module_obj obj); // name of any object (part, instr, etc..)
LIBFOMUS_EXPORT int module_less(module_obj obj1, module_obj obj2);

LIBFOMUS_EXPORT int module_isnote(module_noteobj note);
LIBFOMUS_EXPORT int module_isrest(module_noteobj note);
LIBFOMUS_EXPORT int module_ismarkrest(module_noteobj note);
LIBFOMUS_EXPORT int module_isgrace(module_noteobj note);
LIBFOMUS_EXPORT int module_isperc(module_noteobj note);
LIBFOMUS_EXPORT int module_isfullmeasrest(module_measobj meas);

LIBFOMUS_EXPORT struct module_value
module_vtime(module_obj obj);                                 // measure or note
LIBFOMUS_EXPORT struct fomus_rat module_time(module_obj obj); // measure or note
LIBFOMUS_EXPORT struct module_value
module_vgracetime(module_obj obj); // measure or note--could return module_none
                                   // (if not grace note!)
LIBFOMUS_EXPORT struct fomus_rat
module_gracetime(module_obj obj); // measure or note--could return module_none
                                  // (if not grace note!)

LIBFOMUS_EXPORT struct module_value
module_vdur(module_obj obj); // grace notes have dur = 0 and endtime = time
LIBFOMUS_EXPORT struct fomus_rat
module_dur(module_obj obj); // grace notes have dur = 0 and endtime = time
LIBFOMUS_EXPORT struct fomus_rat module_gracedur(module_obj obj);
LIBFOMUS_EXPORT struct module_value module_vgracedur(module_noteobj note);
LIBFOMUS_EXPORT struct fomus_rat
module_adjdur(module_obj obj,
              int level); // level is tuplet level, 1/8 = eighth note, 1/16 =
                          // sixteenth, etc., if level = -1, all levels
LIBFOMUS_EXPORT struct fomus_rat
module_adjgracedur(module_obj obj,
                   int level); // level is tuplet level, 1/8 = eighth note, 1/16
                               // = sixteenth, etc.
LIBFOMUS_EXPORT struct fomus_rat
module_beatstoadjdur(module_obj obj, struct fomus_rat dur, int level);

LIBFOMUS_EXPORT struct module_value module_vendtime(
    module_obj obj); // --could return module_none (if not grace note!)
LIBFOMUS_EXPORT struct fomus_rat module_endtime(
    module_obj obj); // --could return module_none (if not grace note!)
LIBFOMUS_EXPORT struct module_value module_vgraceendtime(module_obj obj);
LIBFOMUS_EXPORT struct fomus_rat module_graceendtime(module_obj obj);
#ifdef NO_INLINE
LIBFOMUS_EXPORT struct module_value module_vgracetiedendtime(module_obj obj);
LIBFOMUS_EXPORT struct fomus_rat module_gracetiedendtime(module_obj obj);
#else
inline struct module_value module_vgracetiedendtime(module_obj obj) {
  return module_vgraceendtime(obj);
}
inline struct fomus_rat module_gracetiedendtime(module_obj obj) {
  return module_graceendtime(obj);
}
#endif
LIBFOMUS_EXPORT struct module_value module_vtiedendtime(module_noteobj note);
LIBFOMUS_EXPORT struct fomus_rat module_tiedendtime(module_noteobj note);

LIBFOMUS_EXPORT int module_istiedleft(module_noteobj note);
LIBFOMUS_EXPORT int module_istiedright(module_noteobj note);
LIBFOMUS_EXPORT module_noteobj module_leftmosttiednote(module_noteobj note);
LIBFOMUS_EXPORT module_noteobj module_rightmosttiednote(module_noteobj note);

LIBFOMUS_EXPORT int module_isbeginchord(module_noteobj note);
LIBFOMUS_EXPORT int module_isendchord(module_noteobj note);
LIBFOMUS_EXPORT int module_ischordlow(module_noteobj note);
LIBFOMUS_EXPORT int module_ischordhigh(module_noteobj note);

LIBFOMUS_EXPORT int
module_tupletbegin(module_noteobj note,
                   int level); // begin or end of tuplet?--should return same
                               // for ea. note in chord
LIBFOMUS_EXPORT int module_tupletend(module_noteobj note, int level);
LIBFOMUS_EXPORT struct fomus_rat
module_tuplet(module_noteobj note,
              int level); // get the tuple--rational is NOT normalized--returns
                          // 0 if no tuplet at that level
LIBFOMUS_EXPORT struct fomus_rat
module_fulltupdur(module_noteobj note,
                  int level); // get full duration span of tuplet at that level
LIBFOMUS_EXPORT struct fomus_rat module_writtenmult(
    module_measobj meas); // multiplier to get the ACTUAL written dur

LIBFOMUS_EXPORT struct module_value module_vpitch(module_noteobj note);
LIBFOMUS_EXPORT struct fomus_rat module_pitch(module_noteobj note);
LIBFOMUS_EXPORT fomus_int module_writtennote(
    module_noteobj note); // with accidentals applied--should be a white note

LIBFOMUS_EXPORT struct module_value
module_dyn(module_noteobj note); // grace notes have dur = 0 and endtime = time

LIBFOMUS_EXPORT int module_voice(
    module_noteobj note); // gets first one if there's still a list, otherwise
                          // return 0 (-1 is reserved for special cases)
LIBFOMUS_EXPORT struct module_intslist module_voices(
    module_obj obj); // for parts, measures or notes--returns SORTED list
LIBFOMUS_EXPORT int module_hasvoice(module_noteobj note, int voice);
LIBFOMUS_EXPORT int module_staffvoice(module_noteobj note);

LIBFOMUS_EXPORT int
module_staff(module_noteobj note); // gets first one if there's still a list,
                                   // otherwise return 0
LIBFOMUS_EXPORT struct module_intslist
module_staves(module_obj obj); // for parts, measures or notes--returns SORTED
                               // list of ONLY the staves that are occupied
LIBFOMUS_EXPORT int module_hasstaff(module_noteobj note,
                                    int staff); // (in possible choices)
LIBFOMUS_EXPORT int module_totalnstaves(
    module_obj obj); // for instruments and parts--returns actual number of
                     // staves on score (not list of used staves!)

// possible clefs obtained by going down to all lists of clefs in staves in
// instr--if note, then take extra note clef setting into account
LIBFOMUS_EXPORT struct module_intslist module_clefs(
    module_obj obj); // for parts, measures or notes--returns SORTED list
LIBFOMUS_EXPORT struct module_intslist module_clefsinstaff(
    module_obj obj,
    int staff); // for parts, measures or notes--returns SORTED list
LIBFOMUS_EXPORT int module_hasclef(module_obj obj,
                                   int clef); // (in possible choices)
LIBFOMUS_EXPORT int module_hasstaffclef(module_obj obj, int clef,
                                        int staff); // (in possible choices)
LIBFOMUS_EXPORT int module_clef(module_noteobj note);
LIBFOMUS_EXPORT const char* module_staffclef(
    module_partobj part,
    int staff); // gets the `clef' setting from the `1st defined clef in the
                // staff in the given part (ie. the "default" clef)
LIBFOMUS_EXPORT int module_clefmidpitch(int clef);

LIBFOMUS_EXPORT struct fomus_rat module_fullwrittenacc(
    module_noteobj note); // returns FOMUS_INT_MAX/1 if no accidental
LIBFOMUS_EXPORT struct fomus_rat module_writtenacc1(
    module_noteobj note); // returns FOMUS_INT_MAX/1 if no accidental
LIBFOMUS_EXPORT struct fomus_rat module_writtenacc2(
    module_noteobj note); // returns FOMUS_INT_MAX/1 if no accidental
LIBFOMUS_EXPORT struct fomus_rat module_fullacc(module_noteobj note);
LIBFOMUS_EXPORT struct fomus_rat module_acc1(module_noteobj note);
LIBFOMUS_EXPORT struct fomus_rat module_acc2(module_noteobj note);

LIBFOMUS_EXPORT int
module_octsign(module_noteobj note); // -1 = octave down, 1 = octave up, etc..
LIBFOMUS_EXPORT int module_octavebegin(
    module_noteobj obj); // notes in same chord + staffn have same octave sign
LIBFOMUS_EXPORT int module_octaveend(module_noteobj obj);

LIBFOMUS_EXPORT struct module_ratslist module_divs(
    module_measobj meas); // get the initial dividing points of the measure

LIBFOMUS_EXPORT enum parts_grouptype module_partgroupbegin(module_partobj part,
                                                           int lvl);
LIBFOMUS_EXPORT int module_partgroupend(module_partobj part, int lvl);

LIBFOMUS_EXPORT struct module_markslist module_singlemarks(module_noteobj note);
LIBFOMUS_EXPORT struct module_markslist
module_spannerbegins(module_noteobj note);
LIBFOMUS_EXPORT struct module_markslist module_spannerends(module_noteobj note);
LIBFOMUS_EXPORT struct module_markslist module_marks(module_noteobj note);
LIBFOMUS_EXPORT int module_markid(module_markobj mark); // return a mark id
LIBFOMUS_EXPORT int
module_markbaseid(int id); // return the mark id or "base" mark id if it's a
                           // mark w/ extra flags or attachments
LIBFOMUS_EXPORT int module_markcantouch(int id, module_noteobj note);
LIBFOMUS_EXPORT int module_markcanspanone(int id, module_noteobj note);
LIBFOMUS_EXPORT int
module_markisdetachable(int id); // returns 2 if it MUST be detached
LIBFOMUS_EXPORT int module_markisvoice(int id);
LIBFOMUS_EXPORT int module_markisstaff(int id);
LIBFOMUS_EXPORT int module_markisspannerbegin(int id);
LIBFOMUS_EXPORT int module_markisspannerend(int id);
LIBFOMUS_EXPORT int module_markcanreduce(int id);
LIBFOMUS_EXPORT int module_markcanspanrests(int id, module_noteobj note);
LIBFOMUS_EXPORT int module_markcanendonrests(int id, module_noteobj note);
LIBFOMUS_EXPORT int module_markspangroup(int type);
LIBFOMUS_EXPORT const char*
module_markstring(module_markobj mark); // optional string argument
LIBFOMUS_EXPORT struct module_value
module_marknum(module_markobj mark); // optional number argument
LIBFOMUS_EXPORT int module_markisspecialpair(int type1, int type2);
LIBFOMUS_EXPORT struct module_objlist module_getmarkevlist(module_partobj part);
LIBFOMUS_EXPORT enum module_markpos module_markpos(module_markobj mark);

LIBFOMUS_EXPORT struct fomus_rat
module_timesig(module_measobj meas); // the time signature (rat isn't reduced)
LIBFOMUS_EXPORT int module_partialmeas(module_measobj meas);
LIBFOMUS_EXPORT int module_partialbarline(module_measobj meas);

LIBFOMUS_EXPORT const char* module_pitchtostr(struct module_value pitch);
LIBFOMUS_EXPORT const char* module_pitchnametostr(struct module_value pitch,
                                                  const int forceacc);
LIBFOMUS_EXPORT const char* module_percinststr(module_noteobj note);
LIBFOMUS_EXPORT int module_strtoclef(const char* str);
LIBFOMUS_EXPORT const char* module_cleftostr(int en);
LIBFOMUS_EXPORT int module_strtomark(const char* str);
LIBFOMUS_EXPORT const char* module_marktostr(int id);
LIBFOMUS_EXPORT struct fomus_rat module_strtonote(
    const char* str,
    struct module_noteparts* parts); // might return FOMUS_INT_MAX/1!
LIBFOMUS_EXPORT struct fomus_rat module_strtoacc(
    const char* str,
    struct module_noteparts* parts); // might return FOMUS_INT_MAX/1!

LIBFOMUS_EXPORT void module_skipassign(module_noteobj note);

LIBFOMUS_EXPORT const char* module_getposstring(
    module_noteobj note); // print the file position to the module stdout

// always returns array of 75 (diatonic notes)
LIBFOMUS_EXPORT const struct module_keysigref*
module_keysigref(module_measobj meas);
// get the accidental that appears in the keysig
LIBFOMUS_EXPORT struct module_keysigref module_keysigacc(module_noteobj note);
LIBFOMUS_EXPORT struct module_keysigref
module_measkeysigacc(module_measobj meas,
                     int note); // this is a white chromatic note

LIBFOMUS_EXPORT module_instobj module_inst(module_obj obj);
LIBFOMUS_EXPORT module_percinstobj module_percinst(
    module_noteobj note); // does a look-up, return NULL if no perc inst

// use an auxiliary module
LIBFOMUS_EXPORT void module_get_auxiface(const char* modname, int ifaceid,
                                         void* iface);

// --------------------------------------------------------------------------------CALLBACK
// FUNCTIONS----------------------

LIBFOMUS_EXPORT void module_register(const char* name,
                                     const struct module_callbacks* callbacks);

#ifndef FOMUSMOD_HIDE

// initialization and unloading
FOMUSMOD_EXPORT void module_init();
FOMUSMOD_EXPORT void module_free();
// report any errors with an error string or 0 if none
FOMUSMOD_EXPORT const char* module_initerr();

// return a name for this module
FOMUSMOD_EXPORT const char* module_longname();
FOMUSMOD_EXPORT const char* module_author();
// return a documentation string
FOMUSMOD_EXPORT const char* module_doc();
// return the module category
FOMUSMOD_EXPORT enum module_type module_type();

FOMUSMOD_EXPORT int module_get_setting(int n, struct module_setting* set,
                                       int id);
// called after all settings in all modules are processed
FOMUSMOD_EXPORT void module_ready();

#endif

// --------------------------------------------------------------------------------ASSIGNMENTS----------------------

LIBFOMUS_EXPORT void accs_assign(module_noteobj note, struct fomus_rat acc,
                                 struct fomus_rat micacc);

LIBFOMUS_EXPORT void beams_assign_beams(module_noteobj note, int leftbeams,
                                        int rightbeams);

// void accs_assign(module_noteobj note, fomus_int acc, fomus_int micacc);
LIBFOMUS_EXPORT void
cautaccs_assign(module_noteobj note); // assign a cautionary accidental
// void cautaccs_skip(module_noteobj note);

struct divide_tuplet {
  struct fomus_rat tuplet;
  int begin, end;
  struct fomus_rat fulldur;
};
struct divide_split {
  module_noteobj note;
  struct fomus_rat time;
  int n;
  struct divide_tuplet* tups; // outer to inner
};
struct divide_gracesplit {
  module_noteobj note;
  struct fomus_rat grtime;
};
// initdivs are the initial measure divisions
LIBFOMUS_EXPORT void divide_assign_initdivs(
    module_measobj meas,
    struct module_ratslist initdivs); // called once per measure
LIBFOMUS_EXPORT void divide_assign_split(module_measobj meas,
                                         struct divide_split split);
LIBFOMUS_EXPORT void divide_assign_gracesplit(module_measobj meas,
                                              struct divide_gracesplit split);
LIBFOMUS_EXPORT void divide_assign_unsplit(module_noteobj rightnote);

// call this after all notes have been assigned
LIBFOMUS_EXPORT void markevs_assign_add(module_partobj part, int voice,
                                        struct module_value off, int type,
                                        const char* arg1,
                                        struct module_value arg2);

LIBFOMUS_EXPORT void markpos_assign(module_markobj mark,
                                    enum module_markpos pos);

enum marks_which { marks_begin, marks_end, marks_sing };
LIBFOMUS_EXPORT void marks_assign_add(module_noteobj note, int type,
                                      const char* arg1,
                                      struct module_value arg2);
LIBFOMUS_EXPORT void marks_assign_remove(
    module_noteobj note, int type, const char* arg1,
    struct module_value arg2); // fomus also matches the arguments
LIBFOMUS_EXPORT void marks_assign_done(module_noteobj note);

// module_measobj meas_defaultmeas();
// rmable = removeable  (for measures at end of score--there's a tiny chance an
// extra one might be created because notes aren't quantized yet)
LIBFOMUS_EXPORT void meas_assign(module_measobj measbase, struct fomus_rat time,
                                 struct fomus_rat dur, int rmable /*= 0*/);

// change the note's voice and replace it in its current voice with an invisible
// rest, optionally adding a text mark together = note is actually eliminated
// and tovoice part is marked with "a2" marking
LIBFOMUS_EXPORT void merge_assign_merge(module_noteobj note,
                                        module_noteobj tonote);
// makes the note invisible and adds a I or II

typedef void* metaparts_partmapobj;
struct metaparts_partmaps {
  int n;
  const metaparts_partmapobj* partmaps;
};
LIBFOMUS_EXPORT struct metaparts_partmaps
metaparts_getpartmaps(module_partobj part);
LIBFOMUS_EXPORT module_partobj
metaparts_partmappart(metaparts_partmapobj partsmap);
LIBFOMUS_EXPORT int module_ismetapart(module_partobj part);
LIBFOMUS_EXPORT void metaparts_assign(module_noteobj note, module_partobj part,
                                      int voice, struct fomus_rat pitch);
LIBFOMUS_EXPORT void metaparts_assign_done(module_noteobj note);

LIBFOMUS_EXPORT void octs_assign(module_noteobj note, int octs);

// assignments can be any order since not dealing w/ notes
// void accs_assign(module_noteobj note, fomus_int acc, fomus_int micacc);
LIBFOMUS_EXPORT void parts_assigngroup(module_partobj begin, module_partobj end,
                                       enum parts_grouptype type);
LIBFOMUS_EXPORT void parts_assignorder(module_partobj part, int ord,
                                       int tempomarks);

// not an assign function
// int parts_groupsingle(module_partobj part);
LIBFOMUS_EXPORT void percnotes_assign(module_noteobj note, int voice,
                                      struct fomus_rat pitch);

// only deal with first note in ties
LIBFOMUS_EXPORT void pquant_assign(module_noteobj note, struct fomus_rat pitch);

// time1 & time2 are gracetimes if note is a grace note
LIBFOMUS_EXPORT void prune_assign(module_noteobj note, struct fomus_rat time1,
                                  struct fomus_rat time2);
LIBFOMUS_EXPORT void prune_assign_done(module_noteobj note);

LIBFOMUS_EXPORT void rstaves_assign(module_noteobj note, int staff, int clef);

struct special_markspec {
  int id;
  const char* str;
  struct module_value val;
};
struct special_markslist {
  int n;
  const struct special_markspec* marks;
};
// this are all final assignments, don't call marks_assign_done
LIBFOMUS_EXPORT void special_assign_delete(module_noteobj note);
LIBFOMUS_EXPORT void special_assign_makeinv(module_noteobj note);
LIBFOMUS_EXPORT void special_assign_newnote(module_noteobj note,
                                            struct fomus_rat pitch,
                                            struct fomus_rat acc1,
                                            struct fomus_rat acc2,
                                            struct special_markslist marks);

LIBFOMUS_EXPORT module_staffobj staves_staff(module_staffobj part, int staff);
LIBFOMUS_EXPORT module_clefobj staves_clef(module_partobj part, int staff,
                                           int clef);
LIBFOMUS_EXPORT void staves_assign(module_noteobj note, int staff, int clef);

// void accs_assign(module_noteobj note, fomus_int acc, fomus_int micacc);
LIBFOMUS_EXPORT void tpose_assign(module_noteobj note, struct fomus_rat pitch);
LIBFOMUS_EXPORT void
tpose_assign_keysig(module_measobj meas,
                    const char* str); // new value for "keysig"

// for both grace and non-grace notes--dur is ignored for grace notes
// if this is used, another module must handle grace notes!
LIBFOMUS_EXPORT void
tquant_assign_time(module_noteobj note, struct fomus_rat time,
                   struct fomus_rat dur); // fomus protects grace notes from
                                          // having durations overwritten
// special for grace notes
LIBFOMUS_EXPORT void tquant_assign_gracetime(module_noteobj note,
                                             struct fomus_rat time,
                                             struct module_value gracetime,
                                             struct module_value dur);
LIBFOMUS_EXPORT void
tquant_delete(module_noteobj note); // user might want notes deleted if their
                                    // durations quantize to 0

LIBFOMUS_EXPORT void voices_assign(module_noteobj note, int voice);

#ifndef FOMUSMOD_HIDE

// new/free data object
FOMUSMOD_EXPORT void*
module_newdata(FOMUS f); // can return 0 (FOMUS won't call freedata then)
FOMUSMOD_EXPORT void module_freedata(void* data);
FOMUSMOD_EXPORT const char* module_err(void* data);

FOMUSMOD_EXPORT void aux_fill_iface(void* iface); // divrules.cc & fills it up
FOMUSMOD_EXPORT int aux_ifaceid(); // divrules.cc must provide a unique one in
                                   // header divrules.h file

FOMUSMOD_EXPORT void* engine_get_iface(void* engdata);
FOMUSMOD_EXPORT int engine_ifaceid();
FOMUSMOD_EXPORT void engine_run(void* data);

// how module wants its data
FOMUSMOD_EXPORT int module_itertype();
FOMUSMOD_EXPORT int module_engine_iface(); // what engine does the module want?
FOMUSMOD_EXPORT void module_fill_iface(void* moddata, void* iface); // module
FOMUSMOD_EXPORT const char*
module_engine(void* moddata); // what engine module to use?
FOMUSMOD_EXPORT int module_priority();
// module compares settings in a and b and returns true if instance should be
// the same
FOMUSMOD_EXPORT int module_sameinst(module_obj a, module_obj b);

#endif

// import structures
LIBFOMUS_EXPORT const char* modin_importpercid(module_obj imp);

struct modin_imports {
  int n;
  const module_obj* obj;
};
LIBFOMUS_EXPORT struct modin_imports modin_imports(module_partobj part);

#ifndef FOMUSMOD_HIDE

// CALLBACKS
// return lowercase filename extension strings (0 when the last is reached)
FOMUSMOD_EXPORT const char* modin_get_extension(int n);
// return a loadid that can be used instead of a filename extension (this is
// checked first!)--return 0 if none (=file input)
FOMUSMOD_EXPORT const char*
modin_get_loadid(); // for non-file types, return a string identifier (ex.
                    // midiin...) not implemented yet

// data is data object returned by newdata function
// filename is complete (and should be in OS's native format) and exists
// return true on success
FOMUSMOD_EXPORT int modin_load(FOMUS f, void* data, const char* text,
                               int isfile);

#endif

LIBFOMUS_EXPORT module_obj modout_export(module_partobj part);

// SPECIAL FUNCTIONS (TOO SPECIAL FOR MODNOTES.H)
LIBFOMUS_EXPORT int
modout_beamsleft(module_noteobj note); // returns number of bars on beam
LIBFOMUS_EXPORT int modout_beamsright(module_noteobj note);
LIBFOMUS_EXPORT int modout_connbeamsleft(
    module_noteobj note); // returns number of connected bars on beam
LIBFOMUS_EXPORT int modout_connbeamsright(module_noteobj note);

LIBFOMUS_EXPORT int modout_isinvisible(module_noteobj note);

LIBFOMUS_EXPORT enum module_barlines modout_rightbarline(module_measobj meas);

enum modout_keysig_modetype {
  keysig_none,
  keysig_common_maj, // a circle-of-fifths maj/min key signature
  keysig_common_min,
  keysig_indiv,    // vector of up to 7 indivual notes in 1 octave
  keysig_fullindiv // vector of up to 75 notes (= diatonic equiv of 128
                   // chromatic notes)
};
struct modout_keysig_indiv {
  int dianote; // -1 if some user-caused error
  struct fomus_rat acc1, acc2;
};
struct modout_keysig {
  enum modout_keysig_modetype mode;
  int dianote, acc; // used only if common-the diatonic note and acc
  int n;
  const struct modout_keysig_indiv*
      indiv; // always used (common still fills it for reference), filled in
             // order defined
};
LIBFOMUS_EXPORT struct modout_keysig modout_keysigdef(module_measobj meas);
LIBFOMUS_EXPORT int modout_keysigequal(struct modout_keysig key1,
                                       struct modout_keysig key2);

LIBFOMUS_EXPORT int modout_markorder(
    module_markobj mark); // usually is 1--spanner end marks might be 0
                          // (beginning of note) or 2 (end of note), begin marks
                          // are 1 (beginning of note, but after 0)

struct modout_tempostr {
  const char* str1;
  struct fomus_rat beat;
  const char* str2;
};
LIBFOMUS_EXPORT struct modout_tempostr modout_tempostr(module_noteobj note,
                                                       module_markobj mark);

#ifndef FOMUSMOD_HIDE

// CALLBACKS
// return lowercase filename extension strings (0 when the last is reached)
FOMUSMOD_EXPORT const char* modout_get_extension(int n);
// return a loadid that can be used instead of a filename extension (this is
// checked first!)--return 0 if none (=file input)
FOMUSMOD_EXPORT const char*
modout_get_saveid(); // for non-file types, return a string identifier (ex.
                     // midiin...) not implemented yet
FOMUSMOD_EXPORT fomus_bool
modout_ispre(); // whether or not module writes before processing... usually
                // return false here (fmsout.cc returns true)
enum modout_sorttype {
  modout_part,
  modout_measure,
  modout_staff,
  modout_voice,
  modout_offset
};
struct modout_sorttypes {
  int n;
  enum modout_sorttype* types;
};
FOMUSMOD_EXPORT struct modout_sorttypes
modout_sorttype(); // how everything is to be sorted

// data is data object returned by newdata function
// filename is complete (and should be in OS's native format) and exists
FOMUSMOD_EXPORT void modout_write(FOMUS f, void* data, const char* filename);

#endif

#ifdef __cplusplus
}
#endif

// --------------------------------------------------------------------------------
// C++ stuff
#if defined(__cplusplus) && !defined(BUILD_LIBFOMUS)

inline fomus_rat operator-(const struct fomus_rat& x) {
  return module_ratneg(x);
}

inline bool operator==(const struct fomus_rat& a, const struct fomus_rat& b) {
  return module_rateq(a, b);
}
inline bool operator!=(const struct fomus_rat& a, const struct fomus_rat& b) {
  return module_ratneq(a, b);
}
inline bool operator<(const struct fomus_rat& a, const struct fomus_rat& b) {
  return module_ratlt(a, b);
}
inline bool operator<=(const struct fomus_rat& a, const struct fomus_rat& b) {
  return module_ratlteq(a, b);
}
inline bool operator>(const struct fomus_rat& a, const struct fomus_rat& b) {
  return module_ratgt(a, b);
}
inline bool operator>=(const struct fomus_rat& a, const struct fomus_rat& b) {
  return module_ratgteq(a, b);
}
inline struct fomus_rat operator+(const struct fomus_rat& a,
                                  const struct fomus_rat& b) {
  return module_ratplus(a, b);
}
inline struct fomus_rat operator-(const struct fomus_rat& a,
                                  const struct fomus_rat& b) {
  return module_ratminus(a, b);
}
inline struct fomus_rat operator*(const struct fomus_rat& a,
                                  const struct fomus_rat& b) {
  return module_ratmult(a, b);
}
inline struct fomus_rat operator/(const struct fomus_rat& a,
                                  const struct fomus_rat& b) {
  return module_ratdiv(a, b);
}

inline bool operator==(const struct fomus_rat& a, const fomus_int b) {
  return module_rateq(a, module_inttorat(b));
}
inline bool operator!=(const struct fomus_rat& a, const fomus_int b) {
  return module_ratneq(a, module_inttorat(b));
}
inline bool operator<(const struct fomus_rat& a, const fomus_int b) {
  return module_ratlt(a, module_inttorat(b));
}
inline bool operator<=(const struct fomus_rat& a, const fomus_int b) {
  return module_ratlteq(a, module_inttorat(b));
}
inline bool operator>(const struct fomus_rat& a, const fomus_int b) {
  return module_ratgt(a, module_inttorat(b));
}
inline bool operator>=(const struct fomus_rat& a, const fomus_int b) {
  return module_ratgteq(a, module_inttorat(b));
}
inline struct fomus_rat operator+(const struct fomus_rat& a,
                                  const fomus_int b) {
  return module_ratplus(a, module_inttorat(b));
}
inline struct fomus_rat operator-(const struct fomus_rat& a,
                                  const fomus_int b) {
  return module_ratminus(a, module_inttorat(b));
}
inline struct fomus_rat operator*(const struct fomus_rat& a,
                                  const fomus_int b) {
  return module_ratmult(a, module_inttorat(b));
}
inline struct fomus_rat operator/(const struct fomus_rat& a,
                                  const fomus_int b) {
  return module_ratdiv(a, module_inttorat(b));
}

inline bool operator==(const fomus_int a, const struct fomus_rat& b) {
  return module_rateq(module_inttorat(a), b);
}
inline bool operator!=(const fomus_int a, const struct fomus_rat& b) {
  return module_ratneq(module_inttorat(a), b);
}
inline bool operator<(const fomus_int a, const struct fomus_rat& b) {
  return module_ratlt(module_inttorat(a), b);
}
inline bool operator<=(const fomus_int a, const struct fomus_rat& b) {
  return module_ratlteq(module_inttorat(a), b);
}
inline bool operator>(const fomus_int a, const struct fomus_rat& b) {
  return module_ratgt(module_inttorat(a), b);
}
inline bool operator>=(const fomus_int a, const struct fomus_rat& b) {
  return module_ratgteq(module_inttorat(a), b);
}
inline struct fomus_rat operator+(const fomus_int a,
                                  const struct fomus_rat& b) {
  return module_ratplus(module_inttorat(a), b);
}
inline struct fomus_rat operator-(const fomus_int a,
                                  const struct fomus_rat& b) {
  return module_ratminus(module_inttorat(a), b);
}
inline struct fomus_rat operator*(const fomus_int a,
                                  const struct fomus_rat& b) {
  return module_ratmult(module_inttorat(a), b);
}
inline struct fomus_rat operator/(const fomus_int a,
                                  const struct fomus_rat& b) {
  return module_ratdiv(module_inttorat(a), b);
}

inline bool operator==(const struct fomus_rat& a, const fomus_float b) {
  return module_rattofloat(a) == b;
}
inline bool operator!=(const struct fomus_rat& a, const fomus_float b) {
  return module_rattofloat(a) != b;
}
inline bool operator<(const struct fomus_rat& a, const fomus_float b) {
  return module_rattofloat(a) < b;
}
inline bool operator<=(const struct fomus_rat& a, const fomus_float b) {
  return module_rattofloat(a) <= b;
}
inline bool operator>(const struct fomus_rat& a, const fomus_float b) {
  return module_rattofloat(a) > b;
}
inline bool operator>=(const struct fomus_rat& a, const fomus_float b) {
  return module_rattofloat(a) >= b;
}
inline fomus_float operator+(const struct fomus_rat& a, const fomus_float b) {
  return module_rattofloat(a) + b;
}
inline fomus_float operator-(const struct fomus_rat& a, const fomus_float b) {
  return module_rattofloat(a) - b;
}
inline fomus_float operator*(const struct fomus_rat& a, const fomus_float b) {
  return module_rattofloat(a) * b;
}
inline fomus_float operator/(const struct fomus_rat& a, const fomus_float b) {
  return module_rattofloat(a) / b;
}

inline bool operator==(const fomus_float a, const struct fomus_rat& b) {
  return a == module_rattofloat(b);
}
inline bool operator!=(const fomus_float a, const struct fomus_rat& b) {
  return a != module_rattofloat(b);
}
inline bool operator<(const fomus_float a, const struct fomus_rat& b) {
  return a < module_rattofloat(b);
}
inline bool operator<=(const fomus_float a, const struct fomus_rat& b) {
  return a <= module_rattofloat(b);
}
inline bool operator>(const fomus_float a, const struct fomus_rat& b) {
  return a > module_rattofloat(b);
}
inline bool operator>=(const fomus_float a, const struct fomus_rat& b) {
  return a >= module_rattofloat(b);
}
inline fomus_float operator+(const fomus_float a, const struct fomus_rat& b) {
  return a + module_rattofloat(b);
}
inline fomus_float operator-(const fomus_float a, const struct fomus_rat& b) {
  return a - module_rattofloat(b);
}
inline fomus_float operator*(const fomus_float a, const struct fomus_rat& b) {
  return a * module_rattofloat(b);
}
inline fomus_float operator/(const fomus_float a, const struct fomus_rat& b) {
  return a / module_rattofloat(b);
}

struct fomus_exception {};
inline const module_value& module_makeval(const module_value& r) {
  return r;
}
inline module_value module_makeval(const fomus_int x) {
  module_value r;
  r.type = module_int;
  r.val.i = x;
  return r;
}
inline module_value module_makeval(const fomus_rat& x) {
  module_value r;
  r.type = module_rat;
  r.val.r = x;
  return r;
}
inline module_value module_makeval(const fomus_int n, const fomus_int d) {
  module_value r;
  r.type = module_rat;
  r.val.r.num = n;
  r.val.r.den = d;
  return r;
}
inline module_value module_makeval(const fomus_float x) {
  module_value r;
  r.type = module_float;
  r.val.f = x;
  return r;
}
inline module_value module_makenullval() {
  module_value r;
  r.type = module_none;
  return r;
}

inline module_value operator-(const module_value& a) {
  switch (a.type) {
  case module_int:
    return module_makeval(-a.val.i);
  case module_rat:
    return module_makeval(-a.val.r);
  case module_float:
    return module_makeval(-a.val.f);
  default:
    throw fomus_exception();
  }
}

template <typename T>
inline bool operator==(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return a.val.i == b;
  case module_rat:
    return a.val.r == b;
  case module_float:
    return a.val.f == b;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline bool operator!=(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return a.val.i != b;
  case module_rat:
    return a.val.r != b;
  case module_float:
    return a.val.f != b;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline bool operator<(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return a.val.i < b;
  case module_rat:
    return a.val.r < b;
  case module_float:
    return a.val.f < b;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline bool operator<=(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return a.val.i <= b;
  case module_rat:
    return a.val.r <= b;
  case module_float:
    return a.val.f <= b;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline bool operator>(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return a.val.i > b;
  case module_rat:
    return a.val.r > b;
  case module_float:
    return a.val.f > b;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline bool operator>=(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return a.val.i >= b;
  case module_rat:
    return a.val.r >= b;
  case module_float:
    return a.val.f >= b;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline module_value operator+(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return module_makeval(a.val.i + b);
  case module_rat:
    return module_makeval(a.val.r + b);
  case module_float:
    return module_makeval(a.val.f + b);
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline module_value operator-(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return module_makeval(a.val.i - b);
  case module_rat:
    return module_makeval(a.val.r - b);
  case module_float:
    return module_makeval(a.val.f - b);
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline module_value operator*(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return module_makeval(a.val.i * b);
  case module_rat:
    return module_makeval(a.val.r * b);
  case module_float:
    return module_makeval(a.val.f * b);
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline module_value operator/(const module_value& a, const T& b) {
  switch (a.type) {
  case module_int:
    return module_makeval(a.val.i / b);
  case module_rat:
    return module_makeval(a.val.r / b);
  case module_float:
    return module_makeval(a.val.f / b);
  default:
    throw fomus_exception();
  }
}
template <>
inline module_value operator/
    <fomus_int>(const module_value& a, const fomus_int& b) {
  switch (a.type) {
  case module_int:
    return (a.val.i % b) ? module_makeval(module_makerat_reduce(a.val.i, b))
                         : module_makeval(a.val.i / b);
  case module_rat:
    return module_makeval(a.val.r / b);
  case module_float:
    return module_makeval(a.val.f / b);
  default:
    throw fomus_exception();
  }
}

template <typename T>
inline bool operator==(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a == b.val.i;
  case module_rat:
    return a == b.val.r;
  case module_float:
    return a == b.val.f;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline bool operator!=(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a != b.val.i;
  case module_rat:
    return a != b.val.r;
  case module_float:
    return a != b.val.f;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline bool operator<(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a < b.val.i;
  case module_rat:
    return a < b.val.r;
  case module_float:
    return a < b.val.f;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline bool operator<=(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a <= b.val.i;
  case module_rat:
    return a <= b.val.r;
  case module_float:
    return a <= b.val.f;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline bool operator>(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a > b.val.i;
  case module_rat:
    return a > b.val.r;
  case module_float:
    return a > b.val.f;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline bool operator>=(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a >= b.val.i;
  case module_rat:
    return a >= b.val.r;
  case module_float:
    return a >= b.val.f;
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline module_value operator+(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return module_makeval(a + b.val.i);
  case module_rat:
    return module_makeval(a + b.val.r);
  case module_float:
    return module_makeval(a + b.val.f);
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline module_value operator-(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return module_makeval(a - b.val.i);
  case module_rat:
    return module_makeval(a - b.val.r);
  case module_float:
    return module_makeval(a - b.val.f);
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline module_value operator*(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return module_makeval(a * b.val.i);
  case module_rat:
    return module_makeval(a * b.val.r);
  case module_float:
    return module_makeval(a * b.val.f);
  default:
    throw fomus_exception();
  }
}
template <typename T>
inline module_value operator/(const T& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return module_makeval(a / b.val.i);
  case module_rat:
    return module_makeval(a / b.val.r);
  case module_float:
    return module_makeval(a / b.val.f);
  default:
    throw fomus_exception();
  }
}
template <>
inline module_value operator/
    <fomus_int>(const fomus_int& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return (a % b.val.i) ? module_makeval(module_makerat_reduce(a, b.val.i))
                         : module_makeval(a / b.val.i);
  case module_rat:
    return module_makeval(a / b.val.r);
  case module_float:
    return module_makeval(a / b.val.f);
  default:
    throw fomus_exception();
  }
}

inline bool operator==(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a == b.val.i;
  case module_rat:
    return a == b.val.r;
  case module_float:
    return a == b.val.f;
  default:
    throw fomus_exception();
  }
}
inline bool operator!=(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a != b.val.i;
  case module_rat:
    return a != b.val.r;
  case module_float:
    return a != b.val.f;
  default:
    throw fomus_exception();
  }
}
inline bool operator<(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a < b.val.i;
  case module_rat:
    return a < b.val.r;
  case module_float:
    return a < b.val.f;
  default:
    throw fomus_exception();
  }
}
inline bool operator<=(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a <= b.val.i;
  case module_rat:
    return a <= b.val.r;
  case module_float:
    return a <= b.val.f;
  default:
    throw fomus_exception();
  }
}
inline bool operator>(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a > b.val.i;
  case module_rat:
    return a > b.val.r;
  case module_float:
    return a > b.val.f;
  default:
    throw fomus_exception();
  }
}
inline bool operator>=(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a >= b.val.i;
  case module_rat:
    return a >= b.val.r;
  case module_float:
    return a >= b.val.f;
  default:
    throw fomus_exception();
  }
}
inline module_value operator+(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a + b.val.i;
  case module_rat:
    return a + b.val.r;
  case module_float:
    return a + b.val.f;
  default:
    throw fomus_exception();
  }
}
inline module_value operator-(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a - b.val.i;
  case module_rat:
    return a - b.val.r;
  case module_float:
    return a - b.val.f;
  default:
    throw fomus_exception();
  }
}
inline module_value operator*(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a * b.val.i;
  case module_rat:
    return a * b.val.r;
  case module_float:
    return a * b.val.f;
  default:
    throw fomus_exception();
  }
}
inline module_value operator/(const module_value& a, const module_value& b) {
  switch (b.type) {
  case module_int:
    return a / b.val.i;
  case module_rat:
    return a / b.val.r;
  case module_float:
    return a / b.val.f;
  default:
    throw fomus_exception();
  }
}

#endif

#endif
