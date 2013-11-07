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

struct db_tests_query : public DB_test_base
{
};

}

namespace tut {

using namespace dballe::tests;
typedef db_tg<db_tests_query> tg;
typedef tg::object to;

#define TRY_QUERY(qstring, expected_count) wassert(actual(db).try_data_query(qstring, expected_count))

template<> template<> void to::test<1>()
{
    wruntest(populate_database);
    TRY_QUERY("ana_id=1", 4);
    TRY_QUERY("ana_id=2", 0);
}

template<> template<> void to::test<2>()
{
    // Query data in station context
    wruntest(populate_database);
    query.clear();
    query.set_ana_context();
    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 5);
}

template<> template<> void to::test<3>()
{
    // Datetime queries
    wruntest(populate_database);
    TRY_QUERY("year=1001", 0);
    TRY_QUERY("yearmin=1999", 0);
    TRY_QUERY("yearmin=1945", 4);
    TRY_QUERY("yearmax=1944", 0);
    TRY_QUERY("yearmax=1945", 4);
    TRY_QUERY("yearmax=2030", 4);
    TRY_QUERY("year=1944", 0);
    TRY_QUERY("year=1945", 4);
    TRY_QUERY("year=1946", 0);
    /*
    TRY_QUERY(i, DBA_KEY_MONTHMIN, 1);
    TRY_QUERY(i, DBA_KEY_MONTHMAX, 12);
    TRY_QUERY(i, DBA_KEY_MONTH, 5);
    */
    /*
    TRY_QUERY(i, DBA_KEY_DAYMIN, 1);
    TRY_QUERY(i, DBA_KEY_DAYMAX, 12);
    TRY_QUERY(i, DBA_KEY_DAY, 5);
    */
    /*
    TRY_QUERY(i, DBA_KEY_HOURMIN, 1);
    TRY_QUERY(i, DBA_KEY_HOURMAX, 12);
    TRY_QUERY(i, DBA_KEY_HOUR, 5);
    */
    /*
    TRY_QUERY(i, DBA_KEY_MINUMIN, 1);
    TRY_QUERY(i, DBA_KEY_MINUMAX, 12);
    TRY_QUERY(i, DBA_KEY_MIN, 5);
    */
    /*
    TRY_QUERY(i, DBA_KEY_SECMIN, 1);
    TRY_QUERY(i, DBA_KEY_SECMAX, 12);
    TRY_QUERY(i, DBA_KEY_SEC, 5);
    */
}

template<> template<> void to::test<4>()
{
    // Block and station queries
    wruntest(populate_database);
    TRY_QUERY("B01001=1", 4);
    TRY_QUERY("B01001=2", 0);
    TRY_QUERY("B01002=52", 4);
    TRY_QUERY("B01002=53", 0);
}

template<> template<> void to::test<5>()
{
    // ana_filter queries
    wruntest(populate_database);
    TRY_QUERY("ana_filter=block=1", 4);
    TRY_QUERY("ana_filter=B01001=1", 4);
    TRY_QUERY("ana_filter=block>1", 0);
    TRY_QUERY("ana_filter=B01001>1", 0);
    TRY_QUERY("ana_filter=block<=1", 4);
    TRY_QUERY("ana_filter=B01001<=1", 4);
    TRY_QUERY("ana_filter=0<=B01001<=2", 4);
    TRY_QUERY("ana_filter=1<=B01001<=1", 4);
    TRY_QUERY("ana_filter=2<=B01001<=4", 0);
}

template<> template<> void to::test<6>()
{
    // data_filter queries
    wruntest(populate_database);
    TRY_QUERY("data_filter=B01011=DB-All.e!", 1);
    TRY_QUERY("data_filter=B01012<300", 0);
    TRY_QUERY("data_filter=B01012<=300", 1);
    TRY_QUERY("data_filter=B01012=300", 1);
    TRY_QUERY("data_filter=B01012>=300", 2);
    TRY_QUERY("data_filter=B01012>300", 1);
    TRY_QUERY("data_filter=B01012<400", 1);
    TRY_QUERY("data_filter=B01012<=400", 2);
}

