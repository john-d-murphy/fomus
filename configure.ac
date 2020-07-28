# -*- Autoconf -*-

#   Copyright (C) 2009, 2010, 2011  David Psenicka
#   This file is part of FOMUS.

#   FOMUS is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   FOMUS is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.


# Process this file with autoconf to produce a configure script.

# FOR RELEASE:

# change version number
# build release
# docs:
#   add all tut*.fms, ex*.fms, lsp*.lisp, sal*.sal and mid*.fms to doc/Makefile.am
#   add all tut*.fms and ex*.fms to src/test/Makefile.am
#   strip whitespace from all *.fms, *.lisp and *.sal files
#   do make update, make html and make install-html
# tests:
#   do check and installcheck
#   update cm*.png files
# update ChangeLog and TODO
# update web page version number and news
# do make distcheck and everything else

AC_PREREQ(2.61)

# format: ?.?.?-alpha-rc?
AC_INIT([FOMUS],[0.1.18-alpha],[fomus-user@lists.sourceforge.net])

AC_CONFIG_AUX_DIR([libltdl/config])
AC_CONFIG_SRCDIR([src/lib/api.cc])
AM_INIT_AUTOMAKE([no-installinfo foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_HOST
AC_LANG([C++])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
# AC_PROG_RANLIB rendered obsolete by `AC_PROG_LIBTOOL'

# New libtool configuration
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT([dlopen] [win32-dll])
LTDL_INIT([recursive])
AC_SUBST(LTDLINCL)
AC_SUBST(LIBLTDL)

AC_PROG_SED
AC_SUBST(SED)

# Debug
AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [Compile for debugging])], [SPDEBUG="$enableval"], [SPDEBUG="no"])
AC_ARG_ENABLE([dbgout], [AS_HELP_STRING([--enable-dbgout], [Enable copious debugging output])], [SPDEBUGOUT="$enableval"], [SPDEBUGOUT="no"])
AC_ARG_ENABLE([dbgass], [AS_HELP_STRING([--disable-dbgass], [Disable assertions in code (if debugging is enabled)])], [SPDEBUGCODE="$enableval"], [SPDEBUGCODE="yes"])
AS_IF([test "x$SPDEBUGOUT" != "xno"], [SPDEBUG="yes"])
AS_IF([test "x$SPDEBUG" = "xno"], [SPDEBUGOUT="no"])

# Profiling
AC_ARG_ENABLE([prof], [AS_HELP_STRING([--enable-prof], [Compile with prof profiling information])], [SPPROF="$enableval"], [SPPROF="no"])
AS_IF([test "x$SPPROF" != "xno"], [PROFFLAGS0=" -p"])
AC_ARG_ENABLE([gprof], [AS_HELP_STRING([--enable-gprof], [Compile with gprof profiling information])], [SPGPROF="$enableval"], [SPGPROF="no"])
AS_IF([test "x$SPGPROF" != "xno"], [PROFFLAGS1=" -pg"])
AC_ARG_ENABLE([gcov], [AS_HELP_STRING([--enable-gcov], [Compile with gcov profiling information])], [SPGCOV="$enableval"], [SPGCOV="no"])
AS_IF([test "x$SPGCOV" != "xno"], [PROFFLAGS2=" -fprofile-arcs -ftest-coverage"])
PROFFLAGS="$PROFFLAGS0$PROFFLAGS1$PROFFLAGS2"

# static boost?
dnl AC_ARG_ENABLE([static-boost], [AS_HELP_STRING([--enable-static-boost], [Link against Boost static libraries])], [SPSTAT="$enableval"], [SPSTAT="no"])
dnl AS_IF([test "x$SPSTAT" = "xno"], 
dnl       [BST_=""] [_BST=""], 
dnl       [BST_="-Wl,-static"] [_BST="-Wl,-call_shared"])
dnl AC_SUBST(BST_)
dnl AC_SUBST(_BST)

# 32/64 bit
AC_ARG_ENABLE([thirtytwobit], [AS_HELP_STRING([--enable-32bit], [Compile for a 32bit processor])], [SPTHIRTYTWOBIT="$enableval"], [SPTHIRTYTWOBIT="no"])
AC_ARG_ENABLE([sixtyfourbit], [AS_HELP_STRING([--enable-64bit], [Compile for a 64bit processor])], [SPSIXTYFOURBIT="$enableval"], [SPSIXTYFOURBIT="no"])
AS_IF([test "x$SPSIXTYFOURBIT" != "xno"], [SPBIT=" -m64"])
AS_IF([test "x$SPTHIRTYTWOBIT" != "xno"], [SPBIT=" -m32"])

