// Copyright © 2011, 2012, 2015, 2018 Richard Kettlewell.
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
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <cstdio>
#include "Document.h"
#include "Utils.h"

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
  for(auto it: cellMap) {
    int x = it.first.first, y = it.first.second;
    Cell *cell = it.second;
    if(cell->x == x && cell->y == y)
      delete cell;
  }
  cellMap.clear();
}

Document::Cell *Document::Table::addCell(Cell *cell) {
  addCell(cell, x, y);
  while(findOverlappingCell(x, y))
    ++x;
  return cell;
}

void Document::Table::newRow() {
  x = 0;
  ++y;
  while(findOverlappingCell(x, y))
    ++x;
}

Document::Cell *Document::Table::addCell(Cell *cell, int x, int y) {
  cell->x = x;
  cell->y = y;
  // Update table occupancy
  for(int xpos = x; xpos < x + cell->w; xpos++)
    for(int ypos = y; ypos < y + cell->h; ypos++)
      cellMap[std::pair<int, int>(xpos, ypos)] = cell;
  // Update table size tracking
  width = std::max(width, cell->x + cell->w);
  height = std::max(height, cell->y + cell->h);
  return cell;
}

const Document::Cell *Document::Table::findOverlappingCell(int x, int y) const {
  auto it = cellMap.find(std::pair<int, int>(x, y));
  if(it != cellMap.end())
    return it->second;
  else
    return nullptr;
}

const Document::Cell *Document::Table::findRootedCell(int x, int y) const {
  const Cell *cell = findOverlappingCell(x, y);
  if(cell && cell->x == x && cell->y == y)
    return cell;
  else
    return nullptr;
}

// Image

std::string Document::Image::ident() const {
  static boost::uuids::string_generator sg;
  static boost::uuids::uuid ns = sg("1a90a5fb-9558-44d0-a9a9-9955c0ed359f");
  static boost::uuids::name_generator cg(ns);

  boost::uuids::uuid u = cg(content.data(), content.size());
  return boost::uuids::to_string(u) + "@" CID_DOMAIN;
}
