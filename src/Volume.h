// -*-C++-*-
// Copyright © 2011, 2012, 2014-2016 Richard Kettlewell.
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
#ifndef VOLUME_H
#define VOLUME_H
/** @file Volume.h
 * @brief Configuration and state of a volume
 */

#include "ConfBase.h"

class Host;

/** @brief Type of an ordered set of backups
 * @see Volume::backups
 */
typedef std::set<Backup *,compare_backup> backups_type;

/** @brief Possible states of a volume */
enum BackupRequirement {
  /** @brief Volume already backed up */
  AlreadyBackedUp,

  /** @brief Device not usable */
  NotThisDevice,

  /** @brief Volume not usable */
  NotAvailable,

  /** @brief Volume can and should be backed up */
  BackupRequired
};

/** @brief Represents a single volume (usually, filesystem) to back up */
class Volume: public ConfBase {
public:
  /** @brief Construct a volume
   * @param parent_ Host containing volume
   * @param name_ Volume name
   * @param path_ Path to volume
   */
  Volume(Host *parent_,
         const std::string &name_,
         const std::string &path_);

  /** @brief Destructor */
  ~Volume();

  /** @brief Host containing volume */
  Host *parent;

  /** @brief Volume name */
  std::string name;

  /** @brief Path to volume */
  std::string path;

  /** @brief List of exclusion patterns for this volume */
  std::vector<std::string> exclude;

  /** @brief Traverse mount points if true */
  bool traverse = false;

  /** @brief File to check before backing up */
  std::string checkFile;

  /** @brief Check that root path is a mount point before backing up */
  bool checkMounted = false;

  /** @brief Return true if volume is selected */
  bool selected() const { return isSelected; }

  /** @brief (De-)select volume
   * @param sense true to select, false to de-select
   */
  void select(bool sense);

  /** @brief Check whether a proposed volume name is valid
   * @param n Proposed volume name
   * @return true if valid, else false
   */
  static bool valid(const std::string &n);

  /** @brief Test if volume available
   * @return true if volume is available
   */
  bool available() const;

  /** @brief Known backups of this volume */
  backups_type backups;

  /** @brief Per-device information about this volume */
  struct PerDevice {
    /** @brief Number of backups of volume on device */
    int count = 0;

    /** @brief Oldest backup of volume on device */
    Date oldest;

    /** @brief Newest backup of volume on device */
    Date newest;

    /** @brief Size of newest backup on device, or -1 if unknown */
    long long size;
  };

  /** @brief Number of completed backups */
  int completed = 0;

  /** @brief Date of oldest backup (on any device) */
  Date oldest;

  /** @brief Date of newest backup (on any device) */
  Date newest;

  /** @brief Type for @ref perDevice */
  typedef std::map<std::string, PerDevice> perdevice_type;

  /** @brief Map of device names to per-device information */
  perdevice_type perDevice;

  /** @brief Find the per-device information for @p device
   * @param device Device name
   * @return Per-device information or @c nullptr
   */
  const PerDevice *findDevice(const std::string &device) const {
    auto it = perDevice.find(device);
    return it != perDevice.end() ? &it->second : nullptr;
  }

  /** @brief Add a backup
   * @return @c true if the backup was inserted, @c false if already present */
  bool addBackup(Backup *backup);

  /** @brief Remove a backup */
  bool removeBackup(const Backup *backup);

  /** @brief Find the most recent backup
   * @param device If not null pointer, only consider backups from this device
   * @return Most recent backup or null pointer
   */
  const Backup *mostRecentBackup(const Device *device = nullptr) const;

  /** @brief Find the most recent failedbackup
   * @param device If not null pointer, only consider backups from this device
   * @return Most recent failed backup or null pointer
   */
  const Backup *mostRecentFailedBackup(const Device *device = nullptr) const;

  /** @brief Identify whether this volume needs backing up on a particular device
   * @param device Target device
   * @return Volume state
   */
  BackupRequirement needsBackup(Device *device);

  ConfBase *getParent() const override;

  std::string what() const override;

  void write(std::ostream &os, int step, bool verbose)
    const override;

private:
  /** @brief Set to @c true if this volume is selected */
  bool isSelected = false;

  /** @brief Recalculate statistics
   *
   * After calling this method the following members will accurately reflect
   * the contents of the @ref backups container:
   * - @ref completed
   * - @ref oldest
   * - @ref newest
   * - @ref perDevice
   *
   * @ref perDevice will not contain any entries with @ref PerDevice::count
   * equal to 0.
   */
  void calculate();

  friend void Backup::setStatus(int);
};

#endif /* VOLUME_H */
