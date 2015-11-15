//-*-C++-*-
// Copyright © 2014, 2015 Richard Kettlewell.
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
#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>

/** @file Database.h
 * @brief %Database support
 */

/** @brief End of binding information */
#define SQL_END 0

/** @brief Bind an @c int */
#define SQL_INT 1

/** @brief Bind an @c int64_t  */
#define SQL_INT64 2

/** @brief Bind a @c const @c std::string @c * as text */
#define SQL_STRING 3

/** @brief Bind a @c const @c std::string @c * as a blob */
#define SQL_BLOB 4

/** @brief Bind a @c const @c char @c * as text */
#define SQL_CSTRING 5

/** @brief A database handle */
class Database {
public:
  /** @brief A SQL statement */
  class Statement {
  public:
    /** @brief Create a statement
     * @param d Database
     * @throw DatabaseError if an error occurs
     */
    inline Statement(Database &d): db(d.db) {}

    /** @brief Create a statement, prepare it with a command and bind data it
     * @param d Database
     * @param cmd Command
     * @param ... Binding information
     * @throw DatabaseError if an error occurs
     *
     * Binding information is a series of type values followed by parameter
     * values and terminated by @ref SQL_END.  The type values are:
     *
     * - @ref SQL_INT, followed by an @c int parameter value.
     * - @ref SQL_INT64, followed by an @c int64_t parameter value.
     * - @ref SQL_STRING, followed by a @c const @c std::string @c * parameter value.
     * - @ref SQL_CSTRING, followed by a @c const @c char @c * parameter value.
     */
    Statement(Database &d, const char *cmd, ...);

    /** @brief Prepare a statement with a command and bind data to it
     * @param cmd Command
     * @param ... Binding information
     * @throw DatabaseError if an error occurs
     *
     * Binding information is a series of type values followed by parameter
     * values and terminated by @ref SQL_END.  The type values are:
     *
     * - @ref SQL_INT, followed by an @c int parameter value.
     * - @ref SQL_INT64, followed by an @c int64_t parameter value.
     * - @ref SQL_STRING, followed by a @c const @c std::string @c * parameter value.
     * - @ref SQL_CSTRING, followed by a @c const @c char @c * parameter value.
     */
    void prepare(const char *cmd, ...);

    /** @brief Fetch the next row
     * @return @c true if a row is available, otherwise @c false
     * @throw DatabaseError if an error occurs
     * @throw DatabaseBusy if the database is busy
     */
    bool next();

    /** @brief Get an @c int value
     * @param col Column number from 0
     * @return Integer value
     */
    int get_int(int col);

    /** @brief Get an @c int64 value
     * @param col Column number from 0
     * @return Integer value
     */
    sqlite_int64 get_int64(int col);

    /** @brief Get a @c std::string value as text
     * @param col Column number from 0
     * @return String value
     */
    std::string get_string(int col);

    /** @brief Get a @c std::string value as a blob
     * @param col Column number from 0
     * @return String value
     */
    std::string get_blob(int col);

    /** @brief Destroy a statement */
    ~Statement();

  private:
    /** @brief Prepare and bind a statment
     * @param cmd Command
     * @param ap Binding information
     * @throw DatabaseError if an error occurs
     */
    void vprepare(const char *cmd, va_list ap);

    /** @brief Bind to a statement
     * @param ap Binding information
     * @throw DatabaseError if an error occurs
     *
     * Depends on @ref param being initialized so only callable from or after
     * @ref vprepare and its callers.
     */
    void vbind(va_list ap);

    /** @brief Raise an error
     * @param description Context for error
     * @param rc Error code
     * @throw DatabaseError
     * @throw DatabaseBusy
     */
    inline void error [[noreturn]] (const std::string &description,
                                    int rc) {
      Database::error(db, description, rc);
    }

    /** @brief Underlying statement handle */
    sqlite3_stmt *stmt = nullptr;

    /** @brief Underlying database handle */
    sqlite3 *db = nullptr;

    /** @brief Next parameter index
     * Only meaningful after @ref vprepare (and its callers)
     */
    int param = 0;
  };

  /** @brief Create a database object
   * @param path Path to database
   * @param rw Read-write mode
   * @throw DatabaseError if an error occurs
   *
   * In read-write mode, the database is created if it does not exist.
   */
  Database(const std::string &path, bool rw=true);

  /** @brief Test whether a table exists
   * @param name Table name
   * @return @c true if table exists, @c false otherwise
   * @throw DatabaseError if an error occurs
   * @throw DatabaseBusy if the database is busy
   */
  bool hasTable(const std::string &name);

  /** @brief Execute a simple command
   * @param cmd Command to execute
   */
  void execute(const char *cmd);

  /** @brief Begin a transaction */
  void begin();

  /** @brief Commit a transaction */
  void commit();

  /** @brief Abandon a transaction */
  void rollback();

  /** @brief Destructor */
  ~Database();

private:
  /** @brief Underlying database handle */
  sqlite3 *db;

  /** @brief Raise an error
   * @param description Context for error
   * @param rc Error code
   * @throw DatabaseError
   * @throw DatabaseBusy
   */
  inline void error [[noreturn]] (const std::string &description,
                                  int rc) {
    error(db, description, rc);
  }

  /** @brief Raise an error
   * @param db Database handle
   * @param description Context for error
   * @param rc Error code
   * @throw DatabaseError
   * @throw DatabaseBusy
   *
   * Throws a @ref DatabaseError, unless @p rc is @c SQLITE_BUSY, in which case
   * @ref DatabaseBusy is thrown instead.
   */
  static void error [[noreturn]] (sqlite3 *db,
                                  const std::string &description,
                                  int rc);

  friend class Database::Statement;
};

#endif /* DATABASE_H */
