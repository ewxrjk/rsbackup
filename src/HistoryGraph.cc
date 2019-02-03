// Copyright © 2015-16, 2019 Richard Kettlewell.
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
#include "Conf.h"
#include "Backup.h"
#include "Volume.h"
#include "Host.h"
#include "HistoryGraph.h"
#include "Errors.h"
#include "Utils.h"
#include <limits>
#include <cassert>
#include <regex>

HostLabels::HostLabels(Render::Context &ctx): Render::Grid(ctx) {
  unsigned row = 0;
  if(globalConfig.hosts.size() == 0)
    throw std::runtime_error("no hosts found in configuration");
  for(auto host_iterator: globalConfig.hosts) {
    Host *host = host_iterator.second;
    if(!host->selected())
      continue;
    if(host->volumes.size() == 0)
      printf("%s has no volumes!?!\n", host_iterator.first.c_str());
    auto t = new Render::Text(ctx, host_iterator.first,
                              globalConfig.colorGraphForeground,
                              globalConfig.hostNameFont);
    cleanup(t);
    add(t, 0, row);
    for(auto volume_iterator: host->volumes) {
      Volume *volume = volume_iterator.second;
      if(!volume->selected())
        continue;
      ++row;
    }
  }
  set_padding(globalConfig.horizontalPadding, globalConfig.verticalPadding);
}

VolumeLabels::VolumeLabels(Render::Context &ctx): Render::Grid(ctx) {
  unsigned row = 0;
  for(auto host_iterator: globalConfig.hosts) {
    Host *host = host_iterator.second;
    if(!host->selected())
      continue;
    for(auto volume_iterator: host->volumes) {
      Volume *volume = volume_iterator.second;
      if(!volume->selected())
        continue;
      auto t = new Render::Text(ctx, volume_iterator.first,
                                globalConfig.colorGraphForeground,
                                globalConfig.volumeNameFont);
      cleanup(t);
      add(t, 0, row);
      ++row;
    }
  }
  set_padding(globalConfig.horizontalPadding, globalConfig.verticalPadding);
}

DeviceKey::DeviceKey(Render::Context &ctx):
  Render::Grid(ctx) {
  unsigned row = 0;
  for(auto device_iterator: globalConfig.devices) {
    const auto &device = device_iterator.first;
    device_rows[device] = row;
    auto t = new Render::Text(ctx, device,
                              globalConfig.colorGraphForeground,
                              globalConfig.deviceNameFont);
    cleanup(t);
    add(t, 0, row);
    auto r = new Render::Rectangle(ctx,
                                   globalConfig.backupIndicatorKeyWidth,
                                   indicator_height,
                                   device_color(row));
    rectangles.push_back(r);
    cleanup(r);
    add(r, 1, row, -1, 0);
    ++row;
  }
  set_padding(globalConfig.horizontalPadding, globalConfig.verticalPadding);
}

void DeviceKey::set_indicator_height(double h) {
  indicator_height = h;
  for(auto r: rectangles)
    r->set_size(-1,
                indicator_height);
}

const Color DeviceKey::device_color(unsigned row) const {
  /* TODO
  char di[64];
  snprintf(di, sizeof di, "device%u", row);
  if(context.colors.find(di) != context.colors.end())
    return context.colors[di];
  */
  return globalConfig.deviceColorStrategy->get(row, globalConfig.devices.size());
}

HistoryGraphContent::HistoryGraphContent(Render::Context &ctx,
                                         const DeviceKey &device_key):
  Render::Widget(ctx),
  earliest(INT_MAX, 1, 0),
  latest(0),
  device_key(device_key),
  rows(0) {
  for(auto host_iterator: globalConfig.hosts) {
    Host *host = host_iterator.second;
    if(!host->selected())
      continue;
    for(auto volume_iterator: host->volumes) {
      Volume *volume = volume_iterator.second;
      if(!volume->selected())
        continue;
      for(auto backup: volume->backups) {
        if(backup->getStatus() == COMPLETE) {
          earliest = std::min(earliest, backup->date);
          latest = std::max(latest, backup->date);
        }
      }
      ++rows;
    }
  }
  if(rows == 0)
    throw CommandError("no volumes selected");
}

