#ifndef DBALLE_DB_DB_H
#define DBALLE_DB_DB_H

#include <dballe/fwd.h>
#include <dballe/cursor.h>
#include <dballe/db.h>
#include <dballe/db/fwd.h>
#include <dballe/db/defs.h>
#include <dballe/msg/fwd.h>
#include <dballe/sql/fwd.h>
#include <wreport/varinfo.h>
#include <wreport/var.h>
#include <string>
#include <memory>
#include <functional>

/** @file
 * @ingroup db
 *
 * Functions used to connect to DB-All.e and insert, query and delete data.
 */

namespace dballe {

namespace impl {

/// DBImportOptions with public constructor and copy, safe to use in dballe code but not accessible from the public API
struct DBImportOptions : public dballe::DBImportOptions
{
    DBImportOptions() = default;
    DBImportOptions(const DBImportOptions& o) = default;
    DBImportOptions(DBImportOptions&& o) = default;
    DBImportOptions& operator=(const DBImportOptions&) = default;
    DBImportOptions& operator=(DBImportOptions&&) = default;
};

/// DBInsertOptions with public constructor and copy, safe to use in dballe code but not accessible from the public API
struct DBInsertOptions : public dballe::DBInsertOptions
{
    DBInsertOptions() = default;
    DBInsertOptions(const DBInsertOptions& o) = default;
    DBInsertOptions(DBInsertOptions&& o) = default;
    DBInsertOptions& operator=(const DBInsertOptions&) = default;
    DBInsertOptions& operator=(DBInsertOptions&&) = default;
};

}

namespace db {

/// Format a db::Format value to a string
std::string format_format(Format format);

/// Parse a formatted db::Format value
Format format_parse(const std::string& str);


struct CursorStation : public dballe::CursorStation
{
    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    virtual unsigned test_iterate(FILE* dump=0) = 0;
};

struct CursorStationData : public dballe::CursorStationData
{
    /// Get the database that created this cursor
    virtual std::shared_ptr<db::Transaction> get_transaction() const = 0;

    /**
     * Return an integer value that can be used to refer to the current
     * variable for attribute access
     */
    virtual int attr_reference_id() const = 0;

    /**
     * Query/return the attributes for the current value of this cursor
     */
    virtual void query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read=false) = 0;

    /// Insert/update attributes for the current variable
    virtual void insert_attrs(const Values& attrs);

    /// Remove attributes for the current variable
    virtual void remove_attrs(const db::AttrList& attrs);

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    virtual unsigned test_iterate(FILE* dump=0) = 0;
};

struct CursorData : public dballe::CursorData
{
    /// Get the database that created this cursor
    virtual std::shared_ptr<db::Transaction> get_transaction() const = 0;

    /**
     * Return an integer value that can be used to refer to the current
     * variable for attribute access
     */
    virtual int attr_reference_id() const = 0;

    /**
     * Query/return the attributes for the current value of this cursor
     */
    virtual void query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read=false) = 0;

    /// Insert/update attributes for the current variable
    virtual void insert_attrs(const Values& attrs);

    /// Remove attributes for the current variable
    virtual void remove_attrs(const db::AttrList& attrs);

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    virtual unsigned test_iterate(FILE* dump=0) = 0;
};

struct CursorSummary : public dballe::CursorSummary
{
    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    virtual unsigned test_iterate(FILE* dump=0) = 0;
};


class Transaction : public dballe::Transaction
{
public:
    virtual ~Transaction() {}

    /**
     * Clear state information cached during the transaction.
     *
     * This can be used when, for example, a command that deletes data is
     * issued that would invalidate ID information cached inside the
     * transaction.
     */
    virtual void clear_cached_state() = 0;

    /**
     * Query attributes on a station value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param dest
     *   The function that will be called on each resulting attribute
     */
    virtual void attr_query_station(int data_id, std::function<void(std::unique_ptr<wreport::Var>)> dest) = 0;

    /**
     * Query attributes on a data value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param dest
     *   The function that will be called on each resulting attribute
     */
    virtual void attr_query_data(int data_id, std::function<void(std::unique_ptr<wreport::Var>)> dest) = 0;

    /**
     * Insert new attributes on a station value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param attrs
     *   The attributes to be added
     */
    virtual void attr_insert_station(int data_id, const Values& attrs) = 0;

    /**
     * Insert new attributes on a data value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param attrs
     *   The attributes to be added
     */
    virtual void attr_insert_data(int data_id, const Values& attrs) = 0;

    /**
     * Delete attributes from a station value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param attrs
     *   Array of WMO codes of the attributes to delete.  If empty, all attributes
     *   associated to the value will be deleted.
     */
    virtual void attr_remove_station(int data_id, const db::AttrList& attrs) = 0;

