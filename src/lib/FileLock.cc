// Copyright © 2011, 2012, 2015 Richard Kettlewell.
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
#include "FileLock.h"
#include "Errors.h"
#include <sys/file.h>
#include <unistd.h>
#include <cerrno>

FileLock::FileLock(const std::string &path_): path(path_) {}

FileLock::~FileLock() {
  if(fd >= 0)
    close(fd);
}

void FileLock::ensureOpen() {
  if(fd >= 0)
    return;
  if((fd = open(path.c_str(), O_WRONLY | O_CREAT, 0666)) < 0)
    throw IOError("opening " + path, errno);
  int flags;
  if((flags = fcntl(fd, F_GETFL)) < 0)
    throw IOError("fcntl F_GETFL " + path, errno);
  if(fcntl(fd, F_SETFL, flags | FD_CLOEXEC) < 0)
    throw IOError("fcntl F_SETFL " + path, errno);
}

bool FileLock::acquire(bool wait) {
  ensureOpen();
  if(held)
    return true;
  if(flock(fd, wait ? LOCK_EX : LOCK_EX | LOCK_NB) < 0) {
    if(errno == EWOULDBLOCK && !wait)
      return false;
    throw IOError("acquiring lock on " + path, errno);
  }
  held = true;
  return true;
}

void FileLock::release() {
  if(!held)
    throw std::logic_error("release without acquire");
  if(flock(fd, LOCK_UN) < 0)
    throw IOError("releasing lock on " + path, errno);
  held = false;
}
