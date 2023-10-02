// Copyright © Richard Kettlewell.
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
#include "Database.h"
#include "Conf.h"
#include "Command.h

static void test_upgrade(int oldver, int newver) {
  // Create the database at the older version
  globalDatabaseVersion = oldver;
  Conf c;
  remove(c.database.c_str());
  Database &db = c.getdb();
}

int main() {
  // Check real upgrade paths
  test_upgrade(10, 11);
  // Check that upgrade code is idempotent on latest-to-latest
  test_upgrade(11, 11);
}
