//-*-C++-*-
#ifndef FILELOCK_H
#define FILELOCK_H

#include <string>

// Hold an flock() lock on a file
class FileLock {
public:
  FileLock(const std::string &path);
  ~FileLock();

  // Acquire the lock.  If WAIT then block until it can be acquired, otherwise
  // give up immediately if it can't.  Returns true if the lock was acquired.
  bool acquire(bool wait = true);

  // Release the lock.
  void release();
private:
  std::string path;
  int fd;
  bool held;

  void ensureOpen();
};

#endif /* FILELOCK_H */

