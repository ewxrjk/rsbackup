// Copyright © Richard Kettlewell.
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
#include "Errors.h"
#include "Utils.h"
#include <cstdlib>
#include <cerrno>

// Convert a string into an integer, throwing a SyntaxError if it is malformed
// or outside [min,max].
long long parseInteger(const std::string &s, long long min, long long max,
                       int radix) {
  // strtoll is a bit loose and accepts e.g. initial whitespace, which we don't
  // want.
  if(s.size() > 0 && !((s.at(0) >= '0' && s.at(0) <= '9') || s.at(0) == '-'))
    throw SyntaxError("invalid integer '" + s + "'");
  errno = 0;
  const char *sc = s.c_str();
  char *e;
  long long n = strtoll(sc, &e, radix);
  if(errno)
    throw SyntaxError("invalid integer '" + s + "': " + strerror(errno));
  if(*e || e == sc)
    throw SyntaxError("invalid integer '" + s + "'");
  if(n > max || n < min)
    throw SyntaxError("integer '" + s + "' out of range");
  return n;
}
