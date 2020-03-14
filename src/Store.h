// -*-C++-*-
// Copyright © 2011, 2012, 2014, 2015, 2018 Richard Kettlewell.
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
#ifndef STORE_H
#define STORE_H
/** @file Store.h
 * @brief %Store (mount point) support
 */

#include <string>

class Device;

/** @brief Represents a store
 *
 * A store is a path at which a backup device may be mounted.
 */
class Store {
public:
  /** @brief Constructor
   * @param path_ Location of store
   * @param mounted_ True if path must be a mount point
   */
  Store(const std::string &path_, bool mounted_):
      path(path_), mounted(mounted_) {}

  /** @brief Possible states */
  enum State {
    /** @brief A disabled store */
    Disabled = 1,

    /** @brief An enabled store */
    Enabled = 2,
  };

  /** @brief Location of store */
  std::string path;

  /** @brief True if path must be a mount point */
  bool mounted;

  /** @param Device mounted at this store
   *
   * Set to null pointer before checking, or if no device is mounted here
   */
  Device *device = nullptr;

  /** @brief State of this store */
  State state = Enabled;

  /** @brief Identify the device mounted here
   * @throw BadStore
   * @throw FatalStoreError
   * @throw UnavailableStore
   */
  void identify();
};

#endif /* STORE_H */
