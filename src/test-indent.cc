// Copyright © 2017 Richard Kettlewell.
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
#include "Indent.h"
#include "Utils.h"

// Flat config should work OK
static void indent_flat() {
  Indent i;
  unsigned level;

  level = i.check(1, 0);
  assert(level == 1);
  level = i.check(1, 0);
  assert(level == 1);
  level = i.check(3, 0);
  assert(level == 1);
}

// A slightly fiddlier config
static void indent_fiddly() {
  Indent i;
  unsigned level;

  level = i.check(1, 0);
  assert(level == 1);
  i.introduce(2);
  level = i.check(7, 8);
  assert(level == 2);
  level = i.check(7, 8);
  assert(level == 2);
  i.introduce(4);
  level = i.check(7, 15);
  assert(level == 4);
  level = i.check(7, 8);
  assert(level == 2);
  level = i.check(7, 0);
  assert(level == 1);
  i.introduce(2);
  level = i.check(7, 8);
  assert(level == 2);
}

// Short indent forbidden
static void indent_short() {
  Indent i;
  unsigned level;

  level = i.check(1, 0);
  assert(level == 1);
  i.introduce(2);
  level = i.check(7, 8);
  assert(level == 2);
  level = i.check(7, 6);
  assert(level == 0);
}

// Long indent forbidden
static void indent_long() {
  Indent i;
  unsigned level;

  level = i.check(1, 0);
  assert(level == 1);
  i.introduce(2);
  level = i.check(3, 8);
  assert(level == 2);
  level = i.check(3, 12);
  assert(level == 0);
}

int main() {
  indent_flat();
  indent_fiddly();
  indent_short();
  indent_long();
  return 0;
}
