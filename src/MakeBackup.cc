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
#include "Store.h"
#include "Command.h"
#include "IO.h"
#include "Subprocess.h"
#include "Errors.h"
#include "Utils.h"
#include "Database.h"
#include <algorithm>
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fnmatch.h>

/** @brief State for a single backup attempt */
class MakeBackup {
public:
  /** @brief Volume to back up */
  Volume *volume;

  /** @brief Target device */
  Device *device;

  /** @brief Host containing @ref volume */
  Host *host;

  /** @brief Start time of backup */
  time_t startTime;

  /** @brief Today's date */
  Date today;

  /** @brief ID of new backup */
  std::string id;

  /** @brief Volume path on device */
  const std::string volumePath;

  /** @brief Backup path on device */
  const std::string backupPath;

  /** @brief .incomplete file on device */
  const std::string incompletePath;

  /** @brief Path to volume to back up
   *
   * May be modified by the pre-backup hook.
   */
  std::string sourcePath;

  /** @brief Current work */
  const char *what;

  /** @brief Log output */
  std::string log;

  /** @brief The outcome of the backup */
  Backup *outcome;

  /** @brief Constructor */
  MakeBackup(Volume *volume_, Device *device_);

  /** @brief Destructor */
  ~MakeBackup();

  /** @brief Find the most recent matching backup
   *
   * Prefers complete backups if available.
   */
  const Backup *getLastBackup();

  /** @brief Set up the common hook environment for a subprocess
   * @param sp Subprocess
   */
  void hookEnvironment(Subprocess &sp);

  /** @brief Set up logfile IO for a subprocess
   * @param sp Subprocess
   * @param outputToo Log stdout as well as just stderr
   */
  void subprocessIO(Subprocess &sp, bool outputToo = true);

  /** @brief Run the pre-backup hook if there is one
   * @return Wait status
   */
  int preBackup();

  /** @brief Run rsync to make the backup
   * @return Wait status
   */
  int rsyncBackup();

  /** @brief Run the post-backup hook if there is one */
  void postBackup();

  /** @brief Perform a backup */
  void performBackup();
};

MakeBackup::MakeBackup(Volume *volume_, Device *device_):
  volume(volume_),
  device(device_),
  host(volume->parent),
  startTime(Date::now()),
  today(Date::today()),
  id(today.toString()),
  volumePath(device->store->path
             + PATH_SEP + host->name
             + PATH_SEP + volume->name),
  backupPath(volumePath
             + PATH_SEP + id),
  incompletePath(backupPath + ".incomplete"),
  sourcePath(volume->path),
  what("pending"),
  outcome(nullptr) {
}

MakeBackup::~MakeBackup() {
}

// Find a backup to link to.
const Backup *MakeBackup::getLastBackup() {
  // Link against the most recent complete backup if possible.
  for(backups_type::reverse_iterator backupsIterator = volume->backups.rbegin();
      backupsIterator != volume->backups.rend();
      ++backupsIterator) {
    const Backup *backup = *backupsIterator;
    if(backup->rc == 0
       && device->name == backup->deviceName)
      return backup;
  }
  // If there are no complete backups link against the most recent incomplete
  // one.
  for(backups_type::reverse_iterator backupsIterator = volume->backups.rbegin();
      backupsIterator != volume->backups.rend();
      ++backupsIterator) {
    const Backup *backup = *backupsIterator;
    if(device->name == backup->deviceName)
      return backup;
  }
  // Otherwise there is nothing to link to.
  return nullptr;
}

void MakeBackup::hookEnvironment(Subprocess &sp) {
  sp.setenv("RSBACKUP_DEVICE", device->name);
  sp.setenv("RSBACKUP_HOST", host->name);
  sp.setenv("RSBACKUP_SSH_HOSTNAME", host->hostname);
  sp.setenv("RSBACKUP_SSH_USERNAME", host->user);
  sp.setenv("RSBACKUP_SSH_TARGET", host->userAndHost());
  sp.setenv("RSBACKUP_STORE", device->store->path);
  sp.setenv("RSBACKUP_VOLUME", volume->name);
  sp.setenv("RSBACKUP_VOLUME_PATH", volume->path);
  sp.setenv("RSBACKUP_ACT", command.act ? "true" : "false");
  sp.setTimeout(volume->hookTimeout);
}

