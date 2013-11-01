/*
 * dballe/mem/db - Archive for point-based meteorological data, in-memory db
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

#ifndef DBA_DB_MEM_H
#define DBA_DB_MEM_H

#include <dballe/db/db.h>
#include <dballe/memdb/memdb.h>
#include <wreport/varinfo.h>
#include <string>
#include <vector>
#include <memory>

/** @file
 * @ingroup db
 *
 * Functions used to connect to DB-All.e and insert, query and delete data.
 */

namespace dballe {
struct Record;
struct Msg;
struct Msgs;
struct MsgConsumer;

namespace db {

namespace mem {

/**
 * DB-ALLe database connection
 */
class DB : public dballe::DB
{
public:
    /// In-memory database backend
    Memdb memdb;

protected:
#if 0
    /// Store information about the database ID of a variable
    struct VarID
    {
        wreport::Varcode code;
        DBALLE_SQL_C_SINT_TYPE id;
        VarID(wreport::Varcode code, DBALLE_SQL_C_SINT_TYPE id) : code(code), id(id) {}
    };

    /// Store database variable IDs for all last inserted variables
    std::vector<VarID> last_insert_varids;
#endif

public:
    virtual ~DB();

    db::Format format() const { return MEM; }

    void disappear();

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

    std::map<std::string, int> get_repinfo_priorities();

    /**
     * Get the report mnemonic from a report code
     */
    const std::string& rep_memo_from_cod(int rep_cod);

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

    int last_station_id() const;

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

    std::auto_ptr<db::Cursor> query_summary(const Record& rec);

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
