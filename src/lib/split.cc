// Copyright © 2011, 2012, 2014, 2015, 2017 Richard Kettlewell.
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
#include "Errors.h"
#include "Utils.h"

// Split a string into words.  Words are separated by whitespace but can also
// be quoted; inside quotes a backslash escapes any character.  A '#' where the
// start of a word would be introduces a comment which consumes the rest of the
// line.
void split(std::vector<std::string> &bits, const std::string &line,
           size_t *indent) {
  bits.clear();
  std::string::size_type pos = 0;
  std::string s;
  char c;
  if(indent) {
    size_t i = 0;
    while(pos < line.size() && ((c = line.at(pos)) == ' ' || c == '\t')) {
      if(c == ' ')
        ++i;
      else
        i = (i + 8) & ~static_cast<size_t>(7);
      ++pos;
    }
    *indent = i;
  }
  while(pos < line.size()) {
    c = line.at(pos);
    switch(c) {
    case ' ':
    case '\t':
    case '\r':
    case '\f': ++pos; break;
    case '#': return;
    case '"':
      s.clear();
      ++pos;
      while(pos < line.size() && line.at(pos) != '"') {
        if(line.at(pos) == '\\') {
          ++pos;
          if(pos >= line.size())
            throw SyntaxError("unterminated string");
        }
        s += line.at(pos);
        ++pos;
      }
      if(pos >= line.size())
        throw SyntaxError("unterminated string");
      ++pos;
      bits.push_back(s);
      break;
    case '\\': throw SyntaxError("unquoted backslash");
    default:
      s.clear();
      while(pos < line.size() && !isspace(line.at(pos)) && line.at(pos) != '"'
            && line.at(pos) != '\\') {
        s += line.at(pos);
        ++pos;
      }
      bits.push_back(s);
      break;
    }
  }
}
