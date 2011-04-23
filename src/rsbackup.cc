#include <config.h>
#include "rsbackup.h"
#include "Command.h"
#include "Conf.h"
#include "Errors.h"
#include "Document.h"
#include "IO.h"
#include "FileLock.h"
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sstream>

int main(int argc, char **argv) {
  try {
    if(setlocale(LC_CTYPE, "") == NULL)
      throw std::runtime_error(std::string("setlocale: ") + strerror(errno));

    // Parse command line
    command.parse(argc, argv);

    // Read configuration
    config.read();

    // Select volumes
    command.selectVolumes();

    // Take the lock, if one is defined.
    FileLock lockFile(config.lock);
    if((command.backup
        || command.prune
        || command.pruneIncomplete
        || command.pruneUnknown)
       && config.lock.size()) {
      D("attempting to acquire lockfile %s", config.lock.c_str());
      if(!lockFile.acquire(command.wait)) {
        // Failing to acquire the lock is not really an error if --wait was not
        // requested.  Unlike the Perl version the default behavior is to exit
        // silently.
        if(command.verbose)
          fprintf(stderr, "WARNING: cannot acquire lockfile %s\n",
                  config.lock.c_str());
        exit(0);
      }
    }

    // Read in logfiles
    config.readState();
    
    // TODO we need to get the order right (the Perl script does not).
    // In particular if no backup is going to be made we need to NOT
    // spin up the backup disks.

    if(command.html) {
      Document d;
      d.htmlStyleSheet = stylesheet;
      generateReport(d);
      std::stringstream stream;
      d.renderHtml(stream);
      StdioFile f;
      f.open(*command.html, "w");
      f.write(stream.str());
      f.close();
    }

  } catch(std::runtime_error &e) {
    fprintf(stderr, "ERROR: %s\n", e.what());
    exit(1);
  }
  exit(0);
}
