// Copyright © 2011, 2013, 2014 Richard Kettlewell.
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
#include "Database.h"
#include "BulkRemove.h"
#include <cerrno>

static void removeDirectory(const std::string &path) {
  if(command.act && rmdir(path.c_str()) < 0 && errno != ENOENT) {
    error("removing %s: %s", path.c_str(), strerror(errno));
  }
}

// Remove all volume directories for a host
static void removeVolumeSubdirectories(Device *device,
                                       const std::string &hostName) {
  const std::string hostPath = (device->store->path
                                + PATH_SEP + hostName);
  try {
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
  } catch(IOError &e) {
    if(e.errno_value == ENOENT) {
      IO::err.writef("INFO: %s: already removed\n", hostPath.c_str());
      return;
    }
    throw;
  }
}

/** @brief One retirable backup  */
struct Retirable {
  /** @param Volume name */
  std::string volumeName;

  /** @param Pointer to containing device */
  Device *device;

  /** @param Backup ID */
  std::string id;

  /** @brief Constructor
   * @param v Volume name
   * @param d Pointer to containing device
   * @param i Backup ID
   */
  inline Retirable(std::string v, Device *d, std::string i):
    volumeName(v), device(d), id(i) {
  }
};

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
  // Find all the backups to retire
  std::vector<Retirable> retire;
  {
    Database::Statement stmt(config.getdb());
    if(volumeName == "*")
      stmt.prepare("SELECT volume,device,id FROM backup"
                   " WHERE host=?",
                   SQL_STRING, &hostName,
                   SQL_END);
    else
      stmt.prepare("SELECT volume,device,id FROM backup"
                   " WHERE host=? AND volume=?",
                   SQL_STRING, &hostName,
                   SQL_STRING, &volumeName,
                   SQL_END);
    while(stmt.next()) {
      std::string deviceName = stmt.get_string(1);
      config.identifyDevices(Store::Enabled);
      Device *device = config.findDevice(deviceName);
      if(!device) {
        // User should use --retire-device instead
        error("backup on unknown device %s (use --retire-device)",
              deviceName.c_str());
        continue;
      }
      if(!device->store) {
        error("backup on unavailable device %s",
              deviceName.c_str());
        continue;
      }
      retire.push_back(Retirable(stmt.get_string(0),
                                 device,
                                 stmt.get_string(2)));
    }
  }
  // Retire the ones that we can
  for(std::vector<Retirable>::const_iterator it = retire.begin();
      it != retire.end(); ++it) {
    const Retirable &r = *it;
    const std::string backupPath = (r.device->store->path
                                    + PATH_SEP + hostName
                                    + PATH_SEP + r.volumeName
                                    + PATH_SEP + r.id);
    if(command.verbose)
      IO::out.writef("INFO: removing %s\n", backupPath.c_str());
    try {
      BulkRemove b(backupPath);
      if(command.act)
        b.runAndWait();
    } catch(SubprocessFailed &exception) {
      error("removing %s: %s",
            backupPath.c_str(), exception.what());
      // Leave log in place for another go
      continue;
    }
    if(command.act) {
      std::string incompletePath = backupPath + ".incomplete";
      if(unlink(incompletePath.c_str()) < 0 && errno != ENOENT) {
        error("removing %s", incompletePath.c_str(), strerror(errno));
        continue;
      }
      Database::Statement(config.getdb(),
                          "DELETE FROM backup"
                          " WHERE host=? AND volume=? AND device=? AND id=?",
                          SQL_STRING, &hostName,
                          SQL_STRING, &r.volumeName,
                          SQL_STRING, &r.device->name,
                          SQL_STRING, &r.id,
                          SQL_END).next();
    }
  }

  // Clean up empty directories too.
  for(devices_type::iterator devicesIterator = config.devices.begin();
      devicesIterator != config.devices.end();
      ++devicesIterator) {
    Device *device = devicesIterator->second;
    if(!device->store || device->store->state != Store::Enabled)
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
