#ifndef DBA_DB_V7_H
#define DBA_DB_V7_H

#include <dballe/db/db.h>
#include <dballe/db/trace.h>
#include <wreport/varinfo.h>
#include <string>
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

    void init_after_connect();

public:
    DB(std::unique_ptr<dballe::sql::Connection> conn);
    virtual ~DB();

    db::Format format() const { return V7; }

    /// Access the backend DB driver
    v7::Driver& driver();

    std::shared_ptr<dballe::db::Transaction> transaction(bool readonly=false) override;
    std::shared_ptr<dballe::db::Transaction> test_transaction(bool readonly=false) override;

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

    friend class dballe::DB;
    friend class dballe::db::v7::Transaction;
};

}
}
}
#endif
