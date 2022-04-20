// Copyright © 2019 Richard Kettlewell.
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
#include "Backup.h"
#include "Volume.h"
#include "Device.h"
#include "Utils.h"
#include "Errors.h"
#include "BackupPolicy.h"

/** @brief The @c interval backup policy; backups are separate by a configurable
 * minimum interval. */
class BackupPolicyInterval: public BackupPolicy {
public:
  BackupPolicyInterval(): BackupPolicy("interval") {}

  void validate(const Volume *volume) const override {
    if(parseTimeInterval(get(volume, "min-interval"),
                         std::numeric_limits<int>::max())
       < 1)
      throw SyntaxError("min-interval too small");
  }

  bool backup(const Volume *volume, const Device *device) const override {
    time_t now = Date::now();
    int minInterval = parseTimeInterval(get(volume, "min-interval"),
                                        std::numeric_limits<int>::max());
    for(const Backup *backup: volume->backups)
      if(backup->getStatus() == COMPLETE && now - backup->time < minInterval
         && backup->deviceName == device->name)
        return false;
    return true;
  }

} backup_interval;
