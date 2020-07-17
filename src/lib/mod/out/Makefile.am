#   Copyright (C) 2009, 2010, 2011, 2012, 2013  David Psenicka
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

pkglib_LTLIBRARIES = fmsout.la lilyout.la xmlout.la midiout.la 

AM_CFLAGS = @FOMUS_CFLAGS@
AM_CXXFLAGS = @FOMUS_CXXFLAGS@

fmsout_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD @BOOST_CPPFLAGS@ -DLILYPOND_PATH="@LILYPOND_PATH@" -DLILYPOND_VIEWPATH="@LILYPOND_VIEWPATH@" \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
fmsout_la_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ -module @BOOST_LDFLAGS@ -avoid-version -shared
lilyout_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD @BOOST_CPPFLAGS@ -DLILYPOND_PATH="@LILYPOND_PATH@" -DLILYPOND_VIEWPATH="@LILYPOND_VIEWPATH@" \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
lilyout_la_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ -module @BOOST_LDFLAGS@ -avoid-version -shared
xmlout_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD @BOOST_CPPFLAGS@ -DLILYPOND_PATH="@LILYPOND_PATH@" -DLILYPOND_VIEWPATH="@LILYPOND_VIEWPATH@" \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
xmlout_la_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ -module @BOOST_LDFLAGS@ -avoid-version -shared
midiout_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD @BOOST_CPPFLAGS@ -DLILYPOND_PATH="@LILYPOND_PATH@" -DLILYPOND_VIEWPATH="@LILYPOND_VIEWPATH@" \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
midiout_la_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ -module @BOOST_LDFLAGS@ -avoid-version -shared

fmsout_la_LIBADD = @BOOST_FILESYSTEM_DLIB@ @BOOST_SYSTEM_DLIB@
fmsout_la_SOURCES = fmsout.cc 
midiout_la_LIBADD = @BOOST_FILESYSTEM_DLIB@ @BOOST_SYSTEM_DLIB@
midiout_la_SOURCES = midiout.cc 
xmlout_la_LIBADD = @BOOST_FILESYSTEM_DLIB@ @BOOST_SYSTEM_DLIB@
xmlout_la_SOURCES = xmlout.cc
lilyout_la_LIBADD = @BOOST_FILESYSTEM_DLIB@ @BOOST_IOSTREAMS_DLIB@ @BOOST_SYSTEM_DLIB@
lilyout_la_SOURCES = exec.cc lilyout.cc exec.h

if WIN32_BUILD
fmsout_la_LIBADD += $(top_builddir)/src/lib/libfomus.la
lilyout_la_LIBADD += $(top_builddir)/src/lib/libfomus.la
xmlout_la_LIBADD += $(top_builddir)/src/lib/libfomus.la
midiout_la_LIBADD += $(top_builddir)/src/lib/libfomus.la
endif
