// -*-C++-*-
// Copyright © 2011, 2012, 2014 Richard Kettlewell.
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
#ifndef ERRORS_H
#define ERRORS_H
/** @file Errors.h
 * @brief Exceptions
 */

#include <stdexcept>
#include <cstring>
#include <string>

/** @brief System-level error */
class SystemError: public std::runtime_error {
public:
  /** @brief Constructor
   * @param msg Error message
   */
  SystemError(const std::string &msg): std::runtime_error(msg), errno_value(0) {}

  /** @brief Constructor
   * @param msg Error message
   * @param error @c errno value
   */
  SystemError(const std::string &msg, int error):
    std::runtime_error(msg + ": " + strerror(error)),
    errno_value(error) {}

  /** @brief @c errno value */
  int errno_value;
};

/** @brief An I/O error */
class IOError: public SystemError {
public:
  /** @brief Constructor
   * @param msg Error message
   */
  IOError(const std::string &msg): SystemError(msg) {}

  /** @brief Constructor
   * @param msg Error message
   * @param error @c errno value
   */
  IOError(const std::string &msg, int error): SystemError(msg, error) {}
};

/** @brief A syntax error
 *
 * Does not include path/line information.
 */
class SyntaxError: public std::runtime_error {
public:
  /** @brief Constructor
   * @param msg Error message
   */
  SyntaxError(const std::string &msg):
    std::runtime_error(msg) {}
};

/** @brief Represents some problem with a config file
 *
 * Usually equivalent to a SyntaxError but with path/line information included
 * in the message.
 */
class ConfigError: public std::runtime_error {
public:
  /** @brief Constructor
   * @param msg Error message
   */
  ConfigError(const std::string &msg): std::runtime_error(msg) {}
};

/** @brief Represents some problem with a store
 *
 * BadStore errors are reported but are not fatal.
 */
class BadStore: public std::runtime_error {
public:
  /** @brief Constructor
   * @param msg Error message
   */
  BadStore(const std::string &msg): std::runtime_error(msg) {}
};

/** @brief Represents the problem that a store is not mounted
 *
 * UnavailableStore errors are 'normal' and are only displayed if
 * <tt>--warn-store</tt> is enabled.
 */
class UnavailableStore: public std::runtime_error {
public:
  /** @brief Constructor
   * @param msg Error message
   */
  UnavailableStore(const std::string &msg): std::runtime_error(msg) {}
};

/** @brief Represents some problem with a store
 *
 * FatalStoreErrors abort the whole process.
 */
class FatalStoreError: public std::runtime_error {
public:
  /** @brief Constructor
   * @param msg Error message
   */
  FatalStoreError(const std::string &msg): std::runtime_error(msg) {}
};

/** @brief Represents a problem with the command line */
class CommandError: public std::runtime_error {
public:
  /** @brief Constructor
   * @param msg Error message
   */
  CommandError(const std::string &msg): std::runtime_error(msg) {}
};

/** @brief Represents some problem with a date string */
class InvalidDate: public std::runtime_error {
public:
  /** @brief Constructor
   * @param msg Error message
   */
  InvalidDate(const std::string &msg): std::runtime_error(msg) {}
};

/** @brief Represents some problem with a regexp */
class InvalidRegexp: public SystemError {
public:
  /** @brief Constructor
   * @param msg Error message
   */
  InvalidRegexp(const std::string &msg): SystemError(msg) {}
};

/** @brief Represents failure of a subprocess */
class SubprocessFailed: public std::runtime_error {
public:
  /** @brief Constructor
   * @param name Subprocess name
   * @param wstat Wait status
   */
  SubprocessFailed(const std::string &name, int wstat): 
    std::runtime_error(format(name, wstat)) {}

  /** @brief Format the error message
   * @param name Subprocess name
   * @param wstat Wait status
   * @return Error string
   */
  static std::string format(const std::string &name, int wstat);
};

#endif /* ERRORS_H */
