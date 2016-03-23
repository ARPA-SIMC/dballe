#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "levtr.h"
#include "data.h"
#include "attr.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/sql/sqlite.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;
using dballe::sql::SQLiteConnection;
using dballe::sql::SQLiteStatement;
using dballe::sql::Transaction;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

Driver::Driver(SQLiteConnection& conn)
    : conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<v7::Repinfo> Driver::create_repinfo()
{
    return unique_ptr<v7::Repinfo>(new SQLiteRepinfoV7(conn));
}

std::unique_ptr<v7::Station> Driver::create_station()
{
    return unique_ptr<v7::Station>(new SQLiteStationV7(conn));
}

std::unique_ptr<v7::LevTr> Driver::create_levtr()
{
    return unique_ptr<v7::LevTr>(new SQLiteLevTrV7(conn));
}

std::unique_ptr<v7::Data> Driver::create_data()
{
    return unique_ptr<v7::Data>(new SQLiteData(conn));
}

std::unique_ptr<v7::Attr> Driver::create_attr()
{
    return unique_ptr<v7::Attr>(new SQLiteAttr(conn));
}

void Driver::run_built_query_v7(
        const v7::QueryBuilder& qb,
        std::function<void(v7::SQLRecordV7& rec)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    v7::SQLRecordV7 rec;
    stm->execute([&]() {
        int output_seq = 0;

        if (qb.select_station)
        {
            rec.out_ana_id = stm->column_int(output_seq++);
            rec.out_rep_cod = stm->column_int(output_seq++);
            rec.out_lat = stm->column_int(output_seq++);
            rec.out_lon = stm->column_int(output_seq++);
            if (stm->column_isnull(output_seq))
            {
                rec.out_ident_size = -1;
                rec.out_ident[0] = 0;
            } else {
                const char* ident = stm->column_string(output_seq);
                rec.out_ident_size = min(strlen(ident), (string::size_type)63);
                memcpy(rec.out_ident, ident, rec.out_ident_size);
                rec.out_ident[rec.out_ident_size] = 0;
            }
            ++output_seq;
        }

        if (qb.select_varinfo)
        {
            rec.out_id_ltr = stm->column_int(output_seq++);
            rec.out_varcode = stm->column_int(output_seq++);
        }

        if (qb.select_data_id)
            rec.out_id_data = stm->column_int(output_seq++);

        if (qb.select_data)
        {
            rec.out_datetime = stm->column_datetime(output_seq++);

            const char* value = stm->column_string(output_seq++);
            unsigned val_size = min(strlen(value), (string::size_type)255);
            memcpy(rec.out_value, value, val_size);
            rec.out_value[val_size] = 0;
        }

        if (qb.select_summary_details)
        {
            rec.out_id_data = stm->column_int(output_seq++);
            rec.out_datetime = stm->column_datetime(output_seq++);
            rec.out_datetimemax = stm->column_datetime(output_seq++);
        }

        dest(rec);
    });
}

void Driver::create_tables_v7()
{
    conn.exec(R"(
        CREATE TABLE repinfo (
           id           INTEGER PRIMARY KEY,
           memo         VARCHAR(30) NOT NULL,
           description  VARCHAR(255) NOT NULL,
           prio         INTEGER NOT NULL,
           descriptor   CHAR(6) NOT NULL,
           tablea       INTEGER NOT NULL,
           UNIQUE (prio),
           UNIQUE (memo)
        );
    )");
    conn.exec(R"(
        CREATE TABLE station (
           id    INTEGER PRIMARY KEY,
           rep   INTEGER NOT NULL REFERENCES repinfo (id) ON DELETE CASCADE,
           lat   INTEGER NOT NULL,
           lon   INTEGER NOT NULL,
           ident CHAR(64),
           UNIQUE (rep, lat, lon, ident)
        );
        CREATE INDEX pa_rep ON station(rep);
        CREATE INDEX pa_lon ON station(lon);
    )");
    conn.exec(R"(
        CREATE TABLE lev_tr (
           id         INTEGER PRIMARY KEY,
           ltype1      INTEGER NOT NULL,
           l1          INTEGER NOT NULL,
           ltype2      INTEGER NOT NULL,
           l2          INTEGER NOT NULL,
           ptype       INTEGER NOT NULL,
           p1          INTEGER NOT NULL,
           p2          INTEGER NOT NULL,
           UNIQUE (ltype1, l1, ltype2, l2, ptype, p1, p2)
        );
    )");
    conn.exec(R"(
        CREATE TABLE data (
           id          INTEGER PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           id_lev_tr   INTEGER NOT NULL,
           datetime    TEXT NOT NULL,
           id_var      INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL,
           UNIQUE (id_station, datetime, id_lev_tr, id_var)
        );
        CREATE INDEX data_lt ON data(id_lev_tr);
    )");
    conn.exec(R"(
        CREATE TABLE attr (
           id_data     INTEGER NOT NULL REFERENCES data (id) ON DELETE CASCADE,
           type        INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL,
           UNIQUE (id_data, type)
        );
    )");
    conn.set_setting("version", "V7");
}
void Driver::delete_tables_v7()
{
    conn.drop_table_if_exists("attr");
    conn.drop_table_if_exists("data");
    conn.drop_table_if_exists("lev_tr");
    conn.drop_table_if_exists("repinfo");
    conn.drop_table_if_exists("station");
    conn.drop_settings();
}
void Driver::vacuum_v7()
{
    conn.exec(R"(
        DELETE FROM lev_tr WHERE id IN (
            SELECT ltr.id
              FROM lev_tr ltr
         LEFT JOIN data d ON d.id_lev_tr = ltr.id
             WHERE d.id_lev_tr is NULL)
    )");
    conn.exec(R"(
        DELETE FROM station WHERE id IN (
            SELECT p.id
              FROM station p
         LEFT JOIN data d ON d.id_station = p.id
             WHERE d.id is NULL)
    )");
}

void Driver::exec_no_data(const std::string& query)
{
    conn.exec(query);
}

void Driver::explain(const std::string& query)
{
    string explain_query = "EXPLAIN QUERY PLAN ";
    explain_query += query;

    fprintf(stderr, "%s\n", explain_query.c_str());
    auto stm = conn.sqlitestatement(explain_query);
    fprintf(stderr, "sid\torder\tfrom\tdetail\n");
    stm->execute([&]() {
        int selectid = stm->column_int(0);
        int order = stm->column_int(1);
        int from = stm->column_int(2);
        const char* detail = stm->column_string(3);
        fprintf(stderr, "%d\t%d\t%d\t%s\n", selectid, order, from, detail);
    });
}

}
}
}
}
