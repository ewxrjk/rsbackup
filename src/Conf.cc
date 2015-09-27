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
#include "Prune.h"
#include <cctype>
#include <sstream>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <glob.h>
#include <iomanip>

/** @brief Context for configuration file parsing */
struct ConfContext {
  /** @brief Constructor
   *
   * @param conf_ Root configuration node
   */
  ConfContext(Conf *conf_):
    conf(conf_), context(conf_), host(NULL), volume(NULL) {}

  /** @brief Root of configuration */
  Conf *conf;

  /** @brief Current configuration node
   *
   * Could be a @ref Conf, @ref Host or @ref Volume.
   */
  ConfBase *context;

  /** @brief Current host or null */
  Host *host;

  /** @brief Current volume or null */
  Volume *volume;

  /** @brief Parsed directive */
  std::vector<std::string> bits;
};

struct Directive;

/** @brief Type of name-to-directive map */
typedef std::map<std::string, const Directive *> directives_type;

/** @brief Map names to directives */
static directives_type *directives;

/** @brief Base class for configuration file directives */
struct Directive {
  /** @brief Constructor
   *
   * @param name_ Name of directive
   * @param min_ Minimum number of arguments
   * @param max_ Maximum number of arguments
   *
   * Directives are automatically registered in this constructor.
   */
  Directive(const char *name_, int min_=0, int max_=INT_MAX):
    name(name_), min(min_), max(max_) {
    if(!directives)
      directives = new directives_type();
    assert((*directives).find(name) == (*directives).end());
    (*directives)[name] = this;
  }

  /** @brief Name of directive */
  const std::string name;

  /** @brief Minimum number of arguments */
  int min;

  /** @brief Maximum number of arguments */
  int max;

  /** @brief Check directive syntax
   * @param cc Context containing directive
   *
   * The base class implementation just checks the minimum and maximum number
   * of arguments.
   */
  virtual void check(const ConfContext &cc) const {
    int args = cc.bits.size() - 1;
    if(args < min)
      throw SyntaxError("too few arguments to '" + name + "'");
    if(args > max)
      throw SyntaxError("too many arguments to '" + name + "'");
  }

  /** @brief Act on a directive
   *  @param cc Context containing directive
   */
  virtual void set(ConfContext &cc) const = 0;
};

/** @brief Base class for directives that can only appear in a host context */
struct HostOnlyDirective: public Directive {
  /** @brief Constructor
   *
   * @param name_ Name of directive
   * @param min_ Minimum number of arguments
   * @param max_ Maximum number of arguments
   */
  HostOnlyDirective(const char *name_, int min_=0, int max_=INT_MAX):
    Directive(name_, min_, max_) {}
  virtual void check(const ConfContext &cc) const {
    if(cc.host == NULL)
      throw SyntaxError("'" + name + "' command without 'host'");
    Directive::check(cc);
  }
};

/** @brief Base class for directives that can only appear in a volume context */
struct VolumeOnlyDirective: public Directive {
  /** @brief Constructor
   *
   * @param name_ Name of directive
   * @param min_ Minimum number of arguments
   * @param max_ Maximum number of arguments
   */
  VolumeOnlyDirective(const char *name_, int min_=0, int max_=INT_MAX):
    Directive(name_, min_, max_) {}
  virtual void check(const ConfContext &cc) const {
    if(cc.volume == NULL)
      throw SyntaxError("'" + name + "' command without 'volume'");
    Directive::check(cc);
  }
};

// Global directives ----------------------------------------------------------

/** @brief The @c store directive */
static const struct StoreDirective: public Directive {
  StoreDirective(): Directive("store", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->stores[cc.bits[1]] = new Store(cc.bits[1]);
  }
} store_directive;

/** @brief The @c store-pattern directive */
static const struct StorePatternDirective: public Directive {
  StorePatternDirective(): Directive("store-pattern", 1, 1) {}
  void set(ConfContext &cc) const {
    std::vector<std::string> files;
    globFiles(files, cc.bits[1], GLOB_NOCHECK);
    for(size_t n = 0; n < files.size(); ++n)
      cc.conf->stores[files[n]] = new Store(files[n]);
  }
} store_pattern_directive;

