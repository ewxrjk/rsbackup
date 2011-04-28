#include <config.h>
#include "Conf.h"
#include "Store.h"
#include "Errors.h"
#include "IO.h"

// Identify the device on this store, if any
void Store::identify() {
  struct stat sb;

  if(device)
    return;                     // already identified
  if(stat(path.c_str(), &sb) < 0)
    throw BadStore("store '" + path + "' does not exist");
  if(!config.publicStores) {
    // Verify permissions
    if(sb.st_uid)
      throw BadStore("store '" + path + "' not owned by root");
    if(sb.st_mode & 077)
      throw BadStore("store '" + path + "' is not private");
  }
  // Leave a file open on the store to stop it being unmounted while we it's a
  // potential destination for backups.
  StdioFile *f = NULL;
  try {
    // Read the device name
    f = new StdioFile();
    f->open(path + PATH_SEP + "device-id", "r");
    std::string deviceName;
    if(!f->readline(deviceName))
      throw BadStore("store '" + path + "' has a malformed device-id");
    // See if it exists
    devices_type::iterator devices_iterator = config.devices.find(deviceName);
    if(devices_iterator == config.devices.end())
      throw BadStore("store '" + path
                     + "' has unknown device-id '" + deviceName + "'");
    Device *foundDevice = devices_iterator->second;
    // Duplicates are bad, sufficiently so that we don't treat it as
    // just an unsuitable store; something is seriously wrong and it
    // needs immediate attention.
    if(foundDevice->store)
      throw FatalStoreError("store '" + path
                            + "' has duplicate device-id '" + deviceName
                            + "', also found on store '" + foundDevice->store->path
                            + "'");
    device = foundDevice;
    device->store = this;
  } catch(IOError &e) {
    if(f)
      delete f;
    // Re-throw with the expected error type
    throw BadStore(e.what());
  }
}
