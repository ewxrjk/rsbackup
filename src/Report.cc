// Copyright © 2011-2015 Richard Kettlewell.
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
#include "rsbackup.h"
#include "Document.h"
#include "Conf.h"
#include "Command.h"
#include "IO.h"
#include "Database.h"
#include "Report.h"
#include "Utils.h"
#include "Subprocess.h"
#include "Errors.h"
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <boost/range/adaptor/reversed.hpp>

// Split up a color into RGB components
void Report::unpackColor(unsigned color, int rgb[3]) {
  rgb[0] = (color >> 16) & 255;
  rgb[1] = (color >> 8) & 255;
  rgb[2] = color & 255;
}

// Pack a color from RGB components
unsigned Report::packColor(const int rgb[3]) {
  return rgb[0] * 65536 + rgb[1] * 256 + rgb[2];
}

// Pick a color as a (clamped) linear combination of two endpoints
unsigned Report::pickColor(unsigned zero, unsigned one, double param) {
  int zeroRgb[3], oneRgb[3], resultRgb[3];
  unpackColor(zero, zeroRgb);
  unpackColor(one, oneRgb);
  if(param < 0)
    param = 0;
  if(param > 1)
    param = 1;
  resultRgb[0] = zeroRgb[0] * (1 - param) + oneRgb[0] * param;
  resultRgb[1] = zeroRgb[1] * (1 - param) + oneRgb[1] * param;
  resultRgb[2] = zeroRgb[2] * (1 - param) + oneRgb[2] * param;
  return packColor(resultRgb);
}

void Report::compute() {
  backups_missing = 0;
  backups_partial = 0;
  backups_out_of_date = 0;
  backups_failed = 0;
  devices_unknown = config.unknownDevices.size();
  hosts_unknown = config.unknownHosts.size();
  volumes_unknown = 0;
  for(auto &h: config.hosts) {
    const Host *host = h.second;
    volumes_unknown += host->unknownVolumes.size();
    for(auto &v: host->volumes) {
      const Volume *volume = v.second;
      bool out_of_date = true;
      size_t devices_used = 0;
      for(auto &d: config.devices) {
        const Device *device = d.second;
        auto perDevice = volume->findDevice(device->name);
        if(perDevice && perDevice->count) {
          // At least one successful backup exists...
          int newestAge = Date::today() - perDevice->newest;
          if(newestAge <= volume->maxAge)
            out_of_date = false;        // ...and it's recent enough
          ++devices_used;
        }
        // Look for the most recent attempt at this device
        const Backup *most_recent_backup = nullptr;
        for(const Backup *b: boost::adaptors::reverse(volume->backups)) {
          if(b->getStatus() == COMPLETE && b->deviceName == device->name) {
            most_recent_backup = b;
            break;
          }
        }
        if(most_recent_backup && most_recent_backup->rc != 0)
          ++backups_failed;             // most recent backup failed
      }
      if(devices_used < config.devices.size()) { // some device lacks a backup
        if(devices_used == 0)
          ++backups_missing;            // no device has a backup
        else
          ++backups_partial;            // only some devices have a backup
      } else if(out_of_date)
        ++backups_out_of_date;          // no device has a recent enough backup
    }
  }
}

// Generate the list of warnings
void Report::warnings() {
  char buffer[1024];
  Document::List *l = new Document::List();
  for(auto &d: config.unknownDevices)
    l->entry("Unknown device " + d);
  for(auto &h: config.unknownHosts)
    l->entry("Unknown host " + h);
  for(auto &h: config.hosts) {
    const std::string &hostName = h.first;
    Host *host = h.second;
    for(auto &v: host->unknownVolumes)
      l->entry("Unknown volume " + v + " on host " + hostName);
  }
  if(backups_missing) {
    snprintf(buffer, sizeof buffer, "WARNING: %d volumes have no backups.",
             backups_missing);
    l->entry(buffer);
  }
  if(backups_partial) {
    snprintf(buffer, sizeof buffer, "WARNING: %d volumes are not fully backed up.",
             backups_partial);
    l->entry(buffer);
  }
  if(backups_out_of_date) {
    snprintf(buffer, sizeof buffer, "WARNING: %d volumes are out of date.",
             backups_out_of_date);
    l->entry(buffer);
  }
  if(backups_failed) {
    snprintf(buffer, sizeof buffer, "WARNING: %d volumes failed latest backup.",
             backups_failed);
    l->entry(buffer);
  }
  d.append(l);
}

int Report::warningCount() const {
  int warnings = config.unknownDevices.size() + config.unknownHosts.size();
  for(auto &h: config.hosts)
    warnings += h.second->unknownVolumes.size();
  if(backups_missing) ++warnings;
  if(backups_partial) ++warnings;
  if(backups_out_of_date) ++warnings;
  if(backups_failed) ++warnings;
  return warnings;
}

