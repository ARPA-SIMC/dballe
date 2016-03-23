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
class SQLiteStationBase : public v7::Station
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
    bool maybe_get_id(const StationDesc& st, int* id);

    /// Run stm, read its output and generate variables to send to dest
    void read_station_vars(dballe::sql::SQLiteStatement& stm, std::function<void(std::unique_ptr<wreport::Var>)> dest);

public:
    SQLiteStationBase(dballe::sql::SQLiteConnection& conn);
    ~SQLiteStationBase();
    SQLiteStationBase(const SQLiteStationBase&) = delete;
    SQLiteStationBase(const SQLiteStationBase&&) = delete;
    SQLiteStationBase& operator=(const SQLiteStationBase&) = delete;

    State::stations_t::iterator lookup_id(State& st, int id) override;
    State::stations_t::iterator get_id(State& st, const StationDesc& desc) override;
    State::stations_t::iterator obtain_id(State& st, const StationDesc& desc) override;

    void get_station_vars(int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(int id_station, Record& rec) override;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;
};

class SQLiteStationV7 : public SQLiteStationBase
{
public:
    SQLiteStationV7(dballe::sql::SQLiteConnection& conn);
};

}
}
}
}
#endif
