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
#include "Errors.h"
#include <cassert>
#include <cerrno>

int main() {
  {
    Directory d;
    std::vector<std::string> files;
    d.open(".");
    d.get(files);
    for(size_t n = 0; n + 1 < files.size(); ++n)
      assert(files[n] < files[n + 1]);
    d.close();
  }

  {
    std::vector<std::string> files;
    Directory::getFiles(".", files);
    for(size_t n = 0; n + 1 < files.size(); ++n)
      assert(files[n] < files[n + 1]);
  }

  {
    Directory d;
    std::vector<std::string> files;
    d.open(".");
    std::string name;
    while(d.get(name))
      files.push_back(name);
  }

  try {
    Directory d;
    d.open("/dev/null");
    assert(!"unexpectedly succeeded");
  } catch(IOError &e) {
    assert(e.errno_value == ENOTDIR);
  }

  try {
    Directory d;
    d.open("does not exist");
    assert(!"unexpectedly succeeded");
  } catch(IOError &e) {
    assert(e.errno_value == ENOENT);
  }
  return 0;
}
