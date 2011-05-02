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
#include "Command.h"
#include "Errors.h"
#include "Utils.h"
#include "IO.h"
#include <cstdio>
#include <cstdarg>
#include <cerrno>

bool check(const char *format, ...) {
  // --force overrides
  if(command.force)
    return true;
  char buffer[64];
  for(;;) {
    // Display the prompt
    va_list ap;
    va_start(ap, format);
    IO::out.vwritef(format, ap);
    va_end(ap);
    IO::out.writef("yes/no> ");
    IO::out.flush();
    // Get a yes/no answer
    if(!fgets(buffer, sizeof buffer, stdin)) {
      if(feof(stdin))
        throw IOError("unexpected EOF reading stdin");
      if(ferror(stdin))
        throw IOError("reading stdin", errno);
    }
    std::string result = buffer;
    if(result == "yes\n")
      return true;
    if(result == "no\n")
      return false;
    IO::out.writef("Please answer 'yes' or 'no'.\n");
  }
}
