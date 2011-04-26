#include <config.h>
#include "Document.h"
#include "Unicode.h"
#include "Errors.h"
#include <ostream>
#include <cstdio>
#include <cstdarg>

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

// HTML support ---------------------------------------------------------------

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

void Document::Node::renderHtmlOpenTag(std::ostream &os,
                                       const char *name, ...) const {
  va_list ap;
  os << '<' << name;
  if(style.size())
    os << " class=" << style;
  char b[20];
  if(fgcolor != -1) {
    sprintf(b, "#%06x", fgcolor);
    os << " color=\"" << b << "\"";
  }
  if(bgcolor != -1) {
    sprintf(b, "#%06x", bgcolor);
    os << " bgcolor=\"" << b << "\"";
  }
  va_start(ap, name);
  const char *attributeName, *attributeValue;
  while((attributeName = va_arg(ap, const char *))) {
    attributeValue = va_arg(ap, const char *);
    os << " " << attributeName << "=\"";
    quoteHtml(os, attributeValue);
    os << "\"";
  }
  os << '>';
}

void Document::Node::renderHtmlCloseTag(std::ostream &os, const char *name,
                                        bool newline) const {
  os << "</" << name << ">";
  if(newline)
    os << '\n';
}

void Document::LinearContainer::renderHtmlContents(std::ostream &os) const {
  for(size_t n = 0; n < nodes.size(); ++n)
    nodes[n]->renderHtml(os);
}

void Document::String::renderHtml(std::ostream &os) const {
  Document::quoteHtml(os, text);
}

void Document::Paragraph::renderHtml(std::ostream &os) const {
  renderHtmlOpenTag(os, "p", (char *)0);
  renderHtmlContents(os);
  renderHtmlCloseTag(os, "p");
}

void Document::Verbatim::renderHtml(std::ostream &os) const {
  renderHtmlOpenTag(os, "pre", (char *)0);
  renderHtmlContents(os);
  renderHtmlCloseTag(os, "pre");
}

void Document::List::renderHtml(std::ostream &os) const {
  switch(type) {
  case OrderedList: renderHtmlOpenTag(os, "ol", (char *)0); break;
  case UnorderedList: renderHtmlOpenTag(os, "ul", (char *)0); break;
  }
  renderHtmlContents(os);
  switch(type) {
  case OrderedList: renderHtmlCloseTag(os, "ol"); break;
  case UnorderedList: renderHtmlCloseTag(os, "ul"); break;
  }
}

void Document::ListEntry::renderHtml(std::ostream &os) const {
  renderHtmlOpenTag(os, "li", (char *)0);
  renderHtmlContents(os);
  renderHtmlCloseTag(os, "li");
}

void Document::Heading::renderHtml(std::ostream &os) const {
  if(level > 6)
    throw std::runtime_error("heading level too high");
  char tag[10];
  sprintf(tag, "h%d", level);
  renderHtmlOpenTag(os, tag, (char *)0);
  renderHtmlContents(os);
  renderHtmlCloseTag(os, tag);
}

void Document::Cell::renderHtml(std::ostream &os) const {
  const char *const tag = heading ? "th" : "td";
  char ws[20], hs[20];
  sprintf(ws, "%d", w);
  sprintf(hs, "%d", h);
  if(w > 1 && h > 1)
    renderHtmlOpenTag(os, tag, "colspan", ws, "rowspan", hs, (char *)0);
  else if(w > 1)
    renderHtmlOpenTag(os, tag, "colspan", ws, (char *)0);
  else if(h > 1)
    renderHtmlOpenTag(os, tag, "rowspan", hs, (char *)0);
  else
    renderHtmlOpenTag(os, tag, (char *)0);
  renderHtmlContents(os);
  renderHtmlCloseTag(os, tag);
}

void Document::Table::renderHtml(std::ostream &os) const {
  renderHtmlOpenTag(os, "table", (char *)0);
  const int w = width(), h = height();
  for(int row = 0; row < h; ++row) {
    renderHtmlOpenTag(os, "tr", (char *)0);
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
        if(!occupied(col, row)) {
          renderHtmlOpenTag(os, "td", (char *)0);
          renderHtmlCloseTag(os, "td");
        }
        skip = 1;
      }
      col += skip;
    }
    renderHtmlCloseTag(os, "tr");
  }
  renderHtmlCloseTag(os, "table");
}

void Document::RootContainer::renderHtml(std::ostream &os) const {
  renderHtmlOpenTag(os, "body", (char *)0);
  renderHtmlContents(os);
  renderHtmlCloseTag(os, "body");
}

void Document::renderHtml(std::ostream &os) const {
  os << "<html>\n";
  os << "<head>\n";
  os << "<title>";
  quoteHtml(os, title);
  os << "</title>\n";
  if(htmlStyleSheet.size()) {
    os << "<style type=\"text/css\">\n";
    os << htmlStyleSheet;
    os << "</style>\n";
  }
  os << "</head>\n";
  content.renderHtml(os);
  os << "</html>\n";
}
