// -*-C++-*-
// Copyright © Richard Kettlewell.
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
 * @brief Program configuration and state
 *
 * Configuration and state, which are not well-separated, are organized into a
 * tree structure.
 *
 * Nodes that take part in configuration inheritance derive from @ref ConfBase.
 * This class captures inheritable configuration and, in its constructor,
 * implements that inheritance from parent nodes.
 *
 * The root node has type @ref Conf.  As well as the inheritable configuration
 * this contains the global configuration.
 *
 * The children of the @ref Conf node are of type @ref Host (a host that may be
 * backed up), @ref Device (a physical backup device) and @ref Store (a mount
 * point at which a backup device may be found).  @ref Host participates in
 * configuration inheritance; the others do not.
 *
 * The children of the @ref Host nodes are all of type @ref Volume (a volume
 * within a host), which participates in configuration inheritance.  Finally
 * the children of the @ref Volume nodes are of type @ref Backup (a single
 * backup of some value on some device at a particular time).
 */

#include <set>
#include <map>
#include <string>
#include <vector>

#include "Defaults.h"
#include "Color.h"
#include "Location.h"
#include "ConfBase.h"
#include "Selection.h"

class Store;
class Device;
class Host;
class Volume;
class Database;
class Backup;

/** @brief Compare two strings as names
 * @param a First string
 * @param b Second string
 * @return true if @p a < @p b
 *
 * Comparison treats embedded numbers specially.
 */
bool namelt(const std::string &a, const std::string &b);

/** @brief Function type to allow @ref namelt to be used by STL containers */
struct namelt_type {
  /** @brief Compare two strings as names
   * @param a First string
   * @param b Second string
   * @return true if @p a < @p b
   */
  bool operator()(const std::string &a, const std::string &b) const {
    return namelt(a, b);
  }
};

/** @brief Type of map from host names to hosts
 *
 * @see Conf::hosts
 */
typedef std::map<std::string, Host *, namelt_type> hosts_type;

/** @brief Type of map from store names to stores
 *
 * @see Conf::stores
 */
typedef std::map<std::string, Store *, namelt_type> stores_type;

/** @brief Type of map from device names to devices
 *
 * @see Conf::devices
 **/
typedef std::map<std::string, Device *, namelt_type> devices_type;

/** @brief Root node of the entire configuration and state of rsbackup. */
class Conf: public ConfBase {
public:
  /** @brief Constructor */
  Conf();

  /** @brief Destructor */
  virtual ~Conf();

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

  /** @brief Database path */
  std::string database;

  /** @brief Lockfile path */
  std::string lock;

  /** @brief Age to keep pruning logs */
  int keepPruneLogs = DEFAULT_KEEP_PRUNE_LOGS;

  /** @brief Maximum time to spend pruning */
  int pruneTimeout = DEFAULT_PRUNE_TIMEOUT;

  /** @brief Path to @c sendmail */
  std::string sendmail = DEFAULT_SENDMAIL;

  /** @brief @c rm command (overridden for testing only) */
  std::string rm = DEFAULT_RM;

  /** @brief Pre-device-hook, run before a device is accessed */
  std::vector<std::string> preDevice;

  /** @brief Post-device-hook, run after a device is accessed */
  std::vector<std::string> postDevice;

  /** @brief Path to stylesheet for HTML report output
   *
   * If empty, a built-in one will be used.
   */
  std::string stylesheet;

  /** @brief 'good' color code */
  Color colorGood = COLOR_GOOD;

  /** @brief 'bad' color code */
  Color colorBad = COLOR_BAD;

  /** @brief Foregroud color of graph */
  Color colorGraphForeground = {0, 0, 0};

  /** @brief Background color of graph */
  Color colorGraphBackground = {1, 1, 1};

  /** @brief Color of vertical bars repsenting months in graph */
  Color colorMonthGuide = {0.96875, 0.96875, 0.96875};

  /** @brief Color of horizontal lines between hosts in graph */
  Color colorHostGuide = {0.875, 0.875, 0.875};

  /** @brief Color of horizontal lines between volumes in graph */
  Color colorVolumeGuide = {0.9375, 0.9375, 0.9375};

  /** @brief Horizontal padding in graph */
  double horizontalPadding = 8;