# Pd
AC_ARG_WITH([puredata], [AS_HELP_STRING([--with-puredata], [Compile Pd external])], [PD_WITH="$withval"], [PD_WITH="yes"])

# Flags
AS_IF([test "x$SPDEBUG" = "xno" -o "x$SPDEBUGCODE" = "xno"], [DBGFLAGS="-DNDEBUG"], [AX_CXXFLAGS_WARN_ALL(WARNFLAGS)])
AS_IF([test "x$SPDEBUGOUT" = "xno"], [DBGOUTFLAGS=" -DNDEBUGOUT"])
AS_IF([test "x$SPDEBUGCODE" = "xno"], [DBGASSFLAGS=" -DNDEBUGASS"])
AX_NOT_ENABLE_FRAME_POINTER(OMITFLAGS)
# AX_CHECK_COMPILER_FLAGS([-ffinite-math-only], [MATHFLAGS+=" -ffinite-math-only"])
# AX_CHECK_COMPILER_FLAGS([-fno-math-errno], [MATHFLAGS+=" -fno-math-errno"])
# AX_CHECK_COMPILER_FLAGS([-fno-trapping-math], [MATHFLAGS+=" -fno-trapping-math"])
# AX_CHECK_COMPILER_FLAGS([-fno-signaling-nans], [MATHFLAGS+=" -fno-signaling-nans"])
# AX_CHECK_COMPILER_FLAGS([-frepo], [OPTFLAGS+=" -frepo"])
AX_CHECK_COMPILER_FLAGS([-fno-omit-frame-pointer], [DEBFLAGS+=" -fno-omit-frame-pointer"])
AX_CHECK_COMPILER_FLAGS([-ftrapv], [DEBFLAGS+=" -ftrapv"])
# AX_CHECK_COMPILER_FLAGS([-fnon-call-exceptions], [DEBFLAGS+=" -fnon-call-exceptions"])
AX_CHECK_COMPILER_FLAGS([-fsignaling-nans], [DEBFLAGS+=" -fsignaling-nans"])
# AX_CHECK_COMPILER_FLAGS([-fno-inline], [DEBFLAGS+=" -fno-inline"])
AX_CHECK_COMPILER_FLAGS([-fvisibility=hidden], [EXPFLAGS+=" -fvisibility=hidden"], [EXPCPPFLAGS+=" -DDONTHAVE_VISIBILITY"])

# Debug/Nondebug flags
AS_IF([test "x$SPDEBUG" != "xno"], 

      [CFLAGS="-g -O0$DEBFLAGS$PROFFLAGS$SPBIT"] [CXXFLAGS="-g -O0$DEBFLAGS$PROFFLAGS$SPBIT"] 
      [FOMUS_CFLAGS="$WARNFLAGS$EXPFLAGS"] [FOMUS_CFLAGSX="$WARNFLAGS"] 
      [FOMUS_CPPFLAGS="$DBGFLAGS$DBGOUTFLAGS$DBGASSFLAGS$EXPCPPFLAGS"], 

      [FOMUS_CFLAGS="-O2$OMITFLAGS$OPTFLAGS$EXPFLAGS"] [FOMUS_CFLAGSX="-O2$OMITFLAGS$OPTFLAGS"] 
      [FOMUS_CPPFLAGS="$DBGFLAGS$DBGOUTFLAGS$DBGASSFLAGS$EXPCPPFLAGS"] 
      [FOMUS_LDFLAGS="-s"]
      [CFLAGS="`echo $CFLAGS | $SED 's/-g //' | $SED 's/-g$//'`$SPBIT"] [CXXFLAGS="`echo $CXXFLAGS | $SED 's/-g //' | $SED 's/-g$//'`$SPBIT"] 
      [CPPFLAGS="-w $DBGFLAGS$DBGOUTFLAGS$DBGASSFLAGS"])

FOMUS_CXXFLAGS=$FOMUS_CFLAGS
FOMUS_CXXFLAGSX=$FOMUS_CFLAGSX
AC_SUBST(FOMUS_CFLAGS)
AC_SUBST(FOMUS_CXXFLAGS)
AC_SUBST(FOMUS_CFLAGSX)
AC_SUBST(FOMUS_CXXFLAGSX)
AC_SUBST(FOMUSLIB_CFLAGS)
AC_SUBST(FOMUS_LDFLAGS)
dnl AC_SUBST(FOMUS_LDFLAGSX)
AC_SUBST(FOMUS_CPPFLAGS)

