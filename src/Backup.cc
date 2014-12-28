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
#include <config.h>
#include "Conf.h"
#include "Store.h"

// Return the path to this backup
std::string Backup::backupPath() const {
  const Host *host = volume->parent;
  const Device *device = host->parent->findDevice(deviceName);
  const Store *store = device->store;
  return (store->path
          + PATH_SEP + host->name
          + PATH_SEP + volume->name
          + PATH_SEP + date.toString());
}

// Return the path to the logfile for this backup
std::string Backup::logPath() const {
  const Host *host = volume->parent;
  const Device *device = host->parent->findDevice(deviceName);
  return (host->parent->logs
          + PATH_SEP
          + date.toString()
          + "-" + device->name
          + "-" + host->name
          + "-" + volume->name
          + ".log");
}
