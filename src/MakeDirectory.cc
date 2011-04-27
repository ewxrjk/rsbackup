#include <config.h>
#include "IO.h"
#include "Errors.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>

static bool directoryExists(const std::string &path) {
  struct stat sb;
  if(stat(path.c_str(), &sb) == 0) {
    if(!S_ISDIR(sb.st_mode))
      throw IOError(path + " is not a directory");
    return true;
  }
  if(errno == ENOENT)
    return false;
  throw IOError("checking " + path, errno);
}

static std::string parentDirectory(const std::string &path) {
  size_t slash = path.rfind('/');
  if(slash == std::string::npos)
    throw IOError("no slash found in " + path);
  return std::string(path, 0, slash);
}

void makeDirectory(const std::string &path,
                   mode_t mode) {
  if(directoryExists(path))
    return;
  makeDirectory(parentDirectory(path));
  if(mkdir(path.c_str(), mode) < 0)
    throw IOError("creating directory " + path, errno);
}