AS_CASE([$host_os], [cygwin*], [WIN32_LDFLAGS="-no-undefined"], [mingw*], [WIN32_LDFLAGS="-no-undefined"])
AC_SUBST(WIN32_LDFLAGS)
AM_CONDITIONAL([WIN32_BUILD], [test -n "$WIN32_LDFLAGS"])

AS_CASE([$host_os], [mingw*], [ISMINGW="yes"])
AM_CONDITIONAL([MINGW_BUILD], [test -n "$ISMINGW"])

# Boost
AX_BOOST_BASE([1.35])
# copied out of boost .m4 files
MYBOOSTINCDIR=`echo $BOOST_CPPFLAGS | sed -e 's/@<:@^\/@:>@*//'`
MYBOOSTLIBDIR=`echo $BOOST_LDFLAGS | sed -e 's/@<:@^\/@:>@*//'`
LSPROG=`which ls`

# system
AX_BOOST_SYSTEM
BOOST_SYSTEM_LIB_TD=`echo "$BOOST_SYSTEM_LIB" | $SED 's/^-l//' | $SED 's/-[[sgydpn]][[sgydpn]]*\([[^a-z0-9]]\|$\)/\1/'`
AC_MSG_CHECKING([for -l$BOOST_SYSTEM_LIB_TD])
BOOST_SYSTEM_LIB_AFILE=`$LSPROG $MYBOOSTLIBDIR/*$BOOST_SYSTEM_LIB_TD.* 2>/dev/null`
AS_IF([test -n "$BOOST_SYSTEM_LIB_AFILE"],
             [AC_SUBST(BOOST_SYSTEM_DLIB, "-l$BOOST_SYSTEM_LIB_TD")] 
             [AC_MSG_RESULT([yes])], 
             [AC_SUBST(BOOST_SYSTEM_DLIB, "$BOOST_SYSTEM_LIB")] 
             [AC_MSG_RESULT([no])] 
             [BOOSTWARN=1])

# filesystem
AX_BOOST_FILESYSTEM
BOOST_FILESYSTEM_LIB_TD=`echo "$BOOST_FILESYSTEM_LIB" | $SED 's/^-l//' | $SED 's/-[[sgydpn]][[sgydpn]]*\([[^a-z0-9]]\|$\)/\1/'`
AC_MSG_CHECKING([for -l$BOOST_FILESYSTEM_LIB_TD])
BOOST_FILESYSTEM_LIB_AFILE=`$LSPROG $MYBOOSTLIBDIR/*$BOOST_FILESYSTEM_LIB_TD.* 2>/dev/null`
AS_IF([test -n "$BOOST_FILESYSTEM_LIB_AFILE"],
             [AC_SUBST(BOOST_FILESYSTEM_DLIB, "-l$BOOST_FILESYSTEM_LIB_TD")] 
             [AC_MSG_RESULT([yes])], 
             [AC_SUBST(BOOST_FILESYSTEM_DLIB, "$BOOST_FILESYSTEM_LIB")] 
             [AC_MSG_RESULT([no])] 
             [BOOSTWARN=1])

# program options
AX_BOOST_PROGRAM_OPTIONS
BOOST_PROGRAM_OPTIONS_LIB_TD=`echo "$BOOST_PROGRAM_OPTIONS_LIB" | $SED 's/^-l//' | $SED 's/-[[sgydpn]][[sgydpn]]*\([[^a-z0-9]]\|$\)/\1/'`
AC_MSG_CHECKING([for -l$BOOST_PROGRAM_OPTIONS_LIB_TD])
BOOST_PROGRAM_OPTIONS_LIB_AFILE=`$LSPROG $MYBOOSTLIBDIR/*$BOOST_PROGRAM_OPTIONS_LIB_TD.* 2>/dev/null`
AS_IF([test -n "$BOOST_PROGRAM_OPTIONS_LIB_AFILE"],
             [AC_SUBST(BOOST_PROGRAM_OPTIONS_DLIB, "-l$BOOST_PROGRAM_OPTIONS_LIB_TD")] 
             [AC_MSG_RESULT([yes])], 
             [AC_SUBST(BOOST_PROGRAM_OPTIONS_DLIB, "$BOOST_PROGRAM_OPTIONS_LIB")] 
             [AC_MSG_RESULT([no])] 
             [BOOSTWARN=1])

