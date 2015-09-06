// Copyright © 2011, 2012, 2014 Richard Kettlewell.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
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
#include "Prune.h"
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>

PrunePolicy::PrunePolicy(const std::string &name) {
  if(!policies)
    policies = new policies_type();
  (*policies)[name] = this;
}

const std::string &PrunePolicy::get(const Volume *volume,
				    const std::string &name) const {
  std::map<std::string,std::string>::const_iterator it
    = volume->pruneParameters.find(name);
  if(it != volume->pruneParameters.end())
    return it->second;
  else
    throw ConfigError("missing pruning parameter '" + name + "'");
}

const std::string &PrunePolicy::get(const Volume *volume,
                                    const std::string &name,
                                    const std::string &def) const {
  std::map<std::string,std::string>::const_iterator it
    = volume->pruneParameters.find(name);
  if(it != volume->pruneParameters.end())
    return it->second;
  else
    return def;
}

const PrunePolicy *PrunePolicy::find(const std::string &name) {
  policies_type::const_iterator it = policies->find(name);
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
  return policy->prunable(onDevice, prune, total);
}

PrunePolicy::policies_type *PrunePolicy::policies;

// Remove old and incomplete backups
void pruneBackups() {
  // Make sure all state is available
  config.readState();

  // Figure out which backups are obsolete, if any
  std::vector<Backup *> oldBackups;
  for(hosts_type::const_iterator hostsIterator = config.hosts.begin();
      hostsIterator != config.hosts.end();
      ++hostsIterator) {
    const Host *host = hostsIterator->second;
    if(!host->selected())
      continue;
    for(volumes_type::const_iterator volumesIterator = host->volumes.begin();
	volumesIterator != host->volumes.end();
	++volumesIterator) {
      Volume *volume = volumesIterator->second;
      if(!volume->selected())
	continue;
      // For each device, the complete backups on that device
      std::map<std::string, std::vector<Backup *>> onDevices;
      for(backups_type::const_iterator backupsIterator = volume->backups.begin();
	  backupsIterator != volume->backups.end();
	  ++backupsIterator) {
	Backup *backup = *backupsIterator;
	switch(backup->getStatus()) {
	case UNKNOWN:
	case UNDERWAY:
	case FAILED:
	  if(command.pruneIncomplete) {
	    // Prune incomplete backups.  Anything that failed is counted as
	    // incomplete (a succesful retry will overwrite the log entry).
	    backup->contents = std::string("status=")
	      + backup_status_names[backup->getStatus()];
	    oldBackups.push_back(backup);
	  }
	  break;
	case PRUNING:
	  // Both commands continue pruning anything that has started being
	  // pruned.  log should already be set.
	  oldBackups.push_back(backup);
	  break;
	case PRUNED:
	  break;
	case COMPLETE:
          if(command.prune)
            onDevices[backup->deviceName].push_back(backup);
	  break;
	}
      }
      for(auto it = onDevices.begin(); it != onDevices.end(); ++it) {
        std::vector<Backup *> &onDevice = it->second;
        std::map<Backup *, std::string> prune;
        backupPrunable(onDevice, prune, 0/*TODO*/);
        for(auto it = prune.begin(); it != prune.end(); ++it) {
          Backup *backup = it->first;
          backup->contents = it->second;
          oldBackups.push_back(backup);
        }
      }
    }
  }

  // Return straight away if there's nothing to do
  if(oldBackups.size() == 0)
    return;

  // Update the status of everything we're pruning
  if(command.act) {
    config.getdb()->begin();
    for(std::vector<Backup *>::iterator it = oldBackups.begin();
	it != oldBackups.end(); ++it) {
      Backup *b = *it;
      if(b->getStatus() != PRUNING) {
	b->setStatus(PRUNING);
	b->pruned = Date::now();
	b->update(config.getdb());
      }
    }
    config.getdb()->commit();
    // We don't catch DatabaseBusy here; the prune just fails.
  }

  // Identify devices
  config.identifyDevices(Store::Enabled);

  // Delete obsolete backups
  for(size_t n = 0; n < oldBackups.size(); ++n) {
    Backup *backup = oldBackups[n];
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
		       backup->contents.c_str());
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
	    backup->setStatus(PRUNED);
	    backup->pruned = Date::now();
	    backup->update(config.getdb());
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
    } catch(std::runtime_error &exception) {
      // Log anything that goes wrong
      error("failed to remove %s: %s\n", backupPath.c_str(), exception.what());
      ++errors;
    }
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
