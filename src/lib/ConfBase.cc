// Copyright © 2011, 2012, 2014-17, 2019 Richard Kettlewell.
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
#include "ConfBase.h"
#include "Utils.h"
#include <cctype>
#include <sstream>

std::string ConfBase::quote(const std::string &s) {
  bool need_quote = false;
  if(!s.size())
    need_quote = true;
  else if(s[0] == '#')
    need_quote = true;
  else {
    for(auto c: s) {
      if(isspace(c))
        need_quote = true;
      if(c == '\\' || c == '"')
        need_quote = true;
    }
  }
  if(!need_quote)
    return s;
  std::stringstream ss;
  ss << '"';
  for(auto c: s) {
    if(c == '\\' || c == '"')
      ss << '\\';
    ss << c;
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

void ConfBase::write(std::ostream &os, int step, bool verbose) const {
  describe_type *d = verbose && !getParent() ? describe : nodescribe;
  ConfBase *parent = getParent();

  d(os, "# Maximum time with no backups before flagging host in report", step);
  d(os, "#  max-age DAYS", step);
  os << indent(step) << "max-age " << maxAge << '\n';
  d(os, "", 0);

  d(os, "# Backup policy for this " + what(), step);
  d(os, "#  backup-policy always|daily|interval", step);
  os << indent(step) << "backup-policy " << backupPolicy << '\n';
  d(os, "", 0);

  d(os, "# Backup parameters", step);
  d(os, "#  backup-parameter NAME VALUE", step);
  d(os, "#  backup-parameter --remove NAME", step);
  d(os, "# For parameters, see documentation for individual backup policies",
    step);
  for(auto &p: backupParameters)
    os << indent(step) << "backup-parameter " << quote(p.first) << ' '
       << quote(p.second) << '\n';
  // Any parameters set in our parent but not present here must have been
  // removed.
  if(parent)
    for(auto &p: parent->backupParameters)
      if(!contains(backupParameters, p.first))
        os << indent(step) << "backup-parameter --remove " << quote(p.first)
           << '\n';
  d(os, "", 0);

  d(os, "# Prune policy for this " + what(), step);
  d(os, "#  prune-policy age|decay|exec|never", step);
  os << indent(step) << "prune-policy " << prunePolicy << '\n';
  d(os, "", 0);

  d(os, "# Prune parameters", step);
  d(os, "#  prune-parameter NAME VALUE", step);
  d(os, "#  prune-parameter --remove NAME", step);
  d(os, "# For parameters, see documentation for individual pruning policies",
    step);
  for(auto &p: pruneParameters)
    os << indent(step) << "prune-parameter " << quote(p.first) << ' '
       << quote(p.second) << '\n';
  // Any parameters set in our parent but not present here must have been
  // removed.
  if(parent)
    for(auto &p: parent->pruneParameters)
      if(!contains(pruneParameters, p.first))
        os << indent(step) << "prune-parameter --remove " << quote(p.first)
           << '\n';
  d(os, "", 0);

  d(os, "# Command to run prior to making a backup", step);
  d(os, "#  pre-volume-hook COMMAND ...", step);
  if(preVolume.size())
    os << indent(step) << "pre-volume-hook " << quote(preVolume) << '\n';
  d(os, "", 0);

  d(os, "# Command to run after making a backup", step);
  d(os, "#  post-volume-hook COMMAND ...", step);
  if(postVolume.size())
    os << indent(step) << "post-volume-hook " << quote(postVolume) << '\n';
  d(os, "", 0);

  d(os, "# Maximum time to wait for rsync to complete", step);
  d(os, "#  rsync-timeout SECONDS", step);
  if(rsyncTimeout)
    os << indent(step) << "rsync-timeout " << rsyncTimeout << '\n';
  d(os, "", 0);

  d(os, "# Maximum time to wait before giving up on a host", step);
  d(os, "#  ssh-timeout SECONDS", step);
  os << indent(step) << "ssh-timeout " << sshTimeout << '\n';
  d(os, "", 0);

  d(os, "# Maximum time to wait for a hook to complete", step);
  d(os, "#  hook-timeout SECONDS", step);
  if(hookTimeout)
    os << indent(step) << "hook-timeout " << hookTimeout << '\n';
  d(os, "", 0);

  d(os, "# rsync command", step);
  d(os, "# rsync-command COMMAND", step);
  os << indent(step) << "rsync-command " << quote(rsyncCommand) << '\n';
  d(os, "", 0);

  d(os, "# rsync base options", step);
  d(os, "# rsync-base-options OPTION ...", step);
  os << indent(step) << "rsync-base-options";
  for(auto &opt: rsyncBaseOptions)
    os << " " << quote(opt);
  os << "\n";
  d(os, "", 0);

  d(os, "# rsync extra options", step);
  d(os, "# rsync-extra-options OPTION ...", step);
  os << indent(step) << "rsync-extra-options";
  for(auto &opt: rsyncExtraOptions)
    os << " " << quote(opt);
  os << "\n";
  d(os, "", 0);

  // TODO hacky way of managing {toplevel,host}-only directives
  if(what() != "volume") {
    d(os, "# Host check behavior", step);
    d(os, "#  host-check ssh", step);
    d(os, "#  host-check always-up", step);
    d(os, "#  host-check command COMMAND ...", step);
    os << indent(step) << "host-check " << quote(hostCheck) << '\n';
    d(os, "", 0);
  }
}

void ConfBase::describe(std::ostream &os, const std::string &description,
                        int step) {
  if(description.size())
    os << indent(step) << description;
  os << '\n';
}

void ConfBase::nodescribe(std::ostream &, const std::string &, int) {}

void ConfBase::writeVector(std::ostream &os, int step,
                           const std::string &directive,
                           const std::vector<std::string> &value) const {
  if(value.size()) {
    os << indent(step) << directive;
    int y = step + directive.size();
    for(const auto &s: value) {
      if(y + s.size() >= 80) {
        os << '\n' << indent(step) << directive << " +";
        y = step + directive.size() + 2;
      }
      const std::string q = quote(s);
      os << ' ' << q;
      y += q.size() + 1;
    }
    if(y)
      os << '\n';
  } else
    os << indent(step) << directive << "\n";
}
