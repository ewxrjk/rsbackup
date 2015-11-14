// Copyright © 2011, 2012, 2015 Richard Kettlewell.
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
#include "Document.h"
#include "Utils.h"
#include <cstdio>

// LinearContainer ------------------------------------------------------------

Document::LinearContainer::~LinearContainer() {
  deleteAll(nodes);
}

// String ---------------------------------------------------------------------

Document::String::String(int n) {
  char buffer[64];
  snprintf(buffer, sizeof buffer, "%d", n);
  text = buffer;
}

// Table ----------------------------------------------------------------------

Document::Table::~Table() {
  deleteAll(cells);
}

int Document::Table::width() const {
  int w = 0;
  for(const Cell *cell: cells) {
    int ww = cell->x + cell->w;
    if(ww > w)
      w = ww;
  }
  return w;
}

int Document::Table::height() const {
  int h = 0;
  for(const Cell *cell: cells) {
    int hh = cell->y + cell->h;
    if(hh > h)
      h = hh;
  }
  return h;
}

Document::Cell *Document::Table::addCell(Cell *cell) {
  addCell(cell, x, y);
  while(occupied(x, y))
    ++x;
  return cell;
}

void Document::Table::newRow() {
  x = 0;
  ++y;
  while(occupied(x, y))
    ++x;
}

Document::Cell *Document::Table::addCell(Cell *cell, int x, int y) {
  cell->x = x;
  cell->y = y;
  cells.push_back(cell);
  return cell;
}

Document::Cell *Document::Table::occupied(int x, int y) const {
  for(Cell *cell: cells) {
    if(x >= cell->x && x < cell->x + cell->w
       && y >= cell->y && y < cell->y + cell->h)
      return cell;
  }
  return nullptr;
}

