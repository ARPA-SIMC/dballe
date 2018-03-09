#ifndef DBA_DB_V7_H
#define DBA_DB_V7_H

#include <dballe/db/db.h>
#include <dballe/db/trace.h>
#include <dballe/db/v7/state.h>
#include <dballe/db/v7/data.h>
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

namespace v7 {
struct State;
struct Driver;
struct Repinfo;
struct Station;
struct LevTr;
}

namespace v7 {
struct Transaction;

/**
 * DB-ALLe database connection for database format V7
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
    v7::Repinfo* m_repinfo = nullptr;
    /** Station information */
    v7::Station* m_station = nullptr;
    /** Level/timerange information */
    v7::LevTr* m_levtr = nullptr;
    /** Station data */
    v7::StationData* m_station_data = nullptr;
    /** Variable data */
    v7::Data* m_data = nullptr;
    /** @} */

    void init_after_connect();

public:
    DB(std::unique_ptr<dballe::sql::Connection> conn);
    virtual ~DB();

    db::Format format() const { return V7; }

    /// Access the backend DB driver
    v7::Driver& driver();

    /// Access the repinfo table
    v7::Repinfo& repinfo();

    /// Access the station table
    v7::Station& station();

    /// Access the levtr table
    v7::LevTr& levtr();

    /// Access the station_data table
    v7::StationData& station_data();

    /// Access the data table
    v7::Data& data();

    std::shared_ptr<dballe::db::Transaction> transaction() override;
    std::shared_ptr<dballe::db::Transaction> test_transaction() override;

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
     * Dump the entire contents of the database to an output stream
     */
    void dump(FILE* out);

    friend class dballe::DB;
    friend class dballe::db::v7::Transaction;
};

}
}
}
#endif
