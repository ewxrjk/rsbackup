// Copyright © 2011, 2014 Richard Kettlewell.
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
#include "rsbackup.h"
#include "Conf.h"
#include "Command.h"
#include "Utils.h"
#include "IO.h"
#include "Database.h"
#include <cerrno>
#include <cstring>

static void retireDevice(const std::string &deviceName) {
  // If the device is still configured, check whether it should really be
  // retired
  Device *device = config.findDevice(deviceName);
  if(device) {
    if(!check("Really retire device '%s'?", deviceName.c_str()))
      return;
  }
  // Remove all the log records for this device.
  if(command.act) {
    config.getdb()->begin();
    Database::Statement(config.getdb(),
                        "DELETE FROM backup"
                        " WHERE device=?",
                        SQL_STRING, &deviceName,
                        SQL_END).next();
    config.getdb()->commit();
  }
}

void retireDevices() {
  for(size_t n = 0; n < command.devices.size(); ++n)
    retireDevice(command.devices[n]);
}