/** @brief The @c stylesheet directive */
static const struct StyleSheetDirective: public Directive {
  StyleSheetDirective(): Directive("stylesheet", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->stylesheet = cc.bits[1];
  }
} stylesheet_directive;

/** @brief The @c colors directive */
static const struct ColorsDirective: public Directive {
  ColorsDirective(): Directive("colors", 2, 2) {}
  void set(ConfContext &cc) const {
    cc.conf->colorGood = parseInteger(cc.bits[1], 0, 0xFFFFFF, 0);
    cc.conf->colorBad = parseInteger(cc.bits[2], 0, 0xFFFFFF, 0);
  }
} colors_directive;

/** @brief The @c device directive */
static const struct DeviceDirective: public Directive {
  DeviceDirective(): Directive("device", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->devices[cc.bits[1]] = new Device(cc.bits[1]);
  }
} device_directive;

/** @brief The @c max-usage directive */
static const struct MaxUsageDirective: public Directive {
  MaxUsageDirective(): Directive("max-usage", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->maxUsage = parseInteger(cc.bits[1], 0, 100);
  }
} max_usage_directive;

/** @brief The @c max-file-usage directive */
static const struct MaxFileUsageDirective: public Directive {
  MaxFileUsageDirective(): Directive("max-file-usage", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->maxFileUsage = parseInteger(cc.bits[1], 0, 100);
  }
} max_file_usage_directive;

/** @brief The @c public directive */
static const struct PublicDirective: public Directive {
  PublicDirective(): Directive("public", 0, 0) {}
  void set(ConfContext &cc) const {
    cc.conf->publicStores = true;
  }
} public_directive;

/** @brief The @c logs directive */
static const struct LogsDirective: public Directive {
  LogsDirective(): Directive("logs", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->logs = cc.bits[1];
  }
} logs_directive;

/** @brief The @c lock directive */
static const struct LockDirective: public Directive {
  LockDirective(): Directive("lock", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->lock = cc.bits[1];
  }
} lock_directive;

/** @brief The @c sendmail directive */
static const struct SendmailDirective: public Directive {
  SendmailDirective(): Directive("sendmail", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->sendmail = cc.bits[1];
  }
} sendmail_directive;

