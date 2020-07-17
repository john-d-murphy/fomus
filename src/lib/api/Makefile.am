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

# ***** ALSO INSERT THESE INTO HEADTEST.C IN TEST DIRECTORY *****
pkginclude_HEADERS = basetypes.h fomusapi.h infoapi.h module.h $(srcdir)/modtypes.h modutil.h export.h

include_HEADERS = fomus.h

if ISDEVEL
$(srcdir)/modtypes.h: $(top_srcdir)/src/lib/userstrs.cc $(top_srcdir)/src/lib/marks.cc $(srcdir)/modtypes1.h.in $(srcdir)/modtypes2.h.in $(srcdir)/modtypes3.h.in
	trap 'rm -rf $(srcdir)/lock2' 1 2 13 15; \
if `mkdir $(srcdir)/lock2 2>/dev/null`; then \
  cat $(srcdir)/modtypes1.h.in > modtypes.h.in; \
  grep 'type{[a-z0-9_]*}' $(top_srcdir)/src/lib/userstrs.cc | @SED@ 's/^\(.*\)type{\([a-z0-9_]*\)}\(.*\)$$/\2/' | xargs -I{} echo '    module_{},' >> modtypes.h.in; \
  cat $(srcdir)/modtypes2.h.in >> modtypes.h.in; \
  grep 'enum{[a-z0-9_]*}' $(top_srcdir)/src/lib/marks.cc | @SED@ 's/^\(.*\)enum{\([a-z0-9_]*\)}\(.*\)$$/\2/' | xargs -I{} echo '    mark_{},' >> modtypes.h.in; \
  cat modtypes.h.in $(srcdir)/modtypes3.h.in > $(srcdir)/modtypes.h; \
  rm -rf $(srcdir)/lock2; \
else \
  while `test -d $(srcdir)/lock2`; do sleep 1; done; \
fi

CLEANFILES = modtypes.h.in $(srcdir)/modtypes.h

all-local: $(srcdir)/modtypes.h

clean-local:
	-rm -rf lock
endif

# DISTRIBUTION (modenums.h is NOT in SVN!)
# EXTRA_DIST = modenums.h modtypes.h

