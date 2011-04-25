// -*-C++-*-
#ifndef IO_H
#define IO_H

#include <string>
#include <vector>
#include <cstdio>

#include <sys/types.h>
#include <dirent.h>

enum PipeDirection {
  ReadFromPipe,                         // parent reads, child writes
  WriteToPipe,                          // parent writes, child reads
};

// RAII-friendly I/O class.  Members will throw IOError if anything goes wrong.
class StdioFile {
public:
  StdioFile(): fp(NULL), pid(-1) {}
  ~StdioFile();

  void open(const std::string &path, const std::string &mode);
  void popen(const std::vector<std::string> &command,
             PipeDirection d);
  int close();
  
  bool readline(std::string &line);
  void readlines(std::vector<std::string> &lines);
  
  void write(const std::string &s);
  int writef(const char *format, ...);
  void flush();

private:
  FILE *fp;
  std::string path;
  pid_t pid;
  int wait();
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
