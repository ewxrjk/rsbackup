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
#include "Conf.h"
#include "Backup.h"
#include "Volume.h"
#include "Host.h"
#include <cassert>

#define SETUP()                                 \
  Conf c;                                       \
  Host h1(&c, "h1");                            \
  Host h2(&c, "h2");                            \
  Volume h1v1(&h1, "v1", "/v1");                \
  Volume h1v2(&h1, "v2", "/v2");                \
  Volume h2v1(&h2, "v1", "/v1");                \
  Volume h2v2(&h2, "v2", "/v2")

static void test_initial_state() {
  SETUP();
  assert(!h1.selected());
  assert(!h1v1.selected());
  assert(!h1v2.selected());
  assert(!h2.selected());
  assert(!h2v1.selected());
  assert(!h2v2.selected());
}

static void test_select_all() {
  SETUP();
  c.selectVolume("*", "*", true);
  assert(h1.selected());
  assert(h1v1.selected());
  assert(h1v2.selected());
  assert(h2.selected());
  assert(h2v1.selected());
  assert(h2v2.selected());
}

static void test_select_host() {
  SETUP();
  c.selectVolume("h1", "*", true);
  assert(h1.selected());
  assert(h1v1.selected());
  assert(h1v2.selected());
  assert(!h2.selected());
  assert(!h2v1.selected());
  assert(!h2v2.selected());
}

static void test_deselect_host() {
  SETUP();
  c.selectVolume("*", "*", true);
  c.selectVolume("h2", "*", false);
  assert(h1.selected());
  assert(h1v1.selected());
  assert(h1v2.selected());
  assert(!h2.selected());
  assert(!h2v1.selected());
  assert(!h2v2.selected());
}

static void test_select_volume() {
  SETUP();
  c.selectVolume("h1", "v1", true);
  assert(h1.selected());
  assert(h1v1.selected());
  assert(!h1v2.selected());
  assert(!h2.selected());
  assert(!h2v1.selected());
  assert(!h2v2.selected());
}

static void test_deselect_volume() {
  SETUP();
  c.selectVolume("*", "*", true);
  c.selectVolume("h2", "v1", false);
  assert(h1.selected());
  assert(h1v1.selected());
  assert(h1v2.selected());
  assert(h2.selected());
  assert(!h2v1.selected());
  assert(h2v2.selected());
}

int main() {
  test_initial_state();
  test_select_all();
  test_select_host();
  test_deselect_host();
  test_select_volume();
  test_deselect_volume();
  return 0;
}
