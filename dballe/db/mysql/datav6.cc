#include "datav6.h"
#include "dballe/db/sql.h"
#include "dballe/db/querybuf.h"
#include "dballe/core/record.h"
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace mysql {

MySQLDataV6::MySQLDataV6(MySQLConnection& conn)
    : conn(conn)
{
}

MySQLDataV6::~MySQLDataV6()
{
}

void MySQLDataV6::set_date(const Record& rec)
{
    date = rec.get_datetime();
}

void MySQLDataV6::set_date(int ye, int mo, int da, int ho, int mi, int se)
{
    date = Datetime(ye, mo, da, ho, mi, se);
}

void MySQLDataV6::set_station_info(int id_station, int id_report)
{
    this->id_station = id_station;
    this->id_report = id_report;
    // Use -1 instead of NULL, as NULL are considered different in UNIQUE
    // indices by some databases but not others, due to an ambiguity in the SQL
    // standard
    id_lev_tr = -1;
    date = Datetime(1000, 1, 1, 0, 0, 0);
}

void MySQLDataV6::insert_or_fail(const wreport::Var& var, int* res_id)
{
    if (!var.value()) return;
    string escaped_value = conn.escape(var.value());

    Querybuf insert;
    insert.appendf(R"(
        INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value)
        VALUES (%d, %d, %d, '%04d-%02d-%02d %02d:%02d:%02d', %d, '%s')
    )", id_station, id_report, id_lev_tr,
        date.date.year, date.date.month, date.date.day,
        date.time.hour, date.time.minute, date.time.second,
        (int)var.code(), escaped_value.c_str());
    conn.exec_no_data(insert);
    if (res_id) *res_id = conn.get_last_insert_id();
}

void MySQLDataV6::insert_or_overwrite(const wreport::Var& var, int* res_id)
{
    if (!var.value()) return;
    string escaped_value = conn.escape(var.value());

    Querybuf insert;
    insert.appendf(R"(
        INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value)
        VALUES (%d, %d, %d, '%04d-%02d-%02d %02d:%02d:%02d', %d, '%s')
        ON DUPLICATE KEY UPDATE id=LAST_INSERT_ID(id), value=VALUES(value)
    )", id_station, id_report, id_lev_tr,
        date.date.year, date.date.month, date.date.day,
        date.time.hour, date.time.minute, date.time.second,
        (int)var.code(), escaped_value.c_str());
    conn.exec_no_data(insert);
    if (res_id) *res_id = conn.get_last_insert_id();
}

void MySQLDataV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   rep ltr  datetime              var\n");
    auto res = conn.exec_store("SELECT id, id_station, id_report, id_lev_tr, datetime, id_var, value FROM data");
    while (auto row = res.fetch())
    {
        int id_lev_tr = row.as_int(3);
        const char* datetime = row.as_cstring(4);
        Varcode code = row.as_int(5);

        char ltr[20];
        if (id_lev_tr == -1)
            strcpy(ltr, "----");
        else
            snprintf(ltr, 20, "%04d", id_lev_tr);

        fprintf(out, " %4d %4d %3d %s %s %01d%02d%03d",
                row.as_int(0),
                row.as_int(1),
                row.as_int(2),
                ltr,
                datetime,
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
        if (row.isnull(6))
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", row.as_cstring(6));

        ++count;
    }
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}

}
}
}
