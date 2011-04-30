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
#include "rsbackup.h"
#include "Conf.h"
#include "Command.h"
#include "Utils.h"
#include "IO.h"
#include "Retire.h"
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
  // Find all the logfiles for this device.  Retirement looks at the directory
  // not the recorded state so that it can cope with the device already being
  // deconfigured.
  Directory d;
  std::string f;
  std::vector<std::string> obsoleteLogs;
  d.open(config.logs);
  while(d.get(f)) {
    if(!Conf::logfileRegexp.matches(f))
      continue;
    if(Conf::logfileRegexp.sub(2) == deviceName)
      obsoleteLogs.push_back(f);
  }
  // Remove them
  if(command.verbose)
    IO::out.writef("INFO: removing %zu logfiles for device '%s'\n",
                   obsoleteLogs.size(), deviceName.c_str());
  removeObsoleteLogs(obsoleteLogs, false);
}

void retireDevices() {
  for(size_t n = 0; n < command.devices.size(); ++n)
    retireDevice(command.devices[n]);
}
