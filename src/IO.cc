// Copyright © 2011-17 Richard Kettlewell.
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
#include "IO.h"
#include "Errors.h"
#include "Subprocess.h"
#include <cerrno>
#include <cstdlib>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

IO::~IO() {
  if(closeFile && fp)
    fclose(fp);
  if(subprocess)
    delete subprocess;
}

void IO::open(const std::string &path_, const std::string &mode) {
  if(!(fp = fopen(path_.c_str(), mode.c_str())))
    throw IOError("opening " + path_, errno);
  path = path_;
  closeFile = true;
}

void IO::popen(const std::vector<std::string> &command, PipeDirection d,
               bool verbose) {
  subprocess = new Subprocess(command);
  int p[2];
  if(pipe(p) < 0)
    throw IOError("creating pipe for " + command[0], errno);
  switch(d) {
  case ReadFromPipe: subprocess->addChildFD(1, p[1], p[0]); break;
  case WriteToPipe: subprocess->addChildFD(0, p[0], p[1]); break;
  }
  subprocess->reporting(verbose, false);
  subprocess->run();
  switch(d) {
  case ReadFromPipe:
    path = "pipe from " + command[0];
    fp = fdopen(p[0], "r");
    break;
  case WriteToPipe:
    path = "pipe to " + command[0];
    fp = fdopen(p[1], "w");
    break;
  }
  if(!fp)
    throw IOError("fdopen", errno);
  closeFile = true;
}

int IO::close(unsigned waitBehaviour) {
  FILE *fpSave = fp;
  fp = nullptr;
  if(fclose(fpSave) < 0) {
    if(abortOnError)
      abort();
    throw IOError("closing " + path);
  }
  return subprocess ? subprocess->wait(waitBehaviour) : 0;
}

bool IO::readline(std::string &line) {
  int c;
  line.clear();

  while((c = getc(fp)) != EOF && c != '\n')
    line += c;
  if(ferror(fp))
    readError();
  return line.size() || !feof(fp);
}

void IO::readlines(std::vector<std::string> &lines) {
  std::string line;
  lines.clear();

  while(readline(line))
    lines.push_back(line);
}

void IO::readall(std::string &file) {
  int c;
  file.clear();
  while((c = getc(fp)) != EOF)
    file += c;
  if(ferror(fp))
    readError();
}

void IO::write(const std::string &s) {
  fwrite(s.data(), 1, s.size(), fp);
  if(ferror(fp))
    writeError();
}

int IO::writef(const char *format, ...) {
  va_list ap;
  int rc;

  va_start(ap, format);
  rc = vfprintf(fp, format, ap);
  va_end(ap);
  if(rc < 0)
    writeError();
  return rc;
}

int IO::vwritef(const char *format, va_list ap) {
  int rc = vfprintf(fp, format, ap);
  if(rc < 0)
    writeError();
  return rc;
}

void IO::flush() {
  if(fflush(fp) < 0)
    writeError();
}

int IO::width() const {
  int fd = fileno(fp);
  if(!isatty(fd))
    return 0;
  struct winsize ws;
  if(ioctl(fd, TIOCGWINSZ, &ws) < 0)
    return 0;
  return ws.ws_col;
}

void IO::readError() {
  if(abortOnError)
    abort();
  throw IOError("reading " + path, errno);
}

void IO::writeError() {
  if(abortOnError)
    abort();
  throw IOError("writing " + path, errno);
}

IO IO::out(stdout, "stdout");
IO IO::err(stderr, "stderr", true);