# thread
AX_BOOST_THREAD
BOOST_THREAD_LIB_TD=`echo "$BOOST_THREAD_LIB" | $SED 's/^-l//' | $SED 's/-[[sgydpn]][[sgydpn]]*\([[^a-z0-9]]\|$\)/\1/'`
AC_MSG_CHECKING([for -l$BOOST_THREAD_LIB_TD])
BOOST_THREAD_LIB_AFILE=`$LSPROG $MYBOOSTLIBDIR/*$BOOST_THREAD_LIB_TD.* 2>/dev/null`
AS_IF([test -n "$BOOST_THREAD_LIB_AFILE"],
             [AC_SUBST(BOOST_THREAD_DLIB, "-l$BOOST_THREAD_LIB_TD")] 
             [AC_MSG_RESULT([yes])], 
             [AC_SUBST(BOOST_THREAD_DLIB, "$BOOST_THREAD_LIB")] 
             [AC_MSG_RESULT([no])]
             [BOOSTWARN=1])

# iostreams
AX_BOOST_IOSTREAMS
BOOST_IOSTREAMS_LIB_TD=`echo "$BOOST_IOSTREAMS_LIB" | $SED 's/^-l//' | $SED 's/-[[sgydpn]][[sgydpn]]*\([[^a-z0-9]]\|$\)/\1/'`
AC_MSG_CHECKING([for -l$BOOST_IOSTREAMS_LIB_TD])
BOOST_IOSTREAMS_LIB_AFILE=`$LSPROG $MYBOOSTLIBDIR/*$BOOST_IOSTREAMS_LIB_TD.* 2>/dev/null`
AS_IF([test -n "$BOOST_IOSTREAMS_LIB_AFILE"],
             [AC_SUBST(BOOST_IOSTREAMS_DLIB, "-l$BOOST_IOSTREAMS_LIB_TD")] 
             [AC_MSG_RESULT([yes])], 
             [AC_SUBST(BOOST_IOSTREAMS_DLIB, "$BOOST_IOSTREAMS_LIB")] 
             [AC_MSG_RESULT([no])] 
             [BOOSTWARN=1])

# Checks for libraries.
MP_WITH_CURSES
AC_CHECK_HEADERS([term.h], [FTERM="yes"], [CURSES_LIB=""])
AC_SUBST(CURSES_LIB)
AS_CASE([$host_os], [darwin*], AC_CHECK_HEADERS([m_pd.h], [PD_HEAD=1],
                                                AC_CHECK_HEADERS([/Applications/Pd-extended.app/Contents/Resources/include/m_pd.h], 
                                                                 [PD_HEAD=1] [PD_CPPFLAGS="-I/Applications/Pd-extended.app/Contents/Resources/include"],
                                                                 [PD_DIR=`$LSPROG /Applications/Pd-*.app`] 
       								 AC_CHECK_HEADERS([$PD_DIR/Contents/Resources/src/m_pd.h], 
								                  [PD_HEAD=1] [PD_CPPFLAGS="-I$PD_DIR/Contents/Resources/src"]))), 
                    [linux*], AC_CHECK_HEADERS([m_pd.h], [PD_HEAD=1]), 
                    [cygwin*], AC_CHECK_HEADERS([m_pd.h], [PD_HEAD=1]), 
                    [mingw*], AC_CHECK_HEADERS([m_pd.h], [PD_HEAD=1]))
AC_SUBST(PD_CPPFLAGS)

# Checks for header files.
AC_MSG_CHECKING([whether feenableexcept exists in fenv.h])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#define _GNU_SOURCE 1] 
                                    [#include <fenv.h>]],
                                   [[feenableexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);]])], 
                  [AC_MSG_RESULT([yes])] 
                  [AC_DEFINE([HAVE_FENV_H], [1], [Define if feenableexcept exists in fenv.h.)])], 
                  [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([whether Boost::Spirit's classic header files exist])
AS_IF([test -e "$MYBOOSTINCDIR/boost/spirit/include/classic_core.hpp"], 
      [AC_MSG_RESULT([yes])] 
      [AC_DEFINE(BOOST_SPIRIT_CLASSIC, [1], [Define if Boost::Spirit's classic header files exist.])],
      [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([whether Boost::Iostreams is using old API])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <boost/iostreams/device/file_descriptor.hpp>]],
                                   [[boost::iostreams::file_descriptor_source(0, false);]])], 
                  [AC_MSG_RESULT([yes])] 
                  [AC_DEFINE([BOOST_IOSTREAMS_OLDAPI], [1], [Define if Boost::Iostreams is using old API.])], 
                  [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([whether Boost::Filesystem is using old API])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <boost/filesystem/path.hpp>]],
                                   [[boost::filesystem::path().file_string();]])], 
                  [AC_MSG_RESULT([yes])] 
                  [AC_DEFINE([BOOST_FILESYSTEM_OLDAPI], [1], [Define if Boost::Filesystem is using old API.])], 
                  [AC_MSG_RESULT([no])])

