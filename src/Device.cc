#include <config.h>
#include "Conf.h"

bool Device::valid(const std::string &name) {
  return name.find_first_not_of(DEVICE_VALID) == std::string::npos;
}
