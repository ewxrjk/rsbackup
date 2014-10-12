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
#ifndef CONF_H
#define CONF_H
/** @file Conf.h
 * @brief Program configuration */

#include <set>
#include <map>
#include <string>
#include <vector>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Defaults.h"
#include "Date.h"
#include "Regexp.h"

class Store;
class Device;
class Host;
class Volume;
class Database;
class Backup;

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
  ConfBase(): maxAge(DEFAULT_MAX_AGE),
              minBackups(DEFAULT_MIN_BACKUPS),
              pruneAge(DEFAULT_PRUNE_AGE),
              rsyncTimeout(0),
              hookTimeout(0),
              devicePattern("*") {}

  /** @brief Constructor that inherits from a parent
   * @param parent Parent container
   */
  ConfBase(ConfBase *parent): maxAge(parent->maxAge),
                              minBackups(parent->minBackups),
                              pruneAge(parent->pruneAge),
                              preBackup(parent->preBackup),
                              postBackup(parent->postBackup),
                              rsyncTimeout(parent->rsyncTimeout),
                              hookTimeout(parent->hookTimeout),
                              devicePattern(parent->devicePattern) {}

  virtual ~ConfBase();

  /** @brief Maximum comfortable age of most recent backup
   *
   * Corresponds to @c max-age. */
  int maxAge;

  /** @brief Minimum comfortable number of backups
   *
   * Corresponds to @c min-backups */
  int minBackups;

  /** @brief Age at which to prune backups
   *
   * Corresponds to @c prune-age */
  int pruneAge;

  /** @brief Pre-backup hook */
  std::vector<std::string> preBackup;

  /** @brief Post-backup hook */
  std::vector<std::string> postBackup;

  /** @brief rsync timeout */
  int rsyncTimeout;

  /** @brief hook timeout */
  int hookTimeout;

  /** @brief Device pattern to be used */
  std::string devicePattern;

  /** @brief Write this node to a stream
   * @param os Output stream
   * @param step Indent depth
   */
  virtual void write(std::ostream &os, int step = 0) const;

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

  friend void test_quote();
  friend void test_quote_vector();
  friend void test_indent();
};

/** @brief Write a configuration node to a stream
 * @param os Output stream
 * @param c Configuration node
 * @return @p os
 */
inline std::ostream &operator<<(std::ostream &os, const ConfBase &c) {
  c.write(os);
  return os;
}

/** @brief Type of map from host names to hosts */
typedef std::map<std::string,Host *> hosts_type;

/** @brief Type of map from store names to stores */
typedef std::map<std::string,Store *> stores_type;

/** @brief Type of map from device names to devices */
typedef std::map<std::string,Device *> devices_type;

/** @brief Represents the entire configuration of rsbackup. */
class Conf: public ConfBase {
public:
  /** @brief Construct an empty configuration */
  Conf(): maxUsage(DEFAULT_MAX_USAGE),
          maxFileUsage(DEFAULT_MAX_FILE_USAGE),
          publicStores(false),
          logs(DEFAULT_LOGS),
          sshTimeout(DEFAULT_SSH_TIMEOUT),
          keepPruneLogs(DEFAULT_KEEP_PRUNE_LOGS),
          sendmail(DEFAULT_SENDMAIL),
          colorGood(COLOR_GOOD),
          colorBad(COLOR_BAD),
          unknownObjects(0),
          logsRead(false),
          devicesIdentified(false),
          db(NULL) { }

  /** @brief Map of host names to configuration */
  hosts_type hosts;

  /** @brief Map of store names to configuration */
  stores_type stores;

  /** @brief Map of device names to configuration */
  devices_type devices;

  /** @brief Maximum device usage
   *
   * Corresponds to @c max-usage.  Currently not implemented.
   */
  int maxUsage;

  /** @brief Maximum file usage
   *
   * Corresponds to @c max-file-usage.  Currently not implemented.
   */
  int maxFileUsage;

  /** @brief Permit public stores */
  bool publicStores;

  /** @brief Log directory */
  std::string logs;

  /** @brief Lockfile path */
  std::string lock;

  /** @brief Timeout to pass to SSH */
  int sshTimeout;

  /** @brief Age to keep pruning logs */
  int keepPruneLogs;

  /** @brief Path to @c sendmail */
  std::string sendmail;

  /** @brief Pre-access hook */
  std::vector<std::string> preAccess;

