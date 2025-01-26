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
#include <config.h>
#include "rsbackup.h"
#include "Errors.h"
#include "Utils.h"

int parseTimeOfDay(const std::string &s) {
  std::vector<std::string> bits;
  size_t pos = 0;
  while(pos < s.size()) {
    size_t colon = s.find(':', pos);
    bits.push_back(s.substr(pos, colon - pos));
    if(colon == std::string::npos)
      pos = s.size();
    else
      pos = colon + 1;
  }
  if(bits.size() < 2 || bits.size() > 3)
    throw SyntaxError("time of day " + s + " malformed");
  long long hour = parseInteger(bits[0], 0, 24, 10);
  long long minute = parseInteger(bits[1], 0, 59, 10);
  long long second = bits.size() > 2 ? parseInteger(bits[2], 0, 59, 10) : 0;
  if(hour == 24) {
    if(minute || second)
      throw SyntaxError("time of day " + s + " out of range");
  }
  return static_cast<int>(hour * 3600 + minute * 60 + second);
}

/** @brief Format a time of day
 * @param t Number of seconds since start of day
 * @return Representation of @p t
 */
std::string formatTimeOfDay(int t) {
  int seconds = t % 60;
  int minutes = (t / 60) % 60;
  int hours = t / 3600;
  char buffer[64];
  snprintf(buffer, sizeof buffer, "%d:%02d:%02d", hours, minutes, seconds);
  return buffer;
}
