// Copyright © 2011, 2014-2016, 2019 Richard Kettlewell.
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
#include "Device.h"
#include "Backup.h"
#include "Volume.h"
#include "Host.h"
#include "Store.h"
#include "Database.h"
#include "Utils.h"
#include "Errors.h"
#include <cstdio>
#include <cassert>
#include <regex>

// Return the path to this backup
std::string Backup::backupPath() const {
  const Host *host = volume->parent;
  const Device *device = host->parent->findDevice(deviceName);
  const Store *store = device->store;
  assert(store != nullptr);
  return (store->path + PATH_SEP + host->name + PATH_SEP + volume->name
          + PATH_SEP + id);
}

void Backup::insert(Database &db, bool replace) const {
  const std::string command = replace ? "INSERT OR REPLACE" : "INSERT";
  Database::Statement(db,
                      (command
                       + " INTO backup"
                         " (host,volume,device,id,time,pruned,rc,status,log)"
                         " VALUES (?,?,?,?,?,?,?,?,?)")
                          .c_str(),
                      SQL_STRING, &volume->parent->name, SQL_STRING,
                      &volume->name, SQL_STRING, &deviceName, SQL_STRING, &id,
                      SQL_INT64, (sqlite_int64)time, SQL_INT64,
                      (sqlite_int64)pruned, SQL_INT, waitStatus, SQL_INT,
                      status, SQL_STRING, &contents, SQL_END)
      .next();
}

void Backup::update(Database &db) const {
  Database::Statement(db,
                      "UPDATE backup SET rc=?,status=?,log=?,time=?,pruned=?"
                      " WHERE host=? AND volume=? AND device=? AND id=?",
                      SQL_INT, waitStatus, SQL_INT, status, SQL_STRING,
                      &contents, SQL_INT64, (sqlite_int64)time, SQL_INT64,
                      (sqlite_int64)pruned, SQL_STRING, &volume->parent->name,
                      SQL_STRING, &volume->name, SQL_STRING, &deviceName,
                      SQL_STRING, &id, SQL_END)
      .next();
}

void Backup::remove(Database &db) const {
  Database::Statement(db,
                      "DELETE FROM backup"
                      " WHERE host=? AND volume=? AND device=? AND id=?",
                      SQL_STRING, &volume->parent->name, SQL_STRING,
                      &volume->name, SQL_STRING, &deviceName, SQL_STRING, &id,
                      SQL_END)
      .next();
}

void Backup::setStatus(int n) {
  if(status != n) {
    status = n;
    if(volume)
      volume->calculate();
  }
}

Device *Backup::getDevice() const {
  return volume->parent->parent->findDevice(deviceName);
}

long long Backup::getSize() const {
  static std::regex size_regexp("Total file size: ([0-9,]+) bytes");
  std::smatch mr;
  if(!std::regex_search(contents, mr, size_regexp))
    return -1;
  std::string size_string;
  for(auto it = mr[1].first; it != mr[1].second; ++it) {
    auto ch = *it;
    if(isdigit(ch))
      size_string += ch;
  }
  try {
    return parseInteger(size_string, 0, std::numeric_limits<long long>::max());
  } catch(SyntaxError &e) {
    return -1;
  }
}

const char *const backup_status_names[] = {"unknown", "underway", "complete",
                                           "failed",  "pruning",  "pruned"};
