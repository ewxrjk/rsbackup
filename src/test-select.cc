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
#include "Selection.h"
#include <cassert>

#define SETUP()                                                                \
  Conf c;                                                                      \
  auto h1 = new Host(&c, "h1");                                                \
  auto h2 = new Host(&c, "h2");                                                \
  auto h1v1 = new Volume(h1, "v1", "/v1");                                     \
  auto h1v2 = new Volume(h1, "v2", "/v2");                                     \
  auto h2v1 = new Volume(h2, "v1", "/v1");                                     \
  auto h2v2 = new Volume(h2, "v2", "/v2")

static void test_initial_state() {
  SETUP();
  assert(!h1->selected(PurposeBackup));
  assert(!h1v1->selected(PurposeBackup));
  assert(!h1v2->selected(PurposeBackup));
  assert(!h2->selected(PurposeBackup));
  assert(!h2v1->selected(PurposeBackup));
  assert(!h2v2->selected(PurposeBackup));
}

static void test_select_all() {
  SETUP();
  VolumeSelections::select(c, "*", "*", PurposeBackup, true);
  assert(h1->selected(PurposeBackup));
  assert(h1v1->selected(PurposeBackup));
  assert(h1v2->selected(PurposeBackup));
  assert(h2->selected(PurposeBackup));
  assert(h2v1->selected(PurposeBackup));
  assert(h2v2->selected(PurposeBackup));
}

static void test_select_host() {
  SETUP();
  VolumeSelections::select(c, "h1", "*", PurposeBackup, true);
  assert(h1->selected(PurposeBackup));
  assert(h1v1->selected(PurposeBackup));
  assert(h1v2->selected(PurposeBackup));
  assert(!h2->selected(PurposeBackup));
  assert(!h2v1->selected(PurposeBackup));
  assert(!h2v2->selected(PurposeBackup));
}

static void test_deselect_host() {
  SETUP();
  VolumeSelections::select(c, "*", "*", PurposeBackup, true);
  VolumeSelections::select(c, "h2", "*", PurposeBackup, false);
  assert(h1->selected(PurposeBackup));
  assert(h1v1->selected(PurposeBackup));
  assert(h1v2->selected(PurposeBackup));
  assert(!h2->selected(PurposeBackup));
  assert(!h2v1->selected(PurposeBackup));
  assert(!h2v2->selected(PurposeBackup));
}

static void test_select_volume() {
  SETUP();
  VolumeSelections::select(c, "h1", "v1", PurposeBackup, true);
  assert(h1->selected(PurposeBackup));
  assert(h1v1->selected(PurposeBackup));
  assert(!h1v2->selected(PurposeBackup));
  assert(!h2->selected(PurposeBackup));
  assert(!h2v1->selected(PurposeBackup));
  assert(!h2v2->selected(PurposeBackup));
}

static void test_deselect_volume() {
  SETUP();
  VolumeSelections::select(c, "*", "*", PurposeBackup, true);
  VolumeSelections::select(c, "h2", "v1", PurposeBackup, false);
  assert(h1->selected(PurposeBackup));
  assert(h1v1->selected(PurposeBackup));
  assert(h1v2->selected(PurposeBackup));
  assert(h2->selected(PurposeBackup));
  assert(!h2v1->selected(PurposeBackup));
  assert(h2v2->selected(PurposeBackup));
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
