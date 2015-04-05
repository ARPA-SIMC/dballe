/*
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

#include "config.h"
#include "db/test-utils-db.h"
#include "db/v5/db.h"
#include "db/v6/db.h"
#include "db/sql/repinfo.h"

using namespace dballe;
using namespace dballe::tests;
using namespace std;
using namespace wreport;
using namespace wibble::tests;

namespace {

struct db_sql_repinfo : public dballe::tests::db_test
{
    db::sql::Repinfo& repinfo()
    {
        if (db::v5::DB* db5 = dynamic_cast<db::v5::DB*>(db.get()))
            return db5->repinfo();
        if (db::v6::DB* db6 = dynamic_cast<db::v6::DB*>(db.get()))
            return db6->repinfo();
        throw error_consistency("cannot test repinfo on the current DB");
    }
};

}

namespace tut {

typedef db_tg<db_sql_repinfo> tg;
typedef tg::object to;

// Test simple queries
template<> template<> void to::test<1>()
{
    use_db();
    auto& ri = repinfo();

    wassert(actual(ri.get_id("synop")) == 1);
    wassert(actual(ri.get_id("generic")) == 255);
    wassert(actual(ri.get_rep_memo(1)) == "synop");
    wassert(actual(ri.get_priority(199)) == INT_MAX);
}

// Test update
template<> template<> void to::test<2>()
{
    use_db();
    auto& ri = repinfo();

    wassert(actual(ri.get_id("synop")) == 1);

    int added, deleted, updated;
    ri.update(NULL, &added, &deleted, &updated);

    wassert(actual(added) == 0);
    wassert(actual(deleted) == 0);
    wassert(actual(updated) == 13);

    wassert(actual(ri.get_id("synop")) == 1);
}

// Test update from a file that was known to fail
template<> template<> void to::test<3>()
{
    use_db();
    auto& ri = repinfo();

    wassert(actual(ri.get_id("synop")) == 1);

    int added, deleted, updated;
    ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo1.csv").c_str(), &added, &deleted, &updated);

    wassert(actual(added) == 3);
    wassert(actual(deleted) == 11);
    wassert(actual(updated) == 2);

    wassert(actual(ri.get_id("synop")) == 1);
    wassert(actual(ri.get_id("FIXspnpo")) == 201);
}

// Test update from a file with a negative priority
template<> template<> void to::test<4>()
{
    use_db();
    auto& ri = repinfo();

    int id = ri.get_id("generic");
    wassert(actual(ri.get_priority(id)) == 1000);

    int added, deleted, updated;
    ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo2.csv").c_str(), &added, &deleted, &updated);

    wassert(actual(added) == 3);
    wassert(actual(deleted) == 11);
    wassert(actual(updated) == 2);

    wassert(actual(ri.get_priority(id)) == -5);
}

// Test automatic repinfo creation
template<> template<> void to::test<5>()
{
    use_db();
    auto& ri = repinfo();

    int id = ri.obtain_id("foobar");
    wassert(actual(id) > 0);
    wassert(actual(ri.get_rep_memo(id)) == "foobar");
    wassert(actual(ri.get_priority(id)) == 1001);

    id = ri.obtain_id("barbaz");
    wassert(actual(id) > 0);
    wassert(actual(ri.get_rep_memo(id)) == "barbaz");
    wassert(actual(ri.get_priority(id)) == 1002);
}

}

namespace {

#ifdef HAVE_ODBC
tut::tg v5_tg("db_sql_repinfo_v5", db::V5);
#endif
tut::tg v6_tg("db_sql_repinfo_v6", db::V6);

}
