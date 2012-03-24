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
#ifndef UTILS_H
#define UTILS_H
/** @file Utils.h
 * @brief Miscellaneous
 */

#include <string>

/** @brief Display a prompt and retrieve a yes/no reply
 * @param format Format string as per @c printf()
 * @param ... Arguments
 * @return True if the user said yes
 *
 * Overridden by @c --force, which means "always yes".
 */
bool check(const char *format, ...);

/** @brief Bulk remove files and directories
 * @param path Base path to remove
 */
void BulkRemove(const std::string &path);

/** @brief Convert to Unicode
 * @param u Where to put Unicode string
 * @param mbs Multibyte string
 *
 * It is assumed that @c wchar_t is UTF-32.
 */
void toUnicode(std::wstring &u, const std::string &mbs);

/** @brief Display a progress bar
 * @param prompt Prompt string
 * @param done Work done
 * @param total Total work
 *
 * If @p total is 0 then the progress bar is erased.
 */
void progressBar(const char *prompt, size_t done, size_t total);

#endif /* UTILS_H */

