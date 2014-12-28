// -*-C++-*-
// Copyright © 2011, 2012, 2014 Richard Kettlewell.
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
#ifndef IO_H
#define IO_H
/** @file IO.h
 * @brief I/O support
 */

#include <string>
#include <vector>
#include <cstdio>
#include <cstdarg>

#include <sys/types.h>
#include <dirent.h>

/** @brief Possible directions of pipe
 *
 * Both are given from the parent's point of view.
 */
enum PipeDirection {
  /** @brief Parent reads, child writes */
  ReadFromPipe,

  /** @brief Parent writes, child reads */
  WriteToPipe,
};

class Subprocess;

/** @brief RAII-friendly I/O class
 *
 * Members will throw IOError if anything goes wrong.
 */
class IO {
public:
  /** @brief Constructor */
  IO(): fp(NULL),
        subprocess(NULL),
        closeFile(false),
        abortOnError(false) {}

  /** @brief Constructor
   * @param fp_ Underlying stream
   * @param path_ Filename for error messages
   */
  IO(FILE *fp_,
     const std::string &path_): fp(fp_),
                                  path(path_),
                                  subprocess(NULL),
                                  closeFile(false),
                                  abortOnError(false) {}

  /** @brief Destructor
   *
   * Closes underlying stream, discarding any errors, if it is still open.
   */
  ~IO();

  /** @brief Open a file
   * @param path File to open
   * @param mode Open mode as per @c fopen()
   */
  void open(const std::string &path, const std::string &mode);

  /** @brief Open a pipe to/from a subprocess
   * @param command Command to execute in subprocess
   * @param d Pipe direction
   * @param verbose If true, report command and environment to stderr
   */
  void popen(const std::vector<std::string> &command,
             PipeDirection d,
             bool verbose);

  /** @brief Close file
   * @param checkStatus Throw on abnormal subprocess termination
   * @return Wait status for a subprocess, or 0
   *
   * If the file is a pipe and @p checkStatus is true then an
   * exception is thrown if the subprocess indicates an error with its wait
   * status.  The wait status is returned.  If the file is not a pipe then the
   * return value is 0.
   */
  int close(bool checkStatus = true);

  /** @brief Read one line
   * @param line Where to put line
   * @return true on success, false at eof
   *
   * Strips the newline.
   */
  bool readline(std::string &line);

  /** @brief Read all lines
  * @param lines Where to put lines
  *
  * Strips the newlines.
  */
  void readlines(std::vector<std::string> &lines);

  /** @brief Write a string
   * @param s String to write
   */
  void write(const std::string &s);

  /** @brief Write a formatted string
   * @param format Format string as per @c printf()
   * @param ... Arguments
   * @return Number of bytes written
   */
  int writef(const char *format, ...);

  /** @brief Write a formatted string
   * @param format Format string as per @c printf()
   * @param ap Arguments
   * @return Number of bytes written
   */
  int vwritef(const char *format, va_list ap);

  /** @brief Flush any buffered writes */
  void flush();

  /** @brief Standard output */
  static IO out;

  /** @brief Standard error
   *
   * If writes to this stream fail, the process will be aborted.
   */
  static IO err;

private:
  /** @brief Constructor with special error handling
   * @param fp_ Underlying stream
   * @param path_ Filename for error messages
   * @param abortOnError_ If @c true, write errors will abort process
   *
   * This is used for @ref err.
   */
  IO(FILE *fp_,
     const std::string &path_,
     bool abortOnError_): fp(fp_),
                          path(path_),
                          subprocess(NULL),
                          closeFile(false),
                          abortOnError(abortOnError_) {}

  /** @brief Underlying stdio stream */
  FILE *fp;

  /** @brief Path to open file */
  std::string path;

  /** @brief Subprocess handler or @c NULL
   *
   * Used by @ref popen().
   */
  Subprocess *subprocess;

  /** @brief If @c true, close @ref fp in destructor */
  bool closeFile;

  /** @brief If @c true, abort on error */
  bool abortOnError;

  /** @brief Called when a read error occurs */
  void readError();

  /** @brief Called when a write error occurs */
  void writeError();
};

/** @brief RAII-friendly directory reader
 *
 * Members will throw IOError if anything goes wrong.
 */
class Directory {
public:
  /** @brief Constructor */
  Directory(): dp(NULL) {}

  /** @brief Destructor
   *
   * Closes the directory if it is open.
   */
  ~Directory();

  /** @brief Open a directory
   * @param path Directory to open
   * @throws IOError if @p path cannot be opened as a directory
   *
   * The directory must not already be open.
   */
  void open(const std::string &path);

  /** @brief Close a directory
   *
   * Does nothing if the directory is not open.
   */
  void close();

  /** @brief Get one filename
   * @param name Where to put filename
   * @return true if a filename returned, false at end.
   *
   * Only the basename is supplied.
   *
   * The directory must be open.
   */
  bool get(std::string &name) const;

  /** @brief Get all filenames
   * @param files Where to put filenames
   *
   * The directory must be open.
   */
  void get(std::vector<std::string> &files) const;

  /** @brief Get all filenames from a directory
   * @param path Directory to read
   * @param files Where to put filenames
   */
  static void getFiles(const std::string &path,
                       std::vector<std::string> &files);
private:
  /** @brief Underlying directory handle */
  DIR *dp;

  /** @brief Path to directory */
  std::string path;
};

/** @brief Create a directory and its parents
 * @param path Directory to create
 * @param mode Permissions for new directory
 */
void makeDirectory(const std::string &path,
                   mode_t mode = 0777);

#endif /* IO_H */
