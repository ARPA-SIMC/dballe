/*
 * dballe/db - Archive for point-based meteorological data
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_DB_H
#define DBA_DB_H

#include <dballe/core/defs.h>
#include <wreport/varinfo.h>
#include <wreport/var.h>
#include <vector>
#include <string>
#include <map>
#include <memory>

/** @file
 * @ingroup db
 *
 * Functions used to connect to DB-All.e and insert, query and delete data.
 */

/**
 * Flags controlling message import
 * @{
 */
/* Import the attributes. */
#define DBA_IMPORT_ATTRS		1
/* Attempt to merge pseudoana extra information into the existing ones. */
#define DBA_IMPORT_FULL_PSEUDOANA	2
/* Message data will overwrite existing values; otherwise, trying to insert
 * existing data will cause an error. */
#define DBA_IMPORT_OVERWRITE		8
/// @}

namespace dballe {
struct Record;
typedef Record Query;
struct Msg;
struct Msgs;
struct MsgConsumer;
struct DB;

namespace db {
struct Connection;

/**
 * Supported formats
 */
typedef enum {
    V5 = 0,
    V6 = 1,
    MEM = 2,
    MESSAGES = 3,
} Format;

/// Base exception for database errors
struct error : public wreport::error {};

/**
 * Simple typedef to make typing easier
 */
typedef std::vector<wreport::Varcode> AttrList;

class Cursor
{
public:
    virtual ~Cursor();

    /// Get the database that created this cursor
    virtual DB& get_db() const = 0;

    /**
     * Get the number of rows still to be fetched
     *
     * @return
     *   The number of rows still to be queried.  The value is undefined if no
     *   query has been successfully peformed yet using this cursor.
     */
    virtual int remaining() const = 0;

    /**
     * Get a new item from the results of a query
     *
     * @returns
     *   true if a new record has been read, false if there is no more data to read
     */
    virtual bool next() = 0;

    /// Discard the results that have not been read yet
    virtual void discard_rest() = 0;

    /**
     * Fill in a record with the contents of a dba_db_cursor
     *
     * @param rec
     *   The record where to store the values
     */
    virtual void to_record(Record& rec) = 0;

    /// Get the station identifier
    virtual int get_station_id() const = 0;

    /// Get the station latitude
    virtual double get_lat() const = 0;

    /// Get the station longitude
    virtual double get_lon() const = 0;

    /// Get the station identifier, or NULL if missing
    virtual const char* get_ident(const char* def=0) const = 0;

    /// Get the report name
    virtual const char* get_rep_memo(const char* def=0) const = 0;

    /// Get the level
    virtual Level get_level() const = 0;

    /// Get the level
    virtual Trange get_trange() const = 0;

    /// Get the datetime
    virtual void get_datetime(int (&dt)[6]) const = 0;

    /// Get the variable code
    virtual wreport::Varcode get_varcode() const = 0;

    /// Get the variable
    virtual wreport::Var get_var() const = 0;

    /**
     * Return an integer value that can be used to refer to the current
     * variable for attribute access
     */
    virtual int attr_reference_id() const = 0;

    /**
     * Query attributes for the current variable
     */
    virtual unsigned query_attrs(const AttrList& qcs, Record& attrs) = 0;

    /**
     * Insert/overwrite new attributes for the current variable
     *
     * @param attrs
     *   The record with the attributes to be added
     */
    virtual void attr_insert(const Record& attrs) = 0;

    /**
     * Delete attributes for the current variable
     *
     * @param qcs
     *   Array of WMO codes of the attributes to delete.  If empty, all attributes
     *   associated to id_data will be deleted.
     */
    virtual void attr_remove(const AttrList& qcs) = 0;

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    virtual unsigned test_iterate(FILE* dump=0);
};

}

class DB
{
public:
    virtual ~DB();

    static db::Format get_default_format();
    static void set_default_format(db::Format format);

    /**
     * Start a session with DB-All.e
     *
     * @param dsn
     *   The ODBC DSN of the database to use
     * @param user
     *   The user name to use to connect to the DSN
     * @param password
     *   The password to use to connect to the DSN.  To specify an empty password,
     *   pass "" or NULL
     * @return
     *   The new DB object
     */
    static std::unique_ptr<DB> connect(const char* dsn, const char* user, const char* password);

    /**
     * Create from a SQLite file pathname
     *
     * @param pathname
     *   The pathname to a SQLite file
     */
    static std::unique_ptr<DB> connect_from_file(const char* pathname);

    /**
     * Create from an url-like specification, that can be:
     * 
     * @l sqlite:[//]foo.sqlite
     * @l odbc://[user[:pass]@]dsn
     * @l test:[//]
     *
     * @param url
     *   The url-like connection descriptor
     */
    static std::unique_ptr<DB> connect_from_url(const char* url);

    /**
     * Create an in-memory database
     */
    static std::unique_ptr<DB> connect_memory(const std::string& arg = std::string());

    /**
     * Start a test session with DB-All.e
     *
     * Take information from the environment (@see dba_db_create_from_env) and
     * default to ./test.sqlite if nothing is specified.
     */
    static std::unique_ptr<DB> connect_test();

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
    virtual void reset(const char* repinfo_file = 0) = 0;

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
     * Get a mapping between rep_memo and their priorities
     */
    virtual std::map<std::string, int> get_repinfo_priorities() = 0;

    /**
     * Insert a record into the database
     *
     * The reference IDs of all variables that were inserted will be stored in
     * memory until the next insert operation. To insert attributes related to
     * one of the variables just inserted, just call attr_insert() without the
     * reference_id parameter.
     *
     * @param rec
     *   The record to insert.
     * @param can_replace
     *   If true, then existing data can be rewritten, else data can only be added.
     * @param station_can_add
     *   If false, it will not create a missing station record, and only data
     *   for existing stations can be added. If true, then if we are inserting
     *   data for a station that does not yet exists in the database, it will
     *   be created.
     */
    virtual void insert(const Query& rec, bool can_replace, bool station_can_add) = 0;

