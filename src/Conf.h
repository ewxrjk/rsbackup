// -*-C++-*-
#ifndef CONF_H
#define CONF_H

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

class Store;
class Device;
class Host;
class Volume;

// Volume, Host and Conf share certain parameters, which are inherited from
// container to contained.  They are implemented in a ConfBase class.
class ConfBase {
public:
  ConfBase(): maxAge(DEFAULT_MAX_AGE),
              minBackups(DEFAULT_MIN_BACKUPS),
              pruneAge(DEFAULT_PRUNE_AGE) {}
  ConfBase(ConfBase *parent): maxAge(parent->maxAge),
                              minBackups(parent->minBackups),
                              pruneAge(parent->pruneAge) {}
  int maxAge;
  int minBackups;
  int pruneAge;
};

typedef std::map<std::string,Host *> hosts_type;
typedef std::map<std::string,Store *> stores_type;
typedef std::map<std::string,Device *> devices_type;

// Represents the entire configuration of rsbackup.
class Conf: public ConfBase {
public:
  Conf(): maxUsage(DEFAULT_MAX_USAGE),
          maxFileUsage(DEFAULT_MAX_FILE_USAGE),
          publicStores(false),
          logs(DEFAULT_LOGS),
          sshTimeout(DEFAULT_SSH_TIMEOUT),
          keepPruneLogs(DEFAULT_KEEP_PRUNE_LOGS),
          sendmail(DEFAULT_SENDMAIL),
          unknownObjects(0),
          logsRead(false),
          devicesIdentified(false) { }
  hosts_type hosts;
  stores_type stores;
  devices_type devices;
  int maxUsage;
  int maxFileUsage;
  bool publicStores;
  std::string logs;
  std::string lock;
  int sshTimeout;
  int keepPruneLogs;
  std::string sendmail;

  void read();

  void selectVolume(const std::string &hostName,
                    const std::string &volumeName,
                    bool sense = true);

  Host *findHost(const std::string &hostName) const;
  Volume *findVolume(const std::string &hostName,
                     const std::string &volumeName) const;
  Device *findDevice(const std::string &deviceName) const;

  // Read in logfiles
  void readState();

  // Identify devices
  void identifyDevices();

  // Unrecognized device names found in logs
  std::set<std::string> unknownDevices;
  // Unrecognized host names found in logs
  std::set<std::string> unknownHosts;
  // Total number of unknown objects of any kind
  int unknownObjects;

private:
  void readOneFile(const std::string &path);
  void includeFile(const std::string &path);
  static void split(std::vector<std::string> &bits, const std::string &line);
  static int parseInteger(const std::string &s,
                          int min = INT_MIN, int max = INT_MAX);
  void selectAll(bool sense = true);
  void selectHost(const std::string &hostName,
                  bool sense = true);

  bool logsRead;
  bool devicesIdentified;
};

// Represents a backup device.
class Device {
public:
  Device(const std::string &name_): name(name_), store(NULL) {}
  std::string name;
  Store *store;                         // store for this device, or NULL

  static bool valid(const std::string &);
};

typedef std::map<std::string,Volume *> volumes_type;

// Represents a host to back up; a container for volumes.
class Host: public ConfBase {
public:
  Host(Conf *parent_, const std::string &name_):
    ConfBase(static_cast<ConfBase *>(parent_)),
    parent(parent_),
    name(name_),
    hostname(name_) {}
  Conf *parent;
  std::string name;
  volumes_type volumes;
  std::string user;
  std::string hostname;

  // Unrecognized volume names found in logs
  std::set<std::string> unknownVolumes;

  bool selected() const;                // true if any volume selected
  void select(bool sense);              // (de-)select all volumes
  Volume *findVolume(const std::string &volumeName) const;

  std::string userAndHost() const;
  std::string sshPrefix() const;
  bool available() const;

  static bool valid(const std::string &);
};

// Represents the status of one backup
// TODO arguably misnamed
class Status {
public:
  int rc;                               // exit code; 0=OK
  Date date;                            // date of backup
  std::string deviceName;               // target device
  std::vector<std::string> contents;    // log contents
  Volume *volume;                       // owning volume

  inline bool operator<(const Status &that) const {
    int c;
    if((c = date - that.date)) return c < 0;
    if((c = deviceName.compare(that.deviceName))) return c < 0;
    return false;
  }

  std::string backupPath() const;
  std::string logPath() const;
};

typedef std::set<Status> backups_type;

// Represents a single volume (usually, filesystem) to back up.
class Volume: public ConfBase {
public:
  Volume(Host *parent_,
         const std::string &name_,
         const std::string &path_): ConfBase(static_cast<ConfBase *>(parent_)),
                                    parent(parent_),
                                    name(name_),
                                    path(path_),
                                    traverse(false),
                                    isSelected(false) {}
  Host *parent;
  std::string name;
  std::string path;
  std::vector<std::string> exclude;
  bool traverse;

  bool selected() const { return isSelected; }
  void select(bool sense);

  static bool valid(const std::string &);

  // Known backups
  backups_type backups;

  struct PerDevice {
    int count;
    int toBeRemoved;
    Date oldest, newest;
    PerDevice(): count(0), toBeRemoved(0) {}
  };

  int completed;                        // count of completed backups
  Date oldest, newest;                  // time bounds of backups
  typedef std::map<std::string,PerDevice> perdevice_type;
  perdevice_type perDevice;

private:
  friend void Conf::readState();

  bool isSelected;
  void calculate();
};

extern Conf config;

#endif /* CONF_H */
