// Copyright © 2011 Richard Kettlewell.
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
#include <cstring>

void progressBar(const char *prompt, size_t done, size_t total) {
  const int width = 79;
  
  if(total == 0) {
    IO::err.writef("\r%*s\r", width, " ");
  } else {
    std::string s;
    int bar = width - (3 + strlen(prompt));
    s.append("\r");
    s.append(prompt);
    s.append(" [");
    s.append(done * bar / total, '=');
    s.append(bar - (done * bar / total), ' ');
    s.append("]\r");
    IO::err.writef("%s", s.c_str());
  }
}
