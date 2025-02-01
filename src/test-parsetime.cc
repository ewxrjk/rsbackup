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
  assert(parseTimeOfDay("0:0") == 0);
  assert(parseTimeOfDay("1:0") == 3600);
  assert(parseTimeOfDay("0:01") == 60);
  assert(parseTimeOfDay("0:00:01") == 1);
  assert(parseTimeOfDay("24:00:00") == 86400);

  try {
    parseTimeOfDay("0");
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseTimeOfDay("0:0:0:0");
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseTimeOfDay("24:0:1");
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseTimeOfDay("0:60:0");
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    parseTimeOfDay("0:0:60");
    assert(0);
  } catch(SyntaxError &) {
  }

  assert(formatTimeOfDay(0) == "0:00:00");
  assert(formatTimeOfDay(1) == "0:00:01");
  assert(formatTimeOfDay(60) == "0:01:00");
  assert(formatTimeOfDay(120) == "0:02:00");
  assert(formatTimeOfDay(3600) == "1:00:00");
  assert(formatTimeOfDay(86399) == "23:59:59");
  assert(formatTimeOfDay(86400) == "24:00:00");

  return 0;
}
