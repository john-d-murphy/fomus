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

noinst_LTLIBRARIES = libmeas.la

AM_CFLAGS = @FOMUS_CFLAGS@
AM_CXXFLAGS = @FOMUS_CXXFLAGS@

libmeas_la_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSSTATICMOD @BOOST_CPPFLAGS@ \
              -I$(top_srcdir) -I$(top_srcdir)/src/lib/api \
              -I$(top_srcdir)/src/lib/mod/common -I$(top_srcdir)/src/lib/mod/dist -I$(top_srcdir)/src/lib/mod/divrls -I$(top_srcdir)/src/lib/mod/eng
libmeas_la_LDFLAGS = @FOMUS_LDFLAGS@ @BOOST_LDFLAGS@

libmeas_la_SOURCES = meas.cc meas.h
