// Copyright © 2011, 2012, 2014-2019 Richard Kettlewell.
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
#include "Store.h"
#include "Command.h"
#include "IO.h"
#include "Subprocess.h"
#include "Errors.h"
#include "Utils.h"
#include "Database.h"
#include "BulkRemove.h"
#include <algorithm>
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/filesystem.hpp>
#include <sysexits.h>

/** @brief rsync exit status indicating a file vanished during backup */
const int RERR_VANISHED = 24;

/** @brief Subprocess subclass for interpreting @c rsync exit status
 *
 * Exit status @c RERR_VANISHED from @c rsync indicates (as far as I can tell)
 * that a file that @c rsync expected to exist (e.g. because it appeared in a
 * directory listing) did not exist when it attempted to access it. The source
 * code treats it as a warning rather than an error, presumably on the grounds
 * that the file could have disappeared a fraction of a second earlier and the
 * resulting copy would be no different, and we follow the same approach.
 */
class RsyncSubprocess: public Subprocess {
public:
  /** @brief Constructor
   * @param name Action name
   */
  RsyncSubprocess(const std::string &name): Subprocess(name) {
  }

  bool getActionStatus() const {
    int rc = getStatus();
    if(WIFEXITED(rc) && WEXITSTATUS(rc) == RERR_VANISHED)
      return true;                      // just a warning
    return rc == 0;
  }
};

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
  const char *what = "pending";

  /** @brief Log output */
  std::string log;

  /** @brief The outcome of the backup */
  Backup *outcome = nullptr;

  /** @brief Constructor */
  MakeBackup(Volume *volume_, Device *device_);

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
  sourcePath(volume->path) {
}

