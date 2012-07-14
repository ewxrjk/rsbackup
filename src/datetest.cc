#include <config.h>
#include "Date.h"
#include <cassert>
#include <cstdio>
#include <sstream>

int main() {
  Date d("1997-03-02");
  assert(d.toString() == "1997-03-02");
  assert(d.toNumber() == (1997*365+(1997/4)-(1997/100)+(1997/400)
                          +31+28
                          +2-1));
  std::stringstream s;
  s << d;
  assert(s.str() == "1997-03-02");
  Date e("1998-03-02");
  assert(e.toString() == "1998-03-02");
  assert(e.toNumber() == (1998*365+(1997/4)-(1997/100)+(1997/400)
                          +31+28
                          +2-1));
  assert(e - d == 365);
  Date t = Date::today();
  printf("today = %s = %d\n", t.toString().c_str(), t.toNumber());
  return 0;
}
