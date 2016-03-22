#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "levtr.h"
#include "datav6.h"
#include "attrv6.h"
#include "dballe/db/v6/qbuilder.h"
#include "dballe/sql/postgresql.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;
using dballe::sql::PostgreSQLConnection;
//using dballe::sql::Transaction;
//using dballe::sql::Querybuf;
using dballe::sql::error_postgresql;

namespace dballe {
namespace db {
namespace v6 {
namespace postgresql {

Driver::Driver(PostgreSQLConnection& conn)
    : conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<v6::Repinfo> Driver::create_repinfov6()
{
    return unique_ptr<v6::Repinfo>(new PostgreSQLRepinfoV6(conn));
}

std::unique_ptr<v6::Station> Driver::create_stationv6()
{
    return unique_ptr<v6::Station>(new PostgreSQLStationV6(conn));
}

std::unique_ptr<v6::LevTr> Driver::create_levtrv6()
{
    return unique_ptr<v6::LevTr>(new PostgreSQLLevTrV6(conn));
}

std::unique_ptr<v6::DataV6> Driver::create_datav6()
{
    return unique_ptr<v6::DataV6>(new PostgreSQLDataV6(conn));
}

std::unique_ptr<v6::AttrV6> Driver::create_attrv6()
{
    return unique_ptr<v6::AttrV6>(new PostgreSQLAttrV6(conn));
}

void Driver::run_built_query_v6(
        const v6::QueryBuilder& qb,
        std::function<void(v6::SQLRecordV6& rec)> dest)
{
    using namespace dballe::sql::postgresql;

    // fprintf(stderr, "QUERY %d %s\n", qb.bind_in_ident, qb.sql_query.c_str());

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

    // http://www.postgresql.org/docs/9.4/static/libpq-single-row-mode.html
    if (!PQsetSingleRowMode(conn))
    {
        string errmsg(PQerrorMessage(conn));
        conn.cancel_running_query_nothrow();
        conn.discard_all_input_nothrow();
        throw error_postgresql(errmsg, "cannot set single row mode for query " + qb.sql_query);
    }

    v6::SQLRecordV6 rec;
    while (true)
    {
        Result res(PQgetResult(conn));
        if (!res) break;

        // Note: Even when PQresultStatus indicates a fatal error, PQgetResult
        // should be called until it returns a null pointer to allow libpq to
        // process the error information completely.
        //  (http://www.postgresql.org/docs/9.1/static/libpq-async.html)

        // If we get what we don't want, cancel, flush our input and throw
        if (PQresultStatus(res) == PGRES_SINGLE_TUPLE)
        {
            // Ok, we have a tuple
        } else if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            // No more rows will arrive
            continue;
        } else {
            // An error arrived
            conn.cancel_running_query_nothrow();
            conn.discard_all_input_nothrow();

            switch (PQresultStatus(res))
            {
                case PGRES_COMMAND_OK:
                    throw error_postgresql("command_ok", "no data returned by query " + qb.sql_query);
                default:
                    throw error_postgresql(res, "executing " + qb.sql_query);
            }
        }
        // fprintf(stderr, "ST %d vi %d did %d d %d sd %d\n", qb.select_station, qb.select_varinfo, qb.select_data_id, qb.select_data, qb.select_summary_details);
        for (unsigned row = 0; row < res.rowcount(); ++row)
        {
            int output_seq = 0;
            if (qb.select_station)
            {
                rec.out_ana_id = res.get_int4(row, output_seq++);
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
                rec.out_rep_cod = res.get_int4(row, output_seq++);
                rec.out_id_ltr = res.get_int4(row, output_seq++);
                rec.out_varcode = res.get_int4(row, output_seq++);
            }

            if (qb.select_data_id)
                rec.out_id_data = res.get_int4(row, output_seq++);

            if (qb.select_data)
            {
                rec.out_datetime = res.get_timestamp(row, output_seq++);

                int value_len = min(PQgetlength(res, row, output_seq), 255);
                const char* value = res.get_string(row, output_seq++);
                memcpy(rec.out_value, value, value_len);
                rec.out_value[value_len] = 0;
            }

            if (qb.select_summary_details)
            {
                rec.out_id_data = res.get_int8(row, output_seq++);
                rec.out_datetime = res.get_timestamp(row, output_seq++);
                rec.out_datetimemax = res.get_timestamp(row, output_seq++);
            }

            // rec.dump(stderr);
            try {
                dest(rec);
            } catch (std::exception& e) {
                // If we get an exception from downstream, cancel, flush all
                // input and rethrow it
                conn.cancel_running_query_nothrow();
                conn.discard_all_input_nothrow();
                throw;
            }
        }
    }
}

void Driver::create_tables_v6()
{
    conn.exec_no_data(R"(
        CREATE TABLE station (
           id         SERIAL PRIMARY KEY,
           lat        INTEGER NOT NULL,
           lon        INTEGER NOT NULL,
           ident      VARCHAR(64)
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX pa_uniq ON station(lat, lon, ident);");
    conn.exec_no_data("CREATE INDEX pa_lon ON station(lon);");
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
        CREATE TABLE lev_tr (
           id          SERIAL PRIMARY KEY,
           ltype1      INTEGER NOT NULL,
           l1          INTEGER NOT NULL,
           ltype2      INTEGER NOT NULL,
           l2          INTEGER NOT NULL,
           ptype       INTEGER NOT NULL,
           p1          INTEGER NOT NULL,
           p2          INTEGER NOT NULL
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX lev_tr_uniq ON lev_tr(ltype1, l1, ltype2, l2, ptype, p1, p2);");
    conn.exec_no_data(R"(
        CREATE TABLE data (
           id          SERIAL PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           id_report   INTEGER NOT NULL REFERENCES repinfo (id) ON DELETE CASCADE,
           id_lev_tr   INTEGER NOT NULL,
           datetime    TIMESTAMP NOT NULL,
           id_var      INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX data_uniq on data(id_station, datetime, id_lev_tr, id_report, id_var);");
    conn.exec_no_data("CREATE INDEX data_ana ON data(id_station);");
    conn.exec_no_data("CREATE INDEX data_report ON data(id_report);");
    conn.exec_no_data("CREATE INDEX data_dt ON data(datetime);");
    conn.exec_no_data("CREATE INDEX data_lt ON data(id_lev_tr);");
    conn.exec_no_data(R"(
        CREATE TABLE attr (
           id_data     INTEGER NOT NULL REFERENCES data (id) ON DELETE CASCADE,
           type        INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX attr_uniq ON attr(id_data, type);");
/*
 * Not a good idea: it works on ALL inserts, even on those that should fail
    "CREATE RULE data_insert_or_update AS "
    " ON INSERT TO data "
    " WHERE (new.id_context, new.id_var) IN ( "
    " SELECT id_context, id_var "
    " FROM data "
    " WHERE id_context=new.id_context AND id_var=new.id_var) "
    " DO INSTEAD "
    " UPDATE data SET value=new.value "
    " WHERE id_context=new.id_context AND id_var=new.id_var",
*/
    /*"CREATE FUNCTION identity (val anyelement, val1 anyelement, OUT val anyelement) AS 'select $2' LANGUAGE sql STRICT",
    "CREATE AGGREGATE anyval ( basetype=anyelement, sfunc='identity', stype='anyelement' )",*/
    conn.set_setting("version", "V6");
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
    conn.exec_no_data(R"(
        DELETE FROM lev_tr WHERE id IN (
            SELECT ltr.id
              FROM lev_tr ltr
         LEFT JOIN data d ON d.id_lev_tr = ltr.id
             WHERE d.id_lev_tr is NULL)
    )");
    conn.exec_no_data(R"(
        DELETE FROM station WHERE id IN (
            SELECT p.id
              FROM station p
         LEFT JOIN data d ON d.id_station = p.id
             WHERE d.id is NULL)
    )");
}

void Driver::exec_no_data(const std::string& query)
{
    conn.exec_no_data(query);
}

void Driver::explain(const std::string& query)
{
    using namespace dballe::sql::postgresql;

    string explain_query = "EXPLAIN ";
    explain_query += query;

    fprintf(stderr, "%s\n", explain_query.c_str());
    Result res = conn.exec(explain_query);
    for (unsigned row = 0; row < res.rowcount(); ++row)
        fprintf(stderr, "  %s\n", res.get_string(row, 0));
}

}
}
}
}
