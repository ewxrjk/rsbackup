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
#include "Device.h"
#include <getopt.h>
#include <cassert>

int main() {
  assert(!Device::valid(""));
  assert(Device::valid(
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_."));
  assert(!Device::valid("/"));
  assert(!Device::valid("~"));
  assert(!Device::valid("\x80"));
  assert(!Device::valid(" "));
  assert(!Device::valid("\x1F"));
  return 0;
}
