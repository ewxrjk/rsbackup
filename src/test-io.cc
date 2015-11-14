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
#include "IO.h"
#include <cassert>
#include <cstdio>

static void test_io_write(void) {
  IO f;
  f.open("test-io-tmp", "w");
  f.write("line 1\n");
  f.writef("line %d\n", 2);
  f.close();
}

static void test_io_readline(void) {
  IO f;
  f.open("test-io-tmp", "r");
  std::string l;
  bool rc;
  rc = f.readline(l);
  assert(rc);
  assert(l == "line 1");
  rc = f.readline(l);
  assert(rc);
  assert(l == "line 2");
  rc = f.readline(l);
  assert(!rc);
  f.close();
}

static void test_io_readlines(void) {
  IO f;
  f.open("test-io-tmp", "r");
  std::vector<std::string> ls;
  f.readlines(ls);
  assert(ls.size() == 2);
  assert(ls[0] == "line 1");
  assert(ls[1] == "line 2");
  f.close();
}

static void test_io_capture(void) {
  IO f;
  std::vector<std::string> command = { "echo", "spong" };
  std::vector<std::string> ls;
  f.popen(command, ReadFromPipe, false);
  f.readlines(ls);
  f.close();
  assert(ls.size() == 1);
  assert(ls[0] == "spong");
}

int main() {
  test_io_write();
  test_io_readline();
  test_io_readlines();
  test_io_capture();
  remove("test-io-tmp");
  return 0;
}
