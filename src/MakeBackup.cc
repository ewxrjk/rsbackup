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
#include <algorithm>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/filesystem.hpp>
#include <sysexits.h>
#include <thread>
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

/** @brief rsync exit status indicating a file vanished during backup */
const int RERR_VANISHED = 24;

/** @brief State of pre-volume-hook execution */
enum PRE_VOLUME_HOOK_STATE {
  /** @brief Haven't run pre-volume-hook yet */
  PVH_NOT_RUN,

  /** @brief Ran pre-volume-hook successfuly */
  PVH_RUN,

  /** @brief Ran pre-volume-hook but it failed */
  PVH_FAILED,
};

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
  RsyncSubprocess(const std::string &name): Subprocess(name) {}

  bool getActionStatus() const {
    int rc = getStatus();
    if(WIFEXITED(rc) && WEXITSTATUS(rc) == RERR_VANISHED)
      return true; // just a warning
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

  /** @brief .nolink file on device */
  const std::string noLinkPath;

  /** @brief Current work */
  const char *what = "pending";

  /** @brief Log output */
  std::string log;

  /** @brief Constructor */
  MakeBackup(Volume *volume_, Device *device_);

  /** @brief Find backups to link against. */
  void getOldBackups(std::vector<const Backup *> &oldBackups);

  /** @brief Set up logfile IO for a subprocess
   * @param sp Subprocess
   * @param outputToo Log stdout as well as just stderr
   */
  void subprocessIO(Subprocess &sp, bool outputToo = true);

  /** @brief Run rsync to make the backup
   * @return Wait status
   */
  int rsyncBackup(const std::string &sourcePath);

  /** @brief Perform a backup */
  void performBackup(const std::string &sourcePath);

  /** @brief Return the ID for a new backup */
  static std::string backupID() {
    time_t now = Date::now(
        "BACKUP"); // overridden by ${RSBACKUP_TIME} / ${RSBACKUP_TIME_BACKUP}
    struct tm t;
    if(!gmtime_r(&now, &t))
      throw SystemError("gmtime_r", errno);
    char buffer[64];
    strftime(buffer, sizeof buffer, TIMESTAMP_FORMAT, &t);
    return buffer;
  }
};

MakeBackup::MakeBackup(Volume *volume_, Device *device_):
    volume(volume_), device(device_), host(volume->parent),
    startTime(Date::now("BACKUP")), today(Date::today("BACKUP")),
    id(backupID()), volumePath(device->store->path + PATH_SEP + host->name
                               + PATH_SEP + volume->name),
    backupPath(volumePath + PATH_SEP + id),
    incompletePath(backupPath + ".incomplete"),
    noLinkPath(volumePath + ".nolink") {}

// Find backups to link to.
void MakeBackup::getOldBackups(std::vector<const Backup *> &oldBackups) {
  // Start with the most recent backup and work back
  for(const Backup *backup: boost::adaptors::reverse(volume->backups)) {
    // Consider only backups on the right device
    if(device->name == backup->deviceName) {
      if(backup->getStatus() == COMPLETE) {
        // Always link against the most recent complete backup
        oldBackups.push_back(backup);
        // Once we have a complete backup, stop searching.
        break;
      } else {
        // If the most recent backup is incomplete, link against that.
        if(oldBackups.size() == 0)
          oldBackups.push_back(backup);
      }
    }
  }
}

/** @brief Set up the common environment for a subprocess
 * @param sp Subprocess
 */
void setEnvironment(Volume *volume, Subprocess &sp) {
  Host *host = volume->parent;
  sp.setenv("RSBACKUP_HOST", host->name);
  sp.setenv("RSBACKUP_GROUP", host->group);
  sp.setenv("RSBACKUP_SSH_HOSTNAME", host->hostname);
  sp.setenv("RSBACKUP_SSH_USERNAME", host->user);
  sp.setenv("RSBACKUP_SSH_TARGET", host->userAndHost());
  sp.setenv("RSBACKUP_VOLUME", volume->name);
  sp.setenv("RSBACKUP_VOLUME_PATH", volume->path);
  sp.setenv("RSBACKUP_ACT", globalCommand.act ? "true" : "false");
}

