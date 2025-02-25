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
#include <config.h>
#include <cerrno>
#include <regex>
#include <sstream>
#include <boost/filesystem.hpp>
#include <algorithm>
#include "rsbackup.h"
#include "Location.h"
#include "Conf.h"
#include "Backup.h"
#include "Volume.h"
#include "Host.h"
#include "Store.h"
#include "Errors.h"
#include "IO.h"
#include "Command.h"
#include "Utils.h"
#include "Database.h"
#include "PrunePolicy.h"
#include "ConfDirective.h"
#include "Device.h"
#include "Indent.h"
#include "BackupPolicy.h"

Conf::Conf() {
  std::vector<std::string> args;
  args.push_back("120");
  args.push_back("0.75");
  deviceColorStrategy = ColorStrategy::create(DEFAULT_COLOR_STRATEGY, args);
  hostCheck.push_back("ssh");
}

Conf::~Conf() {
  delete deviceColorStrategy;
  deviceColorStrategy = nullptr;
  delete db;
  db = nullptr;
  for(auto &d: devices)
    delete d.second;
  devices.clear();
  for(auto &s: stores)
    delete s.second;
  stores.clear();
  for(auto &h: hosts)
    delete h.second;
  hosts.clear();
}

void Conf::write(std::ostream &os, int step, bool verbose) const {
  describe_type *d = verbose ? describe : nodescribe;

  d(os, "# ---- Inheritable directives ----", step);
  d(os, "", step);

  ConfBase::write(os, step, verbose);
  writeHostCheck(os, step, verbose);

  d(os, "# ---- Non-inheritable directives ----", step);
  d(os, "", step);

  d(os, "# Whether stores are public or private (default)", step);
  d(os, "#  public true|false", step);
  os << indent(step) << "public " << (publicStores ? "true" : "false") << '\n';
  d(os, "", step);

  d(os, "# Path to log directory", step);
  d(os, "#  logs PATH", step);
  os << indent(step) << "logs " << quote(logs) << '\n';
  d(os, "", step);

  d(os, "# Path to database", step);
  d(os, "#  database PATH", step);
  if(database.size())
    os << indent(step) << "database " << quote(database) << '\n';
  d(os, "", step);

  d(os, "# Path to lock file", step);
  d(os, "#  lock PATH", step);
  if(lock.size())
    os << indent(step) << "lock " << quote(lock) << '\n';
  d(os, "", step);

  d(os, "# Command to run before accessing backup devices", step);
  d(os, "#  pre-device-hook COMMAND ...", step);
  if(preDevice.size())
    os << indent(step) << "pre-device-hook " << quote(preDevice) << '\n';
  d(os, "", step);

  d(os, "# Command to run after accessing backup devices", step);
  d(os, "#  pre-device-hook COMMAND ...", step);
  if(postDevice.size())
    os << indent(step) << "post-device-hook " << quote(postDevice) << '\n';
  d(os, "", step);

  d(os, "# Names of backup devices", step);
  d(os, "#  device NAME", step);
  for(auto &d: devices)
    os << "device " << quote(d.first) << '\n';
  d(os, "", step);

  d(os, "# The time period to keep records of pruned backups for", step);
  d(os, "#  keep-prune-logs INTERVAL", step);
  os << indent(step) << "keep-prune-logs " << formatTimeInterval(keepPruneLogs)
     << '\n';
  d(os, "", step);

  d(os, "# The maximum time to spend pruning", step);
  d(os, "#  prune-timeout INTERVAL", step);
  os << indent(step) << "prune-timeout " << formatTimeInterval(pruneTimeout)
     << '\n';
  d(os, "", step);

  d(os, "# ---- Reporting ----", step);
  d(os, "", step);

  d(os, "# 'Good' and 'bad' colors for HTML report", step);
  d(os, "#  color-good 0xRRGGBB", step);
  d(os, "#  color-bad 0xRRGGBB", step);
  os << indent(step) << "color-good 0x" << colorGood << '\n'
     << indent(step) << "color-bad 0x" << colorBad << '\n';
  d(os, "", step);

  d(os, "# Path to mail transport agent", step);
  d(os, "#  sendmail PATH", step);
  os << indent(step) << "sendmail " << quote(sendmail) << '\n';
  d(os, "", step);

  if(rm != DEFAULT_RM) {
    d(os, "# rm command", step);
    d(os, "#  rm COMMAND", step);
    os << indent(step) << "rm " << quote(rm) << '\n';
    d(os, "", step);
  }

  d(os, "# Stylesheet for HTML report", step);
  d(os, "#  stylesheet PATH", step);
  if(stylesheet.size())
    os << indent(step) << "stylesheet " << quote(stylesheet) << '\n';
  d(os, "", step);

  d(os, "# Contents of report", step);
  d(os, "#  report [+] KEY[:VALUE][?CONDITION]", step);
  d(os, "#", step);
  d(os, "# Keys:", step);
  d(os, "#   generated         -- generation time", step);
  d(os, "#   history-graph     -- graphical representation ofbackups", step);
  d(os, "#   h1:HEADING        -- level-1 heading", step);
  d(os, "#   h2:HEADING        -- level-2 heading", step);
  d(os, "#   h3:HEADING        -- level-3 heading", step);
  d(os, "#   logs              -- logs of failed backups", step);
  d(os, "#   p:TEXT            -- arbitrary text", step);
  d(os, "#   prune-logs[:INTERVAL] -- pruning logs (default 3 days)", step);
  d(os, "#   summary           -- summary table", step);
  d(os, "#   title:TITLE       -- report title", step);
  d(os, "#   warnings          -- warning messages", step);
  d(os, "#", step);
  d(os, "# Conditions:", step);
  d(os, "#   warnings          -- true if there are warnings to display", step);
  writeVector(os, step, "report", report);
  d(os, "", step);

  d(os, "# ---- Graphs ----", step);
  d(os, "", step);

  d(os, "# Graph background color", step);
  d(os, "#  color-graph-background 0xRRGGBB", step);
  os << indent(step) << "color-graph-background 0x" << colorGraphBackground
     << '\n';
  d(os, "", step);

  d(os, "# Graph foreground color", step);
  d(os, "#  color-graph-foreground 0xRRGGBB", step);
  os << indent(step) << "color-graph-foreground 0x" << colorGraphForeground
     << '\n';
  d(os, "", step);

  d(os, "# Graph month guide color", step);
  d(os, "#  color-month-guide 0xRRGGBB", step);
  os << indent(step) << "color-month-guide 0x" << colorMonthGuide << '\n';
  d(os, "", step);

  d(os, "# Graph host guide color", step);
  d(os, "#  color-host-guide 0xRRGGBB", step);
  os << indent(step) << "color-host-guide 0x" << colorHostGuide << '\n';
  d(os, "", step);

  d(os, "# Graph volume guide color", step);
  d(os, "#  color-volume-guide 0xRRGGBB", step);
  os << indent(step) << "color-volume-guide 0x" << colorVolumeGuide << '\n';
  d(os, "", step);

  d(os, "# Strategy for picking device colors", step);
  d(os, "#  device-color-strategy equidistant-value HUE", step);
  d(os, "#  device-color-strategy equidistant-value HUE SATURATION", step);
  d(os,
    "#  device-color-strategy equidistant-value HUE SATURATION MINVALUE "
    "MAXVALUE",
    step);
  d(os, "#  device-color-strategy equidistant-hue HUE", step);
  d(os, "#  device-color-strategy equidistant-hue HUE SATURATION VALUE", step);
  os << indent(step) << "device-color-strategy "
     << deviceColorStrategy->description() << '\n';
  d(os, "", step);

  d(os, "# Horizontal padding", step);
  d(os, "#  horizontal-padding PIXELS", step);
  os << indent(step) << "horizontal-padding " << horizontalPadding << '\n';
  d(os, "", step);

  d(os, "# Vertical padding", step);
  d(os, "#  vertical-padding PIXELS", step);
  os << indent(step) << "vertical-padding " << verticalPadding << '\n';
  d(os, "", step);

  d(os, "# Minimum width of a backup indicator", step);
  d(os, "#  backup-indicator-width PIXELS", step);
  os << indent(step) << "backup-indicator-width " << backupIndicatorWidth
     << '\n';
  d(os, "", step);

  d(os, "# Minimum height of a backup indicator ", step);
  d(os, "#  backup-indicator-height PIXELS", step);
  os << indent(step) << "backup-indicator-height " << backupIndicatorHeight
     << '\n';
  d(os, "", step);

  d(os, "# Target width graph of graph", step);
  d(os, "#  graph-target-width PIXELS", step);
  os << indent(step) << "graph-target-width " << graphTargetWidth << '\n';
  d(os, "", step);

  d(os, "# Width of a backup indicator in the device key", step);
  d(os, "#  backup-indicator-key-width PIXELS", step);
  os << indent(step) << "backup-indicator-key-width " << backupIndicatorKeyWidth
     << '\n';
  d(os, "", step);

  d(os, "# Font description for host names", step);
  d(os, "#  host-name-font FONT", step);
  os << indent(step) << "host-name-font " << hostNameFont << '\n';
  d(os, "", step);

  d(os, "# Font description for volume names", step);
  d(os, "#  volume-name-font FONT", step);
  os << indent(step) << "volume-name-font " << volumeNameFont << '\n';
  d(os, "", step);

  d(os, "# Font description for device names", step);
  d(os, "#  device-name-font FONT", step);
  os << indent(step) << "device-name-font " << deviceNameFont << '\n';
  d(os, "", step);

  d(os, "# Font description for time labels", step);
  d(os, "#  time-label-font FONT", step);
  os << indent(step) << "time-label-font " << timeLabelFont << '\n';
  d(os, "", step);

  d(os, "# Layout", step);
  d(os, "#  graph-layout [+] PART:COLUMN,ROW[:HV]", step);
  writeVector(os, step, "graph-layout", graphLayout);
  d(os, "", step);

  d(os, "# ---- Hosts to back up ----", step);

  for(auto &h: hosts) {
    os << '\n';
    h.second->write(os, step, verbose);
  }
}

