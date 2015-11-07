// Copyright © 2011-2014 Richard Kettlewell.
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
#include "Subprocess.h"
#include "Utils.h"
#include <cstdio>
#include <ostream>

void Volume::select(bool sense) {
  isSelected = sense;
}

bool Volume::valid(const std::string &name) {
  return name.size() > 0
    && name.at(0) != '-'
    && name.find_first_not_of(VOLUME_VALID) == std::string::npos;
}

void Volume::calculate() {
  completed = 0;
  for(auto it = perDevice.begin(); it != perDevice.end(); ++it) {
    Volume::PerDevice &pd = it->second;
    pd.count = 0;
  }
  for(auto it = backups.begin(); it != backups.end(); ++it) {
    const Backup *s = *it;
    // Only count complete backups which aren't going to be pruned
    if(s->getStatus() == COMPLETE) {
      // Global figures
      ++completed;
      if(completed == 1 || s->date < oldest)
        oldest = s->date;
      if(completed == 1 || s->date > newest)
        newest = s->date;

      // Per-device figures
      Volume::PerDevice &pd = perDevice[s->deviceName];
      ++pd.count;
      if(pd.count == 1 || s->date < pd.oldest)
        pd.oldest = s->date;
      if(pd.count == 1 || s->date > pd.newest)
        pd.newest = s->date;
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

void Volume::addBackup(Backup *backup) {
  backups.insert(backup);
  calculate();
}

bool Volume::removeBackup(const Backup *backup) {
  for(auto it = backups.begin(); it != backups.end(); ++it) {
    const Backup *s = *it;
    if(s == backup) {
      backups.erase(it);
      // Recalculate totals
      calculate();
      return true;
    }
  }
  return false;
}

const Backup *Volume::mostRecentBackup(const Device *device) const {
  const Backup *result = nullptr;
  for(auto it = backups.begin(); it != backups.end(); ++it) {
    const Backup *b = *it;
    if(!device || b->getDevice() == device) {
      if(!result || *result < *b)
        result = b;
    }
  }
  return result;
}

const Backup *Volume::mostRecentFailedBackup(const Device *device) const {
  const Backup *result = nullptr;
  for(auto it = backups.begin(); it != backups.end(); ++it) {
    const Backup *b = *it;
    if(!device || b->getDevice() == device) {
      if(b->rc)
        if(!result || *result < *b)
          result = b;
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
    if(parent->invoke(&os,
                      "uname", "-s", (const char *)nullptr) != 0)
      return false;
    if(os == "Darwin"
       || (os.size() >= 3
           && os.compare(os.size() - 3, 3, "BSD") == 0)) {
      option = "-f";
    } else {
      // For everything else assume coreutils stat(1)
      option = "-c";
    }
    // Get the device numbers for path and its parent
    if(parent->invoke(&stats,
                      "stat", option, "%d",
                      path.c_str(),
                      parent_directory.c_str(),
                      (const char *)nullptr))
      return false;
    // Split output into lines
    std::vector<std::string> lines;
    toLines(lines, stats);
    // If stats is malformed, or if device numbers match (implying path is not
    // a mount point), volume is not available.
    if(lines.size() != 2
       || lines[0].size() == 0
       || lines[1].size() == 0
       || lines[0] == lines[1])
      return false;
  }
  if(checkFile.size()) {
    std::string file = (checkFile[0] == '/'
                        ? checkFile
                        : path + "/" + checkFile);
    if(parent->invoke(nullptr,
                      "test", "-e", file.c_str(), (const char *)nullptr) != 0)
      return false;
  }
  return true;
}

void Volume::write(std::ostream &os, int step) const {
  os << indent(step) << "volume " << quote(name) << ' ' << quote(path) << '\n';
  step += 4;
  ConfBase::write(os, step);
  if(devicePattern.size())
    os << indent(step) << "devices " << quote(devicePattern) << '\n';
  for(size_t n = 0; n < exclude.size(); ++n)
    os << indent(step) << "exclude " << quote(exclude[n]) << '\n';
  if(traverse)
    os << indent(step) << "traverse" << '\n';
  if(checkFile.size())
    os << indent(step) << "check-file " << quote(checkFile) << '\n';
  if(checkMounted)
    os << indent(step) << "check-mounted\n";
}

ConfBase *Volume::getParent() const {
  return parent;
}
