// Copyright © 2011-19 Richard Kettlewell.
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
#include "Command.h"
#include "Defaults.h"
#include "Errors.h"
#include "Conf.h"
#include "IO.h"
#include "Utils.h"
#include <getopt.h>
#include <cstdlib>

// Long-only options
enum {
  RETIRE_DEVICE = 256,
  RETIRE = 257,
  WARN_UNKNOWN = 258,
  WARN_STORE = 259,
  WARN_UNREACHABLE = 261,
  WARN_PARTIAL = 262,
  REPEAT_ERRORS = 263,
  NO_REPEAT_ERRORS = 264,
  NO_WARN_PARTIAL = 265,
  LOG_VERBOSITY = 266,
  DUMP_CONFIG = 267,
  FORGET_ONLY = 268,
  UNMOUNTED_STORE = 269,
};

const struct option Command::options[] = {
    {"help", no_argument, nullptr, 'h'},
    {"version", no_argument, nullptr, 'V'},
    {"backup", no_argument, nullptr, 'b'},
    {"html", required_argument, nullptr, 'H'},
    {"text", required_argument, nullptr, 'T'},
    {"email", required_argument, nullptr, 'e'},
    {"prune", no_argument, nullptr, 'p'},
    {"prune-incomplete", no_argument, nullptr, 'P'},
    {"store", required_argument, nullptr, 's'},
    {"unmounted-store", required_argument, nullptr, UNMOUNTED_STORE},
    {"retire-device", no_argument, nullptr, RETIRE_DEVICE},
    {"retire", no_argument, nullptr, RETIRE},
    {"config", required_argument, nullptr, 'c'},
    {"wait", no_argument, nullptr, 'w'},
    {"force", no_argument, nullptr, 'f'},
    {"dry-run", no_argument, nullptr, 'n'},
    {"verbose", no_argument, nullptr, 'v'},
    {"warn-unknown", no_argument, nullptr, WARN_UNKNOWN},
    {"warn-store", no_argument, nullptr, WARN_STORE},
    {"warn-unreachable", no_argument, nullptr, WARN_UNREACHABLE},
    {"warn-partial", no_argument, nullptr, WARN_PARTIAL},
    {"no-warn-partial", no_argument, nullptr, NO_WARN_PARTIAL},
    {"errors", no_argument, nullptr, REPEAT_ERRORS},
    {"no-errors", no_argument, nullptr, NO_REPEAT_ERRORS},
    {"warn-all", no_argument, nullptr, 'W'},
    {"debug", no_argument, nullptr, 'd'},
    {"logs", required_argument, nullptr, LOG_VERBOSITY},
    {"dump-config", no_argument, nullptr, DUMP_CONFIG},
    {"database", required_argument, nullptr, 'D'},
    {"forget-only", no_argument, nullptr, FORGET_ONLY},
    {nullptr, 0, nullptr, 0}};

void Command::help() {
  IO::out.writef(helpString());
  IO::out.close();
  exit(0);
}

const char *Command::helpString() {
  return "Usage:\n"
         "  rsbackup [OPTIONS] [--] [[-]HOST...] [[-]HOST:VOLUME...]\n"
         "  rsbackup --retire [OPTIONS] [--] [HOST...] [HOST:VOLUME...]\n"
         "  rsbackup --retire-device [OPTIONS] [--] DEVICES...\n"
         "\n"
         "At least one action option is required:\n"
         "  --backup, -b            Back up selected volumes (default: all)\n"
         "  --html, -H PATH         Write an HTML report to PATH\n"
         "  --text, -T PATH         Write a text report to PATH\n"
         "  --email, -e ADDRESS     Mail HTML report to ADDRESS\n"
         "  --prune, -p             Prune old backups of selected volumes "
         "(default: all)\n"
         "  --prune-incomplete, -P  Prune incomplete backups\n"
         "  --retire                Retire volumes (must specify at least "
         "one)\n"
         "  --forget-only           Retire from database but not disk (with "
         "--retire)\n"
         "  --retire-device         Retire devices (must specify at least "
         "one)\n"
         "  --dump-config           Dump parsed configuration\n"
         "\n"
         "Additional options:\n"
         "  --logs all|errors|recent|latest|failed   Log verbosity in report\n"
         "  --store, -s DIR         Override directory(s) to store backups in\n"
         "  --unmounted-store DIR   Override directory(s) to store backups in\n"
         "  --config, -c PATH       Set config file (default: "
         "/etc/rsbackup/config)\n"
         "  --wait, -w              Wait until running rsbackup finishes\n"
         "  --force, -f             Don't prompt when retiring\n"
         "  --dry-run, -n           Dry run only\n"
         "  --verbose, -v           Verbose output\n"
         "  --debug, -d             Debug output\n"
         "  --database, -D PATH     Override database path\n"
         "  --help, -h              Display usage message\n"
         "  --version, -V           Display version number\n"
         "\n"
         "Warning options:\n"
         "  --warn-unknown          Warn about unknown devices/volumes\n"
         "  --warn-store            Warn about bad stores/unavailable devices\n"
         "  --warn-unreachable      Warn about unreachable hosts\n"
         "  --warn-partial          Warn about partial transfers (default)\n"
         "  --no-warn-partial       Suppress warnings about partial transfers\n"
         "  --warn-all, -W          Enable all warnings\n"
         "  --errors                Display rsync errors (default)\n"
         "  --no-errors             Don't display rsync errors\n";
}