// Read the master configuration file plus anything it includes.
void Conf::read() {
  readOneFile(globalConfigPath);
}

// Read one configuration file.  Throws IOError if some file cannot be
// read or ConfigError if the contents are bad.
void Conf::readOneFile(const std::string &path) {
  ConfContext cc(this);
  Indent indenter;

  IO input;
  D("Conf::readOneFile %s", path.c_str());
  input.open(path, "r");

  std::string line;
  cc.location.path = path;
  cc.location.line = 0;
  while(input.readline(line)) {
    ++cc.location.line; // keep track of where we are
    try {
      size_t indent;
      split(cc.bits, line, &indent);
      if(!cc.bits.size()) // skip blank lines
        continue;
      // Consider all the possible commands
      const ConfDirective *d = ConfDirective::find(cc.bits[0]);
      if(d) {
        unsigned level = indenter.check(d->acceptable_levels, indent);
        switch(level) {
        case 0: throw SyntaxError("inconsistent indentation");
        case LEVEL_TOP:
          cc.context = this;
          cc.host = nullptr;
          cc.volume = nullptr;
          break;
        case LEVEL_HOST:
          cc.context = cc.host;
          cc.volume = nullptr;
          break;
        case LEVEL_VOLUME: cc.context = cc.volume; break;
        default: throw std::logic_error("unexpected indent level");
        }
        d->check(cc);
        d->set(cc);
        indenter.introduce(d->new_level);
      } else {
        throw SyntaxError("unknown command '" + cc.bits[0] + "'");
      }
    } catch(SyntaxError &e) {
      // Wrap up in a ConfigError, which carries the path/line information.
      throw ConfigError(cc.location, e.what());
    }
  }
}

