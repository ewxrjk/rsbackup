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
#include "Errors.h"
#include "Utils.h"

int main(void) {
  assert(parseInteger("0", 0, 0, 0) == 0);
  assert(parseInteger("0", 0, 0, 8) == 0);
  assert(parseInteger("0", 0, 0, 10) == 0);
  assert(parseInteger("0", 0, 0, 16) == 0);

  assert(parseInteger("100", 0, INT_MAX, 0) == 100);
  assert(parseInteger("100", 0, INT_MAX, 8) == 64);
  assert(parseInteger("0100", 0, INT_MAX, 0) == 64);
  assert(parseInteger("100", 0, INT_MAX, 10) == 100);
  assert(parseInteger("100", 0, INT_MAX, 16) == 256);
  assert(parseInteger("0x100", 0, INT_MAX, 0) == 256);

  assert(parseInteger("-100", INT_MIN, INT_MAX, 0) == -100);
  assert(parseInteger("-100", INT_MIN, INT_MAX, 8) == -64);
  assert(parseInteger("-0100", INT_MIN, INT_MAX, 0) == -64);
  assert(parseInteger("-100", INT_MIN, INT_MAX, 10) == -100);
  assert(parseInteger("-100", INT_MIN, INT_MAX, 16) == -256);
  assert(parseInteger("-0x100", INT_MIN, INT_MAX, 0) == -256);

  try {
    parseInteger("10", 0, 9, 0);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseInteger("-10", 0, 9, 0);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseInteger("0x10", 0, 1000, 10);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseInteger("junk", 0, 1000, 0);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseInteger("100 ", 0, 1000, 0);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseInteger(" 100", 0, 1000, 0);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseInteger("2.2", 0, 1000, 0);
    assert(0);
  } catch(SyntaxError &) {
  }

  return 0;
}
