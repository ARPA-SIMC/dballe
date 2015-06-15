#include "station.h"
#include "dballe/db/odbc/internals.h"
#include "dballe/core/var.h"
#include "dballe/record.h"
#include <wreport/var.h>
#include <sqltypes.h>
#include <cstring>
#include <sql.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;

namespace dballe {
namespace db {
namespace odbc {

ODBCStationBase::ODBCStationBase(ODBCConnection& conn)
    : conn(conn), seq_station(0), sfstm(0), smstm(0), sstm(0), istm(0), ustm(0), dstm(0)
{
    const char* select_fixed_query =
        "SELECT id FROM station WHERE lat=? AND lon=? AND ident IS NULL";
    const char* select_mobile_query =
        "SELECT id FROM station WHERE lat=? AND lon=? AND ident=?";
    const char* select_query =
        "SELECT lat, lon, ident FROM station WHERE id=?";
    const char* insert_query =
        "INSERT INTO station (lat, lon, ident)"
        " VALUES (?, ?, ?);";
    const char* update_query =
        "UPDATE station SET lat=?, lon=?, ident=? WHERE id=?";
    const char* remove_query =
        "DELETE FROM station WHERE id=?";

    /* Override queries for some databases */
    switch (conn.server_type)
    {
        case ServerType::ORACLE:
            seq_station = new db::Sequence(conn, "station_id_seq");
            insert_query = "INSERT INTO station (id, lat, lon, ident) VALUES (seq_station.NextVal, ?, ?, ?)";
            break;
        case ServerType::POSTGRES:
            insert_query = "INSERT INTO station (id, lat, lon, ident) VALUES (nextval(pg_get_serial_sequence('station', 'id')), ?, ?, ?)";
            break;
        default: break;
    }

    /* Create the statement for select fixed */
    sfstm = conn.odbcstatement(select_fixed_query).release();
    sfstm->bind_in(1, lat);
    sfstm->bind_in(2, lon);
    sfstm->bind_out(1, id);

    /* Create the statement for select mobile */
    smstm = conn.odbcstatement(select_mobile_query).release();
    smstm->bind_in(1, lat);
    smstm->bind_in(2, lon);
    smstm->bind_in(3, ident, ident_ind);
    smstm->bind_out(1, id);

    /* Create the statement for select station data */
    sstm = conn.odbcstatement(select_query).release();
    sstm->bind_in(1, id);
    sstm->bind_out(1, lat);
    sstm->bind_out(2, lon);
    sstm->bind_out(3, ident, sizeof(ident), ident_ind);

    /* Create the statement for insert */
    istm = conn.odbcstatement(insert_query).release();
    istm->bind_in(1, lat);
    istm->bind_in(2, lon);
    istm->bind_in(3, ident, ident_ind);

    /* Create the statement for update */
    ustm = conn.odbcstatement(update_query).release();
    ustm->bind_in(1, lat);
    ustm->bind_in(2, lon);
    ustm->bind_in(3, ident, ident_ind);
    ustm->bind_in(4, id);

    /* Create the statement for remove */
    dstm = conn.odbcstatement(remove_query).release();
    dstm->bind_in(1, id);
}

ODBCStationBase::~ODBCStationBase()
{
    if (sfstm) delete sfstm;
    if (smstm) delete smstm;
    if (sstm) delete sstm;
    if (istm) delete istm;
    if (ustm) delete ustm;
    if (dstm) delete dstm;
    if (seq_station) delete seq_station;
}

void ODBCStationBase::set_ident(const char* val)
{
    if (val)
    {
        int len = strlen(val);
        if (len > 64) len = 64;
        memcpy(ident, val, len);
        ident[len] = 0;
        ident_ind = len; 
    } else {
        ident[0] = 0;
        ident_ind = SQL_NULL_DATA; 
    }
}

int ODBCStationBase::get_id(int lat, int lon, const char* ident)
{
    this->lat = lat;
    this->lon = lon;
    set_ident(ident);
    ODBCStatement* stm = ident_ind == SQL_NULL_DATA ? sfstm : smstm;
    stm->execute();
    if (stm->fetch_expecting_one())
        return id;

    throw error_notfound("station not found in the database");
}

void ODBCStationBase::get_data(int qid)
{
    id = qid;
    sstm->execute();
    if (!sstm->fetch_expecting_one())
        error_notfound::throwf("looking for information for station id %d", qid);
    if (ident_ind == SQL_NULL_DATA)
        ident[0] = 0;
}

int ODBCStationBase::obtain_id(int lat, int lon, const char* ident, bool* inserted)
{
    this->lat = lat;
    this->lon = lon;
    set_ident(ident);

    // Trying querying
    ODBCStatement* stm = ident_ind == SQL_NULL_DATA ? sfstm : smstm;
    stm->execute();
    if (stm->fetch_expecting_one())
    {
        if (inserted) *inserted = false;
        return id;
    }

    // If nothing was found, insert it
    if (inserted) *inserted = true;
    istm->execute_and_close();
    if (seq_station)
        return seq_station->read();
    else
        return conn.get_last_insert_id();
}

void ODBCStationBase::update()
{
    ustm->execute_and_close();
}

void ODBCStationBase::remove()
{
    dstm->execute_and_close();
}

void ODBCStationBase::get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    static const char query[] =
        "SELECT d.id_var, d.value, a.type, a.value"
        "  FROM context c, data d"
        "  LEFT JOIN attr a ON a.id_context = d.id_context AND a.id_var = d.id_var"
        " WHERE d.id_context = c.id AND c.id_ana = ? AND c.id_report = ?"
        "   AND c.datetime = {ts '1000-01-01 00:00:00.000'}"
        " ORDER BY d.id_var, a.type";

