// Copyright © 2011, 2014, 2015, 2018 Richard Kettlewell.
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
#include "Errors.h"
#include <ostream>
#include <cstdio>
#include <cstdarg>
#include <sstream>

// HTML support ---------------------------------------------------------------

void Document::quoteHtml(std::ostream &os, const std::string &s) {
  // We need the string in UTF-32 in order to quote it correctly
  std::u32string u;
  toUnicode(u, s);
  // SGML-quote anything that might be interpreted as a delimiter, and anything
  // outside of ASCII
  for(auto w: u) {
    switch(w) {
    default:
      if(w >= 127) {
      case '&':
      case '<':
      case '"':
      case '\'': os << "&#" << w << ";"; break;
      } else
        os << (char)w;
    }
  }
}

void Document::Node::renderHtmlOpenTag(std::ostream &os, const char *name,
                                       ...) const {
  va_list ap;
  os << '<' << name;
  if(style.size())
    os << " class=" << style;
  char buffer[64];
  if(fgcolor != -1) {
    snprintf(buffer, sizeof buffer, "#%06x", fgcolor);
    os << " color=\"" << buffer << "\"";
  }
  if(bgcolor != -1) {
    snprintf(buffer, sizeof buffer, "#%06x", bgcolor);
    os << " bgcolor=\"" << buffer << "\"";
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
  va_end(ap);
}

void Document::Node::renderHtmlCloseTag(std::ostream &os, const char *name,
                                        bool newline) const {
  os << "</" << name << ">";
  if(newline)
    os << '\n';
}

void Document::LinearContainer::renderHtmlContents(std::ostream &os,
                                                   Attachments *as) const {
  for(Node *node: nodes)
    node->renderHtml(os, as);
}

void Document::String::renderHtml(std::ostream &os, Attachments *) const {
  Document::quoteHtml(os, text);
}

void Document::LinearContainer::renderHtml(std::ostream &os,
                                           Attachments *as) const {
  renderHtmlOpenTag(os, "div", (char *)nullptr);
  renderHtmlContents(os, as);
  renderHtmlCloseTag(os, "div");
}

void Document::Paragraph::renderHtml(std::ostream &os, Attachments *as) const {
  renderHtmlOpenTag(os, "p", (char *)nullptr);
  renderHtmlContents(os, as);
  renderHtmlCloseTag(os, "p");
}

void Document::Verbatim::renderHtml(std::ostream &os, Attachments *as) const {
  renderHtmlOpenTag(os, "pre", (char *)nullptr);
  renderHtmlContents(os, as);
  renderHtmlCloseTag(os, "pre");
}

void Document::List::renderHtml(std::ostream &os, Attachments *as) const {
  switch(type) {
  case OrderedList: renderHtmlOpenTag(os, "ol", (char *)nullptr); break;
  case UnorderedList: renderHtmlOpenTag(os, "ul", (char *)nullptr); break;
  }
  renderHtmlContents(os, as);
  switch(type) {
  case OrderedList: renderHtmlCloseTag(os, "ol"); break;
  case UnorderedList: renderHtmlCloseTag(os, "ul"); break;
  }
}

void Document::ListEntry::renderHtml(std::ostream &os, Attachments *as) const {
  renderHtmlOpenTag(os, "li", (char *)nullptr);
  renderHtmlContents(os, as);
  renderHtmlCloseTag(os, "li");
}

void Document::Heading::renderHtml(std::ostream &os, Attachments *as) const {
  if(level > 6)
    throw std::runtime_error("heading level too high");
  char tag[64];
  snprintf(tag, sizeof tag, "h%d", level);
  renderHtmlOpenTag(os, tag, (char *)nullptr);
  renderHtmlContents(os, as);
  renderHtmlCloseTag(os, tag);
}

void Document::Cell::renderHtml(std::ostream &os, Attachments *as) const {
  const char *const tag = heading ? "th" : "td";
  char ws[64], hs[64];
  snprintf(ws, sizeof ws, "%d", w);
  snprintf(hs, sizeof hs, "%d", h);
  if(w > 1 && h > 1)
    renderHtmlOpenTag(os, tag, "colspan", ws, "rowspan", hs, (char *)nullptr);
  else if(w > 1)
    renderHtmlOpenTag(os, tag, "colspan", ws, (char *)nullptr);
  else if(h > 1)
    renderHtmlOpenTag(os, tag, "rowspan", hs, (char *)nullptr);
  else
    renderHtmlOpenTag(os, tag, (char *)nullptr);
  renderHtmlContents(os, as);
  renderHtmlCloseTag(os, tag);
}

void Document::Table::renderHtml(std::ostream &os, Attachments *as) const {
  renderHtmlOpenTag(os, "table", (char *)nullptr);
  const int w = width(), h = height();
  for(int row = 0; row < h; ++row) {
    renderHtmlOpenTag(os, "tr", (char *)nullptr);
    bool heading = false;
    for(int col = 0; col < w;) {
      int skip = 0;
      for(const Cell *cell: cells) {
        if(cell->y == row && cell->x == col) {
          heading |= cell->heading;
          cell->renderHtml(os, as);
          skip = cell->w;
          break;
        }
      }
      if(!skip) {
        if(!occupied(col, row)) {
          const char *tag = heading ? "th" : "td";
          renderHtmlOpenTag(os, tag, (char *)nullptr);
          renderHtmlCloseTag(os, tag);
        }
        skip = 1;
      }
      col += skip;
    }
    renderHtmlCloseTag(os, "tr");
  }
  renderHtmlCloseTag(os, "table");
}

void Document::Image::renderHtml(std::ostream &os, Attachments *as) const {
  std::string url;
  if(as) {
    as->images.push_back(this);
    url = "cid:" + ident();
  } else {
    std::stringstream ss;
    ss << "data:" << type << ";base64,";
    write_base64(ss, content);
    url = ss.str();
  }
  renderHtmlOpenTag(os, "p", (char *)nullptr);
  renderHtmlOpenTag(os, "img", "src", url.c_str(), (char *)nullptr);
  renderHtmlCloseTag(os, "p");
}

void Document::RootContainer::renderHtml(std::ostream &os,
                                         Attachments *as) const {
  renderHtmlOpenTag(os, "body", (char *)nullptr);
  renderHtmlContents(os, as);
  renderHtmlCloseTag(os, "body");
}

void Document::renderHtml(std::ostream &os, Attachments *as) const {
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
  content.renderHtml(os, as);
  os << "</html>\n";
}
