#include "config.h"
#include "db/tests.h"
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
    StationValues ds_st_navile;

    Fixture()
    {
        ds_st_navile.info.coords = Coords(44.5008, 11.3288);
        ds_st_navile.info.report = "synop";
        ds_st_navile.values.set("B07030", 78); // Height
    }

    void reset()
    {
        dballe::tests::DBFixture::reset();
        ds_st_navile.clear_ids();
    }
};

template<typename ...Args>
unsigned run_query_attrs(DB& db, Values& dest, Args... args)
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
        wrunchecked(db.insert_station_data(f.ds_st_navile, true, true));
        DataValues vals;
        vals.info = f.ds_st_navile.info;
        vals.info.datetime = Datetime(2013, 10, 16, 10);
        vals.values.set(WR_VAR(0, 12, 101), 16.5);
        wrunchecked(db.insert_data(vals, true, true));
        Values attrs;
        attrs.set("B33007", 50);
        wrunchecked(db.attr_insert(vals.values[WR_VAR(0, 12, 101)].data_id, WR_VAR(0, 12, 101), attrs));

        // Query and verify the station data
        {
            auto cur = db.query_stations(core::Query());
            wassert(actual(cur->remaining()) == 1);
            cur->next();
            wassert(actual(cur).station_vars_match(f.ds_st_navile));
        }

        // Query and verify the measured data
        {
            auto cur = db.query_data(core::Query());
            wassert(actual(cur->remaining()) == 1);
            cur->next();
            wassert(actual(cur).data_context_matches(vals));
            wassert(actual(cur).data_var_matches(vals, WR_VAR(0, 12, 101)));
        }

        // Query and verify attributes
        {
            int count = 0;
            unique_ptr<Var> attr;
            wrunchecked(db.query_attrs(vals.values[WR_VAR(0, 12, 101)].data_id, WR_VAR(0, 12, 101), [&](std::unique_ptr<wreport::Var>&& var) {
                ++count;
                attr = move(var);
            }));
            wassert(actual(count) == 1);
            wassert(actual(attr->code()) == WR_VAR(0, 33, 7));
            wassert(actual(attr->enq(MISSING_INT)) == 50);
        }
    }),
    Test("insert_perms", [](Fixture& f) {
        // Test insert
        auto& db = *f.db;
        OldDballeTestFixture oldf;

        // Check if adding a nonexisting station when not allowed causes an error
        try {
            db.insert_data(oldf.data["synop"], false, false);
            ensure(false);
        } catch (error_consistency& e) {
            wassert(actual(e.what()).contains("insert a station entry when it is forbidden"));
        } catch (error_notfound& e) {
            wassert(actual(e.what()).contains("station not found"));
        }
        wassert(actual(oldf.data["synop"].info.ana_id) == MISSING_INT);
        wassert(actual(oldf.data["synop"].values["B01011"].data_id) == MISSING_INT);
        wassert(actual(oldf.data["synop"].values["B01012"].data_id) == MISSING_INT);
        oldf.data["synop"].clear_ids();

        // Insert the record
        wrunchecked(db.insert_data(oldf.data["synop"], false, true));
        oldf.data["synop"].clear_ids();
        // Check if duplicate updates are allowed by insert
        wrunchecked(db.insert_data(oldf.data["synop"], true, false));
        oldf.data["synop"].clear_ids();
        // Check if overwrites are trapped by insert_new
        oldf.data["synop"].values.set("B01011", "DB-All.e?");
        try {
            db.insert_data(oldf.data["synop"], false, false);
            ensure(false);
        } catch (wreport::error& e) {
            wassert(actual(e.what()).matches("refusing to overwrite existing data|cannot replace an existing value|Duplicate entry"));
        }
    }),
    Test("insert_twice", [](Fixture& f) {
        // Test double station insert
        auto& db = *f.db;
        OldDballeTestFixture oldf;

        // Insert the record twice
        wrunchecked(db.insert_data(oldf.data["synop"], false, true));
        // This should fail, refusing to replace station info
        oldf.data["synop"].values.set("B01011", "DB-All.e?");
        try {
            db.insert_data(oldf.data["synop"], false, true);
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
        auto cur = db.query_stations(core::Query());

        if (dynamic_cast<db::mem::DB*>(f.db))
        {
            // Memdb has one station entry per (lat, lon, ident, network)
            wassert(actual(cur->remaining()) == 2);

            wassert(actual(cur->next()).istrue());
            wassert(actual(cur->get_lat()) == 12.34560);
            wassert(actual(cur->get_lon()) == 76.54320);
            wassert(actual(cur->get_rep_memo()) == "synop");
            wassert(actual((void*)cur->get_ident()) == (void*)0);
            wassert(actual(cur).station_keys_match(oldf.stations["synop"].info));

            wassert(actual(cur->next()).istrue());
            wassert(actual(cur->get_lat()) == 12.34560);
            wassert(actual(cur->get_lon()) == 76.54320);
            wassert(actual(cur->get_rep_memo()) == "metar");
            wassert(actual((void*)cur->get_ident()) == (void*)0);
            wassert(actual(cur).station_keys_match(oldf.stations["metar"].info));
        } else {
            // V5 and V6 have one station entry (lat, lon, ident)
            wassert(actual(cur->remaining()) == 1);

            // There should be an item
            wassert(actual(cur->next()).istrue());
            wassert(actual(cur->get_lat()) == 12.34560);
            wassert(actual(cur->get_lon()) == 76.54320);
            wassert(actual((void*)cur->get_ident()) == (void*)0);

            // Check that the result matches
            wassert(actual(cur).station_keys_match(oldf.stations["metar"].info));

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
        query.latrange = LatRange(1000000, LatRange::IMAX);
        query.query = "best";

        // Make the query
        auto cur = db.query_data(query);

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
        auto cur = db.query_data(query);
        ensure_equals(cur->remaining(), 4);
        cur->discard_rest();

        query.clear();
        query.datetime = DatetimeRange(Datetime(1945, 4, 25, 8, 10), Datetime());
        db.remove(query);

        // 2 remaining after remove
        query.clear();
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 2);
        cur->discard_rest();

        // Did it remove the right ones?
        query.clear();
        query.latrange = LatRange(10.0, LatRange::DMAX);
        cur = db.query_data(query);
        ensure_equals(cur->remaining(), 2);
        ensure(cur->next());
        wassert(actual(cur).data_context_matches(oldf.data["synop"]));

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
                    wassert(actual(cur).data_var_matches(oldf.data["synop"], last_code));
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

        /* Prepare test data */
        DataValues base;
        base.info.coords = Coords(12.0, 48.0);
        base.info.report = "synop";
        base.info.level = Level(1, 0, 1, 0);
        base.info.trange = Trange(1, 0, 0);
        base.values.set("B01012", 500);

#define WANTRESULT(querystr, ab) do { \
        core::Record result; \
        auto cur = db.query_data(*dballe::tests::query_from_string(querystr)); \
        wassert(actual(cur->remaining()) == 1); \
        wassert(actual(cur->next()).istrue()); \
        cur->to_record(result); \
        wassert(actual(cur->remaining()) == 0); \
        ensure_varcode_equals(result.vars()[0]->code(), WR_VAR(0, 1, 12)); \
        ensure(cur->get_datetime() == ab.info.datetime); \
        cur->discard_rest(); \
    } while(0)

        DataValues a, b;

        /* Year */
        db.reset();
        a = base;
        a.info.datetime = Datetime(2005);
        db.insert_data(a, false, true);
        b = base;
        b.info.datetime = Datetime(2006);
        db.insert_data(b, false, false);
        WANTRESULT("yearmin=2006", b);
        WANTRESULT("yearmax=2005", a);
        WANTRESULT("year=2006", b);

        /* Month */
        db.reset();
        a = base;
        a.info.datetime = Datetime(2006, 4);
        db.insert_data(a, false, true);
        b = base;
        b.info.datetime = Datetime(2006, 5);
        db.insert_data(b, false, false);
        WANTRESULT("year=2006, monthmin=5", b);
        WANTRESULT("year=2006, monthmax=4", a);
        WANTRESULT("year=2006, month=5", b);

        /* Day */
        db.reset();
        a = base;
        a.info.datetime = Datetime(2006, 5, 2);
        db.insert_data(a, false, true);
        b = base;
        b.info.datetime = Datetime(2006, 5, 3);
        db.insert_data(b, false, false);
        WANTRESULT("year=2006, month=5, daymin=3", b);
        WANTRESULT("year=2006, month=5, daymax=2", a);
        WANTRESULT("year=2006, month=5, day=3", b);

        /* Hour */
        db.reset();
        a = base;
        a.info.datetime = Datetime(2006, 5, 3, 12);
        db.insert_data(a, false, true);
        b = base;
        b.info.datetime = Datetime(2006, 5, 3, 13);
        db.insert_data(b, false, false);
        WANTRESULT("year=2006, month=5, day=3, hourmin=13", b);
        WANTRESULT("year=2006, month=5, day=3, hourmax=12", a);
        WANTRESULT("year=2006, month=5, day=3, hour=13", b);

        /* Minute */
        db.reset();
        a = base;
        a.info.datetime = Datetime(2006, 5, 3, 12, 29);
        db.insert_data(a, false, true);
        b = base;
        b.info.datetime = Datetime(2006, 5, 3, 12, 30);
        db.insert_data(b, false, false);
        WANTRESULT("year=2006, month=5, day=3, hour=12, minumin=30", b);
        WANTRESULT("year=2006, month=5, day=3, hour=12, minumax=29", a);
        WANTRESULT("year=2006, month=5, day=3, hour=12, min=30", b);
    }),
    Test("attrs", [](Fixture& f) {
        // Test QC
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        core::Query query;
        core::Record result;
        query.latrange.set(1000000, LatRange::IMAX);
        auto cur = db.query_data(query);

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
        Values qc;
        qc.set("B33002", 2);
        qc.set("B33003", 5);
        qc.set("B33005", 33);
        db.attr_insert(context_id, WR_VAR(0, 1, 11), qc);

        // Query back the data
        qc.clear();
        wassert(actual(run_query_attrs(db, qc, context_id, WR_VAR(0, 1, 11))) == 3);

        const auto* attr = qc.get("B33002");
        ensure(attr != NULL);
        ensure_equals(attr->var->enqi(), 2);

        attr = qc.get("B33003");
        ensure(attr != NULL);
        ensure_equals(attr->var->enqi(), 5);

        attr = qc.get("B33005");
        ensure(attr != NULL);
        ensure_equals(attr->var->enqi(), 33);

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

        ensure(qc.get("B33002") == nullptr);
        ensure(qc.get("B33005") == nullptr);
        attr = qc.get("B33003");
        ensure(attr != NULL);
        ensure_equals(attr->var->enqi(), 5);
        /*dba_error_remove_callback(DBA_ERR_NONE, crash, 0);*/
    }),
    Test("query_station", [](Fixture& f) {
        // Test station queries
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        auto cur = db.query_stations(*query_from_string("rep_memo=synop"));
        ensure_equals(cur->remaining(), 1);

        ensure(cur->next());
        ensure(!cur->next());
    }),
    Test("attrs1", [](Fixture& f) {
        // Test attributes
        auto& db = *f.db;
        OldDballeTestFixture oldf;

        // Insert a data record
        db.insert_data(oldf.data["synop"], true, true);

        Values qc;
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
        auto cur = db.query_data(*query_from_string("var=B01011"));
        ensure_equals(cur->remaining(), 1);
        cur->next();
        int attr_id = cur->attr_reference_id();
        cur->discard_rest();

        qc.clear();
        wassert(actual(run_query_attrs(db, qc, attr_id, WR_VAR(0, 1, 11))) == 10);

        // Check that all the attributes come out
        ensure_equals(qc.size(), 10);
        ensure_varcode_equals(qc["B01007"].var->code(), WR_VAR(0,   1,  7)); ensure_var_equals(*qc["B01007"].var,  1);
        ensure_varcode_equals(qc["B02048"].var->code(), WR_VAR(0,   2, 48)); ensure_var_equals(*qc["B02048"].var,  2);
        ensure_varcode_equals(qc["B05021"].var->code(), WR_VAR(0,   5, 21)); ensure_var_equals(*qc["B05021"].var,  8);
        ensure_varcode_equals(qc["B05022"].var->code(), WR_VAR(0,   5, 22)); ensure_var_equals(*qc["B05022"].var, 10);
        ensure_varcode_equals(qc["B05040"].var->code(), WR_VAR(0,   5, 40)); ensure_var_equals(*qc["B05040"].var,  3);
        ensure_varcode_equals(qc["B05041"].var->code(), WR_VAR(0,   5, 41)); ensure_var_equals(*qc["B05041"].var,  4);
        ensure_varcode_equals(qc["B05043"].var->code(), WR_VAR(0,   5, 43)); ensure_var_equals(*qc["B05043"].var,  5);
        ensure_varcode_equals(qc["B07024"].var->code(), WR_VAR(0,   7, 24)); ensure_var_equals(*qc["B07024"].var,  7);
        ensure_varcode_equals(qc["B07025"].var->code(), WR_VAR(0,   7, 25)); ensure_var_equals(*qc["B07025"].var,  9);
        ensure_varcode_equals(qc["B33032"].var->code(), WR_VAR(0,  33, 32)); ensure_var_equals(*qc["B33032"].var,  6);
    }),
    Test("longitude_wrap", [](Fixture& f) {
        // Test longitude wrapping around
        auto& db = *f.db;
        OldDballeTestFixture oldf;

        // Insert a data record
        db.insert_data(oldf.data["synop"], true, true);

        auto cur = db.query_data(*query_from_string("latmin=10.0, latmax=15.0, lonmin=70.0, lonmax=-160.0"));
        ensure_equals(cur->remaining(), 2);
        cur->discard_rest();
    }),
    Test("query_ana_filter", [](Fixture& f) {
        // Test numeric comparisons in ana_filter
        auto& db = *f.db;
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        auto cur = db.query_data(*query_from_string("rep_memo=metar, var=B01011"));
        ensure_equals(cur->remaining(), 1);

        // Move the cursor to B01011
        cur->next();
        int context_id = cur->attr_reference_id();
        cur->discard_rest();

        // Insert new attributes about this report
        Values qc;
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
        q.datetime = DatetimeRange(Datetime(1000, 1, 1, 0, 0, 0), Datetime(1000, 1, 1, 0, 0, 0));
        q.query = "best";
        auto cur = db.query_data(q);
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
            Messages inmsgs = read_msgs(files[i], File::BUFR);
            wrunchecked(db.import_msg(inmsgs[0], NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_OVERWRITE));
        }

        // Query all with best
        auto cur = db.query_data(*query_from_string("var=B12101, query=best"));
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
        StationValues vals;
        // Set station data
        vals.info.coords = Coords(12.34560, 76.54320);
        vals.info.report = "synop";
        vals.values.set("B07030", 42.0); // Height

        // Assume a max open file limit of 1100
        for (unsigned i = 0; i < 1100; ++i)
        {
            std::unique_ptr<DB> db = f.create_db();
            vals.clear_ids();
            wrunchecked(db->insert_station_data(vals, true, true));
        }
    }),
    Test("update", [](Fixture& f) {
        auto& db = *f.db;
        // Test value update
        OldDballeTestFixture oldf;

        DataValues dataset = oldf.data["synop"];
        db.insert_data(dataset, true, true);
        Values attrs;
        attrs.set("B33007", 50);
        db.attr_insert(dataset.values["B01012"].data_id, WR_VAR(0, 1, 12), attrs);

        core::Query q;
        q.latrange.set(12.34560, 12.34560);
        q.lonrange.set(76.54320, 76.54320);
        q.datetime = DatetimeRange(Datetime(1945, 4, 25, 8, 0, 0), Datetime(1945, 4, 25, 8, 0, 0));
        q.rep_memo = "synop";
        q.level = Level(10, 11, 15, 22);
        q.trange = Trange(20, 111, 122);
        q.varcodes.insert(WR_VAR(0, 1, 12));

        // Query the initial value
        auto cur = db.query_data(q);
        wassert(actual(cur->remaining()) == 1);
        cur->next();
        int ana_id = cur->get_station_id();
        wreport::Var var = cur->get_var();
        wassert(actual(var.enqi()) == 300);

        // Query the attributes and check that they are there
        Values qattrs;
        wassert(actual(run_query_attrs(db, qattrs, cur->attr_reference_id(), cur->get_varcode())) == 1);
        wassert(actual(qattrs["B33007"].var->enq(MISSING_INT)) == 50);

        // Update it
        DataValues update;
        update.info.ana_id = ana_id;
        update.info.report = "synop";
        update.info.datetime = q.datetime.min;
        update.info.level = q.level;
        update.info.trange = q.trange;
        update.values.set(var.code(), 200);
        db.insert_data(update, true, false);

        // Query again
        cur = db.query_data(q);
        wassert(actual(cur->remaining()) == 1);
        cur->next();
        var = cur->get_var();
        wassert(actual(var.enqi()) == 200);

        qattrs.clear();
        wassert(actual(run_query_attrs(db, qattrs, cur->attr_reference_id(), cur->get_varcode())) == 1);
        wassert(actual(qattrs["B33007"].var->enq(MISSING_INT)) == 50);
    }),
    Test("query_stepbystep", [](Fixture& f) {
        auto& db = *f.db;
        // Try a query checking all the steps
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);

        // Make the query
        auto cur = db.query_data(*query_from_string("latmin=10.0"));
        wassert(actual(cur->remaining()) == 4);

        wassert(actual(cur->next()).istrue());
        // remaining() should decrement
        wassert(actual(cur->remaining()) == 3);
        // results should match what was inserted
        wassert(actual(cur).data_matches(oldf.data["synop"]));
        // just call to_record now, to check if in the next call old variables are removed
        core::Record result;
        cur->to_record(result);

        ensure(cur->next());
        wassert(actual(cur->remaining()) == 2);
        wassert(actual(cur).data_matches(oldf.data["synop"]));

        // Variables from the previous to_record should be removed
        cur->to_record(result);
        wassert(actual(result.vars().size()) == 1u);


        ensure(cur->next());
        wassert(actual(cur->remaining()) == 1);
        wassert(actual(cur).data_matches(oldf.data["metar"]));

        ensure(cur->next());
        wassert(actual(cur->remaining()) == 0);
        wassert(actual(cur).data_matches(oldf.data["metar"]));

        // Now there should not be anything anymore
        wassert(actual(cur->remaining()) == 0);
        ensure(!cur->next());
    }),
    Test("insert_stationinfo_twice", [](Fixture& f) {
        // Test double insert of station info
        auto& db = *f.db;

        //wassert(actual(f.db).empty());
        db.insert_station_data(f.ds_st_navile, true, true);
        db.insert_station_data(f.ds_st_navile, true, true);

        // Query station data and ensure there is only one info (height)
        core::Query query;
        auto cur = db.query_station_data(query);
        wassert(actual(cur->remaining()) == 1);
        cur->next();
        wassert(actual(cur).station_vars_match(f.ds_st_navile));
    }),
    Test("insert_stationinfo_twice1", [](Fixture& f) {
        // Test double insert of station info
        auto& db = *f.db;
        StationValues ds_st_navile_metar(f.ds_st_navile);
        ds_st_navile_metar.info.report = "metar";
        db.insert_station_data(f.ds_st_navile, true, true);
        db.insert_station_data(ds_st_navile_metar, true, true);

        // Query station data and ensure there is only one info (height)
        core::Query query;
        auto cur = db.query_station_data(query);
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
        DataValues dataset;
        dataset.info = oldf.data["synop"].info;
        dataset.info.level = Level(44, 55);
        dataset.info.trange = Trange(20);
        dataset.values.set("B01012", 300);
        db.insert_data(dataset, true, true);

        // Query it back
        auto cur = db.query_data(*query_from_string("leveltype1=44, l1=55"));
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
        auto cur = db.query_data(*query_from_string("leveltype1=10, l1=11"));
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
        DataValues insert;
        insert.info.coords = Coords(1.0, 1.0);
        insert.info.level = Level(1, 0);
        insert.info.trange = Trange(254, 0, 0);
        insert.info.datetime = Datetime(2009, 11, 11, 0, 0, 0);

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
            insert.clear_ids();
            insert.info.report = *i;
            insert.values.set("B12101", (int)(i - rep_memos));
            db.insert_data(insert, false, true);
        }

        // Query with querybest only
        {
            core::Query query;
            query.query = "best";
            query.datetime = DatetimeRange(Datetime(2009, 11, 11, 0, 0, 0), Datetime(2009, 11, 11, 0, 0, 0));
            query.varcodes.insert(WR_VAR(0, 12, 101));
            auto cur = db.query_data(query);

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
            query.prio_max = 100;
            query.query = "best";
            query.datetime = DatetimeRange(Datetime(2009, 11, 11, 0, 0, 0), Datetime(2009, 11, 11, 0, 0, 0));
            query.varcodes.insert(WR_VAR(0, 12, 101));
            auto cur = db.query_data(query);
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
        auto cur = db.query_data(core::Query());
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