  /** @brief Post-access hook */
  std::vector<std::string> postAccess;

  /** @brief Path to stylesheet for HTML report output
   *
   * If empty, a built-in one will be used.
   */
  std::string stylesheet;

  /** @brief RGB triple for 'good' color code */
  unsigned colorGood;

  /** @brief RGB triple for 'bad' color code */
  unsigned colorBad;

  /** @brief Read the master configuration file
   * @throws IOError if a file cannot be read
   * @throws ConfigError if the contents of a file are malformed
   */
  void read();

  /** @brief (De-)select one or more volumes
   * @param hostName Name of host containing volume to select
   * @param volumeName Name of volume to select or "*" for all
   * @param sense True to select, false to dselect
   */
  void selectVolume(const std::string &hostName,
                    const std::string &volumeName,
                    bool sense = true);

  /** @brief Add a host
   * @param h New host
   *
   * The host name must not be in use.
   */
  void addHost(Host *h);

  /** @brief Find a host by name
   * @param hostName Host to find
   * @return Host, or NULL
   */
  Host *findHost(const std::string &hostName) const;

  /** @brief Find a volume by name
   * @param hostName Name of host containing volume
   * @param volumeName Name of volume to find
   * @return Volume, or NULL
   */
  Volume *findVolume(const std::string &hostName,
                     const std::string &volumeName) const;

  /** @brief Find a device by name
   * @param deviceName Name of device to find
   * @return Device, or NULL
   */
  Device *findDevice(const std::string &deviceName) const;

  /** @brief Read logfiles
   *
   * Safe to call multiple times - the second and subsequent calls are
   * ignored. */
  void readState();

  /** @brief Identify devices
   * @param states Bitmap of store states to consider
   * 
   * Safe to call multiple times.
   */
  void identifyDevices(int states);

  /** @brief Unrecognized device names found in logs
   *
   * Set by readState().
   */
  std::set<std::string> unknownDevices;

  /** @brief Unrecognized host names found in logs
   *
   * Set by readState().
   */
  std::set<std::string> unknownHosts;

  /** @brief Total number of unknown objects
   *
   * Set by readState().
   */
  int unknownObjects;

  /** @brief Get the database access object
   * @return Pointer to database object
   *
   * Creates tables if they don't exist.
   */
  Database *getdb();

  /** @brief Regexp used to parse logfiles names */
  static Regexp logfileRegexp;

private:
  /** @brief Read a single configuration file
   * @param path Path to file to read
   * @throws IOError if @p path cannot be read
   * @throws ConfigError if the contents of @p path are malformed
   */
  void readOneFile(const std::string &path);

  /** @brief Read a configuration file or directory
   * @param path Path to file or directory to read
   * @throws IOError if a file cannot be read
   * @throws ConfigError if the contents of a file are malformed
   *
   * If @p path is a directory then the files in it are read (via @ref
   * readOneFile; there is no recursion).  Dotfiles and backup files (indicated
   * by a trailing "~") are skipped.
   *
   * Otherwise the behavior is the same as @ref readOneFile().
   */
  void includeFile(const std::string &path);

  /** @brief Split and parse a list represented as a string
   * @param bits Destination for components of the string
   * @param line String to parse
   * @throws SyntaxError if @p line is malformed.
   *
   * Each component can be quoted or unquoted.
   *
   * Unquoted components are delimited by whitespace and cannot contain double
   * quotes or backslashes.
   *
   * Quoted components are delimited by double quotes.  Within the quotes
   * backslash can be used to escape the next character.
   *
   * The hash character can appear inside quotes or noninitially in an unquoted
   * component, but otherwise introduces a comment which extends to the end of
   * @p line.
   */
  static void split(std::vector<std::string> &bits, const std::string &line);

  /** @brief Parse an integer
   * @param s Representation of integer
   * @param min Minimum acceptable value
   * @param max Maximum acceptable value
   * @param radix Base, or 0 to follow C conventions
   * @return Integer value
   * @throws SyntaxError if the @p s doesn't represent an integer
   * @throws SyntaxError if the integer value is out of range
   */
  static int parseInteger(const std::string &s,
                          int min = INT_MIN, int max = INT_MAX,
                          int radix = 10);

  /** @brief (De-)select all hosts
   * @param sense @c true to select all hosts, @c false to deselect them all
   */
  void selectAll(bool sense = true);

