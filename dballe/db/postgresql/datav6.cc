#include "datav6.h"
#include "dballe/db/sql.h"
#include "dballe/db/v6/db.h"
#include "dballe/core/record.h"
#include <cstring>
#include <sstream>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace postgresql {

PostgreSQLDataV6::PostgreSQLDataV6(PostgreSQLConnection& conn)
    : conn(conn)
{
    conn.prepare("datav6_insert", R"(
        INSERT INTO data (id, id_station, id_report, id_lev_tr, datetime, id_var, value)
             VALUES (DEFAULT, $1::int4, $2::int4, $3::int4, $4::timestamp, $5::int4, $6::text)
          RETURNING id
    )");
    conn.prepare("datav6_insert_ignore", R"(
        INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value)
             SELECT $1::int4, $2::int4, $3::int4, $4::timestamp, $5::int4, $6::text
              WHERE NOT EXISTS (
                    SELECT 1
                      FROM data
                     WHERE id_station = $1::int4 AND datetime = $4::timestamp
                       AND id_lev_tr = $3::int4 AND id_report = $2::int4
                       AND id_var = $5::int4
                    )
          RETURNING id
    )");
    conn.prepare("datav6_update", R"(
        UPDATE data SET value=$2::text WHERE id=$1::int4
    )");
    conn.prepare("datav6_select_id", R"(
        SELECT id FROM data
         WHERE id_station=$1::int4 AND id_report=$2::int4
           AND id_lev_tr=$3::int4 AND datetime=$4::timestamp AND id_var=$5::int4
    )");
}

PostgreSQLDataV6::~PostgreSQLDataV6()
{
}

void PostgreSQLDataV6::set_date(const Record& rec)
{
    date = rec.get_datetime();
}

void PostgreSQLDataV6::set_date(int ye, int mo, int da, int ho, int mi, int se)
{
    date = Datetime(ye, mo, da, ho, mi, se);
}

void PostgreSQLDataV6::set_station_info(int id_station, int id_report)
{
    this->id_station = id_station;
    this->id_report = id_report;
    // Use -1 instead of NULL, as NULL are considered different in UNIQUE
    // indices by some databases but not others, due to an ambiguity in the SQL
    // standard
    id_lev_tr = -1;
    date = Datetime(1000, 1, 1, 0, 0, 0);
}

void PostgreSQLDataV6::insert_or_fail(const wreport::Var& var, int* res_id)
{
    using namespace postgresql;
    Result res;
    if (const char* val = var.value())
        res = move(conn.exec_prepared_one_row("datav6_insert", id_station, id_report, id_lev_tr, date, (int)var.code(), val));
    else
        res = move(conn.exec_prepared_one_row("datav6_insert", id_station, id_report, id_lev_tr, date, (int)var.code(), nullptr));

    if (res_id)
        *res_id = res.get_int4(0, 0);
}

void PostgreSQLDataV6::insert_or_overwrite(const wreport::Var& var, int* res_id)
{
    using namespace postgresql;

    conn.exec_no_data("LOCK TABLE data IN EXCLUSIVE MODE");

    auto res_sel = conn.exec_prepared("datav6_select_id", id_station, id_report, id_lev_tr, date, (int)var.code());
    switch (res_sel.rowcount())
    {
        case 0:
        {
            // None present, do insert
            Result res;
            if (const char* val = var.value())
                res = move(conn.exec_prepared_one_row("datav6_insert", id_station, id_report, id_lev_tr, date, (int)var.code(), val));
            else
                res = move(conn.exec_prepared_one_row("datav6_insert", id_station, id_report, id_lev_tr, date, (int)var.code(), nullptr));
            if (res_id) *res_id = res.get_int4(0, 0);
            break;
        }
        case 1:
        {
            // Value present, do update
            int id = res_sel.get_int4(0, 0);
            if (res_id) *res_id = id;

            if (const char* val = var.value())
                conn.exec_prepared_no_data("datav6_update", id, val);
            else
                conn.exec_prepared_no_data("datav6_update", id, nullptr);
            break;
        }
        default: error_consistency::throwf("get id data v6 query returned %u results", res_sel.rowcount());
    }
}

void PostgreSQLDataV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   rep ltr  datetime              var\n");
    auto res = conn.exec("SELECT id, id_station, id_report, id_lev_tr, datetime, id_var, value FROM data");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        int id_lev_tr = res.get_int4(row, 3);
        Datetime datetime = res.get_timestamp(row, 4);
        Varcode code = res.get_int4(row, 5);

        char ltr[20];
        if (id_lev_tr == -1)
            strcpy(ltr, "----");
        else
            snprintf(ltr, 20, "%04d", id_lev_tr);

        stringstream dtstr;
        dtstr << datetime;
        fprintf(out, " %4d %4d %3d %s %s %01d%02d%03d",
                res.get_int4(row, 0),
                res.get_int4(row, 1),
                res.get_int4(row, 2),
                ltr,
                dtstr.str().c_str(),
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
        if (res.is_null(row, 6))
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", res.get_string(row, 6));

        ++count;
    };
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}

}
}
}
