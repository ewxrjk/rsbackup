//-*-C++-*-
// Copyright © 2011, 2012, 2014-2017 Richard Kettlewell.
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
#include "Action.h"

/** @brief Subprocess execution
 *
 * This class supports three modes of operation.
 *
 * 1. Standalone synchronous execution.  The caller sets up the subprocess and
 * then invokes @ref Subprocess::runAndWait.  An internal @ref EventLoop is
 * used.  In this case it is possible to capture output to strings with @ref
 * Subprocess::capture but not to manage pipes directly since the caller has no
 * opportunity to read or write them.  Only one subprocess may exist at a time.
 *
 * 2. Standalone asynchronous execution.  The caller sets up the subprocess and
 * then invokes @ref Subprocess::run.  Later the caller invokes @ref
 * Subprocess::wait.  As above, an internal @ref EventLoop is used.  In this
 * case it is possible to manage pipes directly with @ref
 * Subprocess::addChildFD.  Captures will work with small outputs but since the
 * event loop only has an opportunity to read or write the capture pipes when
 * @c wait() is called, they do not work in general.  Only one subprocess may
 * exist at a time.
 *
 * 3. Concurrent execution as part of an @ref ActionList.  The caller sets up
 * many subprocesses and adds them to the action list with @ref
 * ActionList::add, before starting them and waiting for them with @ref
 * ActionList::go.  When the action list is complete, wait statuses may be
 * retrieved with @ref Subprocess::getStatus.  String captures will work and if
 * the caller registers reactors with the event loop then pipes can also be
 * managed directly.
 */
class Subprocess: private Reactor, public Action {
public:
  /** @brief Possible wait behaviors */
  enum WaitBehavior {
    /** @brief Throw if the process terminates normally with nonzero status */
    THROW_ON_ERROR = 1,

    /** @brief Throw if the process terminates due to a signal other than SIGPIPE */
    THROW_ON_CRASH = 2,

    /** @brief Throw if the process terminates due to SIGPIPE */
    THROW_ON_SIGPIPE = 4,
  };
  /** @brief Constructor
   */
  Subprocess(): Subprocess("<anon>") {}

  /** @brief Constructor
   * @param name Action name
   */
  Subprocess(const std::string &name);

  /** @brief Constructor
   * @param cmd Command that will be executed
   */
  Subprocess(const std::vector<std::string> &cmd):
    Subprocess("<anon>", cmd) {}

  /** @brief Constructor
   * @param name Action name
   * @param cmd Command that will be executed
   */
  Subprocess(const std::string &name, const std::vector<std::string> &cmd);

  /** @brief Destructor */
  ~Subprocess() override;

  /** @brief Set the command to execute
   * @param cmd Command that will be executed
   */
  void setCommand(const std::vector<std::string> &cmd);

  /** @brief Get the command to execute
   * @return Command that will be executed
   */
  const std::vector<std::string> &getCommand() const {
    return cmd;
  }

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

  /** @brief Configure reporting
   * @param reportCommand The command must be logged
   * @param reportNow Log command immediately
   *
   * If @p reportCommand is @c true then the command will be reported, either
   * now or just before execution.
   *
   * If @p reportNow is @c true then if the command is to be reported, it will
   * happen immediately.
   *
   * A command is never reported more than once.
   */
  void reporting(bool reportCommand, bool reportNow);

  /** @brief Wait for the subprocess.
   * @param waitBehaviour How to check exit status
   * @return Wait status
   *
   * @p waitBehaviour may contain the following bits:
   * - @ref THROW_ON_ERROR - throw if the process terminates normally with nonzero status.
   * - @ref THROW_ON_CRASH - throw if the process terminates due to a signal other than SIGPIPE.
   * - @ref THROW_ON_SIGPIPE - throw if the process terminates due to SIGPIPE.
   */
  int wait(unsigned waitBehaviour = THROW_ON_ERROR|THROW_ON_CRASH);

  /** @brief Run and then wait
   * @param waitBehaviour How to check exit status
   * @return Wait status
   *
   * @p waitBehaviour may contain the following bits:
   * - @ref THROW_ON_ERROR - throw if the process terminates normally with nonzero status.
   * - @ref THROW_ON_CRASH - throw if the process terminates due to a signal other than SIGPIPE.
   * - @ref THROW_ON_SIGPIPE - throw if the process terminates due to SIGPIPE.
   */
  int runAndWait(unsigned waitBehaviour = THROW_ON_ERROR|THROW_ON_CRASH) {
    run();
    return wait(waitBehaviour);
  }

  /** @brief Return the wait status
   * @return Wait status
   *
   * Meaningless until the process has terminated.
   */
  int getStatus() const {
    return status;
  }

  /** @brief Get the status to report to @ref ActionList::completed */
  virtual bool getActionStatus() const;

  /** @brief Find @p name on the path
   * @param name Program to execute
   * @return Full path to @p name, or "" if not found
   */
  static std::string pathSearch(const std::string &name);

private:
  /** @brief Process ID of child
   * Set to -1 before there is a child.
   */
  pid_t pid = -1;

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
  std::map<std::string, std::string> env;

  /** @brief Ouptuts to capture from the child
   *
   * Keys are file descriptors to read from, values are pointers to the strings
   * used to accumulate the output.
   */
  std::map<int, std::string *> captures;

  /** @brief Launch subprocess
   * @param e Event loop
   * @return Process ID
   */
  pid_t launch(EventLoop *e);

  /** @brief Setup event loop integration
   * @param e Event loop
   */
  void setup(EventLoop *e);

  /** @brief Report what will or would be run */
  void report();

  /** @brief Timeout after which child is killed, in seconds
   * 0 means no timeout: the child may run indefinitely.
   */
  int timeout = 0;

  void onReadable(EventLoop *e, int fd, const void *ptr, size_t n) override;
  void onReadError(EventLoop *e, int fd, int errno_value) override;
  void onTimeout(EventLoop *e, const struct timespec &now) override;
  void onWait(EventLoop *e, pid_t pid, int status, const struct rusage &ru) override;
  void go(EventLoop *e, ActionList *al) override;

  /** @brief Wait status */
  int status = -1;

  /** @brief Containing action list */
  ActionList *actionlist = nullptr;

  /** @brief Private event loop */
  EventLoop *eventloop = nullptr;

  /** @brief True if the command has been logged */
  bool reported = false;

  /** @brief True if the command needs to be logged */
  bool reportNeeded = false;
};

#endif /* SUBPROCESS_H */
