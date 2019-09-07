// Copyright © 2011, 2012, 2015, 2016, 2019 Richard Kettlewell.
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
#include <cassert>
#include "BulkRemove.h"

void BulkRemove::initialize(const std::string &path) {
  // Invoking rm makes more sense than re-implementing it.
  std::vector<std::string> cmd = {"rm", "-rf", path};
  setCommand(cmd);
  reporting(globalWarningMask & WARNING_VERBOSE, false);
  // BulkRemoves only get created when the caller has committed to removing
  // things, so no point checking command.act here.
}
