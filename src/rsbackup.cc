// Copyright © 2011-13 Richard Kettlewell.
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
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sstream>

int errors;

int main(int argc, char **argv) {
  try {
    if(setlocale(LC_CTYPE, "") == NULL)
      throw std::runtime_error(std::string("setlocale: ") + strerror(errno));

    // Parse command line
    command.parse(argc, argv);

    // Read configuration
    config.read();

    // Override stores
    if(command.stores.size() != 0) {
      while(config.stores.size()) {
        stores_type::iterator it = config.stores.begin();
        delete it->second;
        config.stores.erase(it);
      }
      for(size_t n = 0; n < command.stores.size(); ++n)
        config.stores[command.stores[n]] = new Store(command.stores[n]);
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
        // requested.  Unlike the Perl version the default behavior is to exit
        // silently.
        if(command.verbose)
          IO::err.writef("WARNING: cannot acquire lockfile %s\n",
                         config.lock.c_str());
        exit(0);
      }
    }

    // Select volumes
    if(command.backup || command.prune || command.pruneIncomplete)
      command.selectVolumes();

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
      d.htmlStyleSheet = stylesheet;
      generateReport(d);
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
        e.setSubject(d.title);
        e.setType("multipart/alternative; boundary="MIME_BOUNDARY);
        std::stringstream body;
        body << "--"MIME_BOUNDARY"\n";
        body << "Content-Type: text/plain\n";
        body << "\n";
        body << textStream.str();
        body << "\n";
        body << "--"MIME_BOUNDARY"\n";
        body << "Content-Type: text/html\n";
        body << "\n";
        body << htmlStream.str();
        body << "\n";
        body << "--"MIME_BOUNDARY"--\n";
        e.setContent(body.str());
        e.send();
      }
    }
    if(errors && command.verbose)
      IO::err.writef("WARNING: %d errors detected\n", errors);
    IO::out.close();
  } catch(std::runtime_error &e) {
    IO::err.writef("ERROR: %s\n", e.what());
    exit(1);
  }
  exit(!!errors);
}