void Command::version() {
  IO::out.writef("%s", VERSION);
  if(strlen(TAG) > 0)
    IO::out.writef(" (git: %s)", TAG);
  IO::out.writef("\n");
  IO::out.close();
  exit(0);
}

void Command::parse(int argc, const char *const *argv) {
  int n;

  // Override debug
  if(getenv("RSBACKUP_DEBUG"))
    globalDebug = true;

  // Parse options
  optind = 1;
  while((n = getopt_long(argc, (char *const *)argv,
                         "+hVbH:T:e:pPs:c:wnfvdWD:", options, nullptr))
        >= 0) {
    switch(n) {
    case 'h': help();
    case 'V': version();
    case 'b': backup = true; break;
    case 'H': html = new std::string(optarg); break;
    case 'T': text = new std::string(optarg); break;
    case 'e': email = new std::string(optarg); break;
    case 'p': prune = true; break;
    case 'P': pruneIncomplete = true; break;
    case 's':
      stores.push_back(optarg);
      enable_warning(WARNING_STORE);
      break;
    case UNMOUNTED_STORE:
      unmountedStores.push_back(optarg);
      enable_warning(WARNING_STORE);
      break;
    case 'c': globalConfigPath = optarg; break;
    case 'w': wait = true; break;
    case 'n':
      act = false;
      enable_warning(WARNING_VERBOSE);
      break;
    case 'f': force = true; break;
    case 'v': enable_warning(WARNING_VERBOSE); break;
    case 'd': globalDebug = true; break;
    case 'D': globalDatabase = optarg; break;
    case RETIRE_DEVICE: retireDevice = true; break;
    case RETIRE: retire = true; break;
    case WARN_UNKNOWN: enable_warning(WARNING_UNKNOWN); break;
    case WARN_STORE: enable_warning(WARNING_STORE); break;
    case WARN_UNREACHABLE: enable_warning(WARNING_UNREACHABLE); break;
    case WARN_PARTIAL: enable_warning(WARNING_PARTIAL); break;
    case NO_WARN_PARTIAL: disable_warning(WARNING_PARTIAL); break;
    case REPEAT_ERRORS: enable_warning(WARNING_ERRORLOGS); break;
    case NO_REPEAT_ERRORS: disable_warning(WARNING_ERRORLOGS); break;
    case LOG_VERBOSITY: logVerbosity = getVerbosity(optarg); break;
    case 'W': enable_warning(static_cast<unsigned>(-1)); break;
    case DUMP_CONFIG: dumpConfig = true; break;
    case FORGET_ONLY: forgetOnly = true; break;
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
  if(forgetOnly && !retire)
    throw CommandError("--forget-only may only be used with --retire");
  if(dumpConfig
     && (backup || html || text || email || prune || pruneIncomplete
         || retireDevice || retire))
    throw CommandError("--dump-config cannot be used with any other action");

  // We have to do *something*
  if(!backup && !html && !text && !email && !prune && !pruneIncomplete
     && !retireDevice && !retire && !dumpConfig)
    throw CommandError("no action specified");

  if(backup || prune || pruneIncomplete || retire) {
    // Volumes to back up, prune or retire
    if(optind < argc) {
      for(n = optind; n < argc; ++n)
        selections.add(argv[n]);
    } else {
      if(retire)
        throw CommandError("no volumes specified to retire");
    }
  }
  if(retireDevice) {
    if(optind >= argc)
      throw CommandError("no devices specified to retire");
    for(n = optind; n < argc; ++n)
      devices.push_back(argv[n]);
  }
  if(dumpConfig) {
    if(optind < argc)
      throw CommandError("no arguments allowed to --dump-config");
  }
}

Command::LogVerbosity Command::getVerbosity(const std::string &v) {
  if(v == "all")
    return All;
  if(v == "errors")
    return Errors;
  if(v == "recent")
    return Recent;
  if(v == "latest")
    return Latest;
  if(v == "failed")
    return Failed;
  throw CommandError("invalid argument to --logs: " + v);
}

Command::~Command() {
  delete html;
  delete text;
  delete email;
}

Command globalCommand;
std::string globalConfigPath = DEFAULT_CONFIG;
std::string globalDatabase;
