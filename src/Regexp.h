//-*-C++-*-
// Copyright © 2011, 2012 Richard Kettlewell.
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
/** @file Regexp.h
 * @brief Regular expression support
 */

#include <sys/types.h>
#include <regex.h>
#include <string>
#include <vector>

/** @brief Regexp matching */
class Regexp {
public:
  /** @brief Constructor
   * @param regex Regular expression to compare with
   * @param cflags Flags to @c regcomp()
   */
  Regexp(const std::string &regex, int cflags = REG_EXTENDED);

  /** @brief Destructor */
  ~Regexp();

  /** @brief Test for a match
   * @param s Subject string
   * @param eflags Flags to @c regexec()
   * @return True if a match found
   */
  bool matches(const std::string &s, int eflags = 0);

  /** @brief Return a capture
   * @param n Capture number
   */
  std::string sub(size_t n) const;

private:
  std::string subject;
  regex_t compiled;
  std::vector<regmatch_t> capture;
};

#endif /* REGEXP_H */