  /** @brief (De-)select a host
   * @param hostName Host to select or @c *
   * @param sense @c true to select hosts, @c false to deselect
   *
   * If @p hostName is @c * then all hosts are (de-)selected, as by @ref
   * selectAll().
   */
  void selectHost(const std::string &hostName,
                  bool sense = true);

  /** @brief Set to @c true when logfiles have been read
   * Set by @ref readState().
   */
  bool logsRead;

  /** @brief Set to @c true when devices have been identified
   * Set by @ref identifyDevices().
   */
  int devicesIdentified;

  /** @brief Database access object */
  Database *db;

  /** @brief Create database tables */
  void createTables();

  /** @brief Validate and add a backup to a volume
   * @param backup Populated backup
   * @param hostName Host owning @p backup
   * @param volumeName Volume owning @p backup
   *
   * Identifies the volume from the parameters, fills in @p Backup::volume, and
   * adds the backup to the volume's list of backups.  If the backup belongs to
   * an unknown device, host or volume, logs this but does not add it to
   * anything.
   */
  void addBackup(Backup &backup,
                 const std::string &hostName,
                 const std::string &volumeName);

  /** @brief Write this node to a stream
   * @param os Output stream
   * @param step Indent depth
   */
  virtual void write(std::ostream &os, int step = 0) const;
};

/** @brief Represents a backup device */
class Device {
public:
  /** @brief Constructor
   * @param name_ Name of device
   */
  Device(const std::string &name_): name(name_), store(NULL) {}

  /** @brief Name of device */
  std::string name;

  /** @brief Store for this device, or NULL
   *
   * Set by Store::identify().
   */
  Store *store;

  /** @brief Validity test for device names
   * @param n Name of device
   * @return true if @p n is a valid device name, else false
   */
  static bool valid(const std::string &n);
};

/** @brief Type of map from volume names to volumes */
typedef std::map<std::string,Volume *> volumes_type;

/** @brief Represents a host */
class Host: public ConfBase {
public:
  /** @brief Constructor
   * @param parent_ Parent configuration
   * @param name_ Name of host
   */
  Host(Conf *parent_, const std::string &name_):
    ConfBase(static_cast<ConfBase *>(parent_)),
    parent(parent_),
    name(name_),
    hostname(name_),
    alwaysUp(false) {
    parent->addHost(this);
  }

  /** @brief Parent configuration */
  Conf *parent;

  /** @brief Name of host */
  std::string name;

  /** @brief Volumes for this host */
  volumes_type volumes;

  /** @brief Remote username */
  std::string user;

  /** @brief Remote hostname */
  std::string hostname;

  /** @brief True if host is expected to always be up */
  bool alwaysUp;

  /** @brief Unrecognized volume names found in logs
   *
   * Set by Conf::readState().
   */
  std::set<std::string> unknownVolumes;

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
   * @return Volume or NULL
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
   * @param capture Where to put capture stdout, or NULL
   * @param cmd Command to invoke
   * @param ... Arguments to command, terminatd by a null pointer
   * @return Exit status
   */
  int invoke(std::string *capture, const char *cmd, ...) const;

private:
  /** @brief Write this node to a stream
   * @param os Output stream
   * @param step Indent depth
   */
  virtual void write(std::ostream &os, int step = 0) const;
};

/** @brief Represents the status of one backup */
class Backup {
public:
  /** @brief Wait status
   *
   * 0 means the backup succeeded.
   */
  int rc;

  /** @brief True if pruning has commenced */
  bool pruning;

  /** @brief Date of backup */
  Date date;

  /** @brief Id of backup */
  std::string id;

  /** @brief Time of backup */
  time_t time;

  /** @brief Device containing backup */
  std::string deviceName;

  /** @brief Log contents */
  std::string contents;

  /** @brief Volume backed up */
  Volume *volume;

  /** @brief Why this backup was pruned */
  std::string whyPruned;

  /** @brief Ordering on backups
   * @param that Other backup
   *
   * Backups are ordered by date first and by device name for backups of the
   * same date.
   */
  inline bool operator<(const Backup &that) const {
    int c;
    if((c = date - that.date)) return c < 0;
    if((c = deviceName.compare(that.deviceName))) return c < 0;
    return false;
  }

  /** @brief Return path to backup */
  std::string backupPath() const;

  /** @brief Return containing device
   *
   * TODO could this be NULL if device has been retired?
   */
  Device *getDevice() const;

