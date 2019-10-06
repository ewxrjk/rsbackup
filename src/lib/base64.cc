// Copyright © 2015 Richard Kettlewell.
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
#include <ostream>
#include "Utils.h"

std::ostream &write_base64(std::ostream &os, const std::string &s,
                           const char *alphabet) {
  int line_length = 76;
  int offset = 0;
  size_t pos = 0, limit = s.size();
  while(pos < limit) {
    unsigned b;
    int bit, bitlimit;
    switch(limit - pos) {
    default:
      b = static_cast<unsigned char>(s.at(pos++));
      b = 256 * b + static_cast<unsigned char>(s.at(pos++));
      b = 256 * b + static_cast<unsigned char>(s.at(pos++));
      bitlimit = 0;
      break;
    case 2:
      b = static_cast<unsigned char>(s.at(pos++));
      b = 256 * b + static_cast<unsigned char>(s.at(pos++));
      b = 256 * b;
      bitlimit = 8;
      break;
    case 1:
      b = static_cast<unsigned char>(s.at(pos++));
      b = 256 * b;
      b = 256 * b;
      bitlimit = 16;
      break;
    }
    bit = 3 * 6;
    while(bit + 6 > bitlimit) {
      os.put(alphabet[(b >> bit) & 0x3F]);
      bit -= 6;
      ++offset;
    }
    while(bit >= 0) {
      os.put(alphabet[64]);
      bit -= 6;
      ++offset;
    }
    if(offset >= line_length) {
      os.put('\n');
      offset = 0;
    }
  }
  return os;
}

const char rfc4648_base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
