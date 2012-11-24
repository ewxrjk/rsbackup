// Copyright © 2011, 2012 Richard Kettlewell.
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
#include <cstdio>

void Volume::select(bool sense) {
  isSelected = sense;
}

bool Volume::valid(const std::string &name) {
  return name.find_first_not_of(VOLUME_VALID) == std::string::npos;
}

void Volume::calculate() {
  completed = 0;
  for(std::set<Backup *>::const_iterator it = backups.begin();
      it != backups.end();
      ++it) {
    const Backup *s = *it;
    // Only count complete backups
    if(s->rc == 0) {
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
}

void Volume::addBackup(Backup *backup) {
  backups.insert(backup);
  calculate();
}

bool Volume::removeBackup(const Backup *backup) {
  for(std::set<Backup *>::const_iterator it = backups.begin();
      it != backups.end();
      ++it) {
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
  const Backup *result = NULL;
  for(std::set<Backup *>::const_iterator it = backups.begin();
      it != backups.end();
      ++it) {
    const Backup *b = *it;
    if(!device || b->getDevice() == device) {
      if(!result || *result < *b)
        result = b;
    }
  }
  return result;
}

const Backup *Volume::mostRecentFailedBackup(const Device *device) const {
  const Backup *result = NULL;
  for(std::set<Backup *>::const_iterator it = backups.begin();
      it != backups.end();
      ++it) {
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
  if(!checkFile.size())
    return true;
  std::vector<std::string> cmd;
  if(parent->hostname != "localhost") {
    cmd.push_back("ssh");
    if(parent->parent->sshTimeout > 0) {
      char buffer[64];
      snprintf(buffer, sizeof buffer, "%d", parent->parent->sshTimeout);
      cmd.push_back(std::string("-oConnectTimeout=") + buffer);
    }
    cmd.push_back(parent->userAndHost());
  }
  cmd.push_back("test");
  cmd.push_back("-e");
  cmd.push_back(checkFile[0] == '/' ? checkFile : path + "/" + checkFile);
  Subprocess sp(cmd);
  sp.nullChildFD(1);
  sp.nullChildFD(2);
  int rc = sp.runAndWait(false);
  return rc == 0;
}
