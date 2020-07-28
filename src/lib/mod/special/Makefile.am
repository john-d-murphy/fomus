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

pkglib_LTLIBRARIES = tpose.la harms.la percchs.la trems.la
noinst_LTLIBRARIES = libpnotes.la 

AM_CFLAGS = @FOMUS_CFLAGS@
AM_CXXFLAGS = @FOMUS_CXXFLAGS@

tpose_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
tpose_la_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ -module @BOOST_LDFLAGS@ -avoid-version -shared
harms_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
harms_la_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ -module @BOOST_LDFLAGS@ -avoid-version -shared
percchs_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
percchs_la_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ -module @BOOST_LDFLAGS@ -avoid-version -shared
trems_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
trems_la_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ -module @BOOST_LDFLAGS@ -avoid-version -shared

libpnotes_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSSTATICMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
libpnotes_la_LDFLAGS = @FOMUS_LDFLAGS@ @BOOST_LDFLAGS@

if WIN32_BUILD
tpose_la_LIBADD = $(top_builddir)/src/lib/libfomus.la
harms_la_LIBADD = $(top_builddir)/src/lib/libfomus.la
percchs_la_LIBADD = $(top_builddir)/src/lib/libfomus.la
trems_la_LIBADD = $(top_builddir)/src/lib/libfomus.la
endif

harms_la_SOURCES = harms.cc
percchs_la_SOURCES = percchs.cc
tpose_la_SOURCES = tpose.cc
libpnotes_la_SOURCES = pnotes.cc
trems_la_SOURCES = trems.cc

EXTRA_DIST = pnotes.h
