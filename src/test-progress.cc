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
#include "IO.h"
#include <cassert>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>

#define WIDTH 79

static std::string getBytes(int fd) {
  std::string s;
  char buffer[128];
  int n;
  while((n = read(fd, buffer, sizeof buffer)) > 0)
    s.append(buffer, static_cast<size_t>(n));
  if(n < 0)
    assert(errno == EAGAIN);
  return s;
}

int main() {
  FILE *ofp;
  int p[2], r;
  assert(pipe(p) >= 0);
  assert((ofp = fdopen(p[1], "w")));
  assert((r = fcntl(p[0], F_GETFL)) >= 0);
  assert(fcntl(p[0], F_SETFL, r | O_NONBLOCK) >= 0);

  IO o(ofp, "output");

  progressBar(o, "spong", 0, 0);
  std::string s = getBytes(p[0]);
  assert(s == "\r" + std::string(WIDTH, ' ') + "\r");

  progressBar(o, "spong", 0, 10);
  s = getBytes(p[0]);
  assert(s == "\rspong [" + std::string(WIDTH - 8, ' ') + "]\r");

  progressBar(o, "spong", 1, 10);
  s = getBytes(p[0]);
  assert(s
         == "\rspong [" + std::string((WIDTH - 8) / 10, '=')
                + std::string((WIDTH - 8) - (WIDTH - 8) / 10, ' ') + "]\r");

  progressBar(o, "spong", 10, 10);
  s = getBytes(p[0]);
  assert(s == "\rspong [" + std::string(WIDTH - 8, '=') + "]\r");

  return 0;
}
