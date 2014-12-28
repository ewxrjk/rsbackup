// Copyright © 2011-2013 Richard Kettlewell.
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
#include "Regexp.h"
#include "IO.h"
#include <cmath>
#include <cstdlib>
#include <stdexcept>

// Split up a color into RGB components
static void unpackColor(unsigned color, int rgb[3]) {
  rgb[0] = (color >> 16) & 255;
  rgb[1] = (color >> 8) & 255;
  rgb[2] = color & 255;
}

// Pack a color from RGB components
static unsigned packColor(const int rgb[3]) {
  return rgb[0] * 65536 + rgb[1] * 256 + rgb[2];
}

// Pick a color as a (clamped) linear combination of two endpoints
static unsigned pickColor(unsigned zero, unsigned one, double param) {
  int zeroRgb[3], oneRgb[3], resultRgb[3];
  unpackColor(zero, zeroRgb);
  unpackColor(one, oneRgb);
  if(param < 0) param = 0;
  if(param > 1) param = 1;
  resultRgb[0] = zeroRgb[0] * (1 - param) + oneRgb[0] * param;
  resultRgb[1] = zeroRgb[1] * (1 - param) + oneRgb[1] * param;
  resultRgb[2] = zeroRgb[2] * (1 - param) + oneRgb[2] * param;
  return packColor(resultRgb);
}

// Generate the list of unknown devices, hosts and volumes
static void reportUnknown(Document &d) {
  Document::List *l = new Document::List();
  for(std::set<std::string>::iterator it = config.unknownDevices.begin();
      it != config.unknownDevices.end();
      ++it) {
    l->entry("Unknown device " + *it);
  }
  for(std::set<std::string>::iterator it = config.unknownHosts.begin();
      it != config.unknownHosts.end();
      ++it) {
    l->entry("Unknown host " + *it);
  }
  for(hosts_type::iterator hostsIterator = config.hosts.begin();
      hostsIterator != config.hosts.end();
      ++hostsIterator) {
    const std::string &hostName = hostsIterator->first;
    Host *host = hostsIterator->second;
    for(std::set<std::string>::iterator it = host->unknownVolumes.begin();
        it != host->unknownVolumes.end();
        ++it) {
      l->entry("Unknown volume " + *it + " on host " + hostName);
    }
  }
  d.append(l);
}

// Generate the summary table
static Document::Table *reportSummary() {
  Document::Table *t = new Document::Table();

  t->addHeadingCell(new Document::Cell("Host", 1, 3));
  t->addHeadingCell(new Document::Cell("Volume", 1, 3));
  t->addHeadingCell(new Document::Cell("Oldest", 1, 3));
  t->addHeadingCell(new Document::Cell("Total", 1, 3));
  t->addHeadingCell(new Document::Cell("Devices", 2 * config.devices.size(), 1));
  t->newRow();

  for(devices_type::const_iterator it = config.devices.begin();
      it != config.devices.end();
      ++it) {
    const Device *device = it->second;
    t->addHeadingCell(new Document::Cell(device->name, 2, 1));
  }
  t->newRow();

  for(devices_type::const_iterator it = config.devices.begin();
      it != config.devices.end();
      ++it) {
    t->addHeadingCell(new Document::Cell("Newest"));
    t->addHeadingCell(new Document::Cell("Count"));
  }
  t->newRow();

  for(hosts_type::iterator ith = config.hosts.begin();
      ith != config.hosts.end();
      ++ith) {
    Host *host = ith->second;
    t->addCell(new Document::Cell(host->name, 1, host->volumes.size()))
      ->style = "host";
    for(volumes_type::iterator itv = host->volumes.begin();
        itv != host->volumes.end();
        ++itv) {
      Volume *volume = itv->second;
      // See if every device has a backup
      bool missingDevice = false;
      for(devices_type::iterator itd = config.devices.begin();
          itd != config.devices.end();
          ++itd) {
        Device *device = itd->second;
        if(volume->perDevice.find(device->name) == volume->perDevice.end())
          missingDevice = true;
      }
      t->addCell(new Document::Cell(volume->name))
        ->style = "volume";
      t->addCell(new Document::Cell(volume->oldest != Date()
                                    ? volume->oldest.toString()
                                    : "none"));
      t->addCell(new Document::Cell(new Document::String(volume->completed)))
        ->style = missingDevice ? "bad" : "good";
      for(devices_type::const_iterator it = config.devices.begin();
          it != config.devices.end();
          ++it) {
        const Device *device = it->second;
        Volume::PerDevice &perDevice = volume->perDevice[device->name];
        if(perDevice.count) {
          Document::Cell *c
            = t->addCell(new Document::Cell(perDevice.newest.toString()));
          int newestAge = Date::today() - perDevice.newest;
          if(newestAge <= volume->maxAge) {
            double param = (pow(2, (double)newestAge / volume->maxAge) - 1) / 2.0;
            c->bgcolor = pickColor(COLOR_GOOD, COLOR_BAD, param);
          } else {
            c->style = "bad";
          }
        } else
          t->addCell(new Document::Cell("none"))
            ->style = "bad";
        t->addCell(new Document::Cell(new Document::String(perDevice.count)))
          ->style = perDevice.count ? "good" : "bad";
      }
      t->newRow();
    }
  }

  return t;
}

