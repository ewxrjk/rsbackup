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

#include <map>

class Backup;
class Volume;

/** @brief Base class for pruning policies
 */
class PrunePolicy {
public:
  /** @brief Constructor
   *
   * Policies are automatically registered upon construction.
   */
  PrunePolicy(const std::string &name);

  /** @brief Validate a pruning policy
   * @param volume Volume to validate
   */
  virtual void validate(const Volume *volume) const = 0;

  /** @brief Get a parameter value
   * @param volume Volume to validate
   * @param name Name of parameter
   */
  const std::string &get(const Volume *volume,
                         const std::string &name) const;

  /** @brief Get a parameter value
   * @param volume Volume to validate
   * @param name Name of parameter
   * @param def Default value
   */
  const std::string &get(const Volume *volume,
                         const std::string &name,
                         const std::string &def) const;

  /** @brief Identify prunable backups
   * @param onDevice Surviving backups of same volume on same device
   * @param total Number of backups anywhere
   * @param prune Map of backups to prune to reason strings
   *
   * @p total does not include backups on other devices that have "only just"
   * been selected for pruning.
   */
  virtual void prunable(std::vector<Backup *> &onDevice,
                        std::map<Backup *, std::string> &prune,
                        int total) const = 0;

  /** @brief Find a prune policy by name
   * @param name Name of policy
   * @return Prune policy
   */
  static const PrunePolicy *find(const std::string &name);

private:
  typedef std::map<std::string,const PrunePolicy *> policies_type;
  static policies_type *policies;
};

/** @brief Validate a pruning policy
 * @param volume Volume to validate
 */
void validatePrunePolicy(const Volume *volume);

/** @brief Identify prunable backups
 * @param onDevice Number of backups of same volume on same device
 * @param prune Map of backups to prune to reason strings
 * @param total Number of backups anywhere
  */
void backupPrunable(std::vector<Backup *> &onDevice,
                    std::map<Backup *, std::string> &prune,
                    int total);

#endif /* PRUNE_H */
