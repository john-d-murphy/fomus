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

# pdextdir = $(libdir)/pd/externs

# trick automake into thinking it's allowed to do this
pdlibdir = @pkgdatadir@
lib_LTLIBRARIES = fomus.la

AM_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_FOMUSMOD -I$(top_srcdir) -I$(top_srcdir)/src/lib/api @PD_CPPFLAGS@
AM_CFLAGS = @FOMUS_CFLAGS@
AM_CXXFLAGS = @FOMUS_CXXFLAGS@
AM_LDFLAGS = @WIN32_LDFLAGS@ -module -avoid-version -shared -shrext .@PD_EXT@
fomus_la_LIBADD = $(top_builddir)/src/lib/libfomus.la

if WIN32_BUILD
AM_CPPFLAGS += -DMSW
fomus_la_LIBADD += -lpd
endif

fomus_la_SOURCES = pdin.cc

dist_pkgdata_DATA = fomus.pd

if WIN32_BUILD
install-data-hook:
	rm -f "$(DESTDIR)$(libdir)/fomus.a" "$(DESTDIR)$(libdir)/fomus.dll.a" "$(DESTDIR)$(libdir)/fomus.la"
	mv -f "$(DESTDIR)$(libdir)/fomus.@PD_EXT@" "$(DESTDIR)$(pkgdatadir)/"
else
install-data-hook:
	rm -f "$(DESTDIR)$(libdir)/fomus.a" "$(DESTDIR)$(libdir)/fomus.la"
	mv -f "$(DESTDIR)$(libdir)/fomus.@PD_EXT@" "$(DESTDIR)$(pkgdatadir)/"
endif

uninstall-hook:
	rm -f "$(DESTDIR)$(pkgdatadir)/fomus.@PD_EXT@"

# SAVE ME!
# HOWTO MAKE IMPORT LIBRARY for MINGW:
#   gendef pd.dll (OR pexports pd.dll >pd.def)
#   dlltool -d pd.def -l libpd.dll.a

# old way:
#   mv pd.dll /mingw/mingw/bin/ (might be /mingw/bin/)
#   mv libpd.dll.a /mingw/mingw/lib/
#   mv m_pd.h /mingw/mingw/include/

# new way:
#   mv pd.dll /usr/local/bin/
#   mv libpd.dll.a /usr/local/lib/
#   mv m_pd.h /usr/local/include/
