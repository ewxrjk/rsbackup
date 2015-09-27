// Copyright © 2011, 2012 Richard Kettlewell.
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
#include "Email.h"
#include "Conf.h"
#include "Errors.h"
#include "IO.h"
#include "Command.h"

Email::Email(): type("text/plain") {
}

void Email::send() const {
  if(to.size() == 0)
    throw std::logic_error("no recipients for email");
  std::vector<std::string> command;
  command.push_back(config.sendmail);
  command.push_back("-t");              // recipients from header
  command.push_back("-oee");            // request bounce xor error
  command.push_back("-oi");             // de-magic '.'
  command.push_back("-odb");            // background delivery
  IO mail;
  mail.popen(command, WriteToPipe, ::command.verbose);
  if(from.size())
    mail.writef("From: %s\n", from.c_str());
  mail.writef("To: ");
  for(size_t n = 0; n < to.size(); ++n) {
    if(n)
      mail.writef(", ");
    mail.writef("%s", to[n].c_str());
  }
  mail.writef("\n");
  mail.writef("Subject: %s\n", subject.c_str());
  mail.writef("MIME-Version: 1.0\n");
  mail.writef("User-Agent: rsbackup/" VERSION "\n");
  mail.writef("Content-Type: %s\n", type.c_str());
  mail.writef("\n");
  mail.write(content);
  if(content.size()
     && content[content.size() - 1] != '\n')
    mail.write("\n");
  mail.close();
}
