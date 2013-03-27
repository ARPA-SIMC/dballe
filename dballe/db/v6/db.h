/*
 * dballe/v6/db - Archive for point-based meteorological data, db layout version 6
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_DB_V6_H
#define DBA_DB_V6_H

#include <dballe/db/odbcworkarounds.h>
#include <dballe/db/db.h>
#include <wreport/varinfo.h>
#include <string>
#include <vector>
#include <memory>

/** @file
 * @ingroup db
 *
 * Functions used to connect to DB-All.e and insert, query and delete data.
 */

/**
 * Constants used to define what values we should retrieve from a query
 */
/** Retrieve latitude and longitude */
#define DBA_DB_WANT_COORDS      (1 << 0)
/** Retrieve the mobile station identifier */
#define DBA_DB_WANT_IDENT       (1 << 1)
/** Retrieve the level information */
#define DBA_DB_WANT_LEVEL       (1 << 2)
/** Retrieve the time range information */
#define DBA_DB_WANT_TIMERANGE   (1 << 3)
/** Retrieve the date and time information */
#define DBA_DB_WANT_DATETIME    (1 << 4)
/** Retrieve the variable name */
#define DBA_DB_WANT_VAR_NAME    (1 << 5)
/** Retrieve the variable value */
#define DBA_DB_WANT_VAR_VALUE   (1 << 6)
/** Retrieve the report code */
#define DBA_DB_WANT_REPCOD      (1 << 7)
/** Retrieve the station ID */
#define DBA_DB_WANT_ANA_ID      (1 << 8)
/** Retrieve the lev_tr ID */
#define DBA_DB_WANT_CONTEXT_ID  (1 << 9)

/**
 * Values for query modifier flags
 */
/** When values from different reports exist on the same point, only report the
 * one from the report with the highest priority */
#define DBA_DB_MODIFIER_BEST        (1 << 0)
/** Tell the database optimizer that this is a query on a database with a big
 * pseudoana table (this serves to hint the MySQL optimizer, which would not
 * otherwise apply the correct strategy */
#define DBA_DB_MODIFIER_BIGANA      (1 << 1)
/** Remove duplicates in the results */
#define DBA_DB_MODIFIER_DISTINCT    (1 << 2)
/** Include the extra anagraphical data in the results */
#define DBA_DB_MODIFIER_ANAEXTRA    (1 << 3)
/** Do not include the extra anagraphical data in the results */
#define DBA_DB_MODIFIER_NOANAEXTRA  (1 << 4)
/** Do not bother sorting the results */
#define DBA_DB_MODIFIER_UNSORTED    (1 << 5)
/** Start geting the results as soon as they are available, without waiting for
 * the database to finish building the result set.  As a side effect, it is
 * impossible to know in advance the number of results.  Currently, it does not
 * work with the MySQL ODBC driver */
#define DBA_DB_MODIFIER_STREAM      (1 << 6)
/** Sort by rep_cod after ana_id, to ease reconstructing messages on export */
#define DBA_DB_MODIFIER_SORT_FOR_EXPORT (1 << 7)

namespace dballe {
struct Record;
struct Msg;
struct Msgs;
struct MsgConsumer;

namespace db {
struct Connection;
struct Statement;
struct Sequence;

namespace v5 {
struct Station;
}

namespace v6 {
struct Repinfo;
struct LevTr;
struct LevTrCache;
struct Data;
struct Attr;

/**
 * DB-ALLe database connection
 */
class DB : public dballe::DB
{
public:
    /** ODBC database connection */
    db::Connection* conn;

protected:
    /// Store information about the database ID of a variable
    struct VarID
    {
        wreport::Varcode code;
        DBALLE_SQL_C_SINT_TYPE id;
        VarID(wreport::Varcode code, DBALLE_SQL_C_SINT_TYPE id) : code(code), id(id) {}
    };

    /// Store database variable IDs for all last inserted variables
    std::vector<VarID> last_insert_varids;

    /**
     * Accessors for the various parts of the database.
     *
     * @warning Before using these 5 pointers, ensure they are initialised
     * using one of the dba_db_need_* functions
     * @{
     */
    /** Report information */
    struct v6::Repinfo* m_repinfo;
    /** Station information */
    struct v5::Station* m_station;
    /** Level/timerange information */
    struct LevTr* m_lev_tr;
    /// Level/timerange cache
    struct LevTrCache* m_lev_tr_cache;
    /** Variable data */
    struct Data* m_data;
    /** Variable attributes */
    struct Attr* m_attr;
    /** @} */

    /**
     * Sequence accessors.
     *
     * They are NULL for databases such as MySQL that do not use sequences.
     * @{
     */
    /** lev_tr ID sequence */
    db::Sequence* seq_lev_tr;
    /** data ID sequence */
    db::Sequence* seq_data;
    /** @} */

    void init_after_connect();

    /**
     * Run a one-off SQL query
     */
    void run_sql(const char* query);

    DB();

public:
    ~DB();

    void open_odbc(const char* dsn, const char* user, const char* password);
    void open_file(const char* pathname);

    /// Access the repinfo table
    v6::Repinfo& repinfo();

    /// Access the station table
    v5::Station& station();

    /// Access the lev_tr table
    LevTr& lev_tr();

    /// Access the lev_tr cache
    LevTrCache& lev_tr_cache();

    /// Access the data table
    Data& data();

    /// Access the data table
    Attr& attr();

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
    void reset(const char* repinfo_file = 0);

