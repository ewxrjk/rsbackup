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
#include "Utils.h"
#include <cassert>

int main() {
  std::vector<std::string> lines;
  toLines(lines, "");
  assert(lines.size() == 0);

  lines.push_back("");
  toLines(lines, "");
  assert(lines.size() == 0);

  toLines(lines, "1");
  assert(lines.size() == 1);
  assert(lines[0] == "1");

  toLines(lines, "1\n");
  assert(lines.size() == 1);
  assert(lines[0] == "1");

  toLines(lines, "1\n2");
  assert(lines.size() == 2);
  assert(lines[0] == "1");
  assert(lines[1] == "2");

  return 0;
}
