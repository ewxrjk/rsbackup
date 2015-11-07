// Copyright © 2014 Richard Kettlewell.
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
#include "Errors.h"
#include "Conf.h"
#include <getopt.h>
#include <cassert>

static void test_action_backup(void) {
  static const char *argv[] = { "rsbackup", "--backup", nullptr };
  Command c;
  assert(c.backup == false);
  c.parse(2, argv);
  assert(c.backup == true);
}

static void test_action_html(void) {
  static const char *argv[] = { "rsbackup", "--html", "PATH", nullptr };
  Command c;
  assert(c.html == nullptr);
  c.parse(3, argv);
  assert(c.html != nullptr);
  assert(*c.html == "PATH");
}

static void test_action_text(void) {
  static const char *argv[] = { "rsbackup", "--text", "PATH", nullptr };
  Command c;
  assert(c.text == nullptr);
  c.parse(3, argv);
  assert(c.text != nullptr);
  assert(*c.text == "PATH");
}

static void test_action_email(void) {
  static const char *argv[] = { "rsbackup", "--email", "user@domain", nullptr };
  Command c;
  assert(c.email == nullptr);
  c.parse(3, argv);
  assert(c.email != nullptr);
  assert(*c.email == "user@domain");
}

static void test_action_prune(void) {
  static const char *argv[] = { "rsbackup", "--prune", nullptr };
  Command c;
  assert(c.prune == false);
  c.parse(2, argv);
  assert(c.prune == true);
}

static void test_action_prune_incomplete(void) {
  static const char *argv[] = { "rsbackup", "--prune-incomplete", nullptr };
  Command c;
  assert(c.pruneIncomplete == false);
  c.parse(2, argv);
  assert(c.pruneIncomplete == true);
}

static void test_action_retire(void) {
  static const char *argv[] = { "rsbackup", "--retire", "VOLUME", nullptr };
  Command c;
  assert(c.retire == false);
  c.parse(3, argv);
  assert(c.retire == true);
  assert(c.selections.size() == 1);

  Command d;
  assert(d.retire == false);
  try {
    d.parse(2, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
  }
}

static void test_action_retire_device(void) {
  static const char *argv[] = { "rsbackup", "--retire-device", "DEVICE", nullptr };
  Command c;
  assert(c.retireDevice == false);
  c.parse(3, argv);
  assert(c.retireDevice == true);
  assert(c.devices.size() == 1);
  assert(c.devices.at(0) == "DEVICE");

  Command d;
  assert(d.retireDevice == false);
  try {
    d.parse(2, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
  }
}

static void test_action_dump_config(void) {
  static const char *argv[] = { "rsbackup", "--dump-config", "JUNK", nullptr };
  Command c;
  assert(c.dumpConfig == false);
  c.parse(2, argv);
  assert(c.dumpConfig == true);

  Command d;
  assert(d.dumpConfig == false);
  try {
    d.parse(3, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
  }
}

static void test_action_none(void) {
  static const char *argv[] = { "rsbackup", nullptr };
  Command c;
  try {
    c.parse(1, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
  }
}

static void test_action_incompatible(void) {
  try {
    static const char *argv[] = { "rsbackup", "--retire", "--retire-device", "XYZ", nullptr };
    Command c;
    c.parse(4, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
    assert(std::string(e.what()).find("cannot be used together")
           != std::string::npos);
  }
  try {
    static const char *argv[] = { "rsbackup", "--retire", "--backup", "XYZ", nullptr };
    Command c;
    c.parse(4, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
    assert(std::string(e.what()).find("cannot be used together")
           != std::string::npos);
  }
  try {
    static const char *argv[] = { "rsbackup", "--retire-device", "--backup", "XYZ", nullptr };
    Command c;
    c.parse(4, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
    assert(std::string(e.what()).find("cannot be used together")
           != std::string::npos);
  }
  try {
    static const char *argv[] = { "rsbackup", "--dump-config", "--backup", "XYZ", nullptr };
    Command c;
    c.parse(4, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
    assert(std::string(e.what()).find("cannot be used with any other action")
           != std::string::npos);
  }

}

static void test_selection(void) {
  {
    static const char *argv[] = { "rsbackup", "--backup", "A", "-A:B", "!C",
                                  nullptr };
    Command c;
    c.parse(5, argv);
    assert(c.backup == true);
    assert(c.selections.size() == 3);
    assert(c.selections[0].sense == true);
    assert(c.selections[0].host == "A");
    assert(c.selections[0].volume == "*");
    assert(c.selections[1].sense == false);
    assert(c.selections[1].host == "A");
    assert(c.selections[1].volume == "B");
    assert(c.selections[2].sense == false);
    assert(c.selections[2].host == "C");
    assert(c.selections[2].volume == "*");
  }

  try {
    static const char *argv[] = { "rsbackup", "--backup", "~A",
                                  nullptr };
    Command c;
    c.parse(3, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
  }

  try {
    static const char *argv[] = { "rsbackup", "--backup", "A:~",
                                  nullptr };
    Command c;
    c.parse(3, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
  }

  try {
    static const char *argv[] = { "rsbackup", "--backup", "A:B:C",
                                  nullptr };
    Command c;
    c.parse(3, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
  }

  try {
    static const char *argv[] = { "rsbackup", "--backup", "*:C",
                                  nullptr };
    Command c;
    c.parse(3, argv);
    assert(!"unexpectedly succeeded");
  } catch(CommandError &e) {
  }
}

int main() {
  int errors = 0;
  const std::string h = Command::helpString();
  for(size_t n = 0; Command::options[n].name; ++n) {
    std::string full = "--" + std::string(Command::options[n].name);
    if(h.find(full + ",") == std::string::npos
       && h.find(full + " ") == std::string::npos) {
      fprintf(stderr, "ERROR: help for option %s not found\n", full.c_str());
      ++errors;
    }
  }
  test_action_backup();
  test_action_html();
  test_action_text();
  test_action_email();
  test_action_prune();
  test_action_prune_incomplete();
  test_action_retire();
  test_action_retire_device();
  test_action_dump_config();
  test_action_none();
  test_action_incompatible();
  test_selection();
  return !!errors;
}
