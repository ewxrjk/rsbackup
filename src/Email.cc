#include <config.h>
#include "Email.h"
#include "Conf.h"
#include "Errors.h"
#include "IO.h"

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
  mail.popen(command, WriteToPipe);
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
  mail.writef("Content-Type: %s\n", type.c_str());
  mail.writef("\n");
  mail.write(content);
  if(content.size()
     && content[content.size() - 1] != '\n')
    mail.write("\n");
  mail.close();
}
