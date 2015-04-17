/*
 * db/sqlite/attrv6 - attribute table management
 *
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */
#include "attrv6.h"
#include "dballe/db/sql/internals.h"
#include "dballe/db/querybuf.h"
#include "dballe/core/var.h"

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace sqlite {

SQLiteAttrV6::SQLiteAttrV6(SQLiteConnection& conn)
    : conn(conn)
{
    // Precompile the statement for select
    sstm = conn.sqlitestatement("SELECT type, value FROM attr WHERE id_data=?").release();
}

SQLiteAttrV6::~SQLiteAttrV6()
{
    delete sstm;
    delete istm;
    delete ustm;
}

void SQLiteAttrV6::read(int id_data, function<void(unique_ptr<Var>)> dest)
{
    sstm->bind_val(1, id_data);
    sstm->execute([&]() {
        if (sstm->column_isnull(1))
            dest(newvar(sstm->column_int(0)));
        else
            dest(newvar(sstm->column_int(0), sstm->column_string(1)));
    });
}

void SQLiteAttrV6::insert(Transaction& t, sql::bulk::InsertAttrsV6& attrs, UpdateMode update_mode)
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
    auto sstm = conn.sqlitestatement(select_query);

    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    sql::bulk::AnnotateAttrsV6 todo(attrs);
    sstm->execute([&]() {
        todo.annotate(
                sstm->column_int(0),
                sstm->column_int(1),
                sstm->column_string(2));
    });
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case UPDATE:
            if (todo.do_update)
            {
                if (!ustm) ustm = conn.sqlitestatement("UPDATE attr SET value=? WHERE id_data=? AND type=?").release();
                for (auto& v: attrs)
                {
                    if (!v.needs_update()) continue;
                    ustm->bind(v.attr->value(), v.id_data, v.attr->code());
                    ustm->execute();
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
        if (!istm) istm = conn.sqlitestatement("INSERT INTO attr (id_data, type, value) VALUES (?, ?, ?)").release();
        for (auto& v: attrs)
        {
            if (!v.needs_insert()) continue;
            istm->bind(v.id_data, v.attr->code(), v.attr->value());
            istm->execute();
            v.set_inserted();
        }
    }
}

void SQLiteAttrV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table attr:\n");
    auto stm = conn.sqlitestatement("SELECT id_data, type, value FROM attr");
    stm->execute([&]() {
        Varcode type = stm->column_int(1);
        fprintf(out, " %4d, %01d%02d%03d",
                stm->column_int(0),
                WR_VAR_F(type), WR_VAR_X(type), WR_VAR_Y(type));
        if (stm->column_isnull(2))
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", stm->column_string(2));
        ++count;
    });
    fprintf(out, "%d element%s in table attr\n", count, count != 1 ? "s" : "");
}

}
}
}

