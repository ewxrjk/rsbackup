#ifndef STORE_H
#define STORE_H

#include <string>

class Device;

// Represents a store, i.e. a path at which a backup device may be mounted.
class Store {
public:
  Store(const std::string &path_): path(path_), device(NULL) {}
  std::string path;
  Device *device;                       // device for this, or NULL

  void identify();                      // identify device
};

#endif /* STORE_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
