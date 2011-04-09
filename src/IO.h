// -*-C++-*-
#ifndef IO_H
#define IO_H

#include <string>
#include <cstdio>

#include <sys/types.h>
#include <dirent.h>

// RAII-friendly I/O class.  Members will throw IOError if anything goes wrong.
class StdioFile {
public:
  StdioFile(): fp(NULL) {}
  ~StdioFile();

  void open(const std::string &path, const std::string &mode);
  void close();
  
  bool readline(std::string &line);
  
private:
  FILE *fp;
  std::string path;
};

// RAII-friendly directory reader.  Members will throw IOError if anything goes
// wrong.
class Directory {
public:
  Directory(): dp(NULL) {}
  ~Directory();

  void open(const std::string &path);

  bool get(std::string &name) const;
private:
  DIR *dp;
  std::string path;
};

#endif /* IO_H */
