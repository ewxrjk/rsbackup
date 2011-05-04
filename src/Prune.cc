// Copyright © 2011 Richard Kettlewell.
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
#include "Subprocess.h"
#include "Utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>

static bool completed(const Backup &backup) {
  if(backup.rc == 0)
    return true;
  if(WIFEXITED(backup.rc) && WEXITSTATUS(backup.rc) == 24)
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
  for(hosts_type::iterator hostsIterator = config.hosts.begin();
      hostsIterator != config.hosts.end();
      ++hostsIterator) {
    Host *host = hostsIterator->second;
    for(volumes_type::iterator volumesIterator = host->volumes.begin();
        volumesIterator != host->volumes.end();
        ++volumesIterator) {
      Volume *volume = volumesIterator->second;
      for(backups_type::iterator backupsIterator = volume->backups.begin();
          backupsIterator != volume->backups.end();
          ++backupsIterator) {
        const Backup &backup = *backupsIterator;
        if(command.pruneIncomplete && !completed(backup)) {
          // Prune incomplete backups.  Unlike the Perl version anything that
          // failed is counted as incomplete (a succesful retry will overwrite
          // the logfile).
          oldBackups.push_back(&backup);
        }
        if(command.prune && completed(backup)) {
          // Prune obsolete complete backups
          int age = today - backup.date;
          // Keep backups that are young enough
          if(age <= volume->pruneAge)
            continue;
          // Keep backups that are on underpopulated devices
          Volume::PerDevice &pd = volume->perDevice[backup.deviceName];
          if(pd.count - pd.toBeRemoved <= volume->minBackups)
            continue;
          // Prune whatever's left
          oldBackups.push_back(&backup);
          ++pd.toBeRemoved;
        }
      }
    }
  }

  // Return straight away if there's nothing to do
  if(oldBackups.size() == 0)
    return;

  // Identify devices
  config.identifyDevices();

  // Log what we delete
  IO logFile;
  if(command.act)
    logFile.open(config.logs + PATH_SEP + "prune-" + today.toString() + ".log", "a");

  // Delete obsolete backups
  for(size_t n = 0; n < oldBackups.size(); ++n) {
    const Backup &backup = *oldBackups[n];
    Device *device = config.findDevice(backup.deviceName);
    Store *store = device->store;
    // Can't delete backups from unavailable stores
    if(!store)
      continue;
    std::string backupPath = backup.backupPath();
    std::string logPath = backup.logPath();
    std::string incompletePath = backupPath + ".incomplete";
    try {
      // We remove the backup
      if(command.verbose)
        IO::out.writef("INFO: pruning %s\n", backupPath.c_str());
      // TODO perhaps we could parallelize removal across devices.
      if(command.act)
        BulkRemove(backupPath);
      // We remove the 'incomplete' marker left by the Perl version.
      if(command.verbose)
        IO::out.writef("INFO: removing %s\n", incompletePath.c_str());
      if(command.act) {
        if(unlink(incompletePath.c_str()) < 0 && errno != ENOENT)
          throw IOError("removing " + incompletePath, errno);
      }
      // We remove the logfile last of all (so that if any of the above fail,
      // we'll revisit on a subsequent prune).
      if(command.verbose)
        IO::out.writef("INFO: removing %s\n", logPath.c_str());
      if(command.act) {
        if(unlink(logPath.c_str()) < 0)
          throw IOError("removing " + logPath, errno);
        backup.volume->removeBackup(&backup);
      }
      // Log successful pruning
      if(command.act) {
        logFile.writef("%s: removed %s\n",
                       today.toString().c_str(), backupPath.c_str());
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
  // Format is YYYY-MM-DD-DEVICE-HOST-VOLUME.log
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
