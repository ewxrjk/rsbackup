// -*-C++-*-
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
#ifndef REPORT_H
#define REPORT_H

/** @brief Generator for current state */
class Report {
  /** @brief Destination for report */
  Document &d;
public:
  /** @brief Constructor
   * @param d_ Destination for report
   */
  Report(Document &d_): d(d_) {}

  /** @brief Generate the report and set counters */
  void generate();

  /** @brief Number of volumes with no backups at all */
  int backups_missing;

  /** @brief Number of volumes missing a backup on at least one device */
  int backups_partial;

  /** @brief Number of volumes with no backup within max-age */
  int backups_out_of_date;

  /** @brief Number of volume/device pairs where most recent backup failed */
  int backups_failed;

  /** @brief Number of unknown devices */
  int devices_unknown;

  /** @brief Number of unknown hosts */
  int hosts_unknown;

  /** @brief Number of unknown volumes */
  int volumes_unknown;

 private:
  /** @brief Split up a color into RGB components */
  static void unpackColor(unsigned color, int rgb[3]);

  /** @brief Pack a color from RGB components */
  static unsigned packColor(const int rgb[3]);

  /** @brief Pick a color as a (clamped) linear combination of two endpoints */
  static unsigned pickColor(unsigned zero, unsigned one, double param);

  /** @brief Generate the list of warnings
   *
   * The counters must have been set.
   */
  void reportWarnings();

  /** @brief Generate the summary table and set counters */
  Document::Table *reportSummary();

  /** @brief Return @c true if this is a suitable log for the report */
  bool suitableLog(const Volume *volume, const Backup *backup);

  /** @brief Generate the report of backup logs for a volume */
  void reportLogs(const Volume *volume);

  /** @brief Generate the report of backup logs for everything */
  void reportLogs();

  /** @brief Generate the report of pruning logfiles */
  Document::Node *reportPruneLogs();

};

#endif /* REPORT_H */
