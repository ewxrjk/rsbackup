//-*-C++-*-
#ifndef DATE_H
#define DATE_H

#include <string>

// A (proleptic) Gregorian date.  Don't try years before 1CE.
class Date {
public:
  Date(): y(0), m(1), d(1) {}
  Date(const std::string &dateString);  // expects YYYY-MM-DD
  Date(int y_, int m_, int d_): y(y_), m(m_), d(d_) {}

  int operator-(const Date &that) const;

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
private:
  bool isLeapYear() const {
    return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
  }
  int y, m, d;
  static const int mday[];
};

#endif /* DATE_H */

