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
#include "PrunePolicy.h"
#include "Utils.h"
#include "Errors.h"
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
    PolicyParameter decayStart =
        get(volume, "decay-start", DEFAULT_DECAY_START);
    PolicyParameter decayWindow =
        get(volume, "decay-window", DEFAULT_DECAY_WINDOW);
    PolicyParameter decayScale =
        get(volume, "decay-scale", DEFAULT_DECAY_SCALE);
    PolicyParameter decayLimit = get(volume, "decay-limit", DEFAULT_PRUNE_AGE);
    try {
      if(parseTimeInterval(decayStart.value) < 1)
        throw SyntaxError("decay-start too small");
    } catch(SyntaxError &e) {
      throw ConfigError(decayStart.location, e.what());
    }
    try {
      if(parseTimeInterval(decayWindow.value) < 1)
        throw SyntaxError("decay-window too small");
    } catch(SyntaxError &e) {
      throw ConfigError(decayWindow.location, e.what());
    }
    try {
      parseFloat(decayScale.value, 1, std::numeric_limits<double>::max(),
                 ExclusiveLimit);
    } catch(SyntaxError &e) {
      throw ConfigError(decayScale.location, e.what());
    }
    try {
      if(parseTimeInterval(decayLimit.value) < 1)
        throw SyntaxError("decay-limit too small");
    } catch(SyntaxError &e) {
      throw ConfigError(decayLimit.location, e.what());
    }
  }

  void prunable(std::vector<Backup *> &onDevice,
                std::map<Backup *, std::string> &prune, int) const override {
    const Volume *volume = onDevice.at(0)->volume;
    int decayStart =
        parseTimeInterval(get(volume, "decay-start", DEFAULT_DECAY_START).value)
        / 86400;
    int decayWindow =
        parseTimeInterval(
            get(volume, "decay-window", DEFAULT_DECAY_WINDOW).value)
        / 86400;
    double decayScale =
        parseFloat(get(volume, "decay-scale", DEFAULT_DECAY_SCALE).value, 1,
                   std::numeric_limits<double>::max(), ExclusiveLimit);
    int decayLimit =
        parseTimeInterval(get(volume, "decay-limit", DEFAULT_PRUNE_AGE).value)
        / 86400;
    if(onDevice.size() == 1)
      return;
    // Map of bucket numbers to oldest backup in the bucket.  These will be
    // preserved.
    std::map<int, const Backup *> oldest;
    for(Backup *backup: onDevice) {
      int age = Date::today("PRUNE") - Date(backup->time);
      // Keep backups that are young enough
      int a = age - decayStart;
      if(a <= 0)
        continue;
      // Prune backups that are much too old
      if(age > decayLimit) {
        std::ostringstream ss;
        ss << "age " << age << " > " << decayLimit
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
      int age = Date::today("PRUNE") - Date(backup->time);
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
        ss << "age " << age << " > " << decayStart << " and oldest in bucket "
           << bucket;
        prune[backup] = ss.str();
      }
    }
  }
} prune_decay;
