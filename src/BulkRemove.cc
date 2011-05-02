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
#include "Subprocess.h"
#include "Utils.h"
#include "Command.h"
#include <cassert>

void BulkRemove(const std::string &path) {
  assert(command.act);
  // Invoking rm makes more sense than re-implementing it.
  std::vector<std::string> cmd;
  cmd.push_back("rm");
  cmd.push_back("-rf");
  cmd.push_back(path);
  Subprocess::execute(cmd);
}
