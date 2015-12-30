//-*-C++-*-
// Copyright © 2011, 2012, 2014, 2015 Richard Kettlewell.
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
#ifndef UTILS_H
#define UTILS_H
/** @file Utils.h
 * @brief Miscellaneous utilities
 */

#include <string>
#include <vector>
#include <climits>
#include <cassert>
#include <ctime>
#include <cmath>
#include <limits>

class IO;                               // forward declaration

/** @brief Display a prompt and retrieve a yes/no reply
 * @param format Format string as per @c printf()
 * @param ... Arguments
 * @return True if the user said yes
 *
 * Overridden by @c --force, which means "always yes".
 */
bool check(const char *format, ...);

/** @brief Convert to Unicode
 * @param u Where to put Unicode string
 * @param mbs Multibyte string
 */
void toUnicode(std::u32string &u, const std::string &mbs);

/** @brief Display a progress bar
 * @param stream Output stream
 * @param prompt Prompt string
 * @param done Work done
 * @param total Total work
 *
 * If @p total is 0 then the progress bar is erased.
 */
void progressBar(IO &stream, const char *prompt, size_t done, size_t total);

/** @brief Break up a string into lines
 * @param lines Where to put lines
 * @param s Input string
 * @return Number of lines
 *
 * Lines are terminated by @c \\n.  The members of @p lines do not include the
 * newline.
 */
size_t toLines(std::vector<std::string> &lines,
               const std::string &s);

/** @brief Expand a filename glob pattern
 * @param files List of filenames
 * @param pattern Pattern
 * @param flags Flags as per glob(3)
 */
void globFiles(std::vector<std::string> &files,
               const std::string &pattern,
               int flags);

/** @brief Parse an integer
 * @param s Representation of integer
 * @param min Minimum acceptable value
 * @param max Maximum acceptable value
 * @param radix Base, or 0 to follow C conventions
 * @return Integer value
 * @throws SyntaxError if the @p s doesn't represent an integer
 * @throws SyntaxError if the integer value is out of range
 */
int parseInteger(const std::string &s,
                 int min = INT_MIN, int max = INT_MAX,
                 int radix = 0);

/** @brief Parse a float
 * @param s Representation of float
 * @param min Minimum acceptable value
 * @param max Maximum acceptable value
 * @return Floating point value
 * @throws SyntaxError if the @p s doesn't represent a number
 * @throws SyntaxError if the value is out of range
 */
double parseFloat(const std::string &s,
                  double min = -std::numeric_limits<double>::max(),
                  double max = std::numeric_limits<double>::max());

/** @brief Split and parse a list represented as a string
 * @param bits Destination for components of the string
 * @param line String to parse
 * @throws SyntaxError if @p line is malformed.
 *
 * Each component can be quoted or unquoted.
 *
 * Unquoted components are delimited by whitespace and cannot contain double
 * quotes or backslashes.
 *
 * Quoted components are delimited by double quotes.  Within the quotes
 * backslash can be used to escape the next character.
 *
 * The hash character can appear inside quotes or noninitially in an unquoted
 * component, but otherwise introduces a comment which extends to the end of
 * @p line.
 */
void split(std::vector<std::string> &bits, const std::string &line);

/** @brief Display an error message
 * @param fmt Format string, as printf()
 * @param ... Arguments to format string
 *
 * Writes an error message to standard error.
 *
 * Increments @ref errors.
 */
void error(const char *fmt, ...);

/** @brief Display a warning message
 * @param fmt Format string, as printf()
 * @param ... Arguments to format string
 *
 * Writes an warning message to standard error.
 */
void warning(const char *fmt, ...);

/** @brief Compare timespec values */
inline int compare_timespec(const struct timespec &a, const struct timespec &b) {
  if(a.tv_sec < b.tv_sec)
    return -1;
  if(a.tv_sec > b.tv_sec)
    return 1;
  if(a.tv_nsec < b.tv_nsec)
    return -1;
  if(a.tv_nsec > b.tv_nsec)
    return 1;
  return 0;
}

/** @brief Compare timespec values */
inline bool operator>=(const struct timespec &a, const struct timespec &b) {
  return compare_timespec(a, b) >= 0;
}

/** @brief Compare timespec values */
inline bool operator==(const struct timespec &a, const struct timespec &b) {
  return compare_timespec(a, b) == 0;
}

/** @brief Compare timespec values */
inline bool operator<(const struct timespec &a, const struct timespec &b) {
  return compare_timespec(a, b) < 0;
}

/** @brief Subtract timespec values */
inline struct timespec operator-(const struct timespec &a,
                                 const struct timespec &b) {
  struct timespec r;
  r.tv_sec = a.tv_sec - b.tv_sec;
  r.tv_nsec = a.tv_nsec - b.tv_nsec;
  if(r.tv_nsec < 0) {
    r.tv_nsec += 1000000000;
    r.tv_sec -= 1;
  }
  return r;
}

/** @brief Log to arbitrary base
 * @param x Argument to logarithm
 * @param b Base for logarithm
 * @return \f$log_b(x)\f$
 */
inline double logbase(double x, double b) {
  /* Why log2 instead of log?  Because FreeBSD's log() implementation is
   * inaccurate. */
  return log2(x) / log2(b);
}

/** @brief Make a file descriptor nonblocking
 * @param fd File descriptor
 */
void nonblock(int fd);

/** @brief Delete all the children of a container
 * @param container Container
 *
 * Uses @c delete to destroy all the elements of @p container, and then empties
 * it.
 */
template<typename C>
void deleteAll(C &container) {
  for(auto element: container)
    delete element;
  container.clear();
}

/** @brief Test whether a container contains a particular element
 * @param container container to search
 * @param element element to test for
 * @return @c true if @p element can be found in @p container
 */
template<typename C, typename E>
bool contains(const C &container, const E &element) {
  return container.find(element) != container.end();
}

/** @brief RFC4684 base64 alphabet for @ref write_base64() */
extern const char rfc4684_base64[];

/** @brief Convert a string to base64
 * @param os Output stream
 * @param s String to convert
 * @param alphabet Digits and padding (must be 65 bytes long)
 * @return @p os
 */
std::ostream &write_base64(std::ostream &os,
                           const std::string &s,
                           const char *alphabet = rfc4684_base64);

/** @brief Expand environment variable references
 * @param s Input string
 * @param pos Start position within @p s
 * @param n Number of characters to read within @p s
 * @return @p s with environment variables expanded
 *
 * Within @p s:
 *
 * 1. The @c \ character quotes the following character.  The exception is that
 * if it is at the end of the string, it is not replaced.
 *
 * 2. The sequence @c ${NAME} is replaced (nonrecursively) with the value
 * of the environment variable @c NAME, if it is set.
 */
std::string substitute(const std::string &s,
                       std::string::size_type pos = 0,
                       std::string::size_type n = std::string::npos);

#endif /* UTILS_H */
