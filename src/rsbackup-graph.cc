// Copyright © 2015-17, 2019 Richard Kettlewell.
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
#include "Backup.h"
#include "Command.h"
#include "Selection.h"
#include "IO.h"
#include "HistoryGraph.h"
#include "Errors.h"
#include "Utils.h"

#include <pangomm/init.h>
#include <pango/pangocairo.h>

static const struct option options[] = {
    {"help", no_argument, nullptr, 'h'},
    {"version", no_argument, nullptr, 'V'},
    {"config", required_argument, nullptr, 'c'},
    {"debug", no_argument, nullptr, 'd'},
    {"database", required_argument, nullptr, 'D'},
    {"output", required_argument, nullptr, 'o'},
    {"fonts", no_argument, nullptr, 'F'},
    {nullptr, 0, nullptr, 0}};

static const char *graphHelpString() {
  return "Usage:\n"
         "  rsbackup-graph [OPTIONS] [--] [[-]HOST...] [[-]HOST:VOLUME...]\n"
         "\n"
         "Options:\n"
         "  --config, -c PATH       Set config file (default: "
         "/etc/rsbackup/config)\n"
         "  --debug, -d             Debug output\n"
         "  --database, -D PATH     Override database path\n"
         "  --output, -o PATH       Output filename\n"
         "  --fonts, -F             List supported fonts\n"
         "  --help, -h              Display usage message\n"
         "  --version, -V           Display version number\n"
         "\n";
}

[[noreturn]] static void help() {
  IO::out.writef(graphHelpString());
  IO::out.close();
  exit(0);
}

[[noreturn]] static void version() {
  IO::out.writef("%s\n", VERSION);
  IO::out.close();
  exit(0);
}

static Cairo::ErrorStatus stdout_write_func(const unsigned char *data,
                                            unsigned int length) {
  fwrite(data, 1, length, stdout);
  if(ferror(stdout))
    throw SystemError("writing to stdout", errno);
  return CAIRO_STATUS_SUCCESS;
}

static void listFonts() {
  auto pfm = pango_cairo_font_map_get_default();
  PangoFontFamily **families;
  int n_families;
  pango_font_map_list_families(pfm, &families, &n_families);
  std::vector<std::string> names;
  for(int n = 0; n < n_families; n++)
    names.push_back(pango_font_family_get_name(families[n]));
  std::sort(names.begin(), names.end());
  for(auto name: names)
    IO::out.writef("%s\n", name.c_str());
}

int main(int argc, char **argv) {
  try {

    int n;
    const char *output = "rsbackup.png";
    VolumeSelections selections;

    // Override debug
    if(getenv("RSBACKUP_DEBUG"))
      globalDebug = true;

    // Hack to avoid graph generation causing a database upgrade
    globalCommand.act = false;

    // Parse options
    optind = 1;
    while((n = getopt_long(argc, (char *const *)argv, "+hVdc:D:o:F", options,
                           nullptr))
          >= 0) {
      switch(n) {
      case 'h': help();
      case 'V': version();
      case 'c': globalConfigPath = optarg; break;
      case 'd': globalDebug = true; break;
      case 'D': globalDatabase = optarg; break;
      case 'o': output = optarg; break;
      case 'F': listFonts(); exit(0);
      default: exit(1);
      }
    }

    if(optind < argc)
      for(n = optind; n < argc; ++n)
        selections.add(argv[n]);

    globalConfig.read();
    globalConfig.validate();
    globalConfig.readState();
    selections.select(globalConfig);

    // Eliminates segfault with "Failed to wrap object of type
    // 'PangoLayout'. Hint: this error is commonly caused by failing to call a
    // library init() function.".
    //
    // How you're supposed to know about this I've not discovered.
    Pango::init();

    // Rendering context
    Render::Context context;

    // Use a throwaway graph and surface to work out size
    Cairo::RefPtr<Cairo::Surface> surface =
        Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 1, 1);
    context.cairo = Cairo::Context::create(surface);
    HistoryGraph graph0(context);
    graph0.addParts(globalConfig.graphLayout);
    graph0.set_extent();
    graph0.adjustConfig();

    // Create the real graph
    HistoryGraph graph(context);
    graph.addParts(globalConfig.graphLayout);
    graph.set_extent();
    // Create the real surface
    surface = Cairo::ImageSurface::create(
        Cairo::FORMAT_ARGB32, ceil(graph.width), ceil(graph.height));
    context.cairo = Cairo::Context::create(surface);
    graph.render();

    if(std::string(output) == "-")
      surface->write_to_png_stream(&stdout_write_func);
    else
      surface->write_to_png(output);
    return 0;
  } catch(Error &e) {
    error("%s", e.what());
    if(globalDebug)
      e.trace(stderr);
    return 1;
  } catch(std::runtime_error &e) {
    error("%s", e.what());
    return 1;
  }
}
