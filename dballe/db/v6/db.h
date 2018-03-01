#ifndef DBA_DB_V6_H
#define DBA_DB_V6_H

#include <dballe/db/db.h>
#include <dballe/db/trace.h>
#include <wreport/varinfo.h>
#include <string>
#include <vector>
#include <memory>

namespace dballe {
struct Station;
struct StationValues;
struct DataValues;

namespace sql {
struct Connection;
struct Statement;
struct Sequence;
}

namespace db {

namespace v6 {
struct Driver;
struct Repinfo;
struct Station;
struct LevTr;
struct LevTrCache;
struct DataV6;
struct AttrV6;
struct Transaction;
}

namespace v6 {
struct DB;

struct Transaction : public dballe::db::Transaction
{
    DB& db;
    dballe::Transaction* sql_transaction = nullptr;

    Transaction(v6::DB& db, std::unique_ptr<dballe::Transaction> sql_transaction);
    Transaction(const Transaction&) = delete;
    Transaction(Transaction&&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction& operator=(Transaction&&) = delete;
    ~Transaction();

    void commit() override;
    void rollback() override;
    void clear_cached_state() override;
    void remove_all() override;
    void insert_station_data(StationValues& vals, bool can_replace, bool station_can_add) override;
    void insert_data(DataValues& vals, bool can_replace, bool station_can_add) override;
    void remove_station_data(const Query& query) override;
    void remove(const Query& query) override;
    void attr_insert_station(int data_id, const Values& attrs) override;
    void attr_insert_data(int data_id, const Values& attrs) override;
    void attr_remove_station(int data_id, const db::AttrList& qcs) override;
    void attr_remove_data(int data_id, const db::AttrList& qcs) override;
    void import_msg(const Message& msg, const char* repmemo, int flags) override;
    bool export_msgs(const Query& query, std::function<bool(std::unique_ptr<Message>&&)> dest) override;
};

/**
 * DB-ALLe database connection, database format V6
 */
class DB : public dballe::DB
{
public:
    /// Database connection
    dballe::sql::Connection* conn;
    /// Database query tracing
    Trace trace;
    /// True if we print an EXPLAIN trace of all queries to stderr
    bool explain_queries = false;

protected:
    /// SQL driver backend
    v6::Driver* m_driver;

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
    struct v6::Station* m_station;
    /** Level/timerange information */
    struct v6::LevTr* m_lev_tr;
    /// Level/timerange cache
    struct v6::LevTrCache* m_lev_tr_cache;
    /** Variable data */
    struct v6::DataV6* m_data;
    /** Variable attributes */
    struct v6::AttrV6* m_attr;
    /** @} */

    void init_after_connect();

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
    int obtain_station(const dballe::Station& st, bool can_add=true);

public:
    DB(std::unique_ptr<dballe::sql::Connection> conn);
    virtual ~DB();

    db::Format format() const { return V6; }

    /// Access the backend DB driver
    v6::Driver& driver();

    /// Access the repinfo table
    v6::Repinfo& repinfo();

    /// Access the station table
    v6::Station& station();

    /// Access the lev_tr table
    v6::LevTr& lev_tr();

    /// Access the lev_tr cache
    v6::LevTrCache& lev_tr_cache();

    /// Access the data table
    v6::DataV6& data();

    /// Access the data table
    v6::AttrV6& attr();

    std::unique_ptr<dballe::db::Transaction> transaction() override;

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

    std::map<std::string, int> get_repinfo_priorities();

    /**
     * Get the report code from a report mnemonic
     */
    int rep_cod_from_memo(const char* memo);

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

    std::unique_ptr<db::CursorStation> query_stations(const Query& query);
    std::unique_ptr<db::CursorStationData> query_station_data(const Query& query) override;
    std::unique_ptr<db::CursorData> query_data(const Query& query);
    std::unique_ptr<db::CursorSummary> query_summary(const Query& query);

    void attr_query_station(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest) override;
    void attr_query_data(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest) override;
    bool is_station_variable(int data_id, wreport::Varcode varcode) override;

    /**
     * Dump the entire contents of the database to an output stream
     */
    void dump(FILE* out);

    friend class dballe::DB;
    friend class dballe::db::v6::Transaction;
};

}
}
}
#endif
