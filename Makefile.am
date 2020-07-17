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

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libltdl src doc

# cleanup the empty directories
uninstall-local:
	rm -rf $(DESTDIR)$(pkgincludedir)
	rm -rf $(DESTDIR)$(pkglibdir)
	rm -rf $(DESTDIR)$(docdir)
	rm -rf $(DESTDIR)$(pkgdatadir)



# to build boost in MinGW:

# download bjam.exe and put in /usr/local/bin
# ./bootstrap.sh --with-bjam=/usr/local/bin/bjam.exe --prefix=/usr/local --with-toolset=gcc
# bjam --build-type=complete --prefix=/usr/local --variant=release --layout=tagged --buildid=fomus --link=shared --threading=multi --runtime-link=shared --with-system --with-program_options --with-filesystem --with-iostreams --with-thread install
# go to /usr/local/lib and put .dll files in /bin removing lib prefixes
# remove everything except -mt- libraries
# move boost headers directory from include/boost-?.?? directory into includes directory

# also, prepare libpd.dll.a import library

# valgrind --suppressions=/home/david/local/projects/fomus/valgrind.supp fomus ... 2>out



# to build boost in OS X:

# ./bootstrap.sh
# ./bjam --variant=release --layout=versioned --link=shared --threading=multi --runtime-link=shared architecture=x86 address-model=32_64 --with-system --with-program_options --with-filesystem --with-iostreams --with-thread cflags='-mmacosx-version-min=10.4' cxxflags='-mmacosx-version-min=10.4' linkflags='-mmacosx-version-min=10.4' install
