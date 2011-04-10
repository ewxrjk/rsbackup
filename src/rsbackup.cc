#include <config.h>
#include "rsbackup.h"
#include "Command.h"
#include "Conf.h"
#include "Errors.h"
#include "Document.h"
#include "IO.h"
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sstream>

int main(int argc, char **argv) {
  try {
    if(setlocale(LC_CTYPE, "") == NULL)
      throw std::runtime_error(std::string("setlocale: ") + strerror(errno));

    // Parse command line
    Command::parse(argc, argv);

    // Read configuration
    config.read();

    // Select volumes
    Command::selectVolumes();

    // Take the lock
    if((Command::backup
        || Command::prune
        || Command::pruneIncomplete
        || Command::pruneUnknown)
       && config.lock.size()) {
      // TODO
    }

    // Read in logfiles
    config.readState();
    
    // TODO we need to get the order right (the Perl script does not).
    // In particular if no backup is going to be made we need to NOT
    // spin up the backup disks.

    if(Command::html) {
      Document d;
      generateReport(d);
      std::stringstream stream;
      d.renderHtml(stream);
      StdioFile f;
      f.open(*Command::html, "w");
      f.write(stream.str());
      f.close();
    }

  } catch(std::runtime_error &e) {
    fprintf(stderr, "ERROR: %s\n", e.what());
    exit(1);
  }
  exit(0);
}
