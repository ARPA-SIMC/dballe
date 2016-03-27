#include "datav6.h"
#include "dballe/sql/odbc.h"
#include "dballe/db/v6/qbuilder.h"
#include "dballe/record.h"
#include <algorithm>
#include <cstring>
#include <sqltypes.h>
#include <sql.h>

using namespace wreport;
using dballe::sql::ODBCConnection;
using dballe::sql::ODBCStatement;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v6 {
namespace odbc {

ODBCDataV6::ODBCDataV6(ODBCConnection& conn)
    : conn(conn)
{
}

ODBCDataV6::~ODBCDataV6()
{
}

void ODBCDataV6::insert(dballe::sql::Transaction& t, v6::bulk::InsertV6& vars, UpdateMode update_mode)
{
    Querybuf select_query(512);
    select_query.appendf(R"(
        SELECT id, id_lev_tr, id_var, value
          FROM data
         WHERE id_station=%d AND id_report=%d AND datetime={ts '%04d-%02d-%02d %02d:%02d:%02d'}
         ORDER BY id_lev_tr, id_var
    )", vars.id_station, vars.id_report,
        vars.datetime.year, vars.datetime.month, vars.datetime.day,
        vars.datetime.hour, vars.datetime.minute, vars.datetime.second);

    // Get the current status of variables for this context
    auto stm = conn.odbcstatement(select_query);

    int id_data;
    int id_levtr;
    int id_var;
    char value[255];
    stm->bind_out(1, id_data);
    stm->bind_out(2, id_levtr);
    stm->bind_out(3, id_var);
    stm->bind_out(4, value, 255);

    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    v6::bulk::AnnotateVarsV6 todo(vars);
    stm->execute();
    while (stm->fetch())
        todo.annotate(id_data, id_levtr, id_var, value);
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case UPDATE:
            if (todo.do_update)
            {
                auto update_stm = conn.odbcstatement("UPDATE data SET value=? WHERE id=?");
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    update_stm->bind_in(1, v.var->enqc());
                    update_stm->bind_in(2, v.id_data);
                    update_stm->execute_and_close();
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
        Querybuf dq(512);
        dq.appendf(R"(
            INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value)
                 VALUES (%d, %d, ?, '%04d-%02d-%02d %02d:%02d:%02d', ?, ?)
        )", vars.id_station, vars.id_report,
            vars.datetime.year, vars.datetime.month, vars.datetime.day,
            vars.datetime.hour, vars.datetime.minute, vars.datetime.second);
        auto insert = conn.odbcstatement(dq);
        int varcode;
        insert->bind_in(2, varcode);
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            insert->bind_in(1, v.id_levtr);
            varcode = v.var->code();
            insert->bind_in(3, v.var->enqc());
            insert->execute();
            v.id_data = conn.get_last_insert_id();
            v.set_inserted();
        }
    }
}

void ODBCDataV6::remove(const v6::QueryBuilder& qb)
{
    auto stm = conn.odbcstatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_in(1, qb.bind_in_ident);

    // Get the list of data to delete
    int out_id_data;
    stm->bind_out(1, out_id_data);

    // Compile the DELETE query for the data
    auto stmd = conn.odbcstatement("DELETE FROM data WHERE id=?");
    stmd->bind_in(1, out_id_data);

    // Compile the DELETE query for the attributes
    auto stma = conn.odbcstatement("DELETE FROM attr WHERE id_data=?");
    stma->bind_in(1, out_id_data);

    stm->execute();

    // Iterate all the data_id results, deleting the related data and attributes
    while (stm->fetch())
    {
        stmd->execute_ignoring_results();
        stma->execute_ignoring_results();
    }
}


void ODBCDataV6::dump(FILE* out)
{
    int id;
    int id_station;
    int id_report;
    int id_lev_tr;
    SQL_TIMESTAMP_STRUCT date;
    wreport::Varcode id_var;
    char value[255];
    SQLLEN value_ind;

    auto stm = conn.odbcstatement("SELECT id, id_station, id_report, id_lev_tr, datetime, id_var, value FROM data");
    stm->bind_out(1, id);
    stm->bind_out(2, id_station);
    stm->bind_out(3, id_report);
    stm->bind_out(4, id_lev_tr);
    stm->bind_out(5, date);
    stm->bind_out(6, id_var);
    stm->bind_out(7, value, 255, value_ind);
    stm->execute();
    int count;
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   rep ltr  datetime              var\n");
    for (count = 0; stm->fetch(); ++count)
    {
        char ltr[20];
        if (id_lev_tr == -1)
            strcpy(ltr, "----");
        else
            snprintf(ltr, 20, "%04d", (int)id_lev_tr);

        fprintf(out, " %4d %4d %3d %s %04d-%02d-%02d %02d:%02d:%02d.%d %01d%02d%03d",
                (int)id, (int)id_station, (int)id_report, ltr,
                (int)date.year, (int)date.month, (int)date.day,
                (int)date.hour, (int)date.minute, (int)date.second,
                (int)date.fraction,
                WR_VAR_F(id_var), WR_VAR_X(id_var), WR_VAR_Y(id_var));
        if (value_ind == SQL_NTS)
            fprintf(out, "\n");
        else
            fprintf(out, " %.*s\n", (int)value_ind, value);
    }
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
    stm->close_cursor();
}

}
}
}
}
