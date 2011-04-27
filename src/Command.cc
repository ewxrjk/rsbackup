#include <config.h>
#include "Command.h"
#include "Defaults.h"
#include "Errors.h"
#include "Conf.h"
#include <getopt.h>
#include <cstdio>
#include <cstdlib>

static const struct option options[] = {
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { "backup", no_argument, 0, 'b' },
  { "html", required_argument, 0, 'H' },
  { "email", required_argument, 0, 'e' },
  { "prune", no_argument, 0, 'p' },
  { "prune-incomplete", no_argument, 0, 'P' },
  { "prune-unknown", no_argument, 0, 'u' },
  { "store", required_argument, 0, 's' },
  { "config", required_argument, 0, 'c' },
  { "wait", no_argument, 0, 'w' },
  { "dry-run", no_argument, 0, 'n' },
  { "verbose", no_argument, 0, 'v' },
  { "debug", no_argument, 0, 'd' },
  { 0, 0, 0, 0 }
};
// TODO --retire-device, --retire-host, --retire-volume

Command::Command(): backup(false),
                    prune(false),
                    pruneIncomplete(false),
                    pruneUnknown(false),
                    html(NULL),
                    email(NULL),
                    configPath(DEFAULT_CONFIG),
                    wait(false),
                    act(true),
                    verbose(false),
                    debug(false) {
}

void Command::help() {
  printf("Usage:\n"
         "  rsbackup [OPTIONS] [--] [[-]HOST...] [[-]HOST:VOLUME...]\n"
         "\n"
         "At least one action option is required:\n"
         "  --backup            Make a backup\n"
         "  --html PATH         Write an HTML report\n"
         "  --email ADDRESS     Mail HTML report to ADDRESS\n"
         "  --prune             Prune old backups\n"
         "  --prune-incomplete  Prune incomplete backups\n"
         "  --prune-unknown     Prune logs for lost devices\n"
         "\n"
         "Additional options:\n"
         "  --store DIR         Override directory to store backups in\n"
         "  --config PATH       Set config file (/etc/rsbackup/config)\n"
         "  --wait              Wait until running rsbackup finishes\n"
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
  while((n = getopt_long(argc, argv, "+hVbH:e:pPus:c:wnvd", options, 0)) >= 0) {
    switch(n) {
    case 'h': help();
    case 'V': version();
    case 'b': backup = true; break;
    case 'H': html = new std::string(optarg); break;
    case 'e': email = new std::string(optarg); break;
    case 'p': prune = true; break;
    case 'P': pruneIncomplete = true; break;
    case 'u': pruneUnknown = true; break;
    case 's': stores.push_back(optarg); break;
    case 'c': configPath = optarg; break;
    case 'w': wait = true; break;
    case 'n': act = false; verbose = true; break;
    case 'v': verbose = true; break;
    case 'd': debug = true; break;
    default: exit(1);
    }
  }
  
  // Volumes to back up.
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
    // No volumes requested = back up everything
    selections.push_back(Selection("*", "*", true));
  }

  // We have to to *something*
  if(!backup
     && !html
     && !email
     && !prune
     && !pruneIncomplete
     && !pruneUnknown)
    throw std::runtime_error("no action specified"); // TODO exception class
}

void Command::selectVolumes() {
  for(size_t n = 0; n < selections.size(); ++n)
    config.selectVolume(selections[n].host, 
                        selections[n].volume,
                        selections[n].sense);
}

Command command;
