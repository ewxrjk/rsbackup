// Copyright © 2020 Richard Kettlewell.
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
  assert(parseTimeInterval("0s") == 0);
  assert(parseTimeInterval("60s") == 60);
  assert(parseTimeInterval("10m") == 600);
  assert(parseTimeInterval("10d") == 86400 * 10);

  try {
    parseTimeInterval("10w");
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseTimeInterval("10ss");
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseTimeInterval("");
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseTimeInterval("3600s", 1000);
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseTimeInterval("3600");
    assert(0);
  } catch(SyntaxError &) {
  }

  assert(formatTimeInterval(0) == "0d");
  assert(formatTimeInterval(1) == "1s");
  assert(formatTimeInterval(60) == "1m");
  assert(formatTimeInterval(120) == "2m");
  assert(formatTimeInterval(3600) == "1h");
  assert(formatTimeInterval(86400) == "1d");

  return 0;
}
