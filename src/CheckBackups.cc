// Copyright © Richard Kettlewell.
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
#include "Command.h"
#include "Conf.h"
#include "Device.h"
#include "Errors.h"
#include "Host.h"
#include "IO.h"
#include "Store.h"
#include "Utils.h"
#include "Volume.h"

static void checkDirectory(std::vector<std::string> &unexpected,
                           const std::string &root,
                           const std::set<std::string> &expected) {
  // Enumerate files
  Directory d;
  std::string f;
  std::set<std::string> files;
  try {
    d.open(root);
  } catch(IOError &e) {
    return;
  }
  while(d.get(f)) {
    if(f == "." || f == "..")
      continue;
    if(expected.find(f) != expected.end())
      continue;
    unexpected.push_back(root + "/" + f);
  }
}

static void checkTopLevelFiles(std::vector<std::string> &unexpected,
                               const Device *device) {
  std::set<std::string> expected;
  expected.insert("device-id");
  expected.insert("lost+found");
  for(auto h: globalConfig.hosts) {
    expected.insert(h.first);
  }
  checkDirectory(unexpected, device->store->path, expected);
}

static void checkVolume(std::vector<std::string> &unexpected,
                        const Device *device, const Volume *volume) {
  std::set<std::string> expected;
  for(auto backup: volume->backups) {
    if(backup->getDevice() == device) {
      expected.insert(backup->id);
      if(backup->getStatus() != COMPLETE) {
        expected.insert(backup->id + ".incomplete");
      }
    }
  }
  checkDirectory(unexpected,
                 device->store->path + "/" + volume->parent->name + "/"
                     + volume->name,
                 expected);
}

static void checkHost(std::vector<std::string> &unexpected,
                      const Device *device, const Host *host) {
  std::set<std::string> expected;
  for(auto v: host->volumes) {
    expected.insert(v.first);
  }
  checkDirectory(unexpected, device->store->path + "/" + host->name, expected);
  for(auto v: host->volumes) {
    Volume *volume = v.second;
    checkVolume(unexpected, device, volume);
  }
}

static void checkDevice(std::vector<std::string> &unexpected,
                        const Device *device) {
  checkTopLevelFiles(unexpected, device);
  for(auto h: globalConfig.hosts) {
    const Host *host = h.second;
    checkHost(unexpected, device, host);
  }
}

void checkUnexpected() {
  // Load up log files
  globalConfig.readState();
  // Find the devices to scan
  globalConfig.identifyDevices(Store::Enabled);
  std::vector<std::string> unexpected;
  for(auto d: globalConfig.devices) {
    const Device *device = d.second;
    if(device->store)
      checkDevice(unexpected, device);
  }
  if(unexpected.size()) {
    warning(WARNING_ALWAYS, "%zu unexpected files found", unexpected.size());
    for(auto it: unexpected) {
      IO::out.writef("%s%c", it.c_str(), globalCommand.eol);
    }
  }
}
