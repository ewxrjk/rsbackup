#include <config.h>
#include "Document.h"
#include <cstdio>

// LinearContainer ------------------------------------------------------------

Document::LinearContainer::~LinearContainer() {
  for(size_t n = 0; n < nodes.size(); ++n)
    delete nodes[n];
}

// String ---------------------------------------------------------------------

Document::String::String(int n) {
  char buffer[64];
  snprintf(buffer, sizeof buffer, "%d", n);
  text = buffer;
}

// Table ----------------------------------------------------------------------

Document::Table::~Table() {
  for(size_t n = 0; n < cells.size(); ++n)
    delete cells[n];
}

int Document::Table::width() const {
  int w = 0;
  for(size_t n = 0; n < cells.size(); ++n) {
    const Cell *cell = cells[n];
    int ww = cell->x + cell->w;
    if(ww > w)
      w = ww;
  }
  return w;
}

int Document::Table::height() const {
  int h = 0;
  for(size_t n = 0; n < cells.size(); ++n) {
    const Cell *cell = cells[n];
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
  for(size_t n = 0; n < cells.size(); ++n) {
    Cell *cell = cells[n];
    if(x >= cell->x && x < cell->x + cell->w
       && y >= cell->y && y < cell->y + cell->h)
      return cell;
  }
  return NULL;
}