  /** @brief Insert this backup into the database
   * @param db Database to update
   */
  void insert(Database *db) const;

  /** @brief Update this backup in the database
   * @param db Database to update
   */
  void update(Database *db) const;

  /** @brief Remove this backup from the database
   * @param db Database to update
   */
  void remove(Database *db) const;
};

/** @brief Comparison for backup pointers */
struct compare_backup {
  /** @brief Comparison operator
   * @param a A backup
   * @param b Another backup
   * @return true Ordering
   */
  bool operator()(Backup *a, Backup *b) const {
    return *a < *b;
  }
};

/** @brief Type of an ordered set of backups */
typedef std::set<Backup *,compare_backup> backups_type;

/** @brief Represents a single volume (usually, filesystem) to back up */
class Volume: public ConfBase {
public:
  /** @brief Construct a volume
   * @param parent_ Host containing volume
   * @param name_ Volume name
   * @param path_ Path to volume
   */
  Volume(Host *parent_,
         const std::string &name_,
         const std::string &path_): ConfBase(static_cast<ConfBase *>(parent_)),
                                    parent(parent_),
                                    name(name_),
                                    path(path_),
                                    traverse(false),
                                    checkMounted(false),
                                    completed(0),
                                    isSelected(false) {
    parent->addVolume(this);
  }

  /** @brief Host containing volume */
  Host *parent;

  /** @brief Volume name */
  std::string name;

  /** @brief Path to volume */
  std::string path;

  /** @brief List of exclusion patterns for this volume */
  std::vector<std::string> exclude;

  /** @brief Traverse mount points if true */
  bool traverse;

  /** @brief File to check before backing up */
  std::string checkFile;

  /** @brief Check that root path is a mount point before backing up */
  bool checkMounted;

  /** @brief Return true if volume is selected */
  bool selected() const { return isSelected; }

  /** @brief (De-)select volume
   * @param sense true to select, false to de-select
   */
  void select(bool sense);

  /** @brief Check whether a proposed volume name is valid
   * @param n Proposed volume name
   * @return true if valid, else false
   */
  static bool valid(const std::string &n);

  /** @brief Test if volume available
   * @return true if volume is available
   */
  bool available() const;

  /** @brief Known backups of this volume */
  backups_type backups;

  /** @brief Per-device information about this volume */
  struct PerDevice {
    /** @brief Number of backups of volume on device */
    int count;

    /** @brief Number of backups of volume to be removed from device */
    int toBeRemoved;

    /** @brief Oldest backup of volume on device */
    Date oldest;

    /** @brief Newest backup of volume on device */
    Date newest;

    /** @brief Construct per-device information */
    PerDevice(): count(0), toBeRemoved(0) {}
  };

  /** @brief Number of completed backups */
  int completed;

  /** @brief Date of oldest backup (on any device) */
  Date oldest;

  /** @brief Date of newest backup (on any device) */
  Date newest;

  /** @brief Type for @ref perDevice */
  typedef std::map<std::string,PerDevice> perdevice_type;

  /** @brief Map of device names to per-device information */
  perdevice_type perDevice;

  /** @brief Add a backup */
  void addBackup(Backup *backup);

  /** @brief Remove a backup */
  bool removeBackup(const Backup *backup);

  /** @brief Find the most recent backup
   * @param device If not NULL, only consider backups from this device
   * @return Most recent backup or NULL
   */
  const Backup *mostRecentBackup(const Device *device = NULL) const;

  /** @brief Find the most recent failedbackup
   * @param device If not NULL, only consider backups from this device
   * @return Most recent failed backup or NULL
   */
  const Backup *mostRecentFailedBackup(const Device *device = NULL) const;

private:
  /** @brief Set to @c true if this volume is selected */
  bool isSelected;

  /** @brief Recalculate statistics
   *
   * After calling this method the following members will accurately reflect
   * the contents of the @ref backups container:
   * - @ref completed
   * - @ref oldest
   * - @ref newest
   * - @ref perDevice
   *
   * @ref perDevice will not contain any entries with @ref PerDevice::count
   * equal to 0.
   */
  void calculate();

  /** @brief Write this node to a stream
   * @param os Output stream
   * @param step Indent depth
   */
  virtual void write(std::ostream &os, int step = 0) const;
};

inline Device *Backup::getDevice() const {
  return volume->parent->parent->findDevice(deviceName);
}

/** @brief Global configuration */
extern Conf config;

#endif /* CONF_H */
