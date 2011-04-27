#include <config.h>
#include "Errors.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>

std::string SubprocessFailed::format(const std::string &name, int wstat) {
  if(WIFSIGNALED(wstat)) {
    int sig = WTERMSIG(wstat);
    return (name + ": " + strsignal(sig)
            + (WCOREDUMP(wstat) ? " (core dumped)" : ""));
  } else if(WIFEXITED(wstat)) {
    char buffer[64];
    int rc = WEXITSTATUS(wstat);
    snprintf(buffer, sizeof buffer, "%d", rc);
    return (name + ": exited with status " + buffer);
  } else {
    char buffer[64];
    snprintf(buffer, sizeof buffer, "%x", wstat);
    return (name + ": exited with wait status " + buffer);
  }
}
