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
#include "Command.h"
#include "Conf.h"
#include <getopt.h>

int main() {
  int errors = 0;
  const std::string h = Command::helpString();
  for(size_t n = 0; Command::options[n].name; ++n) {
    std::string full = "--" + std::string(Command::options[n].name);
    if(h.find(full + ",") == std::string::npos
       && h.find(full + " ") == std::string::npos) {
      fprintf(stderr, "ERROR: help for option %s not found\n", full.c_str());
      ++errors;
    }
  }
  return !!errors;
}
