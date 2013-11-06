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

#ifndef TUT_TEST_BODY

#include "db-tut.h"
#include "db/querybuf.h"
#include <wibble/string.h>

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace dballe {
namespace tests {

void db_tests::test_simple_insert()
{
    // Insert some data
    dballe::tests::TestRecord ds;
    ds.station = ds_st_navile;
    ds.data.set(DBA_KEY_REP_MEMO, "synop");
    ds.data.set_datetime(2013, 10, 16, 10);
    ds.set_var("temp_2m", 16.5, 50);
    wruntest(ds.insert, *db);

    query.clear();

    // Query and verify the station data
    {
        auto_ptr<db::Cursor> cur = db->query_stations(query);
        wassert(actual(cur->remaining()) == 1);
        cur->next();
        wassert(actual(cur).station_vars_match(ds));
    }

    // Query and verify the measured data
    {
        auto_ptr<db::Cursor> cur = db->query_data(query);
        wassert(actual(cur->remaining()) == 1);
        cur->next();
        wassert(actual(cur).data_context_matches(ds));
        wassert(actual(cur).data_var_matches(ds, WR_VAR(0, 12, 101)));
    }

    // Query and verify attributes
    // TODO
}

void db_tests::test_insert()
{
    // Prepare a valid record to insert
    Record insert(dataset0.data);
    wrunchecked(dataset0.station.set_latlonident_into(insert));

    // Check if adding a nonexisting station when not allowed causes an error
    try {
        db->insert(insert, false, false);
        ensure(false);
    } catch (error_consistency& e) {
        wassert(actual(e.what()).contains("insert a station entry when it is forbidden"));
    } catch (error_notfound& e) {
        wassert(actual(e.what()).contains("synop station not found at"));
    }

    // Insert the record
    wrunchecked(db->insert(insert, false, true));
    // Check if duplicate updates are allowed by insert
    wrunchecked(db->insert(insert, true, false));
    // Check if duplicate updates are trapped by insert_new
    try {
        db->insert(insert, false, false);
        ensure(false);
    } catch (wreport::error& e) {
        wassert(actual(e.what()).matches("([Dd]uplicate|not unique|cannot replace an existing value)"));
    }
}

void db_tests::test_double_station_insert()
{
    // Prepare a valid record to insert
    Record insert(dataset0.data);
    wrunchecked(dataset0.station.set_latlonident_into(insert));

    // Insert the record twice
    wrunchecked(db->insert(insert, false, true));
    // This should fail, refusing to replace station info
    try {
        db->insert(insert, false, true);
        ensure(false);
    } catch (wreport::error& e) {
        wassert(actual(e.what()).matches("([Dd]uplicate|not unique|cannot replace an existing value)"));
    }
}

void db_tests::test_ana_query()
{
    wruntest(populate_database);

    /*
       CHECKED(dba_ana_count(db, &count));
       fail_unless(count == 1);
       */
    query.clear();

    /* Iterate the anagraphic database */
    auto_ptr<db::Cursor> cur = db->query_stations(query);
    ensure_equals(cur->remaining(), 1);

    /* There should be an item */
    ensure(cur->next());
    ensure_equals(cur->get_lat(), 12.34560);
    ensure_equals(cur->get_lon(), 76.54320);
    ensure_equals(cur->get_ident(), (const char*)0);

    // Check that the result matches
    wassert(actual(cur).station_keys_match(dataset0.station));

    /* There should be only one item */
    ensure_equals(cur->remaining(), 0);

    ensure(!cur->next());
}

void db_tests::test_misc_queries()
{
    wruntest(populate_database);

/* Try a query using a KEY query parameter */
#define TRY_QUERY(qstring, expected_count) wassert(actual(db).try_data_query(qstring, expected_count))

    TRY_QUERY("ana_id=1", 4);
    TRY_QUERY("ana_id=2", 0);
    {
        query.clear();
        query.set_ana_context();
        auto_ptr<db::Cursor> cur = db->query_data(query);
        ensure_equals(cur->remaining(), 5);
    }
    //TRY_QUERY(DBA_KEY_YEAR, 1000, 10);
    TRY_QUERY("year=1001", 0);
    TRY_QUERY("yearmin=1999", 0);
    TRY_QUERY("yearmin=1945", 4);
    TRY_QUERY("yearmax=1944", 0);
    TRY_QUERY("yearmax=1945", 4);
    TRY_QUERY("yearmax=2030", 4);
    TRY_QUERY("year=1944", 0);
    TRY_QUERY("year=1945", 4);
    TRY_QUERY("year=1946", 0);
    TRY_QUERY("B01001=1", 4);
    TRY_QUERY("B01001=2", 0);
    TRY_QUERY("B01002=52", 4);
    TRY_QUERY("B01002=53", 0);
    TRY_QUERY("ana_filter=block=1", 4);
    TRY_QUERY("ana_filter=B01001=1", 4);
    TRY_QUERY("ana_filter=block>1", 0);
    TRY_QUERY("ana_filter=B01001>1", 0);
    TRY_QUERY("ana_filter=block<=1", 4);
    TRY_QUERY("ana_filter=B01001<=1", 4);
    TRY_QUERY("ana_filter=0<=B01001<=2", 4);
    TRY_QUERY("ana_filter=1<=B01001<=1", 4);
    TRY_QUERY("ana_filter=2<=B01001<=4", 0);
    TRY_QUERY("data_filter=B01011=DB-All.e!", 1);
    TRY_QUERY("data_filter=B01012<300", 0);
    TRY_QUERY("data_filter=B01012<=300", 1);
    TRY_QUERY("data_filter=B01012=300", 1);
    TRY_QUERY("data_filter=B01012>=300", 2);
    TRY_QUERY("data_filter=B01012>300", 1);
    TRY_QUERY("data_filter=B01012<400", 1);
    TRY_QUERY("data_filter=B01012<=400", 2);

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
    TRY_QUERY("mobile=0", 4);
    TRY_QUERY("mobile=1", 0);
    //TRY_QUERY(c, DBA_KEY_IDENT_SELECT, "pippo");
    TRY_QUERY("pindicator=20", 4);
    TRY_QUERY("pindicator=21", 0);
    TRY_QUERY("p1=111", 4);
    TRY_QUERY("p1=112", 0);
    TRY_QUERY("p2=121", 0);
    TRY_QUERY("p2=122", 2);
    TRY_QUERY("p2=123", 2);
    TRY_QUERY("leveltype1=10", 4);
    TRY_QUERY("leveltype1=11", 0);
    TRY_QUERY("leveltype2=15", 4);
    TRY_QUERY("leveltype2=16", 0);
    TRY_QUERY("l1=11", 4);
    TRY_QUERY("l1=12", 0);
    TRY_QUERY("l2=22", 4);
    TRY_QUERY("l2=23", 0);
    TRY_QUERY("var=B01011", 2);
    TRY_QUERY("var=B01012", 2);
    TRY_QUERY("var=B01013", 0);
    TRY_QUERY("rep_memo=synop", 2);
    TRY_QUERY("rep_memo=metar", 2);
    TRY_QUERY("rep_memo=temp", 0);
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
    TRY_QUERY("context_id=1", 1);
    TRY_QUERY("context_id=11", 0);

#undef TRY_QUERY
#undef TRY_QUERY2
}

void db_tests::test_querybest()
{
    wruntest(populate_database);

    //if (db->server_type == ORACLE || db->server_type == POSTGRES)
    //      return;

    // Prepare a query
    query.clear();
    query.set(DBA_KEY_LATMIN, 1000000);
    query.set(DBA_KEY_QUERY, "best");

    // Make the query
    auto_ptr<db::Cursor> cur = db->query_data(query);

    ensure_equals(cur->remaining(), 4);

    // There should be four items
    ensure(cur->next());
    ensure_equals(cur->get_lat(), 12.34560);
    ensure_equals(cur->get_lon(), 76.54320);
    ensure_equals(cur->get_ident(), (const char*)0);
    ensure(cur->get_rep_memo());
    ensure_equals(string(cur->get_rep_memo()), "synop");
    ensure_equals(cur->get_level(), Level(10, 11, 15, 22));
    ensure_equals(cur->get_trange(), Trange(20, 111, 122));
    ensure_equals(cur->get_varcode(), WR_VAR(0, 1, 11));
    ensure_equals(cur->get_var().code(), WR_VAR(0, 1, 11));
    ensure_equals(cur->remaining(), 3);
    ensure(cur->next());
    ensure_equals(cur->remaining(), 2);
    ensure(cur->next());
    ensure_equals(cur->remaining(), 1);
    ensure(cur->next());
    ensure_equals(cur->remaining(), 0);

    // Now there should not be anything anymore
    ensure(!cur->next());
}

void db_tests::test_deletion()
{
    wruntest(populate_database);

    // 4 items to begin with
    query.clear();
    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 4);
    cur->discard_rest();

    query.clear();
    query.set(DBA_KEY_YEARMIN, 1945);
    query.set(DBA_KEY_MONTHMIN, 4);
    query.set(DBA_KEY_DAYMIN, 25);
    query.set(DBA_KEY_HOURMIN, 8);
    query.set(DBA_KEY_MINUMIN, 10);
    db->remove(query);

    // 2 remaining after remove
    query.clear();
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 2);
    cur->discard_rest();

    // Did it remove the right ones?
    query.clear();
    query.set(DBA_KEY_LATMIN, 1000000);
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 2);
    ensure(cur->next());
    wassert(actual(cur).data_context_matches(dataset0));

    Varcode last_code = 0;
    for (unsigned i = 0; i < 2; ++i)
    {
        // Check that varcodes do not repeat
        if (last_code != 0)
            wassert(actual(cur->get_varcode()) != last_code);
        last_code = cur->get_varcode();

        switch (last_code)
        {
            case WR_VAR(0, 1, 11):
            case WR_VAR(0, 1, 12):
                wassert(actual(cur).data_var_matches(dataset0, last_code));
                break;
            default:
                ensure(false);
        }

        if (i == 0)
        {
            /* The item should have two data in it */
            ensure(cur->next());
        } else {
            ensure(!cur->next());
        }
    }
}

