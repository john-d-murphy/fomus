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


fomus.scm fomus_wrap.c.in: fomuschicken.i $(top_srcdir)/src/lib/api/fomusapi.h $(top_srcdir)/src/lib/api/basetypes.h
	$(SWIG) -chicken -I$(top_srcdir)/src/lib/api -includeall -nounit -o fomus_wrap.c.in fomuschicken.i

fomus_iface.scm: fomushead.scm.in fomus.scm fomusiface.scm.in
	cat fomushead.scm.in fomus.scm fomusiface.scm.in > fomus_iface.scm

fomus_wrap.c: fomus_wrap.c.in
	echo '#include "$(top_srcdir)/src/lib/api/fomusapi.h"' > fomus_wrap.c
	cat fomus_wrap.c.in >> fomus_wrap.c

# must happen in this order
fomus.so fomus.o: fomus_iface.scm fomus_wrap.c $(top_srcdir)/src/lib/api/fomusapi.h $(top_srcdir)/src/lib/api/basetypes.h
	csc -sv fomus_iface.scm fomus_wrap.c -L/home/david/local/lib -lfomus -o fomus.so
	csc -cv fomus_iface.scm fomus_wrap.c -L/home/david/local/lib -lfomus -unit fomus -o fomus.o

CLEANFILES = *.scm *.c *.o *.a *.so *.c.in
