#include "config.h"
#include "tests.h"
#include "dballe/db/v6/db.h"
#include "dballe/db/sql.h"
#include "dballe/db/sql/driver.h"
#include "dballe/msg/vars.h"
#include <wreport/error.h>
#include <wibble/string.h>
#include <algorithm>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace wreport;
using namespace std;
using namespace wibble;
using namespace wibble::tests;

namespace dballe {
namespace tests {

OverrideTestDBFormat::OverrideTestDBFormat(dballe::db::Format fmt)
    : old_format(DB::get_default_format())
{
    DB::set_default_format(fmt);
}

OverrideTestDBFormat::~OverrideTestDBFormat()
{
    DB::set_default_format(old_format);
}

Messages messages_from_db(DB& db, const dballe::Query& query)
{
    Messages res;
    db.export_msgs(query, [&](unique_ptr<Message>&& msg) {
        res.append(move(msg));
        return true;
    });
    return res;
}

Messages messages_from_db(DB& db, const char* query)
{
    Messages res;
    db.export_msgs(*dballe::tests::query_from_string(query), [&](unique_ptr<Message>&& msg) {
        res.append(move(msg));
        return true;
    });
    return res;
}

#if 0
void StationValues::set_latlonident_into(Record& rec) const
{
    rec.set("lat", lat);
    rec.set("lon", lon);
    if (!ident.empty())
        rec.set("ident", ident);
    else
        rec.unset("ident");
}

core::Record TestStation::merged_info_with_highest_prio(DB& db) const
{
    core::Record res;
    map<string, int> prios = db.get_repinfo_priorities();
    map<wreport::Varcode, int> cur_prios;
    for (const auto& i: info)
    {
        int prio = prios[i.first];
        const vector<Var*>& vars = i.second.vars();
        for (vector<Var*>::const_iterator v = vars.begin(); v != vars.end(); ++v)
        {
            map<wreport::Varcode, int>::const_iterator cur_prio = cur_prios.find((*v)->code());
            if (cur_prio == cur_prios.end() || cur_prio->second < prio)
            {
                res.set(**v);
                cur_prios[(*v)->code()] = prio;
            }
        }
    }
    return res;
}

void TestStation::insert(WIBBLE_TEST_LOCPRM, DB& db, bool can_replace)
{
    for (const auto& i: info)
    {
        WIBBLE_TEST_INFO(locinfo);
        if (i.second.vars().empty()) continue;
        core::Record insert(i.second);
        set_latlonident_into(insert);
        insert.set("rep_memo", i.first);
        insert.set_datetime(Datetime());
        insert.set_level(Level());
        insert.set_trange(Trange());
        locinfo() << insert.to_string();
        wrunchecked(db.insert(insert, can_replace, true));
    }
}

void TestRecord::insert(WIBBLE_TEST_LOCPRM, DB& db, bool can_replace)
{
    // Insert the station info
    wruntest(station.insert, db, can_replace);
    ana_id = db.last_station_id();

    // Insert variables
    if (!data.vars().empty())
    {
        WIBBLE_TEST_INFO(locinfo);
        core::Record insert(data);
        wrunchecked(station.set_latlonident_into(insert));
        locinfo() << insert.to_string();
        wrunchecked(db.insert(insert, can_replace, true));
        ana_id = db.last_station_id();
    }

    // Insert attributes
    for (const auto& i: attrs)
    {
        WIBBLE_TEST_INFO(locinfo);
        locinfo() << wreport::varcode_format(i.first) << ": " << i.second.to_string();
        wrunchecked(db.attr_insert(i.first, i.second));
    }
}

void TestRecord::set_var(const char* msgvarname, double val, int conf)
{
    int msgvarid = resolve_var(msgvarname);
    const MsgVarShortcut& v = shortcutTable[msgvarid];
    data.set(Level(v.ltype1, v.l1, v.ltype2, v.l2));
    data.set(Trange(v.pind, v.p1, v.p2));
    data.set(newvar(v.code, val));
    if (conf != -1)
        attrs[v.code].set("B33007", conf);
}
#endif

void TestCursorStationKeys::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur.get_lat()) == expected.coords.dlat());
    wassert(actual(cur.get_lon()) == expected.coords.dlon());
    if (expected.ident.is_missing())
        wassert(actual(cur.get_ident() == nullptr).istrue());
    else
        wassert(actual(cur.get_ident()) == expected.ident);