void db_tests::test_datetime_queries()
{
    /* Prepare test data */
    Record base, a, b;

    base.set(DBA_KEY_LAT, 12.0);
    base.set(DBA_KEY_LON, 48.0);
    base.set(DBA_KEY_MOBILE, 0);

    /*
       base.set(DBA_KEY_HEIGHT, 42);
       base.set(DBA_KEY_HEIGHTBARO, 234);
       base.set(DBA_KEY_BLOCK, 1);
       base.set(DBA_KEY_STATION, 52);
       base.set(DBA_KEY_NAME, "Cippo Lippo");
       */

    base.set(DBA_KEY_LEVELTYPE1, 1);
    base.set(DBA_KEY_L1, 0);
    base.set(DBA_KEY_LEVELTYPE2, 1);
    base.set(DBA_KEY_L2, 0);
    base.set(DBA_KEY_PINDICATOR, 1);
    base.set(DBA_KEY_P1, 0);
    base.set(DBA_KEY_P2, 0);

    base.set(DBA_KEY_REP_MEMO, "synop");
    base.set(DBA_KEY_PRIORITY, 101);

    base.set(WR_VAR(0, 1, 12), 500);

    base.set(DBA_KEY_YEAR, 2006);
    base.set(DBA_KEY_MONTH, 5);
    base.set(DBA_KEY_DAY, 15);
    base.set(DBA_KEY_HOUR, 12);
    base.set(DBA_KEY_MIN, 30);
    base.set(DBA_KEY_SEC, 0);

#define WANTRESULT(ab) do { \
    auto_ptr<db::Cursor> cur = db->query_data(query); \
    ensure_equals(cur->remaining(), 1); \
    ensure(cur->next()); \
    cur->to_record(result); \
    ensure_equals(cur->remaining(), 0); \
    ensure_varcode_equals(result.vars()[0]->code(), WR_VAR(0, 1, 12)); \
    ensure(result.contains(ab)); \
    cur->discard_rest(); \
} while(0)

    /* Year */
    db->reset();

    insert.clear();
    a = base;
    a.set(DBA_KEY_YEAR, 2005);
    insert.add(a);
    db->insert(insert, false, true);

    insert.clear();
    b = base;
    b.set(DBA_KEY_YEAR, 2006);
    insert.add(b);
    db->insert(insert, false, false);

    query.clear();
    query.set(DBA_KEY_YEARMIN, 2006);
    WANTRESULT(b);

    query.clear();
    query.set(DBA_KEY_YEARMAX, 2005);
    WANTRESULT(a);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    WANTRESULT(b);


    /* Month */
    db->reset();

    insert.clear();
    a = base;
    a.set(DBA_KEY_YEAR, 2006);
    a.set(DBA_KEY_MONTH, 4);
    insert.add(a);
    db->insert(insert, false, true);

    insert.clear();
    b = base;
    b.set(DBA_KEY_YEAR, 2006);
    b.set(DBA_KEY_MONTH, 5);
    insert.add(b);
    db->insert(insert, false, false);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTHMIN, 5);
    WANTRESULT(b);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTHMAX, 4);
    WANTRESULT(a);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    WANTRESULT(b);

    /* Day */
    db->reset();

    insert.clear();
    a = base;
    a.set(DBA_KEY_YEAR, 2006);
    a.set(DBA_KEY_MONTH, 5);
    a.set(DBA_KEY_DAY, 2);
    insert.add(a);
    db->insert(insert, false, true);

    insert.clear();
    b = base;
    b.set(DBA_KEY_YEAR, 2006);
    b.set(DBA_KEY_MONTH, 5);
    b.set(DBA_KEY_DAY, 3);
    insert.add(b);
    db->insert(insert, false, false);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAYMIN, 3);
    WANTRESULT(b);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAYMAX, 2);
    WANTRESULT(a);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    WANTRESULT(b);

    /* Hour */
    db->reset();

    insert.clear();
    a = base;
    a.set(DBA_KEY_YEAR, 2006);
    a.set(DBA_KEY_MONTH, 5);
    a.set(DBA_KEY_DAY, 3);
    a.set(DBA_KEY_HOUR, 12);
    insert.add(a);
    db->insert(insert, false, true);

    insert.clear();
    b = base;
    b.set(DBA_KEY_YEAR, 2006);
    b.set(DBA_KEY_MONTH, 5);
    b.set(DBA_KEY_DAY, 3);
    b.set(DBA_KEY_HOUR, 13);
    insert.add(b);
    db->insert(insert, false, false);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOURMIN, 13);
    WANTRESULT(b);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOURMAX, 12);
    WANTRESULT(a);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOUR, 13);
    WANTRESULT(b);

    /* Minute */
    db->reset();

    insert.clear();
    a = base;
    a.set(DBA_KEY_YEAR, 2006);
    a.set(DBA_KEY_MONTH, 5);
    a.set(DBA_KEY_DAY, 3);
    a.set(DBA_KEY_HOUR, 12);
    a.set(DBA_KEY_MIN, 29);
    insert.add(a);
    db->insert(insert, false, true);

    insert.clear();
    b = base;
    b.set(DBA_KEY_YEAR, 2006);
    b.set(DBA_KEY_MONTH, 5);
    b.set(DBA_KEY_DAY, 3);
    b.set(DBA_KEY_HOUR, 12);
    b.set(DBA_KEY_MIN, 30);
    insert.add(b);
    db->insert(insert, false, false);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOUR, 12);
    query.set(DBA_KEY_MINUMIN, 30);
    WANTRESULT(b);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOUR, 12);
    query.set(DBA_KEY_MINUMAX, 29);
    WANTRESULT(a);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOUR, 12);
    query.set(DBA_KEY_MIN, 30);
    WANTRESULT(b);
}

