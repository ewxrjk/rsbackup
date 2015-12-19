// -*-C++-*-
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
#ifndef COLOR_H
#define COLOR_H

#include <string>
#include <map>
#include <vector>

/** @brief An RGB color */
struct Color {
  /** @brief Constructor
   * @param r Red component
   * @param g Green component
   * @param b Blue component
   *
   * All components have values in the closed interval [0,1].
   */
  Color(double r, double g, double b):
    red(r), green(g), blue(b) {
  }

  /** @brief Constructor
   * @param rgb 24-bit red/green/blue color
   */
  Color(unsigned rgb = 0): red(component(rgb, 16)),
                           green(component(rgb, 8)),
                           blue(component(rgb, 0)) {
  }

  /** @brief Convert to integer form
   * @return 24-bit red/green/blue color value
   */
  operator unsigned() const {
    return pack(red, 16) + pack(green, 8) + pack(blue, 0);
  }

  /** @brief Red component */
  double red;

  /** @brief Green component */
  double green;

  /** @brief Blue component */
  double blue;

  /** @brief Convert from HSV
   * @param h Hue, 0<=h<360
   * @param s Saturation, 0<=s<=1
   * @param v Value, 0<=v<=1
   * @return Color value
   */
  static Color HSV(double h, double s, double v);

private:
  /** @brief Compute a component from integer form
   * @param n Integer form
   * @param shift Right shift
   * @return Component value
   */
  static double component(unsigned n, unsigned shift) {
    return (double)((n >> shift) & 255) / 255.0;
  }

  /** @brief Pack a component into integer form
   * @param c Component
   * @param shift Left shift
   * @return Partial integer form
   */
  static unsigned pack(double c, unsigned shift) {
    return static_cast<unsigned>(255.0 * c) << shift;
  }
};

/** @brief Output a color
 * @param os Output stream
 * @param c Color
 *
 * Output @p c as a 6-digit hex valuee.
 */
std::ostream &operator<<(std::ostream &os, const Color &c);

/** @brief A strategy for picking a small set of distinct colors */
class ColorStrategy {
public:
  /** @brief Constructor
   * @param name Name of this strategy
   */
  ColorStrategy(const char *name);

  /* @brief Destructor */
  virtual ~ColorStrategy() = default;

  /** @brief Get a color
   * @param n Item number, 0 <= @p n < @p items
   * @param items Total number of items
   * @return Selected color
   */
  virtual Color get(unsigned n, unsigned items) const = 0;

  /** @brief Get the description of this strategy */
  virtual std::string description() const;

  static ColorStrategy *create(const std::string &name,
                               std::vector<std::string> &params,
                               size_t pos = 0);

protected:
  /** @brief Name of this strategy */
  const char *name;
};

#endif /* COLOR_H */
