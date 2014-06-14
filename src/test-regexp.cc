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
#include "Regexp.h"
#include "Errors.h"
#include <cassert>

int main() {
#if !__FreeBSD__                        // weird prejudice against empty re
  Regexp r1("");

  assert(r1.matches(""));
  assert(r1.sub(0) == "");
  assert(r1.sub(1) == "");

  assert(r1.matches("spong"));
  assert(r1.sub(0) == "");
  assert(r1.sub(1) == "");
#endif

  Regexp r2(".");

  assert(r2.matches("spong"));
  assert(r2.sub(0) == "s");
  assert(r2.sub(1) == "");

  assert(!r2.matches(""));
  assert(r2.sub(0) == "");
  assert(r2.sub(1) == "");

  Regexp r3("xy");

  assert(!r3.matches(""));
  assert(r3.sub(0) == "");
  assert(r3.sub(1) == "");

  assert(!r3.matches("spong"));
  assert(r3.sub(0) == "");
  assert(r3.sub(1) == "");

  assert(r3.matches("xyzzy"));
  assert(r3.sub(0) == "xy");
  assert(r3.sub(1) == "");

  Regexp r4("@(._.)@");
  
  assert(!r4.matches("spong"));
  assert(r4.matches("@x_y@"));
  assert(r4.sub(0) == "@x_y@");
  assert(r4.sub(1) == "x_y");
  assert(r4.sub(2) == "");

  try {
    Regexp r5("(");
    assert(!"unexpectedly succeeded");
  } catch(InvalidRegexp &e) {
    /* expected */
  }

  return 0;
}
