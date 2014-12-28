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
#ifndef RSBACKUP_H
#define RSBACKUP_H
/** @file rsbackup.h
 * @brief Common definitions
 */

#include <vector>
#include <string>

class Document;

/** @brief Make backups */
void makeBackups();

/** @brief Retire volumes */
void retireVolumes();

/** @brief Retire devices */
void retireDevices();

/** @brief Prune backups */
void pruneBackups();

/** @brief Prune redundant logs */
void prunePruneLogs();

/** @brief Generate the report */
void generateReport(Document &d);

/** @brief HTML stylesheet */
extern char stylesheet[];

/** @brief Error count */
extern int errors;

#endif /* RSBACKUP_H */