    /**
     * Delete attributes from a data value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param attrs
     *   Array of WMO codes of the attributes to delete.  If empty, all attributes
     *   associated to the value will be deleted.
     */
    virtual void attr_remove_data(int data_id, const db::AttrList& attrs) = 0;

    /**
     * Update the repinfo table in the database, with the data found in the given
     * file.
     *
     * @param repinfo_file
     *   The name of the CSV file with the report type information data to load.
     *   The file is in CSV format with 6 columns: report code, mnemonic id,
     *   description, priority, descriptor, table A category.
     *   If repinfo_file is NULL, then the default of /etc/dballe/repinfo.csv is
     *   used.
     * @retval added
     *   The number of repinfo entries that have been added
     * @retval deleted
     *   The number of repinfo entries that have been deleted
     * @retval updated
     *   The number of repinfo entries that have been updated
     */
    virtual void update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated) = 0;

    /**
     * Dump the entire contents of the database to an output stream
     */
    virtual void dump(FILE* out) = 0;
};

class DB: public dballe::DB
{
public:
    static db::Format get_default_format();
    static void set_default_format(db::Format format);

    /**
     * Create from a SQLite file pathname
     *
     * @param pathname
     *   The pathname to a SQLite file
     */
    static std::shared_ptr<DB> connect_from_file(const char* pathname);

    /**
     * Create an in-memory database
     */
    static std::shared_ptr<DB> connect_memory();

    /**
     * Start a test session with DB-All.e
     */
    static std::shared_ptr<DB> connect_test();

    /**
     * Create a database from an open Connection
     */
    static std::shared_ptr<DB> create(std::unique_ptr<sql::Connection> conn);

    /**
     * Return TRUE if the string looks like a DB URL
     *
     * @param str
     *   The string to test
     * @return
     *   true if it looks like a URL, else false
     */
    static bool is_url(const char* str);

    /// Return the format of this DB
    virtual db::Format format() const = 0;

    /**
     * Remove all our traces from the database, if applicable.
     *
     * After this has been called, all other DB methods except for reset() will
     * fail.
     */
    virtual void disappear() = 0;

    /**
     * Reset the database, removing all existing Db-All.e tables and re-creating them
     * empty.
     *
     * @param repinfo_file
     *   The name of the CSV file with the report type information data to load.
     *   The file is in CSV format with 6 columns: report code, mnemonic id,
     *   description, priority, descriptor, table A category.
     *   If repinfo_file is NULL, then the default of /etc/dballe/repinfo.csv is
     *   used.
     */
    virtual void reset(const char* repinfo_file=0) = 0;

    /**
     * Same as transaction(), but the resulting transaction will throw an
     * exception if commit is called. Used for tests.
     */
    virtual std::shared_ptr<dballe::db::Transaction> test_transaction(bool readonly=false) = 0;

    /**
     * Perform database cleanup operations.
     *
     * Orphan values are currently:
     * \li context values for which no data exists
     * \li station values for which no context exists
     *
     * Depending on database size, this routine can take a few minutes to execute.
     */
    virtual void vacuum() = 0;

    /**
     * Query attributes on a station value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param dest
     *   The function that will be called on each resulting attribute
     */
    virtual void attr_query_station(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest);

    /**
     * Query attributes on a data value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param dest
     *   The function that will be called on each resulting attribute
     */
    virtual void attr_query_data(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest);

    /**
     * Insert new attributes on a station value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param attrs
     *   The attributes to be added
     */
    void attr_insert_station(int data_id, const Values& attrs);

    /**
     * Insert new attributes on a data value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param attrs
     *   The attributes to be added
     */
    void attr_insert_data(int data_id, const Values& attrs);

    /**
     * Delete attributes from a station value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param attrs
     *   Array of WMO codes of the attributes to delete.  If empty, all attributes
     *   associated to the value will be deleted.
     */
    void attr_remove_station(int data_id, const db::AttrList& attrs);

    /**
     * Delete attributes from a data value
     *
     * @param data_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the value
     * @param attrs
     *   Array of WMO codes of the attributes to delete.  If empty, all attributes
     *   associated to the value will be deleted.
     */
    void attr_remove_data(int data_id, const db::AttrList& attrs);

    /**
     * Dump the entire contents of the database to an output stream
     */
    void dump(FILE* out);

    /// Print informations about the database to the given output stream
    virtual void print_info(FILE* out);

    /// Return the default repinfo file pathname
    static const char* default_repinfo_file();
};

}
}
#endif
