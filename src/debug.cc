// Copyright © 2016 Richard Kettlewell.
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
#include <cstdarg>

bool debug;

int write_debug(const char *path, long line, const char *msg, ...) {
  if(debug) {
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "%s:%ld: ", path, line);
    vfprintf(stderr, msg, ap);
    return fputc('\n', stderr);
  } else
    return 0;
}
