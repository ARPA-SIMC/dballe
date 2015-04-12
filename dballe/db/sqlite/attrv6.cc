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
#include "dballe/core/var.h"

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace sqlite {

SQLiteAttrV6::SQLiteAttrV6(SQLiteConnection& conn)
    : conn(conn)
{
    const char* select_query =
        "SELECT type, value FROM attr WHERE id_data=?";
    const char* replace_query =
        "INSERT OR REPLACE INTO attr (id_data, type, value)"
        " VALUES(?, ?, ?)";

    // Create the statement for select
    sstm = conn.sqlitestatement(select_query).release();

    // Create the statement for replace
    rstm = conn.sqlitestatement(replace_query).release();
}

SQLiteAttrV6::~SQLiteAttrV6()
{
    delete sstm;
    delete rstm;
}

void SQLiteAttrV6::impl_add(int id_data, sql::AttributeList& attrs)
{
    for (auto& i : attrs)
    {
        rstm->bind_val(1, id_data);
        rstm->bind_val(2, i.first);
        rstm->bind_val(3, i.second);
        rstm->execute();
    }
}

void SQLiteAttrV6::read(int id_data, function<void(unique_ptr<Var>)> dest)
{
    sstm->bind_val(1, id_data);
    sstm->execute([&]() {
        if (sstm->column_isnull(1))
            dest(newvar(sstm->column_int(0)));
        else
            dest(newvar(sstm->column_int(0), sstm->column_string(1).c_str()));
    });
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
        {
            string val = stm->column_string(2);
            fprintf(out, " %.*s\n", (int)val.size(), val.data());
        }
        ++count;
    });
    fprintf(out, "%d element%s in table attr\n", count, count != 1 ? "s" : "");
}

}
}
}

