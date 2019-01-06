// Copyright © 2011, 2012, 2014-17, 2019 Richard Kettlewell.
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
#include "rsbackup.h"
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
#include "Prune.h"
#include "ConfDirective.h"
#include "Device.h"
#include "Indent.h"
#include <cerrno>
#include <regex>
#include <sstream>
#include <boost/filesystem.hpp>

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
  d(os, "#  pre-access-hook COMMAND ...", step);
  if(preAccess.size())
    os << indent(step) << "pre-access-hook " << quote(preAccess) << '\n';
  d(os, "", step);

  d(os, "# Command to run after accessing backup devices", step);
  d(os, "#  pre-access-hook COMMAND ...", step);
  if(postAccess.size())
    os << indent(step) << "post-access-hook " << quote(postAccess) << '\n';
  d(os, "", step);

  d(os, "# Names of backup devices", step);
  d(os, "#  device NAME", step);
  for(auto &d: devices)
    os << "device " << quote(d.first) << '\n';
  d(os, "", step);

  d(os, "# ---- Reporting ----", step);
  d(os, "", step);

  d(os, "# 'Good' and 'bad' colors for HTML report", step);
  d(os, "#  color-good 0xRRGGBB", step);
  d(os, "#  color-bad 0xRRGGBB", step);
  os << indent(step) << "color-good 0x" << colorGood << '\n'
     << indent(step) << "color-bad 0x" << colorBad << '\n';
  d(os, "", step);

  if(reportPruneLogs) {
    d(os, "# How many days worth of pruning logs to report", step);
    d(os, "#  report-prune-logs DAYS", step);
    os << indent(step) << "report-prune-logs " << reportPruneLogs << '\n';
    d(os, "", step);
  }

  d(os, "# Path to mail transport agent", step);
  d(os, "#  sendmail PATH", step);
  os << indent(step) << "sendmail " << quote(sendmail) << '\n';
  d(os, "", step);

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
  d(os, "#   prune-logs[:DAYS] -- pruning logs (default 3 days)", step);
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
  os << indent(step) << "color-graph-background 0x" << colorGraphBackground << '\n';
  d(os, "", step);

  d(os, "# Graph foreground color", step);
  d(os, "#  color-graph-foreground 0xRRGGBB", step);
  os << indent(step) << "color-graph-foreground 0x" << colorGraphForeground << '\n';
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
  d(os, "#  device-color-strategy equidistant-value HUE SATURATION MINVALUE MAXVALUE", step);
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
  os << indent(step) << "backup-indicator-width " << backupIndicatorWidth << '\n';
  d(os, "", step);

  d(os, "# Minimum height of a backup indicator ", step);
  d(os, "#  backup-indicator-height PIXELS", step);
  os << indent(step) << "backup-indicator-height " << backupIndicatorHeight << '\n';
  d(os, "", step);

  d(os, "# Target width graph of graph", step);
  d(os, "#  graph-target-width PIXELS", step);
  os << indent(step) << "graph-target-width " << graphTargetWidth << '\n';
  d(os, "", step);

  d(os, "# Width of a backup indicator in the device key", step);
  d(os, "#  backup-indicator-key-width PIXELS", step);
  os << indent(step) << "backup-indicator-key-width " << backupIndicatorKeyWidth << '\n';
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
  int lineno = 0;
  while(input.readline(line)) {
    ++lineno;                           // keep track of where we are
    cc.path = path;
    cc.line = lineno;
    try {
      size_t indent;
      split(cc.bits, line, &indent);
      if(!cc.bits.size())                  // skip blank lines
        continue;
      // Consider all the possible commands
      const ConfDirective *d = ConfDirective::find(cc.bits[0]);
      if(d) {
        unsigned level = indenter.check(d->acceptable_levels, indent);
        switch(level) {
        case 0:
          throw SyntaxError("inconsistent indentation");
        case LEVEL_TOP:
          cc.context = this;
          cc.host = nullptr;
          cc.volume = nullptr;
          break;
        case LEVEL_HOST:
          cc.context = cc.host;
          cc.volume = nullptr;
          break;
        case LEVEL_VOLUME:
          cc.context = cc.volume;
          break;
        default:
          throw std::logic_error("unexpected indent level");
        }
        d->check(cc);
        d->set(cc);
        indenter.introduce(d->new_level);
      } else {
        throw SyntaxError("unknown command '" + cc.bits[0] + "'");
      }
    } catch(SyntaxError &e) {
      // Wrap up in a ConfigError, which carries the path/line information.
      std::stringstream s;
      s << path << ":" << lineno << ": " << e.what();
      throw ConfigError(s.str());
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
      if(!name.size()
         || name.at(0) == '.'
         || name.at(0) == '#'
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
    for(auto &v: h.second->volumes)
      validatePrunePolicy(v.second);
}

// (De-)select all hosts
void Conf::selectAll(bool sense) {
  for(auto &h: hosts)
    h.second->select(sense);
}

// (De-)select one host (or all if hostName="*")
void Conf::selectHost(const std::string &hostName, bool sense) {
  if(hostName == "*") {
    selectAll(sense);
  } else {
    auto hosts_iterator = hosts.find(hostName);
    if(hosts_iterator == hosts.end())
      throw CommandError("no such host as '" + hostName + "'");
    hosts_iterator->second->select(sense);
  }
}

// (De-)select one volume (or all if volumeName="*")
void Conf::selectVolume(const std::string &hostName,
                        const std::string &volumeName,
                        bool sense) {
  if(volumeName == "*") {
    selectHost(hostName, sense);
  } else {
    auto hosts_iterator = hosts.find(hostName);
    if(hosts_iterator == hosts.end())
      throw CommandError("no such host as '" + hostName + "'");
    Host *host = hosts_iterator->second;
    auto volumes_iterator = host->volumes.find(volumeName);
    if(volumes_iterator == host->volumes.end())
      throw CommandError("no such volume as '" + hostName
                         + ":" + volumeName + "'");
    volumes_iterator->second->select(sense);
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
  std::vector<std::string> files;
  const bool progress = (globalWarningMask & WARNING_VERBOSE) && isatty(2);
  std::vector<std::string> upgraded;

  std::string log;
  // Read database contents
  // Better would be to read only the rows required, on demand.
  {
    Database::Statement stmt(getdb(),
                             "SELECT host,volume,device,id,time,pruned,rc,status,log"
                             " FROM backup",
                             SQL_END);
    while(stmt.next()) {
      Backup backup;
      hostName = stmt.get_string(0);
      volumeName = stmt.get_string(1);
      backup.deviceName = stmt.get_string(2);
      backup.id = stmt.get_string(3);
      backup.time = stmt.get_int64(4);
      backup.date = Date(backup.time);
      backup.pruned = stmt.get_int64(5);
      backup.rc = stmt.get_int(6);
      backup.setStatus(stmt.get_int(7));
      backup.contents = stmt.get_blob(8);
      addBackup(backup, hostName, volumeName);
    }
  }

  // Upgrade old-format logfiles
  if(boost::filesystem::exists(logs)) {
    Directory::getFiles(logs, files);
    std::regex logfileRegexp("^([0-9]+-[0-9]+-[0-9]+)-([^-]+)-([^-]+)-([^-]+)\\.log$");
    for(size_t n = 0; n < files.size(); ++n) {
      if(progress)
        progressBar(IO::err, "Upgrading old logs", n, files.size());
      // Parse the filename
      std::smatch mr;
      if(!std::regex_match(files[n], mr, logfileRegexp))
        continue;
      Backup backup;
      backup.date = Date(mr[1]);
      backup.id = mr[1];
      backup.time = backup.date.toTime();
      backup.deviceName = mr[2];
      hostName = mr[3];
      volumeName = mr[4];

      // Read the log
      IO input;
      std::vector<std::string> contents;
      input.open(logs + "/" + files[n], "r");
      input.readlines(contents);
      // Skip empty files
      if(contents.size() == 0) {
        if(progress)
          progressBar(IO::err, nullptr, 0, 0);
        warning(WARNING_ALWAYS, "empty file: %s", files[n].c_str());
        continue;
      }
      // Find the status code
      const std::string &last = contents[contents.size() - 1];
      backup.rc = -1;
      if(last.compare(0, 3, "OK:") == 0)
        backup.rc = 0;
      else {
        std::string::size_type pos = last.rfind("error=");
        if(pos < std::string::npos)
          sscanf(last.c_str() + pos + 6, "%i", &backup.rc);
      }
      if(last.rfind("pruning") != std::string::npos)
        backup.setStatus(PRUNING);
      else if(backup.rc == 0)
        backup.setStatus(COMPLETE);
      else
        backup.setStatus(FAILED);
      for(std::string &c: contents) {
        backup.contents += c;
        backup.contents += "\n";
      }

      addBackup(backup, hostName, volumeName, true);

      if(globalCommand.act) {
        // addBackup might fail to set volume
        if(backup.volume != nullptr) {
          if(upgraded.size() == 0)
            getdb().begin();
          try {
            backup.insert(getdb());
          } catch(DatabaseError &e) {
            if(e.status != SQLITE_CONSTRAINT)
              throw;
          }
          upgraded.push_back(files[n]);
        } else {
          if(progress)
            progressBar(IO::err, nullptr, 0, 0);
          warning(WARNING_ALWAYS, "cannot upgrade %s", files[n].c_str());
        }
      }
    }
    logsRead = true;
    if(globalCommand.act && upgraded.size()) {
      getdb().commit();
      bool upgradeFailure = false;
      for(std::string &u: upgraded) {
        const std::string path = logs + "/" + u;
        if(unlink(path.c_str())) {
          error("removing %s: %s", path.c_str(), strerror(errno));
          upgradeFailure = true;
        }
      }
      if(upgradeFailure)
        throw SystemError("could not remove old logfiles");
    }
  }
  if(progress)
    progressBar(IO::err, nullptr, 0, 0);
}

void Conf::addBackup(Backup &backup,
                     const std::string &hostName,
                     const std::string &volumeName,
                     bool forceWarn) {
  const bool progress = (globalWarningMask & WARNING_VERBOSE) && isatty(2);
  unsigned warning_type = forceWarn ? WARNING_ALWAYS : WARNING_UNKNOWN;

  /* Don't keep pruned backups around */
  if(backup.getStatus() == PRUNED)
    return;

  if(!contains(devices, backup.deviceName)) {
    if(!contains(unknownDevices, backup.deviceName)) {
      if(progress)
        progressBar(IO::err, nullptr, 0, 0);
      warning(warning_type,
              "unknown device %s", backup.deviceName.c_str());
      unknownDevices.insert(backup.deviceName);
      ++globalConfig.unknownObjects;
    }
    return;
  }
  // Find the volume for this status record.  If it cannot be found, we warn
  // about it once.
  Host *host = findHost(hostName);
  if(!host) {
    if(!contains(unknownHosts, hostName)) {
      if(progress)
        progressBar(IO::err, nullptr, 0, 0);
      warning(warning_type, "unknown host %s", hostName.c_str());
      unknownHosts.insert(hostName);
      ++globalConfig.unknownObjects;
    }
    return;
  }
  Volume *volume = host->findVolume(volumeName);
  if(!volume) {
    if(!contains(host->unknownVolumes, volumeName)) {
      if(progress)
        progressBar(IO::err, nullptr, 0, 0);
      warning(warning_type, "unknown volume %s:%s",
              hostName.c_str(), volumeName.c_str());
      host->unknownVolumes.insert(volumeName);
      ++globalConfig.unknownObjects;
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
    if(globalCommand.act) {
      db = new Database(globalDatabase);
      if(!db->hasTable("backup"))
        createTables();
    } else {
      try {
        db = new Database(globalDatabase, false);
      } catch(DatabaseError &) {
        db = new Database(":memory:");
        createTables();
      }
    }
  }
  return *db;
}

void Conf::createTables() {
  db->begin();
  db->execute("CREATE TABLE backup (\n"
              "  host TEXT,\n"
              "  volume TEXT,\n"
              "  device TEXT,\n"
              "  id TEXT,\n"
              "  time INTEGER,\n"
              "  pruned INTEGER,\n"
              "  rc INTEGER,\n"
              "  status INTEGER,\n"
              "  log BLOB,\n"
              "  PRIMARY KEY (host,volume,device,id)\n"
              ")");
  db->commit();
}

ConfBase *Conf::getParent() const {
  return nullptr;
}

std::string Conf::what() const {
  return "system";
}

Conf globalConfig;
