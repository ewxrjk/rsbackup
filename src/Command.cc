#include <config.h>
#include "Command.h"
#include "Defaults.h"
#include "Errors.h"
#include "Conf.h"
#include <getopt.h>
#include <cstdio>
#include <cstdlib>

enum {
  RETIRE_DEVICE = 256,
  RETIRE = 257,
};

static const struct option options[] = {
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { "backup", no_argument, 0, 'b' },
  { "html", required_argument, 0, 'H' },
  { "email", required_argument, 0, 'e' },
  { "prune", no_argument, 0, 'p' },
  { "prune-incomplete", no_argument, 0, 'P' },
  //{ "store", required_argument, 0, 's' },
  { "retire-device", no_argument, 0, RETIRE_DEVICE },
  { "retire", no_argument, 0, RETIRE },
  { "config", required_argument, 0, 'c' },
  { "wait", no_argument, 0, 'w' },
  { "force", no_argument, 0, 'f' },
  { "dry-run", no_argument, 0, 'n' },
  { "verbose", no_argument, 0, 'v' },
  { "debug", no_argument, 0, 'd' },
  { 0, 0, 0, 0 }
};

Command::Command(): backup(false),
                    prune(false),
                    pruneIncomplete(false),
                    retire(false),
                    retireDevice(false),
                    html(NULL),
                    email(NULL),
                    configPath(DEFAULT_CONFIG),
                    wait(false),
                    act(true),
                    force(false),
                    verbose(false),
                    debug(false) {
}

void Command::help() {
  printf("Usage:\n"
         "  rsbackup [OPTIONS] [--] [[-]HOST...] [[-]HOST:VOLUME...]\n"
         "  rsbackup --retire [OPTIONS] [--] [HOST...] [HOST:VOLUME...]\n"
         "  rsbackup --retire-device [OPTIONS] [--] DEVICES...\n"
         "\n"
         "At least one action option is required:\n"
         "  --backup            Make a backup\n"
         "  --html PATH         Write an HTML report\n"
         "  --email ADDRESS     Mail HTML report to ADDRESS\n"
         "  --prune             Prune old backups\n"
         "  --prune-incomplete  Prune incomplete backups\n"
         "  --retire            Retire volumes\n"
         "  --retire-device     Retire devices\n"
         "\n"
         "Additional options:\n"
         //"  --store DIR         Override directory to store backups in\n"
         "  --config PATH       Set config file (/etc/rsbackup/config)\n"
         "  --wait              Wait until running rsbackup finishes\n"
         "  --force             Don't prompt when retiring\n"
         "  --dry-run           Dry run only\n"
         "  --verbose           Verbose output\n"
         "  --help              Display usage message\n"
         "  --version           Display version number\n"
         "\n"
         "If no volumes are specified then all volumes in the config file are backed up.\n"
         "Otherwise the specified volumes are backed up.\n");
  exit(0);
}

void Command::version() {
  puts(VERSION);
  exit(0);
}

void Command::parse(int argc, char **argv) {
  int n;

  // Parse options
  while((n = getopt_long(argc, argv, "+hVbH:e:pP:c:wnfvd", options, 0)) >= 0) {
    switch(n) {
    case 'h': help();
    case 'V': version();
    case 'b': backup = true; break;
    case 'H': html = new std::string(optarg); break;
    case 'e': email = new std::string(optarg); break;
    case 'p': prune = true; break;
    case 'P': pruneIncomplete = true; break;
    //case 's': stores.push_back(optarg); break;
    case 'c': configPath = optarg; break;
    case 'w': wait = true; break;
    case 'n': act = false; verbose = true; break;
    case 'f': force = true;
    case 'v': verbose = true; break;
    case 'd': debug = true; break;
    case RETIRE_DEVICE: retireDevice = true; break;
    case RETIRE: retire = true; break;
    default: exit(1);
    }
  }

  // Various options are incompatible with one another
  if(retire && retireDevice)
    throw CommandError("--retire and --retire-device cannot be used together");
  if(backup && retire)
    throw CommandError("--retire and --backup cannot be used together");
  if(backup && retireDevice)
    throw CommandError("--retire-device and --backup cannot be used together");

  // We have to do *something*
  if(!backup
     && !html
     && !email
     && !prune
     && !pruneIncomplete
     && !retireDevice
     && !retire)
    throw CommandError("no action specified");

  if(backup || retire) {
    // Volumes to back up or retire
    if(optind < argc) {
      for(n = optind; n < argc; ++n) {
        // Establish the sense of this entry
        bool sense;
        const char *s = argv[n], *t;
        if(*s == '-' || *s == '!') {
          sense = false;
          ++s;
        } else
          sense = true;
        if((t = strchr(s, ':'))) {
          // A host:volume pair
          selections.push_back(Selection(std::string(s, t - s), t + 1, sense));
        } else {
          // Just a host
          selections.push_back(Selection(s, "*", sense));
        }
      }
    } else {
      if(retire)
        throw CommandError("no volumes specified to retire");
      // No volumes requested = back up everything
      selections.push_back(Selection("*", "*", true));
    }
  }
  if(retireDevice) {
    if(optind >= argc)
      throw CommandError("no devices specified to retire");
    for(n = optind; n < argc; ++n)
      devices.push_back(argv[n]);
  }
}

void Command::selectVolumes() {
  for(size_t n = 0; n < selections.size(); ++n)
    config.selectVolume(selections[n].host,
                        selections[n].volume,
                        selections[n].sense);
}

Command command;
