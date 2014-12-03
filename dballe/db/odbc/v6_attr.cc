/*
 * db/v6/odbc/attr - attribute table management
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "v6_attr.h"
#include "dballe/core/var.h"
#include <memory>
#include <sqltypes.h>
#include <sql.h>
#include <cstring>

using namespace std;

namespace dballe {
namespace db {
namespace v6 {

ODBCAttr::ODBCAttr(ODBCConnection& conn)
    : conn(conn), sstm(0), istm(0), rstm(0)
{
    const char* select_query =
        "SELECT type, value FROM attr WHERE id_data=?";
    const char* insert_query =
        "INSERT INTO attr (id_data, type, value)"
        " VALUES(?, ?, ?)";
    const char* replace_query_mysql =
        "INSERT INTO attr (id_data, type, value)"
        " VALUES(?, ?, ?) ON DUPLICATE KEY UPDATE value=VALUES(value)";
    const char* replace_query_sqlite =
        "INSERT OR REPLACE INTO attr (id_data, type, value)"
        " VALUES(?, ?, ?)";
    const char* replace_query_oracle =
        "MERGE INTO attr USING"
        " (SELECT ? as data, ? as t, ? as val FROM dual)"
        " ON (id_data=data AND type=t)"
        " WHEN MATCHED THEN UPDATE SET value=val"
        " WHEN NOT MATCHED THEN"
        "  INSERT (id_data, type, value) VALUES (data, t, val)";
    const char* replace_query_postgres =
        "UPDATE attr SET value=? WHERE id_data=? AND type=?";

    // Create the statement for select
    sstm = conn.odbcstatement().release();
    sstm->bind_in(1, id_data);
    sstm->bind_out(1, type);
    sstm->bind_out(2, value, sizeof(value));
    sstm->prepare(select_query);

    // Create the statement for insert
    istm = conn.odbcstatement().release();
    istm->bind_in(1, id_data);
    istm->bind_in(2, type);
    istm->bind_in(3, value, value_ind);
    istm->prepare(insert_query);

    // Create the statement for replace
    rstm = conn.odbcstatement().release();
    if (conn.server_type == POSTGRES)
    {
        rstm->bind_in(1, value, value_ind);
        rstm->bind_in(2, id_data);
        rstm->bind_in(3, type);
    } else {
        rstm->bind_in(1, id_data);
        rstm->bind_in(2, type);
        rstm->bind_in(3, value, value_ind);
    }
    switch (conn.server_type)
    {
        case MYSQL: rstm->prepare(replace_query_mysql); break;
        case SQLITE: rstm->prepare(replace_query_sqlite); break;
        case ORACLE: rstm->prepare(replace_query_oracle); break;
        case POSTGRES: rstm->prepare(replace_query_postgres); break;
        default: rstm->prepare(replace_query_mysql); break;
    }
}

ODBCAttr::~ODBCAttr()
{
    if (sstm) delete sstm;
    if (istm) delete istm;
    if (rstm) delete rstm;
}

void ODBCAttr::set_value(const char* qvalue)
{
    if (qvalue == NULL)
    {
        value[0] = 0;
        value_ind = SQL_NULL_DATA;
    } else {
        int len = strlen(qvalue);
        if (len > 255) len = 255;
        memcpy(value, qvalue, len);
        value[len] = 0;
        value_ind = len;
    }
}

void ODBCAttr::write(int id_data, const wreport::Var& var)
{
    this->id_data = id_data;
    type = var.code();
    set_value(var.value());

    if (conn.server_type == POSTGRES)
    {
        if (rstm->execute_and_close() == SQL_NO_DATA)
            istm->execute_and_close();
    } else
        rstm->execute_and_close();
}

void ODBCAttr::read(int id_data, wreport::Var& var)
{
    this->id_data = id_data;

    // Query all attributes for this var in the current context
    sstm->execute();

    // Make attribues from the result, and add them to var
    while (sstm->fetch())
        var.seta(auto_ptr<wreport::Var>(newvar(type, value).release()));

    sstm->close_cursor();
}

void ODBCAttr::dump(FILE* out)
{
    int id_data;
    wreport::Varcode type;
    char value[255];
    SQLLEN value_ind;

    auto stm = conn.odbcstatement();
    stm->bind_out(1, id_data);
    stm->bind_out(2, type);
    stm->bind_out(3, value, 255, value_ind);
    stm->exec_direct("SELECT id_data, type, value FROM attr");
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

