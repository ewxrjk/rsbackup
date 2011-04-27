#include <config.h>
#include "Conf.h"
#include "Store.h"

// Return the path to this backup
std::string Backup::backupPath() const {
  const Host *host = volume->parent;
  const Device *device = host->parent->findDevice(deviceName);
  const Store *store = device->store;
  return (store->path
          + PATH_SEP + host->name
          + PATH_SEP + volume->name
          + PATH_SEP + date.toString());
}

// Return the path to the logfile for this backup
std::string Backup::logPath() const {
  const Host *host = volume->parent;
  const Device *device = host->parent->findDevice(deviceName);
  return (host->parent->logs
          + PATH_SEP
          + date.toString()
          + "-" + device->name
          + "-" + host->name
          + "-" + volume->name
          + ".log");
}
