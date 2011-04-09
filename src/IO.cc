#include <config.h>
#include "IO.h"
#include "Errors.h"
#include <cerrno>

// StdioFile ------------------------------------------------------------------

StdioFile::~StdioFile() {
  if(fp)
    fclose(fp);
}

void StdioFile::open(const std::string &path_, const std::string &mode) {
  if(!(fp = fopen(path_.c_str(), mode.c_str())))
    throw IOError("opening " + path_, errno);
  path = path_;
}

void StdioFile::close() {
  if(fclose(fp) < 0)
    throw IOError("closing " + path, errno);
}

bool StdioFile::readline(std::string &line) {
  int c;
  line.clear();

  while((c = getc(fp)) != EOF && c != '\n')
    line += c;
  if(ferror(fp))
    throw IOError("reading " + path);
  return line.size() || !feof(fp);
}

// Directory ------------------------------------------------------------------

Directory::~Directory() {
  if(dp)
    closedir(dp);
}

void Directory::open(const std::string &path_) {
  if(!(dp = opendir(path_.c_str())))
    throw IOError("opening " + path_);
  path = path_;
}

bool Directory::get(std::string &name) const {
  errno = 0;
  struct dirent *de = readdir(dp);
  if(de) {
    name = de->d_name;
    return true;
  } else {
    if(errno)
      throw IOError("reading " + path);
    return false;
  }
}
