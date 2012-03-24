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
#include "Store.h"
#include "Command.h"
#include "Utils.h"
#include "Errors.h"
#include "IO.h"
#include "Retire.h"
#include <cerrno>

static void removeDirectory(const std::string &path) {
  if(command.act && rmdir(path.c_str()) < 0 && errno != ENOENT) {
    IO::err.writef("WARNING: removing %s: %s\n",
                   path.c_str(), strerror(errno));
    ++errors;
  }
}

static void removeVolumeSubdirectories(Device *device,
                                       const std::string &hostName) {
  const std::string hostPath = (device->store->path
                                + PATH_SEP + hostName);
  Directory d;
  d.open(hostPath);
  std::string f;
  std::vector<std::string> files;
  while(d.get(f)) {
    if(f != "." && f != "..")
      files.push_back(f);
  }
  for(size_t n = 0; n < files.size(); ++n)
    removeDirectory(hostPath + PATH_SEP + files[n]);
}

// Retire one volume or host
static void retireVolume(const std::string &hostName,
                         const std::string &volumeName) {
  if(volumeName == "*") {
    if(config.findHost(hostName)) {
      if(!check("Really retire host '%s'?",
                hostName.c_str()))
        return;
    }
  } else {
    if(config.findVolume(hostName, volumeName)) {
      if(!check("Really retire volume '%s:%s'?",
                hostName.c_str(), volumeName.c_str()))
        return;
    }
  }
  // Accumulate a list of logs to remove.  Retirement looks at the directory
  // not the recorded state so that it can cope with the volume already being
  // deconfigured.
  Directory d;
  std::string f;
  std::vector<std::string> obsoleteLogs;
  d.open(config.logs);
  while(d.get(f)) {
    if(!Conf::logfileRegexp.matches(f))
      continue;
    if(Conf::logfileRegexp.sub(3) == hostName
       && (volumeName == "*" || Conf::logfileRegexp.sub(4) == volumeName))
      obsoleteLogs.push_back(f);
  }
  // Remove them
  removeObsoleteLogs(obsoleteLogs, true);
  // Clean up empty directories too.
  for(devices_type::iterator devicesIterator = config.devices.begin();
      devicesIterator != config.devices.end();
      ++devicesIterator) {
    Device *device = devicesIterator->second;
    if(!device->store)
      continue;
    if(volumeName == "*") {
      removeVolumeSubdirectories(device, hostName);
      removeDirectory(device->store->path + PATH_SEP + hostName);
    } else {
      removeDirectory(device->store->path
                      + PATH_SEP + hostName
                      + PATH_SEP + volumeName);
    }
  }
}

void retireVolumes() {
  for(size_t n = 0; n < command.selections.size(); ++n) {
    if(command.selections[n].sense == false)
      throw CommandError("cannot use negative selections with --retire");
  }
  for(size_t n = 0; n < command.selections.size(); ++n) {
    if(command.selections[n].host == "*")
      throw CommandError("cannot retire all hosts");
    retireVolume(command.selections[n].host, command.selections[n].volume);
  }
}
