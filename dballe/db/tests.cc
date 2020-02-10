#include "tests.h"
#include "v7/db.h"
#include "v7/transaction.h"
#include "v7/driver.h"
#include "dballe/sql/sql.h"
#include "dballe/sql/sqlite.h"
#include "dballe/db/summary_memory.h"
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

impl::Messages messages_from_db(std::shared_ptr<db::Transaction> tr, const dballe::Query& query)
{
    impl::Messages res;
    auto cursor = tr->query_messages(query);
    while (cursor->next())
        res.emplace_back(cursor->detach_message());
    return res;
}

impl::Messages messages_from_db(std::shared_ptr<db::Transaction> tr, const char* query)
{
    impl::Messages res;
    auto cursor = tr->query_messages(*dballe::tests::query_from_string(query));
    while (cursor->next())
        res.emplace_back(cursor->detach_message());
    return res;
}


void ActualCursor::station_keys_match(const DBStation& expected)
{
    wassert(actual(_actual.get_station()) == expected);
}

void ActualCursor::data_context_matches(const Data& expected)
{
    db::CursorData* c = dynamic_cast<db::CursorData*>(&_actual);
    if (!c) throw TestFailed("cursor is not an instance of CursorData");

    const core::Data& exp = core::Data::downcast(expected);
    wassert(actual(_actual).station_keys_match(exp.station));
    wassert(actual(c->get_level()) == exp.level);
    wassert(actual(c->get_trange()) == exp.trange);
    wassert(actual(c->get_datetime()) == exp.datetime);

    wassert(actual(c->get_station().report) == exp.station.report);
    wassert(actual(c->get_level()) == exp.level);
    wassert(actual(c->get_trange()) == exp.trange);
    wassert(actual(c->get_datetime()) == exp.datetime);
}

void ActualCursor::data_var_matches(const wreport::Var& expected)
{
    if (db::CursorStationData* c = dynamic_cast<db::CursorStationData*>(&_actual))
    {
        wassert(actual(c->get_varcode()) == expected.code());
        wassert(actual(c->get_var()) == expected);
    }
    else if (db::CursorData* c = dynamic_cast<db::CursorData*>(&_actual))
    {
        wassert(actual(c->get_varcode()) == expected.code());
        wassert(actual(c->get_var()) == expected);
    }
    else
        throw TestFailed("cursor is not an instance of CursorValue");
}

void ActualCursor::data_matches(const Data& ds, wreport::Varcode code)
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
    unique_ptr<Cursor> cur = this->_actual->query_data(query);

    // Check the number of results
    wassert(actual(cur->remaining()) == expected);
    unsigned count = dynamic_cast<db::CursorData*>(cur.get())->test_iterate(/* stderr */);
    wassert(actual(count) == expected);
}

template<typename DB>
void ActualDB<DB>::try_station_query(const std::string& query, unsigned expected)
{
    // Run the query
    unique_ptr<Cursor> cur = this->_actual->query_stations(core_query_from_string(query));

    // Check the number of results
    wassert(actual(cur->remaining()) == expected);
    unsigned count = dynamic_cast<db::CursorStation*>(cur.get())->test_iterate(/* stderr */);
    wassert(actual(count) == expected);
}

template<typename DB>
void ActualDB<DB>::try_summary_query(const std::string& query, unsigned expected, result_checker check_results)
{
    // Run the query
    auto cur = this->_actual->query_summary(core_query_from_string(query));

    // Check the number of results
    // query_summary counts results in advance only optionally
    if (cur->remaining() != 0)
        wassert(actual(cur->remaining()) == expected);

    db::DBSummaryMemory summary;
    unsigned found = 0;
    while (cur->next())
    {
        ++found;
        summary.add_cursor(*cur);
    }
    wassert(actual(found) == expected);

    if (check_results)
        wassert(check_results(summary));
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
    db = DB::create_db(backend, true);
    /*
    if (auto d = dynamic_cast<db::v7::DB*>(db.get()))
        if (auto c = dynamic_cast<sql::SQLiteConnection*>(d->conn))
            c->trace();
    */
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
    auto tr = dynamic_pointer_cast<db::Transaction>(this->db->transaction());
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

std::shared_ptr<dballe::db::v7::DB> V7DB::create_db(const std::string& backend, bool wipe)
{
    auto options = DBConnectOptions::test_create(backend.c_str());
    options->wipe = wipe;
    return std::dynamic_pointer_cast<dballe::db::v7::DB>(dballe::DB::connect(*options));
}


void TestDataSet::populate_db(dballe::DB& db)
{
    auto tr = db.transaction();
    populate_transaction(*tr);
    tr->commit();
}

void TestDataSet::populate_transaction(Transaction& tr)
{
    // TODO: do everything in a single batch
    impl::DBInsertOptions opts;
    opts.can_replace = true;
    opts.can_add_stations = true;
    for (auto& d: stations)
        wassert(tr.insert_station_data(d.second, opts));
    for (auto& d: data)
        wassert(tr.insert_data(d.second, opts));
}

OldDballeTestDataSet::OldDballeTestDataSet()
{
    stations["synop"].station.report = "synop";
    stations["synop"].station.coords = Coords(12.34560, 76.54320);
    stations["synop"].values.set("B07030", 42);     // Height
    stations["synop"].values.set("B07031", 234);    // Heightbaro
    stations["synop"].values.set("B01001", 1);      // Block
    stations["synop"].values.set("B01002", 52);     // Station
    stations["synop"].values.set("B01019", "Cippo Lippo");  // Name

    stations["metar"] = stations["synop"];
    stations["metar"].station.report = "metar";

    data["synop"].station = stations["synop"].station;
    data["synop"].level = Level(10, 11, 15, 22);
    data["synop"].trange = Trange(20, 111, 122);
    data["synop"].datetime = Datetime(1945, 4, 25, 8);
    data["synop"].values.set("B01011", "DB-All.e!");
    data["synop"].values.set("B01012", 300);

    data["metar"].station = stations["metar"].station;
    data["metar"].level = Level(10, 11, 15, 22);
    data["metar"].trange = Trange(20, 111, 123);
    data["metar"].datetime = Datetime(1945, 4, 25, 8, 30);
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
