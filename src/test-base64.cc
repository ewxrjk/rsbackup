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
#include <sstream>
#include "Utils.h"

static std::string base64(const std::string &s) {
  std::stringstream ss;
  write_base64(ss, s);
  std::cerr << "[" << s << "] -> [" << ss.str() << "]\n";
  return ss.str();
}

int main() {
  assert(base64("") == "");
  assert(base64("f") == "Zg==");
  assert(base64("fo") == "Zm8=");
  assert(base64("foo") == "Zm9v");
  assert(base64("foob") == "Zm9vYg==");
  assert(base64("fooba") == "Zm9vYmE=");
  assert(base64("foobar") == "Zm9vYmFy");
}
