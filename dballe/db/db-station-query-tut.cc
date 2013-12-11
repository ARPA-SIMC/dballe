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

struct db_tests_query : public db_test
{
    dballe::tests::TestStation st1;
    dballe::tests::TestStation st2;

    const int some;
    const int all;

    db_tests_query()
        : some(db->format() == MEM ? 2 : 1), all(db->format() == MEM ? 4 : 2)
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
    {
        st1.lat = 12.34560;
        st1.lon = 76.54320;
        st1.info["synop"].set("block", 1);
        st1.info["synop"].set("station", 2);
        st1.info["synop"].set("B07030", 42.0); // height
        st1.info["metar"].set("block", 1);
        st1.info["metar"].set("station", 2);
        st1.info["metar"].set("B07030", 50.0); // height

        st2.lat = 23.45670;
        st2.lon = 65.43210;
        st2.info["temp"].set("block", 3);
        st2.info["temp"].set("station", 4);
        st2.info["temp"].set("B07030", 100.0); // height
        st2.info["metar"].set("block", 3);
        st2.info["metar"].set("station", 4);
        st2.info["metar"].set("B07030", 110.0); // height

        dballe::tests::TestRecord rec1;
        rec1.station = st1;
        rec1.data.set(DBA_KEY_REP_MEMO, "metar");
        rec1.data.set_datetime(1945, 4, 25, 8);
        rec1.data.set(Level(10, 11, 15, 22));
        rec1.data.set(Trange(20, 111, 122));
        rec1.data.set(WR_VAR(0, 12, 101), 290.0);

        dballe::tests::TestRecord rec2;
        rec2.station = st2;
        rec2.data.set(DBA_KEY_REP_MEMO, "metar");
        rec2.data.set_datetime(1945, 4, 25, 8);
        rec2.data.set(Level(10, 11, 15, 22));
        rec2.data.set(Trange(20, 111, 122));
        rec2.data.set(WR_VAR(0, 12, 101), 300.0);
        rec2.data.set(WR_VAR(0, 12, 103), 298.0);

        wruntest(rec1.insert, *db, true);
        wruntest(rec2.insert, *db, true);
    }
};

}

namespace tut {

using namespace dballe::tests;
typedef db_tg<db_tests_query> tg;
typedef tg::object to;

#define TRY_QUERY(qstring, expected_count) wassert(actual(db).try_data_query(qstring, expected_count))

template<> template<> void to::test<1>()
{
    wassert(actual(db).try_station_query("ana_id=1", 1));
    wassert(actual(db).try_station_query("ana_id=2", 1));
}

template<> template<> void to::test<2>()
{
    wassert(actual(db).try_station_query("lat=12.00000", 0));
    wassert(actual(db).try_station_query("lat=12.34560", some));
    wassert(actual(db).try_station_query("lat=23.45670", some));
    wassert(actual(db).try_station_query("latmin=12.00000", all));
    wassert(actual(db).try_station_query("latmin=12.34560", all));
    wassert(actual(db).try_station_query("latmin=12.34570", some));
    wassert(actual(db).try_station_query("latmin=23.45670", some));
    wassert(actual(db).try_station_query("latmin=23.45680", 0));
    wassert(actual(db).try_station_query("latmax=12.00000", 0));
    wassert(actual(db).try_station_query("latmax=12.34560", some));
    wassert(actual(db).try_station_query("latmax=12.34570", some));
    wassert(actual(db).try_station_query("latmax=23.45670", all));
    wassert(actual(db).try_station_query("latmax=23.45680", all));
    wassert(actual(db).try_station_query("lon=76.00000", 0));
    wassert(actual(db).try_station_query("lon=76.54320", some));
    wassert(actual(db).try_station_query("lon=65.43210", some));
    wassert(actual(db).try_station_query("lonmin=10., lonmax=20.", 0));
    wassert(actual(db).try_station_query("lonmin=76.54320, lonmax=76.54320", some));
    wassert(actual(db).try_station_query("lonmin=76.54320, lonmax=77.", some));
    wassert(actual(db).try_station_query("lonmin=76.54330, lonmax=77.", 0));
    wassert(actual(db).try_station_query("lonmin=60., lonmax=77.", all));
    wassert(actual(db).try_station_query("lonmin=77., lonmax=76.54310", some));
    wassert(actual(db).try_station_query("lonmin=77., lonmax=76.54320", all));
    wassert(actual(db).try_station_query("lonmin=77., lonmax=-10", 0));
}

template<> template<> void to::test<3>()
{
    wassert(actual(db).try_station_query("mobile=0", all));
    wassert(actual(db).try_station_query("mobile=1", 0));
}

template<> template<> void to::test<4>()
{
    // FIXME: add some mobile stations to the test fixture to test ident
}

template<> template<> void to::test<5>()
{
    wassert(actual(db).try_station_query("B01001=1", 1));
    wassert(actual(db).try_station_query("B01001=2", 0));
    wassert(actual(db).try_station_query("B01001=3", 1));
    wassert(actual(db).try_station_query("B01001=4", 0));
    wassert(actual(db).try_station_query("B01002=1", 0));
    wassert(actual(db).try_station_query("B01002=2", 1));
    wassert(actual(db).try_station_query("B01002=3", 0));
    wassert(actual(db).try_station_query("B01002=4", 1));
}

template<> template<> void to::test<6>()
{
    wassert(actual(db).try_station_query("ana_filter=block=1", some));
    wassert(actual(db).try_station_query("ana_filter=block=2", 0));
    wassert(actual(db).try_station_query("ana_filter=block=3", some));
    wassert(actual(db).try_station_query("ana_filter=block>=1", all));
    wassert(actual(db).try_station_query("ana_filter=B07030=42", 1));
    wassert(actual(db).try_station_query("ana_filter=B07030=50", 1));
    wassert(actual(db).try_station_query("ana_filter=B07030=100", 1));
    wassert(actual(db).try_station_query("ana_filter=B07030=110", 1));
    wassert(actual(db).try_station_query("ana_filter=B07030=120", 0));
    wassert(actual(db).try_station_query("ana_filter=B07030>50", some));
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
    if (db->format() == MEM)
        wassert(actual(db).try_station_query("ana_filter=B07030>=50", 3));
    else
        wassert(actual(db).try_station_query("ana_filter=B07030>=50", 2));
    wassert(actual(db).try_station_query("ana_filter=50<=B07030<=100", 2));
}

template<> template<> void to::test<7>()
{
    wassert(actual(db).try_station_query("var=B12101", 2));
    wassert(actual(db).try_station_query("var=B12103", 1));
    wassert(actual(db).try_station_query("varlist=B12101", all));
    wassert(actual(db).try_station_query("varlist=B12103", some));
    wassert(actual(db).try_station_query("varlist=B12101,B12103", all));
}

}

namespace {

tut::tg db_tests_query_mem_tg("db_station_query_mem", MEM);
#ifdef HAVE_ODBC
tut::tg db_tests_query_v5_tg("db_station_query_v5", V5);
tut::tg db_tests_query_v6_tg("db_station_query_v6", V6);
#endif

}

