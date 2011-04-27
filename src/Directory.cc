#include <config.h>
#include "IO.h"
#include "Errors.h"
#include <cerrno>

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
