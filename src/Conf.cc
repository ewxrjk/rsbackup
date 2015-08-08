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
#include <config.h>
#include "rsbackup.h"
#include "Conf.h"
#include "Store.h"
#include "Errors.h"
#include "IO.h"
#include "Command.h"
#include "Utils.h"
#include "Database.h"
#include <cctype>
#include <sstream>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <glob.h>
#include <iomanip>

void Conf::write(std::ostream &os, int step) const {
  ConfBase::write(os, step);
  if(publicStores)
    os << indent(step) << "public" << '\n';
  os << indent(step) << "logs " << quote(logs) << '\n';
  if(lock.size())
    os << indent(step) << "lock " << quote(lock) << '\n';
  os << indent(step) << "ssh-timeout " << sshTimeout << '\n';
  os << indent(step) << "sendmail " << quote(sendmail) << '\n';
  if(preAccess.size())
    os << indent(step) << "pre-access-hook " << quote(preAccess) << '\n';
  if(postAccess.size())
    os << indent(step) << "post-access-hook " << quote(postAccess) << '\n';
  if(stylesheet.size())
    os << indent(step) << "stylesheet " << quote(stylesheet) << '\n';
  if(colorGood != COLOR_GOOD || colorBad != COLOR_BAD)
    os << indent(step) << "colors "
       << std::hex
       << "0x" << std::setw(6) << std::setfill('0') << colorGood
       << ' '
       << "0x" << std::setw(6) << std::setfill('0') << colorBad
       << '\n'
       << std::dec;
  for(devices_type::const_iterator it = devices.begin();
      it != devices.end();
      ++it)
    os << "device " << quote(it->first) << '\n';
  for(hosts_type::const_iterator it = hosts.begin();
      it != hosts.end();
      ++it) {
    os << '\n';
    static_cast<ConfBase *>(it->second)->write(os, step);
  }
}

// Read the master configuration file plus anything it includes.
void Conf::read() {
  readOneFile(command.configPath);
}

