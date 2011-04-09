// -*-C++-*-
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
