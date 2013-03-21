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

#include <dballe/db/v5/cursor.h>
#include <string>
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
/* Import datetime information as data to preserve their attributes. */
#define DBA_IMPORT_DATETIME_ATTRS	4
/* Message data will overwrite existing values; otherwise, trying to insert
 * existing data will cause an error. */
#define DBA_IMPORT_OVERWRITE		8
/// @}

/**
 * Constants used to define what values we should retrieve from a query
 */
/** Retrieve latitude and longitude */
#define DBA_DB_WANT_COORDS		(1 << 0)
/** Retrieve the mobile station identifier */
#define DBA_DB_WANT_IDENT		(1 << 1)
/** Retrieve the level information */
#define DBA_DB_WANT_LEVEL		(1 << 2)
/** Retrieve the time range information */
#define DBA_DB_WANT_TIMERANGE	(1 << 3)
/** Retrieve the date and time information */
#define DBA_DB_WANT_DATETIME	(1 << 4)
/** Retrieve the variable name */
#define DBA_DB_WANT_VAR_NAME	(1 << 5)
/** Retrieve the variable value */
#define DBA_DB_WANT_VAR_VALUE	(1 << 6)
/** Retrieve the report code */
#define DBA_DB_WANT_REPCOD		(1 << 7)
/** Retrieve the station ID */
#define DBA_DB_WANT_ANA_ID		(1 << 8)
/** Retrieve the context ID */
#define DBA_DB_WANT_CONTEXT_ID	(1 << 9)

/**
 * Values for query modifier flags
 */
/** When values from different reports exist on the same point, only report the
 * one from the report with the highest priority */
#define DBA_DB_MODIFIER_BEST		(1 << 0)
/** Tell the database optimizer that this is a query on a database with a big
 * pseudoana table (this serves to hint the MySQL optimizer, which would not
 * otherwise apply the correct strategy */
#define DBA_DB_MODIFIER_BIGANA		(1 << 1)
/** Remove duplicates in the results */
#define DBA_DB_MODIFIER_DISTINCT	(1 << 2)
/** Include the extra anagraphical data in the results */
#define DBA_DB_MODIFIER_ANAEXTRA	(1 << 3)
/** Do not include the extra anagraphical data in the results */
#define DBA_DB_MODIFIER_NOANAEXTRA	(1 << 4)
/** Do not bother sorting the results */
#define DBA_DB_MODIFIER_UNSORTED	(1 << 5)
/** Start geting the results as soon as they are available, without waiting for
 * the database to finish building the result set.  As a side effect, it is
 * impossible to know in advance the number of results.  Currently, it does not
 * work with the MySQL ODBC driver */
#define DBA_DB_MODIFIER_STREAM		(1 << 6)
/** Sort by rep_cod after ana_id, to ease reconstructing messages on export */
#define DBA_DB_MODIFIER_SORT_FOR_EXPORT	(1 << 7)

namespace dballe {
struct Record;
struct Msg;
struct Msgs;
struct MsgConsumer;

class DB
{
public:
    virtual ~DB();

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
    static std::auto_ptr<DB> connect(const char* dsn, const char* user, const char* password);

    /**
     * Create from a SQLite file pathname
     *
     * @param pathname
     *   The pathname to a SQLite file
     */
    static std::auto_ptr<DB> connect_from_file(const char* pathname);

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
    static std::auto_ptr<DB> connect_from_url(const char* url);

    /**
     * Start a test session with DB-All.e
     *
     * Take information from the environment (@see dba_db_create_from_env) and
     * default to ./test.sqlite if nothing is specified.
     */
    static std::auto_ptr<DB> connect_test();

    /**
     * Return TRUE if the string looks like a DB URL
     *
     * @param str
     *   The string to test
     * @return
     *   true if it looks like a URL, else false
     */
    static bool is_url(const char* str);

    /**
     * Reset the database, removing all existing DBALLE tables and re-creating them
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
     *   The number of repinfo entryes that have been added
     * @retval deleted
     *   The number of repinfo entryes that have been deleted
     * @retval updated
     *   The number of repinfo entryes that have been updated
     */
    virtual void update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated) = 0;

    /**
     * Get the report mnemonic from a report code
     */
    virtual const std::string& rep_memo_from_cod(int rep_cod) = 0;

