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

SUBDIRS = api 
if WIN32_BUILD
SUBDIRS += .
endif
SUBDIRS += mod

if WIN32_BUILD
# mod/marks/libmarkpos.la
BUILT_SOURCES = api/modtypes.h \
                mod/eng/libdumb.la mod/voices/libprune.la mod/parts/libmparts.la mod/marks/libvsmarks.la mod/marks/libmarks.la mod/staves/librstaves.la \
                mod/meas/libmeas.la mod/special/libpnotes.la mod/marks/libmarkevs1.la mod/marks/libmarkevs2.la mod/div/libgrdiv.la

api/modtypes.h:
	(cd api; $(MAKE) $(AM_MAKEFLAGS) modtypes.h)

mod/eng/libdumb.la:
	(cd mod/eng; $(MAKE) $(AM_MAKEFLAGS) libdumb.la)

mod/voices/libprune.la:
	(cd mod/voices; $(MAKE) $(AM_MAKEFLAGS) libprune.la)

mod/parts/libmparts.la:
	(cd mod/parts; $(MAKE) $(AM_MAKEFLAGS) libmparts.la)

mod/marks/libvsmarks.la:
	(cd mod/marks; $(MAKE) $(AM_MAKEFLAGS) libvsmarks.la)

mod/marks/libmarks.la:
	(cd mod/marks; $(MAKE) $(AM_MAKEFLAGS) libmarks.la)

# mod/marks/libmarkpos.la:
# 	(cd mod/marks; $(MAKE) $(AM_MAKEFLAGS) libmarkpos.la)

mod/staves/librstaves.la:
	(cd mod/staves; $(MAKE) $(AM_MAKEFLAGS) librstaves.la)

mod/meas/libmeas.la:
	(cd mod/meas; $(MAKE) $(AM_MAKEFLAGS) libmeas.la)

mod/special/libpnotes.la:
	(cd mod/special; $(MAKE) $(AM_MAKEFLAGS) libpnotes.la)

mod/marks/libmarkevs1.la:
	(cd mod/marks; $(MAKE) $(AM_MAKEFLAGS) libmarkevs1.la)

mod/marks/libmarkevs2.la:
	(cd mod/marks; $(MAKE) $(AM_MAKEFLAGS) libmarkevs2.la)

mod/div/libgrdiv.la:
	(cd mod/div; $(MAKE) $(AM_MAKEFLAGS) libgrdiv.la)
endif

lib_LTLIBRARIES = libfomus.la

if MINGW_BUILD
AM_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_LIBFOMUS @BOOST_CPPFLAGS@ @LTDLINCL@ -DMODULE_PATH="$(subst /,\\\\,$(pkglibdir))" -DCONFFILE_PATH="$(subst /,\\\\,$(datadir))\\\\fomus.conf" \
              -DPRESETS_PATH="$(subst /,\\\\,$(datadir))\\\\presets" -I$(srcdir)/api -I$(top_srcdir)/libltdl -I$(srcdir)/mod/eng -I$(srcdir)/mod
else
AM_CPPFLAGS = @FOMUS_CPPFLAGS@ -DBUILD_LIBFOMUS @BOOST_CPPFLAGS@ @LTDLINCL@ -DMODULE_PATH="$(pkglibdir)" -DCONFFILE_PATH="$(pkgdatadir)/fomus.conf" \
              -DPRESETS_PATH="$(pkgdatadir)/presets" -I$(srcdir)/api -I$(top_srcdir)/libltdl -I$(srcdir)/mod/eng -I$(srcdir)/mod
endif
AM_CFLAGS = @FOMUS_CFLAGS@
AM_CXXFLAGS = @FOMUS_CXXFLAGS@
AM_LDFLAGS = @FOMUS_LDFLAGS@ @WIN32_LDFLAGS@ @BOOST_LDFLAGS@ -avoid-version -export-dynamic

# mod/marks/libmarkpos.la
libfomus_la_LIBADD = mod/eng/libdumb.la mod/voices/libprune.la mod/parts/libmparts.la \
                     mod/marks/libvsmarks.la mod/marks/libmarks.la mod/staves/librstaves.la \
                     mod/meas/libmeas.la mod/special/libpnotes.la mod/marks/libmarkevs1.la mod/marks/libmarkevs2.la \
                     mod/div/libgrdiv.la \
                     @LIBLTDL@ @BOOST_SYSTEM_DLIB@ @BOOST_FILESYSTEM_DLIB@ @BOOST_THREAD_DLIB@
# Don't add fomus.h here
libfomus_la_SOURCES = api.cc \
                      heads.h \
                      error.cc error.h \
                      parse.cc parse.h \
                      numbers.cc numbers.h \
                      classes.h classes.cc \
                      vars.cc vars.h \
                      data.cc data.h \
                      mods.cc mods.h \
                      algext.h \
                      info.cc \
                      userstrs.cc userstrs.h \
                      instrs.cc instrs.h \
                      marks.cc marks.h \
                      parseins.h \
                      moremath.h \
                      schedr.cc schedr.h \
                      intern.cc intern.h \
                      util.cc \
                      $(srcdir)/paractstrs.cc

if ISDEVEL
$(srcdir)/paractstrs.cc: $(srcdir)/api/fomusapi.h $(srcdir)/paractstrs1.cc.in $(srcdir)/paractstrs2.cc.in $(srcdir)/paractstrs3.cc.in
	cat $(srcdir)/paractstrs1.cc.in > paractstrs.cc.in
	grep 'fomus\_par\_[a-z_]*' $(srcdir)/api/fomusapi.h | @SED@ 's/^\(.*\)\(fomus\_par\_[a-z_]*\)\(.*\)$$/\2/' | xargs -I{} echo '"{}",' | @SED@ 's/fomus_par_//' | @SED@ 's/"n",//' >> paractstrs.cc.in
	cat $(srcdir)/paractstrs2.cc.in >> paractstrs.cc.in
	grep 'fomus\_act\_[a-z_]*' $(srcdir)/api/fomusapi.h | @SED@ 's/^\(.*\)\(fomus\_act\_[a-z_]*\)\(.*\)$$/\2/' | xargs -I{} echo '"{}",' | @SED@ 's/fomus_act_//' | @SED@ 's/"n",//' >> paractstrs.cc.in
	cat paractstrs.cc.in $(srcdir)/paractstrs3.cc.in > $(srcdir)/paractstrs.cc

all-local: $(srcdir)/paractstrs.cc

CLEANFILES = paractstrs.cc.in $(srcdir)/paractstrs.cc
endif

# DISTRIBUTION (paractstrs.cc is NOT in SVN!)
# EXTRA_DIST = paractstrs1.cc.in paractstrs2.cc.in paractstrs3.cc.in

# heads.h.gch: heads.h
# 	$(CXXCOMPILE) -o $@ $<