void db_tests::test_qc()
{
    wruntest(populate_database);

    query.clear();
    query.set(DBA_KEY_LATMIN, 1000000);
    auto_ptr<db::Cursor> cur = db->query_data(query);

    // Move the cursor to B01011
    bool found = false;
    while (cur->next())
    {
        cur->to_record(result);
        if (result.vars()[0]->code() == WR_VAR(0, 1, 11))
        {
            cur->discard_rest();
            found = true;
            break;
        }
    }
    ensure(found);

    int context_id = cur->attr_reference_id();

    // Insert new attributes about this report
    qc.clear();
    qc.set(WR_VAR(0, 33, 2), 2);
    qc.set(WR_VAR(0, 33, 3), 5);
    qc.set(WR_VAR(0, 33, 5), 33);
    db->attr_insert(context_id, WR_VAR(0, 1, 11), qc);

    // Query back the data
    qc.clear();
    vector<Varcode> codes;
    ensure_equals(db->query_attrs(context_id, WR_VAR(0, 1, 11), codes, qc), 3);

    const Var* attr = qc.var_peek(WR_VAR(0, 33, 2));
    ensure(attr != NULL);
    ensure_equals(attr->enqi(), 2);

    attr = qc.var_peek(WR_VAR(0, 33, 3));
    ensure(attr != NULL);
    ensure_equals(attr->enqi(), 5);

    attr = qc.var_peek(WR_VAR(0, 33, 5));
    ensure(attr != NULL);
    ensure_equals(attr->enqi(), 33);

    // Delete a couple of items
    codes.push_back(WR_VAR(0, 33, 2));
    codes.push_back(WR_VAR(0, 33, 5));
    db->attr_remove(context_id, WR_VAR(0, 1, 11), codes);

    // Deleting non-existing items should not fail.  Also try creating a
    // query with just one item
    codes.clear();
    codes.push_back(WR_VAR(0, 33, 2));
    db->attr_remove(context_id, WR_VAR(0, 1, 11), codes);

    /* Query back the data */
    qc.clear();
    codes.clear();
    codes.push_back(WR_VAR(0, 33, 2));
    codes.push_back(WR_VAR(0, 33, 3));
    codes.push_back(WR_VAR(0, 33, 5));
    ensure_equals(db->query_attrs(context_id, WR_VAR(0, 1, 11), codes, qc), 1);

    ensure(qc.var_peek(WR_VAR(0, 33, 2)) == NULL);
    ensure(qc.var_peek(WR_VAR(0, 33, 5)) == NULL);
    attr = qc.var_peek(WR_VAR(0, 33, 3));
    ensure(attr != NULL);
    ensure_equals(attr->enqi(), 5);
    /*dba_error_remove_callback(DBA_ERR_NONE, crash, 0);*/
}