void MakeBackup::subprocessIO(Subprocess &sp, bool outputToo) {
  sp.capture(2, &log, outputToo ? 1 : -1);
}

int MakeBackup::preBackup() {
  if(volume->preBackup.size()) {
    std::string output;
    Subprocess sp(volume->preBackup);
    sp.capture(1, &output);
    sp.setenv("RSBACKUP_HOOK", "pre-backup-hook");
    hookEnvironment(sp);
    if(command.verbose)
      sp.report();
    subprocessIO(sp, false);
    int rc = sp.runAndWait(false);
    if(output.size()) {
      if(output[output.size() - 1] == '\n')
        output.erase(output.size() - 1);
      sourcePath = output;
    }
    return rc;
  }
  return 0;
}

int MakeBackup::rsyncBackup() {
  int rc;
  try {
    // Create volume directory
    if(command.act) {
      what = "creating volume directory";
      makeDirectory(volumePath);
      // Create the .incomplete flag file
      what = "creating .incomplete file";
      IO ifile;
      ifile.open(incompletePath, "w");
      ifile.close();
      // Create backup directory
      what = "creating backup directory";
      makeDirectory(backupPath);
      what = "constructing command";
    }
    // Synthesize command
    std::vector<std::string> cmd;
    cmd.push_back("rsync");
    cmd.push_back("--archive");         // == -rlptgoD
    // --recursive                         recurse into directories
    // --links                             preserve symlinks
    // --perms                             preserve permissions
    // --times                             preserve modification times
    // --group                             preserve group IDs
    // --owner                             preserve user IDs
    // --devices                           preserve device files
    // --specials                          preserve special files
    cmd.push_back("--sparse");          // handle spare files efficiently
    cmd.push_back("--numeric-ids");     // don't remap UID/GID by name
    cmd.push_back("--compress");        // compress during file transfer
    cmd.push_back("--fuzzy");           // look for similar files
    cmd.push_back("--hard-links");      // preserve hard links
    cmd.push_back("--delete");          // delete extra files in destination
    if(!command.verbose)
      cmd.push_back("--quiet");         // suppress non-errors
    if(!volume->traverse)
      cmd.push_back("--one-file-system"); // don't cross mount points
    // Exclusions
    for(size_t n = 0; n < volume->exclude.size(); ++n)
      cmd.push_back("--exclude=" + volume->exclude[n]);
    const Backup *lastBackup = getLastBackup();
    if(lastBackup != nullptr)
      cmd.push_back("--link-dest=" + lastBackup->backupPath());
    // Source
    cmd.push_back(host->sshPrefix() + sourcePath + "/.");
    // Destination
    cmd.push_back(backupPath + "/.");
    // Set up subprocess
    Subprocess sp(cmd);
    if(command.verbose)
      sp.report();
    if(!command.act)
      return 0;
    subprocessIO(sp, true);
    sp.setTimeout(volume->rsyncTimeout);
    // Make the backup
    rc = sp.runAndWait(false);
    what = "rsync";
    // Suppress exit status 24 "Partial transfer due to vanished source files"
    if(WIFEXITED(rc) && WEXITSTATUS(rc) == 24) {
      if(command.warnPartial)
        warning("partial transfer backing up %s:%s to %s",
                host->name.c_str(),
                volume->name.c_str(),
                device->name.c_str());
      rc = 0;
    }
  } catch(std::runtime_error &e) {
    // Try to handle any other errors the same way as rsync failures.  If we
    // can't even write to the logfile we error out.
    log += "ERROR: ";
    log += e.what();
    log += "\n";
    // This is a bit misleading (it's not really a wait status) but it will
    // do for now.
    rc = 255;
  }
  // If the backup completed, remove the 'incomplete' flag file
  if(!rc) {
    if(unlink(incompletePath.c_str()) < 0)
      throw IOError("removing " + incompletePath, errno);
  }
  return rc;
}

