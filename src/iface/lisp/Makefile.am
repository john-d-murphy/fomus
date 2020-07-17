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

# STUFF
if MINGW_BUILD
fomus.lisp: $(srcdir)/fomus.lisp.in
	@SED@ "s%_FOMUS_LIB_%\"libfomus\"%" $(srcdir)/fomus.lisp.in > fomus.lisp
fomusmod.lisp: $(srcdir)/fomusmod.lisp.in
	@SED@ "s%_FOMUS_LIB_%\"libfomus\"%" $(srcdir)/fomusmod.lisp.in > fomusmod.lisp
else
fomus.lisp: $(srcdir)/fomus.lisp.in
	LD=`echo "$(libdir)" | @SED@ s/:/\\\\\\\\:/g | @SED@ s/\"/\\\\\\\\\\\\\\\\\"/g`; @SED@ "s:_FOMUS_LIB_:\"$$LD/libfomus\":" $(srcdir)/fomus.lisp.in > fomus.lisp
fomusmod.lisp: $(srcdir)/fomusmod.lisp.in
	LD=`echo "$(libdir)" | @SED@ s/:/\\\\\\\\:/g | @SED@ s/\"/\\\\\\\\\\\\\\\\\"/g`; @SED@ "s:_FOMUS_LIB_:\"$$LD/libfomus\":" $(srcdir)/fomusmod.lisp.in > fomusmod.lisp
endif

CLEANFILES = fomus.lisp fomusmod.lisp

if ISDEVEL
install-data-hook:
	rm -f $(top_builddir)/doc/stamp-lisp

$(srcdir)/fomus.lisp.in: $(srcdir)/fomuscffi.i $(top_srcdir)/src/lib/api/fomusapi.h $(top_srcdir)/src/lib/api/basetypes.h $(top_srcdir)/src/lib/api/export.h $(srcdir)/fomus1.lisp.in $(srcdir)/fomus2.lisp.in 
	swig -cffi -I$(top_srcdir)/src/lib/api -includeall -noswig-lisp -o fomuscffi.lisp $(srcdir)/fomuscffi.i
	cat $(srcdir)/fomus1.lisp.in fomuscffi.lisp $(srcdir)/fomus2.lisp.in > $(srcdir)/fomus.lisp.in

SED1 = 's/"module.h"/"module_cffi.h"/'
SED2 = 's/"modtypes.h"/"modtypes_cffi.h"/'
SED3 = 's/"basetypes.h"/"basetypes_cffi.h"/'
SED4 = 's/struct  *\([a-z_][a-z_]*\)/struct struct_\1/'
SED5 = 's/enum  *\([a-z_][a-z_]*\)/enum enum_\1/'
SED6 = 's/ iface/ /'

%_cffi.h: $(top_srcdir)/src/lib/api/%.h
	cat $< | @SED@ $(SED1) | @SED@ $(SED2) | @SED@ $(SED3) | @SED@ $(SED4) | @SED@ $(SED5) > $@

%_engcffi.h: $(top_srcdir)/src/lib/mod/eng/%.h
	cat $< | @SED@ $(SED1) | @SED@ $(SED2) | @SED@ $(SED3) | @SED@ $(SED4) | @SED@ $(SED5) \
| @SED@ "s/\(#define  *\)\([A-Z_][A-Z_]*_INTERFACEID  *[0-9][0-9]*\)/\1$(*F)_\2/" | @SED@ $(SED6) > $@

%_distcffi.h: $(top_srcdir)/src/lib/mod/dist/%.h
	cat $< | @SED@ $(SED1) | @SED@ $(SED2) | @SED@ $(SED3) | @SED@ $(SED4) | @SED@ $(SED5) \
| @SED@ "s/\(#define  *\)\([A-Z_][A-Z_]*_INTERFACEID  *[0-9][0-9]*\)/\1$(*F)_\2/" | @SED@ $(SED6) > $@

%_divrlscffi.h: $(top_srcdir)/src/lib/mod/divrls/%.h
	cat $< | @SED@ $(SED1) | @SED@ $(SED2) | @SED@ $(SED3) | @SED@ $(SED4) | @SED@ $(SED5) \
