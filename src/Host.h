// -*-C++-*-
// Copyright © 2011, 2012, 2014-2016 Richard Kettlewell.
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
#ifndef HOST_H
#define HOST_H
/** @file Host.h
 * @brief Configuration and state of a host
 */

#include "ConfBase.h"

/** @brief Type of map from volume names to volumes
 * @see Host::volumes
 */
typedef std::map<std::string, Volume *, namelt_type> volumes_type;

/** @brief Represents a host */
class Host: public ConfBase {
public:
  /** @brief Constructor
   * @param parent_ Parent configuration
   * @param name_ Name of host
   */
  Host(Conf *parent_, const std::string &name_):
      ConfBase(static_cast<ConfBase *>(parent_)), parent(parent_), name(name_),
      group(name_), hostname(name_) {
    parent->addHost(this);
  }

  /** @brief Destructor */
  virtual ~Host();

  /** @brief Parent configuration */
  Conf *parent;

  /** @brief Name of host */
  std::string name;

  /** @brief Host group name */
  std::string group;

  /** @brief Volumes for this host */
  volumes_type volumes;

  /** @brief Remote username */
  std::string user;

  /** @brief Remote hostname */
  std::string hostname;

  /** @brief Priority of this host */
  int priority = 0;

  /** @brief Unrecognized volume names found in logs
   *
   * Maps volume names to device names.
   *
   * Set by Conf::readState().
   */
  std::set<std::pair<std::string, std::string>> unknownVolumes;

  /** @brief Test whether host is selected
   * @return True if any volume for this host is selected
   */
  bool selected() const;

  /** @brief (De-)select all volumes
   * @param sense True to select all volumes, false to deselect
   */
  void select(bool sense);

  /** @brief Add a volume
   * @param v Pointer to new volume
   *
   * The volume name must not be in use.
   */
  void addVolume(Volume *v);

  /** @brief Find a volume by name
   * @param volumeName Name of volume to find
   * @return Volume or null pointer
   */
  Volume *findVolume(const std::string &volumeName) const;

  /** @brief SSH user+host string
   * @return String to pass to SSH client
   */
  std::string userAndHost() const;

  /** @brief SSH prefix
   * @return Either "" or "user@host:"
   */
  std::string sshPrefix() const;

  /** @brief Test if host available
   * @return true if host is available
   */
  bool available() const;

  /** @brief Test whether a host name is valid
   * @param n Host name
   * @return true if @p n is a valid host name
   */
  static bool valid(const std::string &n);

  /** @brief Invoke a command on the host and return its exit status
   * @param capture Where to put capture stdout, or null pointer
   * @param cmd Command to invoke
   * @param ... Arguments to command, terminatd by a null pointer
   * @return Exit status
   */
  int invoke(std::string *capture, const char *cmd, ...) const;

  ConfBase *getParent() const override;

  std::string what() const override;

  void write(std::ostream &os, int step, bool verbose) const override;
};

#endif /* HOST_H */
