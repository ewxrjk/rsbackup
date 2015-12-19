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
#include "Color.h"
#include <cassert>
#include <cstdio>

int main() {
  for(unsigned n = 0; n < 256; ++n) {
    Color c(n);
    unsigned u = n;
    printf("%s:%d: c=(%g,%g,%g) n=%u u=%u\n", __FILE__, __LINE__,
           c.red, c.green, c.blue, n, u);
    assert(u == n);
    c = Color(n << 8);
    u = c >> 8;
    printf("%s:%d: c=(%g,%g,%g) n=%u u=%u\n", __FILE__, __LINE__,
           c.red, c.green, c.blue, n, u);
    assert(u == n);
    c = Color(n << 16);
    u = c >> 16;
    printf("%s:%d: c=(%g,%g,%g) n=%u u=%u\n", __FILE__, __LINE__,
           c.red, c.green, c.blue, n, u);
    assert(u == n);
  }
  assert(Color::HSV(0, 1, 1) == Color(0xFF0000));
  assert(Color::HSV(60, 1, 1) == Color(0xFFFF00));
  assert(Color::HSV(120, 1, 1) == Color(0x00FF00));
  assert(Color::HSV(180, 1, 1) == Color(0x00FFFF));
  assert(Color::HSV(240, 1, 1) == Color(0x0000FF));
  assert(Color::HSV(300, 1, 1) == Color(0xFF00FF));

  assert(Color::HSV(30, 1, 1) == Color(0xFF7F00));
  assert(Color::HSV(90, 1, 1) == Color(0x7FFF00));
  assert(Color::HSV(150, 1, 1) == Color(0x00FF7F));
  assert(Color::HSV(210, 1, 1) == Color(0x007FFF));
  assert(Color::HSV(270, 1, 1) == Color(0x7F00FF));
  assert(Color::HSV(330, 1, 1) == Color(0xFF007F));
  return 0;
}