    /**
     * Insert a record into the database
     *
     * In a record with the same phisical situation already exists, the function
     * fails.
     *
     * ana_id and context_id will be set in the record at the end of this function.
     *
     * @param rec
     *   The record to insert.
     * @param can_replace
     *   If true, then existing data can be rewritten, else data can only be added.
     * @param station_can_add
     *   If true, then it is allowed to add new station records to the database.
     *   Otherwise, data can be added only by reusing existing ones.
     */
    virtual void insert(Record& rec, bool can_replace, bool station_can_add) = 0;

    /**
     * Remove data from the database
     *
     * @param rec
     *   The record with the query data (see technical specifications, par. 1.6.4
     *   "parameter output/input") to select the items to be deleted
     */
    virtual void remove(const Record& rec) = 0;

    /**
     * Remove orphan values from the database.
     *
     * Orphan values are currently:
     * \li context values for which no data exists
     * \li station values for which no context exists
     *
     * Depending on database size, this routine can take a few minutes to execute.
     */
    virtual void remove_orphans() = 0;

    /**
     * Create and execute a database query.
     *
     * The results are retrieved by iterating the cursor.
     *
     * @param query
     *   The record with the query data (see technical specifications, par. 1.6.4
     *   "parameter output/input"
     * @param wanted
     *   The values wanted in output
     * @param modifiers
     *   Optional modifiers to ask for special query behaviours
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::auto_ptr<db::Cursor> query(const Record& query, unsigned int wanted, unsigned int modifiers) = 0;

    /**
     * Start a query on the station archive
     *
     * @param query
     *   The record with the query data (see @ref dba_record_keywords)
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::auto_ptr<db::Cursor> query_stations(const Record& query) = 0;

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
    virtual std::auto_ptr<db::Cursor> query_data(const Record& rec) = 0;

    /**
     * Query attributes
     *
     * @param id_context
     *   The database id of the context related to the attributes to retrieve
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
    virtual unsigned query_attrs(int id_context, wreport::Varcode id_var, const db::AttrList& qcs, Record& attrs) = 0;

    /**
     * Insert new attributes into the database.
     *
     * @param id_context
     *   The database id of the context related to the attributes to insert
     * @param id_var
     *   The varcode of the variable related to the attributes to add.  See @ref vartable.h
     * @param attrs
     *   The record with the attributes to be added
     * @param can_replace
     *   If true, then existing data can be rewritten, else data can only be added.
     */
    virtual void attr_insert_or_replace(int id_context, wreport::Varcode id_var, const Record& attrs, bool can_replace) = 0;

    /**
     * Insert new attributes into the database.
     *
     * If the same attribute exists for the same data, it is
     * overwritten
     *
     * @param id_context
     *   The database id of the context related to the attributes to insert
     * @param id_var
     *   The varcode of the variable related to the attributes to add.  See @ref vartable.h
     * @param attrs
     *   The record with the attributes to be added
     */
    virtual void attr_insert(int id_context, wreport::Varcode id_var, const Record& attrs) = 0;

    /**
     * Insert new attributes into the database.
     *
     * If the same attribute exists for the same data, the function fails.
     *
     * @param id_context
     *   The database id of the context related to the attributes to insert
     * @param id_var
     *   The varcode of the variable related to the attributes to add.  See @ref vartable.h
     * @param attrs
     *   The record with the attributes to be added
     */
    virtual void attr_insert_new(int id_context, wreport::Varcode id_var, const Record& attrs) = 0;

    /**
     * Delete QC data for the variable `var' in record `rec' (coming from a previous
     * dba_query)
     *
     * @param id_context
     *   The database id of the context related to the attributes to remove
     * @param id_var
     *   The varcode of the variable related to the attributes to remove.  See @ref vartable.h
     * @param qcs
     *   Array of WMO codes of the attributes to delete.  If empty, all attributes
     *   associated to id_data will be deleted.
     */
    virtual void attr_remove(int id_context, wreport::Varcode id_var, const db::AttrList& qcs) = 0;

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
    virtual void import_msgs(const Msgs& msgs, const char* repmemo, int flags) = 0;

    /**
     * Perform the query in `query', and return the results as a NULL-terminated
     * array of dba_msg.
     *
     * @param query
     *   The query to perform
     * @param cons
     *   The MsgsConsumer that will handle the resulting messages
     */
    virtual void export_msgs(const Record& query, MsgConsumer& cons) = 0;

    /**
     * Dump the entire contents of the database to an output stream
     */
    virtual void dump(FILE* out) = 0;
};

}

/* vim:set ts=4 sw=4: */
#endif
