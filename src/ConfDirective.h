//-*-C++-*-
// Copyright © 2011, 2012, 2014, 2015 Richard Kettlewell.
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
#ifndef CONFDIRECTIVE_H
#define CONFDIRECTIVE_H
/** @file ConfDirective.h
 * @brief Configuration file parser support
 *
 * This file contains classes used during configuration parsing, including the
 * classes that define directives (@ref ConfDirective).
 */

/** @brief Context for configuration file parsing
 *
 * This class captures the state for a configuration file that we are reading.
 * If it includes further configuration files, there will be further
 * ConfContext objects.
 */
struct ConfContext {
  /** @brief Constructor
   *
   * @param conf_ Root configuration node
   */
  ConfContext(Conf *conf_):
    conf(conf_), context(conf_) {}

  /** @brief Root of configuration */
  Conf *conf;

  /** @brief Current configuration node
   *
   * Could be a @ref Conf, @ref Host or @ref Volume.
   */
  ConfBase *context;

  /** @brief Current host or null */
  Host *host = nullptr;

  /** @brief Current volume or null */
  Volume *volume = nullptr;

  /** @brief Parsed directive */
  std::vector<std::string> bits;

  /** @brief Containing filename */
  std::string path;

  /** @brief Line number */
  int line = -1;
};

class ConfDirective;

/** @brief Type of name-to-directive map */
typedef std::map<std::string, const ConfDirective *> directives_type;

/** @brief Base class for configuration file directives
 *
 * Each configuration file directive has a corresponding subclass and a single
 * instance.
 */
class ConfDirective {
public:
  /** @brief Constructor
   *
   * @param name_ Name of directive
   * @param min_ Minimum number of arguments
   * @param max_ Maximum number of arguments
   *
   * Directives are automatically registered in this constructor.
   */
  ConfDirective(const char *name_, int min_=0, int max_=INT_MAX);

  /** @brief Name of directive */
  const std::string name;

  /** @brief Check directive syntax
   * @param cc Context containing directive
   *
   * The base class implementation just checks the minimum and maximum number
   * of arguments.
   */
  virtual void check(const ConfContext &cc) const;

  /** @brief Get a boolean parameter
   * @param cc Context containing directive
   * @return @c true or @c false
   *
   * Use in ConfDirective::set implementations for boolean-sense directives.
   */
  bool get_boolean(const ConfContext &cc) const;

  /** @brief Set or extend a vector directive
   * @param cc Context containing directive
   * @param conf Configuration value to update
   */
  void extend(const ConfContext &cc, std::vector<std::string> &conf) const;

  /** @brief Act on a directive
   *  @param cc Context containing directive
   */
  virtual void set(ConfContext &cc) const = 0;

  /** @brief Find a directive
   * @param name Name of directive
   * @return Pointer to implementation or null pointer if not found
   */
  static const ConfDirective *find(const std::string &name);

private:
  /** @brief Minimum number of arguments */
  int min;

  /** @brief Maximum number of arguments */
  int max;

  /** @brief Map names to directives */
  static directives_type *directives;
};

/** @brief Base class for directives that can only appear in a host context */
class HostOnlyDirective: public ConfDirective {
public:
  /** @brief Constructor
   *
   * @param name_ Name of directive
   * @param min_ Minimum number of arguments
   * @param max_ Maximum number of arguments
   * @param inheritable_ If @c true, also allowed in volume context
   */
  HostOnlyDirective(const char *name_, int min_=0, int max_=INT_MAX,
                    bool inheritable_ = false):
    ConfDirective(name_, min_, max_),
    inheritable(inheritable_) {}
  void check(const ConfContext &cc) const override;

  /** @brief If @c true, also allowed in volume context */
  bool inheritable;
};

/** @brief Base class for directives that can only appear in a volume context */
class VolumeOnlyDirective: public ConfDirective {
public:
  /** @brief Constructor
   *
   * @param name_ Name of directive
   * @param min_ Minimum number of arguments
   * @param max_ Maximum number of arguments
   */
  VolumeOnlyDirective(const char *name_, int min_=0, int max_=INT_MAX):
    ConfDirective(name_, min_, max_) {}
  void check(const ConfContext &cc) const override;
};

/** @brief Base class for color-setting directives */
class ColorDirective: public ConfDirective {
public:
  /** @brief Constructor
   * @param name Name of directive
   */
  ColorDirective(const char *name): ConfDirective(name, 1, 4) {}

  void check(const ConfContext &cc) const override;

  void set(ConfContext &cc) const override;

  /** @brief Set a color
   * @param cc Configuration context
   * @param c Color to set
   */
  virtual void set(ConfContext &cc, const Color &c) const = 0;

private:
  /** @brief Parse and set an RGB color representation
   * @param cc Configuration context
   * @param n Index for first element
   */
  void set_rgb(ConfContext &cc, size_t n) const;

  /** @brief Parse and set an HSV color representation
   * @param cc Configuration context
   * @param n Index for first element
   */
  void set_hsv(ConfContext &cc, size_t n) const;

  /** @brief Parse and set a packed integer color representation
   * @param cc Configuration context
   * @param n Index for first element
   * @param radix Radix or 0 to pick as per strtol(3)
   */
  void set_packed(ConfContext &cc, size_t n, int radix = 0) const;

};

#endif /* CONFDIRECTIVE_H */
