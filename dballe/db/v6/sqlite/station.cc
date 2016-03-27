#include "station.h"
#include "dballe/sql/sqlite.h"
#include "dballe/record.h"
#include "dballe/core/var.h"
#include <wreport/var.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;
using dballe::sql::SQLiteConnection;
using dballe::sql::SQLiteStatement;

namespace dballe {
namespace db {
namespace v6 {
namespace sqlite {

SQLiteStationBase::SQLiteStationBase(SQLiteConnection& conn)
    : conn(conn)
{
    const char* select_fixed_query =
        "SELECT id FROM station WHERE lat=? AND lon=? AND ident IS NULL";
    const char* select_mobile_query =
        "SELECT id FROM station WHERE lat=? AND lon=? AND ident=?";
    const char* insert_query =
        "INSERT INTO station (lat, lon, ident)"
        " VALUES (?, ?, ?);";

    // Create the statement for select fixed
    sfstm = conn.sqlitestatement(select_fixed_query).release();

    // Create the statement for select mobile
    smstm = conn.sqlitestatement(select_mobile_query).release();

    // Create the statement for insert
    istm = conn.sqlitestatement(insert_query).release();
}

SQLiteStationBase::~SQLiteStationBase()
{
    delete sfstm;
    delete smstm;
    delete istm;
}

bool SQLiteStationBase::maybe_get_id(int lat, int lon, const char* ident, int* id)
{
    SQLiteStatement* s;
    if (ident)
    {
        smstm->bind_val(1, lat);
        smstm->bind_val(2, lon);
        smstm->bind_val(3, ident);
        s = smstm;
    } else {
        sfstm->bind_val(1, lat);
        sfstm->bind_val(2, lon);
        s = sfstm;
    }
    bool found = false;
    s->execute([&]() {
        found = true;
        *id = s->column_int(0);
    });
    return found;
}

int SQLiteStationBase::get_id(int lat, int lon, const char* ident)
{
    int id;
    if (maybe_get_id(lat, lon, ident, &id))
        return id;
    throw error_notfound("station not found in the database");
}

int SQLiteStationBase::obtain_id(int lat, int lon, const char* ident, bool* inserted)
{
    int id;
    if (maybe_get_id(lat, lon, ident, &id))
    {
        if (inserted) *inserted = false;
        return id;
    }

    // If no station was found, insert a new one
    istm->bind_val(1, lat);
    istm->bind_val(2, lon);
    if (ident)
        istm->bind_val(3, ident);
    else
        istm->bind_null_val(3);
    istm->execute();
    if (inserted) *inserted = true;
    return conn.get_last_insert_id();
}

void SQLiteStationBase::read_station_vars(SQLiteStatement& stm, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Retrieve results
    Varcode last_varcode = 0;
    unique_ptr<Var> var;

    stm.execute([&]() {
        Varcode code = stm.column_int(0);
        TRACE("fill_ana_layer Got B%02ld%03ld %s\n", WR_VAR_X(code), WR_VAR_Y(code), stm.column_string(3));

        // First process the variable, possibly inserting the old one in the message
        if (last_varcode != code)
        {
            TRACE("fill_ana_layer new var\n");
            if (var.get())
            {
                TRACE("fill_ana_layer inserting old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
                dest(move(var));
            }
            var = newvar(code, stm.column_string(1));
            last_varcode = code;
        }

        if (!stm.column_isnull(2))
        {
            TRACE("fill_ana_layer new attribute\n");
            var->seta(newvar(stm.column_int(2), stm.column_string(3)));
        }
    });

    if (var.get())
    {
        TRACE("fill_ana_layer inserting leftover old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
        dest(move(var));
    }
}

void SQLiteStationBase::get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    static const char query[] = R"(
        SELECT d.id_var, d.value, a.type, a.value
          FROM data d
          LEFT JOIN attr a ON a.id_data = d.id
         WHERE d.id_station=? AND d.id_report=?
           AND d.id_lev_tr = -1
         ORDER BY d.id_var, a.type
    )";

    auto stm = conn.sqlitestatement(query);
    stm->bind(id_station, id_report);
    TRACE("fill_ana_layer Performing query: %s with idst %d idrep %d\n", query, id_station, id_report);

    read_station_vars(*stm, dest);
}

void SQLiteStationBase::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table station:\n");

    auto stm = conn.sqlitestatement("SELECT id, lat, lon, ident FROM station");
    stm->execute([&]() {
        fprintf(out, " %d, %.5f, %.5f",
                stm->column_int(0),
                stm->column_int(1) / 100000.0,
                stm->column_int(2) / 100000.0);
        if (stm->column_isnull(3))
            putc('\n', out);
        else
            fprintf(out, ", %s\n", stm->column_string(3));
        ++count;
    });
    fprintf(out, "%d element%s in table station\n", count, count != 1 ? "s" : "");
}

void SQLiteStationBase::add_station_vars(int id_station, Record& rec)
{
    const char* query = R"(
        SELECT d.id_var, d.value
          FROM data d, repinfo ri
         WHERE d.id_lev_tr = -1 AND ri.id = d.id_report AND d.id_station = ?
         AND ri.prio=(
          SELECT MAX(sri.prio) FROM repinfo sri
            JOIN data sd ON sri.id=sd.id_report
          WHERE sd.id_station=d.id_station AND sd.id_lev_tr = -1
            AND sd.id_var=d.id_var)
    )";

    auto stm = conn.sqlitestatement(query);
    stm->bind(id_station);
    stm->execute([&]() {
        rec.set(newvar((wreport::Varcode)stm->column_int(0), stm->column_string(1)));
    });
}

SQLiteStationV6::SQLiteStationV6(SQLiteConnection& conn)
    : SQLiteStationBase(conn) {}

}
}
}
}