void db_tests::test_ana_queries()
{
    wruntest(populate_database);

    query.clear();
    query.set(DBA_KEY_REP_MEMO, "synop");

    auto_ptr<db::Cursor> cur = db->query_stations(query);
    ensure_equals(cur->remaining(), 1);

    ensure(cur->next());
    ensure(!cur->next());
}

void db_tests::test_vacuum()
{
    db->vacuum();
}

void db_tests::test_attrs()
{
    // Insert a data record
    wruntest(dataset0.insert, *db);

    qc.clear();
    qc.set(WR_VAR(0,  1,  7),  1);
    qc.set(WR_VAR(0,  2, 48),  2);
    qc.set(WR_VAR(0,  5, 40),  3);
    qc.set(WR_VAR(0,  5, 41),  4);
    qc.set(WR_VAR(0,  5, 43),  5);
    qc.set(WR_VAR(0, 33, 32),  6);
    qc.set(WR_VAR(0,  7, 24),  7);
    qc.set(WR_VAR(0,  5, 21),  8);
    qc.set(WR_VAR(0,  7, 25),  9);
    qc.set(WR_VAR(0,  5, 22), 10);

    db->attr_insert(WR_VAR(0, 1, 11), qc, false);

    // Query back the B01011 variable to read the attr reference id
    Record query;
    query.set(DBA_KEY_VAR, "B01011");
    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->next();
    int attr_id = cur->attr_reference_id();
    cur->discard_rest();

    qc.clear();
    vector<Varcode> codes;
    int count = db->query_attrs(attr_id, WR_VAR(0, 1, 11), codes, qc);
    ensure_equals(count, 10);

    // Check that all the attributes come out
    const vector<Var*> vars = qc.vars();
    ensure_equals(vars.size(), 10);
    ensure_varcode_equals(vars[0]->code(), WR_VAR(0,   1,  7)); ensure_var_equals(*vars[0],  1);
    ensure_varcode_equals(vars[1]->code(), WR_VAR(0,   2, 48)); ensure_var_equals(*vars[1],  2);
    ensure_varcode_equals(vars[2]->code(), WR_VAR(0,   5, 21)); ensure_var_equals(*vars[2],  8);
    ensure_varcode_equals(vars[3]->code(), WR_VAR(0,   5, 22)); ensure_var_equals(*vars[3], 10);
    ensure_varcode_equals(vars[4]->code(), WR_VAR(0,   5, 40)); ensure_var_equals(*vars[4],  3);
    ensure_varcode_equals(vars[5]->code(), WR_VAR(0,   5, 41)); ensure_var_equals(*vars[5],  4);
    ensure_varcode_equals(vars[6]->code(), WR_VAR(0,   5, 43)); ensure_var_equals(*vars[6],  5);
    ensure_varcode_equals(vars[7]->code(), WR_VAR(0,   7, 24)); ensure_var_equals(*vars[7],  7);
    ensure_varcode_equals(vars[8]->code(), WR_VAR(0,   7, 25)); ensure_var_equals(*vars[8],  9);
    ensure_varcode_equals(vars[9]->code(), WR_VAR(0,  33, 32)); ensure_var_equals(*vars[9],  6);
}

