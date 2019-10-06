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

#ifndef PRUNE_H
#define PRUNE_H
/** @file Prune.h
 * @brief Definitions used by the pruning logic
 */

#include <vector>
#include <map>
#include <string>

class Backup;

/** @brief Identify prunable backups
 * @param onDevice Number of backups of same volume on same device
 * @param prune Map of backups to prune to reason strings
 * @param total Number of backups anywhere
 */
void backupPrunable(std::vector<Backup *> &onDevice,
                    std::map<Backup *, std::string> &prune, int total);

/** @brief Identify the bucket for a backup
 * @param w Decay window
 * @param s Decay scale
 * @param a Age of backup
 * @return Bucket number from 0
 *
 * See <a
 * href="https://www.greenend.org.uk/rjk/rsbackup/decay.pdf">decay.pdf</a> for
 * more information.
 */
int prune_decay_bucket(double w, double s, int a);

#endif /* PRUNE_H */
