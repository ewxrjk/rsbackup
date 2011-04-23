// -*-C++-*-
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

  Command();

  void parse(int argc, char **argv);

  void selectVolumes();

  bool backup;
  bool prune;
  bool pruneIncomplete;
  bool pruneUnknown;
  std::string *html;
  std::string *email;
  std::vector<std::string> stores;
  std::string configPath;
  bool wait;
  bool act;
  bool verbose;
  bool debug;

  std::vector<Selection> selections;
private:
  void help();
  void version();
};

extern Command command;

#define D(...) (void)(command.debug && fprintf(stderr, __VA_ARGS__) >= 0 && fputc('\n', stderr))

#endif /* COMMANDLINE_H */