void db_tests::test_wrap_longitude()
{
    // Insert a data record
    wruntest(dataset0.insert, *db);

    query.clear();
    query.set(DBA_KEY_LATMIN, 10.0);
    query.set(DBA_KEY_LATMAX, 15.0);
    query.set(DBA_KEY_LONMIN, 70.0);
    query.set(DBA_KEY_LONMAX, -160.0);

    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 2);
    cur->discard_rest();
}

void db_tests::test_ana_filter()
{
    // Test numeric comparisons in ana_filter
    wruntest(populate_database);

    query.clear();
    query.set(DBA_KEY_REP_MEMO, "metar");
    query.set(DBA_KEY_VAR, "B01011");
    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);

    // Move the cursor to B01011
    cur->next();
    int context_id = cur->attr_reference_id();
    cur->discard_rest();

    // Insert new attributes about this report
    qc.clear();
    qc.set(WR_VAR(0, 1, 1), 50);
    qc.set(WR_VAR(0, 1, 8), "50");
    db->attr_insert(context_id, WR_VAR(0, 1, 11), qc);

    // Try queries filtered by numeric attributes
    query.clear();
    query.set(DBA_KEY_REP_MEMO, "metar");
    query.set(DBA_KEY_VAR, "B01011");

    query.set(DBA_KEY_ATTR_FILTER, "B01001=50");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01001<=50");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01001<51");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01001<8");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 0);
    cur->discard_rest();

    // Try queries filtered by string attributes
    query.clear();
    query.set(DBA_KEY_REP_MEMO, "metar");
    query.set(DBA_KEY_VAR, "B01011");

    query.set(DBA_KEY_ATTR_FILTER, "B01008=50");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01008<=50");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01008<8");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01008<100");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 0);
    cur->discard_rest();
}

