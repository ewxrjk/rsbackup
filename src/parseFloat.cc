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

double parseFloat(const std::string &s, double min, double max,
                  LimitType limitType) {
  // strtod is a bit loose and accepts e.g. initial whitespace, which we don't
  // want.
  if(s.size() > 0 && !((s.at(0) >= '0' && s.at(0) <= '9') || s.at(0) == '-'))
    throw SyntaxError("invalid integer '" + s + "'");
  errno = 0;
  const char *sc = s.c_str();
  char *e;
  double n = strtod(sc, &e);
  if(errno)
    throw SyntaxError("invalid number '" + s + "': " + strerror(errno));
  if(*e || e == sc)
    throw SyntaxError("invalid number '" + s + "'");
  switch(limitType) {
  case InclusiveLimit:
    if(!(n >= min && n <= max))
      throw SyntaxError("number '" + s + "' out of range");
    break;
  case ExclusiveLimit:
    if(!(n > min && n < max))
      throw SyntaxError("number '" + s + "' out of range");
    break;
  default: throw std::logic_error("unrecognized LimitType");
  }
  return n;
}
