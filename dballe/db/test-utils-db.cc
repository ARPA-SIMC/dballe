/*
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "test-utils-db.h"
#include <dballe/db/internals.h>
#include "dballe/db/v5/db.h"
#include <wreport/error.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace wreport;

namespace dballe {
namespace tests {

db_test::db_test(bool reset) : db(NULL)
{
    db = DB::connect_test();
    if (reset) db->reset();
}
db_test::~db_test()
{
}
void db_test::use_db()
{
    if (!has_db()) throw tut::no_such_test();
}

db::v5::DB& db_test::v5()
{
    if (db::v5::DB* d = dynamic_cast<db::v5::DB*>(db.get()))
        return *d;
    else
        throw error_consistency("test DB is not a v5 DB");
}

} // namespace tests
} // namespace dballe
