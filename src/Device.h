// -*-C++-*-
// Copyright © 2011, 2012, 2014-2016 Richard Kettlewell.
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
#ifndef DEVICE_H
#define DEVICE_H
/** @file Device.h
 * @brief Configuration and state of a device
 */

#include <string>

class Store;

/** @brief Represents a backup device */
class Device {
public:
  /** @brief Constructor
   * @param name_ Name of device
   */
  Device(const std::string &name_): name(name_) {}

  /** @brief Name of device */
  std::string name;

  /** @brief Store for this device, or null pointer
   *
   * Set by Store::identify().
   */
  Store *store = nullptr;

  /** @brief Validity test for device names
   * @param n Name of device
   * @return true if @p n is a valid device name, else false
   */
  static bool valid(const std::string &n);
};

#endif /* DEVICE_H */
