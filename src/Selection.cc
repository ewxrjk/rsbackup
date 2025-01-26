// Copyright © 2015, 2016 Richard Kettlewell.
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
#include "Selection.h"
#include "Errors.h"
#include "Conf.h"
#include "Backup.h"
#include "Volume.h"
#include "Host.h"
#include <fnmatch.h>

Selection::Selection(const std::string &host_, const std::string &volume_,
                     bool sense_):
    sense(sense_),
    host(host_), volume(volume_) {
  if(!Host::valid(host) && host != "*")
    throw CommandError("invalid host: '" + host + "'");
  if(!Volume::valid(volume) && volume != "*")
    throw CommandError("invalid volume: '" + volume + "'");
  if(host == "*" && volume != "*")
    throw CommandError("invalid host: '" + host + "'");
}

void VolumeSelections::add(const std::string &selection) {
  // Establish the sense of this entry
  bool sense;
  if(selection.size() == 0)
    throw CommandError("invalid selection");
  size_t pos = 0;
  switch(selection.at(pos)) {
  case '-':
  case '!':
    sense = false;
    ++pos;
    break;
  default: sense = true; break;
  }
  size_t colon = selection.find(':', pos);
  if(colon != std::string::npos) {
    // A host:volume pair
    selections.push_back(Selection(selection.substr(pos, colon - pos),
                                   selection.substr(colon + 1), sense));
  } else {
    // Just a host
    selections.push_back(Selection(selection.substr(pos), "*", sense));
  }
}

void VolumeSelections::select(Conf &config, const std::string &hosts,
                              const std::string &volumes,
                              SelectionPurpose purpose, bool sense,
                              int *current_time) {
  for(auto hosts_iterator: config.hosts) {
    Host &host = *hosts_iterator.second;
    if(fnmatch(hosts.c_str(), host.name.c_str(), 0) == FNM_NOMATCH)
      continue;
    for(auto volume_iterator: host.volumes) {
      Volume &volume = *volume_iterator.second;
      if(fnmatch(volumes.c_str(), volume.name.c_str(), 0) == FNM_NOMATCH)
        continue;
      if(current_time) {
        if(*current_time < volume.earliest || *current_time > volume.latest)
          continue;
      }
      volume.select(purpose, sense);
    }
  }
}

void VolumeSelections::select(Conf &config) const {
  if(selections.size() == 0) {
    time_t now = Date::now("BACKUP");
    struct tm tmnow;

    if(!localtime_r(&now, &tmnow))
      throw SystemError("localtime_r", errno);
    int current_time = 3600 * tmnow.tm_hour + 60 * tmnow.tm_min + tmnow.tm_sec;
    for(auto purpose = 0; purpose < PurposeMax; purpose++)
      select(config, "*", "*", SelectionPurpose(purpose), true,
             purpose == PurposeBackup ? &current_time : nullptr);
  } else {
    for(auto &selection: selections) {
      for(auto purpose = 0; purpose < PurposeMax; purpose++)
        select(config, selection.host, selection.volume,
               SelectionPurpose(purpose), selection.sense);
    }
  }
}