# /dev/null--the output modules route output to it when executing programs
AC_MSG_CHECKING([whether /dev/null exists])
AS_IF([test -e "/dev/null"],
      [AC_MSG_RESULT([yes])]
      [GOT_DEV_NULL=1]
      [AC_DEFINE(DEV_NULL_EXISTS, [1], [Define if /dev/null exists.])],
      [AC_MSG_RESULT([no])])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <unistd.h>])], 
                  AS_IF([test -z "$GOT_DEV_NULL"], [AC_MSG_ERROR([/dev/null is required for building on this system.])]))

# Links
AC_CONFIG_LINKS([src/test/.fomus:src/test/dotfomus])
dnl AC_CONFIG_LINKS([doc/comments.fms:doc/ex007.fms])

# Check if this is repository checkout
AM_CONDITIONAL([ISDEVEL], [test -e "$srcdir/stamp-dev"])
AM_COND_IF([ISDEVEL], [ISDEV=1])
AC_SUBST(ISDEV)

# Emacs
AM_PATH_LISPDIR

# Pd
AS_CASE([$host_os], [darwin*], [PD_EXT="pd_darwin"], [linux*], [PD_EXT="pd_linux"], [cygwin*], [PD_EXT="dll"], [mingw*], [PD_EXT="dll"])
AC_SUBST(PD_EXT)
AM_CONDITIONAL(BUILD_PDEXT, [test -n "$PD_HEAD" -a -n "$PD_EXT" -a "x$PD_WITH" != "xno"])

# LilyPond default exe path
AS_CASE([$host_os], 
        [darwin*], [LILYPOND_PATH="/Applications/LilyPond.app/Contents/Resources/bin/lilypond"] [LILYPOND_VIEWPATH="open"], 
        [linux*], [LILYPOND_PATH="lilypond"], 
        [cygwin*], [LILYPOND_PATH="lilypond"], 
        [mingw*], [LILYPOND_PATH="C:\\\\\\\\Program Files\\\\\\\\LilyPond\\\\\\\\usr\\\\\\\\bin\\\\\\\\lilypond.exe"] 
                  [LILYPOND_VIEWPATH="C:\\\\\\\\Program Files\\\\\\\\Adobe\\\\\\\\Reader 9.0\\\\\\\\Reader\\\\\\\\AcroRd32.exe"])
AC_SUBST(LILYPOND_PATH)
AC_SUBST(LILYPOND_VIEWPATH)

# Troublemaking functions
AC_CHECK_FUNCS([ctime_r localtime_r random])

# Blast Makefiles
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/lib/Makefile
                 src/lib/mod/Makefile
                 src/lib/mod/in/Makefile
                 src/lib/mod/out/Makefile
                 src/lib/mod/eng/Makefile
                 src/lib/mod/check/Makefile
                 src/lib/mod/voices/Makefile
                 src/lib/mod/staves/Makefile
                 src/lib/mod/divrls/Makefile
                 src/lib/mod/quant/Makefile
                 src/lib/mod/div/Makefile
                 src/lib/mod/beams/Makefile
                 src/lib/mod/dist/Makefile
                 src/lib/mod/accs/Makefile
                 src/lib/mod/octs/Makefile
                 src/lib/mod/parts/Makefile
                 src/lib/mod/marks/Makefile
                 src/lib/mod/meas/Makefile
                 src/lib/mod/dyns/Makefile
                 src/lib/mod/special/Makefile
                 src/lib/mod/common/Makefile
                 src/iface/Makefile
                 src/iface/lisp/Makefile
                 src/iface/pd/Makefile
                 src/lib/api/Makefile
                 src/exe/Makefile
                 src/data/Makefile
                 src/test/Makefile
                 libltdl/Makefile
                 doc/Makefile
])
AC_OUTPUT

