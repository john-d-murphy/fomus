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

#ifndef FOMUS_INFOAPI_H
#define FOMUS_INFOAPI_H

#include "modtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

  // modules
  #define FOMUS_INFOAPI_VERSION 2
    
  struct info_module {
    const char* name;
    const char* filename;
    const char* longname;
    const char* author;
    const char* doc;
    enum module_type type;
    int ifaceid;
  };
  struct info_modlist {
    int n;
    struct info_module* mods;
  };

  struct info_modfilter {
    const char* name;
    const char* longname;
    const char* author;
    enum module_type type;
    int ifaceid;
  };
  struct info_modfilterlist {
    int n;
    struct info_modfilter* mods;
  };

  struct info_modsearch {
    const char* name;
    const char* longname;
    const char* author;
    const char* doc;
  };

  // settings

  enum info_setwhere { // MUST MATCH ENUM IN MODINOUTAPI!!!
    info_none,
    
    info_default, // default means all
    info_config,
    info_global    
  };
  
  struct info_setting {
    const char* modname;
    
    const char* name;
    int id; // id used for fast lookup
    enum module_value_type basetype;
    const char* typedoc;
    const char* descdoc;
    enum module_setting_loc loc;
    struct module_value val;
    const char* valstr; // default value, formatted as string
    int uselevel;
    enum info_setwhere where;
    int reset; // if true, a reset is necessary after setting is set
  };
  struct info_setlist {
    int n;
    struct info_setting* sets;
  };

  struct info_setfilter {
    const char* modname;
    const char* modlongname;
    const char* modauthor;
    enum module_type modtype;
    
    const char* name;
    enum module_setting_loc loc;
    int uselevel;
    enum info_setwhere where; // where it was defined
  };
  struct info_setfilterlist {
    int n;
    struct info_setfilter* sets;
  };

  struct info_setsearch {
    const char* modname;
    const char* modlongname;
    const char* modauthor;
    const char* moddoc;

    const char* name;
    const char* doc;
  };

  // marks

  struct info_mark {
    const char* modname;
    
    const char* name;
    const char* argsdoc;
    enum module_value_type type; // string, stringnum or number?
    const char* descdoc;
  };
  struct info_marklist {
    int n;
    struct info_mark* marks;
  };

  struct info_markfilter {
    const char* modname;
    const char* modlongname;
    const char* modauthor;
    enum module_type modtype;

    const char* name;
  };
  struct info_markfilterlist {
    int n;
    struct info_markfilter* marks;
  };

  struct info_marksearch {
    const char* modname;
    const char* modlongname;
    const char* modauthor;
    const char* moddoc;

    const char* name;
    const char* doc;
  };
  
  // sorting mods & sets
  
  enum info_key {
    info_modname,
    info_modlongname,
    info_modauthor,
    info_modtype,
    
    info_setname,
    info_setloc,
    info_setuselevel,

    info_markname,
    info_markdoc
  };
  enum info_sort {
    info_ascending,
    info_descending
  };
  
  struct info_sortpair {
    enum info_key key;
    enum info_sort sort;
  };
  struct info_sortlist {
    int n;
    struct info_sortpair* keys;
  };

  struct info_extslist {
    int n;
    const char** exts;
  };

#ifndef LIBFOMUS_HIDE

  LIBFOMUS_EXPORT int info_infoapi_version();
  
  // if limit < 0, all values returned
  // if no sortlst given, sorted alphabetically by name
  // NULL pointers accepted in _list_ functions
  LIBFOMUS_EXPORT const struct info_modlist info_list_modules(struct info_modfilterlist* filter, struct info_modsearch* sim, struct info_sortlist* sortlst, int limit);
  LIBFOMUS_EXPORT const struct info_setlist info_list_settings(FOMUS fom, struct info_setfilterlist* filter, struct info_setsearch* sim, struct info_sortlist* sortlst, int limit);
  LIBFOMUS_EXPORT const struct info_marklist info_list_marks(FOMUS fom, struct info_markfilterlist* filter, struct info_marksearch* sim, struct info_sortlist* sortlst, int limit);
  // backend extensions;
  LIBFOMUS_EXPORT const struct info_extslist info_list_exts();

  LIBFOMUS_EXPORT const char* info_get_fomusconfigfile();
  LIBFOMUS_EXPORT const char* info_get_userconfigfile();

  // these are here because this is where they'll get used
  LIBFOMUS_EXPORT const char* info_modtype_to_str(enum module_type type);
  LIBFOMUS_EXPORT enum module_type info_str_to_modtype(const char* str);

  LIBFOMUS_EXPORT const char* info_settingloc_to_str(enum module_setting_loc loc);
  LIBFOMUS_EXPORT const char* info_settingloc_to_extstr(enum module_setting_loc loc);
  LIBFOMUS_EXPORT enum module_setting_loc info_str_to_settingloc(const char* str);

#endif

  struct info_objinfo {
    module_obj obj;
    struct module_value val;
    const char* valstr;
  };
  struct info_objinfo_list {
    int n;
    struct info_objinfo *objs;
  };

#ifndef LIBFOMUS_HIDE
  
  LIBFOMUS_EXPORT const struct info_setlist info_get_settings(module_obj o); 
  LIBFOMUS_EXPORT const struct info_marklist info_get_marks();

  LIBFOMUS_EXPORT const struct info_objinfo_list info_get_percinsts(FOMUS f);
  LIBFOMUS_EXPORT const struct info_objinfo_list info_get_insts(FOMUS f);
  LIBFOMUS_EXPORT const struct info_objinfo_list info_get_parts(FOMUS f);
  LIBFOMUS_EXPORT const struct info_objinfo_list info_get_metaparts(FOMUS f);
  LIBFOMUS_EXPORT const struct info_objinfo_list info_get_measdefs(FOMUS f);

  LIBFOMUS_EXPORT const char* info_valstr(module_obj obj); // returns "" if default, "id" if has id, or full def if no id--use module_free_string to free it

  LIBFOMUS_EXPORT struct info_objinfo info_getlastentry(FOMUS f); // if one of the above has been entered, this gets the last entry (so frontend can redisplay it, etc..)

  // if f is NULL, you get the default value (only need to do this from within validdeps function)
  LIBFOMUS_EXPORT fomus_int fomus_get_ival(FOMUS f, const char* set);
  LIBFOMUS_EXPORT struct fomus_rat fomus_get_rval(FOMUS f, const char* set);
  LIBFOMUS_EXPORT fomus_float fomus_get_fval(FOMUS f, const char* set);
  LIBFOMUS_EXPORT const char* fomus_get_sval(FOMUS f, const char* set);
  
#endif

#ifdef __cplusplus
}
#endif

#endif