void db_tests::test_datetime_extremes()
{
#warning temporary disabled
#if 0
    wruntest(populate_database);

    // All DB
    query.clear();
    db->query_datetime_extremes(query, result);

    ensure_equals(result.get("yearmin",  -1), 1945);
    ensure_equals(result.get("monthmin", -1),    4);
    ensure_equals(result.get("daymin",   -1),   25);
    ensure_equals(result.get("hourmin",  -1),    8);
    ensure_equals(result.get("minumin",  -1),    0);
    ensure_equals(result.get("secmin",   -1),    0);

    ensure_equals(result.get("yearmax",  -1), 1945);
    ensure_equals(result.get("monthmax", -1),    4);
    ensure_equals(result.get("daymax",   -1),   25);
    ensure_equals(result.get("hourmax",  -1),    8);
    ensure_equals(result.get("minumax",  -1),   30);
    ensure_equals(result.get("secmax",   -1),    0);

    // Subset of the DB
    query.clear();
    query.set("pindicator", 20);
    query.set("p1", 111);
    query.set("p2", 122);
    db->query_datetime_extremes(query, result);

    ensure_equals(result.get("yearmin",  -1), 1945);
    ensure_equals(result.get("monthmin", -1),    4);
    ensure_equals(result.get("daymin",   -1),   25);
    ensure_equals(result.get("hourmin",  -1),    8);
    ensure_equals(result.get("minumin",  -1),    0);
    ensure_equals(result.get("secmin",   -1),    0);

    ensure_equals(result.get("yearmax",  -1), 1945);
    ensure_equals(result.get("monthmax", -1),    4);
    ensure_equals(result.get("daymax",   -1),   25);
    ensure_equals(result.get("hourmax",  -1),    8);
    ensure_equals(result.get("minumax",  -1),    0);
    ensure_equals(result.get("secmax",   -1),    0);

    // No matches
    query.clear();
    query.set("pindicator", 1);
    db->query_datetime_extremes(query, result);

    ensure_equals(result.get("yearmin",  -1), -1);
    ensure_equals(result.get("monthmin", -1), -1);
    ensure_equals(result.get("daymin",   -1), -1);
    ensure_equals(result.get("hourmin",  -1), -1);
    ensure_equals(result.get("minumin",  -1), -1);
    ensure_equals(result.get("secmin",   -1), -1);

    ensure_equals(result.get("yearmax",  -1), -1);
    ensure_equals(result.get("monthmax", -1), -1);
    ensure_equals(result.get("daymax",   -1), -1);
    ensure_equals(result.get("hourmax",  -1), -1);
    ensure_equals(result.get("minumax",  -1), -1);
    ensure_equals(result.get("secmax",   -1), -1);
#endif
}

