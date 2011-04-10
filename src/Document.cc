#include <config.h>
#include "Document.h"
#include "Unicode.h"
#include "Errors.h"
#include <ostream>
#include <cstdio>

#include "Command.h"

// LinearContainer ------------------------------------------------------------

Document::LinearContainer::~LinearContainer() {
  for(size_t n = 0; n < nodes.size(); ++n)
    delete nodes[n];
}

// String ---------------------------------------------------------------------

Document::String::String(int n) {
  char buffer[64];
  sprintf(buffer, "%d", n);
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

void Document::Table::addCell(Cell *cell) {
  addCell(cell, x, y);
  while(occupied(x, y))
    ++x;
}

void Document::Table::newRow() {
  x = 0;
  ++y;
  while(occupied(x, y))
    ++x;
}

void Document::Table::addCell(Cell *cell, int x, int y) {
  cell->x = x;
  cell->y = y;
  cells.push_back(cell);
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

// HTML support ---------------------------------------------------------------

// TODO style support

void Document::quoteHtml(std::ostream &os,
                         const std::string &s) {
  // We need the string in UTF-32 in order to quote it correctly
  std::wstring u;
  toUnicode(u, s);
  // SGML-quote anything that might be interpreted as a delimiter, and anything
  // outside of ASCII
  for(size_t n = 0; n < u.size(); ++n) {
    int w = u[n];
    switch(w) {
    default:
      if(w >= 127) {
      case '&':
      case '<':
      case '"':
      case '\'':
        os << "&" << w << ";";
        break;
      }
      else
        os << (char)w;
    }
  }
}

void Document::LinearContainer::renderHtmlContents(std::ostream &os) const {
  for(size_t n = 0; n < nodes.size(); ++n)
    nodes[n]->renderHtml(os);
}

void Document::String::renderHtml(std::ostream &os) const {
  Document::quoteHtml(os, text);
}

void Document::Paragraph::renderHtml(std::ostream &os) const {
  os << "<p>";
  for(size_t n = 0; n < nodes.size(); ++n)
    nodes[n]->renderHtml(os);
  os << "</p>\n";
}

void Document::Heading::renderHtml(std::ostream &os) const {
  if(level > 6)
    throw std::runtime_error("heading level too high");
  os << "<h" << level << ">";
  renderHtmlContents(os);
  os << "</h" << level << ">\n";
}

void Document::Cell::renderHtml(std::ostream &os) const {
  const char *const element = heading ? "th" : "td";
  if(w > 1 && h > 1)
    os << "<" << element << " colspan=" << w << " rowspan=" << h << ">";
  else if(w > 1)
    os << "<" << element << " colspan=" << w << ">";
  else if(h > 1)
    os << "<" << element << " rowspan=" << h << ">";
  else
    os << "<" << element << ">";
  renderHtmlContents(os);
  os << "</td>\n";
}

void Document::Table::renderHtml(std::ostream &os) const {
  os << "<table border=1>\n";
  const int w = width(), h = height();
  for(int row = 0; row < h; ++row) {
    os << "<tr>\n";
    for(int col = 0; col < w;) {
      int skip = 0;
      for(size_t n = 0; n < cells.size(); ++n) {
        const Cell *cell = cells[n];
        if(cell->y == row && cell->x == col) {
          cell->renderHtml(os);
          skip = cell->w;
          break;
        }
      }
      if(!skip) {
        if(!occupied(col, row))
          os << "<td></td>\n";
        skip = 1;
      }
      col += skip;
    }
    os << "</tr>\n";
  }
  os << "</table>\n";
}

void Document::RootContainer::renderHtml(std::ostream &os) const {
  os << "<body>\n";
  renderHtmlContents(os);
  os << "</body>\n";
}

void Document::renderHtml(std::ostream &os) const {
  // TODO stylesheet?
  os << "<html>\n";
  os << "<head>\n";
  os << "<title>";
  quoteHtml(os, title);
  os << "</title>\n";
  os << "</head>\n";
  content.renderHtml(os);
  os << "</html>\n";
}
