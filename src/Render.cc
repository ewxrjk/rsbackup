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
#include <config.h>
#include "Render.h"
#include "Utils.h"

#include <pangomm/layout.h>

// Widget

Render::Widget::~Widget() {
  deleteAll(cleanup_list);
}

void Render::Widget::changed() {
  if(width >= 0) {
    width = height = -1;
    if(parent)
      parent->changed();
  }
}

void Render::Widget::cleanup(Widget *w) {
  cleanup_list.push_back(w);
}

// Container

void Render::Container::add(Widget *widget, double x, double y) {
  children.push_back(Child(widget, x, y));
  widget->parent = this;
  changed();
}

void Render::Container::set_extent() {
  width = 0;
  height = 0;
  for(const auto &child: children) {
    if(child.widget->width < 0)
      child.widget->set_extent();
    width = std::max(child.x + child.widget->width, width);
    height = std::max(child.y + child.widget->height, height);
  }
}

void Render::Container::render() {
  Render::Guard g(this);
  for(const auto &child: children) {
    context.cairo->translate(child.x, child.y);
    child.widget->render();
    context.cairo->translate(-child.x, -child.y);
  }
}


// Grid

void Render::Grid::add(Widget *widget, unsigned column, unsigned row,
                       int hj, int vj) {
  children.push_back(GridChild(widget, column, row, hj, vj));
  widget->parent = this;
  changed();
}

void Render::Grid::set_padding(double xp, double yp) {
  xpadding = xp;
  ypadding = yp;
  changed();
}

void Render::Grid::set_minimum(double w, double h) {
  force_width = w;
  force_height = h;
  changed();
}

void Render::Grid::set_extent() {
  column_widths.clear();
  row_heights.clear();
  for(const auto &child: children) {
    if(child.widget->width < 0)
      child.widget->set_extent();
    if(child.column >= column_widths.size())
      column_widths.resize(child.column + 1);
    column_widths[child.column] = std::max(child.widget->width,
                                           column_widths[child.column]);
    if(child.row >= row_heights.size())
      row_heights.resize(child.row + 1);
    row_heights[child.row] = std::max(child.widget->height,
                                      row_heights[child.row]);
  }

  for(auto &column_width: column_widths)
    column_width=std::max(column_width, force_width);
  width = xpadding * (column_widths.size() - 1);
  for(auto column_width: column_widths)
    width += column_width;

  for(auto &row_height: row_heights)
    row_height=std::max(row_height, force_height);
  height = ypadding * (row_heights.size() - 1);
  for(auto row_height: row_heights)
    height += row_height;
}

void Render::Grid::render() {
  Render::Guard g(this);
  for(const auto &child: children) {
    double child_x = 0;
    for(unsigned column = 0; column < child.column; ++column)
      child_x += column_widths[column] + xpadding;
    double child_y = 0;
    for(unsigned row = 0; row < child.row; ++row)
      child_y += row_heights[row] + ypadding;
    child_x = justify(child_x, column_widths[child.column],
                      child.widget->width, child.hj);
    child_y = justify(child_y, row_heights[child.row],
                      child.widget->height, child.vj);
    context.cairo->translate(child_x, child_y);
    child.widget->render();
    context.cairo->translate(-child_x, -child_y);
  }
}

double Render::Grid::justify(double x, double cell_width,
                             double child_width, int justification) {
  return x + floor((cell_width - child_width) * (justification + 1) / 2.0);
}

// Colored

void Render::Colored::render() {
  set_source_color(color);
}

// Text

Render::Text::Text(Context &context,
                   const std::string &t,
                   const Color &c):
  Colored(context, c),
  text(t) {
}

void Render::Text::set_text(const std::string &t) {
  text = t;
  changed();
}

void Render::Text::set_extent() {
  if(!layout)
    layout = Pango::Layout::create(context.cairo);
  layout->set_text(text);
  //TODO font
  Pango::Rectangle ink, logical;
  layout->get_pixel_extents(ink, logical);
  width = logical.get_width();
  height = logical.get_height();
}

void Render::Text::render() {
  Colored::render();
  context.cairo->move_to(0, 0);
  layout->show_in_cairo_context(context.cairo);
}

// Rectangle

void Render::Rectangle::set_extent() {
}

void Render::Rectangle::render() {
  Colored::render();
  context.cairo->rectangle(0, 0, width, height);
  context.cairo->fill();
}

void Render::Rectangle::changed() {
}
