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
#include "Device.h"
#include "Backup.h"
#include "Volume.h"
#include "Host.h"
#include "Command.h"
#include "Errors.h"
#include "IO.h"
#include "Utils.h"
#include "Store.h"
#include "Database.h"
#include "Prune.h"
#include "PrunePolicy.h"
#include "BulkRemove.h"
#include <algorithm>
#include <regex>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>
#include <unistd.h>

#include <boost/core/noncopyable.hpp>

class RemovableBackup;

/** @brief Action class to clean up after a backup has been removed */
class RemovedBackup: public Action {
public:
  /** @brief Constructor */
  RemovedBackup(const std::string &name, RemovableBackup *rb):
      Action(name), parent(rb) {}

  /** @brief Initiate cleanup */
  void go(EventLoop *e, ActionList *al) override;

private:
  /** @brief Owning @ref RemovableBackup
   *
   * This forces @ref RemovableBackup to be noncopyable.
   */
  RemovableBackup *parent = nullptr;
};

/** @brief A removable backup */
class RemovableBackup: boost::noncopyable {
public:
  /** @brief Constructor
   * @param b Backup to remove
   */
  RemovableBackup(Backup *b):
      backup(b),
      bulkRemover("remove/" + b->volume->parent->name + "/" + b->volume->name
                  + "/" + b->deviceName + "/" + b->id),
      removedBackup("removed/" + b->volume->parent->name + "/" + b->volume->name
                        + "/" + b->deviceName + "/" + b->id,
                    this) {
    // Cleanup happens after removal (unconditionally)
    removedBackup.after(bulkRemover.get_name(), 0);
  }

  /** @brief Initialize the child objects */
  void initialize(ActionList &al) {
    bulkRemover.initialize(backup->backupPath());
    bulkRemover.uses(backup->deviceName);
    removedBackup.uses(backup->deviceName);
    al.add(&bulkRemover);
    al.add(&removedBackup);
  }

  /** @brief Called when a removal has completed (not necessarily successfully)
   * @param timedOut Indicates the whole pruning job timed out
   */
  void completed(bool timedOut) {
    // Only run once
    if(alreadyComplete)
      return;
    alreadyComplete = true;
    // Log what happened and (on success) remove .incomplete
    const std::string backupPath = backup->backupPath();
    int status = bulkRemover.getStatus();
    switch(status) {
    default: // Log failed prunes
      // If we timed out then a SIGTERM is expected, so hide that behind
      // WARNING_VERBOSE. Any other failure is an error.
      if(timedOut && WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM)
        warning(WARNING_VERBOSE, "failed to remove %s: %s", backupPath.c_str(),
                SubprocessFailed::format(globalConfig.rm, status).c_str());
      else
        error("failed to remove %s: %s", backupPath.c_str(),
              SubprocessFailed::format(globalConfig.rm, status).c_str());
      break;
    case -1: // Never ran
      // This happens if we timed out or errored before getting to
      // this backup.
      warning(WARNING_VERBOSE, "failed to remove %s: cancelled",
              backupPath.c_str());
      break;
    case 0: // Succeeded
      const std::string incompletePath = backupPath + ".incomplete";
      // Remove the 'incomplete' marker.
      if(globalWarningMask & WARNING_VERBOSE)
        IO::out.writef("INFO: removing %s\n", incompletePath.c_str());
      if(unlink(incompletePath.c_str()) < 0 && errno != ENOENT) {
        error("removing %s: %s", incompletePath.c_str(), strerror(errno));
        ++globalErrors;
      }
    }
    if(status == 0) {
      backup->setStatus(PRUNED);
      // When the state is PRUNING, the pruned date indicates
      // when it was decided to prune the backup; when the state
      // is PRUNED, it indicates when pruning completed.
      backup->pruned = Date::now("PRUNE");
      backup->update(globalConfig.getdb());
      // Update the database
      for(;;) {
        int retries = 0;
        try {
          globalConfig.getdb().begin();
          globalConfig.getdb().commit();
        } catch(DatabaseBusy &) {
          // Keep trying, database should be in sync with reality
          // Log a message every second or so
          if(!(retries++ & 1023))
            warning(WARNING_DATABASE, "pruning: retrying database update");
          // Wait a millisecond and try again
          usleep(1000);
          continue;
        }
        break; // success
      }
      // Update internal state
      if(status == 0)
        backup->volume->removeBackup(backup);
    }
  }

  /** @brief The backup to remove */
  Backup *backup = nullptr;

  /** @brief The bulk remove instance for this backup */
  BulkRemove bulkRemover;

  /** @brief The cleanup instance for this backup */
  RemovedBackup removedBackup;

  /** @brief Completion status */
  bool alreadyComplete = false;
};

void RemovedBackup::go(EventLoop *, ActionList *al) {
  // Hand everything off to the parent RemovableBackup
  parent->completed(false);
  // Report back that we finished immediately
  al->completed(this, true);
}

