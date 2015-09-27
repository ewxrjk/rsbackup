// Copyright © 2011, 2012, 2014, 2015 Richard Kettlewell.
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
#include <sys/time.h>
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

void Subprocess::addChildFD(int childFD, int pipeFD, int closeFD,
                            int otherChildFD) {
  fds.push_back(ChildFD(childFD, pipeFD, closeFD, otherChildFD));
}

void Subprocess::nullChildFD(int childFD) {
  fds.push_back(ChildFD(childFD, -1, -1, -1));
}

void Subprocess::capture(int childFD, std::string *s, int otherChildFD) {
  int p[2];
  if(pipe(p) < 0)
    throw IOError("creating pipe", errno);
  addChildFD(childFD, p[1], p[0], otherChildFD);
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
        if(cfd.childOther >= 0
           && dup2(cfd.child, cfd.childOther) < 0) {
          perror("dup2");
          _exit(-1);
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

void Subprocess::onReadable(EventLoop *e, int fd, const void *ptr, size_t n) {
  if(n)
    captures[fd]->append((char *)ptr, n);
  else
    e->cancelRead(fd);
}

void Subprocess::onReadError(EventLoop *, int, int errno_value) {
  throw IOError("reading pipe", errno_value);
}

void Subprocess::onTimeout(EventLoop *, const struct timespec &) {
  warning("%s exceeded timeout of %d seconds",
          cmd[0].c_str(), timeout);
  kill(pid, SIGKILL);
}

void Subprocess::onWait(EventLoop *, pid_t, int status, const struct rusage &) {
  this->status = status;
}

int Subprocess::wait(bool checkStatus) {
  if(pid < 0)
    throw std::logic_error("Subprocess::wait but not running");

  EventLoop e;
  for(auto it = captures.begin(); it != captures.end(); ++it)
    e.whenReadable(it->first, static_cast<Reactor *>(this));
  if(timeout > 0) {
    struct timespec timeLimit;
    getTimestamp(timeLimit);
    if(timeLimit.tv_sec <= time_t_max() - timeout)
      timeLimit.tv_sec += timeout;
    else
      timeLimit.tv_sec = time_t_max();
    e.whenTimeout(timeLimit, this);
  }
  e.whenWaited(pid, this);
  e.wait();

  if(checkStatus && status) {
    if(WIFSIGNALED(status) && WTERMSIG(status) == SIGPIPE)
      ;
    else
      throw SubprocessFailed(cmd[0], status);
  }
  pid = -1;
  return status;
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

void Subprocess::getTimestamp(struct timespec &now) {
#ifdef CLOCK_MONOTONIC
  if(clock_gettime(CLOCK_MONOTONIC, &now) < 0)
    throw IOError("clock_gettime", errno);
#else
  struct timeval tv;
  if(gettimeofday(&tv, NULL) < 0)
    throw IOError("gettimeofday", errno);
  now.tv_sec = tv.tv_sec;
  now.tv_nsec = tv.tv_sec * 1000;
#endif
}
