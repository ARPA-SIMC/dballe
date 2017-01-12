#include "datav6.h"
#include "dballe/sql/mysql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/db/v6/qbuilder.h"
#include "dballe/record.h"
#include <algorithm>
#include <cstring>

using namespace wreport;
using namespace std;
using dballe::sql::MySQLConnection;
using dballe::sql::Querybuf;
using dballe::sql::mysql::Row;

namespace dballe {
namespace db {
namespace v6 {
namespace mysql {

MySQLDataV6::MySQLDataV6(MySQLConnection& conn)
    : conn(conn)
{
}

MySQLDataV6::~MySQLDataV6()
{
}

void MySQLDataV6::insert(dballe::sql::Transaction& t, v6::bulk::InsertV6& vars, UpdateMode update_mode)
{
    // Get the current status of variables for this context
    Querybuf select;
    select.appendf(R"(
        SELECT id, id_lev_tr, id_var, value
          FROM data
         WHERE id_station=%d AND id_report=%d AND datetime='%04d-%02d-%02d %02d:%02d:%02d'
         ORDER BY id_lev_tr, id_var
    )", vars.id_station, vars.id_report,
        vars.datetime.year, vars.datetime.month, vars.datetime.day,
        vars.datetime.hour, vars.datetime.minute, vars.datetime.second);
    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    v6::bulk::AnnotateVarsV6 todo(vars);
    auto select_res = conn.exec_store(select);
    while (auto row = select_res.fetch())
    {
        todo.annotate(
                row.as_int(0),
                row.as_int(1),
                row.as_int(2),
                row.as_cstring(3));
    }
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case UPDATE:
            if (todo.do_update)
            {
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    string escaped_value = conn.escape(v.var->enqc());
                    Querybuf update;
                    update.appendf("UPDATE data SET value='%s' WHERE id=%d", escaped_value.c_str(), v.id_data);
                    conn.exec_no_data(update);
                    v.set_updated();
                }
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
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            string escaped_value = conn.escape(v.var->enqc());
            Querybuf insert(512);
            insert.appendf(R"(
                INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value)
                     VALUES (%d, %d, %d, '%04d-%02d-%02d %02d:%02d:%02d', %d, '%s')
            )", vars.id_station, vars.id_report, v.id_levtr,
                vars.datetime.year, vars.datetime.month, vars.datetime.day,
                vars.datetime.hour, vars.datetime.minute, vars.datetime.second,
                v.var->code(), escaped_value.c_str());
            conn.exec_no_data(insert);
            v.id_data = conn.get_last_insert_id();
            v.set_inserted();
        }
    }
}

namespace {

struct Remover
{
    dballe::sql::MySQLConnection& conn;
    Querybuf dq;
    unsigned cur_size = 0;
    unsigned batch_size;

    Remover(dballe::sql::MySQLConnection& conn, unsigned batch_size=1024)
        : conn(conn), dq(512), batch_size(batch_size) {}

    void add(const char* id)
    {
        if (!cur_size)
        {
            dq.clear();
            dq.append("DELETE FROM data WHERE id IN (");
            dq.start_list(",");
        }
        dq.append_list(id);
        ++cur_size;
        if (cur_size >= batch_size)
            flush();
    }

    void flush()
    {
        if (!cur_size) return;
        dq.append(")");
        conn.exec_no_data(dq);
        cur_size = 0;
    }
};

}

void MySQLDataV6::remove(const v6::QueryBuilder& qb)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");
    Remover remover(conn);
    auto res = conn.exec_store(qb.sql_query);
    while (auto row = res.fetch())
        remover.add(row.as_cstring(0));
    remover.flush();
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
}
