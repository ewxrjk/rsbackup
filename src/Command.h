// -*-C++-*-
// Copyright © 2011 Richard Kettlewell.
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
#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <string>
#include <vector>
#include <cstdio>

// Represents the parsed command line.
class Command {
public:
  struct Selection {
    Selection(const std::string &host_,
              const std::string &volume_,
              bool sense_ = true): sense(sense_),
                                   host(host_),
                                   volume(volume_) {}
    bool sense;
    std::string host;                   // or "*"
    std::string volume;                 // or "*"
  };

  enum LogVerbosity {
    All,
    Errors,
    Recent,
    Latest,
    Failed,
  };

  Command();

  void parse(int argc, char **argv);

  void selectVolumes();

  bool backup;
  bool prune;
  bool pruneIncomplete;
  bool retire;
  bool retireDevice;
  std::string *html;
  std::string *text;
  std::string *email;
  std::vector<std::string> stores;
  std::string configPath;
  bool wait;
  bool act;
  bool force;
  bool verbose;
  bool warnUnknown;
  bool warnStore;
  bool warnUnreachable;
  bool warnPartial;
  bool repeatErrorLogs;
  LogVerbosity logVerbosity;
  bool debug;

  std::vector<std::string> devices;
  std::vector<Selection> selections;

  static LogVerbosity getVerbosity(const std::string &v);
private:
  void help();
  void version();
};

extern Command command;

#define D(...) (void)(command.debug && fprintf(stderr, __VA_ARGS__) >= 0 && fputc('\n', stderr))

#endif /* COMMANDLINE_H */
