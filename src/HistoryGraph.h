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
#ifndef HISTORYGRAPH_H
#define HISTORYGRAPH_H

#include "Render.h"
#include "Conf.h"

/** @brief Host name labels */
class HostLabels: public Render::Grid {
public:
  /** @brief Constructor
   * @param ctx Rendering context
   */
  HostLabels(Render::Context &ctx);
};

/** @brief Volume name labels */
class VolumeLabels: public Render::Grid {
public:
  /** @brief Constructor
   * @param ctx Rendering context
   */
  VolumeLabels(Render::Context &ctx);
};

/** @brief Key showing mapping of device names to colors */
class DeviceKey: public Render::Grid {
public:
  /** @brief Constructor
   * @param ctx Rendering context
   */
  DeviceKey(Render::Context &ctx);

  /** @brief Return the device row number for a backup
   * @param backup Backup
   * @return Device row number
   */
  unsigned device_row(const Backup *backup) const {
    return device_rows.find(backup->deviceName)->second;
  }

  /** @brief Return the color for a device by number
   * @param row Device row number
   * @return Color
   */
  const Color device_color(unsigned row) const;

  /** @brief Return the color for a backup
   * @param backup Backup
   * @return Color
   */
  const Color device_color(const Backup *backup) const {
    return device_color(device_row(backup));
  }

  void set_indicator_height(double h);

private:
  /** @brief Mapping of device names to device rows */
  std::map<std::string,unsigned> device_rows;

  std::list<Render::Rectangle *> rectangles;

  /** @brief Height of device indicator rectangle */
  double indicator_height = 1;
};

/** @brief Visualization of backup history */
class HistoryGraphContent: public Render:: Widget {
public:
  /** @brief Constructor
   * @param ctx Rendering context
   * @param device_key Corresponding @ref DeviceKey structure
   */
  HistoryGraphContent(Render::Context &ctx,
                      const DeviceKey &device_key);

  /** @brief Set the rot height
   * @param h Row height
   */
  void set_row_height(double h) {
    row_height = h;
    changed();
  }

  /** @brief Render the vertical guides */
  void render_vertical_guides();

  /** @brief Render the horizontal guides */
  void render_horizontal_guides();

  /** @brief Render the data */
  void render_data();

  void set_extent() override;
  void render() override;

  /** @brief Earliest date of any backup */
  Date earliest;

  /** @brief Latest date of any backup */
  Date latest;

  void set_indicator_height(double h) {
    indicator_height = h;
  }

private:
  /** @brief Height of a single row
   *
   * Set by @ref set_extent.
   */
  double row_height = 0;

  /** @brief Corresponding @ref DeviceKey object */
  const DeviceKey &device_key;

  /** @brief Number of rows */
  unsigned rows;

  /** @brief Height of device indicator rectangle */
  double indicator_height = 1;
};

/** @brief Time-axis labels */
class TimeLabels: public Render::Container {
public:
  /** @brief Constructor
   * @param ctx Rendering context
   * @param content Corresponding @ref HistoryGraphContent object
   */
  TimeLabels(Render::Context &ctx,
             HistoryGraphContent &content);

  void set_extent() override;
private:
  /** @brief Corresponding @ref HistoryGraphContent object */
  HistoryGraphContent &content;
};

/** @brief Complete graph showing backup history */
class HistoryGraph: public Render::Grid {
public:
  /** @brief Constructor
   * @param ctx Rendering context
   */
  HistoryGraph(Render::Context &ctx);

  /** @brief Host name labels */
  HostLabels host_labels;

  /** @brief Volume name labels */
  VolumeLabels volume_labels;

  /** @brief Key showing mapping of device names to colors */
  DeviceKey device_key;

  /** @brief Visualization of backup history */
  HistoryGraphContent content;

  /** @brief Time-axis labels */
  TimeLabels time_labels;

  void set_extent() override;
  void render() override;
};

#endif /* HISTORYGRAPH_H */
