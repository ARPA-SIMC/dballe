/*
 * db/station - station table management
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

#include "station.h"
#include "dballe/db/odbc/station.h"
#include "dballe/db/odbc/internals.h"

using namespace wreport;
using namespace dballe::db;
using namespace std;

namespace dballe {
namespace db {
namespace v5 {

Station::~Station()
{
}

std::unique_ptr<Station> Station::create(Connection& conn)
{
    if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<Station>(new ODBCStation(*c));
    else
        throw error_unimplemented("v5 station not yet implemented for non-ODBC connectors");
}

}
}
}
