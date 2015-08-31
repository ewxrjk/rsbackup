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
#include "Prune.h"

/** @brief The @c never pruning policy */
class PruneNever: public PrunePolicy {
public:
  PruneNever(): PrunePolicy("never") {}

  void validate(const Volume *) const {}

  bool prunable(const Backup *,
                std::vector<const Backup *> &,
                int,
                std::string &) const {
    return false;
  }
} prune_never;
