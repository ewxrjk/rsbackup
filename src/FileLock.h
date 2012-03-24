//-*-C++-*-
// Copyright © 2011, 2012 Richard Kettlewell.
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
#ifndef FILELOCK_H
#define FILELOCK_H
/** @file FileLock.h
 * @brief File locking
 */

#include <string>

/** @brief Hold an flock() lock on a file */
class FileLock {
public:
  /** @brief Constructor
   * @param path File to lock
   *
   * The constructor does not acquire the lock.
   */
  FileLock(const std::string &path);

  /** @brief Destructor
   *
   * Releases the lock if is held.
   */
  ~FileLock();

  /** @brief Acquire the lock
   * @param wait If true, block until lock can be acquire; else give up
   * @return true if the lock was acquired
   */
  bool acquire(bool wait = true);

  /** @brief Release the lock */
  void release();
private:
  std::string path;
  int fd;
  bool held;

  void ensureOpen();
};

#endif /* FILELOCK_H */

