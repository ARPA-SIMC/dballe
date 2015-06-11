#include "config.h"
#include "db/test-utils-db.h"
#include "db/mem/db.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace {

typedef dballe::tests::DBFixture Fixture;
typedef dballe::tests::db_test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    // Test simple queries
    Test("reset", [](Fixture& f) {
        // Run twice to see if it is idempotent
        auto& db = *f.db;
        db.reset();
        db.reset();
    }),
    Test("repinfo", [](Fixture& f) {
        // Test repinfo-related functions
        auto& db = *f.db;
        std::map<std::string, int> prios = db.get_repinfo_priorities();
        wassert(actual(prios.find("synop") != prios.end()).istrue());
        wassert(actual(prios["synop"]) == 101);

        int added, deleted, updated;
        db.update_repinfo((string(getenv("DBA_TESTDATA")) + "/test-repinfo1.csv").c_str(), &added, &deleted, &updated);

        wassert(actual(added) == 3);
        wassert(actual(deleted) == 11);
        wassert(actual(updated) == 2);

        prios = db.get_repinfo_priorities();
        wassert(actual(prios.find("fixspnpo") != prios.end()).istrue());
        wassert(actual(prios["fixspnpo"]) == 200);
    }),
    Test("vacuum", [](Fixture& f) {
        // Just invoke vacuum
        auto& db = *f.db;
        db.vacuum();
    }),
    Test("simple", [](Fixture& f) {
        // Test remove_all
        auto& db = *f.db;
        db.remove_all();
        std::unique_ptr<db::Cursor> cur = db.query_data(core::Query());
        wassert(actual(cur->remaining()) == 0);

        // Check that it is idempotent
        db.remove_all();
        cur = db.query_data(core::Query());
        wassert(actual(cur->remaining()) == 0);

        // Insert something
        wruntest(f.populate<OldDballeTestFixture>);

        cur = db.query_data(core::Query());
        wassert(actual(cur->remaining()) == 4);

        db.remove_all();

        cur = db.query_data(core::Query());
        wassert(actual(cur->remaining()) == 0);
    }),
    Test("stationdata", [](Fixture& f) {
        // Test adding station data for different networks
        auto& db = *f.db;
        db.reset();

        core::Record rec;

        rec.set("lat", 12.077);
        rec.set("lon", 44.600);

        // Insert two values in two networks
        rec.set(Level(103, 2000));
        rec.set(Trange::instant());
        rec.set(Datetime(2014, 1, 1, 0, 0, 0));
        rec.set("rep_memo", "synop");
        rec.set("B12101", 273.15);
        db.insert(rec, true, true);
        rec.set("rep_memo", "temp");
        rec.set("B12101", 274.15);
        db.insert(rec, true, true);

        // Insert station names in both networks
        rec.set_datetime(Datetime());
        rec.set_level(Level());
        rec.set_trange(Trange());
        rec.set("rep_memo", "synop");
        rec.set("B01019", "Camse");
        db.insert(rec, true, true);
        rec.set("rep_memo", "temp");
        rec.set("B01019", "Esmac");
        db.insert(rec, true, true);

        // Query back all the data
        auto cur = db.query_stations(core::Query());

        // Check results
        core::Record result;
        if (dynamic_cast<db::mem::DB*>(f.db))
        {
            // For mem databases, we get one record per (station, network)
            // combination
            wassert(actual(cur->next()).istrue());
            wassert(actual(cur->get_station_id()) == 1);
            wassert(actual(cur->get_rep_memo()) == "temp");
            cur->to_record(result);
            wassert(actual(result["B01019"].value()) == "Esmac");

            wassert(actual(cur->next()).istrue());
            wassert(actual(cur->get_station_id()) == 0);
            wassert(actual(cur->get_rep_memo()) == "synop");
            cur->to_record(result);
            wassert(actual(result["B01019"].value()) == "Camse");
        } else {
            // For normal databases, we only get one record, with the station
            // values merged keeping values for the best networks
            wassert(actual(cur->next()).istrue());
            wassert(actual(cur->get_station_id()) == 1);
            cur->to_record(result);
            wassert(actual(result["B01019"].value()) == "Camse");

            wassert(actual(cur->next()).isfalse());
        }

        Msgs msgs;
        msg::AcquireMessages amsg(msgs);
        db.export_msgs(core::Query(), amsg);
        wassert(actual(msgs.size()) == 2);

        //msgs.print(stderr);

        wassert(actual(msgs[0]->get_rep_memo_var()->enqc()) == "synop");
        wassert(actual(msgs[0]->get_st_name_var()->enqc()) == "Camse");
        wassert(actual(msgs[0]->get_temp_2m_var()->enqd()) == 273.15);
        wassert(actual(msgs[1]->get_rep_memo_var()->enqc()) == "temp");
        wassert(actual(msgs[1]->get_st_name_var()->enqc()) == "Esmac");
        wassert(actual(msgs[1]->get_temp_2m_var()->enqd()) == 274.15);
    }),
    Test("query_ident", [](Fixture& f) {
        // Try querying by ident
        auto& db = *f.db;

        // Insert a mobile station
        core::Record rec;
        rec.set_from_string("rep_memo=synop");
        rec.set_from_string("lat=44.10");
        rec.set_from_string("lon=11.50");
        rec.set_from_string("ident=foo");
        rec.set(Level(1));
        rec.set(Trange::instant());
        rec.set(Datetime(2015, 4, 25, 12, 30, 45));
        rec.set("B12101", 295.1);
        db.insert(rec, true, true);

        wassert(actual(db).try_station_query("ident=foo", 1));
        wassert(actual(db).try_station_query("ident=bar", 0));
        wassert(actual(db).try_station_query("mobile=1", 1));
        wassert(actual(db).try_station_query("mobile=0", 0));
        wassert(actual(db).try_data_query("ident=foo", 1));
        wassert(actual(db).try_data_query("ident=bar", 0));
        wassert(actual(db).try_data_query("mobile=1", 1));
        wassert(actual(db).try_data_query("mobile=0", 0));
    }),
};

test_group tg1("db_basic_mem", nullptr, db::MEM, tests);
test_group tg2("db_basic_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("db_basic_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_basic_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_basic_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
