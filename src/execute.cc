#include <config.h>
#include "rsbackup.h"
#include "Errors.h"
#include "Command.h"
#include <cerrno>
#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>

int execute(const std::vector<std::string> &cmd) {
  std::vector<const char *> args;
  for(size_t n = 0; n < cmd.size(); ++n)
    args.push_back(cmd[n].c_str());
  args.push_back(NULL);
  pid_t pid;
  if(command.verbose) {
    printf(">");
    for(size_t n = 0; n < cmd.size(); ++n)
      printf(" %s", cmd[n].c_str());
    printf("\n");
  }
  switch(pid = fork()) {
  case -1:
    throw IOError("creating subprocess for " + cmd[0], errno); // TODO exception??
  case 0:
    execvp(args[0], (char **)&args[0]);
    perror(args[0]);
    _exit(-1);
  }
  int w;
  pid_t p;
  while((p = waitpid(pid, &w, 0)) < 0 && errno == EINTR)
    ;
  if(p < 0)
    throw IOError("waiting for subprocess");
  return w;
}
