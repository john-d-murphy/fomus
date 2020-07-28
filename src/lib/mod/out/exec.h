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

#ifndef FOMUSMOD_EXEC_H
#define FOMUSMOD_EXEC_H

#include "config.h"
// #define BOOST_IOSTREAMS_USE_DEPRECATED

#include <boost/iostreams/stream.hpp>
#include <string>
#include <vector>

#include "modtypes.h"

//#if defined(HAVE_UNISTD_H) && defined(HAVE_SYS_WAIT_H) && defined(HAVE_FORK)
#if !defined __MINGW32__

// ------------------------------------------------------------------------------------------------------------------------
// UNIX PLATFORMS
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <sys/types.h>
#include <sys/wait.h>

namespace execout {

  class execerr {};

  typedef boost::iostreams::stream<boost::iostreams::file_descriptor_source>
      execout;
  typedef pid_t execpid;
  inline void waituntildone(const execpid pid) {
    int st;
    if (waitpid(pid, &st, 0) < 0)
      throw execerr();
    if ((WIFEXITED(st) && WEXITSTATUS(st)) || WIFSIGNALED(st))
      throw execerr();
  }
  execpid exec(execout* out, const char* path,
               const std::vector<std::string>& args0,
               const struct module_list& args, const char* fn);
  void outputcmd(std::ostream& f, const char* path,
                 const std::vector<std::string>& args0,
                 const struct module_list& args1, const char* fn);
} // namespace execout

#else

// ------------------------------------------------------------------------------------------------------------------------
// MINGW PLATFORM
#define WIN32_LEAN_AND_MEAN
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

namespace execout {

  class execerr {};

  struct pipesrc : public boost::iostreams::source {
    HANDLE pip;

public:
    pipesrc(const HANDLE& pip)
        : boost::iostreams::source(), pip(pip) {
    } // gets the read end of the pipe
    std::streamsize read(char* s, std::streamsize n) {
      DWORD red;
      if (!ReadFile(pip, s, n, &red, NULL))
        return -1;
      // if (!red) return -1; MSDN says other end can write 0 bytes to a pipe
      return red;
    }
  };
  typedef boost::iostreams::stream<pipesrc> execout;
  struct procinfo : public PROCESS_INFORMATION {
    HANDLE choutr, choutw;
    procinfo() : choutr(NULL), choutw(NULL) {
      ZeroMemory((PROCESS_INFORMATION*) this, sizeof(PROCESS_INFORMATION));
    }
    ~procinfo() {
      if (choutr)
        CloseHandle(choutr);
      if (choutw)
        CloseHandle(choutw);
      if (hProcess)
        CloseHandle(hProcess);
      if (hThread)
        CloseHandle(hThread);
    }
  };
  typedef std::auto_ptr<procinfo> execpid;
  inline void waituntildone(execpid& pid) {
    if (WaitForSingleObject(pid->hProcess, INFINITE) != WAIT_OBJECT_0)
      throw execerr();
    DWORD x;
    if (!GetExitCodeProcess(pid->hProcess, &x))
      throw execerr();
    if (x)
      throw execerr();
  }
  execpid exec(execout* out, const char* path,
               const std::vector<std::string>& args0,
               const struct module_list& args, const char* fn);
  void outputcmd(std::ostream& f, const char* path,
                 const std::vector<std::string>& args0,
                 const struct module_list& args1, const char* fn);
} // namespace execout

// ------------------------------------------------------------------------------------------------------------------------
// UNKNOWN PLATFORMS
// #include <boost/iostreams/device/null.hpp>
// #include <string>

// namespace execout {

//   class execerr {};

//   typedef boost::iostreams::stream<boost::iostreams::null_source> execout;
//   typedef pid_t execpid;
//   inline void waituntildone(const execpid pid) {}
//   inline execpid exec(execout* out, const char* path, const
//   std::vector<std::string>& args0, const struct module_list& args, const
//   char* fn) {
//     if (out) {
//       out->exceptions(boost::iostreams::stream<boost::iostreams::null_source>::eofbit
// 		      |
// boost::iostreams::stream<boost::iostreams::null_source>::failbit 		      |
// boost::iostreams::stream<boost::iostreams::null_source>::badbit);
//       out->open(boost::iostreams::null_source());
//     }
//     return 0;
//   }

// }
// ------------------------------------------------------------------------------------------------------------------------

#endif

#endif
