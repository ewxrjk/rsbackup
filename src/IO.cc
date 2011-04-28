#include <config.h>
#include "IO.h"
#include "Errors.h"
#include "Subprocess.h"
#include <cerrno>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>

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

void IO::popen(const std::vector<std::string> &command,
               PipeDirection d) {
  subprocess = new Subprocess(command);
  int p[2];
  if(pipe(p) < 0)
    throw IOError("creating pipe for " + command[0], errno);
  switch(d) {
  case ReadFromPipe: subprocess->addChildFD(1, p[1], p[0]); break;
  case WriteToPipe: subprocess->addChildFD(0, p[0], p[1]); break;
  }
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

int IO::close(bool checkStatus) {
  FILE *fpSave = fp;
  fp = NULL;
  if(fclose(fpSave) < 0) {
    if(abortOnError)
      abort();
    throw IOError("closing " + path);
  }
  return subprocess ? subprocess->wait(checkStatus) : 0;
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