    /**
     * Delete all the DB-ALLe tables from the database.
     */
    void delete_tables();

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
    void update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated);

    /**
     * Get the report code from a report mnemonic
     */
    int rep_cod_from_memo(const char* memo);

    /**
     * Get the report mnemonic from a report code
     */
    const std::string& rep_memo_from_cod(int rep_cod);

    /**
     * Verify that a rep_cod is supported by the database
     *
     * @param rep_cod
     *   The report code to verify
     * @returns
     *   true if the report code is supported, false if not
     */
    bool check_rep_cod(int rep_cod);

    /**
     * Return the ID of the last inserted lev_tr
     */
    int last_lev_tr_insert_id();

    /**
     * Return the ID of the last inserted data
     */
    int last_data_insert_id();

    /**
     * Get the report id from this record.
     *
     * If rep_memo is specified instead, the corresponding report id is queried in
     * the database and set as "rep_cod" in the record.
     */
    int get_rep_cod(const Record& rec);

    /*
     * Lookup, insert or replace data in station taking the values from
     * rec.
     *
     * If rec did not contain ana_id, it will be set by this function.
     *
     * @param rec
     *   The record with the station information
     * @param can_add
     *   If true we can insert new stations in the database, if false we
     *   only look up existing records and raise an exception if missing
     * @returns
     *   The station ID
     */
    int obtain_station(const Record& rec, bool can_add=true);

    /*
     * Lookup, insert or replace data in station taking the values from
     * rec.
     *
     * If rec did not contain context_id, it will be set by this function.
     *
     * @param rec
     *   The record with the lev_tr information
     * @returns
     *   The lev_tr ID
     */
    int obtain_lev_tr(const Record& rec);

    /**
     * Insert a record into the database
     *
     * In a record with the same phisical situation already exists, the function
     * fails.
     *
     * @param rec
     *   The record to insert.
     * @param can_replace
     *   If true, then existing data can be rewritten, else data can only be added.
     * @param station_can_add
     *   If true, then it is allowed to add new station records to the database.
     *   Otherwise, data can be added only by reusing existing ones.
     */
    void insert(const Record& rec, bool can_replace, bool station_can_add);

    /**
     * Remove data from the database
     *
     * @param rec
     *   The record with the query data (see technical specifications, par. 1.6.4
     *   "parameter output/input") to select the items to be deleted
     */
    void remove(const Record& rec);

    /**
     * Remove orphan values from the database.
     *
     * Orphan values are currently:
     * \li lev_tr values for which no data exists
     * \li station values for which no lev_tr exists
     *
     * Depending on database size, this routine can take a few minutes to execute.
     */
    void vacuum();

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
    std::auto_ptr<db::Cursor> query(const Record& query, unsigned int wanted, unsigned int modifiers);

    std::auto_ptr<db::Cursor> query_reports(const Record& query);
    std::auto_ptr<db::Cursor> query_levels(const Record& rec);
    std::auto_ptr<db::Cursor> query_tranges(const Record& rec);
    std::auto_ptr<db::Cursor> query_variable_types(const Record& rec);

    /**
     * Start a query on the station archive
     *
     * @param query
     *   The record with the query data (see @ref dba_record_keywords)
     * @return
     *   The cursor to use to iterate over the results
     */
    std::auto_ptr<db::Cursor> query_stations(const Record& query);

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
    std::auto_ptr<db::Cursor> query_data(const Record& rec);

    void query_datetime_extremes(const Record& query, Record& result);

    /**
     * Query attributes
     *
     * @param id_data
     *   The database id of the data related to the attributes to retrieve
     * @param id_var
     *   The varcode of the variable related to the attributes to retrieve.  See @ref vartable.h (ignored)
     * @param qcs
     *   The WMO codes of the QC values requested.  If it is empty, then all values
     *   are returned.
     * @param attrs
     *   The Record that will hold the resulting attributes
     * @return
     *   Number of attributes returned in attrs
     */
    unsigned query_attrs(int id_data, wreport::Varcode id_var, const db::AttrList& qcs, Record& attrs);

    void attr_insert(wreport::Varcode id_var, const Record& attrs, bool can_replace=true);
    void attr_insert(int id_data, wreport::Varcode id_var, const Record& attrs, bool can_replace=true);

    /**
     * Delete QC data for the variable `var' in record `rec' (coming from a previous
     * dba_query)
     *
     * @param id_data
     *   The database id of the lev_tr related to the attributes to remove
     * @param id_var
     *   The varcode of the variable related to the attributes to remove.  See @ref vartable.h (ignored)
     * @param qcs
     *   Array of WMO codes of the attributes to delete.  If empty, all attributes
     *   associated to id_data will be deleted.
     */
    void attr_remove(int id_data, wreport::Varcode id_var, const db::AttrList& qcs);

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
    void import_msg(const Msg& msg, const char* repmemo, int flags);

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
    void import_msgs(const Msgs& msgs, const char* repmemo, int flags);

    /**
     * Perform the query in `query', and return the results as a NULL-terminated
     * array of dba_msg.
     *
     * @param query
     *   The query to perform
     * @param cons
     *   The MsgsConsumer that will handle the resulting messages
     */
    void export_msgs(const Record& query, MsgConsumer& cons);

    /**
     * Dump the entire contents of the database to an output stream
     */
    void dump(FILE* out);

    friend class dballe::DB;
};

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
