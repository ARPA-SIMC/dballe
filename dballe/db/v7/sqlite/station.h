#ifndef DBALLE_DB_V7_SQLITE_STATION_H
#define DBALLE_DB_V7_SQLITE_STATION_H

#include <dballe/db/v7/station.h>
#include <functional>
#include <memory>

namespace wreport {
struct Var;
}

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

/**
 * Precompiled queries to manipulate the station table
 */
class SQLiteStation : public v7::Station
{
protected:
    /**
     * DB connection.
     */
    dballe::sql::SQLiteConnection& conn;

    /** Precompiled select fixed station query */
    dballe::sql::SQLiteStatement* sfstm = nullptr;
    /** Precompiled select mobile station query */
    dballe::sql::SQLiteStatement* smstm = nullptr;
    /** Precompiled insert query */
    dballe::sql::SQLiteStatement* istm = nullptr;

    void _dump(std::function<void(int, int, const Coords& coords, const char* ident)> out) override;

public:
    SQLiteStation(v7::Transaction& tr, dballe::sql::SQLiteConnection& conn);
    ~SQLiteStation();
    SQLiteStation(const SQLiteStation&) = delete;
    SQLiteStation(const SQLiteStation&&) = delete;
    SQLiteStation& operator=(const SQLiteStation&) = delete;

    int maybe_get_id(const dballe::Station& st) override;
    int insert_new(const dballe::Station& desc) override;

    void get_station_vars(int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(int id_station, Record& rec) override;
    void run_station_query(Tracer<>& trc, const v7::StationQueryBuilder& qb, std::function<void(const dballe::Station&)>) override;
};

}
}
}
}
#endif
