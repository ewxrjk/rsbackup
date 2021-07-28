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
#include "Backup.h"
#include "Command.h"
#include "Conf.h"
#include "Store.h"
#include "Utils.h"
#include "Volume.h"
#include "Device.h"
#include "IO.h"

void findLatest() {
  // We will accumulate paths and release output only in non-error cases.
  std::vector<std::string> paths;
  // Make sure all state is available
  globalConfig.readState();
  globalConfig.identifyDevices(Store::Enabled);
  for(auto &s: globalCommand.selections) {
    const Volume *volume = globalConfig.findVolume(s.host, s.volume);
    if(!volume) {
      error("unrecognized volume %s:%s", s.host.c_str(), s.volume.c_str());
      continue;
    }
    const Backup *backup = nullptr;
    for(auto &b: volume->backups) {
      // Only want complete backups
      if(b->getStatus() != COMPLETE)
        continue;
      // Want the latest backup
      if(backup != nullptr && backup->time > b->time)
        continue;
      // Only want available backups
      const Device *device = b->getDevice();
      if(device == nullptr)
        continue;
      const Store *store = device->store;
      if(store == nullptr)
        continue;
      backup = b;
    }
    if(!backup) {
      error("no backup found for %s:%s", s.host.c_str(), s.volume.c_str());
      continue;
    }
    paths.push_back(backup->backupPath());
  }
  if(globalErrors)
    return;
  for(const auto &path: paths)
    IO::out.writef("%s%c", path.c_str(), globalCommand.eol);
  IO::out.flush();
}