    /**
     * Return the station id for the last data that was inserted.
     */
    virtual int last_station_id() const = 0;

    /**
     * Remove data from the database
     *
     * @param rec
     *   The record with the query data (see technical specifications, par. 1.6.4
     *   "parameter output/input") to select the items to be deleted
     */
    virtual void remove(const Query& rec) = 0;

    /**
     * Remove all data from the database.
     *
     * This is faster than remove() with an empty record, and unlike reset() it
     * preserves existing report information.
     */
    virtual void remove_all() = 0;

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
     * Start a query on the station variables archive.
     *
     * The cursor will iterate over unique lat, lon, ident triples, and will
     * contain all station vars. If a station var exists twice on two different
     * networks, only one will be present: the one of the network with the
     * highest priority.
     *
     * @param query
     *   The record with the query data (see @ref dba_record_keywords)
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<db::Cursor> query_stations(const Query& query) = 0;

    /**
     * Query the database.
     *
     * When multiple values per variable are present, the results will be presented
     * in increasing order of priority.
     *
     * @param query
     *   The record with the query data (see technical specifications, par. 1.6.4
     *   "parameter output/input")
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<db::Cursor> query_data(const Query& rec) = 0;

    /**
     * Query a summary of what the result would be for a query.
     *
     * @param query
     *   The record with the query data (see technical specifications, par. 1.6.4
     *   "parameter output/input")
     * @return
     *   The cursor to use to iterate over the results. The results are the
     *   same as query_data, except that no context_id, datetime and value are
     *   provided, so it only gives all the available combinations of data
     *   contexts.
     */
    virtual std::unique_ptr<db::Cursor> query_summary(const Query& rec) = 0;

    /**
     * Query attributes
     *
     * @param reference_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the variable we query
     * @param id_var
     *   The varcode of the variable related to the attributes to retrieve.  See @ref vartable.h
     * @param qcs
     *   The WMO codes of the QC values requested.  If it is empty, then all values
     *   are returned.
     * @param attrs
     *   The Record that will hold the resulting attributes
     * @return
     *   Number of attributes returned in attrs
     */
    virtual unsigned query_attrs(int reference_id, wreport::Varcode id_var, const db::AttrList& qcs, Record& attrs) = 0;

    /**
     * Insert new attributes into the database, reusing the reference IDs stored by the last insert.
     *
     * @param id_var
     *   The varcode of the variable related to the attributes to add.  See @ref vartable.h
     * @param attrs
     *   The record with the attributes to be added
     * @param can_replace
     *   If true, then existing data can be rewritten, else data can only be added.
     */
    virtual void attr_insert(wreport::Varcode id_var, const Record& attrs) = 0;

    /**
     * Insert new attributes into the database.
     *
     * @param reference_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the variable we query
     * @param id_var
     *   The varcode of the variable related to the attributes to add.  See @ref vartable.h
     * @param attrs
     *   The record with the attributes to be added
     * @param can_replace
     *   If true, then existing data can be rewritten, else data can only be added.
     */
    virtual void attr_insert(int reference_id, wreport::Varcode id_var, const Record& attrs) = 0;

    /**
     * Delete QC data for the variable `var' in record `rec' (coming from a previous
     * dba_query)
     *
     * @param reference_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the variable we query
     * @param id_var
     *   The varcode of the variable related to the attributes to remove.  See @ref vartable.h
     * @param qcs
     *   Array of WMO codes of the attributes to delete.  If empty, all attributes
     *   associated to id_data will be deleted.
     */
    virtual void attr_remove(int reference_id, wreport::Varcode id_var, const db::AttrList& qcs) = 0;

    /**
     * Import a Msg message into the DB-All.e database
     *
     * @param db
     *   The DB-All.e database to write the data into
     * @param msg
     *   The Msg containing the data to import
     * @param repmemo
     *   Report mnemonic to which imported data belong.  If NULL is passed, then it
     *   will be chosen automatically based on the message type.
     * @param flags
     *   Customise different aspects of the import process.  It is a bitmask of the
     *   various DBA_IMPORT_* macros.
     */
    virtual void import_msg(const Msg& msg, const char* repmemo, int flags) = 0;

    /**
     * Import Msgs messages into the DB-All.e database
     *
     * @param db
     *   The DB-All.e database to write the data into
     * @param msgs
     *   The Msgs containing the data to import
     * @param repmemo
     *   Report mnemonic to which imported data belong.  If NULL is passed, then it
     *   will be chosen automatically based on the message type.
     * @param flags
     *   Customise different aspects of the import process.  It is a bitmask of the
     *   various DBA_IMPORT_* macros.
     */
    virtual void import_msgs(const Msgs& msgs, const char* repmemo, int flags);

    /**
     * Perform the query in `query', and return the results as a NULL-terminated
     * array of dba_msg.
     *
     * @param query
     *   The query to perform
     * @param cons
     *   The MsgsConsumer that will handle the resulting messages
     */
    virtual void export_msgs(const Query& query, MsgConsumer& cons) = 0;

    /**
     * Dump the entire contents of the database to an output stream
     */
    virtual void dump(FILE* out) = 0;

    /// Return the default repinfo file pathname
    static const char* default_repinfo_file();

protected:
    static std::unique_ptr<DB> instantiate_db(std::unique_ptr<db::Connection> conn);
};

}

/* vim:set ts=4 sw=4: */
#endif
