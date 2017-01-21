// Copyright © 2015 Richard Kettlewell.
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
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <glob.h>
#include <unistd.h>

void create(const char *path) {
  int fd = open(path, O_WRONLY|O_CREAT, 0666);
  assert(fd >= 0);
  close(fd);
}

int main() {
  const char *tmpdir;
  char *dir;
  std::vector<std::string> files;

  tmpdir = getenv("TMPDIR");
  if(!tmpdir)
    tmpdir = "/tmp";
  assert(asprintf(&dir, "%s/XXXXXX", tmpdir) > 0);
  assert(mkdtemp(dir));
  assert(chdir(dir) >= 0);

  globFiles(files, "nothing", 0);
  assert(files.size() == 0);

  files.push_back("spong");
  globFiles(files, "nothing", 0);
  assert(files.size() == 0);

  globFiles(files, "*", 0);
  assert(files.size() == 0);

  globFiles(files, "*", GLOB_NOCHECK);
  assert(files.size() == 1);
  assert(files[0] == "*");

  create("a");

  globFiles(files, "*", 0);
  assert(files.size() == 1);
  assert(files[0] == "a");

  globFiles(files, "b", 0);
  assert(files.size() == 0);

  create("b");

  globFiles(files, "*", 0);
  assert(files.size() == 2);
  assert(files[0] == "a");
  assert(files[1] == "b");

  globFiles(files, "b", 0);
  assert(files.size() == 1);
  assert(files[0] == "b");

  int r = system(("rm -rf " + (std::string)dir).c_str());
  (void)r;                              // Work around GCC/Glibc stupidity

  free(dir);

  return 0;
}