void HistoryGraphContent::set_extent() {
  assert(row_height > 0);
  auto columns = latest.toNumber() - earliest.toNumber() + 1;
  height = (rows ? row_height * rows + globalConfig.verticalPadding * (rows - 1)
            : 0);
  width = latest >= earliest ? globalConfig.backupIndicatorWidth * columns : 0;
}

void HistoryGraphContent::render_vertical_guides() {
  set_source_color(globalConfig.colorMonthGuide);
  Date d = earliest;
  while(d <= latest) {
    d.d = 1;
    d.addMonth();
    Date next = d;
    next.addMonth();
    double x = (d - earliest) * globalConfig.backupIndicatorWidth;
    double w = (next - d) * globalConfig.backupIndicatorWidth;
    w = std::min(w, width - x);
    context.cairo->rectangle(x, 0, w, height);
    d.addMonth();
  }
  context.cairo->fill();
}

void HistoryGraphContent::render_horizontal_guides() {
  unsigned row = 0;
  for(auto host_iterator: globalConfig.hosts) {
    Host *host = host_iterator.second;
    if(!host->selected())
      continue;
    set_source_color(globalConfig.colorHostGuide);
    for(auto volume_iterator: host->volumes) {
      Volume *volume = volume_iterator.second;
      if(!volume->selected())
        continue;
      context.cairo->rectangle(0, row * (row_height + globalConfig.verticalPadding),
                               width, 1);
      context.cairo->fill();
      set_source_color(globalConfig.colorVolumeGuide);
      ++row;
    }
  }
  set_source_color(globalConfig.colorVolumeGuide);
  context.cairo->rectangle(0,
                           row * (row_height + globalConfig.verticalPadding)
                             - globalConfig.verticalPadding,
                           width, 1);
  context.cairo->fill();
}

void HistoryGraphContent::render_data() {
  double y = 0;
  double base = floor((row_height + globalConfig.verticalPadding - 1
                       - (indicator_height
                          * globalConfig.devices.size())) / 2) + 1;
  for(auto host_iterator: globalConfig.hosts) {
    Host *host = host_iterator.second;
    if(!host->selected())
      continue;
    for(auto volume_iterator: host->volumes) {
      Volume *volume = volume_iterator.second;
      if(!volume->selected())
        continue;
      for(auto backup: volume->backups) {
        if(backup->getStatus() == COMPLETE) {
          double x = (backup->date - earliest) * globalConfig.backupIndicatorWidth;
          auto device_row = device_key.device_row(backup);
          double offset = base + device_row * indicator_height;
          set_source_color(device_key.device_color(backup));
          context.cairo->rectangle(x, y + offset,
                                   globalConfig.backupIndicatorWidth,
                                   indicator_height);
          context.cairo->fill();
        }
      }
      y += row_height + globalConfig.verticalPadding;
    }
  }
}

void HistoryGraphContent::render() {
  render_vertical_guides();
  render_horizontal_guides();
  render_data();
}

TimeLabels::TimeLabels(Render::Context &ctx,
                       HistoryGraphContent &content): Render::Container(ctx),
                                                      content(content) {
}

void TimeLabels::set_extent() {
  if(width < 0) {
    Date d = content.earliest;
    int year = -1;
    double limit = 0;
    while(d <= content.latest) {
      Date next = d;
      next.d = 1;
      next.addMonth();
      double xnext = (next - content.earliest) * globalConfig.backupIndicatorWidth;
      auto t = new Render::Text(context, "",
                                globalConfig.colorGraphForeground,
                                globalConfig.timeLabelFont);
      cleanup(t);
      // Try increasingly compact formats until one fits
      static const char *const formats[] = {
        "%B %Y",
        "%b %Y",
        "%b %y",
        "%B",
        "%b",
      };
      static const unsigned nformats = sizeof formats / sizeof *formats;
      unsigned format;
      double x;
      for(format = (d.y != year ? 0 : 3); format < nformats; ++format) {
        t->set_text(d.format(formats[format]));
        t->set_extent();
        // At the right hand edge, push back so it fits
        x = std::min((d - content.earliest) * globalConfig.backupIndicatorWidth,
                     content.width - t->width);
        // If it fits, use it
        if(x >= limit && x + t->width < xnext)
          break;
      }
      if(format < nformats) {
        add(t, x, 0);
        limit = x + t->width;
        year = d.y;
      }
      d = next;
    }
    Container::set_extent();
  }
}

