// Copyright © 2014 Richard Kettlewell.
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
#include "Utils.h"
#include "Database.h"
#include <cassert>
#include <unistd.h>

#define DBPATH "test.db"

static void test_create() {
  Database d(DBPATH);
  assert(!d.hasTable("t"));
  d.execute("CREATE TABLE t (i INT PRIMARY KEY, s TEXT)");
  assert(d.hasTable("t"));
}

static void test_populate() {
  Database d(DBPATH);
  d.begin();
  Database::Statement(d,
                      "INSERT INTO t (i, s) VALUES (?, ?)",
                      SQL_INT, 0,
                      SQL_CSTRING, "zero",
                      SQL_END).next();
  Database::Statement(d, "INSERT INTO t (i, s) VALUES (?, ?)",
                      SQL_INT64, (sqlite_int64)1,
                      SQL_CSTRING, "one",
                      SQL_END).next();
  d.commit();
}

static void test_retrieve() {
  Database d(DBPATH);
  {
    Database::Statement s(d);
    s.prepare("SELECT s FROM t WHERE i = ?",
              SQL_INT, 0,
              SQL_END);
    assert(s.next());
    std::string str = s.get_string(0);
    assert(str == "zero");
    assert(!s.next());
  }
  {
    Database::Statement s(d);
    s.prepare("SELECT i FROM t WHERE s = ?",
              SQL_CSTRING, "one",
              SQL_END);
    assert(s.next());
    int n = s.get_int(0);
    assert(n == 1);
    assert(!s.next());
  }
  {
    Database::Statement s(d);
    s.prepare("SELECT i FROM t WHERE s = ?",
              SQL_CSTRING, "zero",
              SQL_END);
    assert(s.next());
    sqlite_int64 n = s.get_int64(0);
    assert(n == 0);
    assert(!s.next());
  }
}

int main() {
  unlink(DBPATH);
  test_create();
  test_populate();
  test_retrieve();
  unlink(DBPATH);
  return 0;
}
