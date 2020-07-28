// -*- c++ -*-

/*
    Copyright (C) 2009, 2010, 2011  David Psenicka
    This file is part of FOMUS.

    FOMUS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FOMUS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
  ALL COMMENTS IN THIS FILE ARE OLD AND HAVEN'T BEEN UPDATED YET!
*/

#ifndef FOMUS_BASETYPES_H
#define FOMUS_BASETYPES_H

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef double fomus_float;
typedef long fomus_int;
typedef int fomus_bool;
struct fomus_rat {
  fomus_int num, den;
};

typedef void* FOMUS;

#ifdef __cplusplus
}
#endif

#endif
