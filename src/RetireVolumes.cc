// Copyright © 2011, 2013, 2014, 2015 Richard Kettlewell.
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
  if(warning_mask & WARNING_VERBOSE)
    IO::out.writef("INFO: removing %s\n", path.c_str());
  if(command.act && rmdir(path.c_str()) < 0 && errno != ENOENT) {
    error("removing %s: %s", path.c_str(), strerror(errno));
  }
}

/** @brief One retirable backup  */
struct Retirable {
  /** @brief Host name */
  std::string hostName;

  /** @brief Volume name */
  std::string volumeName;

  /** @brief Pointer to containing device */
  Device *device;

  /** @brief Backup ID */
  std::string id;

  /** @brief Bulk removal */
  BulkRemove *b = nullptr;

  /** @brief Constructor
   * @param h Host name
   * @param v Volume name
   * @param d Pointer to containing device
   * @param i Backup ID
   */
  inline Retirable(const std::string &h, const std::string &v, Device *d, std::string i):
    hostName(h), volumeName(v), device(d), id(i) {
  }

  /** @brief Destructor */
  ~Retirable() {
    delete b;
  }

  /** @brief Schedule retire of this backup */
  void scheduleRetire(ActionList &al) {
    assert(!b);
    const std::string backupPath = (device->store->path
                                    + PATH_SEP + hostName
                                    + PATH_SEP + volumeName
                                    + PATH_SEP + id);
    if(warning_mask & WARNING_VERBOSE)
      IO::out.writef("INFO: removing %s\n", backupPath.c_str());
    if(command.act) {
      b = new BulkRemove(backupPath);
      b->uses(device->name);
      al.add(b);
    }
  }

  /** @brief Clean up after retire of this backup */
  void retired() {
    const std::string backupPath = (device->store->path
                                    + PATH_SEP + hostName
                                    + PATH_SEP + volumeName
                                    + PATH_SEP + id);
    if(command.act) {
      if(b->getStatus()) {
        error("removing %s: %s",
              backupPath.c_str(),
              SubprocessFailed::format("rm", b->getStatus()).c_str());
        return;
      }
      // Remove incomplete indicator
      std::string incompletePath = backupPath + ".incomplete";
      if(unlink(incompletePath.c_str()) < 0 && errno != ENOENT) {
        error("removing %s", incompletePath.c_str(), strerror(errno));
        return;
      }
      // Update database
      Database::Statement(config.getdb(),
                          "DELETE FROM backup"
                          " WHERE host=? AND volume=? AND device=? AND id=?",
                          SQL_STRING, &hostName,
                          SQL_STRING, &volumeName,
                          SQL_STRING, &device->name,
                          SQL_STRING, &id,
                          SQL_END).next();
    }
  }
};

// Schedule the retire of one volume or host
static void identifyVolumes(std::vector<Retirable> &retire,
                            std::set<std::string> &volume_directories,
                            std::set<std::string> &host_directories,
                            const std::string &hostName,
                            const std::string &volumeName) {
  // Verify action with user
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
      if(device->store->state != Store::Enabled) {
        error("backup on disabled device %s",
              deviceName.c_str());
        continue;
      }
      retire.push_back(Retirable(hostName,
                                 stmt.get_string(0),
                                 device,
                                 stmt.get_string(2)));
      // Zap all retired volume directories
      volume_directories.insert(device->store->path
                                + PATH_SEP + hostName
                                + PATH_SEP + stmt.get_string(0));
      // ...and host directories if the whole host is being retired.
      if(volumeName == "*")
        host_directories.insert(device->store->path
                                + PATH_SEP + hostName);
    }
  }
}

void retireVolumes() {
  // Sanity-check command
  for(auto &selection: command.selections)
    if(selection.sense == false)
      throw CommandError("cannot use negative selections with --retire");
  // Identify backups to retire and directories to remove
  std::vector<Retirable> retire;
  std::set<std::string> volume_directories, host_directories;
  for(auto &selection: command.selections) {
    if(selection.host == "*")
      throw CommandError("cannot retire all hosts");
    identifyVolumes(retire, volume_directories, host_directories,
                    selection.host, selection.volume);
  }
  // Schedule removal
  EventLoop e;
  ActionList al(&e);
  for(Retirable &r: retire)
    r.scheduleRetire(al);
  // Perform removal
  al.go();
  // Clean up .incomplete files and db after removal
  for(Retirable &r: retire)
    r.retired();
  // Clean up redundant directories
  for(auto &d: volume_directories)
    removeDirectory(d);
  for(auto &d: host_directories)
    removeDirectory(d);
}
