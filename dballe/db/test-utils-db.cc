#include "config.h"
#include "test-utils-db.h"
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

void TestStation::set_latlonident_into(Record& rec) const
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
        if (!i.second.vars().empty())
        {
            WIBBLE_TEST_INFO(locinfo);
            core::Record insert(i.second);
            set_latlonident_into(insert);
            insert.set("rep_memo", i.first);
            insert.set_ana_context();
            locinfo() << insert.to_string();
            wrunchecked(db.insert(insert, can_replace, true));
        }
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
    data.obtain(v.code).setd(val);
    if (conf != -1)
        attrs[v.code].set("B33007", conf);
}

void TestCursorStationKeys::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur.get_lat()) == ds.lat);
    wassert(actual(cur.get_lon()) == ds.lon);
    wassert(actual(cur.get_ident()) == ds.ident);
}

void TestCursorStationVars::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur).station_keys_match(ds));

    core::Record expected = ds.merged_info_with_highest_prio(cur.get_db());

    auto rec = Record::create();
    cur.to_record(*rec);
    wassert(actual(*rec).vars_equal(expected));
}

void TestCursorDataContext::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur).station_keys_match(ds.station));
    wassert(actual(cur.get_rep_memo()) == ds.data.enq("rep_memo", ""));
    wassert(actual(cur.get_level()) == ds.data.get_level());
    wassert(actual(cur.get_trange()) == ds.data.get_trange());
    wassert(actual(cur.get_datetime()) == ds.data.get_datetime());

    auto rec = Record::create();
    cur.to_record(*rec);
    wassert(actual(*rec).equals(ds.data, "rep_memo"));
    wassert(actual(*rec).equals_with_missing_int(ds.data, "leveltype1"));
    wassert(actual(*rec).equals_with_missing_int(ds.data, "l1"));
    wassert(actual(*rec).equals_with_missing_int(ds.data, "leveltype2"));
    wassert(actual(*rec).equals_with_missing_int(ds.data, "l2"));
    wassert(actual(*rec).equals_with_missing_int(ds.data, "pindicator"));
    wassert(actual(*rec).equals_with_missing_int(ds.data, "p1"));
    wassert(actual(*rec).equals_with_missing_int(ds.data, "p2"));
    wassert(actual(*rec).equals(ds.data, "year"));
    wassert(actual(*rec).equals(ds.data, "month"));
    wassert(actual(*rec).equals(ds.data, "day"));
    wassert(actual(*rec).equals(ds.data, "hour"));
    wassert(actual(*rec).equals(ds.data, "min"));
    wassert(actual(*rec).equals(ds.data, "sec"));
}

void TestCursorDataVar::check(WIBBLE_TEST_LOCPRM) const
{
    string scode = format_code(code);
    const Var* orig_var = ds.data.get(scode.c_str());
    wassert(actual(orig_var).istrue());
    wassert(actual(cur.get_varcode()) == code);
    wassert(actual(cur.get_var()) == *orig_var);
    auto rec = Record::create();
    cur.to_record(*rec);
    wassert(actual(*rec).equals(ds.data, scode.c_str()));
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


void DBFixture::populate_database(WIBBLE_TEST_LOCPRM, const TestFixture& fixture)
{
    wruntest(fixture.populate_db, *db);
}


void TestFixture::populate_db(WIBBLE_TEST_LOCPRM, DB& db) const
{
    for (size_t i = 0; i < records_count; ++i)
        wruntest(records[i].insert, db, true);
}

OldDballeTestFixture::OldDballeTestFixture()
    : TestFixture(2), dataset0(records[0]), dataset1(records[1])
{
    ds_st_oldtests.lat = 12.34560;
    ds_st_oldtests.lon = 76.54320;
    ds_st_oldtests.info["synop"].set("B07030", 42);     // Height
    ds_st_oldtests.info["synop"].set("B07031", 234);    // Heightbaro
    ds_st_oldtests.info["synop"].set("B01001", 1);      // Block
    ds_st_oldtests.info["synop"].set("B01002", 52);     // Station
    ds_st_oldtests.info["synop"].set("B01019", "Cippo Lippo");  // Name

    dataset0.station = ds_st_oldtests;
    dataset0.data.set("rep_memo", "synop");
    dataset0.data.set(Level(10, 11, 15, 22));
    dataset0.data.set(Trange(20, 111, 122));
    dataset0.data.set(Datetime(1945, 4, 25, 8));

    dataset1 = dataset0;
    dataset1.data.set("rep_memo", "metar");
    dataset1.data.set("min", 30);
    dataset1.data.set("p2", 123);

    dataset0.data.set("B01011", "DB-All.e!");
    dataset0.data.set("B01012", 300);
    dataset1.data.set("B01011", "Arpa-Sim!");
    dataset1.data.set("B01012", 400);
}

}
}
