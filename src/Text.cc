// Copyright © 2011, 2015 Richard Kettlewell.
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
#include <sstream>
#include <cstdio>
#include <cstdarg>

void Document::LinearContainer::renderTextContents(
    std::ostream &os, RenderDocumentContext *rc) const {
  for(auto &node: nodes)
    node->renderText(os, rc);
}

void Document::String::renderText(std::ostream &os,
                                  RenderDocumentContext *) const {
  os << text;
}

void Document::LinearContainer::renderText(std::ostream &os,
                                           RenderDocumentContext *rc) const {
  renderTextContents(os, rc);
}

void Document::wordWrapText(std::ostream &os, const std::string &s,
                            size_t width, size_t indent, bool indentFirst) {
  size_t x = 0;
  std::string::size_type pos = 0;
  bool first = true;
  while(pos < s.size()) {
    // Skip whitespace
    if(isspace(s[pos])) {
      ++pos;
      continue;
    }
    // Find the length of this word
    std::string::size_type len = 0;
    while(pos + len < s.size() && !isspace(s[pos + len]))
      ++len;
    if(x) {
      if(x + len + 1 <= width) {
        os << ' ';
        ++x;
      } else {
        os << '\n';
        x = 0;
      }
    } else {
      if(indentFirst || !first)
        for(size_t i = 0; i < indent; ++i)
          os << ' ';
    }
    os.write(s.data() + pos, len);
    x += len;
    pos += len;
  }
  if(x)
    os << '\n';
}

void Document::Paragraph::renderText(std::ostream &os,
                                     RenderDocumentContext *rc) const {
  // Convert to a single string
  std::stringstream ss;
  renderTextContents(ss, rc);
  wordWrapText(os, ss.str(), rc ? rc->width : DEFAULT_TEXT_WIDTH);
  os << '\n';
}

void Document::Verbatim::renderText(std::ostream &os,
                                    RenderDocumentContext *rc) const {
  renderTextContents(os, rc);
  os << '\n';
}

void Document::List::renderText(std::ostream &os,
                                RenderDocumentContext *rc) const {
  const size_t width = rc ? rc->width : DEFAULT_TEXT_WIDTH;
  for(size_t n = 0; n < nodes.size(); ++n) {
    char prefix[64];
    std::stringstream ss;
    nodes[n]->renderText(ss, rc);
    switch(type) {
    case OrderedList:
      snprintf(prefix, sizeof prefix, " %zu. ", n + 1);
      os << prefix;
      break;
    case UnorderedList: strcpy(prefix, " * "); break;
    }
    os << prefix;
    wordWrapText(os, ss.str(), width - strlen(prefix), strlen(prefix), false);
  }
  os << '\n';
}

void Document::ListEntry::renderText(std::ostream &os,
                                     RenderDocumentContext *rc) const {
  renderTextContents(os, rc);
}

void Document::Heading::renderText(std::ostream &os,
                                   RenderDocumentContext *rc) const {
  if(level > 6)
    throw std::runtime_error("heading level too high");
  switch(level) {
  case 1:
    os << "==== ";
    renderTextContents(os, rc);
    os << " ====";
    os << '\n';
    break;
  case 2:
    os << "=== ";
    renderTextContents(os, rc);
    os << " ===";
    os << '\n';
    break;
  case 3:
    os << "== ";
    renderTextContents(os, rc);
    os << " ==";
    os << '\n';
    break;
  case 4:
  case 5:
  case 6:
    os << "* ";
    renderTextContents(os, rc);
    os << '\n';
    break;
  }
  os << '\n';
}

void Document::Cell::renderText(std::ostream &os,
                                RenderDocumentContext *rc) const {
  renderTextContents(os, rc);
}

void Document::Table::renderText(std::ostream &os,
                                 RenderDocumentContext *rc) const {
  // First pass: compute column widths based on single-column cells
  std::vector<size_t> columnWidths;
  for(int xpos = 0; xpos < width; ++xpos) {
    size_t columnWidth = 0;
    for(int ypos = 0; ypos < height; ++ypos) {
      const Cell *cell = findOverlappingCell(xpos, ypos);
      // We only consider single-width cells
      if(cell && cell->getWidth() == 1) {
        std::stringstream ss;
        cell->renderText(ss, rc);
        size_t w = ss.str().size();
        if(w > columnWidth)
          columnWidth = w;
      }
    }
    columnWidths.push_back(columnWidth);
  }
  // Second pass: add extra space to accomodate multiple-column cells
  for(int xpos = 0; xpos < width; ++xpos) {
    for(int ypos = 0; ypos < height; ++ypos) {
      const Cell *cell = findOverlappingCell(xpos, ypos);
      if(cell && cell->getX() == xpos && cell->getWidth() > 1) {
        std::stringstream ss;
        cell->renderText(ss, rc);
        // Determine the space available
        size_t availableWidth = 3 * (cell->getWidth() - 1);
        for(int i = 0; i < cell->getWidth(); ++i)
          availableWidth += columnWidths[xpos + i];
        if(ss.str().size() <= availableWidth)
          continue;
        // If there's a shortage of space, bump up the first column
        columnWidths[xpos] += ss.str().size() - availableWidth;
      }
    }
  }
  // We lay out as | <data> | <data> | ... |
  // Third pass: render the table
  for(int ypos = 0; ypos < height; ++ypos) {
    for(int xpos = 0; xpos < width; ++xpos) {
      const Cell *cell = findOverlappingCell(xpos, ypos);
      if(cell) {
        if(cell->getY() == ypos) {
          if(cell->getX() == xpos) {
            // First row/column of the cell
            // Determine the space available
            size_t availableWidth = 3 * (cell->getWidth() - 1);
            for(int i = 0; i < cell->getWidth(); ++i)
              availableWidth += columnWidths[xpos + i];
            if(xpos)
              os << ' ';
            os << "| ";
            std::stringstream ss;
            cell->renderText(ss, rc);
            size_t left = availableWidth - ss.str().size();
            if(cell->getHeader()) {
              for(size_t i = 0; i < left / 2; ++i)
                os << ' ';
              left -= left / 2;
              os << ss.str();
            } else
              os << ss.str();
            for(size_t i = 0; i < left; ++i)
              os << ' ';
          } else {
            // 2nd or subsequent column of the cell
          }
        } else {
          // 2nd or subsequent row of the cell
          if(xpos)
            os << ' ';
          os << "| ";
          for(size_t i = 0; i < columnWidths[xpos]; ++i)
            os << ' ';
        }
      } else {
        // Unfilled cell
        if(xpos)
          os << ' ';
        os << "| ";
        for(size_t i = 0; i < columnWidths[xpos]; ++i)
          os << ' ';
      }
    }
    os << '|';
    os << '\n';
  }
  os << '\n';
}

void Document::Image::renderText(std::ostream &,
                                 RenderDocumentContext *) const {}

void Document::RootContainer::renderText(std::ostream &os,
                                         RenderDocumentContext *rc) const {
  renderTextContents(os, rc);
}

void Document::renderText(std::ostream &os, RenderDocumentContext *rc) const {
  content.renderText(os, rc);
}
