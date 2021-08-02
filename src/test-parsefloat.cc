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
  // In range
  assert(parseFloat("0", 0, 0, InclusiveLimit) == 0);
  assert(parseFloat("0", 0, 0, InclusiveLimit) == 0);
  assert(parseFloat("0.25", 0, INT_MAX, InclusiveLimit) == 0.25);
  assert(parseFloat("-100", INT_MIN, INT_MAX, InclusiveLimit) == -100);
  assert(parseFloat("-0.25", INT_MIN, INT_MAX, InclusiveLimit) == -0.25);
  assert(parseFloat("100", 0, 100, InclusiveLimit) == 100);
  assert(parseFloat("-100", -100, 100, InclusiveLimit) == -100);
  assert(parseFloat("1e40", 0, HUGE_VAL, InclusiveLimit) == 1.0e40);

  // Essentially test that compiler and C library are reasonably consistent
  assert(parseFloat("0.1", 0, INT_MAX, InclusiveLimit) == 0.1);
  assert(parseFloat("0.01", 0, INT_MAX, InclusiveLimit) == 0.01);
  assert(parseFloat("0.001", 0, INT_MAX, InclusiveLimit) == 0.001);

  // Out of range
  try {
    parseFloat("10", 0, 9, InclusiveLimit);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseFloat("10", 0, 10, ExclusiveLimit);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseFloat("-10", -9, 0, InclusiveLimit);
    assert(0);
  } catch(SyntaxError &) {
  }
  try {
    parseFloat("1-0", -10, 0, ExclusiveLimit);
    assert(0);
  } catch(SyntaxError &) {
  }

  // Parse error
  try {
    parseFloat("junk", 0, 1000, InclusiveLimit);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseFloat("100 ", 0, 1000, InclusiveLimit);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseFloat(" 100", 0, 1000, InclusiveLimit);
    assert(0);
  } catch(SyntaxError &) {
  }

  return 0;
}
