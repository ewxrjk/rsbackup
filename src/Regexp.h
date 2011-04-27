//-*-C++-*-
#ifndef REGEXP_H
#define REGEXP_H

#include <sys/types.h>
#include <regex.h>
#include <string>
#include <vector>

// Wrapper for libc's regex support
class Regexp {
public:
  Regexp(const std::string &regex, int cflags = REG_EXTENDED);
  ~Regexp();

  // Return true if S matches REGEX
  bool matches(const std::string &s, int eflags = 0);

  // Return the Nth capture (0=the matched substring)
  std::string sub(size_t n) const;

private:
  std::string subject;
  regex_t compiled;
  std::vector<regmatch_t> capture;
};

#endif /* REGEXP_H */

