// -*-C++-*-
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
   * @param abortOnError_ Force call to abort() on error
   */
  IO(FILE *fp_,
     const std::string &path_,
     bool abortOnError_ = false): fp(fp_),
                                  path(path_),
                                  subprocess(NULL),
                                  closeFile(false),
                                  abortOnError(abortOnError_) {}

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
   */
  void popen(const std::vector<std::string> &command,
             PipeDirection d);

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

  /** @brief Standard error */
  static IO err;

private:
  FILE *fp;
  std::string path;
  Subprocess *subprocess;
  bool closeFile;
  bool abortOnError;

  void readError();
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

  /** @brief Destructor */
  ~Directory();

  /** @brief Open a directory
   * @param path Directory to open
   */
  void open(const std::string &path);

  /** @brief Get one filename
   * @param name Where to put filename
   * @return true if a filename returned, false at end.
   *
   * Only the basename is supplied.
   */
  bool get(std::string &name) const;

  /** @brief Get all filenames
   * @param files Where to put filenames
   */
  void get(std::vector<std::string> &files) const;

  /** @brief Get all filenames from a directory
   * @param path Directory to read
   * @param files Where to put filenames
   */
  static void getFiles(const std::string &path,
                       std::vector<std::string> &files);
private:
  DIR *dp;
  std::string path;
};

/** @brief Create a directory and its parents
 * @param path Directory to create
 * @param mode Permissions for new directory
 */
void makeDirectory(const std::string &path,
                   mode_t mode = 0777);

#endif /* IO_H */
