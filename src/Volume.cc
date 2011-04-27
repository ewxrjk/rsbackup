#include <config.h>
#include "Conf.h"

void Volume::select(bool sense) {
  isSelected = sense;
}

bool Volume::valid(const std::string &name) {
  return name.find_first_not_of(VOLUME_VALID) == std::string::npos;
}

void Volume::calculate() {
  completed = 0;
  for(std::set<Status>::const_iterator it = backups.begin();
      it != backups.end();
      ++it) {
    const Status &s = *it;
    // Only count complete backups
    if(s.rc == 0) {
      // Global figures
      ++completed;
      if(completed == 1 || s.date < oldest)
        oldest = s.date;
      if(completed == 1 || s.date > newest)
        newest = s.date;

      // Per-device figures
      Volume::PerDevice &pd = perDevice[s.deviceName];
      ++pd.count;
      if(pd.count == 1 || s.date < pd.oldest)
        pd.oldest = s.date;
      if(pd.count == 1 || s.date > pd.newest)
        pd.newest = s.date;
    }
  }
}

void Volume::addBackup(const Status &status) {
  backups.insert(status);
  calculate();
}

bool Volume::removeBackup(const Status *status) {
  for(std::set<Status>::const_iterator it = backups.begin();
      it != backups.end();
      ++it) {
    const Status &s = *it;
    if(&s == status) {
      backups.erase(it);
      // Recalculate totals
      calculate();
      return true;
    }
  }
  return false;
}
