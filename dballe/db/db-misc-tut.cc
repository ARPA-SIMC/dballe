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

#include "config.h"
#include "db/test-utils-db.h"
#include "db/querybuf.h"
#include <wibble/string.h>

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace {

struct db_tests_misc : public dballe::tests::db_test
{
    TestStation ds_st_navile;
    db_tests_misc()
    {
        ds_st_navile.lat = 44.5008;
        ds_st_navile.lon = 11.3288;
        ds_st_navile.info["synop"].set(WR_VAR(0, 7, 30), 78); // Height
    }
};

}

namespace tut {

typedef db_tg<db_tests_misc> tg;
typedef tg::object to;

template<> template<> void to::test<1>()
{
    // Test a simple insert round trip

    // Insert some data
    dballe::tests::TestRecord ds;
    ds.station = ds_st_navile;
    ds.data.set(DBA_KEY_REP_MEMO, "synop");
    ds.data.set_datetime(2013, 10, 16, 10);
    ds.set_var("temp_2m", 16.5, 50);
    wruntest(ds.insert, *db);

    Record query;

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

template<> template<> void to::test<2>()
{
    // Test insert
    OldDballeTestFixture f;

    // Prepare a valid record to insert
    Record insert(f.dataset0.data);
    wrunchecked(f.dataset0.station.set_latlonident_into(insert));

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

template<> template<> void to::test<3>()
{
    // Test double station insert
    OldDballeTestFixture f;

    // Prepare a valid record to insert
    Record insert(f.dataset0.data);
    wrunchecked(f.dataset0.station.set_latlonident_into(insert));

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

template<> template<> void to::test<4>()
{
    // Test station query
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    Record query;

    // Iterate the station database
    auto_ptr<db::Cursor> cur = db->query_stations(query);
    wassert(actual(cur->remaining()) == 1);

    // There should be an item
    ensure(cur->next());
    ensure_equals(cur->get_lat(), 12.34560);
    ensure_equals(cur->get_lon(), 76.54320);
    ensure_equals(cur->get_ident(), (const char*)0);

    // Check that the result matches
    wassert(actual(cur).station_keys_match(f.dataset0.station));

    // There should be only one item
    ensure_equals(cur->remaining(), 0);

    ensure(!cur->next());
}

template<> template<> void to::test<5>()
{
    // Test querybest
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    //if (db->server_type == ORACLE || db->server_type == POSTGRES)
    //      return;

    // Prepare a query
    Record query;
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

template<> template<> void to::test<6>()
{
    // Test deletion
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    // 4 items to begin with
    Record query;
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
    wassert(actual(cur).data_context_matches(f.dataset0));

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
                wassert(actual(cur).data_var_matches(f.dataset0, last_code));
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

template<> template<> void to::test<7>()
{
    // Test datetime queries
    Record insert, query, result;

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

template<> template<> void to::test<8>()
{
    // Test QC
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    Record query, result;
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
    Record qc;
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

template<> template<> void to::test<9>()
{
    // Test station queries
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    Record query;
    query.set(DBA_KEY_REP_MEMO, "synop");

    auto_ptr<db::Cursor> cur = db->query_stations(query);
    ensure_equals(cur->remaining(), 1);

    ensure(cur->next());
    ensure(!cur->next());
}

template<> template<> void to::test<10>()
{
    // Test attributes
    OldDballeTestFixture f;

    // Insert a data record
    wruntest(f.dataset0.insert, *db);

    Record qc;
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

    db->attr_insert(WR_VAR(0, 1, 11), qc);

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

template<> template<> void to::test<11>()
{
    // Test longitude wrapping around
    OldDballeTestFixture f;

    // Insert a data record
    wruntest(f.dataset0.insert, *db);

    Record query;
    query.set(DBA_KEY_LATMIN, 10.0);
    query.set(DBA_KEY_LATMAX, 15.0);
    query.set(DBA_KEY_LONMIN, 70.0);
    query.set(DBA_KEY_LONMAX, -160.0);

    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 2);
    cur->discard_rest();
}

template<> template<> void to::test<12>()
{
    // Test numeric comparisons in ana_filter
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    Record query;
    query.set(DBA_KEY_REP_MEMO, "metar");
    query.set(DBA_KEY_VAR, "B01011");
    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);

    // Move the cursor to B01011
    cur->next();
    int context_id = cur->attr_reference_id();
    cur->discard_rest();

    // Insert new attributes about this report
    Record qc;
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


template<> template<> void to::test<13>()
{
    // Reproduce a querybest scenario which produced invalid SQL
    OldDballeTestFixture f;
    wruntest(populate_database, f);

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

template<> template<> void to::test<14>()
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

template<> template<> void to::test<15>()
{
    // Reproduce a query that generated invalid SQL on V6
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    // All DB
    Record query;
    query.set(DBA_KEY_LEVELTYPE1, 103);
    query.set(DBA_KEY_L1, 2000);
    db->query_stations(query);
}

template<> template<> void to::test<16>()
{
    // Test connect leaks
    Record insert;
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

template<> template<> void to::test<17>()
{
    // Test value update
    OldDballeTestFixture f;

    dballe::tests::TestRecord dataset = f.dataset0;
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

template<> template<> void to::test<18>()
{
    // Try a query checking all the steps
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    // Prepare a query
    Record query, result;
    query.set(DBA_KEY_LATMIN, 10.0);

    // Make the query
    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 4);

    ensure(cur->next());
    // remaining() should decrement
    ensure_equals(cur->remaining(), 3);
    // results should match what was inserted
    wassert(actual(cur).data_matches(f.dataset0));
    // just call to_record now, to check if in the next call old variables are removed
    cur->to_record(result);

    ensure(cur->next());
    ensure_equals(cur->remaining(), 2);
    wassert(actual(cur).data_matches(f.dataset0));

    // Variables from the previous to_record should be removed
    cur->to_record(result);
    wassert(actual(result.vars().size()) == 1u);


    ensure(cur->next());
    ensure_equals(cur->remaining(), 1);
    wassert(actual(cur).data_matches(f.dataset1));

    ensure(cur->next());
    ensure_equals(cur->remaining(), 0);
    wassert(actual(cur).data_matches(f.dataset1));

    // Now there should not be anything anymore
    ensure_equals(cur->remaining(), 0);
    ensure(!cur->next());
}

template<> template<> void to::test<19>()
{
    // Test double insert of station info

    //wassert(actual(*db).empty());
    wruntest(ds_st_navile.insert, *db, true);
    wruntest(ds_st_navile.insert, *db, true);

    // Query station data and ensure there is only one info (height)
    Record query;
    query.set_ana_context();
    auto_ptr<db::Cursor> cur = db->query_data(query);
    wassert(actual(cur->remaining()) == 1);
    cur->next();
    wassert(actual(cur).station_vars_match(ds_st_navile));
}

template<> template<> void to::test<20>()
{
    // Test double insert of station info
    dballe::tests::TestStation ds_st_navile_metar(ds_st_navile);
    ds_st_navile_metar.info["metar"] = ds_st_navile_metar.info["synop"];
    ds_st_navile_metar.info.erase("synop");
    wruntest(ds_st_navile.insert, *db, true);
    wruntest(ds_st_navile_metar.insert, *db, true);

    // Query station data and ensure there is only one info (height)
    Record query;
    query.set_ana_context();
    auto_ptr<db::Cursor> cur = db->query_data(query);
    wassert(actual(cur->remaining()) == 2);

    // Ensure that the network info is preserved
    // Use a sorted vector because while all DBs group by report, not all DBs
    // sort by report name.
    vector<string> reports;
    while (cur->next())
        reports.push_back(cur->get_rep_memo());
    std::sort(reports.begin(), reports.end());
    wassert(actual(reports[0]) == "metar");
    wassert(actual(reports[1]) == "synop");
}

template<> template<> void to::test<21>()
{
    // Test handling of values with undefined leveltype2 and l2
    OldDballeTestFixture f;

    // Insert with undef leveltype2 and l2
    dballe::tests::TestRecord dataset = f.dataset0;
    dataset.data.unset(WR_VAR(0, 1, 11));
    dataset.data.set(DBA_KEY_LEVELTYPE1, 44);
    dataset.data.set(DBA_KEY_L1, 55);
    dataset.data.unset(DBA_KEY_LEVELTYPE2);
    dataset.data.unset(DBA_KEY_L2);
    dataset.data.unset(DBA_KEY_P1);
    dataset.data.unset(DBA_KEY_P2);
    wruntest(dataset.insert, *db, true);

    // Query it back
    Record query;
    query.set(DBA_KEY_LEVELTYPE1, 44);
    query.set(DBA_KEY_L1, 55);

    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);

    ensure(cur->next());
    Record result;
    cur->to_record(result);

    ensure(result.key_peek(DBA_KEY_LEVELTYPE1) != NULL);
    ensure_equals(result[DBA_KEY_LEVELTYPE1].enqi(), 44);
    ensure(result.key_peek(DBA_KEY_L1) != NULL);
    ensure_equals(result[DBA_KEY_L1].enqi(), 55);
    ensure(result.key_peek(DBA_KEY_LEVELTYPE2) == NULL);
    ensure(result.key_peek(DBA_KEY_L2) == NULL);
    ensure(result.key_peek(DBA_KEY_PINDICATOR) != NULL);
    ensure_equals(result[DBA_KEY_PINDICATOR].enqi(), 20);
    ensure(result.key_peek(DBA_KEY_P1) == NULL);
    ensure(result.key_peek(DBA_KEY_P2) == NULL);

    ensure(!cur->next());
}

template<> template<> void to::test<22>()
{
    // Test handling of values with undefined leveltype2 and l2
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    // Query with undef leveltype2 and l2
    Record query;
    query.set(DBA_KEY_LEVELTYPE1, 10);
    query.set(DBA_KEY_L1, 11);

    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 4);
    cur->discard_rest();
}

template<> template<> void to::test<23>()
{
    // Query with an incorrect attr_filter
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    Record query;
    query.set(DBA_KEY_ATTR_FILTER, "B12001");

    try {
        db->query_data(query);
    } catch (error_consistency& e) {
        wassert(actual(e.what()).matches("B12001 is not a valid filter|cannot find any operator in filter 'B12001'"));
    }
}

template<> template<> void to::test<24>()
{
    // Test querying priomax together with query=best

    // Prepare the common parts of some data
    Record insert;
    insert.set(DBA_KEY_LAT, 1.0);
    insert.set(DBA_KEY_LON, 1.0);
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
    }

    // Query with querybest only
    {
        Record query;
        query.set(DBA_KEY_QUERY, "best");
        query.set_datetime(2009, 11, 11, 0, 0, 0);
        query.set(DBA_KEY_VAR, "B12101");
        auto_ptr<db::Cursor> cur = db->query_data(query);

        ensure_equals(cur->remaining(), 1);

        ensure(cur->next());
        Record result;
        cur->to_record(result);

        ensure(result.key_peek(DBA_KEY_REP_MEMO) != NULL);
        wassert(actual(result[DBA_KEY_REP_MEMO].enqc()) == "generic");

        cur->discard_rest();
    }

    // Query with querybest and priomax
    {
        Record query;
        query.set(DBA_KEY_PRIOMAX, 100);
        query.set(DBA_KEY_QUERY, "best");
        query.set_datetime(2009, 11, 11, 0, 0, 0);
        query.set(DBA_KEY_VAR, "B12101");
        auto_ptr<db::Cursor> cur = db->query_data(query);
        ensure_equals(cur->remaining(), 1);

        ensure(cur->next());
        Record result;
        cur->to_record(result);

        ensure(result.key_peek(DBA_KEY_REP_MEMO) != NULL);
        wassert(actual(result[DBA_KEY_REP_MEMO].enqc()) == "tempship");

        cur->discard_rest();
    }
}

template<> template<> void to::test<25>()
{
    // Ensure that rep_memo is set in the results
    OldDballeTestFixture f;
    wruntest(populate_database, f);

    Record res;
    Record rec;
    auto_ptr<db::Cursor> cur = db->query_data(rec);
    while (cur->next())
    {
        cur->to_record(res);
        ensure(res.key_peek_value(DBA_KEY_REP_MEMO) != 0);
    }
}

#if 0
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

template<> template<> void to::test<15>()
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
#endif

}

namespace {

tut::tg db_tests_mem_tg("db_misc_mem", MEM);
#ifdef HAVE_ODBC
tut::tg db_tests_v5_tg("db_misc_v5", V5);
tut::tg db_tests_v6_tg("db_misc_v6", V6);
#endif

}