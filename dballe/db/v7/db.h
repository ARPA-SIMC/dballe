#ifndef DBA_DB_V7_H
#define DBA_DB_V7_H

#include <dballe/db/db.h>
#include <dballe/db/trace.h>
#include <dballe/db/v7/state.h>
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

namespace v7 {
struct State;
struct Driver;
struct Repinfo;
struct Station;
struct LevTr;
struct StationData;
struct Data;
struct Attr;
}

namespace v7 {

/**
 * DB-ALLe database connection
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
    v7::Driver* m_driver;

    /**
     * Accessors for the various parts of the database.
     *
     * @warning Before using these 5 pointers, ensure they are initialised
     * using one of the dba_db_need_* functions
     * @{
     */
    /** Report information */
    struct v7::Repinfo* m_repinfo = nullptr;
    /** Station information */
    struct v7::Station* m_station = nullptr;
    /** Level/timerange information */
    struct v7::LevTr* m_lev_tr = nullptr;
    /** Station data */
    struct v7::StationData* m_station_data = nullptr;
    /** Station data attributes */
    struct v7::Attr* m_station_attr = nullptr;
    /** Variable data */
    struct v7::Data* m_data = nullptr;
    /** Variable attributes */
    struct v7::Attr* m_attr = nullptr;
    /** @} */

    void init_after_connect();

    DB(std::unique_ptr<dballe::sql::Connection> conn);

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
    v7::State::stations_t::iterator obtain_station(v7::State& state, const dballe::Station& st, bool can_add=true);

public:
    virtual ~DB();

    db::Format format() const { return V7; }

    /// Access the backend DB driver
    v7::Driver& driver();

    /// Access the repinfo table
    v7::Repinfo& repinfo();

    /// Access the station table
    v7::Station& station();

    /// Access the lev_tr table
    v7::LevTr& lev_tr();

    /// Access the data table
    v7::StationData& station_data();

    /// Access the data table
    v7::Data& data();

    /// Access the data table
    v7::Attr& station_attr();

    /// Access the data table
    v7::Attr& attr();

    std::unique_ptr<dballe::Transaction> transaction() override;

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

    void insert_station_data(dballe::Transaction& transaction, StationValues& vals, bool can_replace, bool station_can_add) override;
    void insert_data(dballe::Transaction& transaction, DataValues& vals, bool can_replace, bool station_can_add) override;

    void remove_station_data(dballe::Transaction& transaction, const Query& query) override;
    void remove(dballe::Transaction& transaction, const Query& query);
    void remove_all(dballe::Transaction& transaction);

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
    void attr_insert_station(dballe::Transaction& transaction, int data_id, const Values& attrs) override;
    void attr_insert_data(dballe::Transaction& transaction, int data_id, const Values& attrs) override;
    void attr_remove_station(dballe::Transaction& transaction, int data_id, const db::AttrList& qcs) override;
    void attr_remove_data(dballe::Transaction& transaction, int data_id, const db::AttrList& qcs) override;
    bool is_station_variable(int data_id, wreport::Varcode varcode) override;

    void import_msg(dballe::Transaction& transaction, const Message& msg, const char* repmemo, int flags) override;
    bool export_msgs(dballe::Transaction& transaction, const Query& query, std::function<bool(std::unique_ptr<Message>&&)> dest) override;

    /**
     * Dump the entire contents of the database to an output stream
     */
    void dump(FILE* out);

    friend class dballe::DB;
};

}
}
}
#endif
