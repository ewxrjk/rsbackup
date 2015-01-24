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
#ifndef COMMANDLINE_H
#define COMMANDLINE_H
/** @file Command.h
 * @brief Command-line parsing
 */

#include <string>
#include <vector>
#include <cstdio>

struct option;

/** @brief Represents the parsed command line */
class Command {
public:
  /** @brief Represents a selection */
  struct Selection {
    /** @brief Construct a selection
     * @param host_ Host or "*"
     * @param volume_ Volume or "*"
     * @param sense_ @c true for "+ and @c false for "-"
     */
    Selection(const std::string &host_,
              const std::string &volume_,
              bool sense_ = true);

    /** @brief Sense of selection
     *
     * @c true for "+ and @c false for "-"
     */
    bool sense;

    /** @brief Host name or "*" */
    std::string host;

    /** @brief Volume name or "*" */
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

  /** @brief Construct a default command line */
  Command();

  /** @brief Parse command line arguments */
  void parse(int argc, const char *const *argv);

  /** @brief Select volumes
   *
   * Invokes Conf::selectVolumes() as required.
   */
  void selectVolumes();

  /** @brief @c --backup action */
  bool backup;

  /** @brief @c --prune action */
  bool prune;

  /** @brief @c --prune-incomplete action */
  bool pruneIncomplete;

  /** @brief @c --retire action */
  bool retire;

  /** @brief @c --retire-device action */
  bool retireDevice;

  /** @brief @c --dump-config action */
  bool dumpConfig;

  /** @brief Output file for HTML report or @c NULL */
  std::string *html;

  /** @brief Output file for text report or @c NULL */
  std::string *text;

  /** @brief Address for email report or @c NULL */
  std::string *email;

  /** @brief Explicitly specified stores */
  std::vector<std::string> stores;

  /** @brief Path to config file */
  std::string configPath;

  /** @brief Wait if lock cannot be held */
  bool wait;

  /** @brief Actually do something.
   *
   * i.e. opposite of @c --no-act
   */
  bool act;

  /** @brief Force retirement */
  bool force;

  /** @brief Verbose operation */
  bool verbose;

  /** @brief Warn for unknown objects */
  bool warnUnknown;

  /** @brief Warn for unsuitable stores */
  bool warnStore;

  /** @brief Warn for unreachable hosts */
  bool warnUnreachable;

  /** @brief Warn about @c rsync partial transfer warnings */
  bool warnPartial;

  /** @brief Repeat @c rsync errors */
  bool repeatErrorLogs;

  /** @brief Log summary verbosity */
  LogVerbosity logVerbosity;

  /** @brief Issue debug output */
  bool debug;

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
  void help();

  /** @brief Display version string and terminate */
  void version();
};

/** @brief Program command line */
extern Command command;

/** @brief Display a debug message
 * Arguments as per @c printf().
 */
#define D(...) (void)(command.debug && fprintf(stderr, __VA_ARGS__) >= 0 && fputc('\n', stderr))

#endif /* COMMANDLINE_H */
