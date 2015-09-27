//-*-C++-*-
// Copyright © 2011, 2012, 2014, 2015 Richard Kettlewell.
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
#include <sys/types.h>
#include "EventLoop.h"

/** @brief Subprocess execution */
class Subprocess: private Reactor {
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
   * @param otherChildFD Another child file descriptor to redirect
   *
   * In the child dup @p pipeFD onto @p childFD and close @p closeFD.
   *
   * @p pipeFD is automatically closed in the parent, but only after all
   * redirections have taken place, so it may be used more than once.
   */
  void addChildFD(int childFD, int pipeFD, int closeFD = -1,
                  int otherChildFD = -1);

  /** @brief Null out a file descriptor
   * @param childFD Child file descriptor
   *
   * In the child dup @c /dev/null onto @p childFD.
   */
  void nullChildFD(int childFD);

  /** @brief Capture output from the child
   * @param childFD Child file descriptor to capture
   * @param s Where to put result
   * @param otherChildFD Another child file descriptor to capture
   *
   * The capture is performed in wait();
   */
  void capture(int childFD, std::string *s, int otherChildFD=-1);

  /** @brief Set an environment variable in the child
   * @param name Environment variable name
   * @param value Environment variable value
   */
  void setenv(const std::string &name,
              const std::string &value) {
    env[name] = value;
  }

  /** @brief Set the child timeout
   * @param seconds Number of seconds after which to give up and kill child
   */
  void setTimeout(int seconds) {
    timeout = seconds;

  }

  /** @brief Start subprocess
   * @return Process ID
   */
  pid_t run();

  /** @brief Report what would be run */
  void report();

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

private:
  /** @brief Process ID of child
   * Set to -1 before there is a child.
   */
  pid_t pid;

  /** @brief A rule for file descriptor changes in the child process */
  struct ChildFD {
    /** @brief Child file descriptor to redirect */
    int child;

    /** @brief Child's pipe endpoint, or -1
     *
     * If this is -1 then the child's file descriptor is redirected to @c
     * /dev/null
     */
    int pipe;

    /** @brief Child file descriptor to close or -1
     *
     * If this is not -1 then this file descriptor will be closed in the child.
     * This is used for the parent's pipe endpoints.
     */
    int close;

    /** @brief Other child file descriptor to redirect or -1 */
    int childOther;

    /** @brief Construct a ChildFD
     * @param child_ Child file descriptor to redirect
     * @param pipe_ Child's pipe endpoint, or -1
     * @param close_ File descriptor to close in child, or -1
     * @param childOther_ Another child file descriptor to redirect, or -1
     */
    ChildFD(int child_, int pipe_, int close_, int childOther_):
      child(child_),
      pipe(pipe_),
      close(close_),
      childOther(childOther_) {}
  };

  /** @brief Rules for file descriptor changes in the child process */
  std::vector<ChildFD> fds;

  /** @brief Command to execute in the child */
  std::vector<std::string> cmd;

  /** @brief Environment variables to set in the child */
  std::map<std::string,std::string> env;

  /** @brief Ouptuts to capture from the child
   *
   * Keys are file descriptors to read from, values are pointers to the strings
   * used to accumulate the output.
   */
  std::map<int,std::string *> captures;

  /** @brief Timeout after which child is killed, in seconds
   * 0 means no timeout: the child may run indefinitely.
   */
  int timeout;

  /** @brief Get a timestamp for the current time
   * @param now Where to store timestamp
   *
   * If possible the monotonic clock is used.  Otherwise the real time clock is
   * used.
   */
  static void getTimestamp(struct timespec &now);

  void onReadable(EventLoop *e, int fd, const void *ptr, size_t n);
  void onReadError(EventLoop *e, int fd, int errno_value);
  void onTimeout(EventLoop *e, const struct timespec &now);
  void onWait(EventLoop *e, pid_t pid, int status, const struct rusage &ru);
  int status;
};

#endif /* SUBPROCESS_H */