  /** @brief Vertical padding in graph */
  double verticalPadding = 2;

  /** @brief Backup indicator width for one day */
  double backupIndicatorWidth = 4;

  /** @brief Minimum backup indicator height */
  double backupIndicatorHeight = 2;

  /** @brief Target graph width */
  double graphTargetWidth = 0;

  /** @brief Backup indicator width in the device key */
  double backupIndicatorKeyWidth = 16;

  /** @brief Strategy for picking device colors */
  const ColorStrategy *deviceColorStrategy = nullptr;

  /** @brief Pango font description for host names */
  std::string hostNameFont = "Normal";

  /** @brief Pango font description for volume names */
  std::string volumeNameFont = "Normal";

  /** @brief Pango font description for device names */
  std::string deviceNameFont = "Normal";

  /** @brief Pango font description for time labels */
  std::string timeLabelFont = "Normal";

  /** @brief List of report sections */
  std::vector<std::string> report = {"title:Backup report (${RSBACKUP_DATE})",
                                     "h1:Backup report (${RSBACKUP_DATE})",
                                     "h2:Warnings?warnings",
                                     "warnings",
                                     "h2:Summary",
                                     "summary",
                                     "history-graph",
                                     "h2:Logfiles",
                                     "logs",
                                     "h3:Pruning logs",
                                     "prune-logs",
                                     "p:Generated ${RSBACKUP_CTIME}"};

  /** @brief Layout of graph */
  std::vector<std::string> graphLayout = {
      "host-labels:0,0", "volume-labels:1,0", "content:2,0",
      "time-labels:2,1", "device-key:2,3:RC",
  };

  /** @brief Read the master configuration file
   * @throws IOError if a file cannot be read
   * @throws ConfigError if the contents of a file are malformed
   */
  void read();

  /** @brief Validate a read configuration file */
  void validate() const;

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
   *
   * After reading the config file:
   * - Conf::devices lists the configured devices (essentially, drives), but
   * each Device::store pointer is null.
   * - Conf::stores lists the configured stores (essentially, mount points), but
   * each Store::device pointer is null.
   *
   * This function is responsible for filling in those pointers.
   *
   * All stores which have a device present will have Store::device set to point
   * to the device and the corresponding Device::store points back the to store.
   *
   * Normally it is called with Store::Enabled.
   * The only use case for calling with Store::Disabled is to issue warnings
   * about disabled stores.
   */
  void identifyDevices(int states);

  /** @brief Unrecognized device names found in logs
   *
   * Set by readState().
   */
  std::set<std::string> unknownDevices;

  /** @brief Unrecognized host names found in logs, mapped to devices that
   * mention them
   *
   * Set by readState().
   */
  std::set<std::pair<std::string, std::string>> unknownHosts;

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

  void write(std::ostream &os, int step, bool verbose) const override;

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
   * readOneFile; there is no recursion).  Dotfiles and backup files
   * (indicated by a trailing "~") are skipped.
   *
   * Otherwise the behavior is the same as @ref readOneFile().
   */
  void includeFile(const std::string &path);
  friend struct IncludeDirective;

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

  /** @brief Create database tables
   * @brief commitAnyway Commit even if @c -n option specified
   * @brief version Database version to create (for upgrade testing only)
   */
  void createTables(bool commitAnyway = false);

  /** @brief Maximum supported database version */
  static int supportedDatabaseVersion();

  /** @brief Figure out what database version is in use */
  int identifyDatabaseVersion();

  /** @brief Update database tables to latest version */
  void updateTables();

  /** @brief Validate and add a backup to a volume
   * @param backup Populated backup
   * @param hostName Host owning @p backup
   * @param volumeName Volume owning @p backup
   * @param forceWarn Force warnings on.
   *
   * Identifies the volume from the parameters, fills in @p Backup::volume,
   * and adds the backup to the volume's list of backups.  If the backup
   * belongs to an unknown device, host or volume, logs this but does not add
   * it to anything.
   */
  void addBackup(Backup &backup, const std::string &hostName,
                 const std::string &volumeName, bool forceWarn = false);
};

/** @brief Global configuration */
extern Conf globalConfig;

#endif /* CONF_H */
