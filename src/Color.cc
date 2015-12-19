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
#include "Color.h"
#include <cmath>
#include <iomanip>
#include <ostream>

Color Color::HSV(double h, double s, double v) {
  // https://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB
  h = fmod(h, 360);
  double c = v * s;
  double hp = h / 60;
  double x = c * (1 - fabs(fmod(hp, 2) - 1));
  double r1, g1, b1;
  if(hp < 1) { r1 = c; g1 = x; b1 = 0; }
  else if(hp < 2) { r1 = x; g1 = c; b1 = 0; }
  else if(hp < 3) { r1 = 0; g1 = c; b1 = x; }
  else if(hp < 4) { r1 = 0; g1 = x; b1 = c; }
  else if(hp < 5) { r1 = x; g1 = 0; b1 = c; }
  else { r1 = c; g1 = 0; b1 = x; }
  double m = v - c;
  return Color(r1+m, g1+m, b1+m);
}

std::ostream &operator<<(std::ostream &os, const Color &c) {
  os << std::hex << std::setw(6) << std::setfill('0')
     << static_cast<unsigned>(c)
     << std::dec;
  return os;
}
