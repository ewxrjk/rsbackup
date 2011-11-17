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
#include "Utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

bool isMountPoint(const std::string &path) {
  int fd;
  struct stat s, sp;
  bool rc = false;

  if((fd = open(path.c_str(), O_RDONLY)) < 0)
    return false;
  if(fstat(fd, &s) >= 0) {
    if(fstatat(fd, "..", &sp, 0) >= 0) {
      if(s.st_dev != sp.st_dev)
        rc = true;                      // parent is on a different fs from PATH
      else if(s.st_ino == sp.st_ino)
        rc = true;                      // parent is the same file as PATH
    }
  }
  close(fd);
  return rc;
}
