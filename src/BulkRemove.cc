#include <config.h>
#include "Subprocess.h"

void BulkRemove(const std::string &path) {
  std::vector<std::string> cmd;
  cmd.push_back("rm");
  cmd.push_back("-rf");
  cmd.push_back(path);
  Subprocess::execute(cmd);
}
