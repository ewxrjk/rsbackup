// Copyright © 2011, 2012 Richard Kettlewell.
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
#include "Utils.h"
#include <csignal>
#include <cerrno>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

Subprocess::Subprocess(): pid(-1),
                          timeout(0) {
}

Subprocess::Subprocess(const std::vector<std::string> &cmd_):
  pid(-1),
  cmd(cmd_),
  timeout(0) {
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

void Subprocess::capture(int childFD, std::string *s) {
  int p[2];
  if(pipe(p) < 0)
    throw IOError("creating pipe", errno);
  addChildFD(childFD, p[1], p[0]);
  captures[p[0]] = s;
}

pid_t Subprocess::run() {
  if(pid >= 0)
    throw std::logic_error("Subprocess::run but already running");
  // Convert the command
  std::vector<const char *> args;
  for(size_t n = 0; n < cmd.size(); ++n)
    args.push_back(cmd[n].c_str());
  args.push_back(NULL);
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
      for(std::map<std::string,std::string>::const_iterator it = env.begin();
          it != env.end();
          ++it) {
        const std::string &name = it->first, &value = it->second;
        if(::setenv(name.c_str(), value.c_str(), 1/*overwrite*/)) {
          perror("setenv");
          _exit(-1);
        }
      }
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

void Subprocess::captureOutput() {
  struct timespec timeLimit;
  if(timeout > 0) {
    if(clock_gettime(CLOCK_MONOTONIC, &timeLimit) < 0)
      throw IOError("clock_gettime", errno);
    if(timeLimit.tv_sec <= time_t_max() - timeout)
      timeLimit.tv_sec += timeout;
    else
      timeLimit.tv_sec = time_t_max();
  } else
    timeLimit.tv_sec = 0;
  while(captures.size() > 0) {
    fd_set fds;
    struct timespec ts, *tsp;
    FD_ZERO(&fds);
    for(std::map<int,std::string *>::const_iterator it = captures.begin();
        it != captures.end();
        ++it) {
      int fd = it->first;
      FD_SET(fd, &fds);
    }
    if(timeLimit.tv_sec && pid >= 0) {
      struct timespec now;
      if(clock_gettime(CLOCK_MONOTONIC, &now) < 0)
        throw IOError("clock_gettime", errno);
      if(now >= timeLimit) {
        IO::err.writef("WARNING: %s exceeded timeout of %d seconds\n",
                       cmd[0].c_str(), timeout);
        kill(pid, SIGKILL);
        pid = -1;
        tsp = NULL;
      } else {
        ts = timeLimit - now;
        if(ts.tv_sec >= 10) {
          ts.tv_sec = 10;
          ts.tv_nsec = 0;
        }
        tsp = &ts;
      }
    } else
      tsp = NULL;
    int n = pselect(captures.rbegin()->first + 1, &fds, NULL, NULL, tsp, NULL);
    if(n < 0) {
      if(errno != EINTR)
        throw IOError("select", errno);
    } else if(n > 0) {
      for(std::map<int,std::string *>::iterator it = captures.begin();
          it != captures.end();
          ++it) {
        int fd = it->first;
        if(FD_ISSET(fd, &fds)) {
          char buffer[4096];
          ssize_t bytes = read(fd, buffer, sizeof buffer);
          if(bytes > 0) {
            std::string *capture = it->second;
            capture->append(buffer, bytes);
          } else if(bytes == 0) {
            captures.erase(it);
            break;
          } else {
            if(!(errno == EINTR || errno == EAGAIN))
              throw IOError("reading pipe", errno);
          }
        }
      }
    }
  }
}

int Subprocess::wait(bool checkStatus) {
  if(pid < 0)
    throw std::logic_error("Subprocess::wait but not running");
  int w;
  pid_t p;

  captureOutput();
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

void Subprocess::report() {
  if(env.size()) {
    IO::out.writef("> # environment for next command\n");
    for(std::map<std::string,std::string>::const_iterator it = env.begin();
        it != env.end();
        ++it)
      IO::out.writef("> %s=%s\n", it->first.c_str(), it->second.c_str());
  }
  std::string command;
  for(size_t i = 0; i < cmd.size(); ++i) {
    if(i)
      command += ' ';
    command += cmd[i];
  }
  IO::out.writef("> %s\n", command.c_str());
}
