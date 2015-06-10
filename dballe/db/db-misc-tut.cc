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
        ds_st_navile.info["synop"].set("B07030", 78); // Height
    }
};

template<typename OBJ, typename ...Args>
unsigned run_query_attrs(OBJ& db, core::Record& dest, Args... args)
{
    unsigned count = 0;
    db.query_attrs(args..., [&](unique_ptr<Var> var) { dest.set(move(var)); ++count; });
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
        ds.data.set("rep_memo", "synop");
        ds.data.set(Datetime(2013, 10, 16, 10));
        ds.set_var("temp_2m", 16.5, 50);
        wruntest(ds.insert, db);

        // Query and verify the station data
        {
            unique_ptr<db::Cursor> cur = db.query_stations(core::Query());
            wassert(actual(cur->remaining()) == 1);
            cur->next();
            wassert(actual(cur).station_vars_match(ds));
        }

        // Query and verify the measured data
        {
            unique_ptr<db::Cursor> cur = db.query_data(core::Query());
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
        core::Record insert(oldf.dataset0.data);
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
        insert.set("B01011", "DB-All.e?");
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
        core::Record insert(oldf.dataset0.data);
        wrunchecked(oldf.dataset0.station.set_latlonident_into(insert));

        // Insert the record twice
        wrunchecked(db.insert(insert, false, true));
        // This should fail, refusing to replace station info
        insert.set("B01011", "DB-All.e?");
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

        // Iterate the station database
        unique_ptr<db::Cursor> cur = db.query_stations(core::Query());

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
        core::Query query;
        query.set("latmin", 1000000);
        query.set("query", "best");

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
        core::Query query;
        unique_ptr<db::Cursor> cur = db.query_data(query);
        ensure_equals(cur->remaining(), 4);
        cur->discard_rest();

        query.clear();
        query.datetime_min = Datetime(1945, 4, 25, 8, 10);
        db.remove(query);

        // 2 remaining after remove
        query.clear();
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 2);
        cur->discard_rest();

        // Did it remove the right ones?
        query.clear();
        query.set("latmin", 1000000);
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
        core::Query query;
        core::Record insert, result;

        /* Prepare test data */
        core::Record base, a, b;

        base.set("lat", 12.0);
        base.set("lon", 48.0);
        base.set("mobile", 0);

        /*
           base.set(DBA_KEY_HEIGHT, 42);
           base.set(DBA_KEY_HEIGHTBARO, 234);
           base.set(DBA_KEY_BLOCK, 1);
           base.set(DBA_KEY_STATION, 52);
           base.set(DBA_KEY_NAME, "Cippo Lippo");
           */

        base.set(Level(1, 0, 1, 0));
        base.set(Trange(1, 0, 0));
        base.set("rep_memo", "synop");
        base.set("priority", 101);
        base.set("B01012", 500);
        base.set(Datetime(2006, 5, 15, 12, 30, 0));

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
        a.set("year", 2005);
        insert.add(a);
        db.insert(insert, false, true);

        insert.clear();
        b = base;
        b.set("year", 2006);
        insert.add(b);
        db.insert(insert, false, false);

        query.clear();
        query.set("yearmin", 2006);
        WANTRESULT(b);

        query.clear();
        query.set("yearmax", 2005);
        WANTRESULT(a);

        query.clear();
        query.set("year", 2006);
        WANTRESULT(b);


        /* Month */
        db.reset();

        insert.clear();
        a = base;
        a.set("year", 2006);
        a.set("month", 4);
        insert.add(a);
        db.insert(insert, false, true);

        insert.clear();
        b = base;
        b.set("year", 2006);
        b.set("month", 5);
        insert.add(b);
        db.insert(insert, false, false);

        query.clear();
        query.set("year", 2006);
        query.set("monthmin", 5);
        WANTRESULT(b);

        query.clear();
        query.set("year", 2006);
        query.set("monthmax", 4);
        WANTRESULT(a);

        query.clear();
        query.set("year", 2006);
        query.set("month", 5);
        WANTRESULT(b);

        /* Day */
        db.reset();

        insert.clear();
        a = base;
        a.set("year", 2006);
        a.set("month", 5);
        a.set("day", 2);
        insert.add(a);
        db.insert(insert, false, true);

        insert.clear();
        b = base;
        b.set("year", 2006);
        b.set("month", 5);
        b.set("day", 3);
        insert.add(b);
        db.insert(insert, false, false);

        query.clear();
        query.set("year", 2006);
        query.set("month", 5);
        query.set("daymin", 3);
        WANTRESULT(b);

        query.clear();
        query.set("year", 2006);
        query.set("month", 5);
        query.set("daymax", 2);
        WANTRESULT(a);

        query.clear();
        query.set("year", 2006);
        query.set("month", 5);
        query.set("day", 3);
        WANTRESULT(b);

        /* Hour */
        db.reset();

        insert.clear();
        a = base;
        a.set("year", 2006);
        a.set("month", 5);
        a.set("day", 3);
        a.set("hour", 12);
        insert.add(a);
        db.insert(insert, false, true);

        insert.clear();
        b = base;
        b.set("year", 2006);
        b.set("month", 5);
        b.set("day", 3);
        b.set("hour", 13);
        insert.add(b);
        db.insert(insert, false, false);

        query.clear();
        query.set("year", 2006);
        query.set("month", 5);
        query.set("day", 3);
        query.set("hourmin", 13);
        WANTRESULT(b);

        query.clear();
        query.set("year", 2006);
        query.set("month", 5);
        query.set("day", 3);
        query.set("hourmax", 12);
        WANTRESULT(a);

        query.clear();
        query.set("year", 2006);
        query.set("month", 5);
        query.set("day", 3);
        query.set("hour", 13);
        WANTRESULT(b);

        /* Minute */
        db.reset();

        insert.clear();
        a = base;
        a.set("year", 2006);
        a.set("month", 5);
        a.set("day", 3);
        a.set("hour", 12);
        a.set("min", 29);
        insert.add(a);
        db.insert(insert, false, true);

        insert.clear();
        b = base;
        b.set("year", 2006);
        b.set("month", 5);
        b.set("day", 3);
        b.set("hour", 12);
        b.set("min", 30);
        insert.add(b);
        db.insert(insert, false, false);

        query.clear();
        query.set("year", 2006);
        query.set("month", 5);
        query.set("day", 3);
        query.set("hour", 12);
        query.set("minumin", 30);
        WANTRESULT(b);

        query.clear();
        query.set("year", 2006);
        query.set("month", 5);
        query.set("day", 3);
        query.set("hour", 12);
        query.set("minumax", 29);
        WANTRESULT(a);

        query.clear();
        query.set("year", 2006);
        query.set("month", 5);
        query.set("day", 3);
        query.set("hour", 12);
        query.set("min", 30);
        WANTRESULT(b);
    }),
    Test("attrs", [](Fixture& f) {
        // Test QC
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        core::Query query;
        core::Record result;
        query.set("latmin", 1000000);
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
        core::Record qc;
        qc.set("B33002", 2);
        qc.set("B33003", 5);
        qc.set("B33005", 33);
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

        unique_ptr<db::Cursor> cur = db.query_stations(*query_from_string("rep_memo=synop"));
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

        core::Record qc;
        qc.set("B01007",  1);
        qc.set("B02048",  2);
        qc.set("B05040",  3);
        qc.set("B05041",  4);
        qc.set("B05043",  5);
        qc.set("B33032",  6);
        qc.set("B07024",  7);
        qc.set("B05021",  8);
        qc.set("B07025",  9);
        qc.set("B05022", 10);

        db.attr_insert(WR_VAR(0, 1, 11), qc);

        // Query back the B01011 variable to read the attr reference id
        unique_ptr<db::Cursor> cur = db.query_data(*query_from_string("var=B01011"));
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

        unique_ptr<db::Cursor> cur = db.query_data(*query_from_string("latmin=10.0, latmax=15.0, lonmin=70.0, lonmax=-160.0"));
        ensure_equals(cur->remaining(), 2);
        cur->discard_rest();
    }),
    Test("query_ana_filter", [](Fixture& f) {
        // Test numeric comparisons in ana_filter
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        unique_ptr<db::Cursor> cur = db.query_data(*query_from_string("rep_memo=metar, var=B01011"));
        ensure_equals(cur->remaining(), 1);

        // Move the cursor to B01011
        cur->next();
        int context_id = cur->attr_reference_id();
        cur->discard_rest();

        // Insert new attributes about this report
        core::Record qc;
        qc.set("B01001", 50);
        qc.set("B01008", "50");
        db.attr_insert(context_id, WR_VAR(0, 1, 11), qc);

        // Try queries filtered by numeric attributes
        cur = db.query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01001=50"));
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        cur = db.query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01001<=50"));
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        cur = db.query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01001<51"));
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        cur = db.query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01001<8"));
        ensure_equals(cur->remaining(), 0);
        cur->discard_rest();

        // Try queries filtered by string attributes
        cur = db.query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01008=50"));
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        cur = db.query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01008<=50"));
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        cur = db.query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01008<8"));
        ensure_equals(cur->remaining(), 1);
        cur->discard_rest();

        cur = db.query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01008<100"));
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

        core::Query q;
        q.datetime_min = q.datetime_max = Datetime(1000, 1, 1, 0, 0, 0);
        q.query = "best";
        unique_ptr<db::Cursor> cur = db.query_data(q);
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
        unique_ptr<db::Cursor> cur = db.query_data(*query_from_string("var=B12101, query=best"));
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
        db.query_stations(*query_from_string("leveltype1=103, l1=2000"));
    }),
    Test("fd_leaks", [](Fixture& f) {
        // Test connect leaks
        core::Record insert;
        insert.set_ana_context();
        insert.set("lat", 12.34560);
        insert.set("lon", 76.54320);
        insert.set("mobile", 0);
        insert.set("rep_memo", "synop");
        insert.set("B07030", 42.0); // Height

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
        core::Record& attrs = dataset.attrs[WR_VAR(0, 1, 12)];
        attrs.set("B33007", 50);
        wruntest(dataset.insert, db);

        core::Query q;
        q.coords_min = q.coords_max = Coords(12.34560, 76.54320);
        q.datetime_min = q.datetime_max = Datetime(1945, 4, 25, 8, 0, 0);
        q.rep_memo = "synop";
        q.level = Level(10, 11, 15, 22);
        q.trange = Trange(20, 111, 122);
        q.varcodes.insert(WR_VAR(0, 1, 12));

        // Query the initial value
        unique_ptr<db::Cursor> cur = db.query_data(q);
        ensure_equals(cur->remaining(), 1);
        cur->next();
        int ana_id = cur->get_station_id();
        wreport::Var var = cur->get_var();
        ensure_equals(var.enqi(), 300);

        // Query the attributes and check that they are there
        core::Record qattrs;
        wassert(actual(run_query_attrs(*cur, qattrs)) == 1);
        wassert(actual(qattrs.enq("B33007", MISSING_INT)) == 50);

        // Update it
        core::Record update;
        update.set("ana_id", ana_id);
        update.set("rep_memo", "synop");
        int dt[6];
        update.set(q.datetime_min);
        update.set(q.level);
        update.set(q.trange);
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
        wassert(actual(qattrs.enq("B33007", MISSING_INT)) == 50);
    }),
    Test("query_stepbystep", [](Fixture& f) {
        auto& db = *f.db;
        // Try a query checking all the steps
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        // Make the query
        unique_ptr<db::Cursor> cur = db.query_data(*query_from_string("latmin=10.0"));
        ensure_equals(cur->remaining(), 4);

        ensure(cur->next());
        // remaining() should decrement
        ensure_equals(cur->remaining(), 3);
        // results should match what was inserted
        wassert(actual(cur).data_matches(oldf.dataset0));
        // just call to_record now, to check if in the next call old variables are removed
        core::Record result;
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
        core::Query query;
        query.query_station_vars = true;
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
        core::Query query;
        query.query_station_vars = true;
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
        dataset.data.unset("B01011");
        dataset.data.set(Level(44, 55));
        dataset.data.unset("p1");
        dataset.data.unset("p2");
        wruntest(dataset.insert, db, true);

        // Query it back
        unique_ptr<db::Cursor> cur = db.query_data(*query_from_string("leveltype1=44, l1=55"));
        ensure_equals(cur->remaining(), 1);

        ensure(cur->next());
        core::Record result;
        cur->to_record(result);
        wassert(actual(result.get_level()) == Level(44, 55));
        wassert(actual(result.get_trange()) == Trange(20));

        ensure(!cur->next());
    }),
    Test("query_undefined_level2", [](Fixture& f) {
        // Test handling of values with undefined leveltype2 and l2
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        // Query with undef leveltype2 and l2
        unique_ptr<db::Cursor> cur = db.query_data(*query_from_string("leveltype1=10, l1=11"));
        ensure_equals(cur->remaining(), 4);
        cur->discard_rest();
    }),
    Test("query_bad_attrfilter", [](Fixture& f) {
        // Query with an incorrect attr_filter
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        try {
            db.query_data(*query_from_string("attr_filter=B12001"));
        } catch (error_consistency& e) {
            wassert(actual(e.what()).matches("B12001 is not a valid filter|cannot find any operator in filter 'B12001'"));
        }
    }),
    Test("query_best_priomax", [](Fixture& f) {
        // Test querying priomax together with query=best
        auto& db = *f.db;

        // Prepare the common parts of some data
        core::Record insert;
        insert.set("lat", 1.0);
        insert.set("lon", 1.0);
        insert.set(Level(1, 0));
        insert.set(Trange(254, 0, 0));
        insert.set(Datetime(2009, 11, 11, 0, 0, 0));

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
            insert.set("rep_memo", *i);
            insert.set("B12101", (int)(i - rep_memos));
            db.insert(insert, false, true);
        }

        // Query with querybest only
        {
            core::Query query;
            query.set("query", "best");
            query.datetime_min = query.datetime_max = Datetime(2009, 11, 11, 0, 0, 0);
            query.varcodes.insert(WR_VAR(0, 12, 101));
            unique_ptr<db::Cursor> cur = db.query_data(query);

            ensure_equals(cur->remaining(), 1);

            ensure(cur->next());
            core::Record result;
            cur->to_record(result);

            ensure(result.get("rep_memo") != NULL);
            wassert(actual(result.enq("rep_memo", "")) == "generic");

            cur->discard_rest();
        }

        // Query with querybest and priomax
        {
            core::Query query;
            query.set("priomax", 100);
            query.set("query", "best");
            query.datetime_min = query.datetime_max = Datetime(2009, 11, 11, 0, 0, 0);
            query.varcodes.insert(WR_VAR(0, 12, 101));
            unique_ptr<db::Cursor> cur = db.query_data(query);
            ensure_equals(cur->remaining(), 1);

            ensure(cur->next());
            core::Record result;
            cur->to_record(result);

            ensure(result.get("rep_memo") != NULL);
            wassert(actual(result.enq("rep_memo", "")) == "tempship");

            cur->discard_rest();
        }
    }),
    Test("query_repmemo_in_results", [](Fixture& f) {
        // Ensure that rep_memo is set in the results
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        core::Record res;
        unique_ptr<db::Cursor> cur = db.query_data(core::Query());
        while (cur->next())
        {
            cur->to_record(res);
            wassert(actual(res["rep_memo"].isset()).istrue());
        }
    }),
};

test_group tg1("db_misc_mem", nullptr, db::MEM, tests);
test_group tg2("db_misc_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("db_misc_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_misc_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_misc_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
