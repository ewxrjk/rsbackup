// Copyright © 2015 Richard Kettlewell.
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
#include <getopt.h>
#include "Conf.h"
#include "Command.h"
#include "Selection.h"
#include "IO.h"
#include "HistoryGraph.h"
#include "Errors.h"
#include "Utils.h"

#include <pangomm/init.h>

static const struct option options[] = {
  { "help", no_argument, nullptr, 'h' },
  { "version", no_argument, nullptr, 'V' },
  { "config", required_argument, nullptr, 'c' },
  { "debug", no_argument, nullptr, 'd' },
  { "database", required_argument, nullptr, 'D' },
  { "output", required_argument, nullptr, 'o' },
  { nullptr, 0, nullptr, 0 }
};

static const char *graphHelpString() {
  return
"Usage:\n"
"  rsbackup-graph [OPTIONS] [--] [[-]HOST...] [[-]HOST:VOLUME...]\n"
"\n"
"Options:\n"
"  --config, -c PATH       Set config file (default: /etc/rsbackup/config)\n"
"  --debug, -d             Debug output\n"
"  --database, -D PATH     Override database path\n"
"  --output, -o PATH       Output filename\n"
"  --help, -h              Display usage message\n"
"  --version, -V           Display version number\n"
"\n"
    ;
}

static void help() {
  IO::out.writef(graphHelpString());
  IO::out.close();
  exit(0);
}

static void version() {
  IO::out.writef("%s\n", VERSION);
  IO::out.close();
  exit(0);
}

int main(int argc, char **argv) {
  try {

    int n;
    const char *output = "rsbackup.png";
    VolumeSelections selections;

    // Override debug
    if(getenv("RSBACKUP_DEBUG"))
      debug = true;

    // Parse options
    optind = 1;
    while((n = getopt_long(argc, (char *const *)argv,
                           "+hVdc:D:o:", options, nullptr)) >= 0) {
      switch(n) {
      case 'h': help();
      case 'V': version();
      case 'c': configPath = optarg; break;
      case 'd': debug = true; break;
      case 'D': database = optarg; break;
      case 'o': output = optarg; break;
      default: exit(1);
      }
    }

    if(optind < argc)
      for(n = optind; n < argc; ++n)
        selections.add(argv[n]);

    config.read();
    config.validate();
    config.readState();
    selections.select(config);

    // Eliminates segfault with "Failed to wrap object of type
    // 'PangoLayout'. Hint: this error is commonly caused by failing to call a
    // library init() function.".
    //
    // How you're supposed to know about this I've not discovered.
    Pango::init();

    // Rendering context
    Render::Context context;

    // The graph
    HistoryGraph graph(context);

    // Use a throwaway surface to work out size
    Cairo::RefPtr<Cairo::Surface> surface
      = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                    1, 1);
    context.cairo = Cairo::Context::create(surface);
    graph.set_extent();

    // Render to the real surface
    surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                          ceil(graph.width),
                                          ceil(graph.height));
    context.cairo = Cairo::Context::create(surface);
    graph.render();

    surface->write_to_png(output);

    return 0;
  } catch(Error &e) {
    error("%s", e.what());
    if(debug)
      e.trace(stderr);
    return 1;
  } catch(std::runtime_error &e) {
    error("%s", e.what());
    return 1;
  }
}
