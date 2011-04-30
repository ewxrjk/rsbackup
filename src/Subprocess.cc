// Copyright © 2011 Richard Kettlewell.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <config.h>
#include "Subprocess.h"
#include "Errors.h"
#include "Command.h"
#include "Defaults.h"
#include "IO.h"
#include <csignal>
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

Subprocess::Subprocess(): pid(-1) {
}

Subprocess::Subprocess(const std::vector<std::string> &cmd_):
  pid(-1),
  cmd(cmd_) {
}

Subprocess::~Subprocess() {
  if(pid >= 0) {
    kill(pid, SIGKILL);
    try { wait(false); } catch(...) {}
  }
}

void Subprocess::setCommand(const std::vector<std::string> &cmd_) {
  cmd = cmd_;
}

void Subprocess::addChildFD(int childFD, int pipeFD, int closeFD) {
  fds.push_back(ChildFD(childFD, pipeFD, closeFD));
}

void Subprocess::nullChildFD(int childFD) {
  fds.push_back(ChildFD(childFD, -1, -1));
}

pid_t Subprocess::run() {
  if(pid >= 0)
    throw std::logic_error("Subprocess::run but already running");
  // Convert the command
  std::vector<const char *> args;
  for(size_t n = 0; n < cmd.size(); ++n)
    args.push_back(cmd[n].c_str());
  args.push_back(NULL);
  // Display the command
  if(command.verbose) {
    IO::out.writef(">");
    for(size_t n = 0; n < cmd.size(); ++n)
      IO::out.writef(" %s", cmd[n].c_str());
    IO::out.writef("\n");
  }
  // Start the subprocess
  switch(pid = fork()) {
  case -1:
    throw SystemError("creating subprocess for " + cmd[0], errno);
  case 0:
    {
      int nullfd = -1;
      // Dup file descriptors into place
      for(size_t n = 0; n < fds.size(); ++n) {
        const ChildFD &cfd = fds[n];
        if(cfd.pipe >= 0) {
          if(dup2(cfd.pipe, cfd.child) < 0) { perror("dup2"); _exit(-1); }
        } else {
          if(nullfd < 0 && (nullfd = open(_PATH_DEVNULL, O_RDWR)) < 0) {
            perror("/dev/null");
            _exit(-1);
          }
          if(dup2(nullfd, cfd.child) < 0) { perror("dup2"); _exit(-1); }
        }
      }
      // Close leftovers
      for(size_t n = 0; n < fds.size(); ++n) {
        const ChildFD &cfd = fds[n];
        if(cfd.pipe >= 0) {
          if(close(cfd.pipe) < 0) { perror("close"); _exit(-1); }
          for(size_t m = n + 1; m < fds.size(); ++m)
            if(fds[m].pipe == cfd.pipe)
              fds[m].pipe = -1;
        }
        if(cfd.close >= 0)
          if(close(cfd.close) < 0) { perror("close"); _exit(-1); }
      }
      if(nullfd >= 0 && close(nullfd) < 0) { perror("close"); _exit(-1); }
      // Execute the command
      execvp(args[0], (char **)&args[0]);
      perror(args[0]);
      _exit(-1);
    }
  }
  // Close file descriptors used by the child
  for(size_t n = 0; n < fds.size(); ++n) {
    const ChildFD &cfd = fds[n];
    if(cfd.pipe >= 0) {
      if(close(cfd.pipe) < 0)
        throw IOError("closing FD for " + cmd[0], errno);
      for(size_t m = n + 1; m < fds.size(); ++m)
        if(fds[m].pipe == cfd.pipe)
          fds[m].pipe = -1;
    }
  }
  return pid;
}

int Subprocess::wait(bool checkStatus) {
  if(pid < 0)
    throw std::logic_error("Subprocess::wait but not running");
  int w;
  pid_t p;

  while((p = waitpid(pid, &w, 0)) < 0 && errno == EINTR)
    ;
  if(p < 0)
    throw SystemError("waiting for subprocess", errno);
  if(checkStatus && w) {
    if(WIFSIGNALED(w) && WTERMSIG(w) == SIGPIPE)
      ;
    else
      throw SubprocessFailed(cmd[0], w);
  }
  pid = -1;
  return w;
}

int Subprocess::execute(const std::vector<std::string> &cmd,
                        bool checkStatus) {
  Subprocess sp(cmd);
  return sp.runAndWait(checkStatus);
}
