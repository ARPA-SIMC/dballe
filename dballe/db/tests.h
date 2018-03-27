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
class Transaction;
}

namespace v7 {
struct Driver;
class DB;
class Transaction;
}

}

namespace tests {

Messages messages_from_db(std::shared_ptr<db::Transaction> tr, const dballe::Query& query);
Messages messages_from_db(std::shared_ptr<db::Transaction> tr, const char* query);

/// Base for datasets used to populate test databases
struct TestDataSet
{
    /// Arbitrarily named station values
    std::map<std::string, StationValues> stations;
    /// Arbitrarily named data values
    std::map<std::string, DataValues> data;

    TestDataSet() {}
    virtual ~TestDataSet() {}

    void populate_db(DB& db);
    virtual void populate_transaction(db::Transaction& tr);
};

struct EmptyTestDataset
{
    void populate_db();
};

/// Test fixture used by old DB-All.e db tests
struct OldDballeTestDataSet : public TestDataSet
{
    OldDballeTestDataSet();
};

bool has_driver(const std::string& backend);

struct V6DB
{
    typedef db::v6::DB DB;
    typedef db::v6::Transaction TR;
    static const auto format = db::V6;
    static std::shared_ptr<DB> create_db(const std::string& backend);
};

struct V7DB
{
    typedef db::v7::DB DB;
    typedef db::v7::Transaction TR;
    static const auto format = db::V7;
    static std::shared_ptr<DB> create_db(const std::string& backend);
};

template<typename DB>
struct BaseDBFixture : public Fixture
{
    std::string backend;
    std::shared_ptr<typename DB::DB> db;

    BaseDBFixture(const char* backend);
    ~BaseDBFixture();

    void test_setup();
    virtual void create_db();
    bool has_driver();
};

template<typename DB>
struct EmptyTransactionFixture : public BaseDBFixture<DB>
{
    using BaseDBFixture<DB>::BaseDBFixture;
    std::shared_ptr<typename DB::TR> tr;

    void test_setup();
    void test_teardown();
    void populate(TestDataSet& data_set);
};

template<typename DB, typename TestData>
struct TransactionFixture : public EmptyTransactionFixture<DB>
{
    using EmptyTransactionFixture<DB>::EmptyTransactionFixture;

    TestData test_data;

    void create_db() override
    {
        EmptyTransactionFixture<DB>::create_db();
        wassert(test_data.populate_db(*this->db));
    }
};

template<typename DB>
struct DBFixture : public BaseDBFixture<DB>
{
    using BaseDBFixture<DB>::BaseDBFixture;

    void test_setup();
    void populate_database(TestDataSet& data_set);
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

template<typename DB>
struct ActualDB : public Actual<std::shared_ptr<DB>>
{
    using Actual<std::shared_ptr<DB>>::Actual;

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
inline ActualDB<dballe::DB> actual(std::shared_ptr<dballe::DB> actual) { return ActualDB<dballe::DB>(actual); }
ActualDB<dballe::DB> actual(std::shared_ptr<dballe::db::v6::DB> actual);
ActualDB<dballe::DB> actual(std::shared_ptr<dballe::db::v7::DB> actual);
inline ActualDB<dballe::db::Transaction> actual(std::shared_ptr<dballe::db::Transaction> actual) { return ActualDB<dballe::db::Transaction>(actual); }
ActualDB<dballe::db::Transaction> actual(std::shared_ptr<dballe::db::v6::Transaction> actual);
ActualDB<dballe::db::Transaction> actual(std::shared_ptr<dballe::db::v7::Transaction> actual);

extern template class BaseDBFixture<V6DB>;
extern template class BaseDBFixture<V7DB>;
extern template class DBFixture<V6DB>;
extern template class DBFixture<V7DB>;
extern template class EmptyTransactionFixture<V6DB>;
extern template class EmptyTransactionFixture<V7DB>;
extern template class ActualDB<dballe::DB>;
extern template class ActualDB<dballe::db::Transaction>;

}
}
