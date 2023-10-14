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
#include <sstream>

/** @brief A unit of time */
struct time_unit {
  /** @brief Character representing time unit */
  char ch;

  /** @brief Number of seconds in time unit */
  int seconds;
};

static const struct time_unit time_units[] = {
    {'d', 86400},
    {'h', 3600},
    {'m', 60},
    {'s', 1},
};

long long parseTimeInterval(std::string s, long long max) {
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
  if(unit == 0)
    throw SyntaxError("time interval must have a unit");
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

std::string formatTimeIntervalCompact(long long n) {
  std::stringstream ss;

  if(n == 0)
    return "0s"; // special case

  // Second count isn't interesting for larger times
  if(n >= 5 * 60) {
    auto seconds = n % 60;
    n -= seconds;
    if(seconds >= 30)
      n += 60;
  }
  // Minute count similarly
  if(n >= 5 * 3600) {
    auto minutes = (n / 60) % 60;
    n -= 60 * minutes;
    if(minutes >= 30)
      n += 3600;
  }
  for(auto &tu: time_units) {
    if(n >= tu.seconds) {
      ss << (n / tu.seconds);
      ss << tu.ch;
      n %= tu.seconds;
    }
  }
  return ss.str();
}
