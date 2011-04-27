#include <config.h>
#include "Subprocess.h"
#include "Errors.h"
#include "Command.h"
#include <csignal>
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

Subprocess::Subprocess(): pid(-1) {
}

Subprocess::Subprocess(const std::vector<std::string> &cmd_): 
  pid(-1),
  cmd(cmd_) {
}

Subprocess::~Subprocess() {
  if(pid >= 0) {
    kill(pid, SIGKILL);
    try { wait(false); } catch(...) {}
  }
}

void Subprocess::setCommand(const std::vector<std::string> &cmd_) {
  cmd = cmd_;
}

void Subprocess::addChildFD(int childFD, int pipeFD, int closeFD) {
  fds.push_back(ChildFD(childFD, pipeFD, closeFD));
}

void Subprocess::nullChildFD(int childFD) {
  fds.push_back(ChildFD(childFD, -1, -1));
}

pid_t Subprocess::run() {
  if(pid >= 0)
    throw std::logic_error("Subprocess::run but already running");
  // Convert the command
  std::vector<const char *> args;
  for(size_t n = 0; n < cmd.size(); ++n)
    args.push_back(cmd[n].c_str());
  args.push_back(NULL);
  // Display the command
  if(command.verbose) {
    printf(">");
    for(size_t n = 0; n < cmd.size(); ++n)
      printf(" %s", cmd[n].c_str());
    printf("\n");
  }
  // Start the subprocess
  switch(pid = fork()) {
  case -1:
    throw IOError("creating subprocess for " + cmd[0], errno); // TODO exception??
  case 0:
    {
      int nullfd = -1;
      for(size_t n = 0; n < fds.size(); ++n) {
        const ChildFD &cfd = fds[n];
        if(cfd.pipe >= 0) {
          if(dup2(cfd.pipe, cfd.child) < 0) { perror("dup2"); _exit(-1); }
        } else {
          if(nullfd < 0 && (nullfd = open("/dev/null", O_RDWR)) < 0) {
            perror("/dev/null");
            _exit(-1);
          }
          if(dup2(nullfd, cfd.child) < 0) { perror("dup2"); _exit(-1); }
        }
      }
      for(size_t n = 0; n < fds.size(); ++n) {
        const ChildFD &cfd = fds[n];
        if(cfd.pipe >= 0) {
          if(close(cfd.pipe) < 0) { perror("close"); _exit(-1); }
          for(size_t m = n + 1; m < fds.size(); ++m)
            if(fds[m].pipe == cfd.pipe)
              fds[m].pipe = -1;
        }
        if(cfd.close >= 0)
          if(close(cfd.close) < 0) { perror("close"); _exit(-1); }
      }
      if(nullfd >= 0 && close(nullfd) < 0) { perror("close"); _exit(-1); }
      execvp(args[0], (char **)&args[0]);
      perror(args[0]);
      _exit(-1);
    }
  }
  for(size_t n = 0; n < fds.size(); ++n) {
    const ChildFD &cfd = fds[n];
    if(cfd.pipe >= 0)
      if(close(cfd.pipe) < 0)
        throw IOError("close", errno);
  }
  return pid;
}

int Subprocess::wait(bool checkStatus) {
  if(pid < 0)
    throw std::logic_error("Subprocess::wait but not running");
  int w;
  pid_t p;
  
  while((p = waitpid(pid, &w, 0)) < 0 && errno == EINTR)
    ;
  if(p < 0)
    throw IOError("waiting for subprocess");
  if(checkStatus && w) {
    if(WIFSIGNALED(w)) {
      int sig = WTERMSIG(w);
      if(sig != SIGPIPE)
        throw std::runtime_error(cmd[0] + ": " + strsignal(sig)
                                 + (WCOREDUMP(w) ? " (core dumped)" : "")); // TODO exception class
    } else if(WIFEXITED(w)) {
      char buffer[10];
      int rc = WEXITSTATUS(w);
      sprintf(buffer, "%d", rc);
      throw std::runtime_error(cmd[0] + ": exited with status " + buffer); // TODO exception class
    }
  }
  pid = -1;
  return w;
}

int Subprocess::execute(const std::vector<std::string> &cmd,
                        bool checkStatus) {
  Subprocess sp(cmd);
  return sp.runAndWait(checkStatus);
}
