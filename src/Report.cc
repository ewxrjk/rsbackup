#include <config.h>
#include "rsbackup.h"
#include "Document.h"
#include "Conf.h"
#include "Command.h"
#include <cmath>

static void unpackColor(unsigned color, int rgb[3]) {
  rgb[0] = (color >> 16) & 255;
  rgb[1] = (color >> 8) & 255;
  rgb[2] = color & 255;
}

static unsigned packColor(const int rgb[3]) {
  return rgb[0] * 65536 + rgb[1] * 256 + rgb[2];
}

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

static Document::Table *generateSummary() {
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
      t->addCell(new Document::Cell(volume->oldest.toString()));
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

void generateReport(Document &d) {
  d.title = "Backup report";            // TODO date
  d.heading(d.title);

  // Unknown objects ----------------------------------------------------------

  if(config.unknownObjects) {
    // TODO styling?
    d.heading("Unknown Objects", 2);
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
    
  
  // Summary table ------------------------------------------------------------
  d.heading("Summary", 2);
  d.append(generateSummary());

  // Disk usage ---------------------------------------------------------------

  // TODO
  
  // Logfiles -----------------------------------------------------------------

  // TODO

  // Prune logs ---------------------------------------------------------------

  // TODO

  // Generation time ----------------------------------------------------------

  Document::Paragraph *p = d.para("Generated ");
  time_t now;
  time(&now);
  p->append(new Document::String(ctime(&now)));

  
}
