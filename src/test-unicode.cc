// Copyright © 2012, 2014 Richard Kettlewell.
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
#include <cassert>
#include <cstdio>
#include <clocale>

static const unsigned char narrow[] = {
  0xf0, 0x90, 0x8c, 0xb2, 0xf0, 0x90, 0x8c, 0xbf,
  0xf0, 0x90, 0x8d, 0x84, 0xf0, 0x90, 0x8c, 0xb0,
  0xf0, 0x90, 0x8c, 0xbd, 0xf0, 0x90, 0x8d, 0x83,
  0x20, 0xf0, 0x90, 0x8c, 0xbf, 0xf0, 0x90, 0x8d,
  0x83, 0x20, 0xf0, 0x90, 0x8d, 0x84, 0xf0, 0x90,
  0x8d, 0x82, 0xf0, 0x90, 0x8c, 0xb9, 0xf0, 0x90,
  0x8c, 0xbe, 0xf0, 0x90, 0x8c, 0xbf, 0xf0, 0x90,
  0x8d, 0x83, 0
};

static const char32_t wide[] = {
  0x10332, 0x1033f, 0x10344, 0x10330,
  0x1033d, 0x10343, 0x20, 0x1033f,
  0x10343, 0x20, 0x10344, 0x10342,
  0x10339, 0x1033e, 0x1033f, 0x10343,
  0
};

int main() {
  std::u32string w;
  if(!setlocale(LC_CTYPE, "C.UTF-8")
     && !setlocale(LC_CTYPE, "en_US.UTF-8")
     && !setlocale(LC_CTYPE, "en_GB.UTF-8")) {
    fprintf(stderr, "ERROR: cannot find a UTF-8 locale to test in\n");
    return 77;
  }
  toUnicode(w, "just ascii");
  assert(w == U"just ascii");
  toUnicode(w, reinterpret_cast<const char *>(narrow));
  assert(w == wide);
  return 0;
}