// Read one configuration file.  Throws IOError if some file cannot be
// read or ConfigError if the contents are bad.
void Conf::readOneFile(const std::string &path) {
  ConfBase *context = this;             // where to set max-age etc
  Host *host = NULL;                    // current host if any
  Volume *volume = NULL;                // current volume if any

  IO input;
  D("Conf::readOneFile %s", path.c_str());
  input.open(path, "r");

  std::string line;
  std::vector<std::string> bits;
  int lineno = 0;
  while(input.readline(line)) {
    ++lineno;                           // keep track of where we are
    try {
      split(bits, line);
      if(!bits.size())                  // skip blank lines
        continue;
      // Consider all the possible commands
      if(bits[0] == "store") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'store'");
        stores[bits[1]] = new Store(bits[1]);
      } else if(bits[0] == "store-pattern") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'store-pattern'");
        std::vector<std::string> files;
        globFiles(files, bits[1], GLOB_NOCHECK);
        for(size_t n = 0; n < files.size(); ++n)
          stores[files[n]] = new Store(files[n]);
      } else if(bits[0] == "stylesheet") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'stylesheet'");
        stylesheet = bits[1];
      } else if(bits[0] == "colors") {
        if(bits.size() != 3)
          throw SyntaxError("wrong number of arguments to 'colors'");
        colorGood = parseInteger(bits[1], 0, 0xFFFFFF, 0);
        colorBad = parseInteger(bits[2], 0, 0xFFFFFF, 0);
      } else if(bits[0] == "device") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'device'");
        if(!Device::valid(bits[1]))
          throw SyntaxError("invalid device name");
        devices[bits[1]] = new Device(bits[1]);
      } else if(bits[0] == "max-usage") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'max-usage'");
        maxUsage = parseInteger(bits[1], 0, 100);
      } else if(bits[0] == "max-file-usage") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'max-file-usage'");
        maxFileUsage = parseInteger(bits[1], 0, 100);
      } else if(bits[0] == "public") {
        if(bits.size() != 1)
          throw SyntaxError("wrong number of arguments to 'public'");
        publicStores = true;
      } else if(bits[0] == "logs") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'logs'");
        logs = bits[1];
      } else if(bits[0] == "lock") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'lock'");
        lock = bits[1];
      } else if(bits[0] == "sendmail") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'sendmail'");
        sendmail = bits[1];
      } else if(bits[0] == "pre-access-hook") {
        preAccess.assign(bits.begin() + 1, bits.end());
      } else if(bits[0] == "post-access-hook") {
        postAccess.assign(bits.begin() + 1, bits.end());
      } else if(bits[0] == "ssh-timeout") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'ssh-timeout'");
        sshTimeout = parseInteger(bits[1], 1);
      } else if(bits[0] == "max-age") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'max-age'");
        context->maxAge = parseInteger(bits[1], 1);
      } else if(bits[0] == "min-backups") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'min-backups'");
        context->minBackups = parseInteger(bits[1], 1);
      } else if(bits[0] == "prune-age") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'prune-age'");
        context->pruneAge = parseInteger(bits[1], 1);
      } else if(bits[0] == "pre-backup-hook") {
        context->preBackup.assign(bits.begin() + 1, bits.end());
      } else if(bits[0] == "post-backup-hook") {
        context->postBackup.assign(bits.begin() + 1, bits.end());
      } else if(bits[0] == "rsync-timeout") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'rsync-timeout'");
        rsyncTimeout = parseInteger(bits[1], 1);
      } else if(bits[0] == "hook-timeout") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'hook-timeout'");
        hookTimeout = parseInteger(bits[1], 1);
      } else if(bits[0] == "keep-prune-logs") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'keep-prune-logs'");
        keepPruneLogs = parseInteger(bits[1], 1);
      } else if(bits[0] == "report-prune-logs") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'report-prune-logs'");
        reportPruneLogs = parseInteger(bits[1], 1);
      } else if(bits[0] == "include") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'include'");
        includeFile(bits[1]);
      } else if(bits[0] == "host") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'host'");
        if(!Host::valid(bits[1]))
          throw SyntaxError("invalid host name");
        if(hosts.find(bits[1]) != hosts.end())
          throw SyntaxError("duplicate host");
        context = host = new Host(this, bits[1]);
        volume = NULL;
        host->hostname = bits[1];
      } else if(bits[0] == "hostname") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'hostname'");
        if(host == NULL)
          throw SyntaxError("'hostname' command without 'host'");
        host->hostname = bits[1];
      } else if(bits[0] == "always-up") {
        if(bits.size() != 1)
          throw SyntaxError("wrong number of arguments to 'always-up'");
        if(host == NULL)
          throw SyntaxError("'always-up' command without 'host'");
        host->alwaysUp = true;
      } else if(bits[0] == "priority") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'priority'");
        if(host == NULL)
          throw SyntaxError("'always-up' command without 'priority'");
        host->priority = parseInteger(bits[1]);
      } else if(bits[0] == "user") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'user'");
        if(host == NULL)
          throw SyntaxError("'user' command without 'host'");
        host->user = bits[1];
      } else if(bits[0] == "volume") {
        if(bits.size() != 3)
          throw SyntaxError("wrong number of arguments to 'volume'");
        if(!Volume::valid(bits[1]))
          throw SyntaxError("invalid volume name");
        if(!host)
          throw SyntaxError("'volume' command without 'host'");
        if(host->volumes.find(bits[1]) != host->volumes.end())
          throw SyntaxError("duplicate volume");
        context = volume = new Volume(host, bits[1], bits[2]);
      } else if(bits[0] == "exclude") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'exclude'");
        if(volume == NULL)
          throw SyntaxError("'exclude' command without 'volume'");
        volume->exclude.push_back(bits[1]);
      } else if(bits[0] == "traverse") {
        if(bits.size() != 1)
          throw SyntaxError("wrong number of arguments to 'traverse'");
        if(volume == NULL)
          throw SyntaxError("'traverse' command without 'volume'");
        volume->traverse = true;
      } else if(bits[0] == "devices") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'devices'");
        if(host == NULL)
          throw SyntaxError("'devices' command without 'volume'");
        context->devicePattern = bits[1];
      } else if(bits[0] == "check-file") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'check-file'");
        if(volume == NULL)
          throw SyntaxError("'check-file' command without 'volume'");
        volume->checkFile = bits[1];
      } else if(bits[0] == "check-mounted") {
        if(bits.size() != 1)
          throw SyntaxError("wrong number of arguments to 'check-mounted'");
        if(volume == NULL)
          throw SyntaxError("'check-mounted' command without 'volume'");
        volume->checkMounted = true;
      } else {
        throw SyntaxError("unknown command '" + bits[0] + "'");
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
  struct stat sb;

  D("Conf::includeFile %s", path.c_str());
  if(stat(path.c_str(), &sb) >= 0 && S_ISDIR(sb.st_mode)) {
    std::vector<std::string> files;
    Directory::getFiles(path, files);
    for(size_t n = 0; n < files.size(); ++n) {
      const std::string name = files.at(n);
      if(!name.size()
         || name.at(0) == '.'
         || name.at(0) == '#'
         || name.find('~') != std::string::npos)
        continue;
      std::string fullname = path + PATH_SEP + name;
      if(stat(fullname.c_str(), &sb) >= 0 && S_ISREG(sb.st_mode))
        readOneFile(fullname);
    }
  } else
    readOneFile(path);
}

