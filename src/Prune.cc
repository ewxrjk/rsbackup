// Copyright © 2011, 2012, 2014, 2015 Richard Kettlewell.
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
#include "Command.h"
#include "Errors.h"
#include "IO.h"
#include "Utils.h"
#include "Store.h"
#include "Database.h"
#include "Prune.h"
#include "BulkRemove.h"
#include <algorithm>
#include <regex>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>

/** @brief A removable backup */
class RemovableBackup {
public:
  /** @brief Constructor
   * @param b Backup to remove
   */
  RemovableBackup(Backup *b): backup(b) {}

  /** @brief Initialize the @ref BulkRemove instance */
  void initialize() {
    bulkRemover.initialize(backup->backupPath());
    bulkRemover.uses(backup->deviceName);
  }

  /** @brief The backup to remove */
  Backup *backup;

  /** @brief The bulk remove instance for this backup */
  BulkRemove bulkRemover;
};

static void findObsoleteBackups(std::vector<Backup *> &obsoleteBackups);
static void markObsoleteBackups(std::vector<Backup *> obsoleteBackups);
static void findRemovableBackups(std::vector<Backup *> obsoleteBackups,
                                 std::vector<RemovableBackup> &removableBackups);
static void checkRemovalErrors(std::vector<RemovableBackup> &removableBackups);
static void commitRemovals(std::vector<RemovableBackup> &removableBackups);

PrunePolicy::PrunePolicy(const std::string &name) {
  if(!policies)
    policies = new policies_type();
  (*policies)[name] = this;
}

const std::string &PrunePolicy::get(const Volume *volume,
                                    const std::string &name) const {
  auto it = volume->pruneParameters.find(name);
  if(it != volume->pruneParameters.end())
    return it->second;
  else
    throw ConfigError("missing pruning parameter '" + name + "'");
}

const std::string &PrunePolicy::get(const Volume *volume,
                                    const std::string &name,
                                    const std::string &def) const {
  auto it = volume->pruneParameters.find(name);
  if(it != volume->pruneParameters.end())
    return it->second;
  else
    return def;
}

const PrunePolicy *PrunePolicy::find(const std::string &name) {
  assert(policies != nullptr);          // policies not statically initialized
  auto it = policies->find(name);
  if(it == policies->end())
    throw ConfigError("unrecognized pruning policy '" + name + "'");
  return it->second;
}

void validatePrunePolicy(const Volume *volume) {
  const PrunePolicy *policy = PrunePolicy::find(volume->prunePolicy);
  policy->validate(volume);
}

void backupPrunable(std::vector<Backup *> &onDevice,
                    std::map<Backup *, std::string> &prune,
                    int total) {
  if(onDevice.size() == 0)
    return;
  const Volume *volume = onDevice.at(0)->volume;
  const PrunePolicy *policy = PrunePolicy::find(volume->prunePolicy);
  policy->prunable(onDevice, prune, total);
}

PrunePolicy::policies_type *PrunePolicy::policies;

// Remove old and incomplete backups
void pruneBackups() {
  // Make sure all state is available
  config.readState();

  // An _obsolete_ backup is a backup which exists on any device which is now
  // due for removal.  This includes devices which aren't currently available.
  std::vector<Backup *> obsoleteBackups;
  findObsoleteBackups(obsoleteBackups);

  // Return straight away if there's nothing to do
  if(obsoleteBackups.size() == 0)
    return;

  // We set all obsolete backups to PRUNING state even if they're on currently
  // unavailable devices.  Note that this means that pruning policies are
  // implemented even for these devices.
  if(command.act)
    markObsoleteBackups(obsoleteBackups);

  // Identify devices
  config.identifyDevices(Store::Enabled);

  // A _removable_ backup is an obsolete backup which is on an available device
  // and can therefore actually be removed.
  std::vector<RemovableBackup> removableBackups;
  findRemovableBackups(obsoleteBackups, removableBackups);

  if(!command.act)
    assert(removableBackups.size() == 0);

  EventLoop e;
  ActionList al(&e);

  // Initialize the bulk remove operations
  for(auto &removable: removableBackups) {
    removable.initialize();
    al.add(&removable.bulkRemover);
  }

  // Perform the deletions
  al.go();

  if(command.act) {
    // Complain about failed prunes
    checkRemovalErrors(removableBackups);
    // Update persistent state
    commitRemovals(removableBackups);
    // Update internal state
    for(auto &removable: removableBackups) {
      if(removable.bulkRemover.getStatus() == 0)
        removable.backup->volume->removeBackup(removable.backup);
    }
  }
}

