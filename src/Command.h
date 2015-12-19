// -*-C++-*-
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
#ifndef COMMANDLINE_H
#define COMMANDLINE_H
/** @file Command.h
 * @brief Command-line parsing
 */

#include <string>
#include <vector>
#include <cstdio>
#include "Defaults.h"
#include "Selection.h"

struct option;

/** @brief Represents the parsed command line */
class Command {
public:
  Command() = default;
  Command(const Command &) = delete;
  Command &operator=(const Command &) = delete;

  /** @brief Verbosity of log summary in report */
  enum LogVerbosity {
    All,
    Errors,
    Recent,
    Latest,
    Failed,
  };

  /** @brief Parse command line arguments */
  void parse(int argc, const char *const *argv);

  /** @brief @c --backup action
   *
   * The default is @c false.
   */
  bool backup = false;

  /** @brief @c --prune action
   *
   * The default is @c false.
   */
  bool prune = false;

  /** @brief @c --prune-incomplete action
   *
   * The default is @c false.
   */
  bool pruneIncomplete = false;

  /** @brief @c --retire action
   *
   * The default is @c false.
   */
  bool retire = false;

  /** @brief @c --retire-device action
   *
   * The default is @c false.
   */
  bool retireDevice = false;

  /** @brief @c --dump-config action
   *
   * The default is @c false.
   */
  bool dumpConfig = false;

  /** @brief Output file for HTML report or null pointer */
  std::string *html = nullptr;

  /** @brief Output file for text report or null pointer */
  std::string *text = nullptr;

  /** @brief Address for email report or null pointer */
  std::string *email = nullptr;

  /** @brief Explicitly specified stores */
  std::vector<std::string> stores;

  /** @brief Wait if lock cannot be held
   *
   * The default is @c false.
   */
  bool wait = false;

  /** @brief Actually do something
   *
   * i.e. opposite of @c --no-act
   *
   * The default is @c true.
   */
  bool act = true;

  /** @brief Force retirement
   *
   * The default is @c false.
   */
  bool force = false;

  /** @brief Verbose operation
   *
   * The default is @c false.
   */
  bool verbose = false;

  /** @brief Warn for unknown objects
   *
   * The default is @c false.
   */
  bool warnUnknown = false;

  /** @brief Warn for unsuitable stores
   *
   * The default is @c false.
   */
  bool warnStore = false;

  /** @brief Warn for unreachable hosts
   *
   * The default is @c false.
   */
  bool warnUnreachable = false;

  /** @brief Warn about @c rsync partial transfer warnings
   *
   * The default is @c true.
   */
  bool warnPartial = true;

  /** @brief Repeat @c rsync errors
   *
   * The default is @c true.
   */
  bool repeatErrorLogs = true;

  /** @brief Log summary verbosity */
  LogVerbosity logVerbosity = Failed;

  /** @brief Devices selected for retirement */
  std::vector<std::string> devices;

  /** @brief Selections */
  VolumeSelections selections;

  /** @brief Convert verbosity from string
   * @param v Verbosity string from command line
   * @return Enumeration value
   */
  static LogVerbosity getVerbosity(const std::string &v);

  /** @brief Return the help string */
  static const char *helpString();

  /** @brief Option table
   * Used by getopt_long(3).
   */
  static const struct option options[];

private:
  /** @brief Display help message and terminate */
  void help();

  /** @brief Display version string and terminate */
  void version();
};

/** @brief Program command line */
extern Command command;

/** @brief Path to config file
 *
 * This defaults to @ref DEFAULT_CONFIG.
 */
extern std::string configPath;

/** @brief Database path */
extern std::string database;

/** @brief Issue debug output
 *
 * Affects the @ref D macro.
 *
 * The default is @c false.
 */
extern bool debug;

/** @brief Write a debug message to standard error
 *
 * The arguments are the same as @c printf().  A newline is added to the output
 * (so debug messages should not end with a newline).
 *
 * Only displays the a message if @ref Command::debug is set (in @ref
 * command).
 */
#define D(...) (void)(debug && fprintf(stderr, __VA_ARGS__) >= 0 && fputc('\n', stderr))

#endif /* COMMANDLINE_H */
