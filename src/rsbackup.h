//-*-C++-*-
// Copyright © 2011 Richard Kettlewell.
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

#include <vector>
#include <string>

class Document;

void makeBackups();
void retireVolumes();
void retireDevices();
void pruneBackups();
void prunePruneLogs();
void generateReport(Document &d);

extern char stylesheet[];
extern int errors;                      // count of errors

#endif /* RSBACKUP_H */
