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
    sstm = conn.odbcstatement(select_query).release();
    sstm->bind_in(1, id_data);
    sstm->bind_out(1, type);
    sstm->bind_out(2, value, sizeof(value));

    // Create the statement for insert
    istm = conn.odbcstatement(insert_query).release();
    istm->bind_in(1, id_data);
    istm->bind_in(2, type);
    istm->bind_in(3, value, value_ind);

    // Create the statement for replace
    switch (conn.server_type)
    {
        case ServerType::MYSQL: rstm = conn.odbcstatement(replace_query_mysql).release(); break;
        case ServerType::SQLITE: rstm = conn.odbcstatement(replace_query_sqlite).release(); break;
        case ServerType::ORACLE: rstm = conn.odbcstatement(replace_query_oracle).release(); break;
        case ServerType::POSTGRES: rstm = conn.odbcstatement(replace_query_postgres).release(); break;
        default: rstm = conn.odbcstatement(replace_query_mysql).release(); break;
    }
    if (conn.server_type == ServerType::POSTGRES)
    {
        rstm->bind_in(1, value, value_ind);
        rstm->bind_in(2, id_data);
        rstm->bind_in(3, type);
    } else {
        rstm->bind_in(1, id_data);
        rstm->bind_in(2, type);
        rstm->bind_in(3, value, value_ind);
    }
}

ODBCAttrV6::~ODBCAttrV6()
{
    if (sstm) delete sstm;
    if (istm) delete istm;
    if (rstm) delete rstm;
}

void ODBCAttrV6::set_value(const char* qvalue)
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

void ODBCAttrV6::impl_add(int id_data, sql::AttributeList& attrs)
{
    this->id_data = id_data;
    for (auto i : attrs)
    {
        type = i.first;
        set_value(i.second);

        if (conn.server_type == ServerType::POSTGRES)
        {
            if (rstm->execute_and_close() == SQL_NO_DATA)
                istm->execute_and_close();
        } else
            rstm->execute_and_close();
    }
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

