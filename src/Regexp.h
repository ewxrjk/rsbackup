//-*-C++-*-
#ifndef REGEXP_H
#define REGEXP_H

#include <sys/types.h>
#include <regex.h>
#include <string>
#include <vector>

class Regexp {
public:
  Regexp(const std::string &regex, int cflags = REG_EXTENDED);
  ~Regexp();
  bool matches(const std::string &s, int eflags = 0);
  std::string sub(size_t n) const;
  std::vector<regmatch_t> capture;
  
private:
  std::string subject;
  regex_t compiled;
};

#endif /* REGEXP_H */

