/*
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "db/querybuf.h"
#include <wibble/string.h>

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct db1_shar : public dballe::tests::DB_test_base
{
    void test_invalid_sql_querybest();
    void test_bug_querybest();
    void test_bug_query_stations_by_level();
    void test_bug_query_levels_by_station();
    void test_connect_leaks();
    void test_query_stations();
    void test_summary_queries();
    void test_value_update();
    void test_query_step_by_step();
    void test_double_stationinfo_insert();
    void test_double_stationinfo_insert1();
    void test_insert_undef_lev2();
    void test_query_undef_lev2();
    void test_query_bad_attr_filter();
    void test_querybest_priomax();
    void test_repmemo_in_output();

    dballe::tests::TestStation st1;
    dballe::tests::TestStation st2;

    db1_shar() : dballe::tests::DB_test_base()
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
    }
};
TESTGRP(db1);

template<> template<> void to::test<1>() { use_db(V5); test_invalid_sql_querybest(); }
template<> template<> void to::test<2>() { use_db(V6); test_invalid_sql_querybest(); }
template<> template<> void to::test<3>() { use_db(MEM); test_invalid_sql_querybest(); }
void db1_shar::test_invalid_sql_querybest()
{
// Reproduce a querybest scenario which produced invalid SQL
    wruntest(populate_database);
    // SELECT pa.lat, pa.lon, pa.ident,
    //        d.datetime, d.id_report, d.id_var, d.value,
    //        ri.prio, pa.id, d.id, d.id_lev_tr
    //   FROM data d
    //   JOIN station pa ON d.id_station = pa.id
    //   JOIN repinfo ri ON ri.id=d.id_report
    //  WHERE pa.lat>=? AND pa.lat<=? AND pa.lon>=? AND pa.lon<=? AND pa.ident IS NULL
    //    AND d.datetime=?
    //    AND d.id_var IN (1822)
    //    AND ri.prio=(SELECT MAX(sri.prio) FROM repinfo sri JOIN data sd ON sri.id=sd.id_report WHERE sd.id_station=d.id_station AND sd.id_lev_tr=d.id_lev_tr AND sd.datetime=d.datetime AND sd.id_var=d.id_var)
    //    AND d.id_lev_tr IS NULLORDER BY d.id_station, d.datetime
    Record rec;
    rec.set(DBA_KEY_YEAR,  1000);
    rec.set(DBA_KEY_MONTH,    1);
    rec.set(DBA_KEY_DAY,      1);
    rec.set(DBA_KEY_HOUR,     0);
    rec.set(DBA_KEY_MIN,     0);
    rec.set(DBA_KEY_QUERY,    "best");
    auto_ptr<db::Cursor> cur = db->query_data(rec);
    while (cur->next())
    {
    }
}

template<> template<> void to::test<4>() { use_db(V5); test_bug_querybest(); }
template<> template<> void to::test<5>() { use_db(V6); test_bug_querybest(); }
template<> template<> void to::test<6>() { use_db(MEM); test_bug_querybest(); }
void db1_shar::test_bug_querybest()
{
    // Reproduce a querybest scenario which produced always the same data record

    // Import lots
    const char** files = dballe::tests::bufr_files;
    for (int i = 0; files[i] != NULL; i++)
    {
        std::auto_ptr<Msgs> inmsgs = read_msgs(files[i], BUFR);
        Msg& msg = *(*inmsgs)[0];
        wrunchecked(db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS | DBA_IMPORT_OVERWRITE));
    }

    // Query all with best
    Record rec;
    rec.set(DBA_KEY_VAR,   "B12101");
    rec.set(DBA_KEY_QUERY, "best");
    auto_ptr<db::Cursor> cur = db->query_data(rec);
    unsigned orig_count = cur->remaining();
    unsigned count = 0;
    int id_data = 0;
    unsigned id_data_changes = 0;
    while (cur->next())
    {
        ++count;
        if (cur->attr_reference_id() != id_data)
        {
            id_data = cur->attr_reference_id();
            ++id_data_changes;
        }
    }

    ensure(count > 1);
    ensure_equals(id_data_changes, count);
    ensure_equals(count, orig_count);
}

template<> template<> void to::test<7>() { use_db(V5); test_bug_query_stations_by_level(); }
template<> template<> void to::test<8>() { use_db(V6); test_bug_query_stations_by_level(); }
template<> template<> void to::test<9>() { use_db(MEM); test_bug_query_stations_by_level(); }
void db1_shar::test_bug_query_stations_by_level()
{
    // Reproduce a query that generated invalid SQL on V6
    wruntest(populate_database);

    // All DB
    query.clear();
    query.set(DBA_KEY_LEVELTYPE1, 103);
    query.set(DBA_KEY_L1, 2000);
    db->query_stations(query);
}

template<> template<> void to::test<10>() { use_db(V5); test_bug_query_levels_by_station(); }
template<> template<> void to::test<11>() { use_db(V6); test_bug_query_levels_by_station(); }
template<> template<> void to::test<12>() { use_db(MEM); test_bug_query_levels_by_station(); }
void db1_shar::test_bug_query_levels_by_station()
{
    // Reproduce a query that generated invalid SQL on V6
    wruntest(populate_database);

    // All DB
    query.clear();
    query.set(DBA_KEY_ANA_ID, 1);
    //db->query_levels(query);
    //db->query_tranges(query);
#warning currently disabled
}

template<> template<> void to::test<13>() { use_db(V5); test_connect_leaks(); }
template<> template<> void to::test<14>() { use_db(V6); test_connect_leaks(); }
template<> template<> void to::test<15>() { use_db(MEM); test_connect_leaks(); }
void db1_shar::test_connect_leaks()
{
    insert.clear();
    insert.set_ana_context();
    insert.set(DBA_KEY_LAT, 12.34560);
    insert.set(DBA_KEY_LON, 76.54320);
    insert.set(DBA_KEY_MOBILE, 0);
    insert.set(DBA_KEY_REP_MEMO, "synop");
    insert.set(WR_VAR(0, 7, 30), 42.0); // Height

    // Assume a max open file limit of 1100
    for (unsigned i = 0; i < 1100; ++i)
    {
        std::auto_ptr<DB> db = DB::connect_test();
        wrunchecked(db->insert(insert, true, true));
    }
}

template<> template<> void to::test<16>() { use_db(V5); test_query_stations(); }
template<> template<> void to::test<17>() { use_db(V6); test_query_stations(); }
template<> template<> void to::test<18>() { use_db(MEM); test_query_stations(); }
void db1_shar::test_query_stations()
{
    // Start with an empty database
    db->reset();

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

    wruntest(rec1.insert, *db, true);
    wruntest(rec2.insert, *db, true);

    wassert(actual(db).try_station_query("ana_id=1", 1));
    wassert(actual(db).try_station_query("ana_id=2", 1));
    wassert(actual(db).try_station_query("lat=12.00000", 0));
    wassert(actual(db).try_station_query("lat=12.34560", 1));
    wassert(actual(db).try_station_query("lat=23.45670", 1));
    wassert(actual(db).try_station_query("latmin=12.00000", 2));
    wassert(actual(db).try_station_query("latmin=12.34560", 2));
    wassert(actual(db).try_station_query("latmin=12.34570", 1));
    wassert(actual(db).try_station_query("latmin=23.45670", 1));
    wassert(actual(db).try_station_query("latmin=23.45680", 0));
    wassert(actual(db).try_station_query("latmax=12.00000", 0));
    wassert(actual(db).try_station_query("latmax=12.34560", 1));
    wassert(actual(db).try_station_query("latmax=12.34570", 1));
    wassert(actual(db).try_station_query("latmax=23.45670", 2));
    wassert(actual(db).try_station_query("latmax=23.45680", 2));
    wassert(actual(db).try_station_query("lon=76.00000", 0));
    wassert(actual(db).try_station_query("lon=76.54320", 1));
    wassert(actual(db).try_station_query("lon=65.43210", 1));
    wassert(actual(db).try_station_query("lonmin=10., lonmax=20.", 0));
    wassert(actual(db).try_station_query("lonmin=76.54320, lonmax=76.54320", 1));
    wassert(actual(db).try_station_query("lonmin=76.54320, lonmax=77.", 1));
    wassert(actual(db).try_station_query("lonmin=76.54330, lonmax=77.", 0));
    wassert(actual(db).try_station_query("lonmin=60., lonmax=77.", 2));
    wassert(actual(db).try_station_query("lonmin=77., lonmax=76.54310", 1));
    wassert(actual(db).try_station_query("lonmin=77., lonmax=76.54320", 2));
    wassert(actual(db).try_station_query("lonmin=77., lonmax=-10", 0));
    wassert(actual(db).try_station_query("mobile=0", 2));
    wassert(actual(db).try_station_query("mobile=1", 0));
    wassert(actual(db).try_station_query("B01001=1", 1));
    wassert(actual(db).try_station_query("B01001=2", 0));
    wassert(actual(db).try_station_query("B01001=3", 1));
    wassert(actual(db).try_station_query("B01001=4", 0));
    wassert(actual(db).try_station_query("B01002=1", 0));
    wassert(actual(db).try_station_query("B01002=2", 1));
    wassert(actual(db).try_station_query("B01002=3", 0));
    wassert(actual(db).try_station_query("B01002=4", 1));
    wassert(actual(db).try_station_query("ana_filter=block=1", 1));
    wassert(actual(db).try_station_query("ana_filter=block=2", 0));
    wassert(actual(db).try_station_query("ana_filter=block=3", 1));
    wassert(actual(db).try_station_query("ana_filter=block>=1", 2));
    wassert(actual(db).try_station_query("ana_filter=B07030=42", 1));
    wassert(actual(db).try_station_query("ana_filter=B07030=50", 1));
    wassert(actual(db).try_station_query("ana_filter=B07030=100", 1));
    wassert(actual(db).try_station_query("ana_filter=B07030=110", 1));
    wassert(actual(db).try_station_query("ana_filter=B07030=120", 0));
    wassert(actual(db).try_station_query("ana_filter=B07030>50", 1));
    wassert(actual(db).try_station_query("ana_filter=B07030>=50", 2));
    wassert(actual(db).try_station_query("ana_filter=50<=B07030<=100", 2));
}

template<> template<> void to::test<19>() {
    use_db(V5);
    bool worked = false;
    try {
        test_summary_queries();
        worked = true;
    } catch (tut::failure& e) {
        wassert(actual(e.what()).contains("not implemented"));
    }
    wassert(actual(worked).isfalse());
}
template<> template<> void to::test<20>() { use_db(V6); test_summary_queries(); }
template<> template<> void to::test<21>() { use_db(MEM); test_summary_queries(); }
void db1_shar::test_summary_queries()
{
    // Start with an empty database
    db->reset();

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

    wassert(actual(db).try_summary_query("ana_id=1", 2));
    wassert(actual(db).try_summary_query("ana_id=2", 2));
    wassert(actual(db).try_summary_query("ana_id=3", 0));
    {
        query.clear();
        query.set_ana_context();
        auto_ptr<db::Cursor> cur = db->query_summary(query);
        ensure_equals(cur->test_iterate(), 8);
    }
    wassert(actual(db).try_summary_query("year=1001", 0));
    wassert(actual(db).try_summary_query("yearmin=1999", 0));
    wassert(actual(db).try_summary_query("yearmin=1945", 4));
    wassert(actual(db).try_summary_query("yearmax=1944", 0));
    wassert(actual(db).try_summary_query("yearmax=1945", 4));
    wassert(actual(db).try_summary_query("yearmax=2030", 4));
    wassert(actual(db).try_summary_query("year=1944", 0));
    wassert(actual(db).try_summary_query("year=1945", 4));
    wassert(actual(db).try_summary_query("year=1946", 0));
    wassert(actual(db).try_summary_query("B01001=1", 2));
    wassert(actual(db).try_summary_query("B01001=2", 0));
    wassert(actual(db).try_summary_query("B01002=3", 0));
    wassert(actual(db).try_summary_query("B01002=4", 2));
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
    wassert(actual(db).try_summary_query("data_filter=B12101<300.0", 1));
    wassert(actual(db).try_summary_query("data_filter=B12101<=300.0", 2));
    wassert(actual(db).try_summary_query("data_filter=B12101=300.0", 1));
    wassert(actual(db).try_summary_query("data_filter=B12101>=300,0", 1));
    wassert(actual(db).try_summary_query("data_filter=B12101>300.0", 1));
    wassert(actual(db).try_summary_query("data_filter=B12101<400.0", 2));
    wassert(actual(db).try_summary_query("data_filter=B12101<=400.0", 2));
    wassert(actual(db).try_summary_query("data_filter=B12102>400.0", 0));
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
    wassert(actual(db).try_summary_query("mobile=0", 4));
    wassert(actual(db).try_summary_query("mobile=1", 0));
    wassert(actual(db).try_summary_query("pindicator=20", 4));
    wassert(actual(db).try_summary_query("pindicator=21", 0));
    wassert(actual(db).try_summary_query("p1=111", 4));
    wassert(actual(db).try_summary_query("p1=112", 0));
    wassert(actual(db).try_summary_query("p2=121", 0));
    wassert(actual(db).try_summary_query("p2=122", 4));
    wassert(actual(db).try_summary_query("p2=123", 0));
    wassert(actual(db).try_summary_query("leveltype1=10", 4));
    wassert(actual(db).try_summary_query("leveltype1=11", 0));
    wassert(actual(db).try_summary_query("leveltype2=15", 4));
    wassert(actual(db).try_summary_query("leveltype2=16", 0));
    wassert(actual(db).try_summary_query("l1=11", 4));
    wassert(actual(db).try_summary_query("l1=12", 0));
    wassert(actual(db).try_summary_query("l2=22", 4));
    wassert(actual(db).try_summary_query("l2=23", 0));
    wassert(actual(db).try_summary_query("var=B01001", 0));
    wassert(actual(db).try_summary_query("var=B12101", 2));
    wassert(actual(db).try_summary_query("var=B12102", 0));
    wassert(actual(db).try_summary_query("rep_cod=1", 0));
    wassert(actual(db).try_summary_query("rep_cod=2", 4));
    wassert(actual(db).try_summary_query("rep_cod=3", 0));
    wassert(actual(db).try_summary_query("priority=101", 0));
    wassert(actual(db).try_summary_query("priority=81", 4));
    wassert(actual(db).try_summary_query("priority=102", 0));
    wassert(actual(db).try_summary_query("priomin=70", 4));
    wassert(actual(db).try_summary_query("priomin=80", 4));
    wassert(actual(db).try_summary_query("priomin=90", 0));
    wassert(actual(db).try_summary_query("priomax=70", 0));
    wassert(actual(db).try_summary_query("priomax=81", 4));
    wassert(actual(db).try_summary_query("priomax=100", 4));
    wassert(actual(db).try_summary_query("context_id=1", 1));
    wassert(actual(db).try_summary_query("context_id=11", 1));
}

template<> template<> void to::test<22>() { use_db(V5); test_value_update(); }
template<> template<> void to::test<23>() { use_db(V6); test_value_update(); }
template<> template<> void to::test<24>() { use_db(MEM); test_value_update(); }
void db1_shar::test_value_update()
{
    db->reset();
    dballe::tests::TestRecord dataset = dataset0;
    Record& attrs = dataset.attrs[WR_VAR(0, 1, 12)];
    attrs.set(WR_VAR(0, 33, 7), 50);
    wruntest(dataset.insert, *db);

    Record q;
    q.set(DBA_KEY_LAT, 12.34560);
    q.set(DBA_KEY_LON, 76.54320);
    q.set(DBA_KEY_YEAR, 1945);
    q.set(DBA_KEY_MONTH, 4);
    q.set(DBA_KEY_DAY, 25);
    q.set(DBA_KEY_HOUR, 8);
    q.set(DBA_KEY_MIN, 0);
    q.set(DBA_KEY_SEC, 0);
    q.set(DBA_KEY_REP_COD, 1);
    q.set(Level(10, 11, 15, 22));
    q.set(Trange(20, 111, 122));
    q.set(DBA_KEY_VAR, "B01012");

    // Query the initial value
    auto_ptr<db::Cursor> cur = db->query_data(q);
    ensure_equals(cur->remaining(), 1);
    cur->next();
    int ana_id = cur->get_station_id();
    wreport::Var var = cur->get_var();
    ensure_equals(var.enqi(), 300);

    // Query the attributes and check that they are there
    AttrList qcs;
    qcs.push_back(WR_VAR(0, 33, 7));
    Record qattrs;
    ensure_equals(cur->query_attrs(qcs, qattrs), 1u);
    ensure_equals(qattrs.get(WR_VAR(0, 33, 7), MISSING_INT), 50);

    // Update it
    Record update;
    update.set(DBA_KEY_ANA_ID, ana_id);
    update.set(DBA_KEY_REP_COD, 1);
    int dt[6];
    q.get_datetime(dt);
    update.set_datetime(dt);
    update.set(q.get_level());
    update.set(q.get_trange());
    var.seti(200);
    update.set(var);
    db->insert(update, true, false);

    // Query again
    cur = db->query_data(q);
    ensure_equals(cur->remaining(), 1);
    cur->next();
    var = cur->get_var();
    ensure_equals(var.enqi(), 200);

    qattrs.clear();
    ensure_equals(cur->query_attrs(qcs, qattrs), 1u);
    ensure_equals(qattrs.get(WR_VAR(0, 33, 7), MISSING_INT), 50);
}

template<> template<> void to::test<25>() { use_db(V5); test_query_step_by_step(); }
template<> template<> void to::test<26>() { use_db(V6); test_query_step_by_step(); }
template<> template<> void to::test<27>() { use_db(MEM); test_query_step_by_step(); }
void db1_shar::test_query_step_by_step()
{
    // Try a query checking all the steps
    wruntest(populate_database);

    // Prepare a query
    query.clear();
    query.set(DBA_KEY_LATMIN, 10.0);

    // Make the query
    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 4);

    ensure(cur->next());
    // remaining() should decrement
    ensure_equals(cur->remaining(), 3);
    // results should match what was inserted
    wassert(actual(cur).data_matches(dataset0));
    // just call to_record now, to check if in the next call old variables are removed
    cur->to_record(result);

    ensure(cur->next());
    ensure_equals(cur->remaining(), 2);
    wassert(actual(cur).data_matches(dataset0));

    // Variables from the previous to_record should be removed
    cur->to_record(result);
    wassert(actual(result.vars().size()) == 1u);


    ensure(cur->next());
    ensure_equals(cur->remaining(), 1);
    wassert(actual(cur).data_matches(dataset1));

    ensure(cur->next());
    ensure_equals(cur->remaining(), 0);
    wassert(actual(cur).data_matches(dataset1));

    // Now there should not be anything anymore
    ensure_equals(cur->remaining(), 0);
    ensure(!cur->next());
}

template<> template<> void to::test<28>() { use_db(V5); test_double_stationinfo_insert(); }
template<> template<> void to::test<29>() { use_db(V6); test_double_stationinfo_insert(); }
template<> template<> void to::test<30>() { use_db(MEM); test_double_stationinfo_insert(); }
void db1_shar::test_double_stationinfo_insert()
{
    //wassert(actual(*db).empty());
    wruntest(ds_st_navile.insert, *db, true);
    wruntest(ds_st_navile.insert, *db, true);

    // Query station data and ensure there is only one info (height)
    query.clear();
    query.set_ana_context();
    auto_ptr<db::Cursor> cur = db->query_data(query);
    wassert(actual(cur->remaining()) == 1);
    cur->next();
    wassert(actual(cur).station_vars_match(ds_st_navile));
}


template<> template<> void to::test<31>() { use_db(V5); test_double_stationinfo_insert1(); }
template<> template<> void to::test<32>() { use_db(V6); test_double_stationinfo_insert1(); }
template<> template<> void to::test<33>() { use_db(MEM); test_double_stationinfo_insert1(); }
void db1_shar::test_double_stationinfo_insert1()
{
    //wassert(actual(*db).empty());

    dballe::tests::TestStation ds_st_navile_metar(ds_st_navile);
    ds_st_navile_metar.info["metar"] = ds_st_navile_metar.info["synop"];
    ds_st_navile_metar.info.erase("synop");
    wruntest(ds_st_navile.insert, *db, true);
    wruntest(ds_st_navile_metar.insert, *db, true);

    // Query station data and ensure there is only one info (height)
    query.clear();
    query.set_ana_context();
    auto_ptr<db::Cursor> cur = db->query_data(query);
    wassert(actual(cur->remaining()) == 2);

    // Ensure that the network info is preserved
    wassert(actual(cur->next()).istrue());
    wassert(actual(cur->get_rep_memo()) == "synop");
    wassert(actual(cur->next()).istrue());
    wassert(actual(cur->get_rep_memo()) == "metar");
}

template<> template<> void to::test<34>() { use_db(V5); test_insert_undef_lev2(); }
template<> template<> void to::test<35>() { use_db(V6); test_insert_undef_lev2(); }
template<> template<> void to::test<36>() { use_db(MEM); test_insert_undef_lev2(); }
void db1_shar::test_insert_undef_lev2()
{
    // Insert with undef leveltype2 and l2
    use_db();
    wruntest(populate_database);

    dballe::tests::TestRecord dataset = dataset0;
    dataset.data.unset(WR_VAR(0, 1, 11));
    dataset.data.set(DBA_KEY_LEVELTYPE1, 44);
    dataset.data.set(DBA_KEY_L1, 55);
    dataset.data.unset(DBA_KEY_LEVELTYPE2);
    dataset.data.unset(DBA_KEY_L2);
    wruntest(dataset.insert, *db, true);

    // Query it back
    query.clear();
    query.set(DBA_KEY_LEVELTYPE1, 44);
    query.set(DBA_KEY_L1, 55);

    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);

    ensure(cur->next());
    result.clear();
    cur->to_record(result);

    ensure(result.key_peek(DBA_KEY_LEVELTYPE1) != NULL);
    ensure_equals(result[DBA_KEY_LEVELTYPE1].enqi(), 44);
    ensure(result.key_peek(DBA_KEY_L1) != NULL);
    ensure_equals(result[DBA_KEY_L1].enqi(), 55);
    ensure(result.key_peek(DBA_KEY_LEVELTYPE2) != NULL);
    ensure_equals(result[DBA_KEY_LEVELTYPE2].enqi(), MISSING_INT);
    ensure(result.key_peek(DBA_KEY_L2) != NULL);
    ensure_equals(result[DBA_KEY_L2].enqi(), MISSING_INT);

    ensure(!cur->next());
}

template<> template<> void to::test<37>() { use_db(V5); test_query_undef_lev2(); }
template<> template<> void to::test<38>() { use_db(V6); test_query_undef_lev2(); }
template<> template<> void to::test<39>() { use_db(MEM); test_query_undef_lev2(); }
void db1_shar::test_query_undef_lev2()
{
    // Query with undef leveltype2 and l2
    use_db();
    wruntest(populate_database);

    query.clear();
    query.set(DBA_KEY_LEVELTYPE1, 10);
    query.set(DBA_KEY_L1, 11);

    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 4);
    cur->discard_rest();
}


template<> template<> void to::test<40>() { use_db(V5); test_query_bad_attr_filter(); }
template<> template<> void to::test<41>() { use_db(V6); test_query_bad_attr_filter(); }
template<> template<> void to::test<42>() { use_db(MEM); test_query_bad_attr_filter(); }
void db1_shar::test_query_bad_attr_filter()
{
    // Query with an incorrect attr_filter
    use_db();
    wruntest(populate_database);

    query.clear();
    query.set(DBA_KEY_ATTR_FILTER, "B12001");

    try {
        db->query_data(query);
    } catch (error_consistency& e) {
        ensure_contains(e.what(), "B12001 is not a valid filter");
    }
}

template<> template<> void to::test<43>() { use_db(V5); test_querybest_priomax(); }
template<> template<> void to::test<44>() { use_db(V6); test_querybest_priomax(); }
template<> template<> void to::test<45>() { use_db(MEM); test_querybest_priomax(); }
void db1_shar::test_querybest_priomax()
{
    // Test querying priomax together with query=best
    use_db();
    // Start with an empty database
    db->reset();

    // Prepare the common parts of some data
    insert.clear();
    insert.set(DBA_KEY_LAT, 1);
    insert.set(DBA_KEY_LON, 1);
    insert.set(Level(1, 0));
    insert.set(Trange(254, 0, 0));
    insert.set_datetime(2009, 11, 11, 0, 0, 0);

    //  1,synop,synop,101,oss,0
    //  2,metar,metar,81,oss,0
    //  3,temp,sounding,98,oss,2
    //  4,pilot,wind profile,80,oss,2
    //  9,buoy,buoy,50,oss,31
    // 10,ship,synop ship,99,oss,1
    // 11,tempship,temp ship,100,oss,2
    // 12,airep,airep,82,oss,4
    // 13,amdar,amdar,97,oss,4
    // 14,acars,acars,96,oss,4
    // 42,pollution,pollution,199,oss,8
    // 200,satellite,NOAA satellites,41,oss,255
    // 255,generic,generic data,1000,?,255
    static int rep_cods[] = { 1, 2, 3, 4, 9, 10, 11, 12, 13, 14, 42, 200, 255, -1 };

    for (int* i = rep_cods; *i != -1; ++i)
    {
        insert.set(DBA_KEY_REP_COD, *i);
        insert.set(WR_VAR(0, 12, 101), *i);
        db->insert(insert, false, true);
        insert.unset(DBA_KEY_CONTEXT_ID);
    }

    // Query with querybest only
    {
        query.clear();
        query.set(DBA_KEY_QUERY, "best");
        query.set_datetime(2009, 11, 11, 0, 0, 0);
        query.set(DBA_KEY_VAR, "B12101");
        auto_ptr<db::Cursor> cur = db->query_data(query);

        ensure_equals(cur->remaining(), 1);

        ensure(cur->next());
        result.clear();
        cur->to_record(result);

        ensure(result.key_peek(DBA_KEY_REP_COD) != NULL);
        ensure_equals(result[DBA_KEY_REP_COD].enqi(), 255);

        cur->discard_rest();
    }

    //db->dump(stderr);
    //system("bash");

    // Query with querybest and priomax
    {
        query.clear();
        query.set(DBA_KEY_PRIOMAX, 100);
        query.set(DBA_KEY_QUERY, "best");
        query.set_datetime(2009, 11, 11, 0, 0, 0);
        query.set(DBA_KEY_VAR, "B12101");
        auto_ptr<db::Cursor> cur = db->query_data(query);
        ensure_equals(cur->remaining(), 1);

        ensure(cur->next());
        result.clear();
        cur->to_record(result);

        ensure(result.key_peek(DBA_KEY_REP_COD) != NULL);
        ensure_equals(result[DBA_KEY_REP_COD].enqi(), 11);

        cur->discard_rest();
    }
}

template<> template<> void to::test<46>() { use_db(V5); test_repmemo_in_output(); }
template<> template<> void to::test<47>() { use_db(V6); test_repmemo_in_output(); }
template<> template<> void to::test<48>() { use_db(MEM); test_repmemo_in_output(); }
void db1_shar::test_repmemo_in_output()
{
    // Ensure that rep_memo is set in the results
    use_db();
    wruntest(populate_database);

    Record res;
    Record rec;
    auto_ptr<db::Cursor> cur = db->query_data(rec);
    while (cur->next())
    {
        cur->to_record(res);
        ensure(res.key_peek_value(DBA_KEY_REP_MEMO) != 0);
    }
}

}

/* vim:set ts=4 sw=4: */

