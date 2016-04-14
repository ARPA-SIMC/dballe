#include "db/tests.h"
#include "summary.h"
#include "config.h"

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
        add_method("summary", [](Fixture& f) {
            // Test building a summary and checking if it supports queries
            wassert(f.populate<OldDballeTestDataSet>());

            core::Query query;
            query.query = "details";
            Summary s(query);
            wassert(actual(s.is_valid()).isfalse());

            // Build the whole db summary
            auto cur = f.db->query_summary(query);
            while (cur->next())
                s.add_summary(*cur, true);

            // Check its contents
            wassert(actual(s.is_valid()).istrue());
            switch (f.db->format())
            {
                case V6:
                    wassert(actual(s.all_stations.size()) == 1);
                    break;
                case V7:
                default:
                    wassert(actual(s.all_stations.size()) == 2);
                    break;
            }
            wassert(actual(s.all_levels.size()) == 1);
            wassert(actual(s.all_tranges.size()) == 2);
            wassert(actual(s.all_varcodes.size()) == 2);
            wassert(actual(s.datetime_min()) == Datetime(1945, 4, 25, 8));
            wassert(actual(s.datetime_max()) == Datetime(1945, 4, 25, 8, 30));
            wassert(actual(s.data_count()) == 4);

            // Check what it can support

            // An existing station is ok: we know we have it
            wassert(actual(s.supports(*query_from_string("ana_id=1"))) == summary::Support::EXACT);

            // A non-existing station is also ok: we know we don't have it
            wassert(actual(s.supports(*query_from_string("ana_id=2"))) == summary::Support::EXACT);

            wassert(actual(s.supports(*query_from_string("ana_id=1, leveltype1=10"))) == summary::Support::EXACT);

            wassert(actual(s.supports(*query_from_string("ana_id=1, leveltype1=10, pindicator=20"))) == summary::Support::EXACT);

            wassert(actual(s.supports(*query_from_string("ana_id=1, leveltype1=10, pindicator=20"))) == summary::Support::EXACT);

            // Still exact, because the query matches the entire summary
            wassert(actual(s.supports(*query_from_string("yearmin=1945"))) == summary::Support::EXACT);

            // Still exact, because although the query partially matches the summary,
            // each summary entry is entier included completely or excluded completely
            wassert(actual(s.supports(*query_from_string("yearmin=1945, monthmin=4, daymin=25, hourmin=8, yearmax=1945, monthmax=4, daymax=25, hourmax=8, minumax=10"))) == summary::Support::EXACT);
        });
        add_method("summary_stack", [](Fixture& f) {
            // Test summary::Stack
            using namespace summary;

            wassert(f.populate<OldDballeTestDataSet>());

            core::Query query;
            query.query = "details";
            Summary s(query);
            wassert(actual(s.is_valid()).isfalse());

            Stack stack;

            // Build the whole db summary
            Summary& general = stack.push(core::Query());
            auto cur = f.db->query_summary(query);
            while (cur->next())
                general.add_summary(*cur, true);

            wassert(actual(stack.size()) == 1);
            wassert(actual(stack.top().data_count()) == 4);

            // Query the stack
            query.clear(); query.rep_memo = "synop";
            Support res = stack.query(query, true, [](const Entry& e) { return e.rep_memo == "synop"; });
            wassert(actual(res) == EXACT);

            wassert(actual(stack.size()) == 2);
            wassert(actual(stack.top().data_count()) == 2);

            // Query further
            query.clear(); query.rep_memo = "synop"; query.varcodes.insert(WR_VAR(0, 1, 11));
            res = stack.query(query, true, [](const Entry& e) { return e.rep_memo == "synop" && e.varcode == WR_VAR(0, 1, 11); });
            wassert(actual(res) == EXACT);

            wassert(actual(stack.size()) == 2);
            wassert(actual(stack.top().data_count()) == 1);

            // Query the same var but a different rep_memo
            query.clear(); query.rep_memo = "metar"; query.varcodes.insert(WR_VAR(0, 1, 11));
            res = stack.query(query, true, [](const Entry& e) { return e.rep_memo == "metar" && e.varcode == WR_VAR(0, 1, 11); });
            wassert(actual(res) == EXACT);

            wassert(actual(stack.size()) == 2);
            wassert(actual(stack.top().data_count()) == 1);
        });
    }
};

Tests tg1("db_summary_mem", nullptr, db::MEM);
Tests tg2("db_summary_v6_sqlite", "SQLITE", db::V6);
#ifdef HAVE_ODBC
Tests tg3("db_summary_v6_odbc", "ODBC", db::V6);
#endif
#ifdef HAVE_LIBPQ
Tests tg4("db_summary_v6_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests tg5("db_summary_v6_mysql", "MYSQL", db::V6);
#endif
Tests tg6("db_summary_v7_sqlite", "SQLITE", db::V7);
#ifdef HAVE_LIBPQ
Tests tg7("db_summary_v7_postgresql", "POSTGRESQL", db::V7);
#endif
#ifdef HAVE_MYSQL
Tests tg8("db_summary_v7_mysql", "MYSQL", db::V7);
#endif

}