// (De-)select all hosts
void Conf::selectAll(bool sense) {
  for(hosts_type::iterator it = hosts.begin();
      it != hosts.end();
      ++it)
    it->second->select(sense);
}

// (De-)select one host (or all if hostName="*")
void Conf::selectHost(const std::string &hostName, bool sense) {
  if(hostName == "*") {
    selectAll(sense);
  } else {
    hosts_type::iterator hosts_iterator = hosts.find(hostName);
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
    hosts_type::iterator hosts_iterator = hosts.find(hostName);
    if(hosts_iterator == hosts.end())
      throw CommandError("no such host as '" + hostName + "'");
    Host *host = hosts_iterator->second;
    volumes_type::iterator volumes_iterator = host->volumes.find(volumeName);
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
  hosts_type::const_iterator it = hosts.find(hostName);
  return it != hosts.end() ? it->second : NULL;
}

// Find a volume by name
Volume *Conf::findVolume(const std::string &hostName,
                         const std::string &volumeName) const {
  Host *host = findHost(hostName);
  return host ? host->findVolume(volumeName) : NULL;
}

// Find a device by name
Device *Conf::findDevice(const std::string &deviceName) const {
  devices_type::const_iterator it = devices.find(deviceName);
  return it != devices.end() ? it->second : NULL;
}

// Read in logfiles
void Conf::readState() {
  if(logsRead)
    return;
  std::string hostName, volumeName;
  std::vector<std::string> files;
  const bool progress = command.verbose && isatty(2);
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
  Directory::getFiles(logs, files);
  for(size_t n = 0; n < files.size(); ++n) {
    if(progress)
      progressBar(IO::err, "Upgrading old logs", n, files.size());
    // Parse the filename
    if(!logfileRegexp.matches(files[n]))
      continue;
    Backup backup;
    backup.date = logfileRegexp.sub(1);
    backup.id = logfileRegexp.sub(1);
    backup.time = backup.date.toTime();
    backup.deviceName = logfileRegexp.sub(2);
    hostName = logfileRegexp.sub(3);
    volumeName = logfileRegexp.sub(4);

    // Read the log
    IO input;
    std::vector<std::string> contents;
    input.open(logs + "/" + files[n], "r");
    input.readlines(contents);
    // Skip empty files
    if(contents.size() == 0) {
      if(progress)
        progressBar(IO::err, NULL, 0, 0);
      warning("empty file: %s", files[n].c_str());
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
    for(std::vector<std::string>::const_iterator it = contents.begin();
        it != contents.end();
        ++it) {
      backup.contents += *it;
      backup.contents += "\n";
    }

    addBackup(backup, hostName, volumeName, true);

    if(command.act) {
      // addBackup might fail to set volume
      if(backup.volume != NULL) {
        if(upgraded.size() == 0)
          getdb()->begin();
        try {
          backup.insert(getdb());
        } catch(DatabaseError &e) {
          if(e.status != SQLITE_CONSTRAINT)
            throw;
        }
        upgraded.push_back(files[n]);
      } else {
        if(progress)
          progressBar(IO::err, NULL, 0, 0);
        warning("cannot upgrade %s", files[n].c_str());
      }
    }
  }
  logsRead = true;
  if(command.act && upgraded.size()) {
    getdb()->commit();
    bool upgradeFailure = false;
    for(std::vector<std::string>::const_iterator it = upgraded.begin();
        it != upgraded.end(); ++it) {
      const std::string path = logs + "/" + *it;
      if(unlink(path.c_str())) {
        error("removing %s: %s", path.c_str(), strerror(errno));
        upgradeFailure = true;
      }
    }
    if(upgradeFailure)
      throw SystemError("could not remove old logfiles");
  }
  if(progress)
    progressBar(IO::err, NULL, 0, 0);
}

void Conf::addBackup(Backup &backup,
                     const std::string &hostName,
                     const std::string &volumeName,
                     bool forceWarn) {
  const bool progress = command.verbose && isatty(2);

  /* Don't keep pruned backups around */
  if(backup.getStatus() == PRUNED)
    return;

  if(devices.find(backup.deviceName) == devices.end()) {
    if(unknownDevices.find(backup.deviceName) == unknownDevices.end()) {
      if(command.warnUnknown || forceWarn) {
        if(progress)
          progressBar(IO::err, NULL, 0, 0);
        warning("unknown device %s", backup.deviceName.c_str());
      }
      unknownDevices.insert(backup.deviceName);
      ++config.unknownObjects;
    }
    return;
  }
  // Find the volume for this status record.  If it cannot be found, we warn
  // about it once.
  Host *host = findHost(hostName);
  if(!host) {
    if(unknownHosts.find(hostName) == unknownHosts.end()) {
      if(command.warnUnknown || forceWarn) {
        if(progress)
          progressBar(IO::err, NULL, 0, 0);
        warning("unknown host %s", hostName.c_str());
      }
      unknownHosts.insert(hostName);
      ++config.unknownObjects;
    }
    return;
  }
  Volume *volume = host->findVolume(volumeName);
  if(!volume) {
    if(host->unknownVolumes.find(volumeName) == host->unknownVolumes.end()) {
      if(command.warnUnknown || forceWarn) {
        if(progress)
          progressBar(IO::err, NULL, 0, 0);
        warning("unknown volume %s:%s",
                hostName.c_str(), volumeName.c_str());
      }
      host->unknownVolumes.insert(volumeName);
      ++config.unknownObjects;
    }
    return;
  }
  backup.volume = volume;
  // Attach the status record to the volume
  volume->addBackup(new Backup(backup));
}

// Create the mapping between stores and devices.
void Conf::identifyDevices(int states) {
  if((devicesIdentified & states) == states)
    return;
  int found = 0;
  std::vector<UnavailableStore> storeExceptions;
  for(stores_type::iterator storesIterator = stores.begin();
      storesIterator != stores.end();
      ++storesIterator) {
    Store *store = storesIterator->second;
    if(!(store->state & states))
      continue;
    try {
      store->identify();
      ++found;
    } catch(UnavailableStore &unavailableStoreException) {
      if(command.warnStore)
        warning("%s", unavailableStoreException.what());
      storeExceptions.push_back(unavailableStoreException);
    } catch(FatalStoreError &fatalStoreException) {
      if(states == Store::Enabled)
        throw;
      else if(command.warnStore)
        warning("%s", fatalStoreException.what());
    } catch(BadStore &badStoreException) {
      if(states == Store::Enabled)
        error("%s", badStoreException.what());
    }
  }
  if(!found && states == Store::Enabled) {
    error("no backup devices found");
    if(!command.warnStore)
      for(size_t n = 0; n < storeExceptions.size(); ++n)
        IO::err.writef("  %s\n", storeExceptions[n].what());
  }
  devicesIdentified |= states;
}

Database *Conf::getdb() {
  if(!db) {
    if(command.database.size() == 0)
      command.database = logs + "/backups.db";
    if(command.act) {
      db = new Database(command.database);
      if(!db->hasTable("backup"))
        createTables();
    } else {
      try {
        db = new Database(command.database, false);
      } catch(DatabaseError &) {
        db = new Database(":memory:");
        createTables();
      }
    }
  }
  return db;
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

// Regexp for parsing log filenames
// Format is YYYY-MM-DD-DEVICE-HOST-VOLUME.log
// Captures are: 1 date
//               2 device
//               3 host
//               4 volume
Regexp Conf::logfileRegexp("^([0-9]+-[0-9]+-[0-9]+)-([^-]+)-([^-]+)-([^-]+)\\.log$");

Conf config;
