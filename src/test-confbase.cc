// Copyright © 2014 Richard Kettlewell.
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
#include "Errors.h"
#include "Conf.h"
#include <cassert>

void test_quote() {
  assert(ConfBase::quote("") == "\"\"");
  assert(ConfBase::quote("x") == "x");
  assert(ConfBase::quote("#") == "\"#\"");
  assert(ConfBase::quote(" ") == "\" \"");
  assert(ConfBase::quote("x y") == "\"x y\"");
  assert(ConfBase::quote("\\") == "\"\\\\\"");
  assert(ConfBase::quote("\"") == "\"\\\"\"");
}

void test_quote_vector() {
  std::vector<std::string> vs;
  vs.push_back("");
  vs.push_back("x");
  vs.push_back("#");
  vs.push_back(" ");
  vs.push_back("x y");
  vs.push_back("\\");
  vs.push_back("\"");
  std::string s = ConfBase::quote(vs);
  assert(s == "\"\" x \"#\" \" \" \"x y\" \"\\\\\" \"\\\"\"");
}

void test_indent() {
  assert(ConfBase::indent(0) == "");
  assert(ConfBase::indent(3) == "   ");
}

int main(void) {
  test_indent();
  test_quote();
  test_quote_vector();
  return 0;
}