// Implementation of the 'include' command.  If PATH is a directory then
// includes all the regular files it contains (excluding dotfiles and backups
// but including symbolic links to regular files of any name), otherwise just
// tries to read it.
void Conf::includeFile(const std::string &path) {
  D("Conf::includeFile %s", path.c_str());
  if(boost::filesystem::is_directory(path)) {
    std::vector<std::string> files;
    Directory::getFiles(path, files);
    for(auto &name: files) {
      if(!name.size() || name.at(0) == '.' || name.at(0) == '#'
         || name.find('~') != std::string::npos)
        continue;
      std::string fullname = path + PATH_SEP + name;
      if(boost::filesystem::is_regular_file(fullname))
        readOneFile(fullname);
    }
  } else
    readOneFile(path);
}

void Conf::validate() const {
  for(auto &h: hosts)
    for(auto &v: h.second->volumes) {
      validateBackupPolicy(v.second);
      validatePrunePolicy(v.second);
    }
}

void Conf::addHost(Host *h) {
  hosts[h->name] = h;
}

// Find a host by name
Host *Conf::findHost(const std::string &hostName) const {
  auto it = hosts.find(hostName);
  return it != hosts.end() ? it->second : nullptr;
}

// Find a volume by name
Volume *Conf::findVolume(const std::string &hostName,
                         const std::string &volumeName) const {
  Host *host = findHost(hostName);
  return host ? host->findVolume(volumeName) : nullptr;
}

