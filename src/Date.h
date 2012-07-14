//-*-C++-*-
// Copyright © 2011, 2012 Richard Kettlewell.
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
/** @file Date.h
 * @brief Date manipulation
 */

#include <string>

/** @brief A (proleptic) Gregorian date
 *
 * Don't try years before 1CE.
 */
class Date {
public:
  /** @brief Constructor */
  Date(): y(0), m(1), d(1) {}

  /** @brief Constructor
   * @param dateString Date in YYYY-MM-DD format
   */
  Date(const std::string &dateString);

  /** @brief Constructor
   * @param y_ Year
   * @param m_ Month from 1
   * @param d_ Day from 1
   */
  Date(int y_, int m_, int d_): y(y_), m(m_), d(d_) {}

  /** @brief Constructor
   * @param when Moment in time
   */
  Date(time_t when);

  /** @brief Different between two dates in days
   * @param that Other date
   */
  int operator-(const Date &that) const;

  /** @brief Count backwards in time
   * @param days Number of days back
   * @return Earlier date
   */
  Date operator-(int days) const;

  /** @brief Comparison operator
   * @param that Other date
   * @return true if this is less than that
   */
  bool operator<(const Date &that) const;

  /** @brief Comparison operator
   * @param that Other date
   * @return true if this is equal to that
   */
  bool operator==(const Date &that) const {
    return y == that.y && m == that.m && d == that.d;
  }

  /** @brief Comparison operator
   * @param that Other date
   * @return true if this is greater than that
   */
  bool operator>(const Date &that) const { return that < *this; }

  /** @brief Comparison operator
   * @param that Other date
   * @return true if this is less or equal to that
   */
  bool operator<=(const Date &that) const { return !(*this > that); }

  /** @brief Comparison operator
   * @param that Other date
   * @return true if this is greater than or equal to that
   */
  bool operator>=(const Date &that) const { return !(*this < that); }

  /** @brief Comparison operator
   * @param that Other date
   * @return true if this is not equal to that
   */
  bool operator!=(const Date &that) const { return !(*this == that); }

  operator const char *() const { return toString().c_str(); }

  /** @brief Convert to string
   * @return Date in "YYYY-MM-DD" format
   */
  std::string toString() const;         // YYYY-MM-DD

  /** @brief Convert to day number
   * @return Day number
   */
  int toNumber() const;

  /** @brief Today
   * @return Today's date
   */
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

