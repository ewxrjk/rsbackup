#include <config.h>
#include "rsbackup.h"
#include "Document.h"
#include "Conf.h"
#include "Command.h"

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
  
  // TODO styling
  for(hosts_type::iterator ith = config.hosts.begin();
      ith != config.hosts.end();
      ++ith) {
    Host *host = ith->second;
    t->addCell(new Document::Cell(host->name, 1, host->volumes.size()));
    for(volumes_type::iterator itv = host->volumes.begin();
        itv != host->volumes.end();
        ++itv) {
      Volume *volume = itv->second;
      t->addCell(new Document::Cell(volume->name));
      t->addCell(new Document::Cell(volume->oldest.toString()));
      t->addCell(new Document::Cell(new Document::String(volume->completed)));
      for(devices_type::const_iterator it = config.devices.begin();
          it != config.devices.end();
          ++it) {
        const Device *device = it->second;
        Volume::PerDevice &perDevice = volume->perDevice[device->name];
        if(perDevice.count)
          t->addCell(new Document::Cell(perDevice.newest.toString()));
        else
          t->addCell(new Document::Cell("none"));
        t->addCell(new Document::Cell(new Document::String(perDevice.count)));
      }
      t->newRow();
    }
  }

  return t;
}

void generateReport(Document &d) {
  d.title = "Backup report";            // TODO date
  d.heading(d.title);

  // Unknown devices ----------------------------------------------------------

  // TODO
  
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
