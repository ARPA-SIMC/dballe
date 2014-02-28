/*
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace {

struct db_tests_basic : public dballe::tests::db_test
{
};

}

namespace tut {

typedef db_tg<db_tests_basic> tg;
typedef tg::object to;


template<> template<> void to::test<1>()
{
    // Run twice to see if it is idempotent
    db->reset();
    db->reset();
}

template<> template<> void to::test<2>()
{
    // Test repinfo-related functions
    std::map<std::string, int> prios = db->get_repinfo_priorities();
    wassert(actual(prios.find("synop") != prios.end()).istrue());
    wassert(actual(prios["synop"]) == 101);

    int added, deleted, updated;
    db->update_repinfo((string(getenv("DBA_TESTDATA")) + "/test-repinfo1.csv").c_str(), &added, &deleted, &updated);

    wassert(actual(added) == 3);
    wassert(actual(deleted) == 11);
    wassert(actual(updated) == 2);

    prios = db->get_repinfo_priorities();
    wassert(actual(prios.find("fixspnpo") != prios.end()).istrue());
    wassert(actual(prios["fixspnpo"]) == 200);
}

template<> template<> void to::test<3>()
{
    // Just invoke vacuum
    db->vacuum();
}

template<> template<> void to::test<4>()
{
    // Test remove_all
    db->remove_all();
    Record query;
    std::auto_ptr<db::Cursor> cur = db->query_data(query);
    wassert(actual(cur->remaining()) == 0);

    // Check that it is idempotent
    db->remove_all();
    cur = db->query_data(query);
    wassert(actual(cur->remaining()) == 0);

    // Insert something
    wruntest(populate<OldDballeTestFixture>);

    cur = db->query_data(query);
    wassert(actual(cur->remaining()) == 4);

    db->remove_all();

    cur = db->query_data(query);
    wassert(actual(cur->remaining()) == 0);
}

}

namespace {

tut::tg db_tests_query_mem_tg("db_basic_mem", MEM);
#ifdef HAVE_ODBC
tut::tg db_tests_query_v5_tg("db_basic_v5", V5);
tut::tg db_tests_query_v6_tg("db_basic_v6", V6);
#endif

}
