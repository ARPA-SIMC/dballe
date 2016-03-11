#include "datav6.h"
#include "dballe/sql/postgresql.h"
#include "dballe/db/v6/qbuilder.h"
#include "dballe/record.h"
#include <algorithm>
#include <cstring>
#include <sstream>

using namespace wreport;
using namespace std;
using dballe::sql::PostgreSQLConnection;
using dballe::sql::Querybuf;
using dballe::sql::error_postgresql;

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

void PostgreSQLDataV6::insert(dballe::sql::Transaction& t, sql::bulk::InsertV6& vars, UpdateMode update_mode)
{
    using namespace dballe::sql::postgresql;
    const char* select_query = R"(
        SELECT id, id_lev_tr, id_var, value
          FROM data
         WHERE id_station=$1::int4 AND id_report=$2::int4 AND datetime=$3::timestamp
         ORDER BY id_lev_tr, id_var
    )";

    // Get the current status of variables for this context
    Result existing(conn.exec(select_query, vars.id_station, vars.id_report, vars.datetime));

    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    sql::bulk::AnnotateVarsV6 todo(vars);
    for (unsigned row = 0; row < existing.rowcount(); ++row)
    {
        if (!todo.annotate(
                existing.get_int4(row, 0),
                existing.get_int4(row, 1),
                (Varcode)existing.get_int4(row, 2),
                existing.get_string(row, 3)))
            break;
    }
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case UPDATE:
            if (todo.do_update)
            {
                Querybuf dq(512);
                dq.append("UPDATE data as d SET value=i.value FROM (values ");
                dq.start_list(",");
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    const char* value = v.var->enqc();
                    char* escaped_val = PQescapeLiteral(conn, value, strlen(value));
                    if (!escaped_val)
                        throw error_postgresql(conn, string("escaping string '") + value + "'");
                    dq.append_listf("(%d, %s)", v.id_data, escaped_val);
                    PQfreemem(escaped_val);
                    v.set_updated();
                }
                dq.append(") AS i(id, value) WHERE d.id = i.id");
                //fprintf(stderr, "Update query: %s\n", dq.c_str());
                conn.exec_no_data(dq);
            }
            break;
        case IGNORE:
            break;
        case ERROR:
            if (todo.do_update)
                throw error_consistency("refusing to overwrite existing data");
    }


    if (todo.do_insert)
    {
        Querybuf dq(512);
        dq.append("INSERT INTO data (id, id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES ");
        dq.start_list(",");
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            const char* value = v.var->enqc();
            char* escaped_val = PQescapeLiteral(conn, value, strlen(value));
            if (!escaped_val)
                throw error_postgresql(conn, string("escaping string '") + value + "'");
            dq.append_listf("(DEFAULT, %d, %d, %d, '%04d-%02d-%02d %02d:%02d:%02d', %d, %s)",
                    vars.id_station, vars.id_report, v.id_levtr,
                    vars.datetime.year, vars.datetime.month, vars.datetime.day,
                    vars.datetime.hour, vars.datetime.minute, vars.datetime.second,
                    (int)v.var->code(), escaped_val);
            PQfreemem(escaped_val);
        }
        dq.append(" RETURNING id");

        //fprintf(stderr, "Insert query: %s\n", dq.c_str());

        // Run the insert query and read back the new IDs
        Result res(conn.exec(dq));
        unsigned row = 0;
        auto v = vars.begin();
        while (row < res.rowcount() && v != vars.end())
        {
            if (!v->needs_insert())
            {
               ++v;
               continue;
            }
            v->id_data = res.get_int4(row, 0);
            v->set_inserted();
            ++v;
            ++row;
        }
    }
}

void PostgreSQLDataV6::remove(const v6::QueryBuilder& qb)
{
    Querybuf dq(512);
    dq.append("DELETE FROM data WHERE id IN (");
    dq.append(qb.sql_query);
    dq.append(")");
    if (qb.bind_in_ident)
    {
        conn.exec_no_data(dq.c_str(), qb.bind_in_ident);
    } else {
        conn.exec_no_data(dq.c_str());
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

        fprintf(out, " %4d %4d %3d %s ",
                res.get_int4(row, 0),
                res.get_int4(row, 1),
                res.get_int4(row, 2),
                ltr);

        stringstream dtstr;
        datetime.print_iso8601(out, ' ', " ");
        fprintf(out, "%01d%02d%03d", WR_VAR_FXY(code));
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
