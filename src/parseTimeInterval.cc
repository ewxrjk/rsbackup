// Copyright © 2011, 2012, 2014, 2015 Richard Kettlewell.
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
#include <cassert>
#include <cstdlib>
#include <cerrno>

/** @brief A unit of time */
struct time_unit {
  /** @brief Character representing time unit */
  int ch;

  /** @brief Number of seconds in time unit */
  int seconds;
};

static const struct time_unit time_units[] = {
    {'d', 86400},
    {'h', 3600},
    {'m', 60},
    {'s', 1},
};

long long parseTimeInterval(std::string s, int default_unit, long long max) {
  assert(default_unit > 0);
  int unit = 0;
  if(s.size() > 0) {
    char ch = s.at(s.size() - 1);
    if(isalpha(ch)) {
      ch = tolower(ch);
      unit = -1;
      for(auto &tu: time_units) {
        if(ch == tu.ch) {
          unit = tu.seconds;
          break;
        }
      }
      if(unit < 0)
        throw SyntaxError("unrecognized time unit");
      s.pop_back();
    }
  }
  if(unit == 0) {
    warning(WARNING_DEPRECATED, "time interval '%s' should have a unit",
            s.c_str());
    unit = default_unit;
  }
  long long n = parseInteger(s, 0);
  if(n > max / unit)
    throw SyntaxError("time interval too large to represent");
  return n * unit;
}

std::string formatTimeInterval(long long n) {
  char buffer[64];
  char ch = 0;
  for(auto &tu: time_units) {
    if(n % tu.seconds == 0) {
      n /= tu.seconds;
      ch = tu.ch;
      break;
    }
  }
  assert(ch);
  snprintf(buffer, sizeof buffer, "%lld%c", n, ch);
  return buffer;
}
