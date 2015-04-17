/*
 * db/odbc/attrv6 - attribute table management
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
#include "dballe/core/var.h"
#include <memory>
#include <sqltypes.h>
#include <sql.h>
#include <cstring>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace odbc {

ODBCAttrV6::ODBCAttrV6(ODBCConnection& conn)
    : conn(conn), sstm(0)
{
    // Create the statement for select
    sstm = conn.odbcstatement("SELECT type, value FROM attr WHERE id_data=?").release();
    sstm->bind_in(1, id_data);
    sstm->bind_out(1, type);
    sstm->bind_out(2, value, sizeof(value));
}

ODBCAttrV6::~ODBCAttrV6()
{
    if (sstm) delete sstm;
}

void ODBCAttrV6::read(int id_data, function<void(unique_ptr<Var>)> dest)
{
    this->id_data = id_data;

    // Query all attributes for this var in the current context
    sstm->execute();

    // Make variables out of the results and send them to dest
    while (sstm->fetch())
        dest(newvar(type, value));

    sstm->close_cursor();
}

void ODBCAttrV6::insert(Transaction& t, sql::bulk::InsertAttrsV6& attrs, UpdateMode update_mode)
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
    auto sstm = conn.odbcstatement(select_query);

    int id_data;
    int id_var;
    char value[255];
    sstm->bind_out(1, id_data);
    sstm->bind_out(2, id_var);
    sstm->bind_out(3, value, 255);

    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    sql::bulk::AnnotateAttrsV6 todo(attrs);
    sstm->execute();
    while (sstm->fetch())
        todo.annotate(id_data, id_var, value);
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case UPDATE:
            if (todo.do_update)
            {
                auto update_stm = conn.odbcstatement("UPDATE attr SET value=? WHERE id_data=? AND type=?");
                wreport::Varcode code;
                update_stm->bind_in(3, code);
                for (auto& a: attrs)
                {
                    if (!a.needs_update()) continue;
                    update_stm->bind_in(1, a.attr->value());
                    update_stm->bind_in(2, a.id_data);
                    code = a.attr->code();
                    update_stm->execute_and_close();
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
        auto insert = conn.odbcstatement("INSERT INTO attr (id_data, type, value) VALUES (?, ?, ?)");
        wreport::Varcode varcode;
        insert->bind_in(2, varcode);
        for (auto& a: attrs)
        {
            if (!a.needs_insert()) continue;
            insert->bind_in(1, a.id_data);
            varcode = a.attr->code();
            insert->bind_in(3, a.attr->value());
            insert->execute_and_close();
            a.set_inserted();
        }
    }
}

void ODBCAttrV6::dump(FILE* out)
{
    int id_data;
    wreport::Varcode type;
    char value[255];
    SQLLEN value_ind;

    auto stm = conn.odbcstatement("SELECT id_data, type, value FROM attr");
    stm->bind_out(1, id_data);
    stm->bind_out(2, type);
    stm->bind_out(3, value, 255, value_ind);
    stm->execute();
    int count;
    fprintf(out, "dump of table attr:\n");
    for (count = 0; stm->fetch(); ++count)
    {
        fprintf(out, " %4d, %01d%02d%03d",
                (int)id_data,
                WR_VAR_F(type), WR_VAR_X(type), WR_VAR_Y(type));
        if (value_ind == SQL_NTS)
                fprintf(out, "\n");
        else
                fprintf(out, " %.*s\n", (int)value_ind, value);
    }
    fprintf(out, "%d element%s in table attr\n", count, count != 1 ? "s" : "");
    stm->close_cursor();
}

}
}
}

