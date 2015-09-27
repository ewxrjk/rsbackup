// Copyright © 2015 Richard Kettlewell.
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
#include "Utils.h"
#include "EventLoop.h"
#include "Errors.h"
#include <cerrno>
#include <unistd.h>
#include <ctime>
#include <csignal>
#include <sys/wait.h>
#include <sys/resource.h>

Reactor::~Reactor() {}

void Reactor::onReadable(EventLoop *, int, const void *, size_t) {
  throw std::logic_error("Reactor::onReadable");
}

void Reactor::onReadError(EventLoop *, int, int) {
  throw std::logic_error("Reactor::onReadError");
}

void Reactor::onWritable(EventLoop *, int) {
  throw std::logic_error("Reactor::onWritable");
}

void Reactor::onTimeout(EventLoop *, const struct timespec &) {
  throw std::logic_error("Reactor::onTimeout");
}

void Reactor::onWait(EventLoop *, pid_t, int, const struct rusage &) {
  throw std::logic_error("Reactor::onWait");
}

EventLoop::EventLoop() {
  if(sigpipe[0] >= 0)
    throw std::logic_error("EventLoop::EventLoop");
  struct sigaction sa;
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss, SIGCHLD);
  if(sigprocmask(SIG_BLOCK, &ss, NULL) < 0)
    throw SystemError("sigprocmask", errno);
  sa.sa_handler = EventLoop::signalled;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGCHLD, &sa, NULL) < 0)
    throw SystemError("sigaction", errno);
  if(pipe(sigpipe) < 0)
    throw SystemError("pipe", errno);
  nonblock(sigpipe[0]);
  nonblock(sigpipe[1]);
  whenReadable(sigpipe[0], this);
}

EventLoop::~EventLoop() {
  sigset_t ss;
  signal(SIGCHLD, SIG_DFL);
  sigemptyset(&ss);
  sigaddset(&ss, SIGCHLD);
  if(sigprocmask(SIG_UNBLOCK, &ss, NULL) < 0)
    throw SystemError("sigprocmask", errno);
  close(sigpipe[0]);
  close(sigpipe[1]);
  sigpipe[0] = sigpipe[1] = -1;
}

void EventLoop::signalled(int) {
  int save_errno = errno;
  write(sigpipe[1], "", 1);
  errno = save_errno;
}

int EventLoop::sigpipe[2] = { -1, -1 };

void EventLoop::whenReadable(int fd, Reactor *r) {
  readers[fd] = r;
  reconf = true;
}

void EventLoop::cancelRead(int fd) {
  readers.erase(fd);
  reconf = true;
}

void EventLoop::whenWritable(int fd, Reactor *r) {
  writers[fd] = r;
  reconf = true;
}

void EventLoop::cancelWrite(int fd) {
  writers.erase(fd);
  reconf = true;
}

void EventLoop::whenTimeout(const struct timespec &t, Reactor *r) {
  timeouts.insert(std::pair<struct timespec, Reactor *>(t, r));
  reconf = true;
}

void EventLoop::whenWaited(pid_t pid, Reactor *r) {
  waiters[pid] = r;
  reconf = true;
}

void EventLoop::wait() {
  sigset_t ss;
  if(sigprocmask(SIG_SETMASK, NULL, &ss) < 0)
    throw SystemError("sigprocmask", errno);
  sigdelset(&ss, SIGCHLD);
  while(readers.size() > 1 || writers.size() > 0 || waiters.size() > 0) {
    fd_set rfds, wfds;
    struct timespec ts, *tsp;
    int maxfd = -1, n;
    if(timeouts.size() > 0) {
      auto it = timeouts.begin();
      struct timespec now, first = it->first;
      getTimestamp(now);
      if(now >= first) {
        Reactor *r = it->second;
        timeouts.erase(it);
        r->onTimeout(this, now);
        continue;
      }
      ts = first - now;
      if(ts.tv_sec >= 10)
        ts.tv_sec = 10;
      tsp = &ts;
    } else
      tsp = NULL;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    for(auto it = readers.begin(); it != readers.end(); ++it) {
      FD_SET(it->first, &rfds);
      maxfd = std::max(maxfd, it->first);
    }
    for(auto it = writers.begin(); it != writers.end(); ++it) {
      FD_SET(it->first, &wfds);
      maxfd = std::max(maxfd, it->first);
    }
    n = pselect(maxfd + 1, &rfds, &wfds, NULL, tsp, &ss);
    if(n < 0) {
      if(errno != EINTR)
        throw IOError("pselect", errno);
    } else if(n > 0) {
      reconf = false;
      for(auto it = readers.begin(); !reconf && it != readers.end(); ++it) {
        if(FD_ISSET(it->first, &rfds)) {
          char buffer[4096];
          ssize_t nbytes = read(it->first, buffer, sizeof buffer);
          if(nbytes < 0) {
            if(errno == EINTR || errno == EAGAIN)
              continue;
            it->second->onReadError(this, it->first, errno);
          } else
            it->second->onReadable(this, it->first, buffer, nbytes);
        }
      }
      for(auto it = writers.begin(); !reconf && it != writers.end(); ++it) {
        if(FD_ISSET(it->first, &wfds))
          it->second->onWritable(this, it->first);
      }
    }
  }
}

void EventLoop::onReadable(EventLoop *, int, const void *, size_t) {
  pid_t pid;
  struct rusage ru;
  int status;
  while(waiters.size() > 0) {
    pid = wait4(-1, &status, WNOHANG, &ru);
    if(pid < 0) {
      if(errno != EINTR)
        throw SystemError("wait4", errno);
    }
    if(pid <= 0)
      return;
    auto it = waiters.find(pid);
    if(it != waiters.end()) {
      Reactor *r = it->second;
      waiters.erase(it);
      r->onWait(this, pid, status, ru);
    }
  }
}

void EventLoop::getTimestamp(struct timespec &now) {
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
