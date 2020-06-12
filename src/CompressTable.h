// Copyright © Richard Kettlewell.
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
#ifndef COMPRESSTABLE_H
#define COMPRESSTABLE_H
/** @file src/CompressTable.h
 * @brief A compressable table
 */

/** @brief A compressable table
 * @param Value Value type represented by table cells.
 *
 * Any two rows in the table are eligible for combination if they
 * differ only in one column.
 */
template <class Value> class Table {
public:
  /** @brief Type of a cell in the table */
  typedef std::set<Value> Cell;

  /** @brief Type of a row in the table */
  class Row {
  public:
    /** @brief The cells of the row */
    std::vector<Cell> cells;

    /** @brief Add a new cell to the row
     * @param v Value to add
     */
    void push_back(const Value &v) {
      Cell c;
      c.insert(v);
      cells.push_back(c);
    }

    /** @brief Return true if two rows can be merged
     * @param other Row to compare with
     */
    bool mergeable(const Row &other) const {
      if(cells.size() != other.cells.size())
        return false;
      size_t differences = 0;
      for(size_t i = 0; i < cells.size(); i++)
        if(cells[i] != other.cells[i])
          ++differences;
      return differences <= 1;
    }

    /** @brief Merge another row into this one
     * @param other Source of merge
     */
    void merge(const Row &other) {
      for(size_t i = 0; i < cells.size(); i++) {
        cells[i].insert(other.cells[i].begin(), other.cells[i].end());
      }
    }
  };

  /** @brief The rows of the table */
  std::vector<Row> rows;

  /** @brief Add a new row to the table */
  template <class Container> void push_back(Container &row) {
    Row r;
    for(auto &v: row) {
      r.push_back(v);
    }
    rows.push_back(r);
  }

  /** @brief Compress the table */
  void compress() {
    bool changed = false;
    do {
      changed = false;
      for(size_t i = 0; i + 1 < rows.size();) {
        if(rows[i].mergeable(rows[i + 1])) {
          rows[i].merge(rows[i + 1]);
          rows.erase(rows.begin() + i + 1);
          changed = true;
        } else
          i++;
      }
    } while(changed);
  }
};

#endif
