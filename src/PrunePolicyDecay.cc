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

// See also https://www.greenend.org.uk/rjk/rsbackup/decay.pdf

int prune_decay_bucket(double w, double s, int a) {
  return ceil(logbase((s - 1) * a / w + 1, s)) - 1;
}

/** @brief The @c decay pruning policy */
class PruneDecay: public PrunePolicy {
public:
  PruneDecay(): PrunePolicy("decay") {}

  void validate(const Volume *volume) const override {
    parseInteger(get(volume, "decay-start", DEFAULT_DECAY_START),
                 1, std::numeric_limits<int>::max());
    parseInteger(get(volume, "decay-window", DEFAULT_DECAY_WINDOW),
                 1, std::numeric_limits<int>::max());
    parseInteger(get(volume, "decay-scale", DEFAULT_DECAY_SCALE),
                 2, std::numeric_limits<int>::max());
    parseInteger(get(volume, "decay-limit", DEFAULT_PRUNE_AGE),
                 1, std::numeric_limits<int>::max());
  }

  void prunable(std::vector<Backup *> &onDevice,
                std::map<Backup *, std::string> &prune,
                int) const override {
    const Volume *volume = onDevice.at(0)->volume;
    int decayStart = parseInteger(get(volume, "decay-start", DEFAULT_DECAY_START),
                                  1, std::numeric_limits<int>::max());
    int decayWindow = parseInteger(get(volume, "decay-window", DEFAULT_DECAY_WINDOW),
                                  1, std::numeric_limits<int>::max());
    int decayScale = parseInteger(get(volume, "decay-scale", DEFAULT_DECAY_SCALE),
                                  2, std::numeric_limits<int>::max());
    int decayLimit = parseInteger(get(volume, "decay-limit", DEFAULT_PRUNE_AGE),
                                  1, std::numeric_limits<int>::max());
    if(onDevice.size() == 1)
      return;
    // Map of bucket numbers to oldest backup in the bucket.  These will be
    // preserved.
    std::map<int, const Backup *> oldest;
    for(Backup *backup: onDevice) {
      int age = Date::today() - Date(backup->time);
      // Keep backups that are young enough
      int a = age - decayStart;
      if(a <= 0)
        continue;
      // Prune backups that are much too old
      if(age > decayLimit) {
        std::ostringstream ss;
        ss << "age " << age
           << " > " << decayLimit
           << " and other backups exist";
        prune[backup] = ss.str();
        continue;
      }
      // Assign backups to buckets
      int bucket = prune_decay_bucket(decayWindow, decayScale, a);
      // Track the oldest backup in this bucket
      auto bucket_iterator = oldest.find(bucket);
      if(bucket_iterator == oldest.end()
         || backup->time < bucket_iterator->second->time)
        oldest[bucket] = backup;
    }
    // Now that we know what the oldest backup in each bucket is, we can prune
    // the rest.
    for(Backup *backup: onDevice) {
      int age = Date::today() - Date(backup->time);
      // Keep backups that are young enough
      int a = age - decayStart;
      if(a <= 0 || age > decayLimit)
        continue;
      int bucket = prune_decay_bucket(decayWindow, decayScale, a);
      auto bucket_iterator = oldest.find(bucket);
      assert(bucket_iterator != oldest.end());
      const Backup *oldest_in_this_bucket = bucket_iterator->second;
      if(backup != oldest_in_this_bucket) {
        std::ostringstream ss;
        ss << "age " << age
           << " > " << decayStart
           << " and oldest in bucket " << bucket;
        prune[backup] = ss.str();
      }
    }
  }
} prune_decay;
