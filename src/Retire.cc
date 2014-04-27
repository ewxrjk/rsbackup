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
#include <config.h>
#include "rsbackup.h"
#include "Conf.h"
#include "Command.h"
#include "Store.h"
#include "Utils.h"
#include "Errors.h"
#include "IO.h"
#include <cerrno>

static void removeObsoleteLog(const std::string &f,
                              bool removeBackup) {
  if(removeBackup) {
    if(!Conf::logfileRegexp.matches(f))
      return;                           // should never happen
    const std::string date = Conf::logfileRegexp.sub(1);
    const std::string deviceName = Conf::logfileRegexp.sub(2);
    const std::string hostName = Conf::logfileRegexp.sub(3);
    const std::string volumeName = Conf::logfileRegexp.sub(4);
    Device *device = config.findDevice(deviceName);
    if(!device) {
      // User should use --retire-device instead
      error("backup %s is on unknown device %s",
            f.c_str(), deviceName.c_str());
      return;
    }
    if(!device->store) {
      error("backup %s is on unavailable device %s",
            f.c_str(), deviceName.c_str());
      return;
    }
    const std::string backupPath = (device->store->path
                                    + PATH_SEP + hostName
                                    + PATH_SEP + volumeName
                                    + PATH_SEP + date);
    if(command.verbose)
      IO::out.writef("INFO: removing %s\n", backupPath.c_str());
    try {
      BulkRemove(backupPath);
    } catch(SubprocessFailed &exception) {
      error("removing %s: %s",
            backupPath.c_str(), exception.what());
      // Leave logfile in place for another go
      return;
    }
    if(command.act) {
      std::string incompletePath = backupPath + ".incomplete";
      if(unlink(incompletePath.c_str()) < 0 && errno != ENOENT) {
        error("removing %s", incompletePath.c_str(), strerror(errno));
        return;
      }
    }
  }
  const std::string path = config.logs + PATH_SEP + f;
  if(command.verbose)
    IO::out.writef("INFO: removing %s\n", path.c_str());
  if(command.act && unlink(path.c_str()) < 0) {
    error("removing %s: %s", path.c_str(), strerror(errno));
  }
}

void removeObsoleteLogs(const std::vector<std::string> &obsoleteLogs,
                        bool removeBackup) {
  if(removeBackup)
    config.identifyDevices();
  for(size_t n = 0; n < obsoleteLogs.size(); ++n)
    removeObsoleteLog(obsoleteLogs[n], removeBackup);
}
