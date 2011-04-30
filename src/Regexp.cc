// Copyright © 2011 Richard Kettlewell.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <config.h>
#include "Regexp.h"
#include "Errors.h"

Regexp::Regexp(const std::string &regex, int cflags) {
  int rc;

  if((rc = regcomp(&compiled, regex.c_str(), cflags))) {
    char errbuf[1024];
    regerror(rc, &compiled, errbuf, sizeof errbuf);
    throw InvalidRegexp(errbuf);
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