# Inform user
echo
echo "-------------------------------------------------------------------------------"
echo "Build = $PACKAGE_STRING"
AS_IF([test -e "$srcdir/stamp-dev"], [echo "  Repository build (extra software required)"])
# AS_IF([test "x$STATICLINK" != "xno"], [echo "  Static linkage enabled"])
echo "Host settings:"
echo "  OS = $host_os"
# echo "  Vender = $host_vender"
echo "  CPU = $host_cpu"
AS_IF([test ! -d "$prefix"], [PREFIX_MSG=" (doesn't exist or isn't a directory--install will create it)"])
echo "Install prefix = $prefix$PREFIX_MSG"
echo "Compiler = $CXX"
AS_IF([test "x$SPTHIRTYTWOBIT" != "xno"], [echo "Force architecture = 32bit"])
AS_IF([test "x$SPSIXTYFOURBIT" != "xno"], [echo "Force architecture = 64bit"])
AS_IF([test "x$SPDEBUGCODE" = "xno"],
      AS_IF([test "x$SPDEBUGOUT" != "xno"], [DEBUG_OUTPUT=" (output enabled, assertions disabled)"], [DEBUG_OUTPUT=" (assertions disabled)"]),
      AS_IF([test "x$SPDEBUGOUT" != "xno"], [DEBUG_OUTPUT=" (output enabled)"], [DEBUG_OUTPUT=""]))
AS_IF([test "x$SPDEBUG" != "xno"], 
      [echo "Debug = enabled$DEBUG_OUTPUT"],
      [echo "Debug = disabled$DEBUG_OUTPUT"])
AS_IF([test -n "$PROFFLAGS0"], [PROFILE_PROGS+=", prof"])
AS_IF([test -n "$PROFFLAGS1"], [PROFILE_PROGS+=", gprof"])
AS_IF([test -n "$PROFFLAGS2"], [PROFILE_PROGS+=", gcov"])
AS_IF([test -n "$PROFILE_PROGS"], [PROFILE_PROGS=`expr substr "$PROFILE_PROGS" 3 100`] [PROFILE_PROGS=" ($PROFILE_PROGS)"])
AS_IF([test -n "$PROFFLAGS"], 
      [echo "Profiling = enabled$PROFILE_PROGS"],
      [echo "Profiling = disabled$PROFILE_PROGS"])
echo "Compile flags = $CXXFLAGS"
AS_IF([test "x$FTERM" = "xyes" -a -n "$CURSES_LIB"], [TERM_ATTR="yes"], [TERM_ATTR="no"])
echo "Terminal attributes (curses library) = $TERM_ATTR"
# AS_IF([test -n "x$lispdir"], [GOTEMACS="$lispdir"], [GOTEMACS="no (emacs not found)"])
# echo "Emacs lisp directory = $GOTEMACS"
# AS_IF([test "x$FDOCS" != "xno"], [BUILD_DOC="yes"], AS_IF([test -n "$MAKEINFOEXE"], [BUILD_DOC="no"], [BUILD_DOC="no (makeinfo not found)"]))
# echo "Build HTML documentation = $BUILD_DOC"
# AS_IF([test "x$FDOCS" = "xyes"],
#       AS_IF([test -n "$CONVERTEXE" -a -n "$TEXIDVIEXE"], [BUILD_PSDOC="yes"], [BUILD_PSDOC="no (convert or texi2dvi not found)"])
#       [echo "Build ps and pdf documentation = $BUILD_PSDOC"])
AS_IF([test -n "$PD_HEAD" -a -n "$PD_EXT" -a "x$PD_WITH" != "xno"], [HAVE_PD="yes"], 
                                            AS_IF([test "x$PD_WITH" = "xno"], [HAVE_PD="no"], 
                                            AS_IF([test -n "$PD_HEAD"], [HAVE_PD="no (currently unsupported on this architecture)"], 
                                                                        [HAVE_PD="no (Pd header file not found)"])))
echo "Build Pd external = $HAVE_PD"
AS_IF([test "x$SPDEBUG" != "xno"], 
      [echo "Boost libraries:"]
      [echo "  $BOOST_SYSTEM_DLIB"]
      [echo "  $BOOST_PROGRAM_OPTIONS_DLIB"]
      [echo "  $BOOST_FILESYSTEM_DLIB"]
      [echo "  $BOOST_THREAD_DLIB"]
      [echo "  $BOOST_IOSTREAMS_DLIB"]
      )
AS_IF([test -n "$BOOSTWARN"], [echo "WARNING: correct Boost libraries not found--FOMUS may not build correctly"])
echo "-------------------------------------------------------------------------------"