#warning Restore report checks once all DBs return rep_memo for station queries
    //wassert(actual(cur.get_rep_memo()) == expected.report);
}

void TestCursorStationVars::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur).station_keys_match(expected.info));

    auto rec = Record::create();
    cur.to_record(*rec);
    wassert(actual(*rec).vars_equal(expected.values));
}

void TestCursorDataContext::check(WIBBLE_TEST_LOCPRM) const
{
    db::CursorData* c = dynamic_cast<db::CursorData*>(&cur);
    if (!c) wibble_test_location.fail_test("cursor is not an instance of CursorData");

    wassert(actual(cur).station_keys_match(ds.info));
    wassert(actual(c->get_rep_memo()) == ds.info.report);
    wassert(actual(c->get_level()) == ds.info.level);
    wassert(actual(c->get_trange()) == ds.info.trange);
    wassert(actual(c->get_datetime()) == ds.info.datetime);

    auto rec = Record::create();
    cur.to_record(*rec);
    wassert(actual(rec->enq("rep_memo", "")) == ds.info.report);
    wassert(actual(rec->enq("leveltype1", MISSING_INT)) == ds.info.level.ltype1);
    wassert(actual(rec->enq("l1", MISSING_INT))         == ds.info.level.l1);
    wassert(actual(rec->enq("leveltype2", MISSING_INT)) == ds.info.level.ltype2);
    wassert(actual(rec->enq("l2", MISSING_INT))         == ds.info.level.l2);
    wassert(actual(rec->enq("pindicator", MISSING_INT)) == ds.info.trange.pind);
    wassert(actual(rec->enq("p1", MISSING_INT))         == ds.info.trange.p1);
    wassert(actual(rec->enq("p2", MISSING_INT))         == ds.info.trange.p2);
    wassert(actual(rec->enq("year", MISSING_INT))  == ds.info.datetime.year);
    wassert(actual(rec->enq("month", MISSING_INT)) == ds.info.datetime.month);
    wassert(actual(rec->enq("day", MISSING_INT))   == ds.info.datetime.day);
    wassert(actual(rec->enq("hour", MISSING_INT))  == ds.info.datetime.hour);
    wassert(actual(rec->enq("min", MISSING_INT))   == ds.info.datetime.minute);
    wassert(actual(rec->enq("sec", MISSING_INT))   == ds.info.datetime.second);
}

void TestCursorDataVar::check(WIBBLE_TEST_LOCPRM) const
{
    db::CursorValue* c = dynamic_cast<db::CursorValue*>(&cur);
    if (!c) wibble_test_location.fail_test("cursor is not an instance of CursorValue");

    wassert(actual(c->get_varcode()) == expected.code());
    wassert(actual(c->get_var()) == expected);
    auto rec = Record::create();
    wrunchecked(cur.to_record(*rec));
    const Var* actvar = nullptr;
    for (const auto& i: core::Record::downcast(*rec).vars())
        if (i->code() == expected.code())
            actvar = i;
    wassert(actual(actvar).istrue());
    wassert(actual(actvar->value_equals(expected)).istrue());
}

void TestCursorDataMatch::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur).data_context_matches(ds));
    wassert(actual(cur).data_var_matches(ds, code));
}

TestDBTryDataQuery::TestDBTryDataQuery(DB& db, const std::string& query, unsigned expected)
    : db(db), expected(expected)
{
    this->query.set_from_test_string(query);
}
void TestDBTryDataQuery::check(WIBBLE_TEST_LOCPRM) const
{
    // Run the query
    unique_ptr<db::Cursor> cur = db.query_data(query);

    // Check the number of results
    wassert(actual(cur->remaining()) == expected);
    unsigned count = cur->test_iterate(/* stderr */);
    wassert(actual(count) == expected);
}

void TestDBTryStationQuery::check(WIBBLE_TEST_LOCPRM) const
{
    // Run the query
    unique_ptr<db::Cursor> cur = db.query_stations(core_query_from_string(query));

    // Check the number of results
    wassert(actual(cur->remaining()) == expected);
    unsigned count = cur->test_iterate(/* stderr */);
    wassert(actual(count) == expected);
}

