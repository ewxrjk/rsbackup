// -*-C++-*-
// Copyright © 2011, 2012, 2014-2017, 2019 Richard Kettlewell.
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
#ifndef CONFBASE_H
#define CONFBASE_H
/** @file ConfBase.h
 * @brief Base class for program configuration and state
 */

#include <map>
#include <string>
#include <vector>

#include "Defaults.h"

/** @brief A policy parameter value */
class PolicyParameter {
public:
  /** @brief Constructor */
  PolicyParameter(const std::string &value = ""): value(value) {}

  /** @brief Constructor */
  PolicyParameter(const std::string &value, const Location &l);

  /** @brief Parameter value */
  std::string value;

  /** @brief Location where value came from */
  Location location;
};

/** @brief Base for Volume, Host and Conf
 *
 * Volume, Host and Conf share certain parameters, which are inherited from
 * container to contained.  They are implemented in this class.
 */
class ConfBase {
public:
  /** @brief Constructor with no parent
   *
   * All parameters are given their default values.
   *
   * @see @ref Defaults.h
   */
  ConfBase() = default;

  ConfBase(const ConfBase &) = delete;
  ConfBase &operator=(const ConfBase &) = delete;

  /** @brief Constructor that inherits from a parent
   * @param parent Parent container
   */
  ConfBase(ConfBase *parent):
      maxAge(parent->maxAge), backupPolicy(parent->backupPolicy),
      backupParameters(parent->backupParameters),
      prunePolicy(parent->prunePolicy),
      pruneParameters(parent->pruneParameters), preVolume(parent->preVolume),
      postVolume(parent->postVolume),
      backupJobTimeout(parent->backupJobTimeout),
      rsyncIOTimeout(parent->rsyncIOTimeout),
      rsyncCommand(parent->rsyncCommand),
      rsyncBaseOptions(parent->rsyncBaseOptions),
      rsyncExtraOptions(parent->rsyncExtraOptions),
      rsyncRemote(parent->rsyncRemote), rsyncLinkDest(parent->rsyncLinkDest),
      sshTimeout(parent->sshTimeout), hookTimeout(parent->hookTimeout),
      hostCheck(parent->hostCheck), devicePattern(parent->devicePattern) {}

  virtual ~ConfBase() = default;

  /** @brief Maximum comfortable age of most recent backup
   *
   * Corresponds to @c max-age. */
  int maxAge = DEFAULT_MAX_AGE;

  /** @brief Name of pruning policy */
  std::string backupPolicy = DEFAULT_BACKUP_POLICY;

  /** @brief Backup policy parameters */
  std::map<std::string, PolicyParameter> backupParameters;

  /** @brief Name of pruning policy */
  std::string prunePolicy = DEFAULT_PRUNE_POLICY;

  /** @brief Pruning policy parameters */
  std::map<std::string, PolicyParameter> pruneParameters;

  /** @brief Pre-volume hook */
  std::vector<std::string> preVolume;

  /** @brief Post-volume hook */
  std::vector<std::string> postVolume;

  /** @brief backup job timeout */
  int backupJobTimeout = 0;

  /** @brief rsync IO timeout */
  int rsyncIOTimeout = 0;

  /** @brief rsync command */
  std::string rsyncCommand = "rsync";

  /** @brief rsync base options */
  std::vector<std::string> rsyncBaseOptions = {
      "--archive", // == -rlptgoD
      // --recursive                         recurse into directories
      // --links                             preserve symlinks
      // --perms                             preserve permissions
      // --times                             preserve modification times
      // --group                             preserve group IDs
      // --owner                             preserve user IDs
      // --devices                           preserve device files
      // --specials                          preserve special files
      "--sparse",            // handle spare files efficiently
      "--numeric-ids",       // don't remap UID/GID by name
      "--compress",          // compress during file transfer
      "--fuzzy",             // look for similar files
      "--hard-links",        // preserve hard links
      "--delete",            // delete extra files in destination
      "--stats",             // generate file transfer stats
      "--no-human-readable", // suppress localization of transfer stats
  };

  /** @brief rsync extra options */
  std::vector<std::string> rsyncExtraOptions = {
      "--xattrs",       // preserve extended attributes
      "--acls",         // preserve ACLs
      "--open-noatime", // suppress atime changes
  };

  /** @brief rsync @c --rsync-path override */
  std::string rsyncRemote;

  /** @brief whether to enable --link-dest */
  bool rsyncLinkDest = true;

  /** @brief Timeout to pass to SSH */
  int sshTimeout = DEFAULT_SSH_TIMEOUT;

  /** @brief hook timeout */
  int hookTimeout = 0;

  /** @brief Host check behavior */
  std::vector<std::string> hostCheck;

  /** @brief Device pattern to be used */
  std::string devicePattern = "*";

  /** @brief Write out the value of a vector directive
   * @param os Output stream
   * @param step Indent depth
   * @param directive Name of directive
   * @param value Value of directive
   */
  void writeVector(std::ostream &os, int step, const std::string &directive,
                   const std::vector<std::string> &value) const;

  /** @brief Write this node to a stream
   * @param os Output stream
   * @param step Indent depth
   * @param verbose Include informative annotations
   */
  virtual void write(std::ostream &os, int step, bool verbose) const;

  /** @brief Write the host-check configuration to a stream
   * @param os Output stream
   * @param step Indent depth
   * @param verbose Include informative annotations
   */
  void writeHostCheck(std::ostream &os, int step, bool verbose) const;

  /** @brief Return the parent of this configuration node
   * @return Parent node or null pointer
   */
  virtual ConfBase *getParent() const = 0;

  /** @brief Return a description of this node */
  virtual std::string what() const = 0;

protected:
  /** @brief Quote a string for use in the config file
   * @param s String to quote
   * @return Possibly quoted form of @p s
   */
  static std::string quote(const std::string &s);

  /** @brief Quote a list of strings for use in the config file
   * @param vs String to quote
   * @return @p vs with appropriate quoting
   */
  static std::string quote(const std::vector<std::string> &vs);

  /** @brief Construct indent text
   * @param step Indent depth
   * @return String containing enough spaces
   */
  static std::string indent(int step);

  /** @brief Type for description functions
   * @param os Output stream
   * @param description Description
   * @param step Indent depth
   *
   * See ConfBase::write and overloads for use.
   */
  typedef void describe_type(std::ostream &, const std::string &, int);

  /** @brief Describe a configuration item
   * @param os Output stream
   * @param description Description
   * @param step Indent depth
   *
   * See ConfBase::write and overloads for use.
   */
  static void describe(std::ostream &os, const std::string &description,
                       int step);

  /** @brief No-op placeholder used instead of ConfBase::describe
   * @param os Output stream
   * @param description Description
   * @param step Indent depth
   *
   * See ConfBase::write and overloads for use.
   */
  static void nodescribe(std::ostream &os, const std::string &description,
                         int step);

  friend void test_quote();
  friend void test_quote_vector();
  friend void test_indent();
};

#endif /* CONFBASE_H */
