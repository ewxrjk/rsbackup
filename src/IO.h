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

class Subprocess;

// RAII-friendly I/O class.  Members will throw IOError if anything goes wrong.
class StdioFile {
public:
  StdioFile(): fp(NULL), subprocess(NULL) {}
  ~StdioFile();

  void open(const std::string &path, const std::string &mode);
  void popen(const std::vector<std::string> &command,
             PipeDirection d);
  int close(bool checkStatus = true);
  
  bool readline(std::string &line);
  void readlines(std::vector<std::string> &lines);
  
  void write(const std::string &s);
  int writef(const char *format, ...);
  void flush();

private:
  FILE *fp;
  std::string path;
  Subprocess *subprocess;
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

// Make directory PATH including any parents
void makeDirectory(const std::string &path,
                   mode_t mode = 0777);

#endif /* IO_H */