HistoryGraph::HistoryGraph(Render::Context &ctx):
  Render::Grid(ctx),
  host_labels(ctx),
  volume_labels(ctx),
  device_key(ctx),
  content(ctx, device_key),
  time_labels(ctx, content) {
  set_padding(globalConfig.horizontalPadding, globalConfig.verticalPadding);
}

void HistoryGraph::set_extent() {
  host_labels.set_extent();
  volume_labels.set_extent();
  device_key.set_extent();
  //time_labels.set_extent();
  double row_height = std::max({host_labels.get_maximum_height(),
        volume_labels.get_maximum_height(),
        (globalConfig.backupIndicatorHeight
         * globalConfig.devices.size())});
  double indicator_height = floor(row_height / globalConfig.devices.size());
  device_key.set_indicator_height(indicator_height);
  content.set_indicator_height(indicator_height);
  host_labels.set_minimum(0, row_height);
  volume_labels.set_minimum(0, row_height);
  content.set_row_height(row_height);
  content.set_extent();
  Grid::set_extent();
}

void HistoryGraph::addPart(const std::string &partspec) {
  static std::regex
    partspec_regex("^([^:]+):([0-9]+),([0-9]+)(?::([LCR])([TCB]))?$");
  std::smatch mr;
  if(!std::regex_match(partspec, mr, partspec_regex))
    throw SyntaxError("invalid graph component specification '" + partspec + "'");
  std::string part = mr[1];
  unsigned column = parseInteger(mr[2], 0, std::numeric_limits<int>::max());
  unsigned row = parseInteger(mr[3], 0, std::numeric_limits<int>::max());
  int hj, vj;
  switch(mr[4].length() ? *mr[4].first : 'L') {
  case 'L': hj = -1; break;
  case 'C': hj = 0; break;
  case 'R': hj = 1; break;
  default: throw std::logic_error("HistoryGraph::addPart hj");
  }
  switch(mr[5].length() ? *mr[5].first : 'T') {
  case 'T': vj = -1; break;
  case 'C': vj = 0; break;
  case 'B': vj = 1; break;
  default: throw std::logic_error("HistoryGraph::addPart vj");
  }
  Widget *w;
  if(part == "host-labels") w = &host_labels;
  else if(part == "volume-labels") w = &volume_labels;
  else if(part == "content") w = &content;
  else if(part == "time-labels") w = &time_labels;
  else if(part == "device-key") w = &device_key;
  else throw SyntaxError("unrecognized graph component '" + part + "'");
  add(w, column, row, hj, vj);
}

void HistoryGraph::addParts(const std::vector<std::string> &partspecs) {
  for(auto &partspec: partspecs)
    addPart(partspec);
}

void HistoryGraph::render() {
  set_source_color(globalConfig.colorGraphBackground);
  context.cairo->rectangle(0, 0, width, height);
  context.cairo->fill();
  Grid::render();
}

void HistoryGraph::adjustConfig() {
  if(content.width == 0)
    return;

  // Figure out how big a content panel we can get away with
  double maxContentWidth = globalConfig.graphTargetWidth - (width - content.width);
  maxContentWidth = floor(maxContentWidth);
  auto columns = content.latest.toNumber() - content.earliest.toNumber() + 1;

  // Work out how big an indicator we can get away with
  double maxIndicatorWidth = maxContentWidth / columns;

  // Constrain to integral widths.  This is a bug, but currently nonintegral
  // widths render badly, so a necessary one.
  if(maxIndicatorWidth >= 1)
    maxIndicatorWidth = floor(maxIndicatorWidth);

  if(width < globalConfig.graphTargetWidth) {
    // We can make the indicators bigger
    // Pick the biggest
    globalConfig.backupIndicatorWidth = std::max(globalConfig.backupIndicatorWidth,
                                                 maxIndicatorWidth);
    content.changed();
    return;
  }
  if(globalConfig.graphTargetWidth > 0 && width > globalConfig.graphTargetWidth) {
    // We have exceeded the target width
    if(maxContentWidth <= 0) {
      // User has asked for the impossible
      warning(WARNING_ALWAYS, "graph-target-width is much too small");
      return;                           // Oh well
    }
    globalConfig.backupIndicatorWidth = maxIndicatorWidth;
    content.changed();
    return;
  }
}
