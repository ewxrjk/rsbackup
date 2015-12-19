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
#ifndef SELECTION_H
#define SELECTION_H

#include <string>
#include <vector>

class Conf;

/** @brief Represents a single selection
 *
 * A selection is a volume or collection of volumes and a positive or
 * negative sense (represented by @c true and @c false respectively).  The
 * list of selections in @ref Command::selections determines which volumes an
 * operation applies to.
 */
struct Selection {
  /** @brief Construct a selection
   * @param host_ Host or "*" for all hosts
   * @param volume_ Volume or "*" for all hosts
   * @param sense_ @c true for "+ and @c false for "-"
   *
   * A @p host_ of "*" but a @p volume_ not equal to "*" does not make sense
   * and will fail in @ref Conf::selectVolume.
   */
  Selection(const std::string &host_,
            const std::string &volume_,
            bool sense_ = true);

  /** @brief Sense of selection
   *
   * @c true for "+" and @c false for "-"
   */
  bool sense;

  /** @brief Host name or "*"
   *
   * "*" means all hosts.
   */
  std::string host;

  /** @brief Volume name or "*"
   *
   * "*" means all volumes.
   */
  std::string volume;

};

/** @brief Represents a list of selections */
class VolumeSelections {
public:

  /** @brief Add a selection to the list
   * @param selection Selection string from caller
   */
  void add(const std::string &selection);

  /** @brief Select volumes
   *
   * Invokes Conf::selectVolumes() according to the selected volumes.
   * If no voumes were selected, selects everyting.
   */
  void select(Conf &config) const;

  /** @brief Return the number of selections */
  size_t size() const { return selections.size(); }

  /** @brief Return the nth selection
   * @param n Index into selections array */
  const Selection &operator[](size_t n) const { return selections.at(n); }

  /** @brief Return an iterator pointing to the first selection
   * @return Iterator
   */
  std::vector<Selection>::const_iterator begin() const {
    return selections.begin();
  }

  /** @brief Return an iterator pointing after the last selection
   * @return Iterator
   */
  std::vector<Selection>::const_iterator end() const {
    return selections.end();
  }

private:
  /** @brief Selections */
  std::vector<Selection> selections;
};

#endif /* SELECTION_H */
