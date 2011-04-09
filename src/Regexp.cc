#include <config.h>
#include "Regexp.h"
#include "Errors.h"

Regexp::Regexp(const std::string &regex, int cflags) {
  int rc;

  if((rc = regcomp(&compiled, regex.c_str(), cflags))) {
    char errbuf[1024];
    regerror(rc, &compiled, errbuf, sizeof errbuf);
    throw std::runtime_error(errbuf);   // TODO exception type
  }
}

Regexp::~Regexp() {
  regfree(&compiled);
}

bool Regexp::matches(const std::string &s, int eflags) {
  int rc;

  subject = s;
  capture.resize(20);                   // TODO how big should we make it?
  if((rc = regexec(&compiled, subject.c_str(),
                   capture.size(), &capture[0],
                   eflags)) == REG_NOMATCH)
    return false;
  return true;
}

std::string Regexp::sub(size_t n) const {
  if(n > capture.size() || capture[n].rm_so == -1)
    return "";
  return std::string(subject, 
                     capture[n].rm_so, capture[n].rm_eo - capture[n].rm_so);
}
