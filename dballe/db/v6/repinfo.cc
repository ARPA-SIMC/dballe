/*
 * db/v6/repinfo - repinfo table management
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "repinfo.h"
#include "dballe/db/odbc/internals.h"
#include <wreport/error.h>

using namespace wreport;

namespace dballe {
namespace db {
namespace v6 {

Repinfo::Repinfo(Connection* conn) : v5::Repinfo(conn) {}

int Repinfo::id_use_count(unsigned id, const char* name)
{
    DBALLE_SQL_C_UINT_TYPE dbid = id;
    DBALLE_SQL_C_UINT_TYPE count;
    db::Statement stm(*conn);
    stm.prepare("SELECT COUNT(1) FROM data WHERE id_report = ?");
    stm.bind_in(1, dbid);
    stm.bind_out(1, count);
    stm.execute();
    if (!stm.fetch_expecting_one())
        error_consistency::throwf("%s is in cache but not in the database (database externally modified?)", name);
    return count;
}

}
}
}
