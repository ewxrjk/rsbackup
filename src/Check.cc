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
    fgets(buffer, sizeof buffer, stdin);
    if(feof(stdin))
      throw IOError("unexpected EOF reading stdin");
    if(ferror(stdin))
      throw IOError("reading stdin", errno);
    std::string result = buffer;
    if(result == "yes\n")
      return true;
    if(result == "no\n")
      return false;
    IO::out.writef("Please answer 'yes' or 'no'.\n");
  }
}
