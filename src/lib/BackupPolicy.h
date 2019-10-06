// -*-C++-*-
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

#ifndef BACKUPPOLICY_H
#define BACKUPPOLICY_H
/** @file BackupPolicy.h
 * @brief Definitions used by the backup policies
 */

#include <map>
#include <string>

class Volume;
class Device;

/** @brief Base class for backup policies
 */
class BackupPolicy {
public:
  /** @brief Constructor
   *
   * Policies are automatically registered upon construction.
   */
  BackupPolicy(const std::string &name);

  /** @brief Validate a backup policy
   * @param volume Volume to validate
   */
  virtual void validate(const Volume *volume) const = 0;

  /** @brief Get a parameter value
   * @param volume Volume to get parameter from
   * @param name Name of parameter
   */
  const std::string &get(const Volume *volume, const std::string &name) const;

  /** @brief Get a parameter value
   * @param volume Volume to get parameter from
   * @param name Name of parameter
   * @param def Default value
   */
  const std::string &get(const Volume *volume, const std::string &name,
                         const std::string &def) const;

  /** @brief Find a backup policy by name
   * @param name Name of policy
   * @return Backup policy
   */
  static const BackupPolicy *find(const std::string &name);

  /** @brief Determine whether to back up a volume
   * @param volume Volume to consider
   * @param device Target device
   * @return @c return to back up and @c false to skip
   */
  virtual bool backup(const Volume *volume, const Device *device) const = 0;

private:
  /** @brief Type for @ref policies */
  typedef std::map<std::string, const BackupPolicy *> policies_type;

  /** @brief Map of policy names to implementations */
  static policies_type *policies;
};

void validateBackupPolicy(const Volume *volume);

#endif /* BACKUPPOLICY_H */
