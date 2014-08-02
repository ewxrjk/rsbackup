// Copyright © 2014 Richard Kettlewell.
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
#include "rsbackup.h"
#include "Utils.h"
#include "Errors.h"
#include <cstring>
#include <glob.h>

extern "C" {
  static int globFilesError(const char *epath, int errno);
};

static std::string globErrorPath;
static int globErrno;

void globFiles(std::vector<std::string> &files,
               const std::string &pattern,
               int flags) {
  glob_t g;
  memset(&g, 0, sizeof g);
  try {
    switch(glob(pattern.c_str(), flags, globFilesError, &g)) {
    case GLOB_NOSPACE:
      throw SystemError("glob: out of memory");
    case GLOB_ABORTED:
      throw SystemError(globErrorPath, globErrno);
    default:
      throw SystemError("glob: unrecognized return value");
    case GLOB_NOMATCH:
    case 0:
      break;
    }
    files.clear();
    for(size_t n = 0; n < g.gl_pathc; ++n)
      files.push_back(g.gl_pathv[n]);
  } catch(std::runtime_error &) {
    globfree(&g);
    throw;
  }
  globfree(&g);
}

static int globFilesError(const char *epath, int errno) {
  // Not re-entrant (stupid API design).  Fortunately this isn't a threaded
  // program...
  globErrorPath = epath;
  globErrno = errno;
  return -1;
}
