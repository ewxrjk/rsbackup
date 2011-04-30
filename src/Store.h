// -*-C++-*-
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
#ifndef STORE_H
#define STORE_H

#include <string>

class Device;

// Represents a store, i.e. a path at which a backup device may be mounted.
class Store {
public:
  Store(const std::string &path_): path(path_), device(NULL) {}
  std::string path;
  Device *device;                       // device for this, or NULL

  void identify();                      // identify device
};

#endif /* STORE_H */
