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
/** @file Color.h
 * @brief Representation and selection of colors
 *
 * The @ref Color class represents colors in RGB form.  No attempt is made to
 * formalize it in terms of color space - you get whatever interpretation Cairo
 * and/or your HTML or image rendering clients use.  It is used by the classes
 * of @ref Render.h and by the various color-setting configuration directives
 * (see @ref ColorDirective).
 *
 * The @ref ColorStrategy class and its subclasses provide a configurable means
 * of coloring a range of items different colors.  It is used by @ref
 * Conf::deviceColorStrategy.
 */

#include <string>
#include <map>
#include <vector>

/** @brief An RGB color
 *
 * Colors are accepted in three ways:
 * - as an RGB triple, with 0<=R,G,B<=1 (and this is the internal representation)
 * - as an HSV triple, with 0<=H<360 and 0<=S,V<=1
 * - as a 24-bit packed RGB integer, with 0<=R,G,B<=255
 */
struct Color {
  /** @brief Constructor
   * @param r Red component
   * @param g Green component
   * @param b Blue component
   *
   * All components have values in the closed interval [0,1].
   * For example, (1,0,0) represents red.
   */
  Color(double r, double g, double b):
    red(r), green(g), blue(b) {
  }

  /** @brief Constructor
   * @param rgb 24-bit red/green/blue color
   *
   * The top 8 bits are red, the middle green and the bottom blue.
   * For example, 0xFF0000 represents red.
   */
  Color(unsigned rgb = 0): red(component(rgb, 16)),
                           green(component(rgb, 8)),
                           blue(component(rgb, 0)) {
  }

  /** @brief Convert to integer form
   * @return 24-bit red/green/blue color value
   *
   * The top 8 bits are red, the middle green and the bottom blue.
   * For example, 0xFF0000 represents red.
   */
  operator unsigned() const {
    return pack(red, 16) + pack(green, 8) + pack(blue, 0);
  }

  /** @brief Red component
   *
   * 0 <= red <= 1
   */
  double red;

  /** @brief Green component
   *
   * 0 <= green <= 1
   */
  double green;

  /** @brief Blue component
   *
   * 0 <= blue <= 1
   */
  double blue;

  /** @brief Convert from HSV
   * @param h Hue, 0<=h<360
   * @param s Saturation, 0<=s<=1
   * @param v Value, 0<=v<=1
   * @return Color value
   *
   * @p h may actually be any value - it is reduced modulo 360.
   */
  static Color HSV(double h, double s, double v);

private:
  /** @brief Compute a component from integer form
   * @param n Integer form, 0 <= @p n <= 255
   * @param shift Right shift (0, 8 or 16)
   * @return Component value, 0 <= value <= 1
   */
  static double component(unsigned n, unsigned shift) {
    return (double)((n >> shift) & 255) / 255.0;
  }

  /** @brief Pack a component into integer form
   * @param c Component, 0 <= @p c <= 1
   * @param shift Left shift (0, 8 or 16)
   * @return Partial integer form
   */
  static unsigned pack(double c, unsigned shift) {
    return static_cast<unsigned>(255.0 * c + 0x0.ffffffffp-1) << shift;
  }
};

/** @brief Output a color
 * @param os Output stream
 * @param c Color
 *
 * Output @p c as a 6-digit hex value.
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

  /** @brief Create a color-picking strategy from configuration
   * @param name Name of strategy to use
   * @param params Strategy parameters
   * @param pos Index of first parameter
   *
   * The supported strategies are @c "equidistant-hue" (see @ref
   * EquidistantHue) and @c "equidistant-value" (see @ref EquidistantValue).
   *
   * The parameters correspond to the constructor arguments for the strategy
   * (generally with a bit of user-facing range-checking).
   */
  static ColorStrategy *create(const std::string &name,
                               std::vector<std::string> &params,
                               size_t pos = 0);

protected:
  /** @brief Name of this strategy */
  const char *name;
};

#endif /* COLOR_H */