void db_tests::test_invalid_sql_querybest()
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

void db_tests::test_bug_querybest()
{
    // Reproduce a querybest scenario which produced always the same data record

    // Import lots
    const char** files = dballe::tests::bufr_files;
    for (int i = 0; files[i] != NULL; i++)
    {
        std::auto_ptr<Msgs> inmsgs = read_msgs(files[i], BUFR);
        Msg& msg = *(*inmsgs)[0];
        wrunchecked(db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_OVERWRITE));
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

void db_tests::test_bug_query_stations_by_level()
{
    // Reproduce a query that generated invalid SQL on V6
    wruntest(populate_database);

    // All DB
    query.clear();
    query.set(DBA_KEY_LEVELTYPE1, 103);
    query.set(DBA_KEY_L1, 2000);
    db->query_stations(query);
}

void db_tests::test_bug_query_levels_by_station()
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

void db_tests::test_connect_leaks()
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

void db_tests::test_query_stations()
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

void db_tests::test_summary_queries()
{
    if (db->format() == V5)
    {
        bool worked = false;
        try {
            query.clear();
            db->query_summary(query);
            worked = true;
        } catch (error_unimplemented& e) {
            wassert(actual(e.what()).contains("not implemented"));
        }
        wassert(actual(worked).isfalse());
        return;
    }

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
    wassert(actual(db).try_summary_query("rep_memo=synop", 0));
    wassert(actual(db).try_summary_query("rep_memo=metar", 4));
    wassert(actual(db).try_summary_query("rep_memo=temp", 0));
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

void db_tests::test_value_update()
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
    q.set(DBA_KEY_REP_MEMO, "synop");
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
    update.set(DBA_KEY_REP_MEMO, "synop");
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

void db_tests::test_query_step_by_step()
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

void db_tests::test_double_stationinfo_insert()
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

void db_tests::test_double_stationinfo_insert1()
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

void db_tests::test_insert_undef_lev2()
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

void db_tests::test_query_undef_lev2()
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

void db_tests::test_query_bad_attr_filter()
{
    // Query with an incorrect attr_filter
    use_db();
    wruntest(populate_database);

    query.clear();
    query.set(DBA_KEY_ATTR_FILTER, "B12001");

    try {
        db->query_data(query);
    } catch (error_consistency& e) {
        wassert(actual(e.what()).matches("B12001 is not a valid filter|cannot find any operator in filter 'B12001'"));
    }
}

void db_tests::test_querybest_priomax()
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
    static const char* rep_memos[] = { "synop", "metar", "temp", "pilot", "buoy", "ship", "tempship", "airep", "amdar", "acars", "pollution", "satellite", "generic", NULL };
    for (const char** i = rep_memos; *i; ++i)
    {
        insert.set(DBA_KEY_REP_MEMO, *i);
        insert.set(WR_VAR(0, 12, 101), (int)(i - rep_memos));
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

        ensure(result.key_peek(DBA_KEY_REP_MEMO) != NULL);
        wassert(actual(result[DBA_KEY_REP_MEMO].enqc()) == "generic");

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

        ensure(result.key_peek(DBA_KEY_REP_MEMO) != NULL);
        wassert(actual(result[DBA_KEY_REP_MEMO].enqc()) == "tempship");

        cur->discard_rest();
    }
}

void db_tests::test_repmemo_in_output()
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
}

