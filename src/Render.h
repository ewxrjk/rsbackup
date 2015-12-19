//-*-C++-*-
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
#ifndef RENDER_H
#define RENDER_H
/** @file Render.h
 * @brief Graphical rendering
 */

#include <cairomm/context.h>
#include <pangomm/layout.h>
#include "Color.h"

namespace Render {

  /** @brief Rendering context */
  struct Context {
    /** @brief Cairo context */
    Cairo::RefPtr<Cairo::Context> cairo;
  };

  /** @brief Base class for widgets */
  class Widget {
  public:
    /** @brief Constructor
     * @param ctx Rendering context
     */
    Widget(Context &ctx): context(ctx) {}

    /** @brief Destructor
     */
    virtual ~Widget();

    /** @brief Width
     *
     * Set by @ref Widget::set_extent
     */
    double width = -1;

    /** @brief Height
     *
     * Set by @ref Widget::set_extent.
     */
    double height = -1;

    /** @brief Parent widget */
    Widget *parent = nullptr;

    /** @brief Set @ref width and @ref height
     *
     * If there are child widgets, the first step should be to recursively
     * calculate their extents.  This may be skipped for children with
     * non-negative @ref width fields.
     */
    virtual void set_extent() = 0;

    /** @brief Render this widget
     */
    virtual void render() = 0;

    /** @brief Clean up some other widget when this one is destroyed */
    void cleanup(Widget *w);

  protected:
    /** @brief Called when the widget's extent potentially changes
     *
     * Invalidates @ref Widget::width and @ref Widget::height, and then
     * recurses into its parent (if there is one). */
    virtual void changed();

    /** @brief Context for drawing */
    Context &context;

    /** @brief Set the drawing color
     * @param color Color to draw in
     */
    inline void set_source_color(const Color &color) {
      context.cairo->set_source_rgb(color.red, color.green, color.blue);
    }

    /** @brief List of widgets to clean up on destruction */
    std::vector<Widget *> cleanup_list;

    friend class Guard;
  };

  /** @brief Container that places widgets at arbitrary locations */
  class Container: public Widget {
  public:
    /** @brief Constructor
     * @param ctx Rendering context
     */
    Container(Context &ctx): Widget(ctx) {}

    /** @brief Add a child widget
     * @param widget Pointer to child widget
     * @param x X coordinate of top left of widget
     * @param y Y coordinate of top left of widget
     */
    void add(Widget *widget, double x, double y);

    void set_extent() override;
    void render() override;

  private:
    /** @brief Annotated child of a grid */
    struct Child {
      /** @brief Child widget */
      Widget *widget;

      /** @brief X coordinate */
      double x;

      /** @brief Y coordinate */
      double y;

      /** @brief Constructor */
      Child(Widget *w, double x, double y): widget(w), x(x), y(y) {}
    };

    /** @brief Child widgets in order of addition */
    std::vector<Child> children;

  };

  /** @brief Container that arranges widgets in a grid */
  class Grid: public Widget {
  public:
    /** @brief Constructor
     * @param ctx Rendering context
     */
    Grid(Context &ctx): Widget(ctx) {}

    /** @brief Add a child widget
     * @param widget Pointer to child widget
     * @param column Column number from 0
     * @param row Row number from 0
     * @param hj Horizontal justification
     * @param vj Vertical justification
     *
     * Justification values are:
     * - -1 for left justified
     * - 0 for centered
     * - 1 for right justified
     */
    void add(Widget *widget, unsigned column, unsigned row,
             int hj = -1, int vj = -1);

    /** @brief Set inter-child padding
     * @param xp Horizontal padding
     * @param yp Vertical padding
     */
    void set_padding(double xp, double yp);

    /** @brief Set minimum child sizes
     * @param w Minimum width
     * @param h Minimum height
     */
    void set_minimum(double w, double h);

