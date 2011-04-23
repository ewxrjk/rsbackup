//-*-C++-*-
#ifndef FILELOCK_H
#define FILELOCK_H

#include <string>

class FileLock {
public:
  FileLock(const std::string &path);
  ~FileLock();
  
  bool acquire(bool wait = true);
  void release();
private:
  std::string path;
  int fd;
  bool held;

  void ensureOpen();
};

#endif /* FILELOCK_H */

