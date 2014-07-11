// -*-C++-*-
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
#ifndef STORE_H
#define STORE_H
/** @file Store.h
 * @brief Store (mount point) support
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
   */
  Store(const std::string &path_): path(path_),
                                   device(NULL),
                                   state(Enabled) {
  }

  /** @brief Possible states */
  enum State {
    /** @brief A disabled store */
    Disabled,

    /** @brief An enabled store */
    Enabled,
  };

  /** @param Location of store */
  std::string path;

  /** @param Device mounted at this store
   * 
   * Set to NULL before checking, or if no device is mounted here
   */
  Device *device;                       // device for this, or NULL

  /** @brief State of this store */
  State state;

  /** @brief Identify the device mounted here
   * @throw BadStore
   * @throw FatalStoreError
   * @throw UnavailableStore
   */
  void identify();
};

#endif /* STORE_H */
