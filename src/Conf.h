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
#ifndef CONF_H
#define CONF_H
/** @file Conf.h
 * @brief Program configuration */

#include <set>
#include <map>
#include <string>
#include <vector>
#include <climits>
#include <unistd.h>

#include "Defaults.h"
#include "Date.h"
#include "Color.h"

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
  ConfBase() = default;

  ConfBase(const ConfBase &) = delete;
  ConfBase& operator=(const ConfBase &) = delete;

  /** @brief Constructor that inherits from a parent
   * @param parent Parent container
   */
  ConfBase(ConfBase *parent): maxAge(parent->maxAge),
                              prunePolicy(parent->prunePolicy),
                              pruneParameters(parent->pruneParameters),
                              preBackup(parent->preBackup),
                              postBackup(parent->postBackup),
                              rsyncTimeout(parent->rsyncTimeout),
                              sshTimeout(parent->sshTimeout),
                              hookTimeout(parent->hookTimeout),
                              devicePattern(parent->devicePattern) {}

  virtual ~ConfBase() = default;

  /** @brief Maximum comfortable age of most recent backup
   *
   * Corresponds to @c max-age. */
  int maxAge = DEFAULT_MAX_AGE;

  /** @brief Name of pruning policy */
  std::string prunePolicy = DEFAULT_PRUNE_POLICY;

  /** @brief Pruning policy parameters */
  std::map<std::string, std::string> pruneParameters;

  /** @brief Pre-backup hook */
  std::vector<std::string> preBackup;

  /** @brief Post-backup hook */
  std::vector<std::string> postBackup;

  /** @brief rsync timeout */
  int rsyncTimeout = 0;

  /** @brief Timeout to pass to SSH */
  int sshTimeout = DEFAULT_SSH_TIMEOUT;

  /** @brief hook timeout */
  int hookTimeout = 0;

  /** @brief Device pattern to be used */
  std::string devicePattern = "*";

  /** @brief Write this node to a stream
   * @param os Output stream
   * @param step Indent depth
   * @param verbose Include informative annotations
   */
  virtual void write(std::ostream &os, int step, bool verbose)
    const;

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
  static void describe(std::ostream &os,
                       const std::string &description,
                       int step);

  /** @brief No-op placeholder used instead of ConfBase::describe
   * @param os Output stream
   * @param description Description
   * @param step Indent depth
   *
   * See ConfBase::write and overloads for use.
   */
  static void nodescribe(std::ostream &os,
                         const std::string &description,
                         int step);

  friend void test_quote();
  friend void test_quote_vector();
  friend void test_indent();
};

/** @brief Type of map from host names to hosts
 *
 * @see Conf::hosts
 */
typedef std::map<std::string, Host *> hosts_type;

/** @brief Type of map from store names to stores
 *
 * @see Conf::stores
 */
typedef std::map<std::string, Store *> stores_type;

/** @brief Type of map from device names to devices
 *
 * @see Conf::devices
 **/
typedef std::map<std::string, Device *> devices_type;

/** @brief Represents the entire configuration of rsbackup. */
class Conf: public ConfBase {
public:
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
  int maxUsage = DEFAULT_MAX_USAGE;

  /** @brief Maximum file usage
   *
   * Corresponds to @c max-file-usage.  Currently not implemented.
   */
  int maxFileUsage = DEFAULT_MAX_FILE_USAGE;

  /** @brief Permit public stores */
  bool publicStores = false;

  /** @brief Log directory */
  std::string logs = DEFAULT_LOGS;

  /** @brief Lockfile path */
  std::string lock;

  /** @brief Age to keep pruning logs */
  int keepPruneLogs = DEFAULT_KEEP_PRUNE_LOGS;

  /** @brief Age to report pruning logs */
  int reportPruneLogs = DEFAULT_PRUNE_REPORT_AGE;

  /** @brief Path to @c sendmail */
  std::string sendmail = DEFAULT_SENDMAIL;

  /** @brief Pre-access hook */
  std::vector<std::string> preAccess;

