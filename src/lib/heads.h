// -*- c++ -*-

/*
    Copyright (C) 2009, 2010, 2011, 2012, 2013  David Psenicka
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

#ifndef FOMUS_HEADS_H
#define FOMUS_HEADS_H

#ifndef BUILD_LIBFOMUS
#error "heads.h shouldn't be included"
#endif

#include "config.h"

#if defined(HAVE_FENV_H) && !defined(NDEBUG)
#define _GNU_SOURCE 1
#include <fenv.h>
#endif

#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib> // getenv, exit
#include <new>
/* #include <cerrno> */
//#include <cstring> // strcasecmp
#include <cctype> // used in parse.h

#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <stack>
#include <string>

#ifndef NDEBUG
#include <iostream>
#endif
#include <fstream>
#include <iomanip>
#include <ios>
#include <sstream>

#include <boost/integer.hpp>

#include <boost/ptr_container/ptr_list.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
/* #include <boost/ptr_container/ptr_deque.hpp> */

/* #include <boost/bind.hpp> */
#include <boost/functional.hpp>

#include <boost/rational.hpp>
#include <boost/variant.hpp> // used in parse.h
/* #include <boost/variant.hpp> // used in parse.h */
#include <boost/tuple/tuple.hpp> // used in algext.h

#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <boost/lambda/lambda.hpp>
/* #include <boost/lambda/switch.hpp> */
#include <boost/lambda/algorithm.hpp>
#include <boost/lambda/construct.hpp>
/* #include <boost/lambda/loops.hpp> */

#include <boost/integer/common_factor.hpp>

//#include <boost/iostreams/device/null.hpp>
//#include <boost/scoped_array.hpp>

#ifndef BOOST_SPIRIT_CLASSIC
#include <boost/spirit/core.hpp>
#include <boost/spirit/dynamic.hpp>
#include <boost/spirit/symbols.hpp>
#include <boost/spirit/utility.hpp>
#include <boost/spirit/utility/rule_parser.hpp>
//#include <boost/spirit/error_handling/exceptions.hpp>
#include <boost/spirit/error_handling.hpp>
#include <boost/spirit/iterator/position_iterator.hpp>
#include <boost/spirit/utility/confix.hpp>
namespace boostspirit = boost::spirit;
#else
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_dynamic.hpp>
#include <boost/spirit/include/classic_rule_parser.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_utility.hpp>
//#include <boost/spirit/include/classic_exceptions.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_error_handling.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
namespace boostspirit = boost::spirit::classic;
#endif
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#define BOOST_SPIRIT__NAMESPACE -

#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/utility.hpp>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <boost/iostreams/concepts.hpp> // sink
#include <boost/iostreams/stream.hpp>

#include <boost/shared_ptr.hpp>

#include <boost/static_assert.hpp>

#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "ltdl.h" // type lt_dlhandle

#define CMD_MACRO_STRINGIFY(xxx) #xxx
#define CMD_MACRO(xxx) CMD_MACRO_STRINGIFY(xxx)

#ifndef NDEBUG
#define NONCOPYABLE , boost::noncopyable
#define _NONCOPYABLE :boost::noncopyable
#else
#define NONCOPYABLE
#define _NONCOPYABLE
#endif

#ifndef NDEBUGOUT
#define DBG(xxx) std::cout << xxx
#else
#define DBG(xxx)
#endif

#ifndef NDEBUG
#define TDBG(xxx) std::cout << xxx
#else
#define TDBG(xxx) error TDBG macro
#endif

#define FNAMESPACE fomus

#ifdef BOOST_FILESYSTEM_OLDAPI
#define FS_COMPLETE(xxx, yyy) boost::filesystem::complete(xxx, yyy)
#define FS_FILE_STRING file_string
#define FS_DIRECTORY_STRING directory_string
#define FS_BASENAME(xxx) boost::filesystem::basename(xxx)
#define FS_CHANGE_EXTENSION(xxx, yyy)                                          \
  boost::filesystem::change_extension(xxx, yyy)
#define FS_IS_COMPLETE is_complete
#define FS_EXTENSION(xxx) boost::filesystem::extension(xxx)
#define FS_BRANCH_PATH branch_path
#else
#define FS_COMPLETE(xxx, yyy) boost::filesystem::absolute(xxx, yyy)
#define FS_FILE_STRING string
#define FS_DIRECTORY_STRING string
#define FS_BASENAME(xxx) xxx.stem().string()
#define FS_CHANGE_EXTENSION(xxx, yyy) xxx.replace_extension(yyy)
#define FS_IS_COMPLETE(xxx) is_absolute()
#define FS_EXTENSION(xxx) xxx.extension().string()
#define FS_BRANCH_PATH parent_path
#endif

#endif
