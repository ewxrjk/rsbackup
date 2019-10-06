// Copyright © 2014, 2016, 2019 Richard Kettlewell.
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
#include "Defaults.h"
#include "IO.h"
#include <cstdlib>

int globalErrors;
unsigned globalWarningMask = DEFAULT_WARNING_MASK;

static void error_generic(const char *tag, const char *fmt, va_list ap) {
  IO::err.writef("%s: ", tag);
  IO::err.vwritef(fmt, ap);
  IO::err.write("\n");
}

void fatal(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  error_generic("ERROR", fmt, ap);
  va_end(ap);
  exit(1);
}

void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  error_generic("ERROR", fmt, ap);
  va_end(ap);
  ++globalErrors;
}

void warning(unsigned warning_type, const char *fmt, ...) {
  if(!warning_type || (warning_type & globalWarningMask)) {
    va_list ap;
    va_start(ap, fmt);
    error_generic("WARNING", fmt, ap);
    va_end(ap);
  }
}
