//-*-C++-*-
// Copyright © 2015, 2016 Richard Kettlewell.
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
 *
 * Bulk removal happens in @ref pruneBackups and @ref retireVolumes.
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
   * @param name Action name
   */
  BulkRemove(const std::string &name): Subprocess(name) {}

  /** @brief Constructor
   * @param name Action name
   * @param path Base path to remove
   *
   * The effect is equivalent to @c rm @c -rf.
   */
  BulkRemove(const std::string &name, const std::string &path):
      Subprocess(name) {
    initialize(path);
  }

  /** @brief Initialize the bulk remover
   * @param path Base path to remove
   *
   * The effect is equivalent to @c rm @c -rf.
   */
  void initialize(const std::string &path);
};

#endif /* BULKREMOVE_H */
