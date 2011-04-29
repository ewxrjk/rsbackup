#include <config.h>
#include "rsbackup.h"
#include "Command.h"
#include "Conf.h"
#include "Errors.h"
#include "Document.h"
#include "Email.h"
#include "IO.h"
#include "FileLock.h"
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

    // Generate report
    if(command.html || command.email) {
      config.readState();

      Document d;
      d.htmlStyleSheet = stylesheet;
      generateReport(d);
      std::stringstream stream;
      d.renderHtml(stream);
      if(command.html) {
        IO f;
        f.open(*command.html, "w");
        f.write(stream.str());
        f.close();
      }
      if(command.email) {
        Email e;
        e.addTo(*command.email);
        e.setSubject(d.title);
        e.setType("text/html");
        e.setContent(stream.str());
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
