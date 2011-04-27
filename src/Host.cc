#include <config.h>
#include "Conf.h"
#include "Subprocess.h"
#include <cstdio>

void Host::select(bool sense) {
  for(volumes_type::iterator volumes_iterator = volumes.begin();
      volumes_iterator != volumes.end();
      ++volumes_iterator)
    volumes_iterator->second->select(sense);
}

bool Host::selected() const {
  for(volumes_type::const_iterator volumes_iterator = volumes.begin();
      volumes_iterator != volumes.end();
      ++volumes_iterator)
    if(volumes_iterator->second->selected())
      return true;
  return false;
}

bool Host::valid(const std::string &name) {
  return name.find_first_not_of(HOST_VALID) == std::string::npos;
}

Volume *Host::findVolume(const std::string &volumeName) const {
  volumes_type::const_iterator it = volumes.find(volumeName);
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
  // Synthesize the command - just execute 'true', perhaps with a short SSH
  // connection timeout.
  std::vector<std::string> cmd;
  cmd.push_back("ssh");
  if(parent->sshTimeout > 0) {
    char buffer[10];
    sprintf(buffer, "%d", parent->sshTimeout);
    cmd.push_back(std::string("-oConnectTimeout=") + buffer);
  }
  cmd.push_back(userAndHost());
  cmd.push_back("true");
  Subprocess sp(cmd);
  sp.nullChildFD(1);
  sp.nullChildFD(2);
  int rc = sp.runAndWait(false);
  return rc == 0;
}
