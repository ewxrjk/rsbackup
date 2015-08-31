// Copyright © 2015 Richard Kettlewell.
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
#include "Prune.h"
#include "Subprocess.h"
#include "Errors.h"
#include <unistd.h>
#include <sstream>

/** @brief Pruning policy that executes a program */
class PruneExec: public PrunePolicy {
public:
  PruneExec(): PrunePolicy("exec") {}

  void validate(const Volume *volume) const {
    const std::string &path = get(volume, "path");
    if(access(path.c_str(), X_OK) < 0)
      throw ConfigError("cannot execute pruning policy "
                        + volume->prunePolicy);
    for(std::map<std::string,std::string>::const_iterator it = volume->pruneParameters.begin();
        it != volume->pruneParameters.end();
        ++it) {
      const std::string &name = it->first;
      for(size_t i = 0; i < name.size(); ++i) {
        char ch = name.at(i);
        if(ch != '_' && !isalnum(ch))
          throw ConfigError("invalid pruning parameter '" + name + "' for executable policies");
      }
    }
  }

  bool prunable(const Backup *backup,
                std::vector<const Backup *> &onDevice,
                int total,
                std::string &reason) const {
    char buffer[64];
    const Volume *volume = backup->volume;
    std::vector<std::string> command;
    command.push_back(get(volume, "path"));
    Subprocess sp(command);
    for(std::map<std::string,std::string>::const_iterator it = volume->pruneParameters.begin();
        it != volume->pruneParameters.end();
        ++it)
      sp.setenv("PRUNE_" + it->first, it->second);
    std::stringstream ss;
    for(size_t i = 0; i < onDevice.size(); ++i) {
      if(i)
        ss << ' ';
      ss << Date::today() - onDevice[i]->date;
    }
    sp.setenv("PRUNE_ONDEVICE", ss.str());
    snprintf(buffer, sizeof buffer, "%d", total);
    sp.setenv("PRUNE_TOTAL", buffer);
    sp.setenv("PRUNE_HOST", volume->parent->name);
    sp.setenv("PRUNE_VOLUME", volume->name);
    snprintf(buffer, sizeof buffer, "%d", Date::today() - backup->date);
    sp.setenv("PRUNE_AGE", buffer);
    sp.setenv("PRUNE_DEVICE", backup->deviceName);
    sp.capture(1, &reason);
    sp.runAndWait();
    while(reason.size() > 0 && isspace(reason.back()))
      reason.pop_back();
    return reason.size() > 0;
  }
} prune_exec;
