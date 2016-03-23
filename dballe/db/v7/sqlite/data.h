#ifndef DBALLE_DB_V7_SQLITE_DATA_H
#define DBALLE_DB_V7_SQLITE_DATA_H

#include <dballe/db/v7/data.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
namespace sqlite {
struct DB;

/**
 * Precompiled query to manipulate the station data table
 */
class SQLiteStationData : public v7::StationData
{
protected:
    /// DB connection
    dballe::sql::SQLiteConnection& conn;

    /// Precompiled select statement to prepare bulk insert
    dballe::sql::SQLiteStatement* sstm = nullptr;

public:
    SQLiteStationData(dballe::sql::SQLiteConnection& conn);
    SQLiteStationData(const SQLiteStationData&) = delete;
    SQLiteStationData(const SQLiteStationData&&) = delete;
    SQLiteStationData& operator=(const SQLiteStationData&) = delete;
    ~SQLiteStationData();

    void insert(dballe::Transaction& t, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE) override;
    void remove(const v7::QueryBuilder& qb) override;
    void dump(FILE* out) override;
};

/**
 * Precompiled query to manipulate the data table
 */
class SQLiteData : public v7::Data
{
protected:
    /// DB connection
    dballe::sql::SQLiteConnection& conn;

    /// Precompiled select statement to prepare bulk insert
    dballe::sql::SQLiteStatement* sstm = nullptr;

public:
    SQLiteData(dballe::sql::SQLiteConnection& conn);
    SQLiteData(const SQLiteData&) = delete;
    SQLiteData(const SQLiteData&&) = delete;
    SQLiteData& operator=(const SQLiteData&) = delete;
    ~SQLiteData();

    void insert(dballe::Transaction& t, v7::bulk::InsertVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE) override;
    void remove(const v7::QueryBuilder& qb) override;
    void dump(FILE* out) override;
};

}
}
}
}
#endif
