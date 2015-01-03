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

class Report {
  Document &d;
public:
  Report(Document &d_): d(d_) {}
  void generate();

  int backups_missing;

  int backups_partial;

  int backups_out_of_date;

  int backups_failed;

  int devices_unknown;

  int hosts_unknown;

  int volumes_unknown;

 private:
  static void unpackColor(unsigned color, int rgb[3]);
  static unsigned packColor(const int rgb[3]);
  static unsigned pickColor(unsigned zero, unsigned one, double param);
  void reportWarnings();
  Document::Table *reportSummary();
  bool suitableLog(const Volume *volume, const Backup *backup);
  void reportLogs(const Volume *volume);
  void reportLogs();
  Document::Node *reportPruneLogs();

};

#endif /* REPORT_H */
