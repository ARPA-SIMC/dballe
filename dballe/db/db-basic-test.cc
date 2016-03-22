#include "config.h"
#include "msg/msg.h"
#include "db/tests.h"
#include "db/mem/db.h"
#include <cstring>

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public FixtureTestCase<DBFixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        // Test simple queries
        add_method("reset", [](Fixture& f) {
            // Run twice to see if it is idempotent
            auto& db = *f.db;
            db.reset();
            db.reset();
        });
        add_method("repinfo", [](Fixture& f) {
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
        });
        add_method("vacuum", [](Fixture& f) {
            // Just invoke vacuum
            auto& db = *f.db;
            db.vacuum();
        });
        add_method("simple", [](Fixture& f) {
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
            wassert(f.populate<OldDballeTestDataSet>());

            cur = db.query_data(core::Query());
            wassert(actual(cur->remaining()) == 4);

            db.remove_all();

            cur = db.query_data(core::Query());
            wassert(actual(cur->remaining()) == 0);
        });
        add_method("stationdata", [](Fixture& f) {
            // Test adding station data for different networks
            auto& db = *f.db;
            db.reset();

            // Insert two values in two networks
            DataValues vals;
            vals.info.coords = Coords(12.077, 44.600);
            vals.info.report = "synop";
            vals.info.level = Level(103, 2000);
            vals.info.trange = Trange::instant();
            vals.info.datetime = Datetime(2014, 1, 1, 0, 0, 0);
            vals.values.set("B12101", 273.15);
            db.insert_data(vals, true, true);
            vals.clear_ids();
            vals.info.report = "temp";
            vals.values.set("B12101", 274.15);
            db.insert_data(vals, true, true);

            // Insert station names in both networks
            StationValues svals_camse;
            svals_camse.info.coords = vals.info.coords;
            svals_camse.info.report = "synop";
            svals_camse.values.set("B01019", "Camse");
            db.insert_station_data(svals_camse, true, true);
            StationValues svals_esmac;
            svals_esmac.info.coords = vals.info.coords;
            svals_esmac.info.report = "temp";
            svals_esmac.values.set("B01019", "Esmac");
            db.insert_station_data(svals_esmac, true, true);

            // Query back all the data
            auto cur = db.query_stations(core::Query());

            // Check results
            core::Record result;
            switch (db.format())
            {
                case MEM:
                case V7:
                {
                    bool have_temp = false;
                    bool have_synop = false;

                    // For mem and v7 databases, we get one record per (station, network)
                    // combination
                    for (unsigned i = 0; i < 2; ++i)
                    {
                        wassert(actual(cur->next()).istrue());
                        if (strcmp(cur->get_rep_memo(), "temp") == 0)
                        {
                            wassert(actual(cur->get_station_id()) == svals_esmac.info.ana_id);
                            cur->to_record(result);
                            wassert(actual(result["B01019"]) == "Esmac");
                            have_temp = true;
                        } else if (strcmp(cur->get_rep_memo(), "synop") == 0) {
                            wassert(actual(cur->get_station_id()) == svals_camse.info.ana_id);
                            cur->to_record(result);
                            wassert(actual(result["B01019"]) == "Camse");
                            have_synop = true;
                        }
                    }
                    wassert(actual(cur->next()).isfalse());

                    wassert(actual(have_temp).istrue());
                    wassert(actual(have_synop).istrue());
                    break;
                }
                case V6:
                    // For v6 databases, we only get one record, with the station
                    // values merged keeping values for the best networks
                    wassert(actual(cur->next()).istrue());
                    wassert(actual(cur->get_station_id()) == 1);
                    cur->to_record(result);
                    wassert(actual(result["B01019"]) == "Camse");

                    wassert(actual(cur->next()).isfalse());
                    break;
                default: throw error_unimplemented("testing stations_without_data on unsupported database");
            }

            Messages msgs;
            db.export_msgs(core::Query(), [&](unique_ptr<Message>&& msg) { msgs.append(move(msg)); return true; });
            wassert(actual(msgs.size()) == 2);

            //msgs.print(stderr);

            wassert(actual(Msg::downcast(msgs[0]).get_rep_memo_var()->enqc()) == "synop");
            wassert(actual(Msg::downcast(msgs[0]).get_st_name_var()->enqc()) == "Camse");
            wassert(actual(Msg::downcast(msgs[0]).get_temp_2m_var()->enqd()) == 273.15);
            wassert(actual(Msg::downcast(msgs[1]).get_rep_memo_var()->enqc()) == "temp");
            wassert(actual(Msg::downcast(msgs[1]).get_st_name_var()->enqc()) == "Esmac");
            wassert(actual(Msg::downcast(msgs[1]).get_temp_2m_var()->enqd()) == 274.15);
        });
        add_method("query_ident", [](Fixture& f) {
            // Try querying by ident
            auto& db = *f.db;

            // Insert a mobile station
            DataValues vals;
            vals.info.report = "synop";
            vals.info.coords = Coords(44.10, 11.50);
            vals.info.ident = "foo";
            vals.info.level = Level(1);
            vals.info.trange = Trange::instant();
            vals.info.datetime = Datetime(2015, 4, 25, 12, 30, 45);
            vals.values.set("B12101", 295.1);
            db.insert_data(vals, true, true);

            wassert(actual(db).try_station_query("ident=foo", 1));
            wassert(actual(db).try_station_query("ident=bar", 0));
            wassert(actual(db).try_station_query("mobile=1", 1));
            wassert(actual(db).try_station_query("mobile=0", 0));
            wassert(actual(db).try_data_query("ident=foo", 1));
            wassert(actual(db).try_data_query("ident=bar", 0));
            wassert(actual(db).try_data_query("mobile=1", 1));
            wassert(actual(db).try_data_query("mobile=0", 0));
        });
    }
};

Tests tg1("db_basic_mem", nullptr, db::MEM);
Tests tg2("db_basic_v6_sqlite", "SQLITE", db::V6);
#ifdef HAVE_ODBC
Tests tg4("db_basic_v6_odbc", "ODBC", db::V6);
#endif
#ifdef HAVE_LIBPQ
Tests tg6("db_basic_v6_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests tg8("db_basic_v6_mysql", "MYSQL", db::V6);
#endif
Tests tg9("db_basic_v7_sqlite", "SQLITE", db::V7);

}
