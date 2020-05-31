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
#include <cctype>
#include "Conf.h"

bool namelt(const std::string &a, const std::string &b) {
  size_t apos = 0, bpos = 0;
  const size_t asize = a.size(), bsize = b.size();

  while(apos < asize && bpos < bsize) {
    int ach = a[apos], bch = b[bpos];
    if(isdigit(ach) && isdigit(bch)) {
      // Skip leading 0s
      while(apos < asize && a[apos] == '0')
        apos++;
      while(bpos < bsize && b[bpos] == '0')
        bpos++;
      size_t astart = apos, bstart = bpos;
      // Find the ends
      while(apos < asize && isdigit(a[apos]))
        apos++;
      while(bpos < bsize && isdigit(b[bpos]))
        bpos++;
      // If the numbers are different lengths, longest wins
      size_t alen = apos - astart, blen = bpos - bstart;
      if(alen < blen)
        return true;
      if(alen > blen)
        return false;
      // The numbers are equal lengths, so lexicograph order matches numeric
      // order
      int cmp = a.compare(astart, alen, b, bstart, blen);
      if(cmp < 0)
        return true;
      if(cmp > 0)
        return false;
    } else if(isdigit(ach)) {
      return true;
    } else if(isdigit(bch)) {
      return false;
    } else {
      if(ach < bch)
        return true;
      if(ach > bch)
        return false;
      apos++;
      bpos++;
    }
  }
  return a < b;
}
