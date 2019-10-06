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
#include "FileLock.h"
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  int p[2];
  char buf[1];
  int rc;

  char path[] = "lock.XXXXXX";
  assert(mkstemp(path));
  assert(pipe(p) >= 0);
  FileLock a(path);
  assert(a.acquire());
  switch(pid_t child = fork()) {
  case -1: abort();
  case 0: {
    FileLock b(path);
    assert(!b.acquire(false));
    assert(write(p[1], "", 1) == 1);
    assert(b.acquire(true));
    _exit(0);
  }
  default:
    while((rc = read(p[0], buf, 1)) < 0 && errno == EINTR)
      ;
    assert(rc == 1);
    a.release();
    int w;
    assert(wait(&w) == child);
    assert(w == 0);
    break;
  }
  unlink(path);
  return 0;
}
