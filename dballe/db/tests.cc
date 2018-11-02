#include "tests.h"
#include "v7/db.h"
#include "v7/transaction.h"
#include "v7/driver.h"
#include "dballe/sql/sql.h"
#include "dballe/sql/sqlite.h"
#include "dballe/msg/vars.h"
#include <wreport/error.h>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "config.h"

using namespace wreport;
using namespace std;

namespace dballe {
namespace tests {

namespace {

std::unique_ptr<dballe::sql::Connection> get_test_connection(const std::string& backend)
{
    std::string envname = "DBA_DB";
    if (!backend.empty())
    {
        envname = "DBA_DB_";
        envname += backend;
    }
    const char* envurl = getenv(envname.c_str());
    if (envurl == NULL)
        error_consistency::throwf("Environment variable %s is not set", envname.c_str());
    return dballe::sql::Connection::create_from_url(envurl);
}

}


Messages messages_from_db(std::shared_ptr<db::Transaction> tr, const dballe::Query& query)
{
    Messages res;
    tr->export_msgs(query, [&](unique_ptr<Message>&& msg) {
        res.emplace_back(move(msg));
        return true;
    });
    return res;
}

Messages messages_from_db(std::shared_ptr<db::Transaction> tr, const char* query)
{
    Messages res;
    tr->export_msgs(*dballe::tests::query_from_string(query), [&](unique_ptr<Message>&& msg) {
        res.emplace_back(move(msg));
        return true;
    });
    return res;
}


void ActualCursor::station_keys_match(const Station& expected)
{
    wassert(actual(_actual.get_coords()) == expected.coords);
    if (expected.ident.is_missing())
        wassert(actual(_actual.get_ident() == nullptr).istrue());
    else
        wassert(actual(_actual.get_ident()) == (const char*)expected.ident);
    wassert(actual(_actual.get_report()) == expected.report);
}

void ActualCursor::station_vars_match(const StationValues& expected)
{
    wassert(actual(_actual).station_keys_match(expected.info));

    auto rec = Record::create();
    _actual.to_record(*rec);
    wassert(actual(*rec).vars_equal(expected.values));
}

void ActualCursor::data_context_matches(const DataValues& expected)
{
    db::CursorData* c = dynamic_cast<db::CursorData*>(&_actual);
    if (!c) throw TestFailed("cursor is not an instance of CursorData");

    wassert(actual(_actual).station_keys_match(expected.info));
    wassert(actual(c->get_report()) == expected.info.report);
    wassert(actual(c->get_level()) == expected.info.level);
    wassert(actual(c->get_trange()) == expected.info.trange);
    wassert(actual(c->get_datetime()) == expected.info.datetime);

    auto rec = Record::create();
    _actual.to_record(*rec);
    wassert(actual(rec->enq("rep_memo", "")) == expected.info.report);
    wassert(actual(rec->enq("leveltype1", MISSING_INT)) == expected.info.level.ltype1);
    wassert(actual(rec->enq("l1", MISSING_INT))         == expected.info.level.l1);
    wassert(actual(rec->enq("leveltype2", MISSING_INT)) == expected.info.level.ltype2);
    wassert(actual(rec->enq("l2", MISSING_INT))         == expected.info.level.l2);
    wassert(actual(rec->enq("pindicator", MISSING_INT)) == expected.info.trange.pind);
    wassert(actual(rec->enq("p1", MISSING_INT))         == expected.info.trange.p1);
    wassert(actual(rec->enq("p2", MISSING_INT))         == expected.info.trange.p2);
    wassert(actual(rec->enq("year", MISSING_INT))  == expected.info.datetime.year);
    wassert(actual(rec->enq("month", MISSING_INT)) == expected.info.datetime.month);
    wassert(actual(rec->enq("day", MISSING_INT))   == expected.info.datetime.day);
    wassert(actual(rec->enq("hour", MISSING_INT))  == expected.info.datetime.hour);
    wassert(actual(rec->enq("min", MISSING_INT))   == expected.info.datetime.minute);
    wassert(actual(rec->enq("sec", MISSING_INT))   == expected.info.datetime.second);
}

void ActualCursor::data_var_matches(const wreport::Var& expected)
{
    db::CursorValue* c = dynamic_cast<db::CursorValue*>(&_actual);
    if (!c) throw TestFailed("cursor is not an instance of CursorValue");

    wassert(actual(c->get_varcode()) == expected.code());
    wassert(actual(c->get_var()) == expected);
    auto rec = Record::create();
    wassert(_actual.to_record(*rec));
    const Var* actvar = nullptr;
    for (const auto& i: core::Record::downcast(*rec).vars())
        if (i->code() == expected.code())
            actvar = i;
    wassert(actual(actvar).istrue());
    wassert(actual(actvar->value_equals(expected)).istrue());
}

void ActualCursor::data_matches(const DataValues& ds, wreport::Varcode code)
{
    wassert(actual(_actual).data_context_matches(ds));
    wassert(actual(_actual).data_var_matches(ds, code));
}

template<typename DB>
void ActualDB<DB>::try_data_query(const std::string& query, unsigned expected)
{
    core::Query q;
    q.set_from_test_string(query);
    try_data_query(q, expected);
}

template<typename DB>
void ActualDB<DB>::try_data_query(const Query& query, unsigned expected)
{
    // Run the query
    unique_ptr<db::Cursor> cur = this->_actual->query_data(query);

    // Check the number of results
    wassert(actual(cur->remaining()) == expected);
    unsigned count = cur->test_iterate(/* stderr */);
    wassert(actual(count) == expected);
}

template<typename DB>
void ActualDB<DB>::try_station_query(const std::string& query, unsigned expected)
{
    // Run the query
    unique_ptr<db::Cursor> cur = this->_actual->query_stations(core_query_from_string(query));

    // Check the number of results
    wassert(actual(cur->remaining()) == expected);
    unsigned count = cur->test_iterate(/* stderr */);
    wassert(actual(count) == expected);
}

template<typename DB>
void ActualDB<DB>::try_summary_query(const std::string& query, unsigned expected, result_checker check_results)
{
    // Run the query
    unique_ptr<db::Cursor> cur = this->_actual->query_summary(core_query_from_string(query));

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

        wassert(check_results(results));
    }
}

bool has_driver(const std::string& backend)
{
    std::string envname = "DBA_DB";
    if (!backend.empty())
    {
        envname = "DBA_DB_";
        envname += backend;
    }
    return getenv(envname.c_str()) != NULL;
}

template<typename DB>
BaseDBFixture<DB>::BaseDBFixture(const char* backend)
    : backend(backend ? backend : "")
{
}

template<typename DB>
BaseDBFixture<DB>::~BaseDBFixture()
{
    if (db && getenv("PAUSE") == nullptr)
        db->disappear();
}

template<typename DB>
bool BaseDBFixture<DB>::has_driver()
{
    return tests::has_driver(backend);
}

template<typename DB>
void BaseDBFixture<DB>::create_db()
{
    db = DB::create_db(backend);
    /*
    if (auto d = dynamic_cast<db::v7::DB*>(db.get()))
        if (auto c = dynamic_cast<sql::SQLiteConnection*>(d->conn))
            c->trace();
    */
    db->reset();
}

template<typename DB>
void BaseDBFixture<DB>::test_setup()
{
    Fixture::test_setup();
    if (!has_driver())
        throw TestSkipped();

    if (!db)
        create_db();
}

template<typename DB>
void EmptyTransactionFixture<DB>::populate(TestDataSet& data_set)
{
    wassert(data_set.populate_transaction(*tr));
}

template<typename DB>
void EmptyTransactionFixture<DB>::test_setup()
{
    BaseDBFixture<DB>::test_setup();
    tr = dynamic_pointer_cast<typename DB::TR>(this->db->test_transaction());
}

template<typename DB>
void EmptyTransactionFixture<DB>::test_teardown()
{
    if (tr) tr->rollback();
    tr.reset();
    BaseDBFixture<DB>::test_teardown();
}

template<typename DB>
void DBFixture<DB>::test_setup()
{
    BaseDBFixture<DB>::test_setup();
    auto tr = this->db->transaction();
    tr->remove_all();
    int added, deleted, updated;
    tr->update_repinfo(nullptr, &added, &deleted, &updated);
    tr->commit();
}

template<typename DB>
void DBFixture<DB>::populate_database(TestDataSet& data_set)
{
    wassert(data_set.populate_db(*this->db));
}

std::shared_ptr<dballe::db::v7::DB> V7DB::create_db(const std::string& backend)
{
    auto conn = get_test_connection(backend);
    return std::make_shared<dballe::db::v7::DB>(move(conn));
}


void TestDataSet::populate_db(db::DB& db)
{
    auto tr = db.transaction();
    populate_transaction(*tr);
    tr->commit();
}

void TestDataSet::populate_transaction(db::Transaction& tr)
{
    // TODO: do everything in a single batch
    for (auto& d: stations)
        wassert(tr.insert_station_data(d.second, true, true));
    for (auto& d: data)
        wassert(tr.insert_data(d.second, true, true));
}

OldDballeTestDataSet::OldDballeTestDataSet()
{
    stations["synop"].info.report = "synop";
    stations["synop"].info.coords = Coords(12.34560, 76.54320);
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

ActualDB<dballe::DB> actual(std::shared_ptr<dballe::db::v7::DB> actual)
{
    return ActualDB<dballe::DB>(std::static_pointer_cast<dballe::DB>(actual));
}
ActualDB<dballe::db::Transaction> actual(std::shared_ptr<dballe::db::v7::Transaction> actual)
{
    return ActualDB<dballe::db::Transaction>(std::static_pointer_cast<dballe::db::Transaction>(actual));
}

template class BaseDBFixture<V7DB>;
template class DBFixture<V7DB>;
template class EmptyTransactionFixture<V7DB>;
template class ActualDB<dballe::db::DB>;
template class ActualDB<dballe::db::Transaction>;

}
}
