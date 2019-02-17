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
#include "Backup.h"
#include "PrunePolicy.h"
#include "Utils.h"
#include <sstream>

/** @brief The @c age pruning policy */
class PruneAge: public PrunePolicy {
public:
  PruneAge(): PrunePolicy("age") {}

  void validate(const Volume *volume) const override {
    parseInteger(get(volume, "prune-age", DEFAULT_PRUNE_AGE), 1,
                 std::numeric_limits<int>::max());
    parseInteger(get(volume, "min-backups", DEFAULT_MIN_BACKUPS), 1,
                 std::numeric_limits<int>::max());
  }

  void prunable(std::vector<Backup *> &onDevice,
                std::map<Backup *, std::string> &prune,
                int) const override {
    const Volume *volume = onDevice.at(0)->volume;
    int pruneAge = parseInteger(get(volume, "prune-age", DEFAULT_PRUNE_AGE),
                                1, std::numeric_limits<int>::max());
    int minBackups = parseInteger(get(volume, "min-backups",
                                      DEFAULT_MIN_BACKUPS),
                                  1, std::numeric_limits<int>::max());
    size_t left = onDevice.size();
    for(Backup *backup: onDevice) {
      int age = Date::today() - Date(backup->time);
      // Keep backups that are young enough
      if(age <= pruneAge)
        continue;
      // Keep backups that are on underpopulated devices
      if(left <= static_cast<unsigned>(minBackups))
        continue;
      std::ostringstream ss;
      ss << "age " << age
         << " > " << pruneAge
         << " and remaining " << onDevice.size()
         << " > " << minBackups;
      prune[backup] = ss.str();
      --left;
    }
  }
} prune_age;
