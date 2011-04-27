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

  // Open a file; MODE as per fopen()
  void open(const std::string &path, const std::string &mode);

  // Open a pipe to (d=WriteToPipe) or from (d=ReadFromPipe) a subprocess.
  void popen(const std::vector<std::string> &command,
             PipeDirection d);

  // Close file.  If the file is a pipe and checkStatus is true then an
  // exception is thrown if the subprocess indicates an error with its wait
  // status.  The wait status is returned.  If the file is not a pipe then the
  // return value is 0.
  int close(bool checkStatus = true);

  // Read one line; strips the newline.
  bool readline(std::string &line);

  // Read all the lines; strips the newlines.
  void readlines(std::vector<std::string> &lines);

  // Write a string
  void write(const std::string &s);

  // Write a formatted string as per printf()
  int writef(const char *format, ...);

  // Flush buffered writes
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

  // Open a directory
  void open(const std::string &path);

  // Get one filename.  Only the basename is supplied.  Returns true if a
  // filename was found, false when there are no more.
  bool get(std::string &name) const;
private:
  DIR *dp;
  std::string path;
};

// Make directory PATH including any parents
void makeDirectory(const std::string &path,
                   mode_t mode = 0777);

#endif /* IO_H */
