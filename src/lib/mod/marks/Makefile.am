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

# libmarkpos.la
noinst_LTLIBRARIES = libvsmarks.la libmarks.la libmarkevs1.la libmarkevs2.la
pkglib_LTLIBRARIES = markgrps.la grslurs.la

AM_CFLAGS = @FOMUS_CFLAGS@
AM_CXXFLAGS = @FOMUS_CXXFLAGS@

libvsmarks_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSSTATICMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
libvsmarks_la_LDFLAGS = @FOMUS_LDFLAGS@ @BOOST_LDFLAGS@
libmarks_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSSTATICMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
libmarks_la_LDFLAGS = @FOMUS_LDFLAGS@ @BOOST_LDFLAGS@

markgrps_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
markgrps_la_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ -module @BOOST_LDFLAGS@ -avoid-version -shared
grslurs_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
grslurs_la_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ -module @BOOST_LDFLAGS@ -avoid-version -shared
libmarkevs1_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSSTATICMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
libmarkevs1_la_LDFLAGS = @FOMUS_LDFLAGS@ @BOOST_LDFLAGS@
libmarkevs2_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSSTATICMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
libmarkevs2_la_LDFLAGS = @FOMUS_LDFLAGS@ @BOOST_LDFLAGS@

if WIN32_BUILD
markgrps_la_LIBADD = $(top_builddir)/src/lib/libfomus.la
grslurs_la_LIBADD = $(top_builddir)/src/lib/libfomus.la
endif

libvsmarks_la_SOURCES = vsmarks.cc
libmarks_la_SOURCES = marks.cc 
markgrps_la_SOURCES = markgrps.cc 
grslurs_la_SOURCES = grslurs.cc
libmarkevs1_la_SOURCES = markevs1.cc
libmarkevs2_la_SOURCES = markevs2.cc

EXTRA_DIST = vmarks.h smarks.h marks.h markevs1.h markevs2.h
