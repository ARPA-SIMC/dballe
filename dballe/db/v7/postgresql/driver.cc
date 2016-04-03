#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "levtr.h"
#include "data.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/sql/postgresql.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;
using dballe::sql::PostgreSQLConnection;
using dballe::sql::error_postgresql;

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

Driver::Driver(PostgreSQLConnection& conn)
    : v7::Driver(conn), conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<v7::Repinfo> Driver::create_repinfo()
{
    return unique_ptr<v7::Repinfo>(new PostgreSQLRepinfo(conn));
}

std::unique_ptr<v7::Station> Driver::create_station()
{
    return unique_ptr<v7::Station>(new PostgreSQLStation(conn));
}

std::unique_ptr<v7::LevTr> Driver::create_levtr()
{
    return unique_ptr<v7::LevTr>(new PostgreSQLLevTr(conn));
}

std::unique_ptr<v7::StationData> Driver::create_station_data()
{
    return unique_ptr<v7::StationData>(new PostgreSQLStationData(conn));
}

std::unique_ptr<v7::Data> Driver::create_data()
{
    return unique_ptr<v7::Data>(new PostgreSQLData(conn));
}

void Driver::run_built_query_v7(
        const v7::QueryBuilder& qb,
        std::function<void(v7::SQLRecordV7& rec)> dest)
{
    using namespace dballe::sql::postgresql;

    // Start the query asynchronously
    int res;
    if (qb.bind_in_ident)
    {
        const char* args[1] = { qb.bind_in_ident };
        res = PQsendQueryParams(conn, qb.sql_query.c_str(), 1, nullptr, args, nullptr, nullptr, 1);
    } else {
        res = PQsendQueryParams(conn, qb.sql_query.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 1);
    }
    if (!res)
        throw error_postgresql(conn, "executing " + qb.sql_query);

    v7::SQLRecordV7 rec;
    conn.run_single_row_mode(qb.sql_query, [&](const Result& res) {
        // fprintf(stderr, "ST %d vi %d did %d d %d sd %d\n", qb.select_station, qb.select_varinfo, qb.select_data_id, qb.select_data, qb.select_summary_details);
        for (unsigned row = 0; row < res.rowcount(); ++row)
        {
            int output_seq = 0;
            if (qb.select_station)
            {
                rec.out_ana_id = res.get_int4(row, output_seq++);
                rec.out_rep_cod = res.get_int4(row, output_seq++);
                rec.out_lat = res.get_int4(row, output_seq++);
                rec.out_lon = res.get_int4(row, output_seq++);
                if (res.is_null(row, output_seq))
                {
                    rec.out_ident_size = -1;
                    rec.out_ident[0] = 0;
                } else {
                    const char* ident = res.get_string(row, output_seq);
                    rec.out_ident_size = min(PQgetlength(res, row, output_seq), 63);
                    memcpy(rec.out_ident, ident, rec.out_ident_size);
                    rec.out_ident[rec.out_ident_size] = 0;
                }
                ++output_seq;
            }

            if (qb.select_varinfo)
            {
                if (!qb.query_station_vars)
                    rec.out_id_ltr = res.get_int4(row, output_seq++);
                rec.out_varcode = res.get_int4(row, output_seq++);
            }

            if (qb.select_data_id)
                rec.out_id_data = res.get_int4(row, output_seq++);

            if (qb.select_data)
            {
                if (!qb.query_station_vars)
                    rec.out_datetime = res.get_timestamp(row, output_seq++);

                int value_len = min(PQgetlength(res, row, output_seq), 255);
                const char* value = res.get_string(row, output_seq++);
                memcpy(rec.out_value, value, value_len);
                rec.out_value[value_len] = 0;
            }

            if (qb.select_summary_details)
            {
                rec.out_id_data = res.get_int8(row, output_seq++);
                if (!qb.query_station_vars)
                {
                    rec.out_datetime = res.get_timestamp(row, output_seq++);
                    rec.out_datetimemax = res.get_timestamp(row, output_seq++);
                }
            }

            // rec.dump(stderr);
            dest(rec);
        }
    });
}

