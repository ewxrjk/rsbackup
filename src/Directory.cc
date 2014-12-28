// Copyright © 2011, 2013 Richard Kettlewell.
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
#include "IO.h"
#include "Errors.h"
#include <cerrno>
#include <algorithm>

Directory::~Directory() {
  if(dp)
    closedir(dp);
}

void Directory::open(const std::string &path_) {
  if(dp)
    throw std::logic_error("Directory::open on open directory");
  if(!(dp = opendir(path_.c_str())))
    throw IOError("opening " + path_, errno);
  path = path_;
}

void Directory::close() {
  if(!dp)
    throw std::logic_error("Directory::close on closed directory");
  closedir(dp);
  dp = NULL;
  path.clear();
}

bool Directory::get(std::string &name) const {
  if(!dp)
    throw std::logic_error("Directory::get on closed directory");
  errno = 0;
  struct dirent *de = readdir(dp);
  if(de) {
    name = de->d_name;
    return true;
  } else {
    if(errno)
      throw IOError("reading " + path, errno);
    return false;
  }
}

void Directory::get(std::vector<std::string> &files) const {
  std::string f;

  files.clear();
  while(get(f))
    files.push_back(f);
  // Sort files by name so that warnings come out in a predictable order.
  std::sort(files.begin(), files.end());
}

void Directory::getFiles(const std::string &path,
                         std::vector<std::string> &files) {
  Directory d;
  d.open(path);
  d.get(files);
}
