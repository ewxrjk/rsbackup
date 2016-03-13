// Copyright © 2011, 2012, 2014-2017 Richard Kettlewell.
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

Subprocess::Subprocess(const std::string &name,
                       const std::vector<std::string> &cmd_): Action(name),
                                                              cmd(cmd_) {
}

Subprocess::Subprocess(const std::string &name): Action(name) {
}

Subprocess::~Subprocess() {
  if(pid >= 0) {
    kill(pid, SIGKILL);
    try {
      if(eventloop)
        wait(0);
    } catch(...) {
    }
  }
  delete eventloop;
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
  assert(!eventloop);
  eventloop = new EventLoop();
  return launch(eventloop);
}

pid_t Subprocess::launch(EventLoop *e) {
  assert(e);                            // EventLoop must already exist
  if(pid >= 0)
    throw std::logic_error("Subprocess::run but already running");
  // Report if necessary
  if(reportNeeded)
    report();
  // Convert the command
  std::vector<const char *> args;
  for(auto &arg: cmd)
    args.push_back(arg.c_str());
  args.push_back(nullptr);
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
      for(auto &e: env) {
        const std::string &name = e.first, &value = e.second;
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
  warning(WARNING_ALWAYS, "%s exceeded timeout of %d seconds",
          cmd[0].c_str(), timeout);
  kill(pid, SIGKILL);
}

void Subprocess::onWait(EventLoop *, pid_t, int status, const struct rusage &) {
  this->status = status;
  if(actionlist)
    actionlist->completed(this, getActionStatus());
}

bool Subprocess::getActionStatus() const {
  return status == 0;
}

void Subprocess::setup(EventLoop *e) {
  if(pid < 0)
    throw std::logic_error("Subprocess::setup but not running");
  for(auto &c: captures)
    e->whenReadable(c.first, static_cast<Reactor *>(this));
  if(timeout > 0) {
    struct timespec timeLimit;
    getMonotonicTime(timeLimit);
    if(timeLimit.tv_sec <= std::numeric_limits<time_t>::max() - timeout)
      timeLimit.tv_sec += timeout;
    else
      timeLimit.tv_sec = std::numeric_limits<time_t>::max();
    e->whenTimeout(timeLimit, this);
  }
  e->whenWaited(pid, this);
}

int Subprocess::wait(unsigned waitBehavior) {
  assert(eventloop);
  setup(eventloop);
  eventloop->wait();
  delete eventloop;
  eventloop = nullptr;
  pid = -1;
  if(waitBehavior & THROW_ON_ERROR) {
    if(WIFEXITED(status) && WEXITSTATUS(status))
      throw SubprocessFailed(cmd[0], status);
  }
  if(waitBehavior & THROW_ON_CRASH) {
    if(WIFSIGNALED(status) && WTERMSIG(status) != SIGPIPE)
      throw SubprocessFailed(cmd[0], status);
  }
  if(waitBehavior & THROW_ON_SIGPIPE) {
    if(WIFSIGNALED(status) && WTERMSIG(status) == SIGPIPE)
      throw SubprocessFailed(cmd[0], status);
  }
  return status;
}

void Subprocess::go(EventLoop *e, ActionList *al) {
  actionlist = al;
  launch(e);
  setup(e);
}

void Subprocess::report() {
  if(reported)
    return;
  if(env.size()) {
    IO::out.writef("> # environment for next command\n");
    for(auto &e: env)
      IO::out.writef("> %s=%s\n", e.first.c_str(), e.second.c_str());
  }
  std::string command;
  for(size_t i = 0; i < cmd.size(); ++i) {
    if(i)
      command += ' ';
    command += cmd[i];
  }
  IO::out.writef("> %s\n", command.c_str());
  reported = true;
}

void Subprocess::reporting(bool reportCommand, bool reportNow) {
  reportNeeded = reportCommand;
  if(reportNeeded && reportNow)
    report();
}

std::string Subprocess::pathSearch(const std::string &name) {
  if(name.find('/') != std::string::npos)
    return name;
  const char *pathc = getenv("PATH");
  char buffer[1024];
  if(!pathc) {
    // Follow documented execvp behavior
    size_t rc = confstr(_CS_PATH, buffer, sizeof buffer);
    if(rc == 0)
      throw SystemError("confstr", errno);
    if(rc > sizeof buffer)
      throw SystemError("confstr: not enough space");
    pathc = buffer;
  }
  std::string path = pathc;
  size_t pos = 0;
  while(pos < path.size()) {
    size_t colon = path.find(':', pos);
    std::string e(path, pos, colon - pos);
    std::string p(e + "/" + name);
    if(access(p.c_str(), X_OK) == 0)
      return p;
    if(!(pos = colon + 1))
      break;
  }
  return "";
}
