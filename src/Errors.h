// -*-C++-*-
#ifndef ERRORS_H
#define ERRORS_H

#include <stdexcept>
#include <cstring>

// Represents a low-level system error
class SystemError: public std::runtime_error {
public:
  SystemError(const std::string &msg): std::runtime_error(msg) {}
  SystemError(const std::string &msg, int error):
    std::runtime_error(msg + ": " + strerror(error)) {}
};

// Represents an I/O error
class IOError: public SystemError {
public:
  IOError(const std::string &msg): SystemError(msg) {}
  IOError(const std::string &msg, int error): SystemError(msg, error) {}
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
public: ConfigError(const std::string &msg): std::runtime_error(msg) {}
};

// Represents some problem with a store.  BadStore errors are reported but are
// not fatal.
class BadStore: public std::runtime_error {
public: BadStore(const std::string &msg): std::runtime_error(msg) {}
};

// Represents some problem with a store.  FatalStoreErrors abort the whole
// process.
class FatalStoreError: public std::runtime_error {
public: FatalStoreError(const std::string &msg): std::runtime_error(msg) {}
};

// Represents a problem with the command line.
class CommandError: public std::runtime_error {
public: CommandError(const std::string &msg): std::runtime_error(msg) {}
};

// Represents some problem with a date string
class InvalidDate: public std::runtime_error {
public: InvalidDate(const std::string &msg): std::runtime_error(msg) {}
};

// Represents some problem with a regexp.
class InvalidRegexp: public SystemError {
public: InvalidRegexp(const std::string &msg): SystemError(msg) {}
};

// Represents failure of a subprocess
class SubprocessFailed: public std::runtime_error {
public:
  SubprocessFailed(const std::string &name, int wstat): 
    std::runtime_error(format(name, wstat)) {}
private:
  static std::string format(const std::string &name, int wstat);
};

#endif /* ERRORS_H */
