// Copyright © 2012 Richard Kettlewell.
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
#include <cassert>
#include <cstdio>
#include <sstream>

#define assert_throws(expr, except) do {        \
  try {                                         \
    (expr);                                     \
    assert(!"unexpected succeeded");            \
  } catch(except &e) {                          \
  }                                             \
} while(0)

int main() {
  Date d("1997-03-02");
  assert(d.toString() == "1997-03-02");
  assert(d.toNumber() == (1997*365+(1997/4)-(1997/100)+(1997/400)
                          +31+28
                          +2-1));
  Date dd(d.toTime());
  assert(d.toString() == dd.toString());
  std::stringstream s;
  s << d;
  assert(s.str() == "1997-03-02");
  Date e("1998-03-02");
  assert(e.toString() == "1998-03-02");
  assert(e.toNumber() == (1998*365+(1997/4)-(1997/100)+(1997/400)
                          +31+28
                          +2-1));
  assert(e - d == 365);
  Date ee(e.toTime());
  assert(e.toString() == ee.toString());
  Date f;
  assert(f.toString() == "0000-01-01");
  Date t = Date::today();
  printf("today = %s = %d\n", t.toString().c_str(), t.toNumber());
  Date tt(t.toTime());
  assert(t.toString() == tt.toString());
  assert(Date("1997-03-01") < Date("1997-03-02"));
  assert(Date("1997-03-02") < Date("1997-04-01"));
  assert(Date("1997-03-02") < Date("1998-01-01"));
  assert(Date::monthLength(1999, 1) == 31);
  assert(Date::monthLength(1999, 2) == 28);
  assert(Date::monthLength(2000, 2) == 29);
  assert(Date::monthLength(2004, 2) == 29);
  assert(Date::monthLength(2100, 2) == 28);
  assert_throws(Date(""), InvalidDate);
  assert_throws(Date("whatever"), InvalidDate);
  assert_throws(Date("2012"), InvalidDate);
  assert_throws(Date("2012-03"), InvalidDate);
  assert_throws(Date("2012--03-01"), InvalidDate);
  assert_throws(Date("2012-03-04-05"), InvalidDate);
  assert_throws(Date("2012/03/04"), InvalidDate);
  assert_throws(Date("0-01-01"), InvalidDate);
  assert_throws(Date("2012-00-01"), InvalidDate);
  assert_throws(Date("2012-13-01"), InvalidDate);
  assert_throws(Date("2012-01-00"), InvalidDate);
  assert_throws(Date("2012-01-32"), InvalidDate);
  assert_throws(Date("2011-02-29"), InvalidDate);
  assert_throws(Date("0x100-02-29"), InvalidDate);
  assert_throws(Date("2147483648-02-01"), InvalidDate);
  assert_throws(Date("9223372036854775808-02-21"), InvalidDate);

  static const struct {
    const char *before, *after;
  } day_increment_tests[] = {
    { "1995-01-01", "1995-01-02" },
    { "1995-01-31", "1995-02-01" },
    { "1995-02-28", "1995-03-01" },
    { "1995-12-31", "1996-01-01" },
    { "1996-02-28", "1996-02-29" },
    { "1996-02-29", "1996-03-01" },
  };
  for(auto &t: day_increment_tests) {
    d = t.before;
    assert(d.toString() == t.before);
    ++d;
    printf("%s:%d: %s %s %s\n",
           __FILE__, __LINE__, t.before, t.after, d.toString().c_str());
    assert(d.toString() == t.after);
  }

  static const struct {
    const char *before, *after;
  } month_increment_tests[] = {
    { "1995-01-01", "1995-02-01" },
    { "1995-01-31", "1995-02-28" },
    { "1995-02-28", "1995-03-28" },
    { "1995-12-31", "1996-01-31" },
    { "1996-01-28", "1996-02-28" },
    { "1996-01-29", "1996-02-29" },
    { "1996-01-30", "1996-02-29" },
    { "1996-02-28", "1996-03-28" },
    { "1996-02-29", "1996-03-29" },
  };
  for(auto &t: month_increment_tests) {
    d = t.before;
    assert(d.toString() == t.before);
    d.addMonth();
    printf("%s:%d: %s %s %s\n",
           __FILE__, __LINE__, t.before, t.after, d.toString().c_str());
    assert(d.toString() == t.after);
  }

  static const struct {
    const std::string a, b;
    int delta;
  } subtract_tests[] = {
    { "2015-12-01", "2015-11-01", 30 },
    { "2016-02-01", "2016-01-01", 31 },
  };
  for(auto &t: subtract_tests) {
    Date a = t.a;
    assert(a.toString() == t.a);
    Date b = t.b;
    assert(b.toString() == t.b);
    int delta = a - b;
    printf("%s-%s=%d\n", a.toString().c_str(), b.toString().c_str(), delta);
    assert(delta == t.delta);
  }

  return 0;
}
