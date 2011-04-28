#include <config.h>
#include "Command.h"
#include "Errors.h"
#include "Utils.h"
#include <cstdio>
#include <cstdarg>
#include <cerrno>

bool check(const char *format, ...) {
  if(command.force)
    return true;
  char buffer[64];
  for(;;) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    printf("yes/no> ");
    fflush(stdout);
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
    printf("Please answer 'yes' or 'no'.\n");
  }
}