void MakeBackup::subprocessIO(Subprocess &sp, bool outputToo) {
  sp.capture(2, &log, outputToo ? 1 : -1);
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
  BeforeBackup(MakeBackup *mb):
      Action("before-backup/" + mb->volume->parent->name + "/"
             + mb->volume->name + "/" + mb->device->name),
      mb(mb) {}

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

int MakeBackup::rsyncBackup(const std::string &sourcePath) {
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
    cmd.insert(cmd.end(), volume->rsyncBaseOptions.begin(),
               volume->rsyncBaseOptions.end());
    cmd.insert(cmd.end(), volume->rsyncExtraOptions.begin(),
               volume->rsyncExtraOptions.end());
    if(!volume->traverse)
      cmd.push_back("--one-file-system"); // don't cross mount points
    if(volume->rsyncRemote.size()) {
      cmd.push_back("--rsync-path");
      cmd.push_back(volume->rsyncRemote);
    }
    // Exclusions
    for(auto &exclusion: volume->exclude)
      cmd.push_back("--exclude=" + exclusion);
    // Use old backups
    if(volume->rsyncLinkDest) {
      // As a hack to deal with untrusted existing backups (e.g. following a
      // fsck), if the <backup>.nolink exists then we suppress all link targets.
      // The file is deleted when a backup succeeds.
      bool suppressLinkDest = false;
      struct stat sb;
      if(stat(noLinkPath.c_str(), &sb) == 0)
        suppressLinkDest = true;
      std::vector<const Backup *> oldBackups;
      getOldBackups(oldBackups);
      if(oldBackups.size() > 0 && suppressLinkDest) {
        warning(WARNING_ALWAYS,
                "suppressing %zu --link-dest candidates due because %s exists",
                oldBackups.size(), noLinkPath.c_str());
      } else {
        for(auto oldBackup: oldBackups) {
          cmd.push_back("--link-dest=" + oldBackup->backupPath());
        }
      }
    }
    // Timeout
    if(volume->rsyncIOTimeout) {
      char buffer[128];
      snprintf(buffer, sizeof buffer, "--timeout=%d", volume->rsyncIOTimeout);
      cmd.push_back(buffer);
    }
    // Source
    cmd.push_back(host->sshPrefix() + sourcePath + "/.");
    // Destination
    cmd.push_back(backupPath + "/.");
    // Set up subprocess
    RsyncSubprocess sp("backup/" + volume->parent->name + "/" + volume->name
                       + "/" + device->name);
    sp.setCommand(cmd);
    setEnvironment(volume, sp);
    sp.reporting(globalWarningMask & WARNING_VERBOSE, !globalCommand.act);
    sp.after(before_backup.get_name(), ACTION_SUCCEEDED);
    if(!globalCommand.act)
      return 0;
    subprocessIO(sp, true);
    sp.setTimeout(volume->backupJobTimeout);
    // Make the backup, with the global lock released
    al.add(&sp);
    {
      release_guard<std::mutex> globalRelease(globalLock);
      al.go();
    }
    rc = sp.getStatus();
    what = "rsync";
    // Suppress exit status 24 "Partial transfer due to vanished source files"
    if(WIFEXITED(rc) && WEXITSTATUS(rc) == RERR_VANISHED) {
      warning(WARNING_PARTIAL, "partial transfer backing up %s:%s to %s",
              host->name.c_str(), volume->name.c_str(), device->name.c_str());
      rc = 0;
    }
    // Clean up when finished
    if(rc == 0) {
      if(remove(incompletePath.c_str()) < 0) {
        throw SystemError("removing " + incompletePath, errno);
      }
      if(remove(noLinkPath.c_str()) < 0 && errno != ENOENT) {
        throw SystemError("removing " + noLinkPath, errno);
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

void MakeBackup::performBackup(const std::string &sourcePath) {
  // Put together the backup record
  Backup *outcome = new Backup();
  outcome->time = startTime;
  outcome->id = id;
  outcome->deviceName = device->name;
  outcome->volume = volume;
  outcome->setStatus(UNDERWAY);
  if(globalCommand.act) {
    // Record in the database that the backup is underway
    // If this fails then the backup just fails.
    globalConfig.getdb().begin();
    outcome->insert(globalConfig.getdb(), true /*replace*/);
    globalConfig.getdb().commit();
  }
  // Make the backup
  int rc = rsyncBackup(sourcePath);
  if(!globalCommand.act) {
    // In dry-run mode, we're done for now
    delete outcome;
    return;
  }
  // Update the backup record
  outcome->waitStatus = rc;
  outcome->contents = log;
  outcome->finishTime = Date::now("FINISH");
  // Enforce explicit time setings in tests
  if(Date::override_time("BACKUP") && !Date::override_time("FINISH"))
    throw Error("incomplete time overrides");
  // Detect clock weirdness
  if(outcome->finishTime < outcome->time) {
    warning(WARNING_ALWAYS,
            "time travel detected: backup started at %s finished at %s",
            Date(outcome->time).format("%Y-%m-%d %H:%M:%S").c_str(),
            Date(outcome->finishTime).format("%Y-%m-%d %H:%M:%S (%Z)").c_str());
    if(Date::override_time("FINISH"))
      throw Error("time travelling clock override");
  }
  if(outcome->contents.size()
     && outcome->contents[outcome->contents.size() - 1] != '\n')
    outcome->contents += '\n';
  //
  if(outcome->waitStatus) {
    // Backup failed
    ++globalErrors;
    if(globalWarningMask & (WARNING_VERBOSE | WARNING_ERRORLOGS)) {
      warning(WARNING_VERBOSE | WARNING_ERRORLOGS, "backup of %s:%s to %s: %s",
              host->name.c_str(), volume->name.c_str(), device->name.c_str(),
              SubprocessFailed::format(what, outcome->waitStatus).c_str());
      IO::err.write(outcome->contents);
      IO::err.writef("\n");
    }
    outcome->setStatus(FAILED);
  } else
    outcome->setStatus(COMPLETE);
  // Attach the backup to the volume
  outcome->volume->addBackup(outcome);
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
                host->name.c_str(), volume->name.c_str(), device->name.c_str());
      // Wait a millisecond and try again
      usleep(1000);
      continue;
    }
  }
}

// Run the pre-volume-hook for VOLUME, if it hasn't been run already.
// Returns true on success and false if the hook failed.
static void runPreVolumeHook(Volume *volume, PRE_VOLUME_HOOK_STATE &pvh) {
  Host *host = volume->parent;
  // Only run once per volume
  if(pvh == PVH_NOT_RUN) {
    // If there's no hook, do nothing
    if(!volume->preVolume.size()) {
      pvh = PVH_RUN;
      return;
    }
    std::string hookLog;
    EventLoop e;
    ActionList al(&e);
    std::string output;
    Subprocess sp("pre-volume-hook/" + volume->parent->name + "/"
                      + volume->name,
                  volume->preVolume);
    sp.capture(1, &output);
    sp.setenv("RSBACKUP_HOOK", "pre-volume-hook");
    setEnvironment(volume, sp);
    sp.setTimeout(volume->hookTimeout);
    sp.reporting(globalWarningMask & WARNING_VERBOSE, false);
    sp.capture(2, &hookLog, false);
    al.add(&sp);
    al.go();
    int hookrc = sp.getStatus();
    if(WIFEXITED(hookrc) && WEXITSTATUS(hookrc) == 0) {
      // The hook succeeded.
      pvh = PVH_RUN;
      // Hook output replaces the volume path
      if(output.size()) {
        if(output[output.size() - 1] == '\n')
          output.erase(output.size() - 1);
        volume->path = output;
      }
    } else {
      // The hook failed.
      pvh = PVH_FAILED;
      if(WIFEXITED(hookrc) && WEXITSTATUS(hookrc) == EX_TEMPFAIL) {
        // It's a harmless 'temporary' failure
        if(globalWarningMask & WARNING_VERBOSE)
          IO::out.writef("INFO: %s:%s is temporarily unavailable due to "
                         "pre-volume-hook\n",
                         host->name.c_str(), volume->name.c_str());
      } else {
        // It's a hard failure.
        ++globalErrors;
        if(hookLog.size() > 0 && hookLog.back() != '\n')
          hookLog += "\n";
        IO::err.writef("ERROR: %s:%s pre-volume-hook failed:\n%s",
                       host->name.c_str(), volume->name.c_str(),
                       hookLog.c_str());
      }
    }
  }
}

// Run the post-volume-hook for VOLUME, if the pre-volume-hook was run
// successfuly.
static void runPostVolumeHook(Volume *volume,
                              const PRE_VOLUME_HOOK_STATE &pvh) {
  Host *host = volume->parent;
  if(pvh == PVH_RUN && volume->postVolume.size()) {
    std::string hookLog;
    EventLoop e;
    ActionList al(&e);

    Subprocess sp("post-volume-hook/" + volume->parent->name + "/"
                      + volume->name,
                  volume->postVolume);
    sp.setenv("RSBACKUP_HOOK", "post-volume-hook");
    setEnvironment(volume, sp);
    sp.setTimeout(volume->hookTimeout);
    sp.reporting(globalWarningMask & WARNING_VERBOSE, false);
    sp.capture(2, &hookLog, 1);
    al.add(&sp);
    al.go();
    if(hookLog.size() && (globalWarningMask & WARNING_VERBOSE)) {
      IO::out.writef("ERROR: %s:%s post-volume-hook output:\n%s\n",
                     host->name.c_str(), volume->name.c_str(), hookLog.c_str());
    }
  }
}

// Backup VOLUME onto DEVICE.
//
// device->store is assumed to be set.
//
// The group lock is assumed to be held on entry, and stays held.
// The global lock is assumed to be held on entry. From time to
// time it will be transiently released while waiting for resource
// availability or (further down the call tree) during command execution.
// The device lock is assumed to be held on entry, and stays held.
static void backupVolumeToDevice(Volume *volume, Device *device,
                                 PRE_VOLUME_HOOK_STATE &pvh) {
  Host *host = volume->parent;
  runPreVolumeHook(volume, pvh);
  if(pvh == PVH_FAILED)
    return;
  assert(pvh == PVH_RUN);
  if(globalWarningMask & WARNING_VERBOSE)
    IO::out.writef("INFO: backup %s:%s to %s\n", host->name.c_str(),
                   volume->name.c_str(), device->name.c_str());
  MakeBackup mb(volume, device);
  mb.performBackup(volume->path);
}

// Backup VOLUME onto DEVICE, if possible.
//
// The group lock is assumed to be held on entry, and stays held.
// The global lock is assumed to be held on entry. From time to
// time it will be transiently released while waiting for resource
// availability or (further down the call tree) during command execution.
// The device lock is assumed to be held on entry, and stays held.
static void maybeBackupVolumeToDevice(Volume *volume, Device *device,
                                      PRE_VOLUME_HOOK_STATE &pvh) {
  Host *host = volume->parent;
  char buffer[1024];
  BackupRequirement br = volume->needsBackup(device);
  if(br == AlreadyBackedUp && globalCommand.force) {
    IO::out.writef("INFO: %s:%s is already backed up on %s, but backing up "
                   "anyway because --force\n",
                   host->name.c_str(), volume->name.c_str(),
                   device->name.c_str());
    br = BackupRequired;
  }
  switch(br) {
  case BackupRequired:
    globalConfig.identifyDevices(Store::Enabled);
    if(device->store && device->store->state == Store::Enabled)
      backupVolumeToDevice(volume, device, pvh);
    else if(globalWarningMask & WARNING_STORE) {
      globalConfig.identifyDevices(Store::Disabled);
      if(device->store)
        switch(device->store->state) {
        case Store::Disabled:
          warning(
              WARNING_STORE,
              "cannot backup %s:%s to %s - device suppressed due to --store",
              host->name.c_str(), volume->name.c_str(), device->name.c_str());
          break;
        default:
          snprintf(buffer, sizeof buffer,
                   "device %s store %s unexpected state %d",
                   device->name.c_str(), device->store->path.c_str(),
                   device->store->state);
          throw FatalStoreError(buffer);
        }
      else
        warning(WARNING_STORE,
                "cannot backup %s:%s to %s - device not available",
                host->name.c_str(), volume->name.c_str(), device->name.c_str());
    }
    break;
  case AlreadyBackedUp:
    if(globalWarningMask & WARNING_VERBOSE)
      IO::out.writef("INFO: %s:%s is already backed up on %s\n",
                     host->name.c_str(), volume->name.c_str(),
                     device->name.c_str());
    break;
  case NotAvailable:
    if(globalWarningMask & WARNING_VERBOSE)
      IO::out.writef("INFO: %s:%s is not available\n", host->name.c_str(),
                     volume->name.c_str());
    break;
  case NotThisDevice:
    if(globalWarningMask & WARNING_VERBOSE)
      IO::out.writef("INFO: %s:%s excluded from %s by device pattern\n",
                     host->name.c_str(), volume->name.c_str(),
                     device->name.c_str());
    break;
  }
}

// Backup VOLUME on all devices.
//
// The group lock is assumed to be held on entry, and stays held.
// The global lock is assumed to be held on entry. From time to
// time it will be transiently released while waiting for resource
// availability or (further down the call tree) during command execution.
static void backupVolumeToAllDevices(Volume *volume) {
  // Build a list of devices
  std::set<Device *> devices;
  for(auto &d: globalConfig.devices) {
    devices.insert(d.second);
  }
  PRE_VOLUME_HOOK_STATE pvh = PVH_NOT_RUN;
  while(devices.size() > 0 && pvh != PVH_FAILED) {
    bool worked = false;
    // Look for a device we can lock
    for(auto device: devices) {
      if(device->lock.try_lock()) {
        std::lock_guard<std::mutex> guard(device->lock, std::adopt_lock);
        maybeBackupVolumeToDevice(volume, device, pvh);
        devices.erase(device);
        worked = true;
        break;
      }
    }
    // If we didn't find a suitable volume wait a bit and try again
    if(!worked) {
      release_guard<std::mutex> globalRelease(globalLock);
      usleep(100 * 000 /*µs*/);
    }
  }
  runPostVolumeHook(volume, pvh);
}

// Backup HOST
static void backupHost(Host *host, std::mutex *lock) {
  // Do a quick check for unavailable hosts
  bool available = host->available();
  // Serialize host groups
  std::lock_guard<std::mutex> groupGuard(*lock);
  std::lock_guard<std::mutex> globalGuard(globalLock);
  if(!available) {
    warning(WARNING_UNREACHABLE, "cannot backup %s - not reachable",
            host->name.c_str());
    return;
  }
  for(auto &v: host->volumes) {
    Volume *volume = v.second;
    if(volume->selected(PurposeBackup))
      backupVolumeToAllDevices(volume);
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
    if(host->selected(PurposeBackup))
      hosts.push_back(host);
  }
  std::sort(hosts.begin(), hosts.end(), order_host);
  // Create concurrency group locks
  std::map<std::string, std::mutex *> locks;
  for(Host *h: hosts) {
    if(locks.find(h->group) == locks.end())
      locks[h->group] = new std::mutex();
  }
  // Initiate backups in threads
  std::map<std::string, std::thread *> threads;
  for(Host *h: hosts) {
    threads[h->name] = new std::thread(backupHost, h, locks[h->group]);
  }
  {
    // Release the global lock while we wait for the threads
    release_guard<std::mutex> globalRelease(globalLock);
    // Wait for the threads
    for(auto it: threads) {
      it.second->join();
    }
  }
  // Clean up the locks and threads
  for(auto it: threads) {
    delete it.second;
  }
  for(auto it: locks) {
    delete it.second;
  }
}
