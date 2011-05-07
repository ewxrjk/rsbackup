// Copyright © 2011 Richard Kettlewell.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <config.h>
#include "IO.h"
#include "Errors.h"
#include "Defaults.h"
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
  size_t slash = path.rfind(PATH_SEP[0]);
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
