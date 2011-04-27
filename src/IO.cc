#include <config.h>
#include "IO.h"
#include "Errors.h"
#include "Subprocess.h"
#include <cerrno>
#include <cstdarg>
#include <sys/types.h>
#include <sys/wait.h>

// StdioFile ------------------------------------------------------------------

StdioFile::~StdioFile() {
  if(fp)
    fclose(fp);
  if(subprocess)
    delete subprocess;
}

void StdioFile::open(const std::string &path_, const std::string &mode) {
  if(!(fp = fopen(path_.c_str(), mode.c_str())))
    throw IOError("opening " + path_, errno);
  path = path_;
}

void StdioFile::popen(const std::vector<std::string> &command,
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
}

int StdioFile::close(bool checkStatus) {
  FILE *fpSave = fp;
  fp = NULL;
  if(fclose(fpSave) < 0)
    throw IOError("closing " + path);
  return subprocess ? subprocess->wait(checkStatus) : 0;
}

bool StdioFile::readline(std::string &line) {
  int c;
  line.clear();

  while((c = getc(fp)) != EOF && c != '\n')
    line += c;
  if(ferror(fp))
    throw IOError("reading " + path);
  return line.size() || !feof(fp);
}

void StdioFile::readlines(std::vector<std::string> &lines) {
  std::string line;
  lines.clear();
  
  while(readline(line))
    lines.push_back(line);
}

void StdioFile::write(const std::string &s) {
  fwrite(s.data(), 1, s.size(), fp);
  if(ferror(fp))
    throw IOError("writing " + path);
}

int StdioFile::writef(const char *format, ...) {
  va_list ap;
  int rc;

  va_start(ap, format);
  rc = vfprintf(fp, format, ap);
  va_end(ap);
  if(rc < 0)
    throw IOError("writing " + path);
  return rc;
}

void StdioFile::flush() {
  if(fflush(fp) < 0)
    throw IOError("writing " + path);
}

// Directory ------------------------------------------------------------------

Directory::~Directory() {
  if(dp)
    closedir(dp);
}

void Directory::open(const std::string &path_) {
  if(!(dp = opendir(path_.c_str())))
    throw IOError("opening " + path_);
  path = path_;
}

bool Directory::get(std::string &name) const {
  errno = 0;
  struct dirent *de = readdir(dp);
  if(de) {
    name = de->d_name;
    return true;
  } else {
    if(errno)
      throw IOError("reading " + path);
    return false;
  }
}
