// -*-C++-*-
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

void Indent::clear() {
  stack.clear();
  stack.push_back(0);
  new_level = 0;
}

unsigned Indent::check(unsigned acceptable_levels, size_t indent) {
  unsigned level;
  // See if the line matches anything in the stack
  for(size_t i = 0; i < stack.size(); ++i) {
    if(stack[i] == indent) {
      // If does; revert to that position
      if(i + 1 != stack.size())
        stack.erase(stack.begin() + i + 1, stack.end());
      new_level = 0;
      level = 1 << i;
      return level & acceptable_levels;
    }
  }
  if(new_level && indent > stack.back()) {
    stack.push_back(indent);
    level = new_level;
    new_level = 0;
    return level & acceptable_levels;
  }
  // Nothing found
  return 0;
}
