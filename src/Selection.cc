// -*-C++-*-
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
#include "Selection.h"
#include "Errors.h"
#include "Conf.h"

Selection::Selection(const std::string &host_,
                     const std::string &volume_,
                     bool sense_): sense(sense_),
                                   host(host_),
                                   volume(volume_) {
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
  case '-': case '!':
    sense = false;
    ++pos;
    break;
  default:
    sense = true;
    break;
  }
  size_t colon = selection.find(':', pos);
  if(colon != std::string::npos) {
    // A host:volume pair
    selections.push_back(Selection(selection.substr(pos, colon-pos),
                                   selection.substr(colon + 1),
                                   sense));
  } else {
    // Just a host
    selections.push_back(Selection(selection.substr(pos),
                                   "*",
                                   sense));
  }
}

void VolumeSelections::select(Conf &config) const {
  if(selections.size() == 0)
    config.selectVolume("*", "*", true);
  for(auto &selection: selections)
    config.selectVolume(selection.host,
                        selection.volume,
                        selection.sense);
}
