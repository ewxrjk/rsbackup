#include <config.h>
#include "Conf.h"

void Volume::select(bool sense) {
  isSelected = sense;
}

bool Volume::valid(const std::string &name) {
  return name.find_first_not_of(VOLUME_VALID) == std::string::npos;
}
