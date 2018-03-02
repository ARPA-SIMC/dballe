#include <dballe/msg/tests.h>
#include <dballe/core/record.h>
#include <dballe/core/values.h>
#include <dballe/db/db.h>
#include <dballe/sql/fwd.h>
#include <utility>
#include <functional>

namespace dballe {
struct DB;

namespace db {

namespace v6 {
struct Driver;
class DB;
}

namespace v7 {
struct Driver;
class DB;
}

}

namespace tests {

Messages messages_from_db(DB& db, const dballe::Query& query);
Messages messages_from_db(DB& db, const char* query);

/// Base for datasets used to populate test databases
struct TestDataSet
{
    /// Arbitrarily named station values
    std::map<std::string, StationValues> stations;
    /// Arbitrarily named data values
    std::map<std::string, DataValues> data;

    TestDataSet() {}
    virtual ~TestDataSet() {}

    virtual void populate_db(DB& db);
};

/// Test fixture used by old DB-All.e db tests
struct OldDballeTestDataSet : public TestDataSet
{
    OldDballeTestDataSet();
};

bool has_driver(const std::string& backend);

template<typename DB>
struct BaseDBFixture : public Fixture
{
    std::string backend;
    db::Format format;
    std::shared_ptr<DB> db;

    BaseDBFixture(const char* backend, db::Format format);
    ~BaseDBFixture();

    /// Open a new DB with the backend and format specified in this fixture
    virtual std::shared_ptr<DB> create_db() = 0;

    void test_setup();
    bool has_driver();

    template<typename DataSet>
    void populate()
    {
        DataSet data_set;
        wassert(populate_database(data_set));
    }

    void populate_database(TestDataSet& data_set);
};

struct DBFixture : public BaseDBFixture<dballe::DB>
{
    DBFixture(const char* backend, db::Format format)
        : BaseDBFixture(backend, format)
    {
    }

    std::shared_ptr<dballe::DB> create_db() override;
};

struct V6DBFixture : public BaseDBFixture<dballe::db::v6::DB>
{
    V6DBFixture(const char* backend)
        : BaseDBFixture(backend, db::Format::V6)
    {
    }

    std::shared_ptr<dballe::db::v6::DB> create_db() override;
};

struct V7DBFixture : public BaseDBFixture<dballe::db::v7::DB>
{
    V7DBFixture(const char* backend)
        : BaseDBFixture(backend, db::Format::V7)
    {
    }

    std::shared_ptr<dballe::db::v7::DB> create_db() override;
};

template<typename FIXTURE>
struct DBFixtureTestCase : public FixtureTestCase<FIXTURE>
{
    using FixtureTestCase<FIXTURE>::FixtureTestCase;
};

struct ActualCursor : public Actual<dballe::db::Cursor&>
{
    using Actual::Actual;

    /// Check cursor context after a query_stations
    void station_keys_match(const Station& expected);

    /// Check cursor context after a query_stations
    void station_vars_match(const StationValues& expected);

    /// Check cursor data context after a query_data
    void data_context_matches(const DataValues& expected);

    /// Check cursor data variable after a query_data
    void data_var_matches(const StationValues& expected, wreport::Varcode code) {
        data_var_matches(*expected.values[code].var);
    }
    /// Check cursor data variable after a query_data
    void data_var_matches(const DataValues& expected, wreport::Varcode code) {
        data_var_matches(*expected.values[code].var);
    }
    /// Check cursor data variable after a query_data
    void data_var_matches(const DataValues& expected) {
        if (auto c = dynamic_cast<dballe::db::CursorValue*>(&_actual))
            data_var_matches(*expected.values[c->get_varcode()].var);
        else
            throw wreport::error_consistency("cannot call data_var_matches on this kind of cursor");
    }
    /// Check cursor data variable after a query_data
    void data_var_matches(const Values& expected, wreport::Varcode code) {
        data_var_matches(*expected[code].var);
    }
    /// Check cursor data variable after a query_data
    void data_var_matches(const wreport::Var& expected);

    /// Check cursor data context and variable after a query_data
    void data_matches(const DataValues& ds)
    {
        if (auto c = dynamic_cast<dballe::db::CursorValue*>(&_actual))
            data_matches(ds, c->get_varcode());
        else
            throw wreport::error_consistency("cannot call data_matches on this kind of cursor");
    }
    /// Check cursor data context and variable after a query_data
    void data_matches(const DataValues& ds, wreport::Varcode code);
};

typedef std::function<void(const std::vector<core::Record>&)> result_checker;

struct ActualDB : public Actual<dballe::DB&>
{
    using Actual::Actual;

    /// Check cursor data context anda variable after a query_data
    void try_data_query(const std::string& query, unsigned expected);

    /// Check cursor data context anda variable after a query_data
    void try_data_query(const Query& query, unsigned expected);

    /// Check results of a station query
    void try_station_query(const std::string& query, unsigned expected);

    /// Check results of a summary query
    void try_summary_query(const std::string& query, unsigned expected, result_checker checker=nullptr);
};

inline ActualCursor actual(dballe::db::Cursor& actual) { return ActualCursor(actual); }
inline ActualCursor actual(dballe::db::CursorStation& actual) { return ActualCursor(actual); }
inline ActualCursor actual(dballe::db::CursorStationData& actual) { return ActualCursor(actual); }
inline ActualCursor actual(dballe::db::CursorData& actual) { return ActualCursor(actual); }
inline ActualCursor actual(dballe::db::CursorSummary& actual) { return ActualCursor(actual); }
inline ActualCursor actual(std::unique_ptr<dballe::db::Cursor>& actual) { return ActualCursor(*actual); }
inline ActualCursor actual(std::unique_ptr<dballe::db::CursorStation>& actual) { return ActualCursor(*actual); }
inline ActualCursor actual(std::unique_ptr<dballe::db::CursorStationData>& actual) { return ActualCursor(*actual); }
inline ActualCursor actual(std::unique_ptr<dballe::db::CursorData>& actual) { return ActualCursor(*actual); }
inline ActualCursor actual(std::unique_ptr<dballe::db::CursorSummary>& actual) { return ActualCursor(*actual); }
inline ActualDB actual(dballe::DB& actual) { return ActualDB(actual); }
inline ActualDB actual(std::shared_ptr<dballe::DB>& actual) { return ActualDB(*actual); }

extern template class BaseDBFixture<dballe::DB>;
extern template class BaseDBFixture<dballe::db::v6::DB>;
extern template class BaseDBFixture<dballe::db::v7::DB>;

}
}
