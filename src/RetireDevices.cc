#include <config.h>
#include "rsbackup.h"
#include "Conf.h"
#include "Command.h"
#include "Check.h"
#include "IO.h"
#include "Regexp.h"
#include <cerrno>
#include <cstring>

static void retireDevice(const std::string &deviceName) {
  config.readState();
  // If the device is still configured, check whether it should really be
  // retired
  Device *device = config.findDevice(deviceName);
  if(device) {
    if(!check("Really retire existing device '%s'?", deviceName.c_str()))
      return;
  }
  // Find all the logfiles for this device
  Directory d;
  Regexp r("^([0-9]+-[0-9]+-[0-9]+)-([^-]+)-([^-]+)-([^-]+)\\.log$"); // TODO share with Conf::readState?
  std::string f;
  std::vector<std::string> obsoleteLogs;
  d.open(config.logs);
  while(d.get(f)) {
    if(!r.matches(f))
      continue;
    if(r.sub(2) == deviceName)
      obsoleteLogs.push_back(f);
  }
  // Remove them
  if(command.verbose)
    printf("INFO: removing %zu logfiles for device '%s'\n",
           obsoleteLogs.size(), deviceName.c_str());
  for(size_t n = 0; n < obsoleteLogs.size(); ++n) {
    std::string path = config.logs + PATH_SEP + obsoleteLogs[n];
    if(command.act && unlink(path.c_str()) < 0) {
      fprintf(stderr, "ERROR: unlink %s: %s\n", path.c_str(), strerror(errno));
      ++errors;
    }
  }
}

void retireDevices() {
  for(size_t n = 0; n < command.devices.size(); ++n)
    retireDevice(command.devices[n]);
}
