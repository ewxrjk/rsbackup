// Copyright © 2011-15 Richard Kettlewell.
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
#include "Command.h"
#include "Conf.h"
#include "Store.h"
#include "Errors.h"
#include "Document.h"
#include "Email.h"
#include "IO.h"
#include "FileLock.h"
#include "Subprocess.h"
#include "DeviceAccess.h"
#include "Utils.h"
#include "Report.h"
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sstream>
#include <iostream>
#include <iomanip>

int main(int argc, char **argv) {
  try {
    if(setlocale(LC_CTYPE, "") == nullptr)
      throw std::runtime_error(std::string("setlocale: ") + strerror(errno));

    // Parse command line
    command.parse(argc, argv);

    // Read configuration
    config.read();

    // Validate configuration
    config.validate();

    // Dump configuration
    if(command.dumpConfig) {
      config.write(std::cout, 0, command.verbose);
      exit(0);
    }

    // Override stores
    if(command.stores.size() != 0) {
      for(auto &s: config.stores)
        s.second->state = Store::Disabled;
      for(auto &s: command.stores) {
        auto it = config.stores.find(s);
        if(it == config.stores.end())
          config.stores[s] = new Store(s);
        else
          it->second->state = Store::Enabled;
      }
    }

    // Take the lock, if one is defined.
    FileLock lockFile(config.lock);
    if((command.backup
        || command.prune
        || command.pruneIncomplete
        || command.retireDevice
        || command.retire)
       && config.lock.size()) {
      D("attempting to acquire lockfile %s", config.lock.c_str());
      if(!lockFile.acquire(command.wait)) {
        // Failing to acquire the lock is not really an error if --wait was not
        // requested.
        if(command.verbose)
          warning("cannot acquire lockfile %s", config.lock.c_str());
        exit(0);
      }
    }

    // Select volumes
    if(command.backup || command.prune || command.pruneIncomplete)
      command.selections.select(config);

    // Execute commands
    if(command.backup)
      makeBackups();
    if(command.retire)
      retireVolumes();
    if(command.retireDevice)
      retireDevices();
    if(command.prune || command.pruneIncomplete)
      pruneBackups();
    if(command.prune)
      prunePruneLogs();

    // Run post-access hook
    postDeviceAccess();

    // Generate report
    if(command.html || command.text || command.email) {
      config.readState();

      Document d;
      if(config.stylesheet.size()) {
        IO ssf;
        ssf.open(config.stylesheet, "r");
        ssf.readall(d.htmlStyleSheet);
      } else
        d.htmlStyleSheet = stylesheet;
      // Include user colors in the stylesheet
      std::stringstream ss;
      ss << "td.bad { background-color: #" << config.colorBad << " }\n";
      ss << "td.good { background-color: #" << config.colorGood << " }\n";
      ss << "span.bad { color: #" << config.colorBad << " }\n";
      d.htmlStyleSheet += ss.str();
      Report report(d);
      report.generate();
      std::stringstream htmlStream, textStream;
      if(command.html || command.email)
        d.renderHtml(htmlStream);
      if(command.text || command.email)
        d.renderText(textStream);
      if(command.html) {
        if(*command.html == "-") {
          IO::out.write(htmlStream.str());
        } else {
          IO f;
          f.open(*command.html, "w");
          f.write(htmlStream.str());
          f.close();
        }
      }
      if(command.text) {
        if(*command.text == "-") {
          IO::out.write(textStream.str());
        } else {
          IO f;
          f.open(*command.text, "w");
          f.write(textStream.str());
          f.close();
        }
      }
      if(command.email) {
        Email e;
        e.addTo(*command.email);
        std::stringstream subject;
        subject << d.title;
        if(report.backups_missing)
          subject << " missing:" << report.backups_missing;
        if(report.backups_partial)
          subject << " partial:" << report.backups_partial;
        if(report.backups_out_of_date)
          subject << " stale:" << report.backups_out_of_date;
        if(report.backups_failed)
          subject << " failed:" << report.backups_failed;
        if(report.devices_unknown
           || report.hosts_unknown
           || report.volumes_unknown)
          subject << " unknown:" << (report.devices_unknown
                                     + report.hosts_unknown
                                     + report.volumes_unknown);
        e.setSubject(subject.str());
        e.setType("multipart/alternative; boundary=" MIME_BOUNDARY);
        std::stringstream body;
        body << "--" MIME_BOUNDARY "\n";
        body << "Content-Type: text/plain\n";
        body << "\n";
        body << textStream.str();
        body << "\n";
        body << "--" MIME_BOUNDARY "\n";
        body << "Content-Type: text/html\n";
        body << "\n";
        body << htmlStream.str();
        body << "\n";
        body << "--" MIME_BOUNDARY "--\n";
        e.setContent(body.str());
        e.send();
      }
    }
    if(errors && command.verbose)
      warning("%d errors detected", errors);
    IO::out.close();
  } catch(Error &e) {
    error("%s", e.what());
    if(debug)
      e.trace(stderr);
  } catch(std::runtime_error &e) {
    error("%s", e.what());
  }
  exit(!!errors);
}
