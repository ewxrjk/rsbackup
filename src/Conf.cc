#include <config.h>
#include "Conf.h"
#include "Store.h"
#include "Errors.h"
#include "IO.h"
#include "Regexp.h"
#include "Command.h"
#include <cctype>
#include <sstream>
#include <cerrno>
#include <cstring>
#include <cstdlib>

// Read the master configuration file plus anything it includes.
void Conf::read() {
  readOneFile(Command::configPath);
}

// Read one configuration file.  Throws IOError if some file cannot be
// read or ConfigError if the contents are bad.
void Conf::readOneFile(const std::string &path) {
  ConfBase *context = this;             // where to set max-age etc
  Host *host = NULL;                    // current host if any
  Volume *volume = NULL;                // current volume if any

  StdioFile input;
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
      } else if(bits[0] == "keep-prune-logs") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'keep-prune-logs'");
        keepPruneLogs = parseInteger(bits[1], 1);
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
        context = hosts[bits[1]] = host = new Host(this, bits[1]);
        volume = NULL;
      } else if(bits[0] == "hostname") {
        if(bits.size() != 2)
          throw SyntaxError("wrong number of arguments to 'hostname'");
        if(host == NULL)
          throw SyntaxError("'hostname' command without 'host'");
        host->hostname = bits[1];
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
        context = host->volumes[bits[1]] = volume =
          new Volume(host, bits[1], bits[2]);
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
    Directory d;
    d.open(path);
    std::string name;
    while(d.get(name)) {
      if(!name.size()
         || name.at(0) == '.'
         || name.find('~') != std::string::npos)
        continue;
      std::string fullname = path + PATH_SEP + name;
      if(stat(fullname.c_str(), &sb) >= 0 && S_ISREG(sb.st_mode))
        readOneFile(fullname);
    }
  } else
    readOneFile(path);
}

// Split a string into words.  Words are separated by whitespace but can also
// be quoted; inside quotes a backslash escapes any character.  A '#' where the
// start of a word would be introduces a comment which consumes the rest of the
// line.
void Conf::split(std::vector<std::string> &bits, const std::string &line) {
  bits.clear();
  std::string::size_type pos = 0;
  std::string s;
  while(pos < line.size()) {
    char c = line.at(pos);
    switch(c) {
    case ' ': case '\t': case '\r': case '\f':
      ++pos;
      break;
    case '#':
      return;
    case '"':
      s.clear();
      ++pos;
      while(pos < line.size() && line.at(pos) != '"') {
        if(line.at(pos) == '\\') {
          ++pos;
          if(pos > line.size())
            throw SyntaxError("unterminated string");
        }
        s += line.at(pos);
      }
      if(pos >= line.size())
        throw SyntaxError("unterminated string");
      ++pos;
      bits.push_back(s);
      break;
    case '\\':
      throw SyntaxError("unquoted backslash");
    default:
      s.clear();
      while(pos < line.size() && !isspace(line.at(pos))
            && line.at(pos) != '"' && line.at(pos) != '\\') {
        s += line.at(pos);
        ++pos;
      }
      bits.push_back(s);
      break;
    }      
  }
}

// Convert a string into an integer, throwing a SyntaxError if it is malformed
// our outside [min,max].
int Conf::parseInteger(const std::string &s,
                       int min, int max) {
  errno = 0;
  const char *sc = s.c_str();
  char *e;
  long n = strtol(sc, &e, 10);
  if(errno)
    throw SyntaxError("invalid integer '" + s + "': " + strerror(errno));
  if(*e || e == sc)
    throw SyntaxError("invalid integer '" + s + "'");
  if(n > max || n < min)
    throw SyntaxError("integer '" + s + "' out of range");
  return (int)n;
}

void Conf::selectAll(bool sense) {
  for(hosts_type::iterator it = hosts.begin();
      it != hosts.end();
      ++it)
    it->second->select(sense);
}

void Conf::selectHost(const std::string &hostName, bool sense) {
  if(hostName == "*") {
    selectAll(sense);
  } else {
    hosts_type::iterator hosts_iterator = hosts.find(hostName);
    if(hosts_iterator == config.hosts.end())
      throw std::runtime_error("no such host as '" + hostName + "'"); // TODO exception class
    hosts_iterator->second->select(sense);
  }
}

void Conf::selectVolume(const std::string &hostName,
                        const std::string &volumeName,
                        bool sense) {
  if(volumeName == "*") {
    selectHost(hostName, sense);
  } else {
    hosts_type::iterator hosts_iterator = hosts.find(hostName);
    if(hosts_iterator == hosts.end())
      throw std::runtime_error("no such host as '" + hostName + "'"); // TODO exception class
    Host *host = hosts_iterator->second;
    volumes_type::iterator volumes_iterator = host->volumes.find(volumeName);
    if(volumes_iterator == host->volumes.end())
      throw std::runtime_error("no such volume as '" + hostName // TODO exception class
                               + ":" + volumeName + "'");
    volumes_iterator->second->select(sense);
  }
}

Host *Conf::findHost(const std::string &hostName) const {
  hosts_type::const_iterator it = hosts.find(hostName);
  return it != hosts.end() ? it->second : NULL;
}

Volume *Conf::findVolume(const std::string &hostName,
                         const std::string &volumeName) const {
  Host *host = findHost(hostName);
  return host ? host->findVolume(volumeName) : NULL;
}

void Conf::readState() {
  Directory d;
  std::string f, hostName, volumeName;
  Status s;
  std::set<std::string> warnedHostNames, warnedVolumeNames;

  // Regexp for parsing the filename
  // Format is YYYY-MM-DD-DEVICE-HOST-VOLUME.log
  Regexp r("^([0-9]+-[0-9]+-[0-9]+)-([^-]+)-([^-]+)-([^-]+)\\.log$");

  d.open(logs);
  while(d.get(f)) {
    // Parse the filename
    if(!r.matches(f))
      continue;
    s.date = r.sub(1);
    s.deviceName = r.sub(2);
    hostName = r.sub(3);
    volumeName = r.sub(4);
    StdioFile input;
    input.open(logs + PATH_SEP + f, "r");
    input.readlines(s.contents);
    // Skip empty files
    if(s.contents.size() == 0)
      continue;
    // Find the status code
    const std::string &last = s.contents[s.contents.size() - 1];
    s.rc = -1;
    if(last.compare(0, 3, "OK:") == 0)
      s.rc = 0;
    else {
      std::string::size_type pos = last.rfind("error=");
      if(pos < std::string::npos)
        sscanf(last.c_str() + pos + 6, "%i", &s.rc);
    }
    // Find the volume for this status record.  If it cannot be found, we warn
    // about it once.
    Host *host = findHost(hostName);
    if(!host) {
      if(warnedHostNames.find(hostName) == warnedHostNames.end()) {
        fprintf(stderr, "WARNING: unknown host %s\n", hostName.c_str());
        warnedHostNames.insert(hostName);
      }
      continue;
    }
    Volume *volume = host->findVolume(volumeName);
    if(!volume) {
      std::string hostVolumeName = hostName + ":" + volumeName;
      if(warnedVolumeNames.find(hostVolumeName) == warnedVolumeNames.end()) {
        fprintf(stderr, "WARNING: unknown volume %s\n", hostVolumeName.c_str());
        warnedVolumeNames.insert(hostVolumeName);
      }
      continue;
    }
    // Attach the status record to the volume
    volume->backups.insert(s);
  }
}

Conf config;
