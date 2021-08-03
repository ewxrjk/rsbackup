// Copyright © 2011-2016 Richard Kettlewell.
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
#include "Conf.h"
#include "Device.h"
#include "Backup.h"
#include "Volume.h"
#include "Host.h"
#include "Subprocess.h"
#include "Utils.h"
#include "Store.h"
#include "BackupPolicy.h"
#include <cstdio>
#include <ostream>
#include <fnmatch.h>

Volume::Volume(Host *parent_, const std::string &name_,
               const std::string &path_):
    ConfBase(static_cast<ConfBase *>(parent_)),
    parent(parent_), name(name_), path(path_) {
  parent->addVolume(this);
}

Volume::~Volume() {
  deleteAll(backups);
}

void Volume::select(bool sense) {
  isSelected = sense;
}

bool Volume::valid(const std::string &name) {
  return name.size() > 0 && name.at(0) != '-'
         && name.find_first_not_of(VOLUME_VALID) == std::string::npos;
}

void Volume::calculate() {
  completed = 0;
  for(auto &pd: perDevice)
    pd.second.count = 0;
  for(const Backup *backup: backups) {
    // Only count complete backups which aren't going to be pruned
    if(backup->getStatus() == COMPLETE) {
      // Global figures
      ++completed;
      if(completed == 1 || backup->time < oldest)
        oldest = backup->time;
      if(completed == 1 || backup->time > newest)
        newest = backup->time;

      // Per-device figures
      Volume::PerDevice &pd = perDevice[backup->deviceName];
      ++pd.count;
      if(pd.count == 1 || backup->time < pd.oldest)
        pd.oldest = backup->time;
      if(pd.count == 1 || backup->time > pd.newest) {
        pd.newest = backup->time;
        pd.size = backup->getSize();
      }
    }
  }
  for(auto it = perDevice.begin(); it != perDevice.end();) {
    auto jt = it;
    ++it;
    Volume::PerDevice &pd = jt->second;
    if(!pd.count)
      perDevice.erase(jt);
  }
}

bool Volume::addBackup(Backup *backup) {
  bool inserted = backups.insert(backup).second;
  if(inserted)
    calculate();
  return inserted;
}

bool Volume::removeBackup(const Backup *backup) {
  for(auto it = backups.begin(); it != backups.end(); ++it) {
    Backup *s = *it;
    if(s == backup) {
      backups.erase(it);
      delete s;
      // Recalculate totals
      calculate();
      return true;
    }
  }
  return false;
}

const Backup *Volume::mostRecentBackup(const Device *device) const {
  const Backup *result = nullptr;
  for(const Backup *backup: backups) {
    // Note that if we ask for 'any device', i.e. device=nullptr,
    // we can get backups on devices not mentioned in the config file.
    if(!device || backup->getDevice() == device) {
      if(!result || *result < *backup)
        result = backup;
    }
  }
  return result;
}

const Backup *Volume::mostRecentFailedBackup(const Device *device) const {
  const Backup *result = nullptr;
  for(const Backup *backup: backups) {
    if(!device || backup->getDevice() == device) {
      switch(backup->getStatus()) {
      case COMPLETE:
      case PRUNING:
      case PRUNED: break;
      case UNKNOWN:
      case FAILED:
      case UNDERWAY:
        if(!result || *result < *backup)
          result = backup;
        break;
      }
    }
  }
  return result;
}

bool Volume::available() const {
  if(checkMounted) {
    std::string os, stats;
    std::string parent_directory = path + "/..";
    const char *option;
    // Guess which version of stat to use based on uname.
    if(parent->invoke(&os, "uname", "-s", (const char *)nullptr) != 0)
      return false;
    if(os == "Darwin"
       || (os.size() >= 3 && os.compare(os.size() - 3, 3, "BSD") == 0)) {
      option = "-f";
    } else {
      // For everything else assume coreutils stat(1)
      option = "-c";
    }
    // Get the device numbers for path and its parent
    if(parent->invoke(&stats, "stat", option, "%d", path.c_str(),
                      parent_directory.c_str(), (const char *)nullptr))
      return false;
    // Split output into lines
    std::vector<std::string> lines;
    toLines(lines, stats);
    // If stats is malformed, or if device numbers match (implying path is not
    // a mount point), volume is not available.
    if(lines.size() != 2 || lines[0].size() == 0 || lines[1].size() == 0
       || lines[0] == lines[1])
      return false;
  }
  if(checkFile.size()) {
    std::string file =
        (checkFile[0] == '/' ? checkFile : path + "/" + checkFile);
    if(parent->invoke(nullptr, "test", "-e", file.c_str(),
                      (const char *)nullptr)
       != 0)
      return false;
  }
  return true;
}

BackupRequirement Volume::needsBackup(Device *device) {
  switch(fnmatch(devicePattern.c_str(), device->name.c_str(), FNM_NOESCAPE)) {
  case 0: break;
  case FNM_NOMATCH: return NotThisDevice;
  default:
    warning(WARNING_ALWAYS, "invalid device pattern '%s'",
            devicePattern.c_str());
    /* fail safe - make the backup */
    break;
  }
  const BackupPolicy *policy = BackupPolicy::find(backupPolicy);
  if(!policy->backup(this, device))
    return AlreadyBackedUp;
  if(!available())
    return NotAvailable;
  return BackupRequired;
}

void Volume::write(std::ostream &os, int step, bool verbose) const {
  describe_type *d = verbose ? describe : nodescribe;

  os << indent(step) << "volume " << quote(name) << ' ' << quote(path) << '\n';
  step += 4;
  ConfBase::write(os, step, verbose);
  d(os, "", step);

  d(os, "# Glob pattern for devices this host will be backed up to", step);
  d(os, "#  devices PATTERN", step);
  if(devicePattern.size())
    os << indent(step) << "devices " << quote(devicePattern) << '\n';
  d(os, "", step);

  d(os, "# Paths to exclude from backup", step);
  d(os,
    "# Patterns are glob patterns, starting at the root of the volume as '/'.",
    step);
  d(os, "# '*' matches multiple characters but not '/'", step);
  d(os, "# '**' matches multiple characters including '/'", step);
  d(os, "# Consult rsync manual for full pattern syntax", step);
  d(os, "#   exclude PATTERN", step);
  for(const std::string &exclusion: exclude)
    os << indent(step) << "exclude " << quote(exclusion) << '\n';
  d(os, "", step);

  d(os, "# Back up across mount points", step);
  d(os, "#  traverse true|false", step);
  os << indent(step) << "traverse " << (traverse ? "true" : "false") << '\n';
  d(os, "", step);

  d(os, "# Check that a named file exists before performing backup", step);
  d(os, "#  check-file PATH", step);
  if(checkFile.size())
    os << indent(step) << "check-file " << quote(checkFile) << '\n';
  d(os, "", step);

  d(os, "# Check that volume is a mount point before performing backup", step);
  d(os, "#  check-mounted true|false", step);
  os << indent(step) << "check-mounted " << (checkMounted ? "true" : "false")
     << '\n';
}

ConfBase *Volume::getParent() const {
  return parent;
}

std::string Volume::what() const {
  return "volume";
}
