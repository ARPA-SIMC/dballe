/*
 * Copyright (C) 2013--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "db/mem/db.h"

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
    std::unique_ptr<db::Cursor> cur = db->query_data(query);
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

// Test adding station data for different networks
template<> template<> void to::test<5>()
{
    db->reset();

    Record rec;

    rec.set(DBA_KEY_LAT, 12.077);
    rec.set(DBA_KEY_LON, 44.600);

    // Insert two values in two networks
    rec.set(Level(103, 2000));
    rec.set(Trange::instant());
    rec.set(Datetime(2014, 1, 1, 0, 0, 0));
    rec.set(DBA_KEY_REP_MEMO, "synop");
    rec.set(WR_VAR(0, 12, 101), 273.15);
    db->insert(rec, true, true);
    rec.set(DBA_KEY_REP_MEMO, "temp");
    rec.set(WR_VAR(0, 12, 101), 274.15);
    db->insert(rec, true, true);

    // Insert station names in both networks
    rec.set_ana_context();
    rec.set(DBA_KEY_REP_MEMO, "synop");
    rec.set(WR_VAR(0, 1, 19), "Camse");
    db->insert(rec, true, true);
    rec.set(DBA_KEY_REP_MEMO, "temp");
    rec.set(WR_VAR(0, 1, 19), "Esmac");
    db->insert(rec, true, true);

    // Query back all the data
    Query query;
    auto cur = db->query_stations(query);

    // Check results
    Record result;
    if (dynamic_cast<db::mem::DB*>(db.get()))
    {
        // For mem databases, we get one record per (station, network)
        // combination
        wassert(actual(cur->next()).istrue());
        wassert(actual(cur->get_station_id()) == 1);
        wassert(actual(cur->get_rep_memo()) == "temp");
        cur->to_record(result);
        wassert(actual(result.get(WR_VAR(0, 1, 19)).value()) == "Esmac");

        wassert(actual(cur->next()).istrue());
        wassert(actual(cur->get_station_id()) == 0);
        wassert(actual(cur->get_rep_memo()) == "synop");
        cur->to_record(result);
        wassert(actual(result.get(WR_VAR(0, 1, 19)).value()) == "Camse");
    } else {
        // For normal databases, we only get one record, with the station
        // values merged keeping values for the best networks
        wassert(actual(cur->next()).istrue());
        wassert(actual(cur->get_station_id()) == 1);
        cur->to_record(result);
        wassert(actual(result.get(WR_VAR(0, 1, 19)).value()) == "Camse");

        wassert(actual(cur->next()).isfalse());
    }

    query.clear();
    Msgs msgs;
    msg::AcquireMessages amsg(msgs);
    db->export_msgs(query, amsg);
    wassert(actual(msgs.size()) == 2);

    //msgs.print(stderr);

    wassert(actual(msgs[0]->get_rep_memo_var()->enqc()) == "synop");
    wassert(actual(msgs[0]->get_st_name_var()->enqc()) == "Camse");
    wassert(actual(msgs[0]->get_temp_2m_var()->enqd()) == 273.15);
    wassert(actual(msgs[1]->get_rep_memo_var()->enqc()) == "temp");
    wassert(actual(msgs[1]->get_st_name_var()->enqc()) == "Esmac");
    wassert(actual(msgs[1]->get_temp_2m_var()->enqd()) == 274.15);
}

}

namespace {

tut::tg db_tests_query_mem_tg("db_basic_mem", MEM);
#ifdef HAVE_ODBC
tut::tg db_tests_query_v5_tg("db_basic_v5", V5);
tut::tg db_tests_query_v6_tg("db_basic_v6", V6);
#endif

}
