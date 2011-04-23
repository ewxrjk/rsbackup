#include <config.h>
#include "Email.h"
#include "Conf.h"
#include "Errors.h"
#include "IO.h"

Email::Email(): type("text/plain") {
}

void Email::send() const {
  StdioFile mail;
  std::vector<std::string> command;

  command.push_back(config.sendmail);
  command.push_back("-t");              // recipients from header
  command.push_back("-oee");            // request bounce xor error
  command.push_back("-oi");             // de-magic '.'
  command.push_back("-odb");            // background delivery
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
  int rc = mail.close();
  if(rc) {
    char buffer[10];
    sprintf(buffer, "%#x", rc);
    throw std::runtime_error(config.sendmail + " exited with wait status " + buffer);
  }
}
