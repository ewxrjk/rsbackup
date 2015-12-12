//-*-C++-*-
// Copyright © 2011, 2012, 2014 Richard Kettlewell.
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
 * @brief %Date manipulation
 */

#include <string>
#include <ctime>

/** @brief A (proleptic) Gregorian date
 *
 * Don't try years before 1CE.
 */
class Date {
public:
  /** @brief Constructor */
  Date(): Date(0, 1, 1) {}

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

  /** @brief Assignment operator
   * @param dateString Date in YYYY-MM-DD format
   */
  Date &operator=(const std::string &dateString);

  /** @brief Different between two dates in days
   * @param that Other date
   */
  int operator-(const Date &that) const;

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

  /** @brief Increment date
   * @return Next day
   */
  Date &operator++();

  /** @brief Advance by 1 month
   * @return @c *this
   *
   * If the day ends up outside the month it is clipped to the last day.
   */
  Date &addMonth();

  /** @brief Convert to string
   * @return Date in "YYYY-MM-DD" format
   */
  std::string toString() const;         // YYYY-MM-DD

  /** @brief Convert to day number
   * @return Day number
   */
  int toNumber() const;

  /** @brief Convert to a @c time_t
   * @return @c time_t value of date
   */
  time_t toTime() const;

  /** @brief Today
   * @return Today's date
   *
   * Overridden by @c RSBACKUP_TODAY.
   */
  static Date today();

  /** @brief Now
   * @return The current time
   *
   * Overridden by @c RSBACKUP_TODAY.
   */
  static time_t now();

  /** @brief Calculate the length of a month in days
   * @param y Year
   * @param m Month (1-12)
   */
  static int monthLength(int y, int m);

private:
  /** @brief Test for a leap year
   * @return @c true iff @ref y is a leap year
   */
  inline bool isLeapYear() const {
    return isLeapYear(y);
  }

  /** @brief Test for a leap year
   * @param y Year
   * @return @c true iff @ref y is a leap year
   */
  static inline bool isLeapYear(int y) {
    return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
  }

  /** @brief Year
   *
   * This is the Gregorian year; unlike the C library it is not offset by
   * 1900.
   */
  int y;

  /** @brief Month
   *
   * 1 = January.
   */
  int m;

  /** @brief Day of month
   *
   * Starts from 1.
   */
  int d;

  /** @brief Day number at start of month
   *
   * The first of January is day 0.
   *
   * The values in this array are index by the month number start from 1, and
   * are cumulative.  So @c mday[1] is 0, @c mday[2] is 31, @c mday[3] is
   * 31+28, etc.  @c mday[0] doesn't mean anything.
   *
   * This array doesn't take leap years into account so its callers must
   * correct for that.
   */
  static const int mday[];
};

/** @brief Write a date string to a stream
 * @param os Output stream
 * @param d Date
 * @return @p os
 *
 * Formats @p d as if by @ref Date::toString() and writes it to @p os.
 */
inline std::ostream &operator<<(std::ostream &os, Date &d) {
  return os << d.toString();
}

#endif /* DATE_H */
