#include <dballe/msg/tests.h>
#include <dballe/core/record.h>
#include <dballe/core/values.h>
#include <dballe/db/db.h>
#include <dballe/db/sql/driver.h>

namespace dballe {
struct DB;

namespace db {
struct Connection;

namespace sql {
struct Driver;
}
namespace v6 {
class DB;
}
}

namespace tests {

Messages messages_from_db(DB& db, const dballe::Query& query);
Messages messages_from_db(DB& db, const char* query);

struct OverrideTestDBFormat
{
    dballe::db::Format old_format;
    OverrideTestDBFormat(dballe::db::Format fmt);
    ~OverrideTestDBFormat();
};

#if 0
template<typename T>
struct db_tg : public tut::test_group<T>
{
    dballe::db::Format db_format;
    const char* backend = 0;
    const char* name;

    db_tg(const char* name, dballe::db::Format fmt, const char* backend=0)
        : tut::test_group<T>(name), db_format(fmt), backend(backend), name(name)
    {
    }

    tut::test_result run_next()
    {
        dballe::tests::OverrideTestDBFormat otf(db_format);
        return tut::test_group<T>::run_next();
    }
    tut::test_result run_test(int n)
    {
        dballe::tests::OverrideTestDBFormat otf(db_format);
        return tut::test_group<T>::run_test(n);
    }
};
#endif

#if 0
/// Fixture data about a station
struct TestStation
{
    double lat;
    double lon;
    std::string ident;
    /// Station information variables for each network
    std::map<std::string, core::Record> info;

    /// Set our lat, lon and indent into the given record
    void set_latlonident_into(Record& rec) const;

    /**
     * Get a record with all info variables. In case of conflict, keeps only
     * those with highest priority.
     *
     * @param db
     *   Database used to read priority information
     */
    core::Record merged_info_with_highest_prio(DB& db) const;

    /**
     * Insert the station and its info in the database.
     *
     * Note that if info is empty, nothing will happen, as a station cannot
     * exist without station vars or measured values.
     */
    void insert(WIBBLE_TEST_LOCPRM, DB& db, bool can_replace=false);
};

/// Fixture data about measured variables
struct TestRecord
{
    StationValues station;
    /// Measured variables context, measured variables
    core::Record data;
    /// Attributes on measured variables
    std::map<wreport::Varcode, core::Record> attrs;
    /// ana_id of this station after inserting it
    int ana_id;

    /// Set a value as identified by the Msg ID, with its level, timerange and
    /// varcode, at the given date and with an optional confidence %
    void set_var(const char* msgvarname, double val, int conf=-1);

    void insert(WIBBLE_TEST_LOCPRM, DB& db, bool can_replace=false);
};
#endif

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

std::unique_ptr<db::Connection> get_test_connection(const std::string& backend);

/// Test fixture for SQL backend drivers
struct DriverFixture : public Fixture
{
    std::string backend;
    db::Format format;

    db::Connection* conn = nullptr;
    db::sql::Driver* driver = nullptr;

    DriverFixture(const char* backend, db::Format format);
    ~DriverFixture();

    void test_setup();
};

#if 0
template<typename T=DriverFixture>
struct driver_test_group : public dballe::tests::test_group<T>
{
    const char* backend;
    db::Format dbformat;

    driver_test_group(const char* name, const char* backend, db::Format dbformat, const typename dballe::tests::test_group<T>::Tests& tests)
        : dballe::tests::test_group<T>(name, tests), backend(backend), dbformat(dbformat)
    {
    }

    T* create_fixture()
    {
        DriverFixture::backend = backend;
        DriverFixture::format = dbformat;
        return dballe::tests::test_group<T>::create_fixture();
    }
};
#endif

struct DBFixture : public Fixture
{
    std::string backend;
    db::Format format;

    DB* db = nullptr;

    DBFixture(const char* backend, db::Format format);
    ~DBFixture();

    /// Open a new DB with the backend and format specified in this fixture
    std::unique_ptr<DB> create_db();

    void test_setup();

    template<typename DataSet>
    void populate()
    {
        DataSet data_set;
        wassert(populate_database(data_set));
    }

    void populate_database(TestDataSet& data_set);
};

#if 0
template<typename T=DBFixture>
struct db_test_group : public dballe::tests::test_group<T>
{
    const char* backend;
    db::Format dbformat;

    db_test_group(const char* name, const char* backend, db::Format dbformat, const typename dballe::tests::test_group<T>::Tests& tests)
        : dballe::tests::test_group<T>(name, tests), backend(backend), dbformat(dbformat)
    {
    }

    T* create_fixture()
    {
        DBFixture::backend = backend;
        DBFixture::format = dbformat;
        return dballe::tests::test_group<T>::create_fixture();
    }
};
#endif

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
inline ActualDB actual(std::unique_ptr<dballe::DB>& actual) { return ActualDB(*actual); }

}
}
