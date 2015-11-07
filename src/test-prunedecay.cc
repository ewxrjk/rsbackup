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
#include "Prune.h"
#include <cassert>
#include <cstdio>

static const int v12[] = { 0,1,1,2,2,2,2,3,-1 };
static const int v22[] = { 0,0,1,1,1,1,2,2,2,2,2,2,2,2,3,-1 };
static const int v13[] = { 0,1,1,1,2,2,2,2,2,2,2,2,2,3,-1 };
static const int v23[] = { 0,0,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,-1 };

static void check(int w, int s, const int *v) {
  int a = 1;
  while(*v >= 0) {
    int n = prune_decay_bucket(w, s, a);
    //printf("w=%d s=%d a=%d n=%d expected %d\n", w, s, a, n, *v);
    assert(n == *v);
    ++v;
    ++a;
  }
}

int main(void) {
  check(1, 2, v12);
  check(2, 2, v22);
  check(1, 3, v13);
  check(2, 3, v23);
  return 0;
}