// Generate the summary table
void Report::summary() {
  Document::Table *t = new Document::Table();

  t->addHeadingCell(new Document::Cell("Host", 1, 3));
  t->addHeadingCell(new Document::Cell("Volume", 1, 3));
  t->addHeadingCell(new Document::Cell("Oldest", 1, 3));
  t->addHeadingCell(new Document::Cell("Total", 1, 3));
  t->addHeadingCell(new Document::Cell("Devices", 2 * config.devices.size(), 1));
  t->newRow();

  for(auto &d: config.devices)
    t->addHeadingCell(new Document::Cell(d.second->name, 2, 1));
  t->newRow();

  for(auto attribute((unused)) &d: config.devices) {
    t->addHeadingCell(new Document::Cell("Newest"));
    t->addHeadingCell(new Document::Cell("Count"));
  }
  t->newRow();

  for(auto &h: config.hosts) {
    const Host *host = h.second;
    t->addCell(new Document::Cell(host->name, 1, host->volumes.size()))
      ->style = "host";
    for(auto &v: host->volumes) {
      const Volume *volume = v.second;
      // See if every device has a backup
      bool missingDevice = false;
      for(const auto &d: config.devices) {
        const Device *device = d.second;
        if(!contains(volume->perDevice, device->name))
          missingDevice = true;
      }
      t->addCell(new Document::Cell(volume->name))
        ->style = "volume";
      t->addCell(new Document::Cell(volume->oldest != Date()
                                    ? volume->oldest.toString()
                                    : "none"));
      t->addCell(new Document::Cell(new Document::String(volume->completed)))
        ->style = missingDevice ? "bad" : "good";
      for(const auto &d: config.devices) {
        const Device *device = d.second;
        auto perDevice = volume->findDevice(device->name);
        int perDeviceCount = perDevice ? perDevice->count : 0;
        if(perDeviceCount) {
          // At least one successful backups
          Document::Cell *c
            = t->addCell(new Document::Cell(perDevice->newest.toString()));
          int newestAge = Date::today() - perDevice->newest;
          if(newestAge <= volume->maxAge) {
            double param = (pow(2, (double)newestAge / volume->maxAge) - 1) / 2.0;
            c->bgcolor = pickColor(config.colorGood, config.colorBad, param);
          } else {
            c->style = "bad";
          }
        } else {
          // No succesful backups!
          t->addCell(new Document::Cell("none"))
            ->style = "bad";
        }
        t->addCell(new Document::Cell(new Document::String(perDeviceCount)))
          ->style = perDeviceCount ? "good" : "bad";
      }
      t->newRow();
    }
  }

  d.append(t);
}

// Return true if this is a suitable log for the report
bool Report::suitableLog(const Volume *volume, const Backup *backup) {
  // Empty logs are never shown.
  if(!backup->contents.size())
    return false;
  switch(command.logVerbosity) {
  case Command::All:
    // Show everything
    return true;
  case Command::Errors:
    // Show all error logs
    return backup->rc != 0;
  case Command::Recent:
    // Show the most recent error log for the device
    return backup == volume->mostRecentFailedBackup(backup->getDevice());
  case Command::Latest:
    // Show the most recent logfile for the device
    return backup == volume->mostRecentBackup(backup->getDevice());
  case Command::Failed:
    // Show the most recent logfile for the device, if it is an error
    return (backup->rc
            && backup == volume->mostRecentBackup(backup->getDevice()));
  default:
    throw std::logic_error("unknown log verbosity");
  }
}

// Generate the report of backup logfiles for a volume
void Report::logs(const Volume *volume) {
  Document::LinearContainer *lc = nullptr;
  const Host *host = volume->parent;
  // Backups for a volume are ordered primarily by date and secondarily by
  // device.  The most recent backups are the most interesting so they are
  // displayed in reverse.
  std::set<std::string> devicesSeen;
  for(const Backup *backup: boost::adaptors::reverse(volume->backups)) {
    // Only include logs of failed backups
    if(suitableLog(volume, backup)) {
      if(!lc) {
        d.heading("Host " + host->name
                  + " volume " + volume->name
                  + " (" + volume->path + ")", 3);
        lc = new Document::LinearContainer();
        lc->style = "volume";
        d.append(lc);
      }
      Document::Heading *heading =
        new Document::Heading(backup->date.toString()
                              + " device " + backup->deviceName
                              + " volume "
                                 + backup->volume->parent->name
                                 + ":" + backup->volume->name,
                              4);
      if(!contains(devicesSeen, backup->deviceName))
        heading->style = "recent";
      lc->append(heading);
      Document::Verbatim *v = new Document::Verbatim();
      v->style = "log";
      v->append(backup->contents);
      lc->append(v);
    }
    devicesSeen.insert(backup->deviceName);
  }
}

