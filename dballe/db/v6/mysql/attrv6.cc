#include "attrv6.h"
#include "dballe/sql/mysql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/var.h"

using namespace std;
using namespace wreport;
using dballe::sql::MySQLConnection;
using dballe::sql::Querybuf;
using dballe::sql::mysql::Row;

namespace dballe {
namespace db {
namespace v6 {
namespace mysql {

MySQLAttrV6::MySQLAttrV6(MySQLConnection& conn)
    : conn(conn)
{
}

MySQLAttrV6::~MySQLAttrV6()
{
}

void MySQLAttrV6::insert(dballe::db::Transaction& t, v6::bulk::InsertAttrsV6& attrs, UpdateMode update_mode)
{
    Querybuf select_query;
    select_query.append("SELECT id_data, type, value FROM attr WHERE id_data IN (");
    select_query.start_list(",");
    int last_data_id = -1;
    for (const auto& a: attrs)
    {
        if (a.id_data == last_data_id) continue;
        select_query.append_listf("%d", a.id_data);
        last_data_id = a.id_data;
    }
    select_query.append(") ORDER BY id_data, type");

    // Get the current status of variables for this context
    v6::bulk::AnnotateAttrsV6 todo(attrs);
    auto select_res = conn.exec_store(select_query);
    while (auto row = select_res.fetch())
    {
        todo.annotate(
                row.as_int(0),
                row.as_int(1),
                row.as_cstring(2));
    }
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case UPDATE:
            if (todo.do_update)
            {
                for (auto& a: attrs)
                {
                    if (!a.needs_update()) continue;
                    string escaped_value = conn.escape(a.attr->enqc());
                    Querybuf update;
                    update.appendf("UPDATE attr SET value='%s' WHERE id_data=%d AND type=%d", escaped_value.c_str(), a.id_data, (int)a.attr->code());
                    conn.exec_no_data(update);
                    a.set_updated();
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
        Querybuf insert(512);
        insert.append("INSERT INTO attr (id_data, type, value) VALUES ");
        insert.start_list(",");
        for (auto& a: attrs)
        {
            if (!a.needs_insert()) continue;
            string escaped_value = conn.escape(a.attr->enqc());
            insert.append_listf("(%d, %d, '%s')", a.id_data, (int)a.attr->code(), escaped_value.c_str());
            a.set_inserted();
        }
        conn.exec_no_data(insert);
    }
}

void MySQLAttrV6::read(int id_data, function<void(unique_ptr<Var>)> dest)
{
    Querybuf q;
    q.appendf("SELECT type, value FROM attr WHERE id_data=%d", id_data);
    auto res = conn.exec_store(q);
    while (auto row = res.fetch())
    {
        if (row.isnull(1))
            dest(newvar(row.as_int(0)));
        else
            dest(newvar(row.as_int(0), row.as_cstring(1)));
    }
}

void MySQLAttrV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table attr:\n");
    auto res = conn.exec_store("SELECT id_data, type, value FROM attr");
    while (auto row = res.fetch())
    {
        Varcode type = row.as_int(1);
        fprintf(out, " %4d, %01d%02d%03d",
                row.as_int(0),
                WR_VAR_F(type), WR_VAR_X(type), WR_VAR_Y(type));
        if (row.isnull(2))
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", row.as_cstring(2));
        ++count;
    }
    fprintf(out, "%d element%s in table attr\n", count, count != 1 ? "s" : "");
}

}
}
}
}