    auto stm = conn.odbcstatement(query);

    stm->bind_in(1, id_station);
    stm->bind_in(2, id_report);

    Varcode out_varcode;
    stm->bind_out(1, out_varcode);

    char out_value[255];
    stm->bind_out(2, out_value, sizeof(out_value));

    int out_attr_varcode;       SQLLEN out_attr_varcode_ind;
    stm->bind_out(3, out_attr_varcode, out_attr_varcode_ind);

    char out_attr_value[255];   SQLLEN out_attr_value_ind;
    stm->bind_out(4, out_attr_value, sizeof(out_attr_value), out_attr_value_ind);

    TRACE("fill_ana_layer Performing query: %s with idst %d idrep %d\n", query, id_station, id_report);
    stm->execute();

    // Retrieve results
    Varcode last_varcode = 0;
    unique_ptr<Var> var;
    while (stm->fetch())
    {
        TRACE("fill_ana_layer Got B%02ld%03ld %s\n", WR_VAR_X(out_varcode), WR_VAR_Y(out_varcode), out_value);

        // First process the variable, possibly inserting the old one in the message
        if (last_varcode != out_varcode)
        {
            TRACE("fill_ana_layer new var\n");
            if (var.get())
            {
                TRACE("fill_ana_layer inserting old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
                dest(move(var));
            }
            var = newvar(out_varcode, out_value);
            last_varcode = out_varcode;
        }

        if (out_attr_varcode_ind != -1)
        {
            TRACE("fill_ana_layer new attribute\n");
            var->seta(ap_newvar(out_attr_varcode, out_attr_value));
        }
    }

    if (var.get())
    {
        TRACE("fill_ana_layer inserting leftover old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
        dest(move(var));
    }
}

void ODBCStationBase::dump(FILE* out)
{
    int id;
    int lat;
    int lon;
    char ident[64];
    SQLLEN ident_ind;

    auto stm = conn.odbcstatement("SELECT id, lat, lon, ident FROM station");
    stm->bind_out(1, id);
    stm->bind_out(2, lat);
    stm->bind_out(3, lon);
    stm->bind_out(4, ident, 64, ident_ind);
    stm->execute();
    int count;
    fprintf(out, "dump of table station:\n");
    for (count = 0; stm->fetch(); ++count)
        if (ident_ind == SQL_NULL_DATA)
            fprintf(out, " %d, %.5f, %.5f\n", (int)id, lat/100000.0, lon/100000.0);
        else
            fprintf(out, " %d, %.5f, %.5f, %.*s\n", (int)id, lat/10000.0, lon/10000.0, (int)ident_ind, ident);
    fprintf(out, "%d element%s in table station\n", count, count != 1 ? "s" : "");
    stm->close_cursor();
}

void ODBCStationBase::impl_add_station_vars(const char* query, int id_station, Record& rec)
{
    Varcode last_code = 0;
    unsigned short st_out_code;
    char st_out_val[256];
    SQLLEN st_out_val_ind;

    auto stm = conn.odbcstatement(query);
    stm->bind_in(1, id_station);
    stm->bind_out(1, st_out_code);
    stm->bind_out(2, st_out_val, sizeof(st_out_val), st_out_val_ind);
    stm->execute();

    // Get the results and save them in the record
    while (stm->fetch())
    {
        // If we have already set the value of a variable skip it: since we get
        // results sorted with descending priority, the first result that we
        // get is already the higher priority result.
        if (last_code == st_out_code)
            continue;

        last_code = st_out_code;
        rec.set(newvar((Varcode)st_out_code, st_out_val));
    }
}

void ODBCStationBase::add_station_vars(int id_station, Record& rec)
{
    /* Extra variables to add:
     *
     * HEIGHT,      B07001  1793
     * HEIGHT_BARO, B07031  1823
     * ST_NAME,     B01019   275
     * BLOCK,       B01001   257
     * STATION,     B01002   258
    */
    const char* query = R"(
        SELECT d.id_var, d.value
          FROM context c, data d, repinfo ri
         WHERE c.id = d.id_context AND ri.id = c.id_report AND c.id_ana = ?
           AND c.datetime = {ts '1000-01-01 00:00:00.000'}
         ORDER BY d.id_var, ri.prio DESC
    )";

    impl_add_station_vars(query, id_station, rec);
}


ODBCStationV6::ODBCStationV6(ODBCConnection& conn)
    : ODBCStationBase(conn) {}

void ODBCStationV6::get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    static const char query[] =
        "SELECT d.id_var, d.value, a.type, a.value"
        "  FROM data d"
        "  LEFT JOIN attr a ON a.id_data = d.id"
        " WHERE d.id_station = ? AND d.id_report = ?"
        "   AND d.id_lev_tr = -1"
        " ORDER BY d.id_var, a.type";

