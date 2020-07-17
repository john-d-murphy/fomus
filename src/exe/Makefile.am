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

bin_PROGRAMS = fomus

AM_CPPFLAGS = @FOMUS_CPPFLAGS@ @BOOST_CPPFLAGS@ -I$(top_srcdir)/src/lib/api
AM_CFLAGS = @FOMUS_CFLAGSX@
AM_CXXFLAGS = @FOMUS_CXXFLAGSX@
# + @FOMUS_LDFLAGSX@ 
AM_LDFLAGS = @BOOST_LDFLAGS@ -rpath $(libdir)

fomus_LDADD = $(top_builddir)/src/lib/libfomus.la @CURSES_LIB@ @BOOST_PROGRAM_OPTIONS_DLIB@
fomus_SOURCES = main.cc

installcheck-local:
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test $(bindir)/fomus --version >/dev/null 2>&1
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test $(bindir)/fomus --list-modules >/dev/null 2>&1
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test $(bindir)/fomus --list-settings --filter uselevel=3 >/dev/null 2>&1
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test $(bindir)/fomus --list-marks >/dev/null 2>&1
