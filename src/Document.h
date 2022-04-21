//-*-C++-*-
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
#ifndef DOCUMENT_H
#define DOCUMENT_H
/** @file Document.h
 * @brief %Document construction and rendering
 */

#include <string>
#include <vector>
#include <map>

#include "Defaults.h"

class RenderDocumentContext;

/** @brief Structured document class
 *
 * Can be rendered to text or HTML.
 */
class Document {
public:
  /** @brief Base class for document node types */
  struct Node {
    /** @brief Destructor */
    virtual ~Node() = default;

    Node() = default;

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    /** @brief Style name */
    std::string style;

    /** @brief Foreground color or -1 */
    int fgcolor = -1;

    /** @brief Background color or -1 */
    int bgcolor = -1;

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    virtual void renderHtml(std::ostream &os,
                            RenderDocumentContext *rc) const = 0;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    virtual void renderText(std::ostream &os,
                            RenderDocumentContext *rc) const = 0;

    /** @brief Render an open tag
     * @param os Output
     * @param name Element name
     * @param ... Attribute name/value pairs
     *
     * Attribute names and values are const char * and are terminated by a null
     * pointer of the same type. */

    void renderHtmlOpenTag(std::ostream &os, const char *name, ...) const;

    /** @brief Render a close tag
     * @param os Output
     * @param name Element name
     * @param newline Add a trailing newline
     */
    void renderHtmlCloseTag(std::ostream &os, const char *name,
                            bool newline = true) const;
  };

  /** @brief Intermediate base for leaf node types */
  struct Leaf: public Node {};

  /** @brief An unadorned string */
  struct String: public Leaf {
    /** @brief Text of string */
    std::string text;

    /** @brief Constructor */
    String() = default;

    /** @brief Constructor
     * @param s Initial text
     */
    String(const std::string &s): text(s) {}

    /** @brief Constructor
     * @param n Integer to convert to string
     */
    String(int n);

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;
  };

  /** @brief Base class for ordered containers */
  struct LinearContainer: public Node {
    /** @brief Destructor */
    ~LinearContainer() override;

    /** @brief List of nodes in container */
    std::vector<Node *> nodes;

    /** @brief Append a node
     * @param node Note to append
     * @return @p node
     */
    Node *append(Node *node) {
      nodes.push_back(node);
      return node;
    }

    /** @brief Append a string node
     * @param text Text for new string node
     * @return New string node
     */
    String *append(const std::string &text) {
      String *s = new String(text);
      nodes.push_back(s);
      return s;
    }

    /** @brief Append a list of string nodes
     * @param text Strings to append
     * @param insertNewlines If true append @c \\n to each
     */
    void append(const std::vector<std::string> &text,
                bool insertNewlines = true) {
      for(size_t n = 0; n < text.size(); ++n) {
        append(text[n]);
        if(insertNewlines)
          append("\n");
      }
    }

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtmlContents(std::ostream &os, RenderDocumentContext *rc) const;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderTextContents(std::ostream &os, RenderDocumentContext *rc) const;

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;
  };

  /** @brief A paragraph */
  struct Paragraph: public LinearContainer {
    /** @brief Constructor */
    Paragraph() = default;

    /** @brief Constructor
     * @param node Initial node
     */
    Paragraph(Node *node) {
      append(node);
    }

    /** @brief Constructor
     * @param s Initial text
     */
    Paragraph(const std::string &s) {
      append(s);
    }

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;
  };

  /** @brief A verbatim section */
  struct Verbatim: public LinearContainer {
    /** @brief Constructor */
    Verbatim() = default;

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;
  };

  /** @brief Possible types of list */
  enum ListType { UnorderedList, OrderedList };

  /** @brief A list */
  struct List: public LinearContainer {
    /** @brief Constructor
     * @param type_ List type
     */
    List(ListType type_ = UnorderedList): type(type_) {}

    /** @brief Constructor
     * @param s Text for initial list entry
     */
    Node *entry(const std::string &s) {
      return append(new ListEntry(s));
    }

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief List type */
    ListType type;
  };

  /** @brief One element in a list */
  struct ListEntry: public LinearContainer {
    /** @brief Constructor
     * @param node Initial node
     */
    ListEntry(Node *node) {
      append(node);
    }

    /** @brief Constructor
     * @param s Initial text
     */
    ListEntry(const std::string &s) {
      append(s);
    }

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;
  };

  /** @brief A heading
   *
   * Level 1 is the highest-level (biggest) heading, level 6 the lowest
   * (smallest).  Don't use levels outside this range.
   */
  struct Heading: public LinearContainer {
    /** @brief Level of heading, 1-6 */
    int level;

    /** @brief Constructor
     * @param level_ Level of heading
     */
    Heading(int level_ = 1): level(level_) {}

    /** @brief Constructor
     * @param node Initial node
     * @param level_ Level of heading
     */
    Heading(Node *node, int level_ = 1): Heading(level_) {
      append(node);
    }

    /** @brief Constructor
     * @param text Initial text
     * @param level_ Level of heading
     */
    Heading(const std::string &text, int level_ = 1): Heading(level_) {
      append(text);
    }

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;
  };

  class Table;

  /** @brief A cell in a table.
   *
   * The origin (x=y=0) is at the top left.  Don't add overlapping cells to a
   * single table.
   */
  class Cell: public LinearContainer {
  public:
    /** @brief Constructor
     * @param w_ Cell width
     * @param h_ Cell height
     */
    Cell(int w_ = 1, int h_ = 1, bool heading_ = false):
        heading(heading_), w(w_), h(h_) {}