template<> template<> void to::test<7>()
{
    // latitude/longitude queries
    wruntest(populate_database);
    TRY_QUERY("latmin=11.0", 4);
    TRY_QUERY("latmin=12.34560", 4);
    TRY_QUERY("latmin=13.0", 0);
    TRY_QUERY("latmax=11.0", 0);
    TRY_QUERY("latmax=12.34560", 4);
    TRY_QUERY("latmax=13.0", 4);
    TRY_QUERY("latmin=0, latmax=20", 4);
    TRY_QUERY("latmin=-90, latmax=20", 4);
    TRY_QUERY("latmin=-90, latmax=0", 0);
    TRY_QUERY("latmin=10, latmax=90", 4);
    TRY_QUERY("latmin=45, latmax=90", 0);
    TRY_QUERY("latmin=-90, latmax=90", 4);
    TRY_QUERY("lonmin=75, lonmax=77", 4);
    TRY_QUERY("lonmin=76.54320, lonmax=76.54320", 4);
    TRY_QUERY("lonmin=76.54330, lonmax=77.", 0);
    TRY_QUERY("lonmin=77., lonmax=76.54330", 4);
    TRY_QUERY("lonmin=77., lonmax=76.54320", 4);
    TRY_QUERY("lonmin=77., lonmax=-10", 0);
    TRY_QUERY("lonmin=0., lonmax=360.", 4);
    TRY_QUERY("lonmin=-180., lonmax=180.", 4);
}

template<> template<> void to::test<8>()
{
    // fixed/mobile queries
    wruntest(populate_database);
    TRY_QUERY("mobile=0", 4);
    TRY_QUERY("mobile=1", 0);
}

template<> template<> void to::test<9>()
{
    // ident queries
    wruntest(populate_database);
    // FIXME: we currently have no mobile station data in the samples
    //TRY_QUERY(c, DBA_KEY_IDENT_SELECT, "pippo");
}

template<> template<> void to::test<10>()
{
    // timerange queries
    wruntest(populate_database);
    TRY_QUERY("pindicator=20", 4);
    TRY_QUERY("pindicator=21", 0);
    TRY_QUERY("p1=111", 4);
    TRY_QUERY("p1=112", 0);
    TRY_QUERY("p2=121", 0);
    TRY_QUERY("p2=122", 2);
    TRY_QUERY("p2=123", 2);
}

template<> template<> void to::test<11>()
{
    // level queries
    wruntest(populate_database);
    TRY_QUERY("leveltype1=10", 4);
    TRY_QUERY("leveltype1=11", 0);
    TRY_QUERY("leveltype2=15", 4);
    TRY_QUERY("leveltype2=16", 0);
    TRY_QUERY("l1=11", 4);
    TRY_QUERY("l1=12", 0);
    TRY_QUERY("l2=22", 4);
    TRY_QUERY("l2=23", 0);
}

template<> template<> void to::test<12>()
{
    // varcode queries
    wruntest(populate_database);
    TRY_QUERY("var=B01011", 2);
    TRY_QUERY("var=B01012", 2);
    TRY_QUERY("var=B01013", 0);
}

template<> template<> void to::test<13>()
{
    // report queries
    wruntest(populate_database);
    TRY_QUERY("rep_memo=synop", 2);
    TRY_QUERY("rep_memo=metar", 2);
    TRY_QUERY("rep_memo=temp", 0);
}

template<> template<> void to::test<14>()
{
    // report priority queries
    wruntest(populate_database);
    TRY_QUERY("priority=101", 2);
    TRY_QUERY("priority=81", 2);
    TRY_QUERY("priority=102", 0);
    TRY_QUERY("priomin=70", 4);
    TRY_QUERY("priomin=80", 4);
    TRY_QUERY("priomin=90", 2);
    TRY_QUERY("priomin=100", 2);
    TRY_QUERY("priomin=110", 0);
    TRY_QUERY("priomax=70", 0);
    TRY_QUERY("priomax=81", 2);
    TRY_QUERY("priomax=100", 2);
    TRY_QUERY("priomax=101", 4);
    TRY_QUERY("priomax=110", 4);
}

template<> template<> void to::test<15>()
{
    // context ID queries
    wruntest(populate_database);
    TRY_QUERY("context_id=1", 1);
    TRY_QUERY("context_id=11", 0);
}

}

namespace {

tut::tg db_tests_query_mem_tg("db_data_query_mem", MEM);
#ifdef HAVE_ODBC
tut::tg db_tests_query_v5_tg("db_data_query_v5", V5);
tut::tg db_tests_query_v6_tg("db_data_query_v6", V6);
#endif

}

