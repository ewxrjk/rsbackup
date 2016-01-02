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
#include "Errors.h"
#include <cerrno>
#include <ctime>
#include <sys/time.h>

void getMonotonicTime(struct timespec &now) {
#ifdef CLOCK_MONOTONIC
  if(clock_gettime(CLOCK_MONOTONIC, &now) < 0)
    throw IOError("clock_gettime", errno);
#else
  struct timeval tv;
  if(gettimeofday(&tv, nullptr) < 0)
    throw IOError("gettimeofday", errno);
  now.tv_sec = tv.tv_sec;
  now.tv_nsec = tv.tv_sec * 1000;
#endif
}