void MakeBackup::postBackup() {
  if(volume->postBackup.size()) {
    Subprocess sp(volume->postBackup);
    sp.setenv("RSBACKUP_STATUS", outcome && outcome->rc == 0 ? "ok" : "failed");
    sp.setenv("RSBACKUP_HOOK", "post-backup-hook");
    hookEnvironment(sp);
    if(command.verbose)
      sp.report();
    subprocessIO(sp, true);
    sp.runAndWait(false);
  }
}

void MakeBackup::performBackup() {
  // Run the pre-backup hook
  what = "preBackup";
  int rc = preBackup();
  if(!rc)
    rc = rsyncBackup();
  // Put together the outcome
  outcome = new Backup();
  outcome->rc = rc;
  outcome->time = startTime;
  outcome->date = today;
  outcome->id = id;
  outcome->deviceName = device->name;
  outcome->volume = volume;
  outcome->setStatus(UNDERWAY);
  if(command.act) {
    // Record in the database that the backup is underway
    // If this fails then the backup just fails.
    config.getdb()->begin();
    outcome->insert(config.getdb(), true/*replace*/);
    config.getdb()->commit();
  }
  // Run the post-backup hook
  postBackup();
  if(!command.act)
    return;
  // Get the logfile
  // TODO we could perhaps share with Conf::readState() here
  outcome->contents = log;
  if(outcome->contents.size()
     && outcome->contents[outcome->contents.size() - 1] != '\n')
    outcome->contents += '\n';
  volume->addBackup(outcome);
  if(rc) {
    // Count up errors
    ++errors;
    if(command.verbose || command.repeatErrorLogs) {
      warning("backup of %s:%s to %s: %s",
              host->name.c_str(),
              volume->name.c_str(),
              device->name.c_str(),
              SubprocessFailed::format(what, rc).c_str());
      IO::err.write(outcome->contents);
      IO::err.writef("\n");
    }
    /*if(WIFEXITED(rc) && WEXITSTATUS(rc) == 24)
      outcome->status = COMPLETE;*/
    outcome->setStatus(FAILED);
  } else
    outcome->setStatus(COMPLETE);
  // Store the result in the database
  // We really care about 'busy' errors - the backup has been made, we must
  // record this fact.
  for(;;) {
    int retries = 0;
    try {
      config.getdb()->begin();
      outcome->update(config.getdb());
      config.getdb()->commit();
      break;
    } catch(DatabaseBusy &) {
      // Log a message every second or so
      if(!(retries++ & 1023))
        warning("backup of %s:%s to %s: retrying database update",
                host->name.c_str(),
                volume->name.c_str(),
                device->name.c_str());
      // Wait a millisecond and try again
      usleep(1000);
      continue;
    }
  }
}

// Backup VOLUME onto DEVICE.
//
// device->store is assumed to be set.
static void backupVolume(Volume *volume, Device *device) {
  Host *host = volume->parent;
  if(command.verbose)
    IO::out.writef("INFO: backup %s:%s to %s\n",
                   host->name.c_str(), volume->name.c_str(),
                   device->name.c_str());
  MakeBackup mb(volume, device);
  mb.performBackup();
}

enum BackupRequirement {
  AlreadyBackedUp,
  NotThisDevice,
  NotAvailable,
  BackupRequired
};

