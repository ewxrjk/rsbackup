#include <config.h>
#include "rsbackup.h"
#include "Conf.h"
#include "Command.h"
#include "Store.h"
#include "BulkRemove.h"
#include "Errors.h"
#include <cerrno>

static void removeObsoleteLog(const std::string &f,
                              bool removeBackup) {
  if(removeBackup) {
    if(!Conf::logfileRegexp.matches(f))
      return;                           // should never happen
    const std::string date = Conf::logfileRegexp.sub(1);
    const std::string deviceName = Conf::logfileRegexp.sub(2);
    const std::string hostName = Conf::logfileRegexp.sub(3);
    const std::string volumeName = Conf::logfileRegexp.sub(4);
    Device *device = config.findDevice(deviceName);
    if(!device) {
      // User should use --retire-device instead
      printf("ERROR: backup %s is on unknown device %s\n",
             f.c_str(), deviceName.c_str());
      ++errors;
      return;
    }
    if(!device->store) {
      printf("ERROR: backup %s is on unavailable device %s\n",
             f.c_str(), deviceName.c_str());
      ++errors;
      return;
    }
    const std::string backupPath = (device->store->path
                                    + PATH_SEP + hostName
                                    + PATH_SEP + volumeName
                                    + PATH_SEP + date);
    if(command.verbose)
      printf("INFO: removing %s\n", backupPath.c_str());
    if(command.act) {
      try {
        BulkRemove(backupPath);
      } catch(SubprocessFailed &exception) {
        printf("ERROR: removing %s: %s\n", 
               backupPath.c_str(), exception.what());
        ++errors;
        // Leave logfile in place for another go
        return;
      }
    }
  }
  const std::string path = config.logs + PATH_SEP + f;
  if(command.act && unlink(path.c_str()) < 0) {
    fprintf(stderr, "ERROR: removing %s: %s\n", path.c_str(), strerror(errno));
    ++errors;
  }
}

void removeObsoleteLogs(const std::vector<std::string> &obsoleteLogs,
                        bool removeBackup) {
  if(removeBackup)
    config.identifyDevices();
  for(size_t n = 0; n < obsoleteLogs.size(); ++n)
    removeObsoleteLog(obsoleteLogs[n], removeBackup);
}
