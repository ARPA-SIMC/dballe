#include "datav6.h"
#include "dballe/db/sql.h"
#include "dballe/db/v6/db.h"
#include "dballe/core/record.h"
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace sqlite {

SQLiteDataV6::SQLiteDataV6(SQLiteConnection& conn)
    : conn(conn)
{
    const char* insert_query =
        "INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)";
    const char* update_query =
        "UPDATE data SET value=? WHERE id=?";
        //"UPDATE data SET value=? WHERE id_station=? AND id_report=? AND id_lev_tr=? AND datetime=? AND id_var=?";
    const char* insert_ignore_query =
        "INSERT OR IGNORE INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)";
    const char* select_id_query =
        "SELECT id FROM data WHERE id_station=? AND id_report=? AND id_lev_tr=? AND datetime=? AND id_var=?";

    // Create the statement for insert
    istm = conn.sqlitestatement(insert_query).release();

    // Create the statement for update
    ustm = conn.sqlitestatement(update_query).release();

    // Create the statement for select id
    sidstm = conn.sqlitestatement(select_id_query).release();
}

SQLiteDataV6::~SQLiteDataV6()
{
    if (istm) delete istm;
    if (ustm) delete ustm;
    if (sidstm) delete sidstm;
}

void SQLiteDataV6::set_date(const Record& rec)
{
    date = rec.get_datetime();
}

void SQLiteDataV6::set_date(int ye, int mo, int da, int ho, int mi, int se)
{
    date = Datetime(ye, mo, da, ho, mi, se);
}

void SQLiteDataV6::set_station_info(int id_station, int id_report)
{
    this->id_station = id_station;
    this->id_report = id_report;
    // Use -1 instead of NULL, as NULL are considered different in UNIQUE
    // indices by some databases but not others, due to an ambiguity in the SQL
    // standard
    id_lev_tr = -1;
    date = Datetime(1000, 1, 1, 0, 0, 0);
}

void SQLiteDataV6::insert_or_fail(const wreport::Var& var, int* res_id)
{
    istm->bind_val(1, id_station);
    istm->bind_val(2, id_report);
    istm->bind_val(3, id_lev_tr);
    istm->bind_val(4, date);
    istm->bind_val(5, var.code());
    if (const char* val = var.value())
        istm->bind_val(6, val);
    else
        istm->bind_null_val(6);
    istm->execute();
    if (res_id) *res_id = conn.get_last_insert_id();
}

void SQLiteDataV6::insert_or_overwrite(const wreport::Var& var, int* res_id)
{
    // select id
    sidstm->bind_val(1, id_station);
    sidstm->bind_val(2, id_report);
    sidstm->bind_val(3, id_lev_tr);
    sidstm->bind_val(4, date);
    sidstm->bind_val(5, var.code());
    bool exists = false;
    int id;
    sidstm->execute([&]() {
        exists = true;
        id = sidstm->column_int(0);
    });

    if (exists)
    {
        if (const char* val = var.value())
            ustm->bind_val(1, val);
        else
            ustm->bind_null_val(1);
        ustm->bind_val(2, id);
        ustm->execute();
        if (res_id) *res_id = id;
    }
    else
    {
        istm->bind_val(1, id_station);
        istm->bind_val(2, id_report);
        istm->bind_val(3, id_lev_tr);
        istm->bind_val(4, date);
        istm->bind_val(5, var.code());
        if (const char* val = var.value())
            istm->bind_val(6, val);
        else
            istm->bind_null_val(6);
        istm->execute();
        if (res_id) *res_id = conn.get_last_insert_id();
    }
}

void SQLiteDataV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   rep ltr  datetime              var\n");
    auto stm = conn.sqlitestatement("SELECT id, id_station, id_report, id_lev_tr, datetime, id_var, value FROM data");
    stm->execute([&]() {
        int id_lev_tr = stm->column_int(3);
        const char* datetime = stm->column_string(4);
        Varcode code = stm->column_int(5);

        char ltr[20];
        if (id_lev_tr == -1)
            strcpy(ltr, "----");
        else
            snprintf(ltr, 20, "%04d", id_lev_tr);

        fprintf(out, " %4d %4d %3d %s %s %01d%02d%03d",
                stm->column_int(0),
                stm->column_int(1),
                stm->column_int(2),
                ltr,
                datetime,
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
        if (stm->column_isnull(6))
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", stm->column_string(6));

        ++count;
    });
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}

}
}
}
