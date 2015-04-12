/*
 * db/postgresql/attrv6 - attribute table management
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <cstring>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace postgresql {

PostgreSQLAttrV6::PostgreSQLAttrV6(PostgreSQLConnection& conn)
    : conn(conn)
{
    conn.prepare("attrv6_select", R"(
        SELECT type, value FROM attr WHERE id_data=$1::int4
    )");
    conn.prepare("attrv6_select_existing", R"(
        SELECT type, value FROM attr WHERE id_data=$1::int4
    )");
    conn.prepare("attrv6_insert", R"(
        INSERT INTO attr (id_data, type, value) VALUES ($1::int4, $2::int4, $3::text)
    )");
    conn.prepare("attrv6_update", R"(
        UPDATE attr SET value=$3::text WHERE id_data=$1::int4 AND type=$2::int4
    )");
}

PostgreSQLAttrV6::~PostgreSQLAttrV6()
{
}

void PostgreSQLAttrV6::impl_add(int id_data, sql::AttributeList& attrs)
{
    using namespace postgresql;

    auto trans = conn.transaction();
    conn.exec_no_data("LOCK TABLE attr IN EXCLUSIVE MODE");

    // Get the current status of attributes
    Result res_current(conn.exec_prepared("attrv6_select_existing", id_data));
    for (unsigned row = 0; row < res_current.rowcount(); ++row)
    {
        Varcode code = res_current.get_int4(row, 0);
        if (const char* val = attrs.pop(code))
        {
            if (strcmp(val, res_current.get_string(row, 1)) == 0)
                // Same value, nothing to do
                continue;

            // Update
            conn.exec_prepared_no_data("attrv6_update", id_data, (int)code, val);
        }
    }
    for (auto& a: attrs)
        // Insert
        conn.exec_prepared_no_data("attrv6_insert", id_data, a.first, a.second);

#if 0
    // Send the new attributes to the server in a temporary table
    conn.exec_no_data("CREATE TEMPORARY TABLE attrv6_insert_tmp (type INTEGER NOT NULL, value VARCHAR(255) NOT NULL) ON COMMIT DROP");
    for (const auto& val: attrs)
        conn.exec_no_data("INSERT INTO attrv6_insert_tmp VALUES ($1::int4, $2::text)", (int)val.first, val.second);

    // Perform updates
    conn.exec_no_data(R"(
        UPDATE attr a
           SET value=i.value
          FROM attrv6_insert_tmp i
         WHERE a.id_data=$1::int4 AND a.type=i.type
    )", id_data);

    // Perform inserts
    conn.exec_no_data(R"(
        INSERT INTO attr (id_data, type, value) (
             SELECT $1::int4, i.type, i.value
               FROM attrv6_insert_tmp i
              WHERE NOT EXISTS (SELECT 1 FROM attr WHERE id_data=$1::int4 AND type=i.type)
        )
    )", id_data);
#endif

    trans->commit();
}

void PostgreSQLAttrV6::read(int id_data, function<void(unique_ptr<Var>)> dest)
{
    using namespace postgresql;
    Result res = conn.exec_prepared("attrv6_select", id_data);
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        if (res.is_null(row, 1))
            dest(newvar(res.get_int4(row, 0)));
        else
            dest(newvar(res.get_int4(row, 0), res.get_string(row, 1)));
    }
}

void PostgreSQLAttrV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table attr:\n");
    auto res = conn.exec("SELECT id_data, type, value FROM attr");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        Varcode type = res.get_int4(row, 1);
        const char* val = res.get_string(row, 2);
        fprintf(out, " %4d, %01d%02d%03d %s",
                res.get_int4(row, 0),
                WR_VAR_F(type), WR_VAR_X(type), WR_VAR_Y(type),
                val);
        ++count;
    }
    fprintf(out, "%d element%s in table attr\n", count, count != 1 ? "s" : "");
}

}
}
}

