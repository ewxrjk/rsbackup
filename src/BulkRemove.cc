#include <config.h>
#include "Subprocess.h"
#include "Utils.h"

void BulkRemove(const std::string &path) {
  // Invoking rm makes more sense than re-implementing it.
  std::vector<std::string> cmd;
  cmd.push_back("rm");
  cmd.push_back("-rf");
  cmd.push_back(path);
  Subprocess::execute(cmd);
}
