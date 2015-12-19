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
#include "Utils.h"
#include "Errors.h"
#include <cassert>
#include <sstream>
#include <cmath>

ColorStrategy::ColorStrategy(const char *name): name(name) {
}

std::string ColorStrategy::description() const {
  return name;
}

/** @brief A color strategy that maximizes distance between hues */
class EquidistantHue: public ColorStrategy {
public:
  /** @brief Constructor
   * @param h Starting hue
   */
  EquidistantHue(double h, double s, double v):
    ColorStrategy("equidistant-hue"),
    hue(fmod(h, 360)),
    saturation(s),
    value(v) {
  }

  Color get(unsigned n, unsigned items) const override {
    double h = hue + 360.0 * n / items;
    return Color::HSV(h, saturation, value);
  }

  std::string description() const override {
    std::stringstream ss;
    ss << name << ' ' << hue << ' ' << saturation << ' ' << value;
    return ss.str();
  }

  double hue;
  double saturation;
  double value;
};

/** @brief A color strategy that maximizes distance between values */
class EquidistantValue: public ColorStrategy {
public:
  /** @brief Constructor
   * @param h Base hue
   * @param s Saturation
   * @param minv Minimum value
   * @param maxv Maximum value
   */
  EquidistantValue(double h, double s, double minv = 0, double maxv = 1):
    ColorStrategy("equidistant-value"),
    hue(fmod(h, 360)),
    saturation(s),
    minvalue(minv),
    maxvalue(maxv) {
  }

  Color get(unsigned n, unsigned items) const override {
    double value = minvalue
      + static_cast<double>(n) / (items-1) * (maxvalue - minvalue);
    return Color::HSV(hue, saturation, value);
  }

  std::string description() const override {
    std::stringstream ss;
    ss << name << ' ' << hue << ' ' << saturation;
    if(minvalue != 0 || maxvalue != 1)
      ss << ' ' << minvalue << ' ' << maxvalue;
    return ss.str();
  }

  double hue;
  double saturation;
  double minvalue;
  double maxvalue;
};

ColorStrategy *ColorStrategy::create(const std::string &name,
                                     std::vector<std::string> &params,
                                     size_t pos) {
  if(name == "equidistant-hue") {
    double h = 0, s = 1, v = 1;
    if(pos < params.size()) h = parseFloat(params[pos++]);
    if(pos < params.size()) s = parseFloat(params[pos++], 0, 1);
    if(pos < params.size()) v = parseFloat(params[pos++], 0, 1);
    if(pos < params.size())
      throw SyntaxError("too many parameters for color strategy '"
                        + name + "'");
    return new EquidistantHue(h, s, v);
  }
  if(name == "equidistant-value") {
    double h = 0, s = 1, minv = 0, maxv = 1;
    if(pos < params.size()) h = parseFloat(params[pos++]);
    if(pos < params.size()) s = parseFloat(params[pos++], 0, 1);
    if(pos < params.size()) minv = parseFloat(params[pos++], 0, 1);
    if(pos < params.size()) maxv = parseFloat(params[pos++], 0, 1);
    if(minv >= maxv)
      throw SyntaxError("inconsistent parameters for color strategy '"
                        + name + "'");
    if(pos < params.size())
      throw SyntaxError("too many parameters for color strategy '"
                        + name + "'");
    return new EquidistantValue(h, s, minv, maxv);
  }
  throw SyntaxError("unrecognized color strategy '" + name + "'");
}
