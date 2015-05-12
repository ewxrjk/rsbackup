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
#include "Errors.h"
#include "Command.h"
#include <cassert>
#include <pthread.h>
#include <unistd.h>
#include <cstdio>

static FILE *input;
static int output;
static bool result;

static void *background(void *) {
  result = check("%s", "spong");
  return NULL;
}

/* We can't use stdio here because reading from an unbuffered file causes a
 * flush of other files, which (at least in Glibc) implies taking a lock on
 * other files.  Since the background thread is blocked in a read, holding a
 * lock while it does so, a deadlock results.
 *
 * References:
 * - C99 7.19.3#3
 * - http://austingroupbugs.net/view.php?id=689
 * - _IO_flush_all_linebuffered in Glibc
 */
static int fd_getc(int fd) {
  unsigned char buffer[1];
  if(read(fd, buffer, 1) < 0)
    return EOF;
  else
    return buffer[0];
}

static char *fd_fgets(char buffer[], size_t bufsize, int fd) {
  size_t index = 0;
  int ch;
  assert(bufsize > 0);
  while(index + 1 < bufsize && (ch = fd_getc(fd)) >= 0) {
    buffer[index++] = ch;
    if(ch == '\n')
      break;
  }
  buffer[index] = 0;
  return ch < 0 ? NULL : buffer;
}

static void test(const char *typed, const char *typed2, bool expect) {
  pthread_t tid;
  char buffer[1024];

  assert(pthread_create(&tid, NULL, background, NULL) == 0);
  assert(fd_fgets(buffer, sizeof buffer, output));
  assert(std::string(buffer) == "spong\n");
  assert(fd_getc(output) == 'y');
  assert(fd_getc(output) == 'e');
  assert(fd_getc(output) == 's');
  assert(fd_getc(output) == '/');
  assert(fd_getc(output) == 'n');
  assert(fd_getc(output) == 'o');
  assert(fd_getc(output) == '>');
  assert(fd_getc(output) == ' ');
  assert(fprintf(input, "%s\n", typed) >= 0);
  assert(fflush(input) >= 0);

  if(typed2) {
    assert(fd_fgets(buffer, sizeof buffer, output));
    assert(std::string(buffer) == "Please answer 'yes' or 'no'.\n");
    assert(fd_fgets(buffer, sizeof buffer, output));
    assert(std::string(buffer) == "spong\n");
    assert(fd_getc(output) == 'y');
    assert(fd_getc(output) == 'e');
    assert(fd_getc(output) == 's');
    assert(fd_getc(output) == '/');
    assert(fd_getc(output) == 'n');
    assert(fd_getc(output) == 'o');
    assert(fd_getc(output) == '>');
    assert(fd_getc(output) == ' ');
    assert(fprintf(input, "%s\n", typed2) >= 0);
    assert(fflush(input) >= 0);
  }

  assert(pthread_join(tid, NULL) == 0);
  assert(result == expect);
}

static void test_force(void) {
  pthread_t tid;

  command.force = true;
  assert(pthread_create(&tid, NULL, background, NULL) == 0);
  assert(pthread_join(tid, NULL) == 0);
  assert(result == true);
  command.force = false;
}

int main() {
  int i[2], o[2];

  assert(pipe(i) >= 0);
  assert(pipe(o) >= 0);
  assert(dup2(i[0], 0) >= 0);
  assert(close(i[0]) >= 0);
  assert(dup2(o[1], 1) >= 0);
  assert(close(o[1]) >= 0);
  assert((input = fdopen(i[1], "w")));
  output = o[0];

  test("yes", NULL, true);
  test("no", NULL, false);
  test("", "yes", true);
  test("whatever", "yes", true);
  test_force();

  assert(fclose(input) >= 0);
  try {
    check("%s", "spong");
    assert(!"unexpectedly succeeded");
  } catch(IOError &e) {
  }

  return 0;
}
