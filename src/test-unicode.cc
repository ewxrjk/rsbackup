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

int main() {
  std::wstring w;
  if(!setlocale(LC_CTYPE, "C.UTF-8")
     && !setlocale(LC_CTYPE, "en_US.UTF-8")
     && !setlocale(LC_CTYPE, "en_GB.UTF-8")) {
    fprintf(stderr, "ERROR: cannot find a UTF-8 locale to test in\n");
    return 77;
  }
  toUnicode(w, "just ascii");
  assert(w == L"just ascii");
  toUnicode(w, "𐌲𐌿𐍄𐌰𐌽𐍃 𐌿𐍃 𐍄𐍂𐌹𐌾𐌿𐍃");
  assert(w == L"𐌲𐌿𐍄𐌰𐌽𐍃 𐌿𐍃 𐍄𐍂𐌹𐌾𐌿𐍃");
  return 0;
}
