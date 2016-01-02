//-*-C++-*-
// Copyright © 2015 Richard Kettlewell.
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
#ifndef EVENTLOOP_H
#define EVENTLOOP_H
/** @file EventLoop.h
 * @brief Asynchronous operations
 *
 * To manage asynchronous operations:
 * - create a single @ref EventLoop object
 * - create file descriptors and subprocesses
 * - create @ref Reactor objects to handle I/O, timeouts and subprocesses
 * - attach the reactor objects to the event loop
 * - use @ref EventLoop::wait to start processing events
 *
 * Note that the event loop should be created before any subprocesses are
 * created; otherwise the signal handling will not work properly.
 */

#include <map>

class EventLoop;
class Reactor;

/** @brief An object that reacts to asynchronous events
 *
 * Reactors must be attached to an @ref EventLoop to be used, using one of the
 * following methods:
 * - @ref EventLoop::whenReadable
 * - @ref EventLoop::whenWritable
 * - @ref EventLoop::whenTimeout
 * - @ref EventLoop::whenWaited
 *
 * Normally you would implement at least one of the @c on... methods.  Those
 * that are not implemented will raise @c std::logic_error.  However they will
 * not be called unless attached to an event loop.
 */
class Reactor {
public:
  /** @brief Destructor */
  virtual ~Reactor() = default;

  /** @brief Called when a file descriptor is readable
   * @param e Calling event loop
   * @param fd File descriptor
   * @param ptr Bytes read from @p fd
   * @param n Number of bytes at @p ptr
   *
   * This will be called when bytes are read from @p fd, or when it reaches end
   * of file, if @ref EventLoop::whenReadable was used to attach this reactor
   * to an event loop.
   *
   * @p n will be 0 at EOF.  (Currently) the implementation must cancel reads
   * (using @ref EventLoop::cancelRead) even at EOF, or the event loop will
   * just call it again.
   */
  virtual void onReadable(EventLoop *e, int fd, const void *ptr, size_t n);

  /** @brief Called when a file descriptor cannot be read due to error
   * @param e Calling event loop
   * @param fd File descriptor
   * @param errno_value @c errno value
   *
   * This will be called when an error occurs while reading @p fd, if @ref
   * EventLoop::whenReadable was used to attach this reactor to an event loop.
   *
   * (Currently) the implementation must cancel reads (using @ref
   * EventLoop::cancelRead) even on error, or the event loop will just call it
   * again.
   */
  virtual void onReadError(EventLoop *e, int fd, int errno_value);

  /** @brief Called when a file descriptor is writable
   * @param e Calling event loop
   * @param fd File descriptor
   *
   * This will be called when @p fd is writable, if @ref
   * EventLoop::whenWritable was used to attach this reactor to an event loop.
   *
   * The implementation is responsible for performing the write and for
   * cancelling further writes (using @ref EventLoop::cancelWrite) if there is
   * nothing more to write or on error.
   */
  virtual void onWritable(EventLoop *e, int fd);

  /** @brief Called when a timeout occurs
   * @param e Calling event loop
   * @param now (Monotonic) timestamp (see @ref getMonotonicTime)
   *
   * This will be called when a time limit is reached, if @ref
   * EventLoop::whenTimeout was used to attach this reactor to an event loop.
   *
   * @p now may be after the time limit, but never before.
   */
  virtual void onTimeout(EventLoop *e, const struct timespec &now);

  /** @brief Called when a subprocess terminates
   * @param e Calling event loop
   * @param pid Subprocess
   * @param status Wait status
   * @param ru Resource usage
   *
   * This will be called when a subprocess terminates, if @ref
   * EventLoop::whenWaited was used to attach this reactor to an event loop.
   *
   * (In contrast to the read/write methods) the event loop stops waiting for
   * the process before this call is made.
   */
  virtual void onWait(EventLoop *e, pid_t pid,
                      int status, const struct rusage &ru);
};

/** @brief An event loop supporting asynchronous I/O
 *
 * Event loops allow multiple file descriptors and subprocesses to be managed
 * together.  All I/O, subprocess and timeout events are reflected in calls to
 * methods of the @ref Reactor class.
 *
 * (Currently) only one event loop object may exist at a time.  This limitation
 * is due to the signal handling strategy.
 */
class EventLoop: private Reactor {
public:
  /** @brief Construct an event loop */
  EventLoop();

  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;

  /** @brief Destroy an event loop */
  ~EventLoop() override;

  /** @brief Notify reactor when a file descriptor is readable
   * @param fd File descriptor to monitor
   * @param r Reactor to notify
   *
   * The reactor is notified by calling @ref Reactor::onReadable and @ref
   * Reactor::onReadError.
   */
  void whenReadable(int fd, Reactor *r);

  /** @brief Stop monitoring a file descriptor for readability
   * @param fd File descriptor to stop monitoring
   */
  void cancelRead(int fd);

  /** @brief Notify reactor when a file descriptor is writable
   * @param fd File descriptor to monitor
   * @param r Reactor to notify
   *
   * The reactor is notified by calling @ref Reactor::onWritable.
   */
  void whenWritable(int fd, Reactor *r);

  /** @brief Stop monitoring a file descriptor for writability
   * @param fd File descriptor to stop monitoring
   */
  void cancelWrite(int fd);

  /** @brief Notify a reactor at a future time
   * @param t (Monotonic) timestamp to wait for (see @ref getMonotonicTime)
   * @param r Reactor to notify
   *
   * The reactor is notified by calling @ref Reactor::onTimeout.
   */
  void whenTimeout(const struct timespec &t, Reactor *r);

  /** @brief Notify a reactor when a subprocess terminates
   * @param pid Subprocess
   * @param r Reactor to notify
   *
   * The reactor is notified by calling @ref Reactor::onWait.
   *
   * Note that the event loop should be created before any subprocesses are
   * created; otherwise the signal handling will not work properly.
   */
  void whenWaited(pid_t pid, Reactor *r);

  /** @brief Stop monitoring a subprocess
   * @param pid Subprocess to stop monitoring
   */
  void cancelWait(pid_t pid);

  /** @brief Wait until there is nothing left to wait for
   *
   * This does not return until all readers and writers have been cancelled and
   * all subprocesses have been waited for.  It does not wait for unreached
   * tmieouts, however.
   */
  void wait();

private:
  /** @brief File descriptors monitored for reading */
  std::map<int, Reactor *> readers;

  /** @brief File descriptors monitored for writing */
  std::map<int, Reactor *> writers;

  /** @brief Timeouts */
  std::multimap<struct timespec, Reactor *> timeouts;

  /** @brief Subprocesses */
  std::map<pid_t, Reactor *> waiters;

  /** @brief Set if reactors change
   *
   * This variable allows the event loop to detect that one of the
   * EventLoop::readers, EventLoop::writers, EventLoop::timeouts or
   * EventLoop::waiters has changed, and therefore to stop relying on
   * iterators that refer to them.
   */
  bool reconf;

  /** @brief Signal handler */
  static void signalled(int);

  /** @brief Signal pipe
   *
   * @ref EventLoop::signalled writes bytes to the pipe to wake up the event
   * loop when a signal occurs.  Handled signals are disabled except when
   * actually waiting for events.
   */
  static int sigpipe[2];

  void onReadable(EventLoop *e, int fd, const void *ptr, size_t n) override;
};

#endif /* EVENTLOOP_H */
