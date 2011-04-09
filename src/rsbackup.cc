#include <config.h>
#include "Command.h"
#include "Conf.h"
#include "Errors.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
  try {
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
    
    // TODO we need to get the order right (the Perl script does not).
    // In particular if no backup is going to be made we need to NOT
    // spin up the backup disks.

  } catch(std::runtime_error &e) {
    fprintf(stderr, "ERROR: %s\n", e.what());
    exit(1);
  }
  exit(0);
}

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
