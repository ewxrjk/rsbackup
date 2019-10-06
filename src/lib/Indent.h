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
#ifndef INDENT_H
#define INDENT_H
/** @file Indent.h
 * @brief File indentation tracking
 */

#include <vector>
#include <cstddef>

/** @brief Indentation tracker
 *
 * The @e indent @e depth of a line is the number of spaces at the start. (The
 * caller will convert tabs in the usual way but that is out of scope for this
 * class.)
 *
 * The @e level of a line is an integer with a single bit set representing its
 * logical depth. Within a single block at a given level, the indent depth must
 * be consistent.
 *
 * The caller is expected to figure out the acceptable levels of a line based
 * on its content (and perhaps the current level), and call @ref Indent::check
 * to identify which actual level to select.
 *
 * For a line that introduces a new, more deeply-indented, section, it should
 * also call @ref Indent::introduce, to indicate the new level. This must be
 * greater than the level returned by @ref Indent::check.
 */
class Indent {
public:
  /** @brief Constructor */
  Indent() {
    clear();
  }

  /** @brief Clear state */
  void clear();

  /** @brief Check whether a line is acceptable
   * @param acceptable_levels Bitmap of acceptable levels for line
   * @param indent Actual indent depth
   * @return Actual level or 0 on error
   *
   * This should be called for each nonempty line.
   */
  unsigned check(unsigned acceptable_levels, size_t indent);

  /** @brief Introduce a new section
   * @param new_level Single-bit bitmap of new level
   *
   * This should be called for lines that introduce a new section.  The
   * expected indentation will be taken from the next line passed to @ref
   * check().
   */
  void introduce(unsigned new_level) {
    this->new_level = new_level;
  }

private:
  /** @brief Indent level stack */
  std::vector<size_t> stack;

  /** @brief Level for next line */
  unsigned new_level;
};

#endif /* INDENT_H */
