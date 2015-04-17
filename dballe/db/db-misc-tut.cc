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
#include "db/mem/db.h"
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
    TestStation ds_st_navile;

    Fixture()
    {
        ds_st_navile.lat = 44.5008;
        ds_st_navile.lon = 11.3288;
        ds_st_navile.info["synop"].set(WR_VAR(0, 7, 30), 78); // Height
    }
};

template<typename OBJ, typename ...Args>
unsigned run_query_attrs(OBJ& db, Record& dest, Args... args)
{
    unsigned count = 0;
    db.query_attrs(args..., [&](unique_ptr<Var> var) { dest.add(move(var)); ++count; });
    return count;
}


typedef dballe::tests::db_test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    Test("insert", [](Fixture& f) {
        // Test a simple insert round trip
        auto& db = *f.db;

        // Insert some data
        dballe::tests::TestRecord ds;
        ds.station = f.ds_st_navile;
        ds.data.set(DBA_KEY_REP_MEMO, "synop");
        ds.data.set_datetime(2013, 10, 16, 10);
        ds.set_var("temp_2m", 16.5, 50);
        wruntest(ds.insert, db);

        Record query;

        // Query and verify the station data
        {
            unique_ptr<db::Cursor> cur = db.query_stations(query);
            wassert(actual(cur->remaining()) == 1);
            cur->next();
            wassert(actual(cur).station_vars_match(ds));
        }

        // Query and verify the measured data
        {
            unique_ptr<db::Cursor> cur = db.query_data(query);
            wassert(actual(cur->remaining()) == 1);
            cur->next();
            wassert(actual(cur).data_context_matches(ds));
            wassert(actual(cur).data_var_matches(ds, WR_VAR(0, 12, 101)));
        }

        // Query and verify attributes
        // TODO
    }),
    Test("insert_perms", [](Fixture& f) {
        // Test insert
        auto& db = *f.db;
        OldDballeTestFixture oldf;

        // Prepare a valid record to insert
        Record insert(oldf.dataset0.data);
        wrunchecked(oldf.dataset0.station.set_latlonident_into(insert));

        // Check if adding a nonexisting station when not allowed causes an error
        try {
            db.insert(insert, false, false);
            ensure(false);
        } catch (error_consistency& e) {
            wassert(actual(e.what()).contains("insert a station entry when it is forbidden"));
        } catch (error_notfound& e) {
            wassert(actual(e.what()).contains("station not found"));
        }

        // Insert the record
        wrunchecked(db.insert(insert, false, true));
        // Check if duplicate updates are allowed by insert
        wrunchecked(db.insert(insert, true, false));
        // Check if overwrites are trapped by insert_new
        insert.set(WR_VAR(0, 1, 11), "DB-All.e?");
        try {
            db.insert(insert, false, false);
            ensure(false);
        } catch (wreport::error& e) {
            wassert(actual(e.what()).matches("refusing to overwrite existing data|cannot replace an existing value|Duplicate entry"));
        }
    }),
    Test("insert_twice", [](Fixture& f) {
        // Test double station insert
        auto& db = *f.db;
        OldDballeTestFixture oldf;

        // Prepare a valid record to insert
        Record insert(oldf.dataset0.data);
        wrunchecked(oldf.dataset0.station.set_latlonident_into(insert));

        // Insert the record twice
        wrunchecked(db.insert(insert, false, true));
        // This should fail, refusing to replace station info
        insert.set(WR_VAR(0, 1, 11), "DB-All.e?");
        try {
            db.insert(insert, false, true);
            ensure(false);
        } catch (wreport::error& e) {
            wassert(actual(e.what()).matches("refusing to overwrite existing data|cannot replace an existing value|Duplicate entry"));
        }
    }),
    Test("query_station", [](Fixture& f) {
        // Test station query
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        Record query;

        // Iterate the station database
        unique_ptr<db::Cursor> cur = db.query_stations(query);

        if (dynamic_cast<db::mem::DB*>(f.db))
        {
            // Memdb has one station entry per (lat, lon, ident, network)
            wassert(actual(cur->remaining()) == 2);

            // There should be an item
            wassert(actual(cur->next()).istrue());
            wassert(actual(cur->get_lat()) == 12.34560);
            wassert(actual(cur->get_lon()) == 76.54320);
            wassert(actual(cur->get_rep_memo()) == "metar");
            wassert(actual((void*)cur->get_ident()) == (void*)0);

            // Check that the result matches
            wassert(actual(cur).station_keys_match(oldf.dataset0.station));

            wassert(actual(cur->next()).istrue());
            wassert(actual(cur->get_lat()) == 12.34560);
            wassert(actual(cur->get_lon()) == 76.54320);
            wassert(actual(cur->get_rep_memo()) == "synop");
            wassert(actual((void*)cur->get_ident()) == (void*)0);

            // Check that the result matches
            wassert(actual(cur).station_keys_match(oldf.dataset0.station));
        } else {
            // V5 and V6 have one station entry (lat, lon, ident)
            wassert(actual(cur->remaining()) == 1);

            // There should be an item
            wassert(actual(cur->next()).istrue());
            wassert(actual(cur->get_lat()) == 12.34560);
            wassert(actual(cur->get_lon()) == 76.54320);
            wassert(actual((void*)cur->get_ident()) == (void*)0);

            // Check that the result matches
            wassert(actual(cur).station_keys_match(oldf.dataset0.station));

            // There should be only one item
        }
        wassert(actual(cur->remaining()) == 0);
        wassert(actual(cur->next()).isfalse());
    }),
    Test("query_best", [](Fixture& f) {
        // Test querybest
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        //if (db.server_type == ORACLE || db.server_type == POSTGRES)
        //      return;

        // Prepare a query
        Record query;
        query.set(DBA_KEY_LATMIN, 1000000);
        query.set(DBA_KEY_QUERY, "best");

        // Make the query
        unique_ptr<db::Cursor> cur = db.query_data(query);

        wassert(actual(cur->remaining()) == 4);

        // There should be four items
        wassert(actual(cur->next()).istrue());
        wassert(actual(cur->get_lat()) == 12.34560);
        wassert(actual(cur->get_lon()) == 76.54320);
        wassert(actual((void*)cur->get_ident()) == (void*)0);
        wassert(actual((void*)cur->get_rep_memo()).istrue());
        wassert(actual(cur->get_rep_memo()) == "synop");
        wassert(actual(cur->get_level()) == Level(10, 11, 15, 22));
        wassert(actual(cur->get_trange()) == Trange(20, 111, 122));
        wassert(actual(cur->get_varcode()) == WR_VAR(0, 1, 11));
        wassert(actual(cur->get_var().code()) == WR_VAR(0, 1, 11));
        wassert(actual(cur->remaining()) == 3);
        wassert(actual(cur->next()).istrue());
        wassert(actual(cur->remaining()) == 2);
        wassert(actual(cur->next()).istrue());
        wassert(actual(cur->remaining()) == 1);
        wassert(actual(cur->next()).istrue());
        wassert(actual(cur->remaining()) == 0);

        // Now there should not be anything anymore
        ensure(!cur->next());
    }),
    Test("delete", [](Fixture& f) {
        // Test deletion
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        // 4 items to begin with
        Record query;
        unique_ptr<db::Cursor> cur = db.query_data(query);
        ensure_equals(cur->remaining(), 4);
        cur->discard_rest();

        query.clear();
        query.set(DBA_KEY_YEARMIN, 1945);
        query.set(DBA_KEY_MONTHMIN, 4);
        query.set(DBA_KEY_DAYMIN, 25);
        query.set(DBA_KEY_HOURMIN, 8);
        query.set(DBA_KEY_MINUMIN, 10);
        db.remove(query);

        // 2 remaining after remove
        query.clear();
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 2);
        cur->discard_rest();

        // Did it remove the right ones?
        query.clear();
        query.set(DBA_KEY_LATMIN, 1000000);
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 2);
        ensure(cur->next());
        wassert(actual(cur).data_context_matches(oldf.dataset0));

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
                    wassert(actual(cur).data_var_matches(oldf.dataset0, last_code));
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
    }),
    Test("query_datetime", [](Fixture& f) {
        // Test datetime queries
        auto& db = *f.db;
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
        unique_ptr<db::Cursor> cur = db.query_data(query); \
        ensure_equals(cur->remaining(), 1); \
        ensure(cur->next()); \
        cur->to_record(result); \
        ensure_equals(cur->remaining(), 0); \
        ensure_varcode_equals(result.vars()[0]->code(), WR_VAR(0, 1, 12)); \
        ensure(result.contains(ab)); \
        cur->discard_rest(); \
    } while(0)

        /* Year */
        db.reset();

        insert.clear();
        a = base;
        a.set(DBA_KEY_YEAR, 2005);
        insert.add(a);
        db.insert(insert, false, true);

        insert.clear();
        b = base;
        b.set(DBA_KEY_YEAR, 2006);
        insert.add(b);
        db.insert(insert, false, false);

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
        db.reset();

        insert.clear();
        a = base;
        a.set(DBA_KEY_YEAR, 2006);
        a.set(DBA_KEY_MONTH, 4);
        insert.add(a);
        db.insert(insert, false, true);

        insert.clear();
        b = base;
        b.set(DBA_KEY_YEAR, 2006);
        b.set(DBA_KEY_MONTH, 5);
        insert.add(b);
        db.insert(insert, false, false);

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
        db.reset();

        insert.clear();
        a = base;
        a.set(DBA_KEY_YEAR, 2006);
        a.set(DBA_KEY_MONTH, 5);
        a.set(DBA_KEY_DAY, 2);
        insert.add(a);
        db.insert(insert, false, true);

        insert.clear();
        b = base;
        b.set(DBA_KEY_YEAR, 2006);
        b.set(DBA_KEY_MONTH, 5);
        b.set(DBA_KEY_DAY, 3);
        insert.add(b);
        db.insert(insert, false, false);

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
        db.reset();

        insert.clear();
        a = base;
        a.set(DBA_KEY_YEAR, 2006);
        a.set(DBA_KEY_MONTH, 5);
        a.set(DBA_KEY_DAY, 3);
        a.set(DBA_KEY_HOUR, 12);
        insert.add(a);
        db.insert(insert, false, true);

        insert.clear();
        b = base;
        b.set(DBA_KEY_YEAR, 2006);
        b.set(DBA_KEY_MONTH, 5);
        b.set(DBA_KEY_DAY, 3);
        b.set(DBA_KEY_HOUR, 13);
        insert.add(b);
        db.insert(insert, false, false);

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
        db.reset();

        insert.clear();
        a = base;
        a.set(DBA_KEY_YEAR, 2006);
        a.set(DBA_KEY_MONTH, 5);
        a.set(DBA_KEY_DAY, 3);
        a.set(DBA_KEY_HOUR, 12);
        a.set(DBA_KEY_MIN, 29);
        insert.add(a);
        db.insert(insert, false, true);

        insert.clear();
        b = base;
        b.set(DBA_KEY_YEAR, 2006);
        b.set(DBA_KEY_MONTH, 5);
        b.set(DBA_KEY_DAY, 3);
        b.set(DBA_KEY_HOUR, 12);
        b.set(DBA_KEY_MIN, 30);
        insert.add(b);
        db.insert(insert, false, false);

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
    }),
    Test("attrs", [](Fixture& f) {
        // Test QC
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        Record query, result;
        query.set(DBA_KEY_LATMIN, 1000000);
        unique_ptr<db::Cursor> cur = db.query_data(query);

        // Move the cursor to B01011
        int context_id;
        bool found = false;
        while (cur->next())
        {
            cur->to_record(result);
            if (result.vars()[0]->code() == WR_VAR(0, 1, 11))
            {
                context_id = cur->attr_reference_id();
                cur->discard_rest();
                found = true;
                break;
            }
        }
        ensure(found);

        // Insert new attributes about this report
        Record qc;
        qc.set(WR_VAR(0, 33, 2), 2);
        qc.set(WR_VAR(0, 33, 3), 5);
        qc.set(WR_VAR(0, 33, 5), 33);
        db.attr_insert(context_id, WR_VAR(0, 1, 11), qc);

        // Query back the data
        qc.clear();
        wassert(actual(run_query_attrs(db, qc, context_id, WR_VAR(0, 1, 11))) == 3);

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
        vector<Varcode> codes;
        codes.push_back(WR_VAR(0, 33, 2));
        codes.push_back(WR_VAR(0, 33, 5));
        db.attr_remove(context_id, WR_VAR(0, 1, 11), codes);

        // Deleting non-existing items should not fail.  Also try creating a
        // query with just one item
        codes.clear();
        codes.push_back(WR_VAR(0, 33, 2));
        db.attr_remove(context_id, WR_VAR(0, 1, 11), codes);

        /* Query back the data */
        qc.clear();
        wassert(actual(run_query_attrs(db, qc, context_id, WR_VAR(0, 1, 11))) == 1);

        ensure(qc.var_peek(WR_VAR(0, 33, 2)) == NULL);
        ensure(qc.var_peek(WR_VAR(0, 33, 5)) == NULL);
        attr = qc.var_peek(WR_VAR(0, 33, 3));
        ensure(attr != NULL);
        ensure_equals(attr->enqi(), 5);
        /*dba_error_remove_callback(DBA_ERR_NONE, crash, 0);*/
    }),
    Test("query_station", [](Fixture& f) {
        // Test station queries
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        Record query;
        query.set(DBA_KEY_REP_MEMO, "synop");

        unique_ptr<db::Cursor> cur = db.query_stations(query);
        ensure_equals(cur->remaining(), 1);

        ensure(cur->next());
        ensure(!cur->next());
    }),
    Test("attrs1", [](Fixture& f) {
        // Test attributes
        auto& db = *f.db;
        OldDballeTestFixture oldf;

        // Insert a data record
        wruntest(oldf.dataset0.insert, db);

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

        db.attr_insert(WR_VAR(0, 1, 11), qc);

        // Query back the B01011 variable to read the attr reference id
        Record query;
        query.set(DBA_KEY_VAR, "B01011");
        unique_ptr<db::Cursor> cur = db.query_data(query);
        ensure_equals(cur->remaining(), 1);
        cur->next();
        int attr_id = cur->attr_reference_id();
        cur->discard_rest();

        qc.clear();
        wassert(actual(run_query_attrs(db, qc, attr_id, WR_VAR(0, 1, 11))) == 10);

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
    }),
    Test("longitude_wrap", [](Fixture& f) {
        // Test longitude wrapping around
        auto& db = *f.db;
        OldDballeTestFixture oldf;

        // Insert a data record
        wruntest(oldf.dataset0.insert, db);

        Record query;
        query.set(DBA_KEY_LATMIN, 10.0);
        query.set(DBA_KEY_LATMAX, 15.0);
        query.set(DBA_KEY_LONMIN, 70.0);
        query.set(DBA_KEY_LONMAX, -160.0);

        unique_ptr<db::Cursor> cur = db.query_data(query);
        ensure_equals(cur->remaining(), 2);
        cur->discard_rest();
    }),
    Test("query_ana_filter", [](Fixture& f) {
        // Test numeric comparisons in ana_filter
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        Record query;
        query.set(DBA_KEY_REP_MEMO, "metar");
        query.set(DBA_KEY_VAR, "B01011");
        unique_ptr<db::Cursor> cur = db.query_data(query);
        ensure_equals(cur->remaining(), 1);

        // Move the cursor to B01011
        cur->next();
        int context_id = cur->attr_reference_id();
        cur->discard_rest();

        // Insert new attributes about this report
        Record qc;
        qc.set(WR_VAR(0, 1, 1), 50);
        qc.set(WR_VAR(0, 1, 8), "50");
        db.attr_insert(context_id, WR_VAR(0, 1, 11), qc);

        // Try queries filtered by numeric attributes
        query.clear();
        query.set(DBA_KEY_REP_MEMO, "metar");
        query.set(DBA_KEY_VAR, "B01011");

        query.set(DBA_KEY_ATTR_FILTER, "B01001=50");
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        query.set(DBA_KEY_ATTR_FILTER, "B01001<=50");
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        query.set(DBA_KEY_ATTR_FILTER, "B01001<51");
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        query.set(DBA_KEY_ATTR_FILTER, "B01001<8");
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 0);
        cur->discard_rest();

        // Try queries filtered by string attributes
        query.clear();
        query.set(DBA_KEY_REP_MEMO, "metar");
        query.set(DBA_KEY_VAR, "B01011");

        query.set(DBA_KEY_ATTR_FILTER, "B01008=50");
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        query.set(DBA_KEY_ATTR_FILTER, "B01008<=50");
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        query.set(DBA_KEY_ATTR_FILTER, "B01008<8");
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        query.set(DBA_KEY_ATTR_FILTER, "B01008<100");
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 0);
        cur->discard_rest();
    }),
    Test("query_station_best", [](Fixture& f) {
#warning BEST queries of station values are not yet implemented for memdb
        if (dynamic_cast<db::mem::DB*>(f.db))
            return;
        auto& db = *f.db;

        // Reproduce a querybest scenario which produced invalid SQL
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        Record rec;
        rec.set(DBA_KEY_YEAR, 1000);
        rec.set(DBA_KEY_MONTH, 1);
        rec.set(DBA_KEY_DAY, 1);
        rec.set(DBA_KEY_HOUR, 0);
        rec.set(DBA_KEY_MIN, 0);
        rec.set(DBA_KEY_QUERY, "best");
        unique_ptr<db::Cursor> cur = db.query_data(rec);
        while (cur->next())
        {
        }
    }),
    Test("query_best_bug1", [](Fixture& f) {
        auto& db = *f.db;
        // Reproduce a querybest scenario which produced always the same data record

        // Import lots
        const char** files = dballe::tests::bufr_files;
        for (int i = 0; files[i] != NULL; i++)
        {
            std::unique_ptr<Msgs> inmsgs = read_msgs(files[i], BUFR);
            Msg& msg = *(*inmsgs)[0];
            wrunchecked(db.import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_OVERWRITE));
        }

        // Query all with best
        Record rec;
        rec.set(DBA_KEY_VAR,   "B12101");
        rec.set(DBA_KEY_QUERY, "best");
        unique_ptr<db::Cursor> cur = db.query_data(rec);
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
    }),
    Test("query_invalid_sql", [](Fixture& f) {
        auto& db = *f.db;
        // Reproduce a query that generated invalid SQL on V6
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        // All DB
        Record query;
        query.set(DBA_KEY_LEVELTYPE1, 103);
        query.set(DBA_KEY_L1, 2000);
        db.query_stations(query);
    }),
    Test("fd_leaks", [](Fixture& f) {
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
            std::unique_ptr<DB> db = f.create_db();
            wrunchecked(db->insert(insert, true, true));
        }
    }),
    Test("update", [](Fixture& f) {
        auto& db = *f.db;
        // Test value update
        OldDballeTestFixture oldf;

        dballe::tests::TestRecord dataset = oldf.dataset0;
        Record& attrs = dataset.attrs[WR_VAR(0, 1, 12)];
        attrs.set(WR_VAR(0, 33, 7), 50);
        wruntest(dataset.insert, db);

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
        unique_ptr<db::Cursor> cur = db.query_data(q);
        ensure_equals(cur->remaining(), 1);
        cur->next();
        int ana_id = cur->get_station_id();
        wreport::Var var = cur->get_var();
        ensure_equals(var.enqi(), 300);

        // Query the attributes and check that they are there
        Record qattrs;
        wassert(actual(run_query_attrs(*cur, qattrs)) == 1);
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
        db.insert(update, true, false);

        // Query again
        cur = db.query_data(q);
        ensure_equals(cur->remaining(), 1);
        cur->next();
        var = cur->get_var();
        ensure_equals(var.enqi(), 200);

        qattrs.clear();
        wassert(actual(run_query_attrs(*cur, qattrs)) == 1);
        ensure_equals(qattrs.get(WR_VAR(0, 33, 7), MISSING_INT), 50);
    }),
    Test("query_stepbystep", [](Fixture& f) {
        auto& db = *f.db;
        // Try a query checking all the steps
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        // Prepare a query
        Record query, result;
        query.set(DBA_KEY_LATMIN, 10.0);

        // Make the query
        unique_ptr<db::Cursor> cur = db.query_data(query);
        ensure_equals(cur->remaining(), 4);

        ensure(cur->next());
        // remaining() should decrement
        ensure_equals(cur->remaining(), 3);
        // results should match what was inserted
        wassert(actual(cur).data_matches(oldf.dataset0));
        // just call to_record now, to check if in the next call old variables are removed
        cur->to_record(result);

        ensure(cur->next());
        ensure_equals(cur->remaining(), 2);
        wassert(actual(cur).data_matches(oldf.dataset0));

        // Variables from the previous to_record should be removed
        cur->to_record(result);
        wassert(actual(result.vars().size()) == 1u);


        ensure(cur->next());
        ensure_equals(cur->remaining(), 1);
        wassert(actual(cur).data_matches(oldf.dataset1));

        ensure(cur->next());
        ensure_equals(cur->remaining(), 0);
        wassert(actual(cur).data_matches(oldf.dataset1));

        // Now there should not be anything anymore
        ensure_equals(cur->remaining(), 0);
        ensure(!cur->next());
    }),
    Test("insert_stationinfo_twice", [](Fixture& f) {
        // Test double insert of station info
        auto& db = *f.db;

        //wassert(actual(f.db).empty());
        wruntest(f.ds_st_navile.insert, db, true);
        wruntest(f.ds_st_navile.insert, db, true);

        // Query station data and ensure there is only one info (height)
        Record query;
        query.set_ana_context();
        unique_ptr<db::Cursor> cur = db.query_data(query);
        wassert(actual(cur->remaining()) == 1);
        cur->next();
        wassert(actual(cur).station_vars_match(f.ds_st_navile));
    }),
    Test("insert_stationinfo_twice1", [](Fixture& f) {
        // Test double insert of station info
        auto& db = *f.db;
        dballe::tests::TestStation ds_st_navile_metar(f.ds_st_navile);
        ds_st_navile_metar.info["metar"] = ds_st_navile_metar.info["synop"];
        ds_st_navile_metar.info.erase("synop");
        wruntest(f.ds_st_navile.insert, db, true);
        wruntest(ds_st_navile_metar.insert, db, true);

        // Query station data and ensure there is only one info (height)
        Record query;
        query.set_ana_context();
        unique_ptr<db::Cursor> cur = db.query_data(query);
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
    }),
    Test("insert_undefined_level2", [](Fixture& f) {
        // Test handling of values with undefined leveltype2 and l2
        auto& db = *f.db;
        OldDballeTestFixture oldf;

        // Insert with undef leveltype2 and l2
        dballe::tests::TestRecord dataset = oldf.dataset0;
        dataset.data.unset(WR_VAR(0, 1, 11));
        dataset.data.set(DBA_KEY_LEVELTYPE1, 44);
        dataset.data.set(DBA_KEY_L1, 55);
        dataset.data.unset(DBA_KEY_LEVELTYPE2);
        dataset.data.unset(DBA_KEY_L2);
        dataset.data.unset(DBA_KEY_P1);
        dataset.data.unset(DBA_KEY_P2);
        wruntest(dataset.insert, db, true);

        // Query it back
        Record query;
        query.set(DBA_KEY_LEVELTYPE1, 44);
        query.set(DBA_KEY_L1, 55);

        unique_ptr<db::Cursor> cur = db.query_data(query);
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
    }),
    Test("query_undefined_level2", [](Fixture& f) {
        // Test handling of values with undefined leveltype2 and l2
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        // Query with undef leveltype2 and l2
        Record query;
        query.set(DBA_KEY_LEVELTYPE1, 10);
        query.set(DBA_KEY_L1, 11);

        unique_ptr<db::Cursor> cur = db.query_data(query);
        ensure_equals(cur->remaining(), 4);
        cur->discard_rest();
    }),
    Test("query_bad_attrfilter", [](Fixture& f) {
        // Query with an incorrect attr_filter
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        Record query;
        query.set(DBA_KEY_ATTR_FILTER, "B12001");

        try {
            db.query_data(query);
        } catch (error_consistency& e) {
            wassert(actual(e.what()).matches("B12001 is not a valid filter|cannot find any operator in filter 'B12001'"));
        }
    }),
    Test("query_best_priomax", [](Fixture& f) {
        // Test querying priomax together with query=best
        auto& db = *f.db;

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
            db.insert(insert, false, true);
        }

        // Query with querybest only
        {
            Record query;
            query.set(DBA_KEY_QUERY, "best");
            query.set_datetime(2009, 11, 11, 0, 0, 0);
            query.set(DBA_KEY_VAR, "B12101");
            unique_ptr<db::Cursor> cur = db.query_data(query);

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
            unique_ptr<db::Cursor> cur = db.query_data(query);
            ensure_equals(cur->remaining(), 1);

            ensure(cur->next());
            Record result;
            cur->to_record(result);

            ensure(result.key_peek(DBA_KEY_REP_MEMO) != NULL);
            wassert(actual(result[DBA_KEY_REP_MEMO].enqc()) == "tempship");

            cur->discard_rest();
        }
    }),
    Test("query_repmemo_in_results", [](Fixture& f) {
        // Ensure that rep_memo is set in the results
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        Record res;
        Record rec;
        unique_ptr<db::Cursor> cur = db.query_data(rec);
        while (cur->next())
        {
            cur->to_record(res);
            ensure(res.key_peek_value(DBA_KEY_REP_MEMO) != 0);
        }
    }),
};

test_group tg1("db_misc_mem", nullptr, db::MEM, tests);
test_group tg2("db_misc_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg3("db_misc_v5_odbc", "ODBC", db::V5, tests);
test_group tg4("db_misc_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_misc_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_misc_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
