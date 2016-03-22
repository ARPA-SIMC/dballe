#ifndef DBALLE_DB_V6_ODBC_STATION_H
#define DBALLE_DB_V6_ODBC_STATION_H

#include <dballe/db/v6/station.h>
#include <dballe/sql/fwd.h>
#include <sqltypes.h>
#include <functional>
#include <memory>

namespace wreport {
struct Var;
}

namespace dballe {
namespace db {
namespace v6 {
namespace odbc {

/**
 * Precompiled queries to manipulate the station table
 */
class ODBCStationBase : public v6::Station
{
protected:
    /**
     * DB connection.
     */
    dballe::sql::ODBCConnection& conn;

    /** Station ID sequence, when the DB requires it */
    dballe::sql::Sequence* seq_station;

    /** Precompiled select fixed station query */
    dballe::sql::ODBCStatement* sfstm;
    /** Precompiled select mobile station query */
    dballe::sql::ODBCStatement* smstm;
    /** Precompiled select data by station id query */
    dballe::sql::ODBCStatement* sstm;
    /** Precompiled insert query */
    dballe::sql::ODBCStatement* istm;
    /** Precompiled update query */
    dballe::sql::ODBCStatement* ustm;
    /** Precompiled delete query */
    dballe::sql::ODBCStatement* dstm;

    /** Station ID SQL parameter */
    int id;
    /** Station latitude SQL parameter */
    int lat;
    /** Station longitude SQL parameter */
    int lon;
    /** Mobile station identifier SQL parameter */
    char ident[64];
    /** Mobile station identifier indicator */
    SQLLEN ident_ind;

    void set_ident(const char* ident);
    void get_data(int id);
    void update();
    void remove();
    void impl_add_station_vars(const char* query, int id_station, Record& rec);

public:
    ODBCStationBase(dballe::sql::ODBCConnection& conn);
    ~ODBCStationBase();
    ODBCStationBase(const ODBCStationBase&) = delete;
    ODBCStationBase(const ODBCStationBase&&) = delete;
    ODBCStationBase& operator=(const ODBCStationBase&) = delete;

    int get_id(int lat, int lon, const char* ident=NULL) override;
    int obtain_id(int lat, int lon, const char* ident=NULL, bool* inserted=NULL) override;
    void get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(int id_station, Record& rec) override;
    void dump(FILE* out) override;
};


class ODBCStationV6 : public ODBCStationBase
{
public:
    ODBCStationV6(dballe::sql::ODBCConnection& conn);
    void get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(int id_station, Record& rec) override;
};

}
}
}
}
#endif
