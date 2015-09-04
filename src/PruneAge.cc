// Copyright © 2015 Richard Kettlewell.
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
#include "Prune.h"
#include "Utils.h"
#include <sstream>

/** @brief The @c age pruning policy */
class PruneAge: public PrunePolicy {
public:
  PruneAge(): PrunePolicy("age") {}

  void validate(const Volume *volume) const {
    parseInteger(get(volume, "prune-age", DEFAULT_PRUNE_AGE), 1);
    parseInteger(get(volume, "min-backups", DEFAULT_MIN_BACKUPS), 1);
  }

  bool prunable(const Backup *backup,
                std::vector<const Backup *> &onDevice,
                int,
                std::string &reason) const {
    const Volume *volume = backup->volume;
    int pruneAge = parseInteger(get(volume, "prune-age", DEFAULT_PRUNE_AGE),
                                1);
    int minBackups = parseInteger(get(volume, "min-backups", DEFAULT_MIN_BACKUPS),
                                  1);
    int age = Date::today() - backup->date;
    // Keep backups that are young enough
    if(age <= pruneAge)
      return false;
    // Keep backups that are on underpopulated devices
    if(onDevice.size() <= static_cast<unsigned>(minBackups))
      return false;
    std::ostringstream ss;
    ss << "age " << age
       << " > " << pruneAge
       << " and remaining " << onDevice.size()
       << " > " << minBackups;
    reason = ss.str();
    return true;
  }
} prune_age;
