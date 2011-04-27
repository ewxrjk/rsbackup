//-*-C++-*-
#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include <vector>
#include <string>

class Subprocess {
public:
  Subprocess();
  Subprocess(const std::vector<std::string> &cmd);
  ~Subprocess();

  // Set the command
  void setCommand(const std::vector<std::string> &cmd);

  // In the child dup pipeFD onto childFD and close closeFD.  pipeFD is
  // automatically closed in the parent, but only after all redirections have
  // taken place, so pipeFD may be used more than once.
  void addChildFD(int childFD, int pipeFD, int closeFD = -1);

  // In the child dup /dev/null onto childFD
  void nullChildFD(int childFD);

  // Start the subprocess.  Returns process ID.
  pid_t run();

  // Wait for the subprocess.  If checkStatus is true, throws on any kind of
  // abnormal termination (which includes nonzero exit() and most signals but
  // not SIGPIPE).  Returns the wait status (if it doesn't throw).
  int wait(bool checkStatus = true);

  int runAndWait(bool checkStatus = true) {
    run();
    return wait(checkStatus);
  }

  // Just execute a command and return its wait status
  static int execute(const std::vector<std::string> &cmd,
                     bool checkStatus = true);

private:
  pid_t pid;
  struct ChildFD {
    int child;                          // child FD to overwrite
    int pipe;                           // pipe FD or -1
    int close;                          // FD to close or -1
    ChildFD(int child_, int pipe_, int close_): child(child_),
                                                pipe(pipe_),
                                                close(close_) {}
  };
  std::vector<ChildFD> fds;
  std::vector<std::string> cmd;
};

#endif /* SUBPROCESS_H */

