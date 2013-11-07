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

    db_tests_query()
    {
        st1.lat = 12.34560;
        st1.lon = 76.54320;
        st1.info["synop"].set("block", 1);
        st1.info["synop"].set("station", 2);
        st1.info["synop"].set("B07030", 42.0); // height
        st1.info["metar"].set("B07030", 50.0); // height

        st2.lat = 23.45670;
        st2.lon = 65.43210;
        st2.info["temp"].set("block", 3);
        st2.info["temp"].set("station", 4);
        st2.info["temp"].set("B07030", 100.0); // height
        st2.info["metar"].set("B07030", 110.0); // height

        dballe::tests::TestRecord rec1;
        rec1.station = st1;
        rec1.data.set(DBA_KEY_REP_MEMO, "metar");
        rec1.data.set_datetime(1945, 4, 25, 8);
        rec1.data.set(Level(10, 11, 15, 22));
        rec1.data.set(Trange(20, 111, 122));
        rec1.data.set(WR_VAR(0, 12, 101), 290.0);
        rec1.data.set(WR_VAR(0, 12, 103), 280.0);
        wruntest(rec1.insert, *db, true);

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

        rec2.data.clear_vars();
        rec2.data.set_datetime(1945, 4, 26, 8);
        rec2.data.set(WR_VAR(0, 12, 101), 301.0);
        rec2.data.set(WR_VAR(0, 12, 103), 291.0);
        wruntest(rec2.insert, *db, true);
    }
};

}

namespace tut {

using namespace dballe::tests;
typedef db_tg<db_tests_query> tg;
typedef tg::object to;

template<> template<> void to::test<1>()
{
    wassert(actual(db).try_summary_query("ana_id=1", 2));
    wassert(actual(db).try_summary_query("ana_id=2", 2));
    wassert(actual(db).try_summary_query("ana_id=3", 0));
}

template<> template<> void to::test<2>()
{
    Record query;
    query.set_ana_context();
    auto_ptr<db::Cursor> cur = db->query_summary(query);
    ensure_equals(cur->test_iterate(), 8);
}

template<> template<> void to::test<3>()
{
    wassert(actual(db).try_summary_query("year=1001", 0));
    wassert(actual(db).try_summary_query("yearmin=1999", 0));
    wassert(actual(db).try_summary_query("yearmin=1945", 4));
    wassert(actual(db).try_summary_query("yearmax=1944", 0));
    wassert(actual(db).try_summary_query("yearmax=1945", 4));
    wassert(actual(db).try_summary_query("yearmax=2030", 4));
    wassert(actual(db).try_summary_query("year=1944", 0));
    wassert(actual(db).try_summary_query("year=1945", 4));
    wassert(actual(db).try_summary_query("year=1946", 0));
}

template<> template<> void to::test<4>()
{
    wassert(actual(db).try_summary_query("B01001=1", 2));
    wassert(actual(db).try_summary_query("B01001=2", 0));
    wassert(actual(db).try_summary_query("B01002=3", 0));
    wassert(actual(db).try_summary_query("B01002=4", 2));
}

template<> template<> void to::test<5>()
{
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
}

template<> template<> void to::test<6>()
{
    wassert(actual(db).try_summary_query("data_filter=B12101<300.0", 1));
    wassert(actual(db).try_summary_query("data_filter=B12101<=300.0", 2));
    wassert(actual(db).try_summary_query("data_filter=B12101=300.0", 1));
    wassert(actual(db).try_summary_query("data_filter=B12101>=300,0", 1));
    wassert(actual(db).try_summary_query("data_filter=B12101>300.0", 1));
    wassert(actual(db).try_summary_query("data_filter=B12101<400.0", 2));
    wassert(actual(db).try_summary_query("data_filter=B12101<=400.0", 2));
    wassert(actual(db).try_summary_query("data_filter=B12102>400.0", 0));
}

template<> template<> void to::test<7>()
{
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
}

template<> template<> void to::test<8>()
{
    wassert(actual(db).try_summary_query("mobile=0", 4));
    wassert(actual(db).try_summary_query("mobile=1", 0));
}

template<> template<> void to::test<9>()
{
    // TODO: add mobile stations to the fixture so we can query ident
}

template<> template<> void to::test<10>()
{
    wassert(actual(db).try_summary_query("pindicator=20", 4));
    wassert(actual(db).try_summary_query("pindicator=21", 0));
    wassert(actual(db).try_summary_query("p1=111", 4));
    wassert(actual(db).try_summary_query("p1=112", 0));
    wassert(actual(db).try_summary_query("p2=121", 0));
    wassert(actual(db).try_summary_query("p2=122", 4));
    wassert(actual(db).try_summary_query("p2=123", 0));
}

template<> template<> void to::test<11>()
{
    wassert(actual(db).try_summary_query("leveltype1=10", 4));
    wassert(actual(db).try_summary_query("leveltype1=11", 0));
    wassert(actual(db).try_summary_query("leveltype2=15", 4));
    wassert(actual(db).try_summary_query("leveltype2=16", 0));
    wassert(actual(db).try_summary_query("l1=11", 4));
    wassert(actual(db).try_summary_query("l1=12", 0));
    wassert(actual(db).try_summary_query("l2=22", 4));
    wassert(actual(db).try_summary_query("l2=23", 0));
}

template<> template<> void to::test<12>()
{
    wassert(actual(db).try_summary_query("var=B01001", 0));
    wassert(actual(db).try_summary_query("var=B12101", 2));
    wassert(actual(db).try_summary_query("var=B12102", 0));
}

template<> template<> void to::test<13>()
{
    wassert(actual(db).try_summary_query("rep_memo=synop", 0));
    wassert(actual(db).try_summary_query("rep_memo=metar", 4));
    wassert(actual(db).try_summary_query("rep_memo=temp", 0));
}

template<> template<> void to::test<14>()
{
    wassert(actual(db).try_summary_query("priority=101", 0));
    wassert(actual(db).try_summary_query("priority=81", 4));
    wassert(actual(db).try_summary_query("priority=102", 0));
    wassert(actual(db).try_summary_query("priomin=70", 4));
    wassert(actual(db).try_summary_query("priomin=80", 4));
    wassert(actual(db).try_summary_query("priomin=90", 0));
    wassert(actual(db).try_summary_query("priomax=70", 0));
    wassert(actual(db).try_summary_query("priomax=81", 4));
    wassert(actual(db).try_summary_query("priomax=100", 4));
}

template<> template<> void to::test<15>()
{
    wassert(actual(db).try_summary_query("context_id=1", 1));
    wassert(actual(db).try_summary_query("context_id=11", 1));
}

}

namespace {

tut::tg db_tests_query_mem_tg("db_summary_query_mem", MEM);
#ifdef HAVE_ODBC
// tut::tg db_tests_query_v5_tg("db_summary_query_v5", V5); Summary queries are not implemented for V5 databases
tut::tg db_tests_query_v6_tg("db_summary_query_v6", V6);
#endif

}