    /** @brief Constructor
     * @param s Initial contents
     * @param w_ Cell width
     * @param h_ Cell height
     */
    Cell(const std::string &s, int w_ = 1, int h_ = 1, bool heading_ = false):
        Cell(w_, h_, heading_) {
      append(new String(s));
    }

    /** @brief Constructor
     * @param n Initial contents
     * @param w_ Cell width
     * @param h_ Cell height
     */
    Cell(Node *n, int w_ = 1, int h_ = 1, bool heading_ = false):
        Cell(w_, h_, heading_) {
      append(n);
    }

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;

    bool getHeader() const {
      return heading;
    }

    int getX() const {
      return x;
    }

    int getY() const {
      return y;
    }

    int getWidth() const {
      return w;
    }

    int getHeight() const {
      return h;
    }

  private:
    /** @brief True if this is a table heading */
    bool heading;

    /** @brief X position in table */
    int x = 0;

    /** @brief Y position in table */
    int y = 0;

    /** @brief Width in columns */
    int w;

    /** @brief Height in rows */
    int h;

    friend class Table;
  };

  /** @brief A table.
   *
   * The origin (x=y=0) is at the top left.  Don't add overlapping cells to a
   * single table.
   */
  class Table: public Node {
  public:
    /** @brief Destructor */
    ~Table() override;

    /** @brief Width of table (columns) */
    int getWidth() const {
      return width;
    }

    /** @brief Height of table (rows) */
    int getHeight() const {
      return height;
    }

    /** @brief Add a cell at the cursor
     * @param cell Cell to add
     * @return @p cell
     */
    Cell *addCell(Cell *cell);

    /** @brief Start a new row */
    void newRow();

    /** @brief Add a cell at a particular position
     * @param cell Cell to add
     * @param x X position
     * @param y Y position
     * @return @p cell
     */
    Cell *addCell(Cell *cell, int x, int y);

    /** @brief Find the cell at a position
     * @param x X position
     * @param y Y position
     * @return @p cell or null pointer
     *
     * Note that this returns both cells located at x,y and also
     * multi-column/row cells that extend to x,y.
     */
    const Cell *findOverlappingCell(int x, int y) const;

    /** @brief Find the cell rooted at a position
     * @param x X position
     * @param y Y position
     * @return @p cell or null pointer
     *
     * Note that this returns pm;u cells located at x,y, not also
     * multi-column/row cells that extend to x,y.
     */
    const Cell *findRootedCell(int x, int y) const;

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;

  private:
    /** @brief Cursor X position */
    int x = 0;

    /** @brief Cursor Y position */
    int y = 0;

    /** @brief Current table width */
    int width = 0;

    /** @brief Current table height */
    int height = 0;

    /** @brief Map of x, y coordinate pairs to cells */
    std::map<std::pair<int, int>, Cell *> cellMap;
  };

  /** @brief An image */
  struct Image: public Node {
    /** @brief Constructor
     * @param type Content type
     * @param content Raw image data
     */
    Image(const std::string &type, const std::string &content):
        type(type), content(content) {}

    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief MIME type of image */
    std::string type;

    /** @brief Content of image */
    std::string content;

    /** @brief Unique ident of image */
    std::string ident() const;
  };

  /** @brief The root container for the document */
  struct RootContainer: public LinearContainer {
    /** @brief Render as HTML
     * @param os Output
     * @param rc Rendering context
     */
    void renderHtml(std::ostream &os, RenderDocumentContext *rc) const override;

    /** @brief Render as text
     * @param os Output
     * @param rc Rendering context
     */
    void renderText(std::ostream &os, RenderDocumentContext *rc) const override;
  };

  /** @brief The content of the document */
  RootContainer content;

  /** @brief The title of the document */
  std::string title;

  /** @brief The stylesheet, for HTML output */
  std::string htmlStyleSheet;

  /** @brief Append something to the document */
  Node *append(Node *node) {
    return content.append(node);
  }

  /** @brief Append a heading */
  Heading *heading(const std::string &text, int level = 1) {
    Heading *h = new Heading(new String(text), level);
    content.append(h);
    return h;
  }

  /** @brief Append a paragraph */
  Paragraph *para(const std::string &text) {
    Paragraph *p = new Paragraph(new String(text));
    content.append(p);
    return p;
  }

  /** @brief Append a verbatim section */
  Verbatim *verbatim() {
    Verbatim *v = new Verbatim();
    content.append(v);
    return v;
  }

  /** @brief Render the document as HTML
   * @param os Output
   * @param rc Rendering context
   */
  void renderHtml(std::ostream &os, RenderDocumentContext *rc) const;

  /** @brief Render the document as text
   * @param os Output
   * @param rc Rendering context
   */
  void renderText(std::ostream &os, RenderDocumentContext *rc) const;

  /** @brief HTML quoting */
  static void quoteHtml(std::ostream &os, const std::string &s);

  /** @brief Word-wrap text */
  static void wordWrapText(std::ostream &os, const std::string &s, size_t width,
                           size_t indent = 0, bool indentFirst = true);
};

/** @brief Container for rendering */
class RenderDocumentContext {
public:
  /** @brief Accumulated images */
  std::vector<const Document::Image *> images;

  /** @brief Page width */
  int width = DEFAULT_TEXT_WIDTH;
};

#endif /* DOCUMENT_H */
