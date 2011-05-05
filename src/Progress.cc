#include <config.h>
#include "IO.h"
#include <cstring>

void progressBar(const char *prompt, size_t done, size_t total) {
  const int width = 79;
  
  if(total == 0) {
    IO::err.writef("\r%*s\r", width, " ");
  } else {
    std::string s;
    int bar = width - (3 + strlen(prompt));
    s.append("\r");
    s.append(prompt);
    s.append(" [");
    s.append(done * bar / total, '=');
    s.append(bar - (done * bar / total), ' ');
    s.append("]\r");
    IO::err.writef("%s", s.c_str());
  }
}
