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
#include <cstdlib>

std::string substitute(const std::string &s, std::string::size_type pos,
                       std::string::size_type n) {
  std::string r;
  pos = std::min(s.size(), pos);
  n = std::min(n, s.size() - pos);
  std::string::size_type l = pos + n;
  while(pos < l) {
    char c = s.at(pos++);
    switch(c) {
    default: r += c; break;
    case '\\':
      if(pos >= l)
        r += c;
      else
        r += s.at(pos++);
      break;
    case '$':
      if(pos < l && s.at(pos) == '{') {
        std::string::size_type e = s.find('}', pos);
        if(e < l) {
          std::string name(s, pos + 1, e - (pos + 1));
          const char *value = getenv(name.c_str());
          if(value) {
            r += value;
            pos = e + 1;
            break;
          }
        }
      }
      r += c;
      break;
    }
  }
  return r;
}