// Find a device by name
Device *Conf::findDevice(const std::string &deviceName) const {
  auto it = devices.find(deviceName);
  return it != devices.end() ? it->second : nullptr;
}

// Read in logfiles
void Conf::readState() {
  if(logsRead)
    return;
  std::string hostName, volumeName;
  const bool progress = (globalWarningMask & WARNING_VERBOSE) && isatty(2);

  // Read database contents
  // Better would be to read only the rows required, on demand.
  {
    const char *cmd = nullptr;
    Database &db = getdb();

    if(globalDatabaseVersion < 11)
      cmd = "SELECT host,volume,device,id,time,pruned,rc,status,log"
            " FROM backup";
    else
      cmd = "SELECT "
            "host,volume,device,id,time,pruned,rc,status,log,finishtime"
            " FROM backup";

    Database::Statement stmt(db, cmd, SQL_END);
    while(stmt.next()) {
      Backup backup;
      hostName = stmt.get_string(0);
      volumeName = stmt.get_string(1);
      backup.deviceName = stmt.get_string(2);
      backup.id = stmt.get_string(3);
      backup.time = stmt.get_int64(4);
      backup.pruned = stmt.get_int64(5);
      backup.waitStatus = stmt.get_int(6);
      backup.setStatus(stmt.get_int(7));
      backup.contents = stmt.get_blob(8);
      if(globalDatabaseVersion < 11)
        backup.finishTime = 0;
      else
        backup.finishTime = stmt.get_int64(9);
      addBackup(backup, hostName, volumeName);
    }
  }
  logsRead = true;
  if(progress)
    progressBar(IO::err, nullptr, 0, 0);
}

void Conf::addBackup(Backup &backup, const std::string &hostName,
                     const std::string &volumeName, bool forceWarn) {
  const bool progress = (globalWarningMask & WARNING_VERBOSE) && isatty(2);
  unsigned warning_type = forceWarn ? WARNING_ALWAYS : WARNING_UNKNOWN;

  /* Don't keep pruned backups around */
  if(backup.getStatus() == PRUNED)
    return;

  if(!contains(devices, backup.deviceName)) {
    if(!contains(unknownDevices, backup.deviceName)) {
      if(progress)
        progressBar(IO::err, nullptr, 0, 0);
      warning(warning_type, "unknown device %s", backup.deviceName.c_str());
      unknownDevices.insert(backup.deviceName);
      ++globalConfig.unknownObjects;
    }
    return;
  }
  // Find the volume for this status record.  If it cannot be found, we warn
  // about it once.
  Host *host = findHost(hostName);
  if(!host) {
    if(!contains(unknownHosts, std::pair<std::string, std::string>{
                                   hostName, backup.deviceName})) {
      if(progress)
        progressBar(IO::err, nullptr, 0, 0);
      warning(warning_type, "unknown host %s", hostName.c_str());
      ++globalConfig.unknownObjects;
      unknownHosts.insert({hostName, backup.deviceName});
    }
    return;
  }
  Volume *volume = host->findVolume(volumeName);
  if(!volume) {
    if(!contains(host->unknownVolumes, std::pair<std::string, std::string>{
                                           volumeName, backup.deviceName})) {
      if(progress)
        progressBar(IO::err, nullptr, 0, 0);
      warning(warning_type, "unknown volume %s:%s", hostName.c_str(),
              volumeName.c_str());
      ++globalConfig.unknownObjects;
      host->unknownVolumes.insert({volumeName, backup.deviceName});
    }
    return;
  }
  backup.volume = volume;
  // Attach the status record to the volume
  Backup *copy = new Backup(backup);
  bool inserted = volume->addBackup(copy);
  if(!inserted)
    delete copy;
}

