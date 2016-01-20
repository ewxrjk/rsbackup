// Copyright © 2012-14 Richard Kettlewell.
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
#include "Subprocess.h"
#include "Errors.h"
#include <cassert>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>
#include <csignal>
#include <cstdarg>

static const char *warnings[64];
static size_t nwarnings;

void warning(const char *fmt, ...) {
  char *w;
  va_list ap;
  assert(nwarnings < sizeof warnings / sizeof *warnings);
  va_start(ap, fmt);
  assert(vasprintf(&w, fmt, ap) >= 0);
  va_end(ap);
  warnings[nwarnings++] = w;
}

void fatal(const char *, ...) {
  abort();
}

int main() {
  // Separate capture of stdout and stderr
  std::vector<std::string> command = {
    "sh", "-c",
    "echo stdout; sleep 1; echo >&2 stderr; sleep 2; echo skipped"
  };
  Subprocess sp(command);
  std::string stdoutCapture, stderrCapture;
  sp.capture(1, &stdoutCapture);
  sp.capture(2, &stderrCapture);
  sp.setTimeout(2);
  int rc = sp.runAndWait(false);
  assert(stdoutCapture == "stdout\n");
  assert(stderrCapture == "stderr\n");
  assert(WIFSIGNALED(rc));
  assert(WTERMSIG(rc) == SIGKILL);
  assert(nwarnings == 1);
  assert(!strcmp(warnings[0], "sh exceeded timeout of 2 seconds"));

  // Capture of both stdout and stderr
  command = { "sh", "-c", "echo stdout; echo >&2 stderr" };
  Subprocess sp2(command);
  std::string bothCapture;
  sp2.capture(1, &bothCapture, 2);
  rc = sp2.runAndWait(false);
  assert(bothCapture == "stdout\nstderr\n");
  assert(WIFEXITED(rc));
  assert(WEXITSTATUS(rc) == 0);
  assert(nwarnings == 1);

  // NB assumes the 'usual' encoding of exit status, will need to do something
  // more sophisticated if some useful platform doesn't play along.
  //
  // For reference the 'usual' encoding is:
  //   bits 0-6:
  //       The termination/stop signal
  //       0     = exited
  //       1-126 = terminated with this signal
  //       127   = stopped
  //   bits 7
  //       Set if core dumped
  //   bits 8-15
  //       If exited: the exit status
  //       If stopped: the stop signal
  std::string d;
  d = SubprocessFailed::format("progname", SIGKILL);
  assert(d == std::string("progname: ") + strsignal(SIGKILL));
  d = SubprocessFailed::format("progname", SIGKILL|0x80);
  assert(d == std::string("progname: ") + strsignal(SIGKILL) + " (core dumped)");
  d = SubprocessFailed::format("progname", 37 << 8);
  assert(d.find("progname") != std::string::npos);
  assert(d == "progname: exited with status 37");
  d = SubprocessFailed::format("progname", (SIGSTOP << 8) + 0x7F);
  assert(d == std::string("progname: ") + strsignal(SIGSTOP));
  return 0;
}
