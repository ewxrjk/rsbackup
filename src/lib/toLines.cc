// Copyright © 2014, 2015 Richard Kettlewell.
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
#include "rsbackup.h"
#include "Utils.h"

size_t toLines(std::vector<std::string> &lines, const std::string &s) {
  lines.clear();
  size_t pos = 0;
  const size_t limit = s.size();
  while(pos < limit) {
    size_t nl = s.find('\n', pos);
    if(nl == std::string::npos)
      break;
    lines.push_back(s.substr(pos, nl - pos));
    pos = nl + 1;
  }
  if(pos < limit)
    lines.push_back(s.substr(pos, limit - pos));
  return lines.size();
}
