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
#ifndef RETIRE_H
#define RETIRE_H
/** @file Retire.h
 * @brief Retiral support
 */

#include <string>
#include <vector>

/** @brief Remove obsolete logfiles and maybe backups
 * @param obsoleteLogs List of logfiles that are now obsolete
 * @param removeBackup If true remove corresponding backups
 */
void removeObsoleteLogs(const std::vector<std::string> &obsoleteLogs,
                        bool removeBackup);

#endif /* RETIRE_H */

