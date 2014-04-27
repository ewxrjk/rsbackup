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
#include <cassert>
#include <cstdio>
#include <sstream>

int main() {
  Date d("1997-03-02");
  assert(d.toString() == "1997-03-02");
  assert(d.toNumber() == (1997*365+(1997/4)-(1997/100)+(1997/400)
                          +31+28
                          +2-1));
  std::stringstream s;
  s << d;
  assert(s.str() == "1997-03-02");
  Date e("1998-03-02");
  assert(e.toString() == "1998-03-02");
  assert(e.toNumber() == (1998*365+(1997/4)-(1997/100)+(1997/400)
                          +31+28
                          +2-1));
  assert(e - d == 365);
  Date t = Date::today();
  printf("today = %s = %d\n", t.toString().c_str(), t.toNumber());
  assert(Date("1997-03-01") < Date("1997-03-02"));
  assert(Date("1997-03-02") < Date("1997-04-01"));
  assert(Date("1997-03-02") < Date("1998-01-01"));
  assert(Date::monthLength(1999, 1) == 31);
  assert(Date::monthLength(1999, 2) == 28);
  assert(Date::monthLength(2000, 2) == 29);
  assert(Date::monthLength(2004, 2) == 29);
  assert(Date::monthLength(2100, 2) == 28);
  return 0;
}
