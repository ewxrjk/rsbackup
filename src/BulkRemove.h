//-*-C++-*-
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
#ifndef BULKREMOVE_H
#define BULKREMOVE_H
/** @file BulkRemove.h
 * @brief Bulk remove operations
 */

#include "Subprocess.h"

/** @brief Bulk remove files and directories, as if by @c rm @c -rf.
 *
 * A @ref BulkRemove is a @ref Subprocess and therefore an @ref Action; it can
 * be invoked either with BulkRemove::runAndWait or as part of an @ref
 * ActionList.
 */
class BulkRemove: public Subprocess {
public:
  /** @brief Constructor
   * @param path Base path to remove
   *
   * The effect is equivalent to @c rm @c -rf.
   */
  BulkRemove(const std::string &path) {
    initialize(path);
  }

  /** @brief Constructor
   * @param path Base path to remove
   *
   * The effect is equivalent to @c rm @c -rf.
   */
  BulkRemove() = default;

  /** @brief Initialize the bulk remover
   * @param path Base path to remove
   *
   * The effect is equivalent to @c rm @c -rf.
   */
  void initialize(const std::string &path);

};

#endif /* BULKREMOVE_H */
