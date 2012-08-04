#include <config.h>
#include "Subprocess.h"
#include <cassert>
#include <sys/types.h>
#include <sys/wait.h>

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
  return 0;
}
