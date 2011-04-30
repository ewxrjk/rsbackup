//-*-C++-*-
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
#ifndef DATE_H
#define DATE_H

#include <string>

// A (proleptic) Gregorian date.  Don't try years before 1CE.
class Date {
public:
  Date(): y(0), m(1), d(1) {}
  Date(const std::string &dateString);  // expects YYYY-MM-DD
  Date(int y_, int m_, int d_): y(y_), m(m_), d(d_) {}
  Date(time_t when);

  int operator-(const Date &that) const;

  Date operator-(int days) const;

  bool operator<(const Date &that) const;

  bool operator==(const Date &that) const {
    return y == that.y && m == that.m && d == that.d;
  }

  bool operator>(const Date &that) const { return that < *this; }

  bool operator<=(const Date &that) const { return !(*this > that); }

  bool operator>=(const Date &that) const { return !(*this < that); }

  bool operator!=(const Date &that) const { return !(*this == that); }

  std::string toString() const;         // YYYY-MM-DD

  int toNumber() const;

  static Date today();
private:
  inline bool isLeapYear() const {
    return isLeapYear(y);
  }
  static inline bool isLeapYear(int y) {
    return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
  }
  static int monthLength(int y, int m);
  int y, m, d;
  static const int mday[];
};

#endif /* DATE_H */