/** @brief The @c pre-access-hook directive */
static const struct PreAccessHookDirective: public Directive {
  PreAccessHookDirective(): Directive("pre-access-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const {
    cc.conf->preAccess.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} pre_access_hook_directive;

/** @brief The @c post-access-hook directive */
static const struct PostAccessHookDirective: public Directive {
  PostAccessHookDirective(): Directive("post-access-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const {
    cc.conf->postAccess.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} post_access_hook_directive;

/** @brief The @c keep-prune-logs directive */
static const struct KeepPruneLogsDirective: public Directive {
  KeepPruneLogsDirective(): Directive("keep-prune-logs", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->keepPruneLogs = parseInteger(cc.bits[1], 1);
  }
} keep_prune_logs_directive;

/** @brief The @c report-prune-logs directive */
static const struct ReportPruneLogsDirective: public Directive {
  ReportPruneLogsDirective(): Directive("report-prune-logs", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->reportPruneLogs = parseInteger(cc.bits[1], 1);
  }
} report_prune_logs_directive;

/** @brief The @c include directive */
static const struct IncludeDirective: public Directive {
  IncludeDirective(): Directive("include", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.conf->includeFile(cc.bits[1]);
  }
} include_directive;

// Inheritable directives -----------------------------------------------------

/** @brief The @c max-age directive */
static const struct MaxAgeDirective: public Directive {
  MaxAgeDirective(): Directive("max-age", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.context->maxAge = parseInteger(cc.bits[1], 1);
  }
} max_age_directive;

/** @brief The @c min-backups directive */
static const struct MinBackupsDirective: public Directive {
  MinBackupsDirective(): Directive("min-backups", 1, 1) {}
  void set(ConfContext &cc) const {
    warning("the 'min-backups' directive is deprecated, use 'prune-parameter min-backups' instead");
    parseInteger(cc.bits[1], 1);
    cc.context->pruneParameters["min-backups"] = cc.bits[1];
  }
} min_backups_directive;

/** @brief The @c prune-age directive */
static const struct PruneAgeDirective: public Directive {
  PruneAgeDirective(): Directive("prune-age", 1, 1) {}
  void set(ConfContext &cc) const {
    warning("the 'prune-age' directive is deprecated, use 'prune-parameter prune-age' instead");
    parseInteger(cc.bits[1], 1);
    cc.context->pruneParameters["prune-age"] = cc.bits[1];
  }
} prune_age_directive;

/** @brief The @c prune-policy directive */
static const struct PrunePolicyDirective: public Directive {
  PrunePolicyDirective(): Directive("prune-policy", 1, 1) {}
  void set(ConfContext &cc) const {
    if(cc.bits[1].size() > 0 && cc.bits[1].at(0) == '/') {
      cc.context->prunePolicy = "exec";
      cc.context->pruneParameters["path"] = cc.bits[1];
    } else
      cc.context->prunePolicy = cc.bits[1];
  }
} prune_policy_directive;

/** @brief The @c prune-parameter directive */
static const struct PruneParameterDirective: public Directive {
  PruneParameterDirective(): Directive("prune-parameter", 2, 2) {}
  void set(ConfContext &cc) const {
    cc.context->pruneParameters[cc.bits[1]] = cc.bits[2];
  }
} prune_parameter_directive;

/** @brief The @c pre-backup-hook directive */
static const struct PreBackupHookDirective: public Directive {
  PreBackupHookDirective(): Directive("pre-backup-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const {
    cc.context->preBackup.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} pre_backup_hook_directive;

/** @brief The @c post-backup-hook directive */
static const struct PostBackupHookDirective: public Directive {
  PostBackupHookDirective(): Directive("post-backup-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const {
    cc.context->postBackup.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} post_backup_hook_directive;

/** @brief The @c rsync-timeout directive */
static const struct RsyncTimeoutDirective: public Directive {
  RsyncTimeoutDirective(): Directive("rsync-timeout", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.context->rsyncTimeout = parseInteger(cc.bits[1], 1);
  }
} rsync_timeout_directive;

/** @brief The @c hook-timeout directive */
static const struct HookTimeoutDirective: public Directive {
  HookTimeoutDirective(): Directive("hook-timeout", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.context->hookTimeout = parseInteger(cc.bits[1], 1);
  }
} hook_timeout_directive;

/** @brief The @c ssh-timeout directive */
static const struct SshTimeoutDirective: public Directive {
  SshTimeoutDirective(): Directive("ssh-timeout", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.context->sshTimeout = parseInteger(cc.bits[1], 1);
  }
} ssh_timeout_directive;

// Host directives ------------------------------------------------------------

/** @brief The @c host directive */
static const struct HostDirective: public Directive {
  HostDirective(): Directive("host", 1, 1) {}
  void set(ConfContext &cc) const {
    if(!Host::valid(cc.bits[1]))
      throw SyntaxError("invalid host name");
    if(cc.conf->hosts.find(cc.bits[1]) != cc.conf->hosts.end())
      throw SyntaxError("duplicate host");
    cc.context = cc.host = new Host(cc.conf, cc.bits[1]);
    cc.volume = NULL;
    cc.host->hostname = cc.bits[1];
  }
} host_directive;

/** @brief The @c hostname directive */
static const struct HostnameDirective: public HostOnlyDirective {
  HostnameDirective(): HostOnlyDirective("hostname", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.host->hostname = cc.bits[1];
  }
} hostname_directive;

/** @brief The @c always-up directive */
static const struct AlwaysUpDirective: public HostOnlyDirective {
  AlwaysUpDirective(): HostOnlyDirective("always-up", 0, 0) {}
  void set(ConfContext &cc) const {
    cc.host->alwaysUp = true;
  }
} always_up_directive;

/** @brief The @c priority directive */
static const struct PriorityDirective: public HostOnlyDirective {
  PriorityDirective(): HostOnlyDirective("priority", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.host->priority = parseInteger(cc.bits[1]);
  }
} priority_directive;

/** @brief The @c user directive */
static const struct UserDirective: public HostOnlyDirective {
  UserDirective(): HostOnlyDirective("user", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.host->user = cc.bits[1];
  }
} user_directive;

// Volume directives ----------------------------------------------------------

/** @brief The @c volume directive */
static const struct VolumeDirective: public HostOnlyDirective {
  VolumeDirective(): HostOnlyDirective("volume", 2, 2) {}
  void set(ConfContext &cc) const {
    if(!Volume::valid(cc.bits[1]))
      throw SyntaxError("invalid volume name");
    if(cc.host->volumes.find(cc.bits[1]) != cc.host->volumes.end())
      throw SyntaxError("duplicate volume");
    cc.context = cc.volume = new Volume(cc.host, cc.bits[1], cc.bits[2]);
  }
} volume_directive;

/** @brief The @c exclude directive */
static const struct ExcludeDirective: public VolumeOnlyDirective {
  ExcludeDirective(): VolumeOnlyDirective("exclude", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.volume->exclude.push_back(cc.bits[1]);
  }
} exclude_directive;

/** @brief The @c traverse directive */
static const struct TraverseDirective: public VolumeOnlyDirective {
  TraverseDirective(): VolumeOnlyDirective("traverse", 0, 0) {}
  void set(ConfContext &cc) const {
    cc.volume->traverse = true;
  }
} traverse_directive;

/** @brief The @c devices directive */
static const struct DevicesDirective: public VolumeOnlyDirective {
  DevicesDirective(): VolumeOnlyDirective("devices", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.volume->devicePattern = cc.bits[1];
  }
} devices_directive;

/** @brief The @c check-file directive */
static const struct CheckFileDirective: public VolumeOnlyDirective {
  CheckFileDirective(): VolumeOnlyDirective("check-file", 1, 1) {}
  void set(ConfContext &cc) const {
    cc.volume->checkFile = cc.bits[1];
  }
} check_file_directive;

/** @brief The @c check-mounted directive */
static const struct CheckMountedDirective: public VolumeOnlyDirective {
  CheckMountedDirective(): VolumeOnlyDirective("check-mounted", 0, 0) {}
  void set(ConfContext &cc) const {
    cc.volume->checkMounted = true;
  }
} check_mounted_directive;

void Conf::write(std::ostream &os, int step) const {
  ConfBase::write(os, step);
  if(publicStores)
    os << indent(step) << "public" << '\n';
  os << indent(step) << "logs " << quote(logs) << '\n';
  if(lock.size())
    os << indent(step) << "lock " << quote(lock) << '\n';
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
  ConfContext cc(this);

  IO input;
  D("Conf::readOneFile %s", path.c_str());
  input.open(path, "r");

  std::string line;
  int lineno = 0;
  while(input.readline(line)) {
    ++lineno;                           // keep track of where we are
    try {
      split(cc.bits, line);
      if(!cc.bits.size())                  // skip blank lines
        continue;
      // Consider all the possible commands
      directives_type::const_iterator it = (*directives).find(cc.bits[0]);
      if(it != (*directives).end()) {
        const Directive *d = it->second;
        d->check(cc);
        d->set(cc);
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

void Conf::validate() const {
  for(hosts_type::const_iterator hosts_iterator = hosts.begin();
      hosts_iterator != hosts.end();
      ++hosts_iterator) {
    Host *host = hosts_iterator->second;
    for(volumes_type::const_iterator volumes_iterator = host->volumes.begin();
        volumes_iterator != host->volumes.end();
        ++volumes_iterator) {
      validatePrunePolicy(volumes_iterator->second);
    }
  }
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
