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

#ifndef FOMUS_EXPORT_H
#define FOMUS_EXPORT_H

#if defined BUILD_LIBFOMUS || defined BUILD_FOMUSSTATICMOD
#define NO_INLINE
#endif

#if defined FOMUS_TYPESONLY

#define LIBFOMUS_HIDE
#define FOMUSMOD_HIDE

#elif defined _WIN32 || defined _WIN64 || defined __CYGWIN__ ||                \
    defined __MINGW32__ || __MINGW64__

// --------------------------------------------------------------------------------
// windows

#if defined BUILD_FOMUSSTATICMOD

#ifndef BUILD_FOMUSMOD
#define BUILD_FOMUSMOD
#endif

#define LIBFOMUS_EXPORT
#define FOMUSMOD_EXPORT

#else

#if defined DLL_EXPORT

#if defined BUILD_LIBFOMUS
#define LIBFOMUS_EXPORT __declspec(dllexport)
#else
#define LIBFOMUS_EXPORT __declspec(dllimport)
#endif

#else

#if defined BUILD_LIBFOMUS && !defined DONTHAVE_VISIBILITY
#define LIBFOMUS_EXPORT __attribute__((__visibility__("default")))
#else
#define LIBFOMUS_EXPORT
#endif

#endif

#if defined DLL_EXPORT

#if !defined BUILD_LIBFOMUS
#define FOMUSMOD_EXPORT __declspec(dllexport)
#else
#define FOMUSMOD_EXPORT
#define FOMUSMOD_HIDE
#endif

#else

#if !defined BUILD_LIBFOMUS
#if !defined DONTHAVE_VISIBILITY
#define FOMUSMOD_EXPORT __attribute__((__visibility__("default")))
#else
#define FOMUSMOD_EXPORT
#endif
#else
#define FOMUSMOD_EXPORT
#define FOMUSMOD_HIDE
#endif

#endif

#endif

#else

// --------------------------------------------------------------------------------
// unix

#if defined BUILD_LIBFOMUS && !defined DONTHAVE_VISIBILITY
#define LIBFOMUS_EXPORT __attribute__((__visibility__("default")))
#else
#define LIBFOMUS_EXPORT
#endif

#if !defined BUILD_LIBFOMUS
#if !defined DONTHAVE_VISIBILITY
#define FOMUSMOD_EXPORT __attribute__((__visibility__("default")))
#else
#define FOMUSMOD_EXPORT
#endif
#else
#define FOMUSMOD_EXPORT
#define FOMUSMOD_HIDE
#endif

#endif

#endif
