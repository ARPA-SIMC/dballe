#include "station.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/sql/sqlite.h"
#include "dballe/record.h"
#include "dballe/core/var.h"
#include "dballe/core/values.h"
#include <wreport/var.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;
using dballe::sql::SQLiteConnection;
using dballe::sql::SQLiteStatement;

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

SQLiteStation::SQLiteStation(SQLiteConnection& conn)
    : conn(conn)
{
    const char* select_fixed_query =
        "SELECT id FROM station WHERE rep=? AND lat=? AND lon=? AND ident IS NULL";
    const char* select_mobile_query =
        "SELECT id FROM station WHERE rep=? AND lat=? AND lon=? AND ident=?";
    const char* insert_query =
        "INSERT INTO station (rep, lat, lon, ident)"
        " VALUES (?, ?, ?, ?);";

    // Create the statement for select fixed
    sfstm = conn.sqlitestatement(select_fixed_query).release();

    // Create the statement for select mobile
    smstm = conn.sqlitestatement(select_mobile_query).release();

    // Create the statement for insert
    istm = conn.sqlitestatement(insert_query).release();
}

SQLiteStation::~SQLiteStation()
{
    delete sfstm;
    delete smstm;
    delete istm;
}

int SQLiteStation::maybe_get_id(v7::Transaction& tr, const dballe::Station& st)
{
    SQLiteStatement* s;
    int rep = tr.repinfo().obtain_id(st.report.c_str());
    if (st.ident.get())
    {
        smstm->bind_val(1, rep);
        smstm->bind_val(2, st.coords.lat);
        smstm->bind_val(3, st.coords.lon);
        smstm->bind_val(4, st.ident.get());
        s = smstm;
    } else {
        sfstm->bind_val(1, rep);
        sfstm->bind_val(2, st.coords.lat);
        sfstm->bind_val(3, st.coords.lon);
        s = sfstm;
    }
    bool found = false;
    int id;
    s->execute_one([&]() {
        found = true;
        id = s->column_int(0);
    });
    if (found)
        return id;
    else
        return MISSING_INT;
}

int SQLiteStation::insert_new(v7::Transaction& tr, const dballe::Station& desc)
{
    // If no station was found, insert a new one
    istm->bind_val(1, tr.repinfo().get_id(desc.report.c_str()));
    istm->bind_val(2, desc.coords.lat);
    istm->bind_val(3, desc.coords.lon);
    if (desc.ident)
        istm->bind_val(4, desc.ident.get());
    else
        istm->bind_null_val(4);
    istm->execute();

    return conn.get_last_insert_id();
}

void SQLiteStation::get_station_vars(int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    static const char query[] = R"(
        SELECT d.code, d.value, d.attrs
          FROM station_data d
         WHERE d.id_station=?
         ORDER BY d.code
    )";

    auto stm = conn.sqlitestatement(query);
    stm->bind(id_station);
    TRACE("get_station_vars Performing query: %s with idst %d\n", query, id_station);

    // Retrieve results
    stm->execute([&]() {
        Varcode code = stm->column_int(0);
        TRACE("get_station_vars Got %d%02d%03d %s\n", WR_VAR_FXY(code), stm->column_string(1));

        unique_ptr<Var> var = newvar(code, stm->column_string(1));
        if (!stm->column_isnull(2))
        {
            TRACE("get_station_vars add attributes\n");
            Values::decode(stm->column_blob(2), [&](unique_ptr<wreport::Var> a) { var->seta(move(a)); });
        }

        dest(move(var));
    });
}

void SQLiteStation::_dump(std::function<void(int, int, const Coords& coords, const char* ident)> out)
{
    auto stm = conn.sqlitestatement("SELECT id, rep, lat, lon, ident FROM station");
    stm->execute([&]() {
        const char* ident = stm->column_isnull(4) ? nullptr : stm->column_string(4);
        out(stm->column_int(0), stm->column_int(1), Coords(stm->column_int(2), stm->column_int(3)), ident);
    });
}

void SQLiteStation::add_station_vars(int id_station, Record& rec)
{
    const char* query = R"(
        SELECT d.code, d.value
          FROM station_data d
         WHERE d.id_station = ?
    )";

    auto stm = conn.sqlitestatement(query);
    stm->bind(id_station);
    stm->execute([&]() {
        rec.set(newvar((wreport::Varcode)stm->column_int(0), stm->column_string(1)));
    });
}

}
}
}
}
