#include <config.h>
#include "Conf.h"
#include "Command.h"
#include "Check.h"
#include "Errors.h"
#include "IO.h"
#include "Retire.h"

static void retireVolume(const std::string &hostName,
                         const std::string &volumeName) {
  if(volumeName == "*") {
    if(config.findHost(hostName)) {
      if(!check("Really retire host '%s'?", 
                hostName.c_str()))
        return;
    }
  } else {
    if(config.findVolume(hostName, volumeName)) {
      if(!check("Really retire volume '%s:%s'?", 
                hostName.c_str(), volumeName.c_str()))
        return;
    }
  }
  // Accumulate a list of logs to remove.  Retirement looks at the directory
  // not the recorded state so that it can cope with the volume already being
  // deconfigured.
  Directory d;
  std::string f;
  std::vector<std::string> obsoleteLogs;
  d.open(config.logs);
  while(d.get(f)) {
    if(!Conf::logfileRegexp.matches(f))
      continue;
    if(Conf::logfileRegexp.sub(3) == hostName
       && (volumeName == "*" || Conf::logfileRegexp.sub(4) == volumeName))
      obsoleteLogs.push_back(f);
  }
  // Remove them
  removeObsoleteLogs(obsoleteLogs, true);
  // TODO we could remove the volume (and perhaps host) directories too, since
  // they are presumably empty.
}

void retireVolumes() {
  config.readState();
  for(size_t n = 0; n < command.selections.size(); ++n) {
    if(command.selections[n].sense == false)
      throw CommandError("cannot use negative selections with --retire");
  }
  for(size_t n = 0; n < command.selections.size(); ++n) {
    if(command.selections[n].host == "*")
      throw CommandError("cannot retire all hosts");
    retireVolume(command.selections[n].host, command.selections[n].volume);
  }
}
