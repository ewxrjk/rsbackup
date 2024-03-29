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
#include "Conf.h"
#include "Backup.h"
#include "Volume.h"
#include "Device.h"
#include "BackupPolicy.h"

/** @brief The @c daily backup policy; backups are made at most once per day. */
class BackupPolicyDaily: public BackupPolicy {
public:
  BackupPolicyDaily(): BackupPolicy("daily") {}

  void validate(const Volume *) const override {}

  bool backup(const Volume *volume, const Device *device) const override {
    Date today = Date::today("BACKUP");
    for(const Backup *backup: volume->backups)
      if(backup->getStatus() == COMPLETE && Date(backup->time) == today
         && backup->deviceName == device->name)
        return false;
    return true;
  }

} backup_daily;