void TestDBTrySummaryQuery::check(WIBBLE_TEST_LOCPRM) const
{
    // Run the query
    unique_ptr<db::Cursor> cur = db.query_summary(core_query_from_string(query));

    // Check the number of results
    // query_summary counts results in advance only optionally
    if (cur->remaining() != 0)
        wassert(actual(cur->remaining()) == expected);

    vector<core::Record> results;
    while (cur->next())
    {
        results.emplace_back(core::Record());
        cur->to_record(results.back());
    }
    wassert(actual(results.size()) == expected);

    if (check_results)
    {
        // Sort the records, to make it easier to test results later
        std::sort(results.begin(), results.end(), [](const core::Record& a, const core::Record& b) {
            if (int res = a.enq("ana_id", MISSING_INT) - b.enq("ana_id", MISSING_INT)) return res < 0;
            string sa = a.enq("rep_memo", "");
            string sb = b.enq("rep_memo", "");
            if (sa < sb) return true;
            if (sa > sb) return false;
            Level la = a.get_level();
            Level lb = b.get_level();
            if (int res = la.compare(lb)) return res < 0;
            sa = a.enq("var", "");
            sb = b.enq("var", "");
            if (sa < sb) return true;
            return false;
        });

        wruntest(check_results, results);
    }
}

std::unique_ptr<db::Connection> get_test_connection(const char* backend)
{
    std::string envname = "DBA_DB";
    if (backend)
    {
        envname = "DBA_DB_";
        envname += backend;
    }
    const char* envurl = getenv(envname.c_str());
    if (envurl == NULL)
        error_consistency::throwf("Environment variable %s is not set", envname.c_str());
    return db::Connection::create_from_url(envurl);
}

const char* DriverFixture::backend = nullptr;
db::Format DriverFixture::format = db::V6;

DriverFixture::DriverFixture()
{
    conn = get_test_connection(backend).release();
    driver = db::sql::Driver::create(*conn).release();
    driver->delete_tables(format);
    driver->create_tables(format);
}

DriverFixture::~DriverFixture()
{
    if (getenv("PAUSE") == nullptr)
        driver->delete_tables(format);
    delete driver;
    delete conn;
}

void DriverFixture::reset()
{
    driver->remove_all(format);
}

const char* DBFixture::backend = nullptr;
db::Format DBFixture::format = db::V6;

DBFixture::DBFixture()
{
    db = create_db().release();
    db->reset();
}

DBFixture::~DBFixture()
{
    if (getenv("PAUSE") == nullptr)
        db->disappear();
    delete db;
}

void DBFixture::reset()
{
    db->remove_all();
    int added, deleted, updated;
    db->update_repinfo(nullptr, &added, &deleted, &updated);
}

std::unique_ptr<DB> DBFixture::create_db()
{
    if (format == db::MEM)
    {
        return DB::connect_memory();
    } else {
        OverrideTestDBFormat odbf(format);
        auto conn = get_test_connection(backend);
        return DB::create(move(conn));
    }
}


void DBFixture::populate_database(WIBBLE_TEST_LOCPRM, TestFixture& fixture)
{
    wruntest(fixture.populate_db, *db);
}


void TestFixture::populate_db(WIBBLE_TEST_LOCPRM, DB& db)
{
    for (auto& vals: stations)
        wrunchecked(db.insert_station_data(vals.second, true, true));
    for (auto& vals: data)
        wrunchecked(db.insert_data(vals.second, true, true));
}

OldDballeTestFixture::OldDballeTestFixture()
{
    stations["synop"].info.coords = Coords(12.34560, 76.54320);
    stations["synop"].info.report = "synop";
    stations["synop"].values.set("B07030", 42);     // Height
    stations["synop"].values.set("B07031", 234);    // Heightbaro
    stations["synop"].values.set("B01001", 1);      // Block
    stations["synop"].values.set("B01002", 52);     // Station
    stations["synop"].values.set("B01019", "Cippo Lippo");  // Name
    stations["metar"] = stations["synop"];
    stations["metar"].info.report = "metar";

    data["synop"].info = stations["synop"].info;
    data["synop"].info.level = Level(10, 11, 15, 22);
    data["synop"].info.trange = Trange(20, 111, 122);
    data["synop"].info.datetime = Datetime(1945, 4, 25, 8);
    data["synop"].values.set("B01011", "DB-All.e!");
    data["synop"].values.set("B01012", 300);

    data["metar"].info = stations["metar"].info;
    data["metar"].info.level = Level(10, 11, 15, 22);
    data["metar"].info.trange = Trange(20, 111, 123);
    data["metar"].info.datetime = Datetime(1945, 4, 25, 8, 30);
    data["metar"].values.set("B01011", "Arpa-Sim!");
    data["metar"].values.set("B01012", 400);
}

}
}