// See whether VOLUME needs a backup on DEVICE
static BackupRequirement needsBackup(Volume *volume, Device *device) {
  switch(fnmatch(volume->devicePattern.c_str(), device->name.c_str(),
                 FNM_NOESCAPE)) {
  case 0:
    break;
  case FNM_NOMATCH:
    return NotThisDevice;
  default:
    warning("invalid device pattern '%s'",
            volume->devicePattern.c_str());
    /* fail safe - make the backup */
    break;
  }
  Date today = Date::today();
  for(auto backupsIterator = volume->backups.begin();
      backupsIterator != volume->backups.end();
      ++backupsIterator) {
    const Backup *backup = *backupsIterator;
    if(backup->rc == 0
       && backup->date == today
       && backup->deviceName == device->name)
      return AlreadyBackedUp;           // Already backed up
  }
  if(!volume->available())
    return NotAvailable;
  return BackupRequired;
}

// Backup VOLUME
static void backupVolume(Volume *volume) {
  Host *host = volume->parent;
  char buffer[1024];
  for(auto devicesIterator = config.devices.begin();
      devicesIterator != config.devices.end();
      ++devicesIterator) {
    Device *device = devicesIterator->second;
    switch(needsBackup(volume, device)) {
    case BackupRequired:
      config.identifyDevices(Store::Enabled);
      if(device->store && device->store->state == Store::Enabled)
        backupVolume(volume, device);
      else if(command.warnStore) {
        config.identifyDevices(Store::Disabled);
        if(device->store)
          switch(device->store->state) {
          case Store::Disabled:
            warning("cannot backup %s:%s to %s - device suppressed due to --store",
                    host->name.c_str(),
                    volume->name.c_str(),
                    device->name.c_str());
            break;
          default:
            snprintf(buffer, sizeof buffer,
                     "device %s store %s unexpected state %d",
                     device->name.c_str(),
                     device->store->path.c_str(),
                     device->store->state);
            throw FatalStoreError(buffer);
          }
        else
          warning("cannot backup %s:%s to %s - device not available",
                  host->name.c_str(),
                  volume->name.c_str(),
                  device->name.c_str());
      }
      break;
    case AlreadyBackedUp:
      if(command.verbose)
        IO::out.writef("INFO: %s:%s is already backed up on %s\n",
                       host->name.c_str(),
                       volume->name.c_str(),
                       device->name.c_str());
      break;
    case NotAvailable:
      if(command.verbose)
        IO::out.writef("INFO: %s:%s is not available\n",
                       host->name.c_str(),
                       volume->name.c_str());
      break;
    case NotThisDevice:
      if(command.verbose)
        IO::out.writef("INFO: %s:%s excluded from %s by device pattern\n",
                       host->name.c_str(),
                       volume->name.c_str(),
                       device->name.c_str());
      break;
    }
  }
}

// Backup HOST
static void backupHost(Host *host) {
  // Do a quick check for unavailable hosts
  if(!host->available()) {
    if(host->alwaysUp) {
      warning("cannot backup always-up host %s - not reachable",
              host->name.c_str());
      ++errors;
      // Try anyway, so that the failures are recorded.
    } else {
      if(command.warnUnreachable)
        warning("cannot backup %s - not reachable", host->name.c_str());
      return;
    }
  }
  for(auto volumesIterator = host->volumes.begin();
      volumesIterator != host->volumes.end();
      ++volumesIterator) {
    Volume *volume = volumesIterator->second;
    if(volume->selected())
      backupVolume(volume);
  }
}

static bool order_host(const Host *a, const Host *b) {
  if(a->priority > b->priority)
    return true;
  if(a->priority < b->priority)
    return false;
  return a->name < b->name;
}

// Backup everything
void makeBackups() {
  // Load up log files
  config.readState();
  std::vector<Host *> hosts;
  for(auto hostsIterator = config.hosts.begin();
      hostsIterator != config.hosts.end();
      ++hostsIterator) {
    Host *host = hostsIterator->second;
    if(host->selected())
      hosts.push_back(host);
  }
  std::sort(hosts.begin(), hosts.end(), order_host);
  for(size_t i = 0; i < hosts.size(); ++i)
    backupHost(hosts[i]);
}
