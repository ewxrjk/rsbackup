// Copyright © 2011, 2014, 2015 Richard Kettlewell.
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
#include "Conf.h"
#include "Backup.h"
#include "Volume.h"
#include "Host.h"
#include "Subprocess.h"
#include <cstdio>
#include <cstdarg>
#include <ostream>

Host::~Host() {
  for(auto &v: volumes)
    delete v.second;
}

void Host::select(bool sense) {
  for(auto &v: volumes)
    v.second->select(sense);
}

bool Host::selected() const {
  for(auto &v: volumes)
    if(v.second->selected())
      return true;
  return false;
}

bool Host::valid(const std::string &name) {
  return name.size() > 0
    && name.at(0) != '-'
    && name.find_first_not_of(HOST_VALID) == std::string::npos;
}

void Host::addVolume(Volume *v) {
  volumes[v->name] = v;
}

Volume *Host::findVolume(const std::string &volumeName) const {
  auto it = volumes.find(volumeName);
  return it != volumes.end() ? it->second : nullptr;
}

std::string Host::userAndHost() const {
  return (user.size()
          ? user + "@" + hostname
          : hostname);
}

std::string Host::sshPrefix() const {
  std::string s = userAndHost();
  return s == "localhost" ? "" : s + ":";
}

bool Host::available() const {
  // localhost is always available
  if(hostname == "localhost")
    return true;
  return invoke(nullptr, "true", (const char *)nullptr) == 0;
}

void Host::write(std::ostream &os, int step, bool verbose) const {
  describe_type *d = verbose ? describe : nodescribe;

  os << indent(step) << "host " << quote(name) << '\n';
  step += 4;
  ConfBase::write(os, step, verbose);
  d(os, "", step);

  d(os, "# Hostname for SSH", step);
  d(os, "#   hostname NAME", step);
  os << indent(step) << "hostname " << quote(hostname) << '\n';
  d(os, "", step);

  d(os, "# Username for SSH; default is not to supply a username", step);
  d(os, "#   user NAME", step);
  if(user.size())
    os << indent(step) << "user " << quote(user) << '\n';
  d(os, "", step);

  d(os, "# Treat host being down as an error", step);
  d(os, "#   always-up true|false", step);
  os << indent(step) << "always-up " << (alwaysUp ? "true" : "false") << '\n';
  d(os, "", step);

  d(os, "# Glob pattern for devices this host will be backed up to", step);
  d(os, "#   devices PATTERN", step);
  if(devicePattern.size())
    os << indent(step) << "devices " << quote(devicePattern) << '\n';
  d(os, "", step);

  d(os,
    "# Priority for this host (higher priority = backed up earlier)",
    step);
  d(os, "#   priority INTEGER", step);
  os << indent(step) << "priority " << priority << '\n';

  for(auto &v: volumes) {
    os << '\n';
    v.second->write(os, step, verbose);
  }
}

int Host::invoke(std::string *capture,
                 const char *cmd, ...) const {
  std::vector<std::string> args;
  const char *arg;
  va_list ap;

  if(hostname != "localhost") {
    args.push_back("ssh");
    if(sshTimeout > 0) {
      char buffer[64];
      snprintf(buffer, sizeof buffer, "%d", sshTimeout);
      args.push_back(std::string("-oConnectTimeout=") + buffer);
    }
    args.push_back(userAndHost());
  }
  args.push_back(cmd);
  va_start(ap, cmd);
  while((arg = va_arg(ap, const char *)))
    args.push_back(arg);
  va_end(ap);
  Subprocess sp(args);
  if(capture) {
    sp.capture(1, capture);
    return sp.runAndWait(true);
  } else {
    sp.nullChildFD(1);
    sp.nullChildFD(2);
    return sp.runAndWait(false);
  }
}

ConfBase *Host::getParent() const {
  return parent;
}

std::string Host::what() const {
  return "host";
}
