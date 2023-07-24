// Copyright © 2019 Richard Kettlewell.
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
#include "Backup.h"
#include "Volume.h"
#include "BackupPolicy.h"
#include "Errors.h"
#include <cassert>

BackupPolicy::BackupPolicy(const std::string &name) {
  if(!policies)
    policies = new policies_type();
  (*policies)[name] = this;
}

const PolicyParameter &BackupPolicy::get(const Volume *volume,
                                         const std::string &name) const {
  auto it = volume->backupParameters.find(name);
  if(it != volume->backupParameters.end())
    return it->second;
  else
    throw ConfigError("missing backup parameter '" + name + "'");
}

const PolicyParameter BackupPolicy::get(const Volume *volume,
                                        const std::string &name,
                                        const std::string &def) const {
  auto it = volume->backupParameters.find(name);
  if(it != volume->backupParameters.end())
    return it->second;
  else
    return def;
}

const BackupPolicy *BackupPolicy::find(const std::string &name) {
  assert(policies != nullptr); // policies not statically initialized
  auto it = policies->find(name);
  if(it == policies->end())
    throw ConfigError("unrecognized backup policy '" + name + "'");
  return it->second;
}

void validateBackupPolicy(const Volume *volume) {
  const BackupPolicy *policy = BackupPolicy::find(volume->backupPolicy);
  policy->validate(volume);
}

BackupPolicy::policies_type *BackupPolicy::policies;
