// -*-C++-*-
// Copyright © Richard Kettlewell.
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
#ifndef BACKUP_H
#define BACKUP_H
/** @file Backup.h
 * @brief State of a backup
 */

#include <string>
#include "Date.h"

class Database;
class Volume;
class Device;

/** @brief Possible states of a backup */
enum BackupStatus {
  /** @brief %Backup status unknown */
  UNKNOWN = 0,

  /** @brief %Backup is underway */
  UNDERWAY = 1,

  /** @brief %Backup is complete */
  COMPLETE = 2,

  /** @brief %Backup failed */
  FAILED = 3,

  /** @brief Pruning is underway */
  PRUNING = 4,

  /** @brief Pruning is complete */
  PRUNED = 5
};

/** @brief Names of @ref BackupStatus constants */
extern const char *const backup_status_names[];

/** @brief Represents the status of one backup */
class Backup {
  /** @brief Status of this backup
   *
   * @see BackupStatus
   */
  int status = UNKNOWN;

public:
  /** @brief Wait status from @c rsync
   *
   * This is a wait status as returned by @c waitpid and similar functions.  0
   * means the backup succeeded.  It is meaningless if @ref Backup::status is
   * @ref UNKNOWN or @ref UNDERWAY.
   */
  int waitStatus = 0;

  /** @brief Id of backup
   *
   * In the current implementation these are days represented by YYYY-MM-DD
   * format.  However, IDs are treated as opaque strings, so this could be
   * changed.
   *
   * Note though that when importing logs from older versions of rsbackup (in
   * Conf::readState) the filename is assumed to start YYYY-MM-DD and this is
   * imported as both the time of the backup and its ID.
   */
  std::string id;

  /** @brief Time of backup
   *
   * This reflects the time that the backup was started.
   */
  time_t time = 0;

  /** @brief Finish Time of backup
   *
   * The time that the backup was finished (if it finished).
   *
   * If the database was created prior to version 11.0, this may be 0.
   */
  time_t finishTime = 0;

  /** @brief Time backup pruned
   *
   * The meaning of this member depends on the value of @ref Backup::status
   *
   * If the current status is @ref PRUNING then it reflects the time that it
   * was decided to prune the backup.
   *
   * If the current status is @ref PRUNED then it reflects the time that the
   * pruning operation completed.
   *
   * For any other status, the value is meaningless. */
  time_t pruned = 0;

  /** @brief Device containing backup */
  std::string deviceName;

  /** @brief Log contents */
  std::string contents;

  /** @brief Volume backed up */
  Volume *volume = nullptr;

  /** @brief Ordering on backups
   * @param that Other backup
   * @return @c true if this sorts earlier than @p that
   *
   * Backups are ordered by date first and by device name for backups of the
   * same date.
   */
  inline bool operator<(const Backup &that) const {
    int c;
    if((c = time - that.time))
      return c < 0;
    if((c = deviceName.compare(that.deviceName)))
      return c < 0;
    return false;
  }

  /** @brief Return path to backup */
  std::string backupPath() const;

  /** @brief Return containing device
   *
   * This can be a null pointer if device is no longer mentioned in the
   * configuration file. In theory the operator should retire such devices,
   * but we can't enforce that.
   */
  Device *getDevice() const;

  /** @brief Insert this backup into the database
   * @param db Database to update
   * @param replace Replace existing row if present
   */
  void insert(Database &db, bool replace = false) const;

  /** @brief Update this backup in the database
   * @param db Database to update
   */
  void update(Database &db) const;

  /** @brief Remove this backup from the database
   * @param db Database to update
   */
  void remove(Database &db) const;

  /** @brief Retrieve status of this backup
   * @return Status (see @ref BackupStatus)
   */
  inline int getStatus() const {
    return status;
  }

  /** @brief Return a size estimate for this backup
   * @return Size in bytes, or -1 if no estimate is available
   */
  long long getSize() const;

  /** @brief Set the status of this backup
   * @param n New status (see @ref BackupStatus)
   *
   * Calls Volume::calculate if necessary. */
  void setStatus(int n);
};

/** @brief Comparison for backup pointers */
struct compare_backup {
  /** @brief Comparison operator
   * @param a A backup
   * @param b Another backup
   * @return @c true if @p a sorts earlier than @p b
   *
   * Backups are ordered by date first and by device name for backups of the
   * same date.
   */
  bool operator()(Backup *a, Backup *b) const {
    return *a < *b;
  }
};

#endif /* BACKUP_H */
