/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include <dballe/msg/test-utils-msg.h>
#include <dballe/db/db.h>
#include <dballe/db/sql/driver.h>

namespace dballe {
struct DB;

namespace db {
struct Connection;

namespace sql {
struct Driver;
}
namespace v5 {
class DB;
}
namespace v6 {
class DB;
}
}

namespace tests {

struct OverrideTestDBFormat
{
    dballe::db::Format old_format;
    OverrideTestDBFormat(dballe::db::Format fmt);
    ~OverrideTestDBFormat();
};

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

/// Fixture data about a station
struct TestStation
{
    double lat;
    double lon;
    std::string ident;
    /// Station information variables for each network
    std::map<std::string, Record> info;

    /// Set our lat, lon and indent into the given record
    void set_latlonident_into(Record& rec) const;

    /**
     * Get a record with all info variables. In case of conflict, keeps only
     * those with highest priority.
     *
     * @param db
     *   Database used to read priority information
     */
    Record merged_info_with_highest_prio(DB& db) const;

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
    TestStation station;
    /// Measured variables context, measured variables
    Record data;
    /// Attributes on measured variables
    std::map<wreport::Varcode, Record> attrs;
    /// ana_id of this station after inserting it
    int ana_id;

    /// Set a value as identified by the Msg ID, with its level, timerange and
    /// varcode, at the given date and with an optional confidence %
    void set_var(const char* msgvarname, float val, int conf=-1);

    void insert(WIBBLE_TEST_LOCPRM, DB& db, bool can_replace=false);
};

/// Base for all database initialization data fixtures
struct TestFixture
{
    TestRecord* records;
    size_t records_count;

    TestFixture(size_t records_count)
        : records(new TestRecord[records_count]),
          records_count(records_count) {}
    virtual ~TestFixture()
    {
        if (records) delete[] records;
    }

    virtual void populate_db(WIBBLE_TEST_LOCPRM, DB& db) const;

private:
    TestFixture(const TestFixture&);
    TestFixture& operator=(const TestFixture&);
};

/// Test fixture used by old DB-All.e db tests
struct OldDballeTestFixture : public TestFixture
{
    TestStation ds_st_oldtests;
    TestRecord& dataset0;
    TestRecord& dataset1;

    OldDballeTestFixture();
};

/// Check cursor context after a query_stations
struct TestCursorStationKeys
{
    db::Cursor& cur;
    const TestStation& ds;

    TestCursorStationKeys(db::Cursor& cur, const TestStation& ds) : cur(cur), ds(ds) {}

    void check(WIBBLE_TEST_LOCPRM) const;
};

/// Check cursor context after a query_stations
struct TestCursorStationVars
{
    db::Cursor& cur;
    const TestStation& ds;

    TestCursorStationVars(db::Cursor& cur, const TestStation& ds) : cur(cur), ds(ds) {}

    void check(WIBBLE_TEST_LOCPRM) const;
};

/// Check cursor data context after a query_data
struct TestCursorDataContext
{
    db::Cursor& cur;
    const TestRecord& ds;

    TestCursorDataContext(db::Cursor& cur, const TestRecord& ds) : cur(cur), ds(ds) {}

    void check(WIBBLE_TEST_LOCPRM) const;
};

/// Check cursor data variable after a query_data
struct TestCursorDataVar
{
    db::Cursor& cur;
    const TestRecord& ds;
    wreport::Varcode code;

    TestCursorDataVar(db::Cursor& cur, const TestRecord& ds, wreport::Varcode code) : cur(cur), ds(ds), code(code) {}

    void check(WIBBLE_TEST_LOCPRM) const;
};

/// Check cursor data context anda variable after a query_data
struct TestCursorDataMatch
{
    db::Cursor& cur;
    const TestRecord& ds;
    wreport::Varcode code;

    TestCursorDataMatch(db::Cursor& cur, const TestRecord& ds, wreport::Varcode code) : cur(cur), ds(ds), code(code) {}

    void check(WIBBLE_TEST_LOCPRM) const;
};

/// Check cursor data context anda variable after a query_data
struct TestDBTryDataQuery
{
    DB& db;
    Query query;
    unsigned expected;

    TestDBTryDataQuery(DB& db, const std::string& query, unsigned expected);
    TestDBTryDataQuery(DB& db, const Query& query, unsigned expected) : db(db), query(query), expected(expected) {}

    void check(WIBBLE_TEST_LOCPRM) const;
};

/// Check cursor data context anda variable after a query_data
struct TestDBTryStationQuery
{
    DB& db;
    std::string query;
    unsigned expected;

