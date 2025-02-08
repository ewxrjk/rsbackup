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

#ifndef CONCURRENCY_H
# define CONCURRENCY_H

class ConcurrencyLimit {
public:
  inline ConcurrencyLimit(int max = 1): unused(max) {}

  inline bool usable() const {
    return unused > 0;
  }

private:
  int unused;
  friend class TakeConcurrencyLimit;
};

class TakeConcurrencyLimit {
public:
  inline TakeConcurrencyLimit(ConcurrencyLimit &cl): limit(&cl) {
    limit->unused--;
  }
  inline ~TakeConcurrencyLimit() {
    limit->unused++;
  }
private:
  ConcurrencyLimit *limit;
};

#endif