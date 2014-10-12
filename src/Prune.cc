// Copyright © 2011, 2012, 2014 Richard Kettlewell.
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
#include "Regexp.h"
#include "IO.h"
#include "Utils.h"
#include "Store.h"
#include "Database.h"
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>

static bool completed(const Backup *backup) {
  if(backup->rc == 0 || backup->pruning)
    return true;
  if(WIFEXITED(backup->rc) && WEXITSTATUS(backup->rc) == 24)
    return true;
  return false;
}

// Remove old and incomplete backups
void pruneBackups() {
  Date today = Date::today();

  // Make sure all state is available
  config.readState();

  // Figure out which backups are obsolete, if any
  std::vector<const Backup *> oldBackups;
  std::vector<Backup *> markAsPruned;
  for(hosts_type::iterator hostsIterator = config.hosts.begin();
      hostsIterator != config.hosts.end();
      ++hostsIterator) {
    Host *host = hostsIterator->second;
    if(!host->selected())
      continue;
    for(volumes_type::iterator volumesIterator = host->volumes.begin();
        volumesIterator != host->volumes.end();
        ++volumesIterator) {
      Volume *volume = volumesIterator->second;
      if(!volume->selected())
        continue;
      for(backups_type::iterator backupsIterator = volume->backups.begin();
          backupsIterator != volume->backups.end();
          ++backupsIterator) {
        Backup *backup = *backupsIterator;
        if(command.pruneIncomplete && !completed(backup)) {
          // Prune incomplete backups.  Anything that failed is counted as
          // incomplete (a succesful retry will overwrite the logfile).
          backup->whyPruned = "incomplete";
          oldBackups.push_back(backup);
        }
        if(command.prune && completed(backup)) {
          Volume::PerDevice &pd = volume->perDevice[backup->deviceName];
          if(backup->pruning) {
            backup->whyPruned = "already partially pruned";
          } else {
            // Prune obsolete complete backups
            int age = today - backup->date;
            // Keep backups that are young enough
            if(age <= volume->pruneAge)
              continue;
            // Keep backups that are on underpopulated devices
            if(pd.count - pd.toBeRemoved <= volume->minBackups)
              continue;
            std::ostringstream ss;
            ss << "age " << age
               << " = today " << today
               << " - backup date " << backup->date
               << " > minimum " << volume->pruneAge
               << " and copies " << pd.count
               << " - removable " << pd.toBeRemoved
               << " > minimum " << volume->minBackups;
            backup->whyPruned = ss.str();
            // Backups to mark as pruned
            markAsPruned.push_back(backup);
          }
          // Prune whatever's left
          oldBackups.push_back(backup);
          ++pd.toBeRemoved;
        }
      }
    }
  }

  // Return straight away if there's nothing to do
  if(oldBackups.size() == 0)
    return;

  // Set the prune flag on newly identified prunable backups
  if(command.act && markAsPruned.size() > 0) {
    config.getdb()->begin();
    for(std::vector<Backup *>::iterator it = markAsPruned.begin();
        it != markAsPruned.end(); ++it) {
      Backup *b = *it;
      b->pruning = true;
      b->update(config.getdb());
    }
    config.getdb()->commit();
    // We don't catch DatabaseBusy here; the prune just fails.
  }

  // Identify devices
  config.identifyDevices(Store::Enabled);

  // Log what we delete
  IO logFile;
  if(command.act)
    logFile.open(config.logs + PATH_SEP + "prune-" + today.toString() + ".log", "a");

  // Delete obsolete backups
  for(size_t n = 0; n < oldBackups.size(); ++n) {
    const Backup *backup = oldBackups[n];
    Device *device = config.findDevice(backup->deviceName);
    Store *store = device->store;
    // Can't delete backups from unavailable stores
    if(!store || store->state != Store::Enabled)
      continue;
    std::string backupPath = backup->backupPath();
    std::string incompletePath = backupPath + ".incomplete";
    try {
      // We remove the backup
      if(command.verbose)
        IO::out.writef("INFO: pruning %s because: %s\n",
                       backupPath.c_str(),
                       backup->whyPruned.c_str());
      // TODO perhaps we could parallelize removal across devices.
      if(command.act) {
        // Create the .incomplete flag file so that the operator knows this
        // backup is now partial
        IO ifile;
        ifile.open(incompletePath, "w");
        ifile.close();
      }
      // Actually remove the backup
      BulkRemove(backupPath);
      // We remove the 'incomplete' marker.
      if(command.verbose)
        IO::out.writef("INFO: removing %s\n", incompletePath.c_str());
      if(command.act) {
        if(unlink(incompletePath.c_str()) < 0 && errno != ENOENT)
          throw IOError("removing " + incompletePath, errno);
      }
      // We update the database last of all (so that if any of the above fail,
      // we'll revisit on a subsequent prune).
      if(command.act) {
        // Update the database
        // We retry on busy, we must keep the db consistent with reality.
        int retries = 0;
        for(;;) {
          try {
            config.getdb()->begin();
            backup->remove(config.getdb());
            config.getdb()->commit();
            break;
          } catch(DatabaseBusy &) {
            // Log a message every second or so
            if(!(retries++ & 1023))
              warning("pruning %s: retrying database update",
                      backupPath.c_str());
            // Wait a millisecond and try again
            usleep(1000);
            continue;
          }
        }
        backup->volume->removeBackup(backup);
      }
      // Log successful pruning
      if(command.act) {
        logFile.writef("%s: removed %s because: %s\n",
                       today.toString().c_str(), backupPath.c_str(),
                       backup->whyPruned.c_str());
      }
    } catch(std::runtime_error &exception) {
      // Log anything that goes wrong
      if(command.act) {
        logFile.writef("%s: FAILED to remove %s: %s\n",
                       today.toString().c_str(), backupPath.c_str(), exception.what());
        ++errors;
      }
    }
    if(command.act)
      logFile.flush();
  }
}

// Remove old prune logfiles
void prunePruneLogs() {
  Date today = Date::today();

  // Regexp for parsing the filename
  // Format is prune-YYYY-MM-DD.log
  Regexp r("^prune-([0-9]+-[0-9]+-[0-9]+)\\.log$");

  Directory d;
  d.open(config.logs);
  std::string f;
  while(d.get(f)) {
    if(!r.matches(f))
      continue;
    Date d = r.sub(1);
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
