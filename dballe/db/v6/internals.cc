/*
 * db/v6/attr - attr table management
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

#include "internals.h"
#include "db.h"
#include "dballe/db/odbc/v6_data.h"
#include "dballe/db/odbc/v6_attr.h"

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v6 {

Data::~Data() {}

unique_ptr<Data> Data::create(DB& db)
{
    return unique_ptr<Data>(new ODBCData(db));
}


Attr::~Attr() {}
unique_ptr<Attr> Attr::create(DB& db)
{
    return unique_ptr<Attr>(new ODBCAttr(*db.conn));
}

}
}
}