  /** @brief Post-access hook */
  std::vector<std::string> postAccess;

  /** @brief Path to stylesheet for HTML report output
   *
   * If empty, a built-in one will be used.
   */
  std::string stylesheet;

  /** @brief 'good' color code */
  Color colorGood = COLOR_GOOD;

  /** @brief 'bad' color code */
  Color colorBad = COLOR_BAD;

  /** @brief Read the master configuration file
   * @throws IOError if a file cannot be read
   * @throws ConfigError if the contents of a file are malformed
   */
  void read();

  /** @brief Validate a read configuration file */
  void validate() const;

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
   * @return Host, or null pointer
   */
  Host *findHost(const std::string &hostName) const;

  /** @brief Find a volume by name
   * @param hostName Name of host containing volume
   * @param volumeName Name of volume to find
   * @return Volume, or null pointer
   */
  Volume *findVolume(const std::string &hostName,
                     const std::string &volumeName) const;

  /** @brief Find a device by name
   * @param deviceName Name of device to find
   * @return Device, or null pointer
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
  int unknownObjects = 0;

  /** @brief Get the database access object
   * @return Reference to database object
   *
   * Creates tables if they don't exist.
   */
  Database &getdb();

  ConfBase *getParent() const override;

  std::string what() const override;

  void write(std::ostream &os, int step, bool verbose)
    const override;

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
  friend struct IncludeDirective;

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
  bool logsRead = false;

  /** @brief Set to @c true when devices have been identified
   * Set by @ref identifyDevices().
   */
  int devicesIdentified = false;

  /** @brief Database access object */
  Database *db = nullptr;

  /** @brief Create database tables */
  void createTables();

  /** @brief Validate and add a backup to a volume
   * @param backup Populated backup
   * @param hostName Host owning @p backup
   * @param volumeName Volume owning @p backup
   * @param forceWarn Force warnings on.
   *
   * Identifies the volume from the parameters, fills in @p Backup::volume, and
   * adds the backup to the volume's list of backups.  If the backup belongs to
   * an unknown device, host or volume, logs this but does not add it to
   * anything.
   */
  void addBackup(Backup &backup,
                 const std::string &hostName,
                 const std::string &volumeName,
                 bool forceWarn = false);
};

/** @brief Represents a backup device */
class Device {
public:
  /** @brief Constructor
   * @param name_ Name of device
   */
  Device(const std::string &name_): name(name_) {}

  /** @brief Name of device */
  std::string name;

  /** @brief Store for this device, or null pointer
   *
   * Set by Store::identify().
   */
  Store *store = nullptr;

  /** @brief Validity test for device names
   * @param n Name of device
   * @return true if @p n is a valid device name, else false
   */
  static bool valid(const std::string &n);
};

/** @brief Type of map from volume names to volumes
 * @see Host::volumes
 */
typedef std::map<std::string, Volume *> volumes_type;

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
    hostname(name_) {
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
  bool alwaysUp = false;

  /** @brief Priority of this host */
  int priority = 0;

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

  void write(std::ostream &os, int step, bool verbose)
    const override;
};

/** @brief Possible states of a backup */
enum BackupStatus {
  /** @brief %Backup status unknown */
  UNKNOWN = 0,

  /** @brief %Backup is underway */
  UNDERWAY = 1,

  /** @brief %Backup is complete */
  COMPLETE = 2,

  /** @brief %Backup failed */
  FAILED = 3,

  /** @brief Pruning is underway */
  PRUNING = 4,

  /** @brief Pruning is complete */
  PRUNED = 5
};

/** @brief Names of @ref BackupStatus constants */
extern const char *const backup_status_names[];

/** @brief Represents the status of one backup */
class Backup {
  /** @brief Status of this backup
   *
   * @see BackupStatus
   */
  int status = UNKNOWN;

public:
  /** @brief Wait status from @c rsync
   *
   * This is a wait status as returned by @c waitpid and similar functions.  0
   * means the backup succeeded.  It is meaningless if @ref Backup::status is
   * @ref UNKNOWN or @ref UNDERWAY.
   */
  int rc = 0;

