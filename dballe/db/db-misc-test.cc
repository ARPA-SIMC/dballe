#include "dballe/db/tests.h"
#include "v7/db.h"
#include "v7/transaction.h"
#include "config.h"
#include <algorithm>
#include <cstring>

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct NavileDataSet : public TestDataSet
{
    NavileDataSet()
    {
        stations["synop"].station.coords = Coords(44.5008, 11.3288);
        stations["synop"].station.report = "synop";
        stations["synop"].values.set("B07030", 78); // Height
    }
};

unsigned run_attr_query_data(std::shared_ptr<db::Transaction> tr, int data_id, core::Values& dest)
{
    unsigned count = 0;
    tr->attr_query_data(data_id, [&](unique_ptr<Var> var) { dest.set(move(var)); ++count; });
    return count;
}


template<typename DB>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

template<typename DB>
class CommitTests : public FixtureTestCase<DBFixture<DB>>
{
    typedef DBFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

Tests<V7DB> tg2("db_misc_tr_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB> tg4("db_misc_tr_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB> tg6("db_misc_tr_v7_mysql", "MYSQL");
#endif

CommitTests<V7DB> ct2("db_misc_db_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
CommitTests<V7DB> ct4("db_misc_db_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
CommitTests<V7DB> ct6("db_misc_db_v7_mysql", "MYSQL");
#endif

template<typename DB>
void Tests<DB>::register_tests()
{

this->add_method("insert", [](Fixture& f) {
    // Test a simple insert round trip

    // Insert some data
    NavileDataSet ds;
    ds.data["synop"].station = ds.stations["synop"].station;
    ds.data["synop"].datetime = Datetime(2013, 10, 16, 10);
    ds.data["synop"].values.set(WR_VAR(0, 12, 101), 16.5);
    wassert(f.populate(ds));

    core::Values attrs;
    attrs.set("B33007", 50);
    wassert(f.tr->attr_insert_data(ds.data["synop"].values.value(WR_VAR(0, 12, 101)).data_id, attrs));

    // Query and verify the station data
    {
        auto cur = f.tr->query_stations(core::Query());
        wassert(actual(cur->remaining()) == 1);
        cur->next();
        wassert(actual(cur).station_keys_match(ds.stations["synop"].station));
        wassert(actual(cur).data_var_matches(ds.stations["synop"].values));
    }

    // Query and verify the measured data
    {
        auto cur = f.tr->query_data(core::Query());
        wassert(actual(cur->remaining()) == 1);
        cur->next();
        wassert(actual(cur).data_context_matches(ds.data["synop"]));
        wassert(actual(cur).data_var_matches(ds.data["synop"], WR_VAR(0, 12, 101)));
    }

    // Query and verify attributes
    {
        int count = 0;
        unique_ptr<Var> attr;
        wassert(f.tr->attr_query_data(ds.data["synop"].values.value(WR_VAR(0, 12, 101)).data_id, [&](std::unique_ptr<wreport::Var>&& var) {
            ++count;
            attr = move(var);
        }));
        wassert(actual(count) == 1);
        wassert(actual(attr->code()) == WR_VAR(0, 33, 7));
        wassert(actual(attr->enq(MISSING_INT)) == 50);
    }
});
this->add_method("insert_perms", [](Fixture& f) {
    // Test insert
    OldDballeTestDataSet oldf;

    // Check if adding a nonexisting station when not allowed causes an error
    try {
        f.tr->insert_data(oldf.data["synop"], false, false);
        throw TestFailed("error_consistency should have been thrown");
    } catch (error_consistency& e) {
        wassert(actual(e.what()).contains("insert a station entry when it is forbidden"));
    } catch (error_notfound& e) {
        wassert(actual(e.what()).contains("station not found"));
    }
    wassert(actual(oldf.data["synop"].station.id) == MISSING_INT);
    wassert(actual(oldf.data["synop"].values.value("B01011").data_id) == MISSING_INT);
    wassert(actual(oldf.data["synop"].values.value("B01012").data_id) == MISSING_INT);
    oldf.data["synop"].clear_ids();

    // Insert the record
    wassert(f.tr->insert_data(oldf.data["synop"], false, true));
    oldf.data["synop"].clear_ids();
    // Check if duplicate updates are allowed by insert
    wassert(f.tr->insert_data(oldf.data["synop"], true, false));
    oldf.data["synop"].clear_ids();
    // Check if overwrites are trapped by insert_new
    oldf.data["synop"].values.set("B01011", "DB-All.e?");
    try {
        f.tr->insert_data(oldf.data["synop"], false, false);
        throw TestFailed("wreport::error should have been thrown");
    } catch (wreport::error& e) {
        wassert(actual(e.what()).matches("refusing to overwrite existing data|cannot replace an existing value|Duplicate entry"));
    }
});
this->add_method("insert_twice", [](Fixture& f) {
    // Test double station insert
    OldDballeTestDataSet oldf;

    // Insert the record twice
    wassert(f.tr->insert_data(oldf.data["synop"], false, true));
    // This should fail, refusing to replace station info
    oldf.data["synop"].values.set("B01011", "DB-All.e?");
    try {
        f.tr->insert_data(oldf.data["synop"], false, true);
        throw TestFailed("wreport::error should have been thrown");
    } catch (wreport::error& e) {
        wassert(actual(e.what()).matches("refusing to overwrite existing data|cannot replace an existing value|Duplicate entry"));
    }
});
this->add_method("query_station", [](Fixture& f) {
    // Test station query
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    // Iterate the station database
    auto cur = f.tr->query_stations(core::Query());

    switch (DB::format)
    {
        case Format::V7:
        {
            bool have_synop = false;
            bool have_metar = false;

            // Memdb and V7 have one station entry per (lat, lon, ident, network)
            wassert(actual(cur->remaining()) == 2);

            for (unsigned i = 0; i < 2; ++i)
            {
                wassert(actual(cur->next()).istrue());
                DBStation station = cur->get_station();

                if (station.report == "synop")
                {
                    wassert(actual(station.coords) == Coords(12.34560, 76.54320));
                    wassert(actual(station.ident.is_missing()).istrue());
                    wassert(actual(cur).station_keys_match(oldf.stations["synop"].station));
                    have_synop = true;
                }

                if (station.report == "metar")
                {
                    wassert(actual(station.coords) == Coords(12.34560, 76.54320));
                    wassert(actual(station.ident.is_missing()).istrue());
                    wassert(actual(cur).station_keys_match(oldf.stations["metar"].station));
                    have_metar = true;
                }
            }

            wassert(actual(have_synop).istrue());
            wassert(actual(have_metar).istrue());
            break;
        }
        default: error_unimplemented::throwf("cannot run this test on a database of format %d", (int)DB::format);
    }
    wassert(actual(cur->remaining()) == 0);
    wassert(actual(cur->next()).isfalse());
});
this->add_method("query_best", [](Fixture& f) {
    // Test querybest
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    //if (db.server_type == ORACLE || db.server_type == POSTGRES)
    //      return;

    // Prepare a query
    core::Query query;
    query.latrange = LatRange(1000000, LatRange::IMAX);
    query.query = "best";

    // Make the query
    auto cur = f.tr->query_data(query);

    wassert(actual(cur->remaining()) == 4);

    // There should be four items
    wassert(actual(cur->next()).istrue());
    DBStation station = cur->get_station();
    wassert(actual(station.coords) == Coords(12.34560, 76.54320));
    wassert(actual(station.ident.is_missing()).istrue());
    wassert(actual(station.report) == "synop");
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
    wassert(actual(cur->next()).isfalse());
});
this->add_method("delete", [](Fixture& f) {
    // Test deletion
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    // 4 items to begin with
    core::Query query;
    auto cur = f.tr->query_data(query);
    wassert(actual(cur->remaining()) == 4);
    cur->discard();

    query.clear();
    query.datetime = DatetimeRange(Datetime(1945, 4, 25, 8, 10), Datetime());
    f.tr->remove_data(query);

    // 2 remaining after remove
    query.clear();
    cur = f.tr->query_data(query);
    wassert(actual(cur->remaining()) == 2);
    cur->discard();

    // Did it remove the right ones?
    query.clear();
    query.latrange = LatRange(10.0, LatRange::DMAX);
    cur = f.tr->query_data(query);
    wassert(actual(cur->remaining()) == 2);
    wassert(actual(cur->next()).istrue());
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
                throw TestFailed("got a varcode that we did not ask for: " + varcode_format(last_code));
        }

        if (i == 0)
        {
            /* The item should have two data in it */
            wassert(actual(cur->next()).istrue());
        } else {
            wassert(actual(cur->next()).isfalse());
        }
    }
});
this->add_method("delete_notfound", [](Fixture& f) {
    // Test deletion
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    // 4 items to begin with
    core::Query query;
    auto cur = f.tr->query_data(query);
    wassert(actual(cur->remaining()) == 4);
    cur->discard();

    // Try to remove using a query that matches none
    query.attr_filter = "B33007<50";
    f.tr->remove_data(query);

    // Verify that nothing has been deleted
    query.clear();
    cur = f.tr->query_data(query);
    wassert(actual(cur->remaining()) == 4);
    cur->discard();
});
this->add_method("query_datetime", [](Fixture& f) {
    // Test datetime queries
    /* Prepare test data */
    core::Data base;
    base.station.coords = Coords(12.0, 48.0);
    base.station.report = "synop";
    base.level = Level(1, 0, 1, 0);
    base.trange = Trange(1, 0, 0);
    base.values.set("B01012", 500);

#define WANTRESULT(querystr, ab) do { \
    auto cur = f.tr->query_data(*dballe::tests::query_from_string(querystr)); \
    wassert(actual(cur->remaining()) == 1); \
    wassert(actual(cur->next()).istrue()); \
    wassert(actual(cur->remaining()) == 0); \
    wassert(actual_varcode(cur->get_varcode()) == WR_VAR(0, 1, 12)); \
    wassert(actual(cur->get_datetime()) == cur->get_datetime()); \
    cur->discard(); \
} while(0)

    core::Data a, b;

    /* Year */
    f.tr->remove_all();
    a = base;
    a.datetime = Datetime(2005);
    f.tr->insert_data(a, false, true);
    b = base;
    b.datetime = Datetime(2006);
    f.tr->insert_data(b, false, false);
    WANTRESULT("yearmin=2006", b);
    WANTRESULT("yearmax=2005", a);
    WANTRESULT("year=2006", b);

    /* Month */
    f.tr->remove_all();
    a = base;
    a.datetime = Datetime(2006, 4);
    f.tr->insert_data(a, false, true);
    b = base;
    b.datetime = Datetime(2006, 5);
    f.tr->insert_data(b, false, false);
    WANTRESULT("year=2006, monthmin=5", b);
    WANTRESULT("year=2006, monthmax=4", a);
    WANTRESULT("year=2006, month=5", b);

    /* Day */
    f.tr->remove_all();
    a = base;
    a.datetime = Datetime(2006, 5, 2);
    f.tr->insert_data(a, false, true);
    b = base;
    b.datetime = Datetime(2006, 5, 3);
    f.tr->insert_data(b, false, false);
    WANTRESULT("year=2006, month=5, daymin=3", b);
    WANTRESULT("year=2006, month=5, daymax=2", a);
    WANTRESULT("year=2006, month=5, day=3", b);

    /* Hour */
    f.tr->remove_all();
    a = base;
    a.datetime = Datetime(2006, 5, 3, 12);
    f.tr->insert_data(a, false, true);
    b = base;
    b.datetime = Datetime(2006, 5, 3, 13);
    f.tr->insert_data(b, false, false);
    WANTRESULT("year=2006, month=5, day=3, hourmin=13", b);
    WANTRESULT("year=2006, month=5, day=3, hourmax=12", a);
    WANTRESULT("year=2006, month=5, day=3, hour=13", b);

    /* Minute */
    f.tr->remove_all();
    a = base;
    a.datetime = Datetime(2006, 5, 3, 12, 29);
    f.tr->insert_data(a, false, true);
    b = base;
    b.datetime = Datetime(2006, 5, 3, 12, 30);
    f.tr->insert_data(b, false, false);
    WANTRESULT("year=2006, month=5, day=3, hour=12, minumin=30", b);
    WANTRESULT("year=2006, month=5, day=3, hour=12, minumax=29", a);
    WANTRESULT("year=2006, month=5, day=3, hour=12, min=30", b);
});
this->add_method("attrs", [](Fixture& f) {
    // Test QC
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    core::Query query;
    query.latrange.set(1000000, LatRange::IMAX);
    auto cur = f.tr->query_data(query);

    // Move the cursor to B01011
    int context_id = 0;
    bool found = false;
    while (cur->next())
    {
        if (cur->get_varcode() == WR_VAR(0, 1, 11))
        {
            context_id = dynamic_cast<db::CursorData*>(cur.get())->attr_reference_id();
            cur->discard();
            found = true;
            break;
        }
    }
    wassert(actual(found).istrue());

    // Insert new attributes about this report
    core::Values qc;
    qc.set("B33002", 2);
    qc.set("B33003", 5);
    qc.set("B33005", 33);
    f.tr->attr_insert_data(context_id, qc);

    // Query back the data
    qc.clear();
    wassert(actual(run_attr_query_data(f.tr, context_id, qc)) == 3);

    const auto* attr = qc.maybe_var("B33002");
    wassert(actual(attr).istrue());
    wassert(actual(attr->enqi()) == 2);

    attr = qc.maybe_var("B33003");
    wassert(actual(attr).istrue());
    wassert(actual(attr->enqi()) == 5);

    attr = qc.maybe_var("B33005");
    wassert(actual(attr).istrue());
    wassert(actual(attr->enqi()) == 33);

    // Delete a couple of items
    vector<Varcode> codes;
    codes.push_back(WR_VAR(0, 33, 2));
    codes.push_back(WR_VAR(0, 33, 5));
    f.tr->attr_remove_data(context_id, codes);

    // Deleting non-existing items should not fail.  Also try creating a
    // query with just one item
    codes.clear();
    codes.push_back(WR_VAR(0, 33, 2));
    f.tr->attr_remove_data(context_id, codes);

    /* Query back the data */
    qc.clear();
    wassert(actual(run_attr_query_data(f.tr, context_id, qc)) == 1);

    wassert(actual(qc.maybe_var("B33002")).isfalse());
    wassert(actual(qc.maybe_var("B33005")).isfalse());
    attr = qc.maybe_var("B33003");
    wassert(actual(attr).istrue());
    wassert(actual(attr->enqi()) == 5);
    /*dba_error_remove_callback(DBA_ERR_NONE, crash, 0);*/
});
this->add_method("query_station", [](Fixture& f) {
    // Test station queries
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    auto cur = f.tr->query_stations(*query_from_string("rep_memo=synop"));
    wassert(actual(cur->remaining()) == 1);

    wassert(actual(cur->next()).istrue());
    wassert(actual(cur->next()).isfalse());
});
this->add_method("attrs1", [](Fixture& f) {
    // Test attributes
    OldDballeTestDataSet oldf;

    // Insert a data record
    f.tr->insert_data(oldf.data["synop"], true, true);

    core::Values qc;
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
    f.tr->attr_insert_data(oldf.data["synop"].values.value(WR_VAR(0, 1, 11)).data_id, qc);

    // Query back the B01011 variable to read the attr reference id
    auto cur = f.tr->query_data(*query_from_string("var=B01011"));
    wassert(actual(cur->remaining()) == 1);
    cur->next();
    int attr_id = dynamic_cast<db::CursorData*>(cur.get())->attr_reference_id();
    cur->discard();

    qc.clear();
    wassert(actual(run_attr_query_data(f.tr, attr_id, qc)) == 10);

    // Check that all the attributes come out
    wassert(actual(qc.size()) == 10);
    wassert(actual_varcode(qc.var("B01007").code()) == WR_VAR(0,   1,  7)); wassert(actual(qc.var("B01007")) ==  1);
    wassert(actual_varcode(qc.var("B02048").code()) == WR_VAR(0,   2, 48)); wassert(actual(qc.var("B02048")) ==  2);
    wassert(actual_varcode(qc.var("B05021").code()) == WR_VAR(0,   5, 21)); wassert(actual(qc.var("B05021")) ==  8);
    wassert(actual_varcode(qc.var("B05022").code()) == WR_VAR(0,   5, 22)); wassert(actual(qc.var("B05022")) == 10);
    wassert(actual_varcode(qc.var("B05040").code()) == WR_VAR(0,   5, 40)); wassert(actual(qc.var("B05040")) ==  3);
    wassert(actual_varcode(qc.var("B05041").code()) == WR_VAR(0,   5, 41)); wassert(actual(qc.var("B05041")) ==  4);
    wassert(actual_varcode(qc.var("B05043").code()) == WR_VAR(0,   5, 43)); wassert(actual(qc.var("B05043")) ==  5);
    wassert(actual_varcode(qc.var("B07024").code()) == WR_VAR(0,   7, 24)); wassert(actual(qc.var("B07024")) ==  7);
    wassert(actual_varcode(qc.var("B07025").code()) == WR_VAR(0,   7, 25)); wassert(actual(qc.var("B07025")) ==  9);
    wassert(actual_varcode(qc.var("B33032").code()) == WR_VAR(0,  33, 32)); wassert(actual(qc.var("B33032")) ==  6);
});
this->add_method("longitude_wrap", [](Fixture& f) {
    // Test longitude wrapping around
    OldDballeTestDataSet oldf;

    // Insert a data record
    f.tr->insert_data(oldf.data["synop"], true, true);

    auto cur = f.tr->query_data(*query_from_string("latmin=10.0, latmax=15.0, lonmin=70.0, lonmax=-160.0"));
    wassert(actual(cur->remaining()) == 2);
    cur->discard();
});
this->add_method("query_ana_filter", [](Fixture& f) {
    // Test numeric comparisons in ana_filter
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    auto cur = f.tr->query_data(*query_from_string("rep_memo=metar, var=B01011"));
    wassert(actual(cur->remaining()) == 1);

    // Move the cursor to B01011
    cur->next();
    int context_id = dynamic_cast<db::CursorData*>(cur.get())->attr_reference_id();
    cur->discard();

    // Insert new attributes about this report
    core::Values qc;
    qc.set("B01001", 50);
    qc.set("B01008", "50");
    f.tr->attr_insert_data(context_id, qc);

    // Try queries filtered by numeric attributes
    cur = f.tr->query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01001=50"));
    wassert(actual(cur->remaining()) == 1);
    cur->discard();

    cur = f.tr->query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01001<=50"));
    wassert(actual(cur->remaining()) == 1);
    cur->discard();

    cur = f.tr->query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01001<51"));
    wassert(actual(cur->remaining()) == 1);
    cur->discard();

    cur = f.tr->query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01001<8"));
    wassert(actual(cur->remaining()) == 0);
    cur->discard();

    // Try queries filtered by string attributes
    cur = f.tr->query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01008=50"));
    wassert(actual(cur->remaining()) == 1);
    cur->discard();

    cur = f.tr->query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01008<=50"));
    wassert(actual(cur->remaining()) == 1);
    cur->discard();

    cur = f.tr->query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01008<8"));
    wassert(actual(cur->remaining()) == 1);
    cur->discard();

    cur = f.tr->query_data(*query_from_string("rep_memo=metar, var=B01011, attr_filter=B01008<100"));
    wassert(actual(cur->remaining()) == 0);
    cur->discard();
});
this->add_method("query_station_best", [](Fixture& f) {
    // Reproduce a querybest scenario which produced invalid SQL
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    core::Query q;
    q.datetime = DatetimeRange(Datetime(1000, 1, 1, 0, 0, 0), Datetime(1000, 1, 1, 0, 0, 0));
    q.query = "best";
    auto cur = f.tr->query_data(q);
    while (cur->next())
    {
    }
});
this->add_method("query_best_bug1", [](Fixture& f) {
    // Reproduce a querybest scenario which produced always the same data record

    // Import lots
    const char** files = dballe::tests::bufr_files;
    DBImportMessageOptions opts;
    opts.import_attributes = true;
    opts.update_station = true;
    opts.overwrite = true;
    for (int i = 0; files[i] != NULL; i++)
    {
        Messages inmsgs = read_msgs(files[i], Encoding::BUFR);
        wassert(f.tr->import_message(*inmsgs[0], opts));
    }

    // Query all with best
    auto cur = f.tr->query_data(*query_from_string("var=B12101, query=best"));
    unsigned orig_count = cur->remaining();
    unsigned count = 0;
    int id_data = 0;
    unsigned id_data_changes = 0;
    while (cur->next())
    {
        ++count;
        int attr_reference_id = dynamic_cast<db::CursorData*>(cur.get())->attr_reference_id();
        if (attr_reference_id != id_data)
        {
            id_data = attr_reference_id;
            ++id_data_changes;
        }
    }

    wassert(actual(count) > 1);
    wassert(actual(id_data_changes) == count);
    wassert(actual(count) == orig_count);
});
this->add_method("query_invalid_sql", [](Fixture& f) {
    // Reproduce a query that generated invalid SQL on V6
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    // All DB
    f.tr->query_stations(*query_from_string("leveltype1=103, l1=2000"));
});

this->add_method("update", [](Fixture& f) {
    // Test value update
    OldDballeTestDataSet oldf;

    core::Data dataset = oldf.data["synop"];
    f.tr->insert_data(dataset, true, true);
    core::Values attrs;
    attrs.set("B33007", 50);
    f.tr->attr_insert_data(dataset.values.value("B01012").data_id, attrs);

    core::Query q;
    q.latrange.set(12.34560, 12.34560);
    q.lonrange.set(76.54320, 76.54320);
    q.datetime = DatetimeRange(Datetime(1945, 4, 25, 8, 0, 0), Datetime(1945, 4, 25, 8, 0, 0));
    q.rep_memo = "synop";
    q.level = Level(10, 11, 15, 22);
    q.trange = Trange(20, 111, 122);
    q.varcodes.insert(WR_VAR(0, 1, 12));

    // Query the initial value
    auto cur = f.tr->query_data(q);
    wassert(actual(cur->remaining()) == 1);
    cur->next();
    int ana_id = cur->get_station().id;
    wreport::Var var = cur->get_var();
    wassert(actual(var.enqi()) == 300);

    // Query the attributes and check that they are there
    core::Values qattrs;
    wassert(actual(run_attr_query_data(f.tr, dynamic_cast<db::CursorData*>(cur.get())->attr_reference_id(), qattrs)) == 1);
    wassert(actual(qattrs.var("B33007").enq(MISSING_INT)) == 50);

    // Update it
    core::Data update;
    update.station.id = ana_id;
    update.station.report = "synop";
    update.datetime = q.datetime.min;
    update.level = q.level;
    update.trange = q.trange;
    update.values.set(var.code(), 200);
    wassert(f.tr->insert_data(update, true, false));

    // Query again
    cur = f.tr->query_data(q);
    wassert(actual(cur->remaining()) == 1);
    cur->next();
    var = cur->get_var();
    wassert(actual(var.enqi()) == 200);

    qattrs.clear();

    // V7 databases implement the semantics in #44 where updating a
    // value removes the attributes
    switch (DB::format)
    {
        case Format::V7:
            wassert(actual(run_attr_query_data(f.tr, dynamic_cast<db::CursorData*>(cur.get())->attr_reference_id(), qattrs)) == 0);
            break;
        default:
            throw TestFailed("Database format " + to_string((int)DB::format) + " not supported");
    }
});
this->add_method("query_stepbystep", [](Fixture& f) {
    // Try a query checking all the steps
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    // Make the query
    auto cur = f.tr->query_data(*query_from_string("latmin=10.0"));
    wassert(actual(cur->remaining()) == 4);

    wassert(actual(cur->next()).istrue());
    // remaining() should decrement
    wassert(actual(cur->remaining()) == 3);
    // results should match what was inserted
    wassert(actual(cur).data_matches(oldf.data["metar"]));
    // just call to_record now, to check if in the next call old variables are removed

    wassert(actual(cur->next()).istrue());
    wassert(actual(cur->remaining()) == 2);
    wassert(actual(cur).data_matches(oldf.data["metar"]));

    wassert(actual(cur->next()).istrue());
    wassert(actual(cur->remaining()) == 1);
    wassert(actual(cur).data_matches(oldf.data["synop"]));

    wassert(actual(cur->next()).istrue());
    wassert(actual(cur->remaining()) == 0);
    wassert(actual(cur).data_matches(oldf.data["synop"]));

    // Now there should not be anything anymore
    wassert(actual(cur->remaining()) == 0);
    wassert(actual(cur->next()).isfalse());
});
this->add_method("insert_stationinfo_twice", [](Fixture& f) {
    // Test double insert of station info
    NavileDataSet ds;

    //wassert(actual(f.db).empty());
    f.tr->insert_station_data(ds.stations["synop"], true, true);
    f.tr->insert_station_data(ds.stations["synop"], true, true);

    // Query station data and ensure there is only one info (height)
    core::Query query;
    auto cur = f.tr->query_station_data(query);
    wassert(actual(cur->remaining()) == 1);
    cur->next();
    wassert(actual(cur).station_keys_match(ds.stations["synop"].station));
    wassert(actual(cur).data_var_matches(ds.stations["synop"].values));
});
this->add_method("insert_stationinfo_twice1", [](Fixture& f) {
    // Test double insert of station info
    NavileDataSet ds;
    ds.stations["metar"] = ds.stations["synop"];
    ds.stations["metar"].station.report = "metar";
    f.tr->insert_station_data(ds.stations["synop"], true, true);
    f.tr->insert_station_data(ds.stations["metar"], true, true);

    // Query station data and ensure there is only one info (height)
    core::Query query;
    auto cur = f.tr->query_station_data(query);
    wassert(actual(cur->remaining()) == 2);

    // Ensure that the network info is preserved
    // Use a sorted vector because while all DBs group by report, not all DBs
    // sort by report name.
    vector<string> reports;
    while (cur->next())
        reports.push_back(cur->get_station().report);
    std::sort(reports.begin(), reports.end());
    wassert(actual(reports[0]) == "metar");
    wassert(actual(reports[1]) == "synop");
});
this->add_method("insert_undefined_level2", [](Fixture& f) {
    // Test handling of values with undefined leveltype2 and l2
    OldDballeTestDataSet oldf;

    // Insert with undef leveltype2 and l2
    core::Data dataset;
    dataset.station = oldf.data["synop"].station;
    dataset.datetime = oldf.data["synop"].datetime;
    dataset.level = Level(44, 55);
    dataset.trange = Trange(20);
    dataset.values.set("B01012", 300);
    f.tr->insert_data(dataset, true, true);

    // Query it back
    auto cur = f.tr->query_data(*query_from_string("leveltype1=44, l1=55"));
    wassert(actual(cur->remaining()) == 1);
    wassert(actual(cur->next()).istrue());
    wassert(actual(cur->get_level()) == Level(44, 55));
    wassert(actual(cur->get_trange()) == Trange(20));
    wassert(actual(cur->next()).isfalse());
});
this->add_method("query_undefined_level2", [](Fixture& f) {
    // Test handling of values with undefined leveltype2 and l2
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    // Query with undef leveltype2 and l2
    auto cur = f.tr->query_data(*query_from_string("leveltype1=10, l1=11"));
    wassert(actual(cur->remaining()) == 4);
    cur->discard();
});
this->add_method("query_bad_attrfilter", [](Fixture& f) {
    // Query with an incorrect attr_filter
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    try {
        f.tr->query_data(*query_from_string("attr_filter=B12001"));
    } catch (error_consistency& e) {
        wassert(actual(e.what()).matches("B12001 is not a valid filter|cannot find any operator in filter 'B12001'"));
    }
});
this->add_method("query_best_priomax", [](Fixture& f) {
    // Test querying priomax together with query=best
    // Prepare the common parts of some data
    core::Data insert;
    insert.station.coords = Coords(1.0, 1.0);
    insert.level = Level(1, 0);
    insert.trange = Trange(254, 0, 0);
    insert.datetime = Datetime(2009, 11, 11, 0, 0, 0);

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
        insert.station.report = *i;
        insert.values.set("B12101", (int)(i - rep_memos));
        f.tr->insert_data(insert, false, true);
    }

    // Query with querybest only
    {
        core::Query query;
        query.query = "best";
        query.datetime = DatetimeRange(Datetime(2009, 11, 11, 0, 0, 0), Datetime(2009, 11, 11, 0, 0, 0));
        query.varcodes.insert(WR_VAR(0, 12, 101));
        auto cur = f.tr->query_data(query);
        wassert(actual(cur->remaining()) == 1);
        wassert(actual(cur->next()).istrue());
        wassert(actual(cur->get_station().report) == "generic");
        cur->discard();
    }

    // Query with querybest and priomax
    {
        core::Query query;
        query.prio_max = 100;
        query.query = "best";
        query.datetime = DatetimeRange(Datetime(2009, 11, 11, 0, 0, 0), Datetime(2009, 11, 11, 0, 0, 0));
        query.varcodes.insert(WR_VAR(0, 12, 101));
        auto cur = f.tr->query_data(query);
        wassert(actual(cur->remaining()) == 1);

        wassert(actual(cur->next()).istrue());
        wassert(actual(cur->get_station().report) == "tempship");

        cur->discard();
    }
});
this->add_method("query_repmemo_in_results", [](Fixture& f) {
    // Ensure that rep_memo is set in the results
    OldDballeTestDataSet oldf;
    wassert(f.populate(oldf));

    auto cur = f.tr->query_data(core::Query());
    while (cur->next())
        wassert_false(cur->get_station().report.empty());
});

}

template<typename DB>
void CommitTests<DB>::register_tests() {

this->add_method("fd_leaks", [](Fixture& f) {
    // Test connect leaks
    core::Data vals;
    // Set station data
    vals.station.coords = Coords(12.34560, 76.54320);
    vals.station.report = "synop";
    vals.values.set("B07030", 42.0); // Height

    // Assume a max open file limit of 1100
    for (unsigned i = 0; i < 1100; ++i)
    {
        auto db = DB::create_db(f.backend);
        vals.clear_ids();
        wassert(db->insert_station_data(vals, true, true));
    }
});

}

}
