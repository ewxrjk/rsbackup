// Copyright © 2012 Richard Kettlewell.
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
#include <sys/signal.h>
#include <cstdio>

int main() {
  std::vector<std::string> command;
  command.push_back("sh");
  command.push_back("-c");
  command.push_back("echo stdout; sleep 1; echo >&2 stderr; sleep 2; echo skipped");
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
  // NB assumes the 'usual' encoding of exit status, will need to do something
  // more sophisticated if some useful platform doesn't play along.
  std::string d;
  d = SubprocessFailed::format("progname", SIGKILL);
  assert(d == std::string("progname: ") + strsignal(SIGKILL));
  d = SubprocessFailed::format("progname", SIGKILL|0x80);
  assert(d == std::string("progname: ") + strsignal(SIGKILL) + " (core dumped)");
  d = SubprocessFailed::format("progname", 37 << 8);
  assert(d.find("progname") != std::string::npos);
  assert(d == "progname: exited with status 37");
  return 0;
}