  /** @brief Date of backup
   *
   * This reflects the day the backup was started.
   */
  Date date;

  /** @brief Id of backup
   *
   * In the current implementation these are days represented by YYYY-MM-DD
   * format.  However, IDs are treated as opaque strings, so this could be
   * changed.
   *
   * Note though that when importing logs from older versions of rsbackup (in
   * Conf::readState) the filename is assumed to start YYYY-MM-DD and this is
   * imported as both the time of the backup and its ID.
   */
  std::string id;

  /** @brief Time of backup
   *
   * This reflects the time that the backup was started.
   */
  time_t time = 0;

  /** @brief Time backup pruned
   *
   * The meaning of this member depends on the value of @ref Backup::status
   *
   * If the current status is @ref PRUNING then it reflects the time that it
   * was decided to prune the backup.
   *
   * If the current status is @ref PRUNED then it reflects the time that the
   * pruning operation completed.
   *
   * For any other status, the value is meaningless. */
  time_t pruned = 0;

  /** @brief Device containing backup */
  std::string deviceName;

  /** @brief Log contents */
  std::string contents;

  /** @brief Volume backed up */
  Volume *volume = nullptr;

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
   * @todo could this be null pointer if device has been retired?
   */
  Device *getDevice() const;

  /** @brief Insert this backup into the database
   * @param db Database to update
   * @param replace Replace existing row if present
   */
  void insert(Database &db,
              bool replace = false) const;

  /** @brief Update this backup in the database
   * @param db Database to update
   */
  void update(Database &db) const;

  /** @brief Remove this backup from the database
   * @param db Database to update
   */
  void remove(Database &db) const;

  /** @brief Retrieve status of this backup
   * @return Status (see @ref BackupStatus)
   */
  inline int getStatus() const {
    return status;
  }

  /** @brief Set the status of this backup
   * @param n New status (see @ref BackupStatus)
   *
   * Calls Volume::calculate if necessary. */
  void setStatus(int n);
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

/** @brief Type of an ordered set of backups
 * @see Volume::backups
 */
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
                                    path(path_) {
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
  bool traverse = false;

  /** @brief File to check before backing up */
  std::string checkFile;

  /** @brief Check that root path is a mount point before backing up */
  bool checkMounted = false;

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
    int count = 0;

    /** @brief Oldest backup of volume on device */
    Date oldest;

    /** @brief Newest backup of volume on device */
    Date newest;
  };

  /** @brief Number of completed backups */
  int completed = 0;

  /** @brief Date of oldest backup (on any device) */
  Date oldest;

  /** @brief Date of newest backup (on any device) */
  Date newest;

  /** @brief Type for @ref perDevice */
  typedef std::map<std::string, PerDevice> perdevice_type;

  /** @brief Map of device names to per-device information */
  perdevice_type perDevice;

  /** @brief Add a backup */
  void addBackup(Backup *backup);

  /** @brief Remove a backup */
  bool removeBackup(const Backup *backup);

  /** @brief Find the most recent backup
   * @param device If not null pointer, only consider backups from this device
   * @return Most recent backup or null pointer
   */
  const Backup *mostRecentBackup(const Device *device = nullptr) const;

  /** @brief Find the most recent failedbackup
   * @param device If not null pointer, only consider backups from this device
   * @return Most recent failed backup or null pointer
   */
  const Backup *mostRecentFailedBackup(const Device *device = nullptr) const;

  ConfBase *getParent() const override;

  std::string what() const override;

  void write(std::ostream &os, int step, bool verbose)
    const override;

private:
  /** @brief Set to @c true if this volume is selected */
  bool isSelected = false;

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

  friend void Backup::setStatus(int);
};

inline Device *Backup::getDevice() const {
  return volume->parent->parent->findDevice(deviceName);
}

/** @brief Global configuration */
extern Conf config;

#endif /* CONF_H */