// Create the mapping between stores and devices.
void Conf::identifyDevices(int states) {
  if((devicesIdentified & states) == states)
    return;
  int found = 0;
  std::vector<UnavailableStore> storeExceptions;
  for(auto &s: stores) {
    Store *store = s.second;
    if(!(store->state & states))
      continue;
    try {
      store->identify();
      ++found;
    } catch(UnavailableStore &unavailableStoreException) {
      warning(WARNING_STORE, "%s", unavailableStoreException.what());
      storeExceptions.push_back(unavailableStoreException);
    } catch(FatalStoreError &fatalStoreException) {
      if(states == Store::Enabled)
        throw;
      else
        warning(WARNING_STORE, "%s", fatalStoreException.what());
    } catch(BadStore &badStoreException) {
      if(states == Store::Enabled)
        error("%s", badStoreException.what());
    }
  }
  if(!found && states == Store::Enabled) {
    error("no backup devices found");
    if(!(globalWarningMask & WARNING_STORE))
      for(size_t n = 0; n < storeExceptions.size(); ++n)
        IO::err.writef("  %s\n", storeExceptions[n].what());
  }
  devicesIdentified |= states;
}

Database &Conf::getdb() {
  if(!db) {
    if(globalDatabase.size() == 0) {
      if(database.size() != 0)
        globalDatabase = database;
      else
        globalDatabase = logs + "/" DEFAULT_DATABASE;
    }
    if(globalCommand.act && globalCommand.readWriteActions()) {
      db = new Database(globalDatabase);
      if(!db->hasTable("backup"))
        createTables();
      else
        updateTables();
    } else {
      try {
        db = new Database(globalDatabase, false);
      } catch(DatabaseError &) {
        // If we cannot open the database even with -n then we
        // create a throwaway in-memory database so that we can
        // make some progress in showing what we'd do anyway.
        db = new Database(":memory:");
        createTables(true);
      }
    }
    // Set the database version based on whatever actually happened
    globalDatabaseVersion = identifyDatabaseVersion();
    if(globalDatabaseVersion < supportedDatabaseVersion())
      warning(WARNING_DATABASE,
              "obsolete database version: supported %d but found %d",
              supportedDatabaseVersion(), globalDatabaseVersion);
  }
  return *db;
}

/** @brief Columns for the backup table */
static const struct {
  const char *name;
  const char *type;
  int version;
} backup_columns[] = {
    // Present in all versions
    {"host", "TEXT", 0},
    {"volume", "TEXT", 0},
    {"device", "TEXT", 0},
    {"id", "TEXT", 0},
    {"time", "INTEGER", 0},
    {"pruned", "INTEGER", 0},
    {"rc", "INTEGER", 0},
    {"status", "INTEGER", 0},
    {"log", "BLOB", 0},
    // Added in 11.0
    {"finishTime", "INTEGER", 11},
};

void Conf::createTables(bool commitAnyway) {
  std::stringstream schema;

  schema << "CREATE TABLE backup (\n";
  for(const auto &bc: backup_columns)
    if(globalDatabaseVersion >= bc.version)
      schema << "  " << bc.name << " " << bc.type << ",\n";
  schema << "  PRIMARY KEY (host,volume,device,id)\n";
  schema << ")";

  db->begin();
  db->execute(schema.str());
  db->commit(commitAnyway);
}

int Conf::supportedDatabaseVersion() {
  int maximum_seen = -1; // highest version we saw in the table
  for(const auto &bc: backup_columns)
    maximum_seen = std::max(maximum_seen, bc.version);
  return maximum_seen;
}

int Conf::identifyDatabaseVersion() {
  // Find out what columns exist
  std::set<std::string> backup_current_columns;
  {
    Database::Statement stmt(
        *db, "SELECT name FROM pragma_table_info('backup');", SQL_END);
    while(stmt.next())
      backup_current_columns.insert(stmt.get_string(0));
  }
  int maximum_usable = supportedDatabaseVersion();
  for(const auto &bc: backup_columns) {
    if(backup_current_columns.find(bc.name) == backup_current_columns.end()) {
      // Column absent
      maximum_usable = std::min(maximum_usable, bc.version - 1);
    }
  }
  return maximum_usable;
}

void Conf::updateTables() {
  db->begin();

  // Find out what version we're on
  int currentVersion = identifyDatabaseVersion();
  // Add missing columns to get up to the latest
  for(const auto &bc: backup_columns) {
    if(bc.version > currentVersion) {
      char buffer[256];
      warning(WARNING_DATABASE, "upgrading database version: adding column %s",
              bc.name);
      snprintf(buffer, sizeof buffer, "ALTER TABLE backup ADD COLUMN %s %s;",
               bc.name, bc.type);
      db->execute(buffer);
    }
  }
  db->commit();
}

ConfBase *Conf::getParent() const {
  return nullptr;
}

std::string Conf::what() const {
  return "system";
}

Conf globalConfig;
