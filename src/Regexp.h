//-*-C++-*-
// Copyright © 2011 Richard Kettlewell.
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
#ifndef REGEXP_H
#define REGEXP_H

#include <sys/types.h>
#include <regex.h>
#include <string>
#include <vector>

// Wrapper for libc's regex support
class Regexp {
public:
  Regexp(const std::string &regex, int cflags = REG_EXTENDED);
  ~Regexp();

  // Return true if S matches REGEX
  bool matches(const std::string &s, int eflags = 0);

  // Return the Nth capture (0=the matched substring)
  std::string sub(size_t n) const;

private:
  std::string subject;
  regex_t compiled;
  std::vector<regmatch_t> capture;
};

#endif /* REGEXP_H */

