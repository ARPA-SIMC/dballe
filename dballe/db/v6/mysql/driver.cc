#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "levtr.h"
#include "datav6.h"
#include "attrv6.h"
#include "dballe/db/v6/qbuilder.h"
#include "dballe/sql/mysql.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;
using dballe::sql::MySQLConnection;
using dballe::sql::mysql::Row;

namespace dballe {
namespace db {
namespace v6 {
namespace mysql {

Driver::Driver(MySQLConnection& conn)
    : conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<v6::Repinfo> Driver::create_repinfov6()
{
    return unique_ptr<v6::Repinfo>(new MySQLRepinfoV6(conn));
}

std::unique_ptr<v6::Station> Driver::create_stationv6()
{
    return unique_ptr<v6::Station>(new MySQLStationV6(conn));
}

std::unique_ptr<v6::LevTr> Driver::create_levtrv6()
{
    return unique_ptr<v6::LevTr>(new MySQLLevTrV6(conn));
}

std::unique_ptr<v6::DataV6> Driver::create_datav6()
{
    return unique_ptr<v6::DataV6>(new MySQLDataV6(conn));
}

std::unique_ptr<v6::AttrV6> Driver::create_attrv6()
{
    return unique_ptr<v6::AttrV6>(new MySQLAttrV6(conn));
}

void Driver::run_built_query_v6(
        const v6::QueryBuilder& qb,
        std::function<void(v6::SQLRecordV6& rec)> dest)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");

    v6::SQLRecordV6 rec;
    conn.exec_use(qb.sql_query, [&](const Row& row) {
        int output_seq = 0;
        if (qb.select_station)
        {
            rec.out_ana_id = row.as_int(output_seq++);
            rec.out_lat = row.as_int(output_seq++);
            rec.out_lon = row.as_int(output_seq++);
            if (row.isnull(output_seq))
            {
                rec.out_ident_size = -1;
                rec.out_ident[0] = 0;
            } else {
                const char* ident = row.as_cstring(output_seq);
                rec.out_ident_size = min(strlen(ident), (string::size_type)63);
                memcpy(rec.out_ident, ident, rec.out_ident_size);
                rec.out_ident[rec.out_ident_size] = 0;
            }
            ++output_seq;
        }

        if (qb.select_varinfo)
        {
            rec.out_rep_cod = row.as_int(output_seq++);
            rec.out_id_ltr = row.as_int(output_seq++);
            rec.out_varcode = row.as_int(output_seq++);
        }

        if (qb.select_data_id)
            rec.out_id_data = row.as_int(output_seq++);

        if (qb.select_data)
        {
            rec.out_datetime = row.as_datetime(output_seq++);

            const char* value = row.as_cstring(output_seq++);
            unsigned val_size = min(strlen(value), (string::size_type)255);
            memcpy(rec.out_value, value, val_size);
            rec.out_value[val_size] = 0;
        }

        if (qb.select_summary_details)
        {
            rec.out_id_data = row.as_int(output_seq++);
            rec.out_datetime = row.as_datetime(output_seq++);
            rec.out_datetimemax = row.as_datetime(output_seq++);
        }

        dest(rec);
    });
}

void Driver::create_tables_v6()
{
    conn.exec_no_data(R"(
        CREATE TABLE station (
           id         INTEGER auto_increment PRIMARY KEY,
           lat        INTEGER NOT NULL,
           lon        INTEGER NOT NULL,
           ident      CHAR(64),
           UNIQUE INDEX(lat, lon, ident(8)),
           INDEX(lon)
        ) ENGINE=InnoDB;
    )");
    conn.exec_no_data(R"(
        CREATE TABLE repinfo (
           id           SMALLINT PRIMARY KEY,
           memo         VARCHAR(20) NOT NULL,
           description  VARCHAR(255) NOT NULL,
           prio         INTEGER NOT NULL,
           descriptor   CHAR(6) NOT NULL,
           tablea       INTEGER NOT NULL,
           UNIQUE INDEX (prio),
           UNIQUE INDEX (memo)
        ) ENGINE=InnoDB;
    )");
    conn.exec_no_data(R"(
        CREATE TABLE lev_tr (
           id          INTEGER auto_increment PRIMARY KEY,
           ltype1      INTEGER NOT NULL,
           l1          INTEGER NOT NULL,
           ltype2      INTEGER NOT NULL,
           l2          INTEGER NOT NULL,
           ptype       INTEGER NOT NULL,
           p1          INTEGER NOT NULL,
           p2          INTEGER NOT NULL,
           UNIQUE INDEX (ltype1, l1, ltype2, l2, ptype, p1, p2)
       ) ENGINE=InnoDB;
    )");
    conn.exec_no_data(R"(
        CREATE TABLE data (
           id          INTEGER auto_increment PRIMARY KEY,
           id_station  SMALLINT NOT NULL,
           id_report   INTEGER NOT NULL,
           id_lev_tr   INTEGER NOT NULL,
           datetime    DATETIME NOT NULL,
           id_var      SMALLINT NOT NULL,
           value       VARCHAR(255) NOT NULL,
           UNIQUE INDEX(id_station, datetime, id_lev_tr, id_report, id_var),
           INDEX(datetime),
           INDEX(id_lev_tr)
       ) ENGINE=InnoDB;
    )");
    conn.exec_no_data(R"(
        CREATE TABLE attr (
           id_data     INTEGER NOT NULL,
           type        SMALLINT NOT NULL,
           value       VARCHAR(255) NOT NULL,
           UNIQUE INDEX (id_data, type)
       ) ENGINE=InnoDB;
    )");
}
void Driver::delete_tables_v6()
{
    conn.drop_table_if_exists("attr");
    conn.drop_table_if_exists("data");
    conn.drop_table_if_exists("lev_tr");
    conn.drop_table_if_exists("repinfo");
    conn.drop_table_if_exists("station");
    conn.drop_settings();
}
void Driver::vacuum_v6()
{
    conn.exec_no_data("DELETE c FROM lev_tr c LEFT JOIN data d ON d.id_lev_tr = c.id WHERE d.id_lev_tr IS NULL");
    conn.exec_no_data("DELETE p FROM station p LEFT JOIN data d ON d.id_station = p.id WHERE d.id IS NULL");
}

void Driver::exec_no_data(const std::string& query)
{
    conn.exec_no_data(query);
}

void Driver::explain(const std::string& query)
{
    string explain_query = "EXPLAIN EXTENDED ";
    explain_query += query;

    fprintf(stderr, "%s\n", explain_query.c_str());
    fprintf(stderr, "sid\tstype\ttable\ttype\tpos_ks\tkey\tkeylen\tref\trows\textra\n");
    conn.exec_use(explain_query, [&](const Row& row) {
        fprintf(stderr, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                row.as_cstring(0),
                row.as_cstring(1),
                row.as_cstring(2),
                row.as_cstring(3),
                row.as_cstring(4),
                row.as_cstring(5),
                row.as_cstring(6),
                row.as_cstring(7),
                row.as_cstring(8),
                row.as_cstring(9));

// 0 id: 1
// 1 select_type: PRIMARY
// 2 table: t1
// 3 type: index
// 4 possible_keys: NULL
// 5 key: PRIMARY
// 6 key_len: 4
// 7 ref: NULL
// 8 rows: 4
// 9 Extra: Using index
    });

    fprintf(stderr, "level\tcode\tmessage\n");
    conn.exec_use("SHOW WARNINGS", [&](const Row& row) {
        fprintf(stderr, "%s\t%s\t%s\n",
                row.as_cstring(0),
                row.as_cstring(1),
                row.as_cstring(2));
    });
}

}
}
}
}
