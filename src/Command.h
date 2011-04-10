// -*-C++-*-
#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <string>
#include <vector>
#include <cstdio>

// Represents the parsed command line.  An entirely static class, never
// instantiated.
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

  static void parse(int argc, char **argv);

  static void selectVolumes();

  static bool backup;
  static bool prune;
  static bool pruneIncomplete;
  static bool pruneUnknown;
  static std::string *html;
  static std::string *email;
  static std::vector<std::string> stores;
  static std::string configPath;
  static bool wait;
  static bool act;
  static bool verbose;
  static bool debug;

  static std::vector<Selection> selections;
private:
  static void help();
  static void version();

  Command();
};

#define D(...) (void)(Command::debug && fprintf(stderr, __VA_ARGS__) >= 0 && fputc('\n', stderr))

#endif /* COMMANDLINE_H */
