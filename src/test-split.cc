// Copyright © 2015 Richard Kettlewell.
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
#include "Utils.h"

int main(void) {
  std::vector<std::string> bits;

  bits.push_back("spong");
  split(bits, "");
  assert(bits.size() == 0);

  bits.push_back("spong");
  split(bits, "   ");
  assert(bits.size() == 0);

  bits.push_back("spong");
  split(bits, "# junk");
  assert(bits.size() == 0);

  bits.push_back("spong");
  split(bits, "word");
  assert(bits.size() == 1);
  assert(bits[0] == "word");

  bits.push_back("spong");
  split(bits, "  word  ");
  assert(bits.size() == 1);
  assert(bits[0] == "word");

  bits.push_back("spong");
  split(bits, "\tword\r");
  assert(bits.size() == 1);
  assert(bits[0] == "word");

  bits.push_back("spong");
  split(bits, "word second");
  assert(bits.size() == 2);
  assert(bits[0] == "word");
  assert(bits[1] == "second");

  bits.push_back("spong");
  split(bits, "word second # junk");
  assert(bits.size() == 2);
  assert(bits[0] == "word");
  assert(bits[1] == "second");

  bits.push_back("spong");
  split(bits, "\"quoted\"");
  assert(bits.size() == 1);
  assert(bits[0] == "quoted");

  bits.push_back("spong");
  split(bits, "\"quoted\"");
  assert(bits.size() == 1);
  assert(bits[0] == "quoted");

  bits.push_back("spong");
  split(bits, "\"with spaces\"");
  assert(bits.size() == 1);
  assert(bits[0] == "with spaces");

  bits.push_back("spong");
  split(bits, "\"\\\\\"");
  assert(bits.size() == 1);
  assert(bits[0] == "\\");

  bits.push_back("spong");
  split(bits, "\"\\\"\"");
  assert(bits.size() == 1);
  assert(bits[0] == "\"");

  bits.push_back("spong");
  split(bits, "\"\\xxx\"");
  assert(bits.size() == 1);
  assert(bits[0] == "xxx");

  try {
    split(bits, "\"unterminated");
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    split(bits, "\"unterminated\\");
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    split(bits, "\"unterminated\\\"");
    assert(0);
  } catch(SyntaxError &) {
  }

  try {
    split(bits, "\\");
    assert(0);
  } catch(SyntaxError &) {
  }

  return 0;
}