// Find a backup to link to.
const Backup *MakeBackup::getLastBackup() {
  // Link against the most recent complete backup if possible.
  for(const Backup *backup: boost::adaptors::reverse(volume->backups)) {
    if(backup->rc == 0
       && device->name == backup->deviceName)
      return backup;
  }
  // If there are no complete backups link against the most recent incomplete
  // one.
  for(const Backup *backup: boost::adaptors::reverse(volume->backups)) {
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
  sp.setenv("RSBACKUP_ACT", globalCommand.act ? "true" : "false");
  sp.setTimeout(volume->hookTimeout);
}

void MakeBackup::subprocessIO(Subprocess &sp, bool outputToo) {
  sp.capture(2, &log, outputToo ? 1 : -1);
}

int MakeBackup::preBackup() {
  if(volume->preBackup.size()) {
    EventLoop e;
    ActionList al(&e);

    std::string output;
    Subprocess sp("pre-backup-hook/"
                  + volume->parent->name + "/"
                  + volume->name + "/"
                  + device->name,
                  volume->preBackup);
    sp.capture(1, &output);
    sp.setenv("RSBACKUP_HOOK", "pre-backup-hook");
    hookEnvironment(sp);
    sp.reporting(globalWarningMask & WARNING_VERBOSE, false);
    subprocessIO(sp, false);
    al.add(&sp);
    al.go();
    if(output.size()) {
      if(output[output.size() - 1] == '\n')
        output.erase(output.size() - 1);
      sourcePath = output;
    }
    return sp.getStatus();
  }
  return 0;
}

/** @brief Action performed before each backup
 *
 * Creates the backup directories and @c .incomplete file.
 */
class BeforeBackup: public Action {
public:
  /** @brief Constructor
   * @param mb Parent @ref MakeBackup object
   */
  BeforeBackup(MakeBackup *mb): Action("before-backup/"
                                       + mb->volume->parent->name + "/"
                                       + mb->volume->name + "/"
                                       + mb->device->name),
                                mb(mb) {
  }

  void go(EventLoop *, ActionList *al) override {
    try {
      mb->what = "creating volume directory";
      boost::filesystem::create_directories(mb->volumePath);
      // Create the .incomplete flag file
      mb->what = "creating .incomplete file";
      IO ifile;
      ifile.open(mb->incompletePath, "w");
      ifile.close();
      // Create backup directory
      mb->what = "creating backup directory";
      boost::filesystem::create_directories(mb->backupPath);
    } catch(std::runtime_error &e) {
      // TODO refactor
      mb->log += "ERROR: ";
      mb->log += e.what();
      mb->log += "\n";
      al->completed(this, false);
      return;
    }
    al->completed(this, true);
  }

  /** @brief Parent @ref MakeBackup object */
  MakeBackup *mb;
};

int MakeBackup::rsyncBackup() {
  int rc;
  try {
    EventLoop e;
    ActionList al(&e);

    BeforeBackup before_backup(this);
    al.add(&before_backup);

    // Synthesize command
    what = "constructing command";
    std::vector<std::string> cmd;
    cmd.push_back(host->rsyncCommand);
    cmd.insert(cmd.end(), volume->rsyncBaseOptions.begin(), volume->rsyncBaseOptions.end());
    cmd.insert(cmd.end(), volume->rsyncExtraOptions.begin(), volume->rsyncExtraOptions.end());
    if(!(globalWarningMask & WARNING_VERBOSE))
      cmd.push_back("--quiet");         // suppress non-errors
    if(!volume->traverse)
      cmd.push_back("--one-file-system"); // don't cross mount points
    // Exclusions
    for(auto &exclusion: volume->exclude)
      cmd.push_back("--exclude=" + exclusion);
    const Backup *lastBackup = getLastBackup();
    if(lastBackup != nullptr)
      cmd.push_back("--link-dest=" + lastBackup->backupPath());
    // Source
    cmd.push_back(host->sshPrefix() + sourcePath + "/.");
    // Destination
    cmd.push_back(backupPath + "/.");
    // Set up subprocess
    RsyncSubprocess sp("backup/"
                       + volume->parent->name + "/"
                       + volume->name + "/"
                       + device->name);
    sp.setCommand(cmd);
    sp.reporting(globalWarningMask & WARNING_VERBOSE, !globalCommand.act);
    sp.after(before_backup.get_name(), ACTION_SUCCEEDED);
    if(!globalCommand.act)
      return 0;
    subprocessIO(sp, true);
    sp.setTimeout(volume->rsyncTimeout);
    // Make the backup
    al.add(&sp);
    al.go();
    rc = sp.getStatus();
    what = "rsync";
    // Suppress exit status 24 "Partial transfer due to vanished source files"
    if(WIFEXITED(rc) && WEXITSTATUS(rc) == RERR_VANISHED) {
      warning(WARNING_PARTIAL, "partial transfer backing up %s:%s to %s",
              host->name.c_str(),
              volume->name.c_str(),
              device->name.c_str());
      rc = 0;
    }
    // Clean up when finished
    if(rc == 0) {
      if(remove(incompletePath.c_str()) < 0) {
        throw SystemError("removing " + incompletePath, errno);
      }
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
  return rc;
}

void MakeBackup::postBackup() {
  if(volume->postBackup.size()) {
    EventLoop e;
    ActionList al(&e);

    Subprocess sp("post-backup-hook/"
                  + volume->parent->name + "/"
                  + volume->name + "/"
                  + device->name,
                  volume->postBackup);
    sp.setenv("RSBACKUP_STATUS", outcome && outcome->rc == 0 ? "ok" : "failed");
    sp.setenv("RSBACKUP_HOOK", "post-backup-hook");
    hookEnvironment(sp);
    sp.reporting(globalWarningMask & WARNING_VERBOSE, false);
    subprocessIO(sp, true);
    al.add(&sp);
    al.go();
  }
}

void MakeBackup::performBackup() {
  // Run the pre-backup hook
  what = "preBackup";
  int rc = preBackup();
  if(WIFEXITED(rc) && WEXITSTATUS(rc) == EX_TEMPFAIL) {
    if(globalWarningMask & WARNING_VERBOSE)
      IO::out.writef("INFO: %s:%s is temporarily unavailable due to pre-backup-hook\n",
                     host->name.c_str(),
                     volume->name.c_str());
    return;
  }
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
  if(globalCommand.act) {
    // Record in the database that the backup is underway
    // If this fails then the backup just fails.
    globalConfig.getdb().begin();
    outcome->insert(globalConfig.getdb(), true/*replace*/);
    globalConfig.getdb().commit();
  }
  // Run the post-backup hook
  postBackup();
  if(!globalCommand.act) {
    delete outcome;
    return;
  }
  // Get the logfile
  // TODO we could perhaps share with Conf::readState() here
  outcome->contents = log;
  if(outcome->contents.size()
     && outcome->contents[outcome->contents.size() - 1] != '\n')
    outcome->contents += '\n';
  volume->addBackup(outcome);
  if(rc) {
    // Count up errors
    ++globalErrors;
    if(globalWarningMask & (WARNING_VERBOSE|WARNING_ERRORLOGS)) {
      warning(WARNING_VERBOSE|WARNING_ERRORLOGS,
              "backup of %s:%s to %s: %s",
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
      globalConfig.getdb().begin();
      outcome->update(globalConfig.getdb());
      globalConfig.getdb().commit();
      break;
    } catch(DatabaseBusy &) {
      // Log a message every second or so
      if(!(retries++ & 1023))
        warning(WARNING_DATABASE,
                "backup of %s:%s to %s: retrying database update",
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
  if(globalWarningMask & WARNING_VERBOSE)
    IO::out.writef("INFO: backup %s:%s to %s\n",
                   host->name.c_str(), volume->name.c_str(),
                   device->name.c_str());
  MakeBackup mb(volume, device);
  mb.performBackup();
}

// Backup VOLUME
static void backupVolume(Volume *volume) {
  Host *host = volume->parent;
  char buffer[1024];
  for(auto &d: globalConfig.devices) {
    Device *device = d.second;
    switch(volume->needsBackup(device)) {
    case BackupRequired:
      globalConfig.identifyDevices(Store::Enabled);
      if(device->store && device->store->state == Store::Enabled)
        backupVolume(volume, device);
      else if(globalWarningMask & WARNING_STORE) {
        globalConfig.identifyDevices(Store::Disabled);
        if(device->store)
          switch(device->store->state) {
          case Store::Disabled:
            warning(WARNING_STORE,
                    "cannot backup %s:%s to %s - device suppressed due to --store",
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
          warning(WARNING_STORE,
                  "cannot backup %s:%s to %s - device not available",
                  host->name.c_str(),
                  volume->name.c_str(),
                  device->name.c_str());
      }
      break;
    case AlreadyBackedUp:
      if(globalWarningMask & WARNING_VERBOSE)
        IO::out.writef("INFO: %s:%s is already backed up on %s\n",
                       host->name.c_str(),
                       volume->name.c_str(),
                       device->name.c_str());
      break;
    case NotAvailable:
      if(globalWarningMask & WARNING_VERBOSE)
        IO::out.writef("INFO: %s:%s is not available\n",
                       host->name.c_str(),
                       volume->name.c_str());
      break;
    case NotThisDevice:
      if(globalWarningMask & WARNING_VERBOSE)
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
      warning(WARNING_ALWAYS, "cannot backup always-up host %s - not reachable",
              host->name.c_str());
      ++globalErrors;
      // Try anyway, so that the failures are recorded.
    } else {
      warning(WARNING_UNREACHABLE, "cannot backup %s - not reachable", host->name.c_str());
      return;
    }
  }
  for(auto &v: host->volumes) {
    Volume *volume = v.second;
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
  globalConfig.readState();
  std::vector<Host *> hosts;
  for(auto &h: globalConfig.hosts) {
    Host *host = h.second;
    if(host->selected())
      hosts.push_back(host);
  }
  std::sort(hosts.begin(), hosts.end(), order_host);
  for(Host *h: hosts)
    backupHost(h);
}