void Driver::run_station_query(const v7::QueryBuilder& qb, std::function<void(int id, const StationDesc&)> dest)
{
    using namespace dballe::sql::postgresql;

    // Start the query asynchronously
    int res;
    if (qb.bind_in_ident)
    {
        const char* args[1] = { qb.bind_in_ident };
        res = PQsendQueryParams(conn, qb.sql_query.c_str(), 1, nullptr, args, nullptr, nullptr, 1);
    } else {
        res = PQsendQueryParams(conn, qb.sql_query.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 1);
    }
    if (!res)
        throw error_postgresql(conn, "executing " + qb.sql_query);

    StationDesc desc;
    conn.run_single_row_mode(qb.sql_query, [&](const Result& res) {
        for (unsigned row = 0; row < res.rowcount(); ++row)
        {
            int output_seq = 0;

            int id = res.get_int4(row, output_seq++);
            desc.rep = res.get_int4(row, output_seq++);
            desc.coords.lat = res.get_int4(row, output_seq++);
            desc.coords.lon = res.get_int4(row, output_seq++);

            if (res.is_null(row, output_seq))
                desc.ident.clear();
            else
                desc.ident = res.get_string(row, output_seq);

            dest(id, desc);
        }
    });
}

void Driver::create_tables_v7()
{
    conn.exec_no_data(R"(
        CREATE TABLE repinfo (
           id           INTEGER PRIMARY KEY,
           memo         VARCHAR(30) NOT NULL,
           description  VARCHAR(255) NOT NULL,
           prio         INTEGER NOT NULL,
           descriptor   CHAR(6) NOT NULL,
           tablea       INTEGER NOT NULL
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX ri_memo_uniq ON repinfo(memo);");
    conn.exec_no_data("CREATE UNIQUE INDEX ri_prio_uniq ON repinfo(prio);");

    conn.exec_no_data(R"(
        CREATE TABLE station (
           id     SERIAL PRIMARY KEY,
           rep    INTEGER NOT NULL REFERENCES repinfo (id) ON DELETE CASCADE,
           lat    INTEGER NOT NULL,
           lon    INTEGER NOT NULL,
           ident  VARCHAR(64)
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX pa_uniq ON station(rep, lat, lon, ident);");
    conn.exec_no_data("CREATE INDEX pa_lon ON station(lon);");

    conn.exec_no_data(R"(
        CREATE TABLE levtr (
           id       SERIAL PRIMARY KEY,
           ltype1   INTEGER NOT NULL,
           l1       INTEGER NOT NULL,
           ltype2   INTEGER NOT NULL,
           l2       INTEGER NOT NULL,
           pind     INTEGER NOT NULL,
           p1       INTEGER NOT NULL,
           p2       INTEGER NOT NULL
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX levtr_uniq ON levtr(ltype1, l1, ltype2, l2, pind, p1, p2);");

    conn.exec_no_data(R"(
        CREATE TABLE station_data (
           id          SERIAL PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           code        INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL,
           attrs       BYTEA
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX station_data_uniq on station_data(id_station, code);");

    conn.exec_no_data(R"(
        CREATE TABLE data (
           id          SERIAL PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           id_levtr    INTEGER NOT NULL REFERENCES levtr(id) ON DELETE CASCADE,
           datetime    TIMESTAMP NOT NULL,
           code        INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL,
           attrs       BYTEA
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX data_uniq on data(id_station, datetime, id_levtr, code);");
    // When possible, replace with a postgresql 9.5 BRIN index
    conn.exec_no_data("CREATE INDEX data_dt ON data(datetime);");

    conn.set_setting("version", "V7");
}
void Driver::delete_tables_v7()
{
    conn.drop_table_if_exists("data");
    conn.drop_table_if_exists("station_data");
    conn.drop_table_if_exists("levtr");
    conn.drop_table_if_exists("station");
    conn.drop_table_if_exists("repinfo");
    conn.drop_settings();
}
void Driver::vacuum_v7()
{
    conn.exec_no_data(R"(
        DELETE FROM levtr WHERE id IN (
            SELECT ltr.id
              FROM levtr ltr
         LEFT JOIN data d ON d.id_levtr = ltr.id
             WHERE d.id_levtr is NULL)
    )");
    conn.exec_no_data(R"(
        DELETE FROM station WHERE id IN (
            SELECT p.id
              FROM station p
         LEFT JOIN data d ON d.id_station = p.id
             WHERE d.id is NULL)
    )");
}

}
}
}
}
