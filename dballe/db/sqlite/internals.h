/*
 * db/sqlite/internals - Implementation infrastructure for the SQLite DB connection
 *
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef DBALLE_DB_SQLITE_INTERNALS_H
#define DBALLE_DB_SQLITE_INTERNALS_H

#include <dballe/db/db.h>
#include <dballe/db/sql.h>
#include <sqlite3.h>

namespace dballe {
namespace db {
struct SQLiteStatement;

/**
 * Report an SQLite error
 */
struct error_sqlite : public db::error
{
    std::string msg;

    /**
     * Copy informations from the ODBC diagnostic record to the dba error
     * report
     */
    error_sqlite(sqlite3* db, const std::string& msg);
    error_sqlite(const std::string& dbmsg, const std::string& msg);
    ~error_sqlite() throw () {}

    wreport::ErrorCode code() const throw () { return wreport::WR_ERR_ODBC; }

    virtual const char* what() const throw () { return msg.c_str(); }

    static void throwf(sqlite3* db, const char* fmt, ...) WREPORT_THROWF_ATTRS(2, 3);
};

/// Database connection
class SQLiteConnection : public Connection
{
protected:
    /// Database connection
    sqlite3* db = nullptr;

    void impl_exec_noargs(const std::string& query) override;
    void init_after_connect();

public:
    SQLiteConnection();
    SQLiteConnection(const SQLiteConnection&) = delete;
    SQLiteConnection(const SQLiteConnection&&) = delete;
    ~SQLiteConnection();

    SQLiteConnection& operator=(const SQLiteConnection&) = delete;

    void open_file(const std::string& pathname, int flags=SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    void open_memory(int flags=SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    void open_private_file(int flags=SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    std::unique_ptr<Transaction> transaction() override;
    std::unique_ptr<Statement> statement() override;
    std::unique_ptr<SQLiteStatement> sqlitestatement();

    /// Check if the database contains a table
    bool has_table(const std::string& name) override;

    /**
     * Get a value from the settings table.
     *
     * Returns the empty string if the table does not exist.
     */
    std::string get_setting(const std::string& key) override;

    /**
     * Set a value in the settings table.
     *
     * The table is created if it does not exist.
     */
    void set_setting(const std::string& key, const std::string& value) override;

    /// Drop the settings table
    void drop_settings() override;

    /**
     * Delete a table in the database if it exists, otherwise do nothing.
     */
    void drop_table_if_exists(const char* name) override;

    /**
     * Delete a sequence in the database if it exists, otherwise do nothing.
     */
    void drop_sequence_if_exists(const char* name) override;

    /**
     * Return LAST_INSERT_ID or LAST_INSER_ROWID or whatever is appropriate for
     * the current database, if supported.
     *
     * If not supported, an exception is thrown.
     */
    int get_last_insert_id() override;

    void add_datetime(Querybuf& qb, const int* dt) const override;

    /// Wrap sqlite3_exec, without a callback
    void wrap_sqlite3_exec(const std::string& query);
    void wrap_sqlite3_exec_nothrow(const std::string& query) noexcept;
};

/// SQLite statement
struct SQLiteStatement : public Statement
{
    const SQLiteConnection& conn;
    sqlite3_stmt *stm = nullptr;

    SQLiteStatement(SQLiteConnection& conn);
    SQLiteStatement(const SQLiteStatement&) = delete;
    SQLiteStatement(const SQLiteStatement&&) = delete;
    ~SQLiteStatement();
    SQLiteStatement& operator=(const SQLiteStatement&) = delete;

    void bind_in(int idx, const int& val) override;
    //void bind_in(int idx, const int& val, const SQLLEN& ind);
    void bind_in(int idx, const unsigned& val) override;
    //void bind_in(int idx, const unsigned& val, const SQLLEN& ind);
    void bind_in(int idx, const unsigned short& val) override;
    //void bind_in(int idx, const unsigned short& val, const SQLLEN& ind);
    void bind_in(int idx, const char* val) override;
    //void bind_in(int idx, const char* val, const SQLLEN& ind);
    //void bind_in(int idx, const SQL_TIMESTAMP_STRUCT& val);

    //void prepare(const char* query);
    //void prepare(const char* query, int qlen);
    void prepare(const std::string& query) override;

    void set_cursor_forward_only() override;
    void execute_ignoring_results() override;

#if 0
    /// @return SQLExecute's result
    int execute();
    /// @return SQLExecute's result
    int exec_direct(const char* query);
    /// @return SQLExecute's result
    int exec_direct(const char* query, int qlen);

    /// @return SQLExecute's result
    int execute_and_close();
    /// @return SQLExecute's result
    int exec_direct_and_close(const char* query);
    /// @return SQLExecute's result
    int exec_direct_and_close(const char* query, int qlen);

    /**
     * @return the number of columns in the result set (or 0 if the statement
     * did not return columns)
     */
    int columns_count();
    bool fetch();
    bool fetch_expecting_one();
    void close_cursor();
    void close_cursor_if_needed();
    /// Row count for select operations
    size_t select_rowcount();
    /// Row count for insert, delete and other non-select operations
    size_t rowcount();

    void set_cursor_static();
#endif
};

}
}
#endif

