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
#include "Utils.h"
#include "EventLoop.h"
#include "Errors.h"
#include <cerrno>
#include <unistd.h>
#include <cassert>

class TestReactor: public Reactor {
public:
  TestReactor(): read_calls(0), wrote_bytes(0) {}

  void onReadable(EventLoop *e, int fd, const void *, size_t) {
    assert(close(fd) == 0);
    e->cancelRead(fd);
    ++read_calls;
  }

  void onWritable(EventLoop *e, int fd) {
    if(wrote_bytes < writeme.size()) {
      size_t remain = writeme.size() - wrote_bytes;
      ssize_t n = ::write(fd, &writeme[wrote_bytes], remain);
      assert(n >= 0);
      assert(static_cast<size_t>(n) <= remain);
      wrote_bytes += n;
    } else {
      assert(close(fd) == 0);
      e->cancelWrite(fd);
    }
  }

  int read_calls;

  std::string writeme;
  size_t wrote_bytes;
};

static void test_read_closed() {
  int p[2];
  assert(pipe(p) == 0);
  assert(close(p[1]) == 0);
  EventLoop e;
  TestReactor tr;
  e.whenReadable(p[0], &tr);
  e.wait();
  assert(tr.read_calls == 1);
}

static void test_write() {
  int p[2];
  assert(pipe(p) == 0);
  EventLoop e;
  TestReactor tr;
  tr.writeme = "test data";
  e.whenWritable(p[1], &tr);
  e.wait();
  char buffer[4096];
  ssize_t n = ::read(p[0], buffer, sizeof buffer);
  assert(static_cast<size_t>(n) == tr.writeme.size());
  assert(std::string(buffer, n) == tr.writeme);
}

int main() {
  test_read_closed();
  test_write();
  return 0;
}
