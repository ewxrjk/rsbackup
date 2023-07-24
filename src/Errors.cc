// Copyright © 2011, 2012, 2014 Richard Kettlewell.
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
#include "Errors.h"
#include "Location.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif

std::string SubprocessFailed::format(const std::string &name, int wstat) {
  if(WIFSIGNALED(wstat)) {
    int sig = WTERMSIG(wstat);
    return (name + ": " + strsignal(sig)
            + (WCOREDUMP(wstat) ? " (core dumped)" : ""));
  } else if(WIFSTOPPED(wstat)) {
    int sig = WSTOPSIG(wstat);
    return (name + ": " + strsignal(sig));
  } else if(WIFEXITED(wstat)) {
    char buffer[64];
    int rc = WEXITSTATUS(wstat);
    snprintf(buffer, sizeof buffer, "%d", rc);
    return (name + ": exited with status " + buffer);
  } else {
    char buffer[64];
    snprintf(buffer, sizeof buffer, "%#x", wstat);
    return (name + ": exited with wait status " + buffer);
  }
}

Error::Error(const std::string &msg): std::runtime_error(msg) {
#if HAVE_EXECINFO_H
  stacksize = backtrace(stack, sizeof stack / sizeof *stack);
#endif
}

void Error::trace(FILE *fp) {
#if HAVE_EXECINFO_H
  char **names = backtrace_symbols(stack, stacksize);
  for(int i = 0; i < stacksize; ++i)
    fprintf(fp, "%s\n", names[i]);
  free(names);
#endif
}

std::string ConfigError::format(const Location &location,
                                const std::string &msg) {
  std::stringstream s;
  s << location.path << ":" << location.line << ": " << msg;
  return s.str();
}
