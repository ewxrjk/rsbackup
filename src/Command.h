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

struct option;

/** @brief Represents the parsed command line */
class Command {
public:
  Command() = default;
  Command(const Command &) = delete;
  Command &operator=(const Command &) = delete;

  /** @brief Represents a selection
   *
   * A selection is a volume or collection of volumes and a positive or
   * negative sense (represented by @c true and @c false respectively).  The
   * list of selections in @ref Command::selections determines which volumes an
   * operation applies to.
   */
  struct Selection {
    /** @brief Construct a selection
     * @param host_ Host or "*" for all hosts
     * @param volume_ Volume or "*" for all hosts
     * @param sense_ @c true for "+ and @c false for "-"
     *
     * A @p host_ of "*" but a @p volume_ not equal to "*" does not make sense
     * and will fail in @ref Conf::selectVolume.
     */
    Selection(const std::string &host_,
              const std::string &volume_,
              bool sense_ = true);

    /** @brief Sense of selection
     *
     * @c true for "+" and @c false for "-"
     */
    bool sense;

    /** @brief Host name or "*"
     *
     * "*" means all hosts.
     */
    std::string host;

    /** @brief Volume name or "*"
     *
     * "*" means all volumes.
     */
    std::string volume;
  };

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

  /** @brief Select volumes
   *
   * Invokes Conf::selectVolumes() as required.
   */
  void selectVolumes();

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

  /** @brief Path to config file
   *
   * This defaults to @ref DEFAULT_CONFIG.
   */
  std::string configPath = DEFAULT_CONFIG;

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

  /** @brief Issue debug output
   *
   * Affects the @ref D macro.
   *
   * The default is @c false.
   */
  bool debug = false;

  /** @brief Devices selected for retirement */
  std::vector<std::string> devices;

  /** @brief Selections */
  std::vector<Selection> selections;

  /** @brief Database path */
  std::string database;

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
  [[noreturn]] void help();

  /** @brief Display version string and terminate */
  [[noreturn]] void version();
};

/** @brief Program command line */
extern Command command;

/** @brief Write a debug message to standard error
 *
 * The arguments are the same as @c printf().  A newline is added to the output
 * (so debug messages should not end with a newline).
 *
 * Only displays the a message if @ref Command::debug is set (in @ref
 * command).
 */
#define D(...) (void)(command.debug && fprintf(stderr, __VA_ARGS__) >= 0 && fputc('\n', stderr))

#endif /* COMMANDLINE_H */
