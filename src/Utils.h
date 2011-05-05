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
#ifndef UTILS_H
#define UTILS_H

#include <string>

// Display a prompt and insist on a yes/no reply.
// Overridden by --force (which means 'always yes').
bool check(const char *format, ...);

// rm -rf PATH
void BulkRemove(const std::string &path);

// Convert mbs from native multibyte encoding to a Unicode string.  We
// assume that wchar_t is UTF-32.
void toUnicode(std::wstring &u, const std::string &mbs);

void progressBar(const char *prompt, size_t done, size_t total);

#endif /* UTILS_H */

