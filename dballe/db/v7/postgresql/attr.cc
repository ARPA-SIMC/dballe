#include "attr.h"
#include "dballe/db/v7/internals.h"
#include "dballe/sql/querybuf.h"
#include "dballe/sql/postgresql.h"
#include "dballe/var.h"
#include <cstring>

using namespace std;
using namespace wreport;
using namespace dballe::sql::postgresql;
using dballe::sql::PostgreSQLConnection;
using dballe::sql::Querybuf;
using dballe::sql::error_postgresql;

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

PostgreSQLAttr::PostgreSQLAttr(PostgreSQLConnection& conn, const std::string& table_name, std::unordered_set<int> State::* new_ids)
    : Attr(table_name, new_ids), conn(conn)
{
    // Precompile the statement for select
    conn.prepare(table_name + "v7_select", "SELECT code, value FROM " + table_name + " WHERE id_data=$1::int4");
}

PostgreSQLAttr::~PostgreSQLAttr()
{
}

void PostgreSQLAttr::read(int id_data, function<void(unique_ptr<Var>)> dest)
{
    Result res = conn.exec_prepared(table_name + "v7_select", id_data);
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        if (res.is_null(row, 1))
            dest(newvar(res.get_int4(row, 0)));
        else
            dest(newvar(res.get_int4(row, 0), res.get_string(row, 1)));
    }
}

void PostgreSQLAttr::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertAttrsV7& attrs, UpdateMode update_mode)
{
    Querybuf select_query;
    select_query.appendf("SELECT id_data, code, value FROM %s WHERE id_data IN (", table_name.c_str());
    select_query.start_list(",");
    int last_data_id = -1;
    bool do_select = false;
    for (const auto& a: attrs)
    {
        if (a.id_data == last_data_id) continue;
        last_data_id = a.id_data;
        if (attrs.id_data_new.find(a.id_data) != attrs.id_data_new.end()) continue;
        select_query.append_listf("%d", a.id_data);
        do_select = true;
    }
    select_query.append(") ORDER BY id_data, code");

    v7::bulk::AnnotateAttrsV7 todo(attrs);
    if (do_select)
    {
        Result res_current(conn.exec(select_query));
        for (unsigned row = 0; row < res_current.rowcount(); ++row)
        {
            todo.annotate(
                    res_current.get_int4(row, 0),
                    res_current.get_int4(row, 1),
                    res_current.get_string(row, 2));
        }
    }
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case UPDATE:
            if (todo.do_update)
            {
                Querybuf dq(512);
                dq.append("UPDATE " + table_name + " as a SET value=i.value FROM (values ");
                dq.start_list(",");
                for (auto& a: attrs)
                {
                    if (!a.needs_update()) continue;
                    const char* value = a.attr->enqc();
                    char* escaped_val = PQescapeLiteral(conn, value, strlen(value));
                    if (!escaped_val)
                        throw error_postgresql(conn, string("cannot escape string '") + value + "'");
                    dq.append_listf("(%d, %d, %s)", a.id_data, (int)a.attr->code(), escaped_val);
                    PQfreemem(escaped_val);
                    a.set_updated();
                }
                dq.append(") AS i(id_data, code, value) WHERE a.id_data = i.id_data and a.code = i.code");
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
        dq.append("INSERT INTO " + table_name + " (id_data, code, value) VALUES ");
        dq.start_list(",");
        for (auto& a: attrs)
        {
            if (!a.needs_insert()) continue;
            const char* value = a.attr->enqc();
            char* escaped_val = PQescapeLiteral(conn, value, strlen(value));
            if (!escaped_val)
                throw error_postgresql(conn, string("cannot escape string '") + value + "'");
            dq.append_listf("(%d, %d, %s)", a.id_data, (int)a.attr->code(), escaped_val);
            PQfreemem(escaped_val);
            a.set_inserted();
        }
        //fprintf(stderr, "Insert query: %s\n", dq.c_str());

        // Run the insert query and read back the new IDs
        conn.exec_no_data(dq);
    }
}

void PostgreSQLAttr::_dump(std::function<void(int, wreport::Varcode, const char*)> out)
{
    auto res = conn.exec("SELECT id_data, code, value FROM " + table_name);
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        const char* val = res.is_null(row, 2) ? nullptr : res.get_string(row, 2);
        out(res.get_int4(row, 0), res.get_int4(row, 1), val);
    }
}

}
}
}
}
