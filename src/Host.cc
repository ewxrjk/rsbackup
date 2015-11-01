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
#include "Subprocess.h"
#include <cstdio>
#include <cstdarg>
#include <ostream>

void Host::select(bool sense) {
  for(auto volumes_iterator = volumes.begin();
      volumes_iterator != volumes.end();
      ++volumes_iterator)
    volumes_iterator->second->select(sense);
}

bool Host::selected() const {
  for(auto volumes_iterator = volumes.begin();
      volumes_iterator != volumes.end();
      ++volumes_iterator)
    if(volumes_iterator->second->selected())
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
  return it != volumes.end() ? it->second : NULL;
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
  return invoke(NULL, "true", (const char *)NULL) == 0;
}

void Host::write(std::ostream &os, int step) const {
  os << indent(step) << "host " << quote(name) << '\n';
  step += 4;
  ConfBase::write(os, step);
  os << indent(step) << "hostname " << quote(hostname) << '\n';
  if(user.size())
    os << indent(step) << "user " << quote(user) << '\n';
  if(alwaysUp)
    os << indent(step) << "always-up" << '\n';
  if(devicePattern.size())
    os << indent(step) << "devices " << quote(devicePattern) << '\n';
  for(auto it = volumes.begin(); it != volumes.end(); ++it) {
    os << '\n';
    static_cast<ConfBase *>(it->second)->write(os, step);
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
