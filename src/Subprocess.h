//-*-C++-*-
// Copyright © 2011, 2012 Richard Kettlewell.
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
#ifndef SUBPROCESS_H
#define SUBPROCESS_H
/** @file Subprocess.h
 * @brief Subprocess/command execution support
 */

#include <vector>
#include <string>
#include <map>

/** @brief Subprocess execution */
class Subprocess {
public:
  /** @brief Constructor */
  Subprocess();

  /** @brief Constructor
   * @param cmd Command that will be executed
   */
  Subprocess(const std::vector<std::string> &cmd);

  /** @brief Destructor */
  ~Subprocess();

  /** @brief Set the command to execute
   * @param cmd Command that will be executed
   */
  void setCommand(const std::vector<std::string> &cmd);

  /** @brief Add a pipe
   * @param childFD Child file descriptor to redirect
   * @param pipeFD Child end of pipe
   * @param closeFD Parent end of pipe
   *
   * In the child dup @p pipeFD onto @p childFD and close @p closeFD.
   *
   * @p pipeFD is automatically closed in the parent, but only after all
   * redirections have taken place, so it may be used more than once.
   */
  void addChildFD(int childFD, int pipeFD, int closeFD = -1);

  /** @brief Null out a file descriptor
   * @param childFD Child file descriptor
   *
   * In the child dup @c /dev/null onto @p childFD.
   */
  void nullChildFD(int childFD);

  /** @brief Set an environment variable in the child
   * @param name Environment variable name
   * @param value Environment variable value
   */
  void setenv(const std::string &name,
              const std::string &value) {
    env[name] = value;
  }

  /** @brief Start subprocess
   * @return Process ID
   */
  pid_t run();

  /** @brief Wait for the subprocess.
   * @param checkStatus Throw on abnormal termination
   * @return Wait status
   *
   * If @p checkStatus is true, throws on any kind of abnormal termination
   * (which includes nonzero exit() and most signals but not SIGPIPE).  Returns
   * the wait status (if it doesn't throw).
   */
  int wait(bool checkStatus = true);

  /** @brief Run and then wait
   * @param checkStatus Throw on abnormal termination
   * @return Wait status
   */
  int runAndWait(bool checkStatus = true) {
    run();
    return wait(checkStatus);
  }

  /** @brief Execute a command and return its wait status
   * @param cmd Command to execute
   * @param checkStatus Throw on abnormal termination
   * @return Wait status
   */
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
  std::map<std::string,std::string> env;
};

#endif /* SUBPROCESS_H */

