// Copyright © 2014, 2015, 2016 Richard Kettlewell.
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
#include "Errors.h"
#include "Utils.h"
#include <cstdio>

Database::Database(const std::string &path, bool rw) {
  int rc = sqlite3_open_v2(path.c_str(), &db,
                           rw ? SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
                              : SQLITE_OPEN_READONLY,
                           nullptr);
  if(rc != SQLITE_OK) {
    sqlite3_close_v2(db);
    error("sqlite3_open_v2 " + path, rc);
  }
}

void Database::error(sqlite3 *db, const std::string &description, int rc) {
  if(db)
    if(rc == SQLITE_BUSY)
      throw DatabaseBusy(rc, description + ": " + sqlite3_errmsg(db));
    else
      throw DatabaseError(rc, description + ": " + sqlite3_errmsg(db));
  else
    throw DatabaseError(rc, description + ": error " + sqlite3_errstr(rc));
}

bool Database::hasTable(const std::string &name) {
  return Statement(*this,
                   "SELECT name FROM sqlite_master"
                   " WHERE type = 'table' AND name = ?",
                   SQL_STRING, &name, SQL_END)
      .next();
}

void Database::execute(const char *cmd) {
  Statement(*this, cmd, SQL_END).next();
}

void Database::begin() {
  execute("BEGIN");
}

void Database::commit() {
  execute("COMMIT");
}

void Database::rollback() {
  execute("ROLLBACK");
}

Database::~Database() {
  int rc = sqlite3_close_v2(db);
  db = nullptr;
  if(rc != SQLITE_OK)
    fatal("sqlite3_close: error: %s", sqlite3_errstr(rc));
}

// Database::Statement --------------------------------------------------------

Database::Statement::Statement(Database &d, const char *cmd, ...):
    Statement(d) {
  va_list ap;
  va_start(ap, cmd);
  try {
    vprepare(cmd, ap);
  } catch(std::runtime_error &e) {
    va_end(ap);
    throw;
  }
  va_end(ap);
}

void Database::Statement::prepare(const char *cmd, ...) {
  va_list ap;
  va_start(ap, cmd);
  try {
    vprepare(cmd, ap);
  } catch(std::runtime_error &e) {
    va_end(ap);
    throw;
  }
  va_end(ap);
}

void Database::Statement::vprepare(const char *cmd, va_list ap) {
  if(stmt)
    throw std::logic_error("Database::Statement::vprepare: already prepared");
  const char *tail;
  D("vprepare: %s", cmd);
  int rc = sqlite3_prepare_v2(db, cmd, -1, &stmt, &tail);
  if(rc != SQLITE_OK)
    error(std::string("sqlite3_prepare_v2: ") + cmd, rc);
  if(tail && *tail)
    throw std::logic_error(
        std::string("Database::Statement::vprepare: trailing junk: \"") + tail
        + "\"");
  try {
    param = 1;
    vbind(ap);
  } catch(std::runtime_error &e) {
    sqlite3_finalize(stmt);
    stmt = nullptr;
    throw;
  }
}

void Database::Statement::vbind(va_list ap) {
  int t, i, rc;
  sqlite3_int64 i64;
  const char *cs;
  const std::string *s;

  if(param <= 0)
    throw std::logic_error("Database::Statement::vbind: invalid 'param' value");
  while((t = va_arg(ap, int)) != SQL_END) {
    switch(t) {
    case SQL_INT:
      i = va_arg(ap, int);
      D("vbind %d: %d", param, i);
      rc = sqlite3_bind_int(stmt, param, i);
      if(rc != SQLITE_OK)
        error("sqlite3_bind_int", rc);
      break;
    case SQL_INT64:
      i64 = va_arg(ap, sqlite3_int64);
      D("vbind %d: %lld", param, (long long)i64);
      rc = sqlite3_bind_int64(stmt, param, i64);
      if(rc != SQLITE_OK)
        error("sqlite3_bind_int64", rc);
      break;
    case SQL_STRING:
      s = va_arg(ap, const std::string *);
      D("vbind %d: %.*s", param, (int)s->size(), s->data());
      rc = sqlite3_bind_text(stmt, param, s->data(), s->size(),
                             SQLITE_TRANSIENT);
      if(rc != SQLITE_OK)
        error("sqlite3_bind_text", rc);
      break;
    case SQL_CSTRING:
      cs = va_arg(ap, const char *);
      D("vbind %d: %s", param, cs);
      rc = sqlite3_bind_text(stmt, param, cs, -1, SQLITE_TRANSIENT);
      if(rc != SQLITE_OK)
        error("sqlite3_bind_text", rc);
      break;
    case SQL_BLOB:
      s = va_arg(ap, const std::string *);
      D("vbind %d: <%zu bytes>", param, s->size());
      rc = sqlite3_bind_blob(stmt, param, s->data(), s->size(),
                             SQLITE_TRANSIENT);
      if(rc != SQLITE_OK)
        error("sqlite3_bind_text", rc);
      break;
    default:
      throw std::logic_error(
          "Database::Statement::vbind: unknown parameter type");
    }
    ++param;
  }
}

bool Database::Statement::next() {
  D("next");
  switch(int rc = sqlite3_step(stmt)) {
  case SQLITE_ROW: return true;
  case SQLITE_DONE: return false;
  case SQLITE_OK:
    throw std::logic_error(
        "Database::Statement::next: sqlite3_step returned SQLITE_OK");
  default: error("sqlite3_step", rc);
  }
}

int Database::Statement::get_int(int col) {
  int n = sqlite3_column_int(stmt, col);
  D("get_int %5d: %d", col, n);
  return n;
}

sqlite_int64 Database::Statement::get_int64(int col) {
  sqlite_int64 n = sqlite3_column_int64(stmt, col);
  D("get_int64 %3d: %lld", col, (long long)n);
  return n;
}

std::string Database::Statement::get_string(int col) {
  const unsigned char *t = sqlite3_column_text(stmt, col);
  int nt = sqlite3_column_bytes(stmt, col);
  D("get_string %2d: %.*s", col, nt, t);
  return std::string((const char *)t, nt);
}

std::string Database::Statement::get_blob(int col) {
  const void *t = sqlite3_column_blob(stmt, col);
  int nt = sqlite3_column_bytes(stmt, col);
  D("get_blob %4d: <%d bytes>", col, nt);
  return std::string((const char *)t, nt);
}

Database::Statement::~Statement() {
  sqlite3_finalize(stmt);
}
