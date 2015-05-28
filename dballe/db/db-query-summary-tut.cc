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
#include <wibble/string.h>

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace {

struct Fixture : public dballe::tests::DBFixture
{
    dballe::tests::TestStation st1;
    dballe::tests::TestStation st2;
    int st1_id;
    int st2_id;

    Fixture()
    {
        st1.lat = 12.34560;
        st1.lon = 76.54320;
        st1.info["synop"].set("B07030", 42.0); // height
        st1.info["metar"].set("B07030", 50.0); // height
        st1.info["metar"].set("block", 1);
        st1.info["metar"].set("station", 2);

        st2.lat = 23.45670;
        st2.lon = 65.43210;
        st2.info["temp"].set("B07030", 100.0); // height
        st2.info["metar"].set("B07030", 110.0); // height
        st2.info["metar"].set("block", 3);
        st2.info["metar"].set("station", 4);

        insert_data();
    }

    void insert_data()
    {

        dballe::tests::TestRecord rec1;
        rec1.station = st1;
        rec1.data.set(DBA_KEY_REP_MEMO, "metar");
        rec1.data.set_datetime(1945, 4, 25, 8);
        rec1.data.set(Level(10, 11, 15, 22));
        rec1.data.set(Trange(20, 111, 122));
        rec1.data.set(WR_VAR(0, 12, 101), 290.0);
        rec1.data.set(WR_VAR(0, 12, 103), 280.0);
        wruntest(rec1.insert, *db, true);
        st1_id = db->last_station_id();

        rec1.data.clear_vars();
        rec1.data.set_datetime(1945, 4, 26, 8);
        rec1.data.set(WR_VAR(0, 12, 101), 291.0);
        rec1.data.set(WR_VAR(0, 12, 103), 281.0);
        wruntest(rec1.insert, *db, true);

        dballe::tests::TestRecord rec2;
        rec2.station = st2;
        rec2.data.set(DBA_KEY_REP_MEMO, "metar");
        rec2.data.set_datetime(1945, 4, 25, 8);
        rec2.data.set(Level(10, 11, 15, 22));
        rec2.data.set(Trange(20, 111, 122));
        rec2.data.set(WR_VAR(0, 12, 101), 300.0);
        rec2.data.set(WR_VAR(0, 12, 103), 290.0);
        wruntest(rec2.insert, *db, true);
        st2_id = db->last_station_id();

        rec2.data.clear_vars();
        rec2.data.set_datetime(1945, 4, 26, 8);
        rec2.data.set(WR_VAR(0, 12, 101), 301.0);
        rec2.data.set(WR_VAR(0, 12, 103), 291.0);
        wruntest(rec2.insert, *db, true);
    }

    void reset()
    {
        dballe::tests::DBFixture::reset();
        insert_data();
    }
};

typedef dballe::tests::db_test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    Test("query_ana_id", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query(str::fmtf("ana_id=%d", f.st1_id), 2));
        wassert(actual(db).try_summary_query(str::fmtf("ana_id=%d", f.st2_id), 2));
        wassert(actual(db).try_summary_query(str::fmtf("ana_id=%d", (f.st1_id + f.st2_id) * 2), 0));
    }),
    Test("query_station_vars", [](Fixture& f) {
        auto& db = *f.db;
        core::Query query;
        query.query_station_vars = true;
        unique_ptr<db::Cursor> cur = db.query_summary(query);
        ensure_equals(cur->test_iterate(), 8);
    }),
    Test("query_year", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("year=1001", 0));
        wassert(actual(db).try_summary_query("yearmin=1999", 0));
        auto check_base = [](WIBBLE_TEST_LOCPRM, const vector<Record>& res) {
            wassert(actual(res[0][DBA_KEY_LAT].enqi()) == 1234560);
            wassert(actual(res[0][DBA_KEY_LON].enqi()) == 7654320);
            wassert(actual(res[0].get_level()) == Level(10, 11, 15, 22));
            wassert(actual(res[0].get_trange()) == Trange(20, 111, 122));
            wassert(actual(res[0][DBA_KEY_VAR].enqc()) == "B12101");
        };
        auto check_nodetails = [&](WIBBLE_TEST_LOCPRM, const vector<Record>& res) {
            wruntest(check_base, res);
            wassert(actual(res[0].contains(DBA_KEY_CONTEXT_ID)).isfalse());
            wassert(actual(res[0].contains(DBA_KEY_YEARMIN)).isfalse());
            wassert(actual(res[0].contains(DBA_KEY_YEARMAX)).isfalse());
        };
        auto check_details = [&](WIBBLE_TEST_LOCPRM, const vector<Record>& res) {
            wruntest(check_base, res);
            wassert(actual(res[0][DBA_KEY_CONTEXT_ID].enqi()) == 2);
            wassert(actual(res[0].get_datetimemin()) == Datetime(1945, 4, 25, 8));
            wassert(actual(res[0].get_datetimemax()) == Datetime(1945, 4, 26, 8));
        };
        wassert(actual(db).try_summary_query("yearmin=1945", 4, check_nodetails));
        wassert(actual(db).try_summary_query("yearmin=1945, query=details", 4, check_details));
        wassert(actual(db).try_summary_query("yearmax=1944", 0));
        wassert(actual(db).try_summary_query("yearmax=1945", 4));
        wassert(actual(db).try_summary_query("yearmax=2030", 4));
        wassert(actual(db).try_summary_query("year=1944", 0));
        wassert(actual(db).try_summary_query("year=1945", 4));
        wassert(actual(db).try_summary_query("year=1946", 0));
    }),
    Test("query_blockstation", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("B01001=1", 2));
        wassert(actual(db).try_summary_query("B01001=2", 0));
        wassert(actual(db).try_summary_query("B01002=3", 0));
        wassert(actual(db).try_summary_query("B01002=4", 2));
    }),
    Test("query_ana_filter", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("ana_filter=block=1", 2));
        wassert(actual(db).try_summary_query("ana_filter=B01001=1", 2));
        wassert(actual(db).try_summary_query("ana_filter=block>1", 2));
        wassert(actual(db).try_summary_query("ana_filter=B01001>1", 2));
        wassert(actual(db).try_summary_query("ana_filter=block<=1", 2));
        wassert(actual(db).try_summary_query("ana_filter=B01001>3", 0));
        wassert(actual(db).try_summary_query("ana_filter=B01001>=3", 2));
        wassert(actual(db).try_summary_query("ana_filter=B01001<=1", 2));
        wassert(actual(db).try_summary_query("ana_filter=0<=B01001<=2", 2));
        wassert(actual(db).try_summary_query("ana_filter=1<=B01001<=1", 2));
        wassert(actual(db).try_summary_query("ana_filter=2<=B01001<=4", 2));
        wassert(actual(db).try_summary_query("ana_filter=4<=B01001<=6", 0));
    }),
    Test("query_data_filter", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("data_filter=B12101<300.0", 1));
        wassert(actual(db).try_summary_query("data_filter=B12101<=300.0", 2));
        wassert(actual(db).try_summary_query("data_filter=B12101=300.0", 1));
        wassert(actual(db).try_summary_query("data_filter=B12101>=300,0", 1));
        wassert(actual(db).try_summary_query("data_filter=B12101>300.0", 1));
        wassert(actual(db).try_summary_query("data_filter=B12101<400.0", 2));
        wassert(actual(db).try_summary_query("data_filter=B12101<=400.0", 2));
        wassert(actual(db).try_summary_query("data_filter=B12102>400.0", 0));
    }),
    Test("query_lat_lon", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("latmin=11.0", 4));
        wassert(actual(db).try_summary_query("latmin=12.34560", 4));
        wassert(actual(db).try_summary_query("latmin=13.0", 2));
        wassert(actual(db).try_summary_query("latmax=11.0", 0));
        wassert(actual(db).try_summary_query("latmax=12.34560", 2));
        wassert(actual(db).try_summary_query("latmax=13.0", 2));
        wassert(actual(db).try_summary_query("lonmin=75., lonmax=77.", 2));
        wassert(actual(db).try_summary_query("lonmin=76.54320, lonmax=76.54320", 2));
        wassert(actual(db).try_summary_query("lonmin=76.54330, lonmax=77.", 0));
        wassert(actual(db).try_summary_query("lonmin=77., lonmax=76.54310", 2));
        wassert(actual(db).try_summary_query("lonmin=77., lonmax=76.54320", 4));
        wassert(actual(db).try_summary_query("lonmin=77., lonmax=-10", 0));
    }),
    Test("query_mobile", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("mobile=0", 4));
        wassert(actual(db).try_summary_query("mobile=1", 0));
    }),
    Test("query_ident", [](Fixture& f) {
        //auto& db = *f.db;
        // TODO: add mobile stations to the fixture so we can query ident
    }),
    Test("query_timerange", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("pindicator=20", 4));
        wassert(actual(db).try_summary_query("pindicator=21", 0));
        wassert(actual(db).try_summary_query("p1=111", 4));
        wassert(actual(db).try_summary_query("p1=112", 0));
        wassert(actual(db).try_summary_query("p2=121", 0));
        wassert(actual(db).try_summary_query("p2=122", 4));
        wassert(actual(db).try_summary_query("p2=123", 0));
    }),
    Test("query_level", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("leveltype1=10", 4));
        wassert(actual(db).try_summary_query("leveltype1=11", 0));
        wassert(actual(db).try_summary_query("leveltype2=15", 4));
        wassert(actual(db).try_summary_query("leveltype2=16", 0));
        wassert(actual(db).try_summary_query("l1=11", 4));
        wassert(actual(db).try_summary_query("l1=12", 0));
        wassert(actual(db).try_summary_query("l2=22", 4));
        wassert(actual(db).try_summary_query("l2=23", 0));
    }),
    Test("query_var", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("var=B01001", 0));
        wassert(actual(db).try_summary_query("var=B12101", 2));
        wassert(actual(db).try_summary_query("var=B12102", 0));
    }),
    Test("query_rep_memo", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("rep_memo=synop", 0));
        wassert(actual(db).try_summary_query("rep_memo=metar", 4));
        wassert(actual(db).try_summary_query("rep_memo=temp", 0));
    }),
    Test("query_priority", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("priority=101", 0));
        wassert(actual(db).try_summary_query("priority=81", 4));
        wassert(actual(db).try_summary_query("priority=102", 0));
        wassert(actual(db).try_summary_query("priomin=70", 4));
        wassert(actual(db).try_summary_query("priomin=80", 4));
        wassert(actual(db).try_summary_query("priomin=90", 0));
        wassert(actual(db).try_summary_query("priomax=70", 0));
        wassert(actual(db).try_summary_query("priomax=81", 4));
        wassert(actual(db).try_summary_query("priomax=100", 4));
    }),
    Test("query_context_id", [](Fixture& f) {
        auto& db = *f.db;
        // Collect a vector of valid context IDs
        vector<int> context_ids;
        // And an invalid one
        int sum = 1;
        unique_ptr<db::Cursor> cur = db.query_data(core::Query());
        while (cur->next())
        {
            context_ids.push_back(cur->attr_reference_id());
            sum += context_ids.back();
        }

        WIBBLE_TEST_INFO(info);

        wassert(actual(db).try_summary_query(str::fmtf("context_id=%d", sum), 0));
        for (vector<int>::const_iterator i = context_ids.begin(); i != context_ids.end(); ++i)
        {
            info() << "Context ID " << *i;
            wassert(actual(db).try_summary_query(str::fmtf("context_id=%d", *i), 1));
        }
    }),
};

test_group tg1("db_query_summary_mem", nullptr, db::MEM, tests);
test_group tg2("db_query_summary_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("db_query_summary_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_query_summary_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_query_summary_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
