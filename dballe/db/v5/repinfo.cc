/*
 * db/v5/repinfo - repinfo table management
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

#include "repinfo.h"
#include "dballe/db/odbc/internals.h"
#include "dballe/db/odbc/repinfo.h"
#include "dballe/db/sql.h"
#include <wreport/error.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v5 {

Repinfo::Repinfo(Connection& conn)
    : conn(conn)
{
}

std::unique_ptr<Repinfo> Repinfo::create(Connection& conn)
{
    if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<Repinfo>(new ODBCRepinfo(*c));
    else
        throw error_unimplemented("v5 DB repinfo not yet implemented for non-ODBC connectors");
}


}
}
}
