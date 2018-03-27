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
    /** Precompiled select station details query */
    dballe::sql::SQLiteStatement* sstm = nullptr;
    /** Precompiled insert query */
    dballe::sql::SQLiteStatement* istm = nullptr;

    /// Lookup the ID of a station, returning true if it was found, false if not
    bool maybe_get_id(v7::Transaction& tr, const dballe::Station& st, int* id) override;

    void _dump(std::function<void(int, int, const Coords& coords, const char* ident)> out) override;

public:
    SQLiteStation(dballe::sql::SQLiteConnection& conn);
    ~SQLiteStation();
    SQLiteStation(const SQLiteStation&) = delete;
    SQLiteStation(const SQLiteStation&&) = delete;
    SQLiteStation& operator=(const SQLiteStation&) = delete;

    const dballe::Station* lookup_id(v7::Transaction& tr, int id) override;
    int obtain_id(v7::Transaction& tr, const dballe::Station& desc) override;

    void get_station_vars(int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(int id_station, Record& rec) override;
};

}
}
}
}
#endif