// Generate the report of backup logfiles for everything
void Report::logs() {
  // Sort by host/volume first, then date, device *last*
  for(auto &h: config.hosts) {
    const Host *host = h.second;
    for(auto &v: host->volumes) {
      const Volume *volume = v.second;
      logs(volume);
    }
  }
}

// Generate the report of pruning logfiles
void Report::pruneLogs(const std::string &days) {
  int ndays = DEFAULT_PRUNE_REPORT_AGE;
  if(days.size())
    ndays = parseInteger(days, 0, INT_MAX);
  if(config.reportPruneLogs)
    ndays = config.reportPruneLogs;
  Document::Table *t = new Document::Table();

  t->addHeadingCell(new Document::Cell("Created", 1, 1));
  t->addHeadingCell(new Document::Cell("Pruned", 1, 1));
  t->addHeadingCell(new Document::Cell("Host", 1, 1));
  t->addHeadingCell(new Document::Cell("Volume", 1, 1));
  t->addHeadingCell(new Document::Cell("Device", 1, 1));
  t->addHeadingCell(new Document::Cell("Reason", 1, 1));
  t->newRow();

  const int64_t cutoff = Date::now() - 86400 * ndays;
  Database::Statement stmt(config.getdb(),
                           "SELECT host,volume,device,time,pruned,log"
                           " FROM backup"
                           " WHERE (status=? OR status=?) AND pruned >= ?"
                           " ORDER BY pruned DESC",
                           SQL_INT, PRUNING,
                           SQL_INT, PRUNED,
                           SQL_INT64, cutoff,
                           SQL_END);
  while(stmt.next()) {
    Backup backup;
    char timestr[64];
    std::string hostName = stmt.get_string(0);
    std::string volumeName = stmt.get_string(1);
    std::string deviceName = stmt.get_string(2);
    time_t when = stmt.get_int64(3);
    time_t pruned = stmt.get_int64(4);
    std::string reason = stmt.get_blob(5);

    strftime(timestr, sizeof timestr, "%Y-%m-%d", localtime(&when));
    t->addCell(new Document::Cell(timestr));
    strftime(timestr, sizeof timestr, "%Y-%m-%d", localtime(&pruned));
    t->addCell(new Document::Cell(timestr));
    t->addCell(new Document::Cell(hostName));
    t->addCell(new Document::Cell(volumeName));
    t->addCell(new Document::Cell(deviceName));
    t->addCell(new Document::Cell(reason));

    t->newRow();
  }

  d.append(t);
}

void Report::historyGraph() {
  std::string history_png;
  std::string rg = Subprocess::pathSearch("rsbackup-graph");
  if(rg.size() == 0)
    return;
  Subprocess sp({"rsbackup-graph", "-o", "-"});
  sp.capture(1, &history_png);
  sp.runAndWait();
 if(history_png.size()) {
    std::stringstream ss;
    ss << "data:image/png;base64,";
    write_base64(ss, history_png);
    d.append(new Document::Image(ss.str()));
  }
}

void Report::generated() {
  Document::Paragraph *p = d.para("Generated ");
  if(getenv("RSBACKUP_TODAY"))
    p->append(new Document::String("<timestamp>"));
  else {
    time_t now;
    time(&now);
    p->append(new Document::String(ctime(&now)));
  }
}

void Report::section(const std::string &n) {
  std::string name = n, value, condition;
  size_t colon = name.find("?");
  if(colon != std::string::npos) {
    condition.assign(name, colon + 1, std::string::npos);
    name.erase(colon);
    if(condition == "warnings") {
      if(!warningCount())
        return;
    } else
      throw SyntaxError("unrecognized report condition '" + condition + "'");
  }
  colon = name.find(":");
  if(colon != std::string::npos) {
    value = substitute(name, colon + 1, std::string::npos);
    name.erase(colon);
  }
  if(name == "warnings") warnings();
  else if(name == "summary") summary();
  else if(name == "logs") logs();
  else if(name == "prune-logs") pruneLogs(value);
  else if(name == "history-graph") historyGraph();
  else if(name == "generated") generated();
  else if(name == "h1") d.heading(value, 1);
  else if(name == "h2") d.heading(value, 2);
  else if(name == "h3") d.heading(value, 3);
  else if(name == "title") d.title = value;
  else throw SyntaxError("unrecognized report name '" + name + "'");
}

// Generate the full report
void Report::generate() {
  if(::setenv("RSBACKUP_DATE",
              Date::today().toString().c_str(), 1/*overwrite*/))
    throw SystemError("setenv", errno);
  compute();
  for(const auto &s: config.report)
    section(s);
}
