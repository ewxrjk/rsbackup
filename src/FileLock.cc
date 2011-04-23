#include <config.h>
#include "FileLock.h"
#include "Errors.h"
#include <sys/file.h>
#include <cerrno>

FileLock::FileLock(const std::string &path_): path(path_),
                                              fd(-1),
                                              held(false) {
}

FileLock::~FileLock() {
  if(fd >= 0)
    close(fd);
}

void FileLock::ensureOpen() {
  if(fd >= 0)
    return;
  if((fd = open(path.c_str(), O_WRONLY|O_CREAT, 0666)) < 0)
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
  if(flock(fd, wait ? LOCK_EX : LOCK_EX|LOCK_NB) < 0) {
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
