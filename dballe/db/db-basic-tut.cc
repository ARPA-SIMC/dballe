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

#include "db/test-utils-db.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct db_basic_shar : public dballe::tests::DB_test_base
{
    void test_reset();
    void test_repinfo();
};
TESTGRP(db_basic);

template<> template<> void to::test<1>() { use_db(db::V5); test_reset(); }
template<> template<> void to::test<2>() { use_db(db::V6); test_reset(); }
template<> template<> void to::test<3>() { use_db(db::MEM); test_reset(); }
void db_basic_shar::test_reset()
{
    // Run twice to see if it is idempotent
    db->reset();
    db->reset();
}

template<> template<> void to::test<4>() { use_db(db::V5); test_repinfo(); }
template<> template<> void to::test<5>() { use_db(db::V6); test_repinfo(); }
template<> template<> void to::test<6>() { use_db(db::MEM); test_repinfo(); }
void db_basic_shar::test_repinfo()
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

}

/* vim:set ts=4 sw=4: */


