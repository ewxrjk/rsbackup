#include <config.h>
#include "IO.h"
#include "Errors.h"
#include <cerrno>
#include <cstdarg>
#include <sys/types.h>
#include <sys/wait.h>

// StdioFile ------------------------------------------------------------------

StdioFile::~StdioFile() {
  if(fp)
    fclose(fp);
  wait();
}

void StdioFile::open(const std::string &path_, const std::string &mode) {
  if(!(fp = fopen(path_.c_str(), mode.c_str())))
    throw IOError("opening " + path_, errno);
  path = path_;
}

void StdioFile::popen(const std::vector<std::string> &command,
                      PipeDirection d) {
  int p[2], fd;
  std::vector<const char *> args;
  size_t n;
  
  for(n = 0; n < command.size(); ++n)
    args.push_back(command[n].c_str());
  args.push_back(NULL);
  if(pipe(p) < 0)
    throw IOError("creating pipe for " + command[0], errno);
  switch(pid = fork()) {
  case -1:
    ::close(p[0]);
    ::close(p[1]);
    throw IOError("creating subprocess for " + command[0], errno);
  case 0: {
    switch(d) {
    case ReadFromPipe: fd = 1; break;
    case WriteToPipe: fd = 0; break;
    default: _exit(-1);
    }
    if(dup2(p[fd], fd) < 0) { perror("dup2"); _exit(-1); }
    ::close(p[0]);
    ::close(p[1]);
    execvp(args[0], (char **)&args[0]);
    perror(args[0]);
    _exit(-1);
  }
  }
  switch(d) {
  case ReadFromPipe:
    path = "pipe from " + command[0];
    fp = fdopen(p[0], "r");
    ::close(p[1]);
    break;
  case WriteToPipe:
    path = "pipe to " + command[0];
    fp = fdopen(p[1], "w");
    ::close(p[0]);
    break;
  }
  if(!fp)
    throw IOError("fdopen", errno);
}

int StdioFile::wait() {
  if(pid != -1) {
    int w;
    pid_t p;
    while((p = waitpid(pid, &w, 0)) < 0 && errno == EINTR)
      ;
    pid = -1;
    if(p < 0)
      throw IOError("waiting for subprocess");
    return w;
  } else
    return 0;
}

int StdioFile::close() {
  FILE *fpSave = fp;
  fp = NULL;
  if(fclose(fpSave) < 0)
    throw IOError("closing " + path);
  return wait();
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