    TestDBTryStationQuery(DB& db, const std::string& query, unsigned expected) : db(db), query(query), expected(expected) {}

    void check(WIBBLE_TEST_LOCPRM) const;
};

/// Check cursor data context anda variable after a query_data
struct TestDBTrySummaryQuery
{
    typedef std::function<void(wibble::tests::Location, const std::vector<Record>&)> result_checker;
    DB& db;
    std::string query;
    unsigned expected;
    result_checker check_results;

    TestDBTrySummaryQuery(DB& db, const std::string& query, unsigned expected, result_checker check_results=nullptr)
        : db(db), query(query), expected(expected), check_results(check_results) {}

    void check(WIBBLE_TEST_LOCPRM) const;
};

std::unique_ptr<db::Connection> get_test_connection(const char* backend);

/// Test fixture for SQL backend drivers
struct DriverFixture
{
    static const char* backend;
    static db::Format format;

    db::Connection* conn = nullptr;
    db::sql::Driver* driver = nullptr;

    DriverFixture();
    ~DriverFixture();
    void reset();
};

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


struct DBFixture
{
    static const char* backend;
    static db::Format format;

    DB* db = nullptr;

    DBFixture();
    ~DBFixture();
    void reset();

    std::unique_ptr<DB> create_db();

    template<typename FIXTURE>
    void populate(WIBBLE_TEST_LOCPRM)
    {
        FIXTURE fixture;
        wruntest(populate_database, fixture);
    }

    void populate_database(WIBBLE_TEST_LOCPRM, const TestFixture& fixture);
};

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


struct ActualCursor : public wibble::tests::Actual<dballe::db::Cursor&>
{
    ActualCursor(dballe::db::Cursor& actual) : wibble::tests::Actual<dballe::db::Cursor&>(actual) {}

    TestCursorStationKeys station_keys_match(const TestStation& ds) { return TestCursorStationKeys(this->actual, ds); }
    TestCursorStationVars station_vars_match(const TestStation& ds) { return TestCursorStationVars(this->actual, ds); }
    TestCursorStationVars station_vars_match(const TestRecord& ds) { return TestCursorStationVars(this->actual, ds.station); }
    TestCursorDataContext data_context_matches(const TestRecord& ds) { return TestCursorDataContext(this->actual, ds); }
    TestCursorDataVar data_var_matches(const TestRecord& ds) { return TestCursorDataVar(this->actual, ds, this->actual.get_varcode()); }
    TestCursorDataVar data_var_matches(const TestRecord& ds, wreport::Varcode code) { return TestCursorDataVar(this->actual, ds, code); }
    TestCursorDataMatch data_matches(const TestRecord& ds) { return TestCursorDataMatch(this->actual, ds, this->actual.get_varcode()); }
    TestCursorDataMatch data_matches(const TestRecord& ds, wreport::Varcode code) { return TestCursorDataMatch(this->actual, ds, code); }
};

struct ActualDB : public wibble::tests::Actual<dballe::DB&>
{
    ActualDB(dballe::DB& actual) : wibble::tests::Actual<dballe::DB&>(actual) {}

    TestDBTryDataQuery try_data_query(const std::string& query, unsigned expected) { return TestDBTryDataQuery(this->actual, query, expected); }
    TestDBTryDataQuery try_data_query(const Query& query, unsigned expected) { return TestDBTryDataQuery(this->actual, query, expected); }
    TestDBTryStationQuery try_station_query(const std::string& query, unsigned expected) { return TestDBTryStationQuery(this->actual, query, expected); }
    TestDBTrySummaryQuery try_summary_query(const std::string& query, unsigned expected, TestDBTrySummaryQuery::result_checker checker=nullptr) { return TestDBTrySummaryQuery(this->actual, query, expected, checker); }
};

} // namespace tests
} // namespace dballe

namespace wibble {
namespace tests {

inline dballe::tests::ActualCursor actual(dballe::db::Cursor& actual) { return dballe::tests::ActualCursor(actual); }
inline dballe::tests::ActualCursor actual(std::unique_ptr<dballe::db::Cursor>& actual) { return dballe::tests::ActualCursor(*actual); }
inline dballe::tests::ActualDB actual(dballe::DB& actual) { return dballe::tests::ActualDB(actual); }
inline dballe::tests::ActualDB actual(std::unique_ptr<dballe::DB>& actual) { return dballe::tests::ActualDB(*actual); }

}
}

// vim:set ts=4 sw=4:
