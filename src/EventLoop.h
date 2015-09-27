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
 */

#include <map>

class EventLoop;
class Reactor;

/** @brief An event loop supporting asynchronous IO */
class EventLoop {
public:
  /** @brief Construct an event loop */
  EventLoop();

  /** @brief Destroy an event loop */
  ~EventLoop();

  /** @brief Notify reactor when a file descriptor is readable
   * @param fd File descriptor to monitor
   * @param r Reactor to notify
   */
  void onRead(int fd, Reactor *r);

  /** @brief Stop monitoring a file descriptor for readability
   * @param fd File descriptor to stop monitoring
   */
  void cancelRead(int fd);

  /** @brief Notify reactor when a file descriptor is writable
   * @param fd File descriptor to monitor
   * @param r Reactor to notify
   */
  void onWrite(int fd, Reactor *r);

  /** @brief Stop monitoring a file descriptor for writability
   * @param fd File descriptor to stop monitoring
   */
  void cancelWrite(int fd);

  /** @brief Notify a reactor at a future time
   * @param t Timestamp to wait for
   * @param r Reactor to notify
   */
  void onTimeout(const struct timespec &t, Reactor *r);

  /** @brief Wait until no more file descriptors are being monitored */
  void wait();

  /** @brief Get the current (monotonic) time */
  static void getTimestamp(struct timespec &now);

private:
  /** @brief File descriptors monitored for reading */
  std::map<int, Reactor *> readers;

  /** @brief File descriptors monitored for writing */
  std::map<int, Reactor *> writers;

  /** @brief Timeouts */
  std::multimap<struct timespec, Reactor *> timeouts;

  /** @brief Set if reactors change */
  bool reconf;
};

/** @brief An object that reacts to asynchronous events */
class Reactor {
public:
  /** @brief Destructor */
  virtual ~Reactor();

  /** @brief Called when a file descriptor is readable
   * @param e Calling event loop
   * @param fd File descriptor
   * @param ptr Bytes read from @p fd
   * @param n Number of bytes at @p ptr
   *
   * @p n will be 0 at EOF.
   */
  virtual void onReadable(EventLoop *e, int fd, const void *ptr, size_t n);

  /** @brief Called when a file descriptor cannot be read due to error
   * @param e Calling event loop
   * @param fd File descriptor
   * @param errno_value @c errno value
   */
  virtual void onReadError(EventLoop *e, int fd, int errno_value);

  /** @brief Called when a file descriptor is writable
   * @param e Calling event loop
   * @param fd File descriptor
   */
  virtual void onWritable(EventLoop *e, int fd);

  /** @brief Called when a timeout occurs
   * @param e Calling event loop
   * @param now Timestamp
   */
  virtual void onTimeout(EventLoop *e, const struct timespec &now);
};

#endif /* EVENTLOOP_H */
