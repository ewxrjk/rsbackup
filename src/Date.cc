// Copyright © 2011, 2015 Richard Kettlewell.
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
#include "Date.h"
#include "Errors.h"
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <sstream>

// Cumulative day numbers at start of each month
// (for a non-leap-year)
const int Date::mday[] = {
  0,
  0,                                    // January
  31,
  31 + 28,
  31 + 28 + 31,
  31 + 28 + 31 + 30,
  31 + 28 + 31 + 30 + 31,
  31 + 28 + 31 + 30 + 31 + 30,
  31 + 28 + 31 + 30 + 31 + 30 + 31,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31,
};

Date::Date(const std::string &dateString) {
  *this = dateString;
}

Date &Date::operator=(const std::string &dateString) {
  long bits[3];
  const char *s = dateString.c_str();
  char *e;
  for(int n = 0; n < 3; ++n) {
    if(n) {
      if(*s != '-')
        throw InvalidDate("invalid date string '" + dateString + "'");
      ++s;
    }
    if(!isdigit(*s))
      throw InvalidDate("invalid date string '" + dateString + "'");
    errno = 0;
    bits[n] = strtol(s, &e, 10);
    if(errno)
      throw InvalidDate("invalid date string '" + dateString + "' - " + strerror(errno));
    s = e;
  }
  if(*s)
    throw InvalidDate("invalid date string '" + dateString + "'");
  if(bits[0] < 1 || bits[0] > INT_MAX)
    throw InvalidDate("invalid date string '" + dateString + "' - year too small");
  y = bits[0];
  if(bits[1] < 1 || bits[1] > 12)
    throw InvalidDate("invalid date string '" + dateString + "' - month out of range");
  m = bits[1];
  if(bits[2] < 1 || bits[2] > monthLength(y, m))
    throw InvalidDate("invalid date string '" + dateString + "' - day out of range");
  d = bits[2];
  return *this;
}

Date::Date(time_t when) {
  struct tm result;
  if(!localtime_r(&when, &result)) {
    std::stringstream ss;
    ss << "invalid time_t: " << when << ": " << strerror(errno);
    throw InvalidDate(ss.str());
  }
  y = result.tm_year + 1900;
  m = result.tm_mon + 1;
  d = result.tm_mday;
}

Date &Date::operator++() {
  if(d < monthLength(y, m))
    ++d;
  else {
    d = 1;
    if(++m > 12) {
      m -= 12;
      y += 1;
    }
  }
  return *this;
}

Date &Date::addMonth() {
  m += 1;
  if(m > 12) {
    m -= 12;
    y += 1;
  }
  d = std::min(d, monthLength(y, m));
  return *this;
}

std::string Date::toString() const {
  char buffer[64];
  snprintf(buffer, sizeof buffer, "%04d-%02d-%02d", y, m, d);
  return buffer;
}

int Date::toNumber() const {
  int dayno = 365 * y + y / 4 - y / 100 + y / 400;
  dayno += mday[m] + (m > 2 && isLeapYear() ? 1 : 0);
  dayno += d - 1;
  return dayno;
}

time_t Date::toTime() const {
  struct tm t;
  memset(&t, 0, sizeof t);
  t.tm_year = y - 1900;
  t.tm_mon = m - 1;
  t.tm_mday = d;
  t.tm_isdst = -1;
  time_t r = mktime(&t);
  if(r == -1)
    throw SystemError("mktime failed"); // not documented as setting errno
  return r;
}

bool Date::operator<(const Date &that) const {
  int delta;
  if((delta = y - that.y)) return delta < 0;
  if((delta = m - that.m)) return delta < 0;
  if((delta = d - that.d)) return delta < 0;
  return false;
}

int Date::operator-(const Date &that) const {
  return toNumber() - that.toNumber();
}

Date Date::today() {
  // Allow overriding of 'today' form environment for testing
  const char *override = getenv("RSBACKUP_TODAY");
  if(override)
    return Date(override);
  time_t now;
  time(&now);
  return Date(now);
}

time_t Date::now() {
  const char *override = getenv("RSBACKUP_TODAY");
  if(override)
    return Date(override).toTime();
  return time(nullptr);
}

int Date::monthLength(int y, int m) {
  int len = mday[m + 1] - mday[m];
  if(m == 2 && isLeapYear(y))
    ++len;
  return len;
}