static void findObsoleteBackups(std::vector<Backup *> &obsoleteBackups) {
  for(auto &h: config.hosts) {
    const Host *host = h.second;
    if(!host->selected())
      continue;
    for(auto &v: host->volumes) {
      Volume *volume = v.second;
      if(!volume->selected())
        continue;
      // For each device, the complete backups on that device
      std::map<std::string, std::vector<Backup *>> onDevices;
      // Total backups of this volume
      int total = 0;
      for(Backup *backup: volume->backups) {
        switch(backup->getStatus()) {
        case UNKNOWN:
        case UNDERWAY:
        case FAILED:
          if(command.pruneIncomplete) {
            // Prune incomplete backups.  Anything that failed is counted as
            // incomplete (a succesful retry will overwrite the log entry).
            backup->contents = std::string("status=")
              + backup_status_names[backup->getStatus()];
            obsoleteBackups.push_back(backup);
          }
          break;
        case PRUNING:
          // Both commands continue pruning anything that has started being
          // pruned.  log should already be set.
          obsoleteBackups.push_back(backup);
          break;
        case PRUNED:
          break;
        case COMPLETE:
          if(command.prune) {
            onDevices[backup->deviceName].push_back(backup);
            ++total;
          }
          break;
        }
      }
      for(auto &od: onDevices) {
        std::vector<Backup *> &onDevice = od.second;
        std::map<Backup *, std::string> prune;
        backupPrunable(onDevice, prune, total);
        for(auto &p: prune) {
          Backup *backup = p.first;
          backup->contents = p.second;
          obsoleteBackups.push_back(backup);
          --total;
        }
      }
    }
  }
}

static void markObsoleteBackups(std::vector<Backup *> obsoleteBackups) {
  config.getdb().begin();
  for(Backup *b: obsoleteBackups) {
    if(b->getStatus() != PRUNING) {
      b->setStatus(PRUNING);
      b->pruned = Date::now();
      b->update(config.getdb());
    }
  }
  config.getdb().commit();
  // We don't catch DatabaseBusy here; the prune just fails.
}

static void findRemovableBackups(std::vector<Backup *> obsoleteBackups,
                                 std::vector<RemovableBackup> &removableBackups)
{
  for(auto backup: obsoleteBackups) {
    Device *device = config.findDevice(backup->deviceName);
    Store *store = device->store;
    // Can't delete backups from unavailable stores
    if(!store || store->state != Store::Enabled)
      continue;
    std::string backupPath = backup->backupPath();
    std::string incompletePath = backupPath + ".incomplete";
    try {
      // Schedule removal of the backup
      if(command.verbose)
        IO::out.writef("INFO: pruning %s because: %s\n",
                       backupPath.c_str(),
                       backup->contents.c_str());
      if(command.act) {
        // Create the .incomplete flag file so that the operator knows this
        // backup is now partial
        IO ifile;
        ifile.open(incompletePath, "w");
        ifile.close();
        // Queue up a bulk remove operation
        removableBackups.push_back(RemovableBackup(backup));
      }
    } catch(std::runtime_error &exception) {
      // Log anything that goes wrong
      error("failed to remove %s: %s\n", backupPath.c_str(), exception.what());
      ++errors;
    }
  }
}

static void checkRemovalErrors(std::vector<RemovableBackup> &removableBackups) {
  for(auto &removable: removableBackups) {
    const std::string backupPath = removable.backup->backupPath();
    if(removable.bulkRemover.getStatus() != 0) {
      // Log failed prunes
      error("failed to remove %s: %s\n",
            backupPath.c_str(),
            SubprocessFailed::format("rm",
                                     removable.bulkRemover.getStatus()).c_str());
    } else {
      const std::string incompletePath = backupPath + ".incomplete";
      // Remove the 'incomplete' marker.
      if(command.verbose)
        IO::out.writef("INFO: removing %s\n", incompletePath.c_str());
      if(unlink(incompletePath.c_str()) < 0 && errno != ENOENT) {
        error("removing %s: %s", incompletePath.c_str(), strerror(errno));
        ++errors;
      }
    }
  }
}

static void commitRemovals(std::vector<RemovableBackup> &removableBackups) {
  for(;;) {
    int retries = 0;
    try {
      config.getdb().begin();
      for(auto &removable: removableBackups) {
        if(removable.bulkRemover.getStatus() == 0) {
          removable.backup->setStatus(PRUNED);
          // TODO actually this value for pruned is a bit late.
          removable.backup->pruned = Date::now();
          removable.backup->update(config.getdb());
        }
      }
      config.getdb().commit();
    } catch(DatabaseBusy &) {
      // Keep trying, database should be in sync with reality
      // Log a message every second or so
      if(!(retries++ & 1023))
        warning("pruning: retrying database update");
      // Wait a millisecond and try again
      usleep(1000);
      continue;
    }
    break;                            // success
  }
}

// Remove old prune logfiles
void prunePruneLogs() {
  // Delete status=PRUNED records that are too old
  Database::Statement(config.getdb(),
                      "DELETE FROM backup"
                      " WHERE status=?"
                      " AND pruned < ?",
                      SQL_INT, PRUNED,
                      SQL_INT64, (int64_t)(Date::now()
                                           - config.keepPruneLogs * 86400),
                      SQL_END);

  // Delete pre-sqlitification pruning logs
  // TODO: one day this code can be removed.
  Date today = Date::today();

  // Regexp for parsing the filename
  // Format is prune-YYYY-MM-DD.log
  std::regex r("^prune-([0-9]+-[0-9]+-[0-9]+)\\.log$");

  Directory d;
  d.open(config.logs);
  std::string f;
  while(d.get(f)) {
    std::smatch mr;
    if(!std::regex_match(f, mr, r))
      continue;
    Date d = Date(mr[1]);
    int age = today - d;
    if(age <= config.keepPruneLogs)
      continue;
    std::string path = config.logs + PATH_SEP + f;
    if(command.verbose)
      IO::out.writef("INFO: removing %s\n", path.c_str());
    if(command.act)
      if(unlink(path.c_str()) < 0)
        throw IOError("removing " + path, errno);
  }
}
