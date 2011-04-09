// -*-C++-*-
#ifndef ERRORS_H
#define ERRORS_H

#include <stdexcept>
#include <cstring>

// Represents an I/O error
class IOError: public std::runtime_error {
public:
  IOError(const std::string &msg):
    std::runtime_error(msg) {}
  IOError(const std::string &msg, int errno_value):
    std::runtime_error(msg + ": " + strerror(errno_value)) {}
};

// Represents a syntax error; does not include path/line information.
class SyntaxError: public std::runtime_error {
public:
  SyntaxError(const std::string &msg):
    std::runtime_error(msg) {}
};

// Represents some problem with a config file; usually equivalent to a
// SyntaxError but with path/line information included in the message.
class ConfigError: public std::runtime_error {
public:
  ConfigError(const std::string &msg):
    std::runtime_error(msg) {}
};

// Represents some problem with a store.
class BadStore: public std::runtime_error {
public:
  BadStore(const std::string &msg):
    std::runtime_error(msg) {}
};

#endif /* ERRORS_H */
