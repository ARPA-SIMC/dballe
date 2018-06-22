#ifndef DBALLE_DB_V7_DRIVER_H
#define DBALLE_DB_V7_DRIVER_H

#include <dballe/core/defs.h>
#include <dballe/core/values.h>
#include <dballe/db/defs.h>
#include <dballe/sql/fwd.h>
#include <dballe/db/v7/fwd.h>
#include <dballe/db/v7/data.h>
#include <wreport/var.h>
#include <memory>
#include <functional>
#include <vector>
#include <cstdio>

namespace dballe {
namespace db {
namespace v7 {

struct Driver
{
public:
    sql::Connection& connection;

    Driver(sql::Connection& connection);
    virtual ~Driver();

    /// Precompiled queries to manipulate the repinfo table
    virtual std::unique_ptr<v7::Repinfo> create_repinfo(v7::Transaction& tr) = 0;

    /// Precompiled queries to manipulate the station table
    virtual std::unique_ptr<v7::Station> create_station(v7::Transaction& tr) = 0;

    /// Precompiled queries to manipulate the levtr table
    virtual std::unique_ptr<v7::LevTr> create_levtr(v7::Transaction& tr) = 0;

    /// Precompiled queries to manipulate the data table
    virtual std::unique_ptr<v7::StationData> create_station_data(v7::Transaction& tr) = 0;

    /// Precompiled queries to manipulate the data table
    virtual std::unique_ptr<v7::Data> create_data(v7::Transaction& tr) = 0;

    /**
     * Run a station query, iterating on the resulting stations
     */
    virtual void run_station_query(const v7::StationQueryBuilder& qb, std::function<void(const dballe::Station& station)>) = 0;

    /// Create all missing tables for a DB with the given format
    void create_tables(db::Format format);

    /// Create all missing tables for V7 databases
    virtual void create_tables_v7() = 0;

    /// Delete all existing tables for a DB with the given format
    void delete_tables(db::Format format);

    /// Delete all existing tables for V7 databases
    virtual void delete_tables_v7() = 0;

    /// Empty all tables for a DB with the given format
    void remove_all(db::Format format);

    /// Empty all tables for V7 databases, assuming that they exist, without touching the repinfo table
    virtual void remove_all_v7();

    /// Perform database cleanup/maintenance on v7 databases
    virtual void vacuum_v7() = 0;

    /// Create a Driver for this connection
    static std::unique_ptr<Driver> create(dballe::sql::Connection& conn);
};

}
}
}
#endif