    auto stm = conn.odbcstatement(query);

    stm->bind_in(1, id_station);
    stm->bind_in(2, id_report);

    Varcode out_varcode;
    stm->bind_out(1, out_varcode);

    char out_value[255];
    stm->bind_out(2, out_value, sizeof(out_value));

    Varcode out_attr_varcode;
    SQLLEN out_attr_varcode_ind;
    stm->bind_out(3, out_attr_varcode, out_attr_varcode_ind);

    char out_attr_value[255];
    SQLLEN out_attr_value_ind;
    stm->bind_out(4, out_attr_value, sizeof(out_attr_value), out_attr_value_ind);

    TRACE("StationLayerCache::fill Performing query: %s with idst %d idrep %d\n", query, id_station, id_report);
    stm->execute();

    // Retrieve results
    Varcode last_varcode = 0;
    unique_ptr<Var> var;
    while (stm->fetch())
    {
        TRACE("StationLayerCache::fill Got B%02ld%03ld %s\n", WR_VAR_X(out_varcode), WR_VAR_Y(out_varcode), out_value);

        // First process the variable, possibly inserting the old one in the message
        if (last_varcode != out_varcode)
        {
            TRACE("StationLayerCache::fill new var\n");
            if (var.get())
            {
                TRACE("StationLayerCache::fill inserting old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
                dest(move(var));
            }
            var = newvar(out_varcode, out_value);
            last_varcode = out_varcode;
        }

        if (out_attr_varcode_ind != -1)
        {
            TRACE("fill_ana_layer new attribute\n");
            var->seta(ap_newvar(out_attr_varcode, out_attr_value));
        }
    }

    if (var.get())
    {
        TRACE("fill_ana_layer inserting leftover old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
        dest(move(var));
    }
}

void ODBCStationV6::add_station_vars(int id_station, Record& rec)
{
    const char* query = R"(
        SELECT d.id_var, d.value
          FROM data d, repinfo ri
         WHERE d.id_lev_tr = -1 AND ri.id = d.id_report AND d.id_station = ?
         ORDER BY d.id_var, ri.prio DESC
    )";

    impl_add_station_vars(query, id_station, rec);
}

}

}
}
