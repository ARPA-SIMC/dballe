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
namespace v7 {
namespace sqlite {

SQLiteStationBase::SQLiteStationBase(SQLiteConnection& conn)
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

SQLiteStationBase::~SQLiteStationBase()
{
    delete sstm;
    delete sfstm;
    delete smstm;
    delete istm;
}

bool SQLiteStationBase::maybe_get_id(const StationDesc& st, int* id)
{
    SQLiteStatement* s;
    if (st.ident.get())
    {
        smstm->bind_val(1, st.rep);
        smstm->bind_val(2, st.coords.lat);
        smstm->bind_val(3, st.coords.lon);
        smstm->bind_val(4, st.ident.get());
        s = smstm;
    } else {
        sfstm->bind_val(1, st.rep);
        sfstm->bind_val(2, st.coords.lat);
        sfstm->bind_val(3, st.coords.lon);
        s = sfstm;
    }
    bool found = false;
    s->execute_one([&]() {
        found = true;
        *id = s->column_int(0);
    });
    return found;
}

stations_t::iterator SQLiteStationBase::lookup_id(State& st, int id)
{
    // First look it up in the transaction cache
    for (auto i = st.stations.begin(); i != st.stations.end(); ++i)
        if (i->second.id == id)
            return i;

    if (!sstm)
        sstm = conn.sqlitestatement("SELECT rep, lat, lon, ident FROM station WHERE id=?").release();

    sstm->bind_val(1, id);

    bool found = false;
    stations_t::iterator res;
    sstm->execute_one([&]() {
        StationDesc desc;
        desc.rep = sstm->column_int(0);
        desc.coords.lat = sstm->column_int(1);
        desc.coords.lon = sstm->column_int(2);
        if (!sstm->column_isnull(3))
            desc.ident = sstm->column_string(3);
        StationState sst;
        sst.id = id;
        sst.is_new = false;
        res = st.add_station(desc, sst);
        found = true;
    });

    if (!found)
        error_notfound::throwf("station with id %d not found in the database", id);

    return res;
}

stations_t::iterator SQLiteStationBase::get_id(State& st, const StationDesc& desc)
{
    auto res = st.stations.find(desc);
    if (res != st.stations.end())
        return res;

    StationState state;
    if (maybe_get_id(desc, &state.id))
    {
        state.is_new = false;
        return st.add_station(desc, state);
    }
    throw error_notfound("station not found in the database");
}

stations_t::iterator SQLiteStationBase::obtain_id(State& st, const StationDesc& desc)
{
    auto res = st.stations.find(desc);
    if (res != st.stations.end())
        return res;

    StationState state;
    if (maybe_get_id(desc, &state.id))
    {
        state.is_new = false;
        return st.add_station(desc, state);
    }

    // If no station was found, insert a new one
    istm->bind_val(1, desc.rep);
    istm->bind_val(2, desc.coords.lat);
    istm->bind_val(3, desc.coords.lon);
    if (desc.ident)
        istm->bind_val(4, desc.ident.get());
    else
        istm->bind_null_val(4);
    istm->execute();
    state.id = conn.get_last_insert_id();
    state.is_new = true;
    return st.add_station(desc, state);
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

void SQLiteStationBase::get_station_vars(int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    static const char query[] = R"(
        SELECT d.id_var, d.value, a.type, a.value
          FROM station_data d
          LEFT JOIN station_attr a ON a.id_data = d.id
         WHERE d.id_station=?
         ORDER BY d.id_var, a.type
    )";

    auto stm = conn.sqlitestatement(query);
    stm->bind(id_station);
    TRACE("fill_ana_layer Performing query: %s with idst %d\n", query, id_station);

    read_station_vars(*stm, dest);
}

void SQLiteStationBase::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table station:\n");

    auto stm = conn.sqlitestatement("SELECT id, rep, lat, lon, ident FROM station");
    stm->execute([&]() {
        fprintf(out, " %d, %d, %.5f, %.5f",
                stm->column_int(0),
                stm->column_int(1),
                stm->column_int(2) / 100000.0,
                stm->column_int(3) / 100000.0);
        if (stm->column_isnull(4))
            putc('\n', out);
        else
            fprintf(out, ", %s\n", stm->column_string(4));
        ++count;
    });
    fprintf(out, "%d element%s in table station\n", count, count != 1 ? "s" : "");
}

void SQLiteStationBase::add_station_vars(int id_station, Record& rec)
{
    const char* query = R"(
        SELECT d.id_var, d.value
          FROM station_data d
         WHERE d.id_station = ?
    )";

    auto stm = conn.sqlitestatement(query);
    stm->bind(id_station);
    stm->execute([&]() {
        rec.set(newvar((wreport::Varcode)stm->column_int(0), stm->column_string(1)));
    });
}

SQLiteStationV7::SQLiteStationV7(SQLiteConnection& conn)
    : SQLiteStationBase(conn) {}

}
}
}
}
