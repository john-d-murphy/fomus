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

if MINGW_BUILD
presetsdir = $(datadir)/presets
else
presetsdir = $(pkgdatadir)/presets
endif

#INSTALL
if MINGW_BUILD
dist_data_DATA = fomus.conf
else
dist_pkgdata_DATA = fomus.conf
endif
dist_presets_DATA = draft.fpr quality.fpr tonal.fpr nottonal.fpr common.fpr contemp.fpr
nodist_lisp_DATA = fomus.el

# STUFF
if MINGW_BUILD
fomus.el: $(srcdir)/fomus.el.in
	BD=`echo "$(bindir)" | @SED@ s/:/\\\\\\\\:/g | @SED@ s/\"/\\\\\\\\\\\\\\\\\"/g | @SED@ s-/-\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\-g`; \
@SED@ "s:_FOMUS_BIN_:(concat (getenv \"WINDIR\") \"$$BD\\\\\\\\fomus\"):" $(srcdir)/fomus.el.in > fomus.el
else
fomus.el: $(srcdir)/fomus.el.in
	BD=`echo "$(bindir)" | @SED@ s/:/\\\\\\\\:/g | @SED@ s/\"/\\\\\\\\\\\\\\\\\"/g`; @SED@ "s:_FOMUS_BIN_:\"$$BD/fomus\":" $(srcdir)/fomus.el.in > fomus.el
endif

CLEANFILES = fomus.el

EXTRA_DIST = fomus.el.in

uninstall-local:
	rm -rf $(DESTDIR)$(presetsdir)
