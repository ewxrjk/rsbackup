// Copyright © 2014 Richard Kettlewell.
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
#include "Utils.h"

int main() {
  const struct timespec ts[] = {
      {0, 0},
      {0, 500 * 1000000},
      {1, 0},
      {1, 500 * 1000000},
  };
#define NTS (sizeof ts / sizeof *ts)
  for(size_t i = 0; i < NTS; ++i) {
    for(size_t j = 0; j < NTS; ++j) {
      if(i < j) {
        assert(!(ts[i] >= ts[j]));
        assert(!(ts[i] == ts[j]));
      } else if(i == j) {
        assert(ts[i] >= ts[j]);
        assert(ts[i] == ts[j]);
      } else {
        assert(ts[i] >= ts[j]);
        assert(!(ts[i] == ts[j]));
      }
      assert(ts[i] - ts[i] == ts[0]);
    }
  }
  assert(ts[2] - ts[1] == ts[1]);
  assert(ts[3] - ts[2] == ts[1]);
  assert(ts[3] - ts[1] == ts[2]);
  return 0;
}