| @SED@ "s/\(#define  *\)\([A-Z_][A-Z_]*_INTERFACEID  *[0-9][0-9]*\)/\1$(*F)_\2/" | @SED@ $(SED6) > $@

IFACES = ifacesearch_engcffi.h ifacedumb_engcffi.h ifacedivsearch_engcffi.h ifacedist_distcffi.h ifacedivrules_divrlscffi.h

$(srcdir)/fomusmod.lisp.in: $(srcdir)/modcffi.i module_cffi.h modutil_cffi.h modtypes_cffi.h basetypes_cffi.h $(IFACES) $(top_srcdir)/src/lib/api/export.h $(srcdir)/mod1.lisp.in $(srcdir)/mod2.lisp.in 
	swig -cffi -DBUILD_LIBFOMUS -DDONTHAVE_VISIBILITY -DSWIG_DEFS -I$(top_srcdir)/src/lib/api -includeall -noswig-lisp -o modcffi.lisp $(srcdir)/modcffi.i
	cat modcffi.lisp | tr '[:upper:]' '[:lower:]' \
| @SED@ ':rep; s/enum_\([a-z0-9-][a-z0-9-]*\)_\([a-z0-9_][a-z0-9_]*\)/enum_\1-\2/; t rep' \
| @SED@ ':rep; s/struct_\([a-z0-9-][a-z0-9-]*\)_\([a-z0-9_][a-z0-9_]*\)/struct_\1-\2/; t rep' \
| @SED@ 's/struct_//' | @SED@ 's/enum_//' \
| @SED@ ':rep; s/\(("[a-z0-9_][a-z0-9_]*"  *[a-z0-9-][a-z0-9-]*\)_\([a-z0-9_][a-z0-9_]*)\)/\1-\2/; t rep' \
| @SED@ ':rep; s/:\([a-z0-9-][a-z0-9-]*\)_\([a-z0-9_][a-z0-9_]*\)/:\1-\2/; t rep' \
| @SED@ ':rep; s/\(cl:defconstant [a-zA-Z0-9_][a-zA-Z0-9_]*\)_\([a-zA-Z0-9_][a-zA-Z0-9_]*\)/\1-\2/; t rep' \
> mod3.lisp.in
	cat mod3.lisp.in | grep '\(cffi\|cl\):' \
| @SED@ 's/(cffi:defcenum \([a-z0-9-][a-z0-9-]*\).*/   #:\1/' \
| @SED@ 's/(cffi:defcstruct \([a-z0-9-][a-z0-9-]*\).*/   #:\1/' \
| @SED@ 's/(cffi:defcunion \([a-z0-9-][a-z0-9-]*\).*/   #:\1/' \
| @SED@ 's/(cffi:defcfun ("[a-z0-9_][a-z0-9_]*"  *\([a-z0-9-][a-z0-9-]*\).*/   #:\1/' \
| @SED@ 's/(cffi:defcvar ("[a-z0-9_][a-z0-9_]*"  *\([a-z0-9-][a-z0-9-]*\).*/   #:\1/' \
| @SED@ 's/(cl:defconstant \([a-z0-9-][a-z0-9-]*\).*/   #:\1/' \
> mod4.lisp.in
	cat $(srcdir)/mod1.lisp.in mod4.lisp.in $(srcdir)/mod2.lisp.in mod3.lisp.in > $(srcdir)/fomusmod.lisp.in

CLEANFILES += fomuscffi.lisp $(srcdir)/fomus.lisp.in modcffi.lisp $(srcdir)/fomusmod.lisp.in module_cffi.h modutil_cffi.h modtypes_cffi.h basetypes_cffi.h \
              mod3.lisp.in mod4.lisp.in $(IFACES)
endif

# EXTRA_DIST = fomus1.lisp.in fomus2.lisp.in fomuscffi.i fomus.lisp.in
EXTRA_DIST = fomus.lisp.in fomusmod.lisp.in

# INSTALL
dist_pkgdata_DATA = $(srcdir)/fomus.asd $(srcdir)/fomusmod.asd
pkgdata_DATA = fomus.lisp fomusmod.lisp