#else

template<> template<> void to::test<1>() { test_simple_insert(); }
template<> template<> void to::test<2>() { test_insert(); }
template<> template<> void to::test<3>() { test_double_station_insert(); }
template<> template<> void to::test<4>() { test_ana_query(); }
template<> template<> void to::test<5>() { test_misc_queries(); }
template<> template<> void to::test<6>() { test_querybest(); }
template<> template<> void to::test<7>() { test_deletion(); }
template<> template<> void to::test<8>() { test_datetime_queries(); }
template<> template<> void to::test<9>() { test_qc(); }
template<> template<> void to::test<10>() { test_ana_queries(); }
template<> template<> void to::test<11>() { test_vacuum(); }
template<> template<> void to::test<12>() { test_attrs(); }
template<> template<> void to::test<13>() { test_wrap_longitude(); }
template<> template<> void to::test<14>() { test_ana_filter(); }
template<> template<> void to::test<15>() { test_datetime_extremes(); }
template<> template<> void to::test<16>() { test_invalid_sql_querybest(); }
template<> template<> void to::test<17>() { test_bug_querybest(); }
template<> template<> void to::test<18>() { test_bug_query_stations_by_level(); }
template<> template<> void to::test<19>() { test_bug_query_levels_by_station(); }
template<> template<> void to::test<20>() { test_connect_leaks(); }
template<> template<> void to::test<21>() { test_query_stations(); }
template<> template<> void to::test<22>() { test_summary_queries(); }
template<> template<> void to::test<23>() { test_value_update(); }
template<> template<> void to::test<24>() { test_query_step_by_step(); }
template<> template<> void to::test<25>() { test_double_stationinfo_insert(); }
template<> template<> void to::test<26>() { test_double_stationinfo_insert1(); }
template<> template<> void to::test<27>() { test_insert_undef_lev2(); }
template<> template<> void to::test<28>() { test_query_undef_lev2(); }
template<> template<> void to::test<29>() { test_query_bad_attr_filter(); }
template<> template<> void to::test<30>() { test_querybest_priomax(); }
template<> template<> void to::test<31>() { test_repmemo_in_output(); }

#endif