static void findObsoleteBackups(std::vector<Backup *> &obsoleteBackups);
static void markObsoleteBackups(std::vector<Backup *> obsoleteBackups);
static void
findRemovableBackups(std::vector<Backup *> obsoleteBackups,
                     std::vector<RemovableBackup *> &removableBackups);

void backupPrunable(std::vector<Backup *> &onDevice,
                    std::map<Backup *, std::string> &prune, int total) {
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
  globalConfig.readState();

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
  if(globalCommand.act)
    markObsoleteBackups(obsoleteBackups);

  // Identify devices
  globalConfig.identifyDevices(Store::Enabled);

  // A _removable_ backup is an obsolete backup which is on an available device
  // and can therefore actually be removed.
  std::vector<RemovableBackup *> removableBackups;
  findRemovableBackups(obsoleteBackups, removableBackups);

  if(!globalCommand.act)
    assert(removableBackups.size() == 0);

  EventLoop e;
  ActionList al(&e);

  // Initialize the bulk remove operations
  for(auto &removable: removableBackups) {
    removable->initialize(al);
  }

  // Give up if it takes too long
  if(globalConfig.pruneTimeout > 0) {
    struct timespec limit;
    getMonotonicTime(limit);
    limit.tv_sec += globalConfig.pruneTimeout;
    al.setLimit(limit);
  }

  // Perform the deletions
  al.go();

  if(globalCommand.act) {
    // If we timed out then complete any uncompleted removal attempts
    if(al.timeLimitExceeded()) {
      for(auto &removable: removableBackups) {
        removable->completed(true);
      }
    }
  }

  deleteAll(removableBackups);
}

// Get a list of all the backups to prune. This means backups for
// which pruning has already started, and backups selected by the
// pruning policy for their volume. It includes backups that are
// on unavailable devices.
static void findObsoleteBackups(std::vector<Backup *> &obsoleteBackups) {
  for(auto &h: globalConfig.hosts) {
    const Host *host = h.second;
    if(!host->selected(PurposePrune))
      continue;
    for(auto &v: host->volumes) {
      Volume *volume = v.second;
      if(!volume->selected(PurposePrune))
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
          if(globalCommand.pruneIncomplete) {
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
        case PRUNED: break;
        case COMPLETE:
          if(globalCommand.prune) {
            onDevices[backup->deviceName].push_back(backup);
            ++total;
          }
          break;
        }
      }
      // Find complete backups that are now prunable
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

// Update the database to mark all obsolete backups for pruning.
static void markObsoleteBackups(std::vector<Backup *> obsoleteBackups) {
  globalConfig.getdb().begin();
  for(Backup *b: obsoleteBackups) {
    if(b->getStatus() != PRUNING) {
      b->setStatus(PRUNING);
      b->pruned = Date::now("PRUNE");
      b->update(globalConfig.getdb());
    }
  }
  globalConfig.getdb().commit();
  // We don't catch DatabaseBusy here; the prune just fails.
}

// Winnow down the obsolete backups to those we can actually remove,
// i.e. those on an available device. We also create .incomplete
// files here, to signal to the operator that these backups are
// not suitable for restoration.
static void
findRemovableBackups(std::vector<Backup *> obsoleteBackups,
                     std::vector<RemovableBackup *> &removableBackups) {
  for(auto backup: obsoleteBackups) {
    Device *device = globalConfig.findDevice(backup->deviceName);
    Store *store = device->store;
    // Can't delete backups from unavailable stores
    if(!store || store->state != Store::Enabled)
      continue;
    std::string backupPath = backup->backupPath();
    std::string incompletePath = backupPath + ".incomplete";
    try {
      // Schedule removal of the backup
      if(globalWarningMask & WARNING_VERBOSE)
        IO::out.writef("INFO: pruning %s because: %s\n", backupPath.c_str(),
                       backup->contents.c_str());
      if(globalCommand.act) {
        // Create the .incomplete flag file so that the operator knows this
        // backup is now partial
        IO ifile;
        ifile.open(incompletePath, "w");
        ifile.close();
        // Queue up a bulk remove operation
        removableBackups.push_back(new RemovableBackup(backup));
      }
    } catch(std::runtime_error &exception) {
      // Log anything that goes wrong
      error("failed to remove %s: %s\n", backupPath.c_str(), exception.what());
      ++globalErrors;
    }
  }
}

// Remove old prune logfiles
void prunePruneLogs() {
  if(globalCommand.act)
    // Delete status=PRUNED records that are too old
    Database::Statement(
        globalConfig.getdb(),
        "DELETE FROM backup"
        " WHERE status=?"
        " AND pruned < ?",
        SQL_INT, PRUNED, SQL_INT64,
        (int64_t)(Date::now("PRUNE") - globalConfig.keepPruneLogs), SQL_END)
        .next();
}
