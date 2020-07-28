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

#include "exec.h"

#if !defined __MINGW32__

// ------------------------------------------------------------------------------------------------------------------------
// UNIX PLATFORMS
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <vector>

#ifndef DEV_NULL_EXISTS
#error "file /dev/null must exist!"
#endif

namespace execout {

  void outputcmd(std::ostream& f, const char* path,
                 const std::vector<std::string>& args0,
                 const struct module_list& args1, const char* fn) {
    f << path;
    for (module_value *i(args1.vals), *ie(args1.vals + args1.n); i < ie; ++i) {
      assert(i->type == module_string);
      f << ' ' << i->val.s;
    }
    for (std::vector<std::string>::const_iterator i(args0.begin());
         i != args0.end(); ++i) {
      f << ' ' << *i;
    }
    f << ' ' << fn;
  }

  execpid exec(execout* out, const char* path,
               const std::vector<std::string>& args0,
               const struct module_list& args1, const char* fn) {
    int tmpstdin[2];
    if (pipe(tmpstdin) < 0)
      throw execerr();
    pid_t pid = fork();
    switch (pid) {
    case 0: // CHILD PROCESS
    {
      std::vector<const char*> args;
      args.push_back(path);
      for (module_value *i(args1.vals), *ie(args1.vals + args1.n); i < ie;
           ++i) {
        assert(i->type == module_string);
        args.push_back(i->val.s);
      }
      for (std::vector<std::string>::const_iterator i(args0.begin());
           i != args0.end(); ++i) {
        args.push_back(i->c_str());
      }
      args.push_back(fn);
      args.push_back(0);
      if (close(tmpstdin[0]) < 0)
        exit(EXIT_FAILURE);
      if (dup2(tmpstdin[1], STDOUT_FILENO) < 0)
        exit(EXIT_FAILURE);
      if (dup2(tmpstdin[1], STDERR_FILENO) < 0)
        exit(EXIT_FAILURE);
      if (close(tmpstdin[1]) < 0)
        exit(EXIT_FAILURE);
      execvp(path, (char* const*) &args[0]); // searches for file in $PATH
      exit(EXIT_FAILURE);
    }
    case -1:
      throw execerr();
    }
    if (close(tmpstdin[1]) < 0)
      throw execerr(); // PARENT PROCESS
    if (out) {
#ifdef BOOST_IOSTREAMS_OLDAPI
      out->open(boost::iostreams::file_descriptor_source(
          tmpstdin[0], true)); // open for reading
#else
      out->open(boost::iostreams::file_descriptor_source(
          tmpstdin[0], boost::iostreams::close_handle)); // open for reading
#endif
    } else {
      int nl = open("/dev/null", O_WRONLY);
      if (nl < 0)
        throw execerr();
      if (dup2(nl, tmpstdin[0]) < 0)
        throw execerr();
      if (close(nl) < 0)
        throw execerr();
    }
    return pid;
  }

} // namespace execout

#else

// ------------------------------------------------------------------------------------------------------------------------
// MINGW PLATFORM
#include <vector>
// #include <cstdlib>
#include <sstream>
// #include <list>
// #include <process.h>
// #include <io.h>
// #include <fcntl.h>
#include <cstring>
#include <iostream>
// #include <share.h>
// #include <boost/filesystem.hpp>
#include <boost/scoped_array.hpp>

namespace execout {

  // DOS delimiters: space, comma, semicolon, equal sign, tab, or carriage
  // return
  inline std::string dosify(const std::string& str) {
    return (str.find_first_of(" ,;=\t\r") == std::string::npos)
               ? str
               : '"' + str + '"';
  }

  void outputcmd(std::ostream& f, const char* path,
                 const std::vector<std::string>& args0,
                 const struct module_list& args1, const char* fn) {
    f << dosify(path);
    for (module_value *i(args1.vals), *ie(args1.vals + args1.n); i < ie; ++i) {
      assert(i->type == module_string);
      f << ' ' << dosify(i->val.s);
    }
    for (std::vector<std::string>::const_iterator i(args0.begin());
         i != args0.end(); ++i) {
      f << ' ' << dosify(*i);
    }
    f << ' ' << dosify(fn);
  }

  execpid exec(execout* out, const char* path,
               const std::vector<std::string>& args0,
               const struct module_list& args1, const char* fn) {
    std::ostringstream ou;
    ou << "\"" << path << '"';
    for (module_value *i(args1.vals), *ie(args1.vals + args1.n); i < ie; ++i) {
      assert(i->type == module_string);
      ou << " \"" << i->val.s << '"';
    }
    for (std::vector<std::string>::const_iterator i(args0.begin());
         i != args0.end(); ++i) {
      ou << " \"" << *i << '"';
    }
    ou << " \"" << fn << '"';
    execpid pid(new procinfo); // NULLed procinfo, also stores handles
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(si);
    if (out) { // assuming if out exists then we don't have a window
      SECURITY_ATTRIBUTES sa;
      sa.nLength = sizeof(SECURITY_ATTRIBUTES);
      sa.bInheritHandle = TRUE;
      sa.lpSecurityDescriptor = NULL;
      if (!CreatePipe(&pid->choutr, &pid->choutw, &sa, 0))
        throw execerr();
      if (!SetHandleInformation(pid->choutr, HANDLE_FLAG_INHERIT, 0))
        throw execerr();
      si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
      // si.hStdInput = NULL;
      si.hStdOutput = pid->choutw;
      si.hStdError = pid->choutw;
      si.wShowWindow = SW_HIDE;
    }
    boost::scoped_array<char> cm(new char[ou.str().length() + 1]);
    if (!CreateProcess(NULL, strcpy(cm.get(), ou.str().c_str()), NULL, NULL,
                       TRUE, 0, NULL, NULL, &si, pid.get()))
      throw execerr();
    if (out) {
      BOOL z = CloseHandle(pid->choutw);
      pid->choutw = NULL;
      if (!z)
        throw execerr();
    }
    if (out)
      out->open(pipesrc(pid->choutr));
    return pid;
  }

} // namespace execout

#endif
