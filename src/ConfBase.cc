// Copyright © 2011, 2012, 2014 Richard Kettlewell.
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
#include "rsbackup.h"
#include "Conf.h"
#include <cctype>
#include <sstream>

ConfBase::~ConfBase() {
}

std::string ConfBase::quote(const std::string &s) {
  bool need_quote = !s.size();
  for(size_t n = 0; n < s.size(); ++n) {
    if(isspace(s[n]))
      need_quote = true;
    if(s[n] == '#' && n == 0)
      need_quote = true;
    if(s[n] == '\\' || s[n] == '"')
      need_quote = true;
  }
  if(!need_quote)
    return s;
  std::stringstream ss;
  ss << '"';
  for(size_t n = 0; n < s.size(); ++n) {
    if(s[n] == '\\' || s[n] == '"')
      ss << '\\';
    ss << s[n];
  }
  ss << '"';
  return ss.str();
}

std::string ConfBase::quote(const std::vector<std::string> &vs) {
  std::stringstream ss;
  for(size_t n = 0; n < vs.size(); ++n) {
    if(n)
      ss << ' ';
    ss << quote(vs[n]);
  }
  return ss.str();
}

std::string ConfBase::indent(int step) {
  return std::string(step, ' ');
}

void ConfBase::write(std::ostream &os, int step) const {
  os << indent(step) << "max-age " << maxAge << '\n';
  os << indent(step) << "min-backups " << minBackups << '\n';
  os << indent(step) << "prune-age " << pruneAge << '\n';
  if(preBackup.size())
    os << indent(step) << "pre-backup-hook " << quote(preBackup) << '\n';
  if(postBackup.size())
    os << indent(step) << "post-backup-hook " << quote(postBackup) << '\n';
  if(rsyncTimeout)
    os << indent(step) << "rsync-timeout " << rsyncTimeout << '\n';
  if(hookTimeout)
    os << indent(step) << "hook-timeout " << hookTimeout << '\n';
}
