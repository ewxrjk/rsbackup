#include <config.h>
#include "Date.h"
#include "Errors.h"
#include <cstdio>

const int Date::mday[] = {
  0,
  0,
  31,
  31 + 28,
  31 + 28 + 31,
  31 + 28 + 31 + 30,
  31 + 28 + 31 + 30 + 31,
  31 + 28 + 31 + 30 + 31 + 30,
  31 + 28 + 31 + 30 + 31 + 30 + 31,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
};


Date::Date(const std::string &dateString) {
  if(sscanf(dateString.c_str(), "%d-%d-%d", &y, &m, &d) != 3)
    throw std::runtime_error("invalid date string '" + dateString + "'"); // TODO exception class
}

std::string Date::toString() const {
  char buffer[64];
  sprintf(buffer, "%04d-%02d-%02d", y, m, d);
  return buffer;
}

int Date::toNumber() const {
  int dayno = 365 * y + y / 4 - y / 100 + y / 400;
  dayno += mday[m] + (m > 2 && isLeapYear() ? 1 : 0);
  dayno += d - 1;
  return dayno;
}

bool Date::operator<(const Date &that) const {
  int delta;
  if((delta = y - that.y)) return delta < 0;
  if((delta = m - that.m)) return delta < 0;
  if((delta = d - that.d)) return delta < 0;
  return false;
}

int Date::operator-(const Date &that) const {
  return toNumber() - that.toNumber();
}
