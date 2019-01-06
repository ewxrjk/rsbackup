// Copyright © 2012, 2015-17, 2019 Richard Kettlewell.
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
#include "DeviceAccess.h"
#include "Conf.h"
#include "Subprocess.h"
#include "Errors.h"
#include "Command.h"
#include "IO.h"
#include "Utils.h"

static bool devicesReady;
static std::vector<IO *> filesToClose;

static void runDeviceAccessHook(const std::vector<std::string> &cmd,
                                const std::string &name) {
  if(cmd.size() == 0)
    return;
  Subprocess sp("device-access", cmd);
  sp.setenv("RSBACKUP_HOOK", name);
  sp.setenv("RSBACKUP_ACT", globalCommand.act ? "true" : "false");
  std::string devices;
  for(auto &d: globalConfig.devices) {
    if(devices.size() != 0)
      devices += " ";
    devices += d.first;
  }
  sp.setenv("RSBACKUP_DEVICES", devices);
  sp.reporting(globalWarningMask & WARNING_VERBOSE, false);
  sp.runAndWait(Subprocess::THROW_ON_ERROR
                |Subprocess::THROW_ON_CRASH
                |Subprocess::THROW_ON_SIGPIPE);
}

void preDeviceAccess() {
  if(!devicesReady) {
    runDeviceAccessHook(globalConfig.preAccess, "pre-access-hook");
    devicesReady = true;
  }
}

void closeOnUnmount(IO *f) {
  filesToClose.push_back(f);
}

void postDeviceAccess() {
  if(devicesReady) {
    deleteAll(filesToClose);
    runDeviceAccessHook(globalConfig.postAccess, "post-access-hook");
    devicesReady = false;
  }
}
