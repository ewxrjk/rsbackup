// Copyright © 2011, 2012, 2014-2016, 2019 Richard Kettlewell.
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
#include "Volume.h"
#include "Errors.h"
#include "PrunePolicy.h"
#include <cassert>

PrunePolicy::PrunePolicy(const std::string &name) {
  if(!policies)
    policies = new policies_type();
  (*policies)[name] = this;
}

const std::string &PrunePolicy::get(const Volume *volume,
                                    const std::string &name) const {
  auto it = volume->pruneParameters.find(name);
  if(it != volume->pruneParameters.end())
    return it->second;
  else
    throw ConfigError("missing pruning parameter '" + name + "'");
}

const std::string &PrunePolicy::get(const Volume *volume,
                                    const std::string &name,
                                    const std::string &def) const {
  auto it = volume->pruneParameters.find(name);
  if(it != volume->pruneParameters.end())
    return it->second;
  else
    return def;
}

const PrunePolicy *PrunePolicy::find(const std::string &name) {
  assert(policies != nullptr); // policies not statically initialized
  auto it = policies->find(name);
  if(it == policies->end())
    throw ConfigError("unrecognized pruning policy '" + name + "'");
  return it->second;
}

void validatePrunePolicy(const Volume *volume) {
  const PrunePolicy *policy = PrunePolicy::find(volume->prunePolicy);
  policy->validate(volume);
}