    /** @brief Get the maximum width of any column
     * @return Maximum column width
     *
     * @ref Grid::set_extent must have been called.
     */
    double get_maximum_width() const {
      return *std::max_element(column_widths.begin(), column_widths.end());
    }

    /** @brief Get the maximum height of any row
     * @return Maximum row height
     *
     * @ref Grid::set_extent must have been called.
     */
    double get_maximum_height() const {
      return *std::max_element(row_heights.begin(), row_heights.end());
    }

    void set_extent() override;
    void render() override;

  private:
    /** @brief Annotated child of a grid */
    struct GridChild {
      /** @brief Child widget */
      Widget *widget;

      /** @brief Column number from 0 */
      unsigned column;

      /** @brief row Row number from 0 */
      unsigned row;

      /** @brief Horizontal justification */
      int hj;

      /** @brief Vertical justification */
      int vj;

      /** @brief Constructor */
      GridChild(Widget *w, int c, int r, int h, int v):
        widget(w), column(c), row(r), hj(h), vj(v) {}
    };

    /** @brief Horizontal padding */
    double xpadding = 0;

    /** @brief Vertical padding */
    double ypadding = 0;

    /** @brief Minimum column width */
    double force_width = 0;

    /** @brief Minimum row height */
    double force_height = 0;

    /** @brief Child widgets in order of addition */
    std::vector<GridChild> children;

    /** @brief Column widths (only after Grid::set_extent) */
    std::vector<double> column_widths;

    /** @brief Column heights (only after Grid::set_extent)  */
    std::vector<double> row_heights;

    /** @brief Justify coordinate
     * @param x Coordinate of container
     * @param cell_width Size of container
     * @param child_width Width of contained element
     * @param justification Justification mode
     * @return Justified coordinate
     */
    static double justify(double x, double cell_width, double child_width,
                          int justification);
  };

  /** @brief Colored widget */
  class Colored: public Widget {
  public:
    /** @brief Constructor
     * @param ctx Rendering context
     * @param c Color
     */
    Colored(Context &ctx, const Color &c = {0,0,0}):
      Widget(ctx),color(c) {
    }

    /** @brief Set color
     * @param c Color
     */
    void set_color(const Color &c) {
      color = c;
    }

    /** @brief Set the color in the rendering context */
    void render() override;

  private:
    /** @brief Color */
    Color color;
  };

  /** @brief Text widget */
  // TODO font
  class Text: public Colored {
  public:
    /** @brief Constructor
     * @param ctx Rendering context
     * @param t Text to display
     * @param c Color for text
     */
    Text(Context &ctx,
         const std::string &t = "",
         const Color &c = {0,0,0});

    /** @brief Set text
     * @param t Text to display
     */
    void set_text(const std::string &t);

    void set_extent() override;
    void render() override;

  private:
    /** @brief Text to render */
    std::string text;

    /** @brief Pango layout for text */
    Glib::RefPtr<Pango::Layout> layout;
  };

  /** @brief Filled rectangular widget */
  class Rectangle: public Colored {
  public:
    /** @brief Constructor
     * @param ctx Rendering context
     * @param w Width
     * @param h Height
     * @param c Color
     */
    Rectangle(Context &ctx,
         double w,
         double h,
         const Color &c = {0,0,0}): Colored(ctx, c) {
      width = w;
      height = h;
    }

    void set_size(double w, double h) {
      if(w > 0)
        width = w;
      if(h > 0)
        height = h;
    }

    void set_extent() override;
    void render() override;

  protected:
    void changed() override;
  };

  /** @brief Guard class for rendering contexts */
  class Guard {
  public:
    /** @brief Constructor
     * @param w Any widget with the right rendering context
     */
    Guard(Widget *w): context(w->context) { context.cairo->save(); }

    /** @brief Destructor */
    ~Guard() { context.cairo->restore(); }
  private:

    /** @brief Rendering context */
    Context &context;
  };

};

#endif /* RENDER_H */
