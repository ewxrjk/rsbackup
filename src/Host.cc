#include <config.h>
#include "Conf.h"

void Host::select(bool sense) {
  for(volumes_type::iterator volumes_iterator = volumes.begin();
      volumes_iterator != volumes.end();
      ++volumes_iterator)
    volumes_iterator->second->select(sense);
}

bool Host::selected() const {
  for(volumes_type::const_iterator volumes_iterator = volumes.begin();
      volumes_iterator != volumes.end();
      ++volumes_iterator)
    if(volumes_iterator->second->selected())
      return true;
  return false;
}

bool Host::valid(const std::string &name) {
  return name.find_first_not_of(HOST_VALID) == std::string::npos;
}

Volume *Host::findVolume(const std::string &volumeName) const {
  volumes_type::const_iterator it = volumes.find(volumeName);
  return it != volumes.end() ? it->second : NULL;
}