// Return true if this is a suitable log for the report
static bool suitableLog(const Volume *volume, const Backup *backup) {
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
static void reportLogs(Document &d,
                       Volume *volume) {
  Document::LinearContainer *lc = NULL;
  Host *host = volume->parent;
  // Backups for a volume are ordered primarily by date and secondarily by
  // device.  The most recent backups are the most interesting so they are
  // displayed in reverse.
  std::set<std::string> devicesSeen;
  for(backups_type::reverse_iterator backupsIterator = volume->backups.rbegin();
      backupsIterator != volume->backups.rend();
      ++backupsIterator) {
    const Backup *backup = *backupsIterator;
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
                              + " (" + backup->logPath() + ")",
                              4);
      if(devicesSeen.find(backup->deviceName) == devicesSeen.end())
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

// Generate the rpeort of backup logfiles for everything
static void reportLogs(Document &d) {
  // Sort by host/volume first, then date, device *last*
  for(hosts_type::iterator hostsIterator = config.hosts.begin();
      hostsIterator != config.hosts.end();
      ++hostsIterator) {
    Host *host = hostsIterator->second;
    for(volumes_type::iterator volumesIterator = host->volumes.begin();
        volumesIterator != host->volumes.end();
        ++volumesIterator) {
      Volume *volume = volumesIterator->second;
      reportLogs(d, volume);
    }
  }
}

// Generate the report of pruning logfiles
static void reportPruneLogs(Document &d) {
  std::map<Date,std::string> pruneLogs;
  // Retrieve pruning logs.
  Regexp r("^prune-([0-9]+-[0-9]+-[0-9]+)\\.log$");
  Directory dir;
  dir.open(config.logs);
  std::string f;
  while(dir.get(f)) {
    if(!r.matches(f))
      continue;
    Date date = r.sub(1);
    pruneLogs[date] = config.logs + PATH_SEP + f;
  }
  // Display them, most recent first
  for(std::map<Date,std::string>::reverse_iterator pruneLogsIterator = pruneLogs.rbegin();
      pruneLogsIterator != pruneLogs.rend();
      ++pruneLogsIterator) {
    const std::string &path = pruneLogsIterator->second;
    Document::Verbatim *v = d.verbatim();
    v->style = "log";
    IO logFile;
    logFile.open(path, "r");
    std::string line;
    while(logFile.readline(line)) {
      v->append(line);
      v->append("\n");
    }
  }
}

// Generate the full report
void generateReport(Document &d) {
  d.title = "Backup report (" + Date::today().toString() + ")";
  d.heading(d.title);

  // Unknown objects ----------------------------------------------------------
  if(config.unknownObjects) {
    d.heading("Warnings", 2);
    reportUnknown(d);
  }

  // Summary table ------------------------------------------------------------
  d.heading("Summary", 2);
  d.append(reportSummary());

  // Logfiles -----------------------------------------------------------------
  d.heading("Logfiles", 2);
  reportLogs(d);

  // Prune logs ---------------------------------------------------------------
  d.heading("Pruning logs", 3);         // TODO anchor
  reportPruneLogs(d);

  // Generation time ----------------------------------------------------------
  Document::Paragraph *p = d.para("Generated ");
  if(getenv("RSBACKUP_TODAY"))
    p->append(new Document::String("<timestamp>"));
  else {
    time_t now;
    time(&now);
    p->append(new Document::String(ctime(&now)));
  }

}
