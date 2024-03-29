// Copyright © 2011-13, 2015-19 Richard Kettlewell.
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
#include "Conf.h"
#include "Store.h"
#include "Device.h"
#include "Errors.h"
#include "IO.h"
#include "Utils.h"
#include "DeviceAccess.h"
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>

// Identify the device on this store, if any
void Store::identify() {
  IO *f = nullptr;
  try {
    struct stat sb;

    if(device)
      return; // already identified
    if(stat(path.c_str(), &sb) < 0)
      throw BadStore("store '" + path + "' does not exist");
    if(mounted) {
      const std::string parent_path = path + "/..";
      struct stat parent_sb;
      if(stat(parent_path.c_str(), &parent_sb) < 0)
        throw FatalStoreError("cannot stat '" + parent_path + "'");
      if(sb.st_dev == parent_sb.st_dev)
        throw UnavailableStore("store '" + path + "' is not mounted");
    }
    // Make sure backup devices are mounted
    preDeviceAccess();
    // Read the device name
    f = new IO();
    f->open(path + PATH_SEP + "device-id", "r");
    std::string deviceName;
    if(!f->readline(deviceName))
      throw BadStore("store '" + path + "' has a malformed device-id");
    // See if it exists
    auto devices_iterator = globalConfig.devices.find(deviceName);
    if(devices_iterator == globalConfig.devices.end())
      throw BadStore("store '" + path + "' has unknown device-id '" + deviceName
                     + "'");
    Device *foundDevice = devices_iterator->second;
    // Duplicates are bad, sufficiently so that we don't treat it as
    // just an unsuitable store; something is seriously wrong and it
    // needs immediate attention.
    if(foundDevice->store)
      throw FatalStoreError("store '" + path + "' has duplicate device-id '"
                            + deviceName + "', also found on store '"
                            + foundDevice->store->path + "'");
    if(!globalConfig.publicStores) {
      // Verify permissions
      if(sb.st_uid)
        throw BadStore("store '" + path + "' not owned by root");
      if(sb.st_mode & 077)
        throw BadStore("store '" + path + "' is not private");
    }
    device = foundDevice;
    device->store = this;
    // On success, leave a file open on the store to stop it being unmounted
    // while we it's a potential destination for backups; but close it before
    // unmounting.
    closeOnUnmount(f);
  } catch(IOError &e) {
    // Re-throw with the appropriate error type
    if(e.errno_value == ENOENT)
      throw UnavailableStore(e.what());
    else
      throw BadStore(e.what());
  } catch(...) {
    delete f;
    throw;
  }
}
