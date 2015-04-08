/*
 * db/v6/attr - attr table management
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
#if 0
#include "db.h"
#include "dballe/db/sqlite/internals.h"
#include "dballe/db/sqlite/repinfo.h"
#include "dballe/db/sqlite/station.h"
#include "dballe/db/sqlite/levtr.h"
#include "dballe/db/sqlite/datav6.h"
#include "dballe/db/sqlite/v6_attr.h"
#include "dballe/db/sqlite/v6_run_query.h"
#include "dballe/db/postgresql/internals.h"
#include "dballe/db/postgresql/repinfo.h"
#include "dballe/db/postgresql/station.h"
#include "dballe/db/postgresql/levtr.h"
#include "dballe/db/postgresql/datav6.h"
#include "dballe/db/odbc/internals.h"
#include "dballe/db/odbc/repinfo.h"
#include "dballe/db/odbc/station.h"
#include "dballe/db/odbc/levtr.h"
#include "dballe/db/odbc/datav6.h"
#include "dballe/db/odbc/v6_attr.h"
#include "dballe/db/odbc/v6_run_query.h"
#include "dballe/core/record.h"
#include "dballe/msg/context.h"
#include "dballe/msg/msg.h"
#include <sstream>
#endif

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace sql {

AttrV6::~AttrV6() {}

}
}
}

