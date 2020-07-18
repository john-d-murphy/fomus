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

#ifndef FOMUS_ERROR_H
#define FOMUS_ERROR_H

#ifdef BUILD_LIBFOMUS
#include "heads.h"
#endif

#include "fomusapi.h"
#include "modutil.h"

namespace FNAMESPACE {

  class errbase {};

// anything other than library shouldn't need this
#ifdef BUILD_LIBFOMUS

  class stage;
  inline void delstageobj(stage*) {} // no cleanup--ptr_vector
  extern boost::thread_specific_ptr<stage> stageobj;
  class fomusdata;
  inline void delfomusdata0(fomusdata*) {} // no cleanup--ptr_vector
  extern boost::thread_specific_ptr<fomusdata> threadfd;
  inline void delthreadcharptr(char* x) {
    delete[] x;
  }
  extern boost::thread_specific_ptr<char> threadcharptr;
  inline void delvoidobj(int*) {} // no cleanup--ptr_vector
  extern boost::thread_specific_ptr<int> fomerr;

  extern const std::string intname;

  inline void setfomuserr() {
    fomerr.reset((int*) 1);
  }
  inline void resetfomuserr() {
    fomerr.reset((int*) 0);
  }
  inline bool getfomuserr() {
    return (bool) fomerr.get();
  }

#define CERR fomus::ferr

  extern bool isinited;

  extern boost::mutex outlock;

  // OUTPUT/ERR STREAMS
  class myout : public boost::iostreams::sink {
    std::string x;
    fomus_output fun;
    bool iserr;

public:
    myout(const fomus_output fun, FILE* s, const bool iserr)
        : fun(fun), iserr(iserr) {
      setbuf(s, 0);
    }
    std::streamsize write(const char* s, std::streamsize n);
    void setfun(const fomus_output fun0) {
      fun = fun0;
    }
  };

  // the standard in/out for fomus
  extern boost::iostreams::stream<myout> fout, ferr;

  inline void checkinit() {
    if (!isinited) {
      CERR << "not initialized or error during initialization" << std::endl;
      throw errbase();
    }
  }

#define ZEROLIST                                                               \
  { 0 }

#define ENTER_MAINAPI                                                          \
  resetfomuserr();                                                             \
  try {                                                                        \
    boost::shared_lock<boost::shared_mutex> xxx(listenermut);                  \
    catchup();
#define ENTER_MAINAPIENT                                                       \
  resetfomuserr();                                                             \
  try {                                                                        \
    boost::shared_lock<boost::shared_mutex> xxx(listenermut);
#define ENTER_APIRT try {
#define ENTER_API                                                              \
  resetfomuserr();                                                             \
  try {

#define EXIT_API_VOID                                                          \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
  }
#define EXIT_API_0                                                             \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    return 0;                                                                  \
  }
#define EXIT_API_0MARKPOS                                                      \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    return markpos_notehead;                                                   \
  }
#define EXIT_API_0MODTYPE                                                      \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    return module_nomodtype;                                                   \
  }
#define EXIT_API_0MODSETLOC                                                    \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    return module_noloc;                                                       \
  }
#define EXIT_API_PART0                                                         \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    return parts_nogroup;                                                      \
  }
#define EXIT_API_0_RET                                                         \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    return 0;                                                                  \
  }                                                                            \
  return 1
#define EXIT_API_RAT0                                                          \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    fomus_rat r = {0, 1};                                                      \
    return r;                                                                  \
  }
#define EXIT_API_MODLIST0                                                      \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct module_list r = ZEROLIST;                                           \
    return r;                                                                  \
  }
#define EXIT_API_MODOBJLIST0                                                   \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct module_objlist r = ZEROLIST;                                        \
    return r;                                                                  \
  }
#define EXIT_API_INOUTOBJLIST0                                                 \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct info_objinfo_list r = ZEROLIST;                                     \
    return r;                                                                  \
  }
#define EXIT_API_MODVAL0                                                       \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct module_value r = {module_none};                                     \
    return r;                                                                  \
  }
#define EXIT_API_FMODLIST0                                                     \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct info_modlist r = ZEROLIST;                                          \
    return r;                                                                  \
  }
#define EXIT_API_SETLIST0                                                      \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct info_setlist r = ZEROLIST;                                          \
    return r;                                                                  \
  }
#define EXIT_API_SETIDLIST0                                                    \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct info_setid_list r = ZEROLIST;                                       \
    return r;                                                                  \
  }
#define EXIT_API_INTSLIST0                                                     \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct module_intslist r = ZEROLIST;                                       \
    return r;                                                                  \
  }
#define EXIT_API_CLEFSLIST0                                                    \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct module_intslist r = ZEROLIST;                                       \
    return r;                                                                  \
  }
#define EXIT_API_RATSLIST0                                                     \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct module_ratslist r = ZEROLIST;                                       \
    return r;                                                                  \
  }
#define EXIT_API_MARKSLIST0                                                    \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct module_markslist r = ZEROLIST;                                      \
    return r;                                                                  \
  }
#define EXIT_API_MARKLIST0                                                     \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct info_marklist r = ZEROLIST;                                         \
    return r;                                                                  \
  }
#define EXIT_API_PARTMAPS0                                                     \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct metaparts_partmaps r = ZEROLIST;                                    \
    return r;                                                                  \
  }
#define EXIT_API_PERCINSTS0                                                    \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct percnotes_percinsts r = ZEROLIST;                                   \
    return r;                                                                  \
  }
#define EXIT_API_MARKEVS0                                                      \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct module_objlist r = ZEROLIST;                                        \
    return r;                                                                  \
  }
#define EXIT_API_INFOEXTOBJINFO0                                               \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct info_objinfo r = {0, {module_none}, 0};                             \
    return r;                                                                  \
  }
#define EXIT_API_0EXTSLIST                                                     \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct info_extslist r = ZEROLIST;                                         \
    return r;                                                                  \
  }
#define EXIT_API_0KEYSIGSTR                                                    \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct modout_keysig r = {keysig_none, 0, 0, 0};                           \
    return r;                                                                  \
  }
#define EXIT_API_0IMPORTS                                                      \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct modin_imports r = ZEROLIST;                                         \
    return r;                                                                  \
  }
#define EXIT_API_KEYSIGREF0                                                    \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct module_keysigref r = {{0, 1}, {0, 1}};                              \
    return r;                                                                  \
  }
#define EXIT_MODUTIL_LOWMULTS0                                                 \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct modutil_lowmults r = ZEROLIST;                                      \
    return r;                                                                  \
  }
#define EXIT_MODUTIL_RHYTHM0                                                   \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct modutil_rhythm r = {{0, 1}, 0};                                     \
    return r;                                                                  \
  }
#define EXIT_MODUTIL_RANGES0                                                   \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct modutil_ranges r = ZEROLIST;                                        \
    return r;                                                                  \
  }
#define EXIT_API_MODBARLINES                                                   \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    return barline_normal;                                                     \
  }
#define EXIT_API_TEMPOSTR                                                      \
  }                                                                            \
  catch (const errbase& e) {                                                   \
    setfomuserr();                                                             \
    struct modout_tempostr r = {0, {0, 1}, 0};                                 \
    return r;                                                                  \
  }

#endif
} // namespace FNAMESPACE
#endif
