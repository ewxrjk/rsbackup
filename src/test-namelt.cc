// Copyright © 2015 Richard Kettlewell.
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
#include "Conf.h"
#include <cassert>

int main() {
  assert(!namelt("", ""));
  assert(!namelt("aaaa", "aaaa"));
  assert(namelt("aaa", "aaaa"));
  assert(!namelt("aaaa", "aaa"));
  assert(namelt("aaaa", "bbbb"));
  assert(!namelt("bbbb", "aaaa"));
  assert(namelt("", "bbbb"));
  assert(!namelt("bbbb", ""));
  assert(namelt("1", "2"));
  assert(namelt("01", "2"));
  assert(namelt("1", "02"));
  assert(namelt("x1", "x2"));
  assert(namelt("x01", "x2"));
  assert(namelt("x1b", "x2a"));
  assert(!namelt("x1", "1"));
  assert(namelt("1", "x1"));
  return 0;
}
