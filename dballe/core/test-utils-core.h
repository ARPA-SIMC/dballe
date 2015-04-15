/*
 * core/test-utils-core - Test utility functions for the core module
 *
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

#include <wreport/tests.h>

#include <dballe/core/record.h>
#include <dballe/core/rawmsg.h>
#include <dballe/core/file.h>

#include <cstdlib>
#include <climits>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

namespace dballe {
namespace tests {

/**
 * Base class for test fixtures.
 *
 * A fixture will have a constructor and a destructor to do setup/teardown, and
 * a reset() function to be called inbetween tests.
 *
 * Fixtures do not need to descend from Fixture: this implementation is
 * provided as a default for tests that do not need one, or as a base for
 * fixtures that do not need reset().
 */
struct Fixture
{
    // Reset inbetween tests
    void reset() {}
};

/**
 * Check if a test can be run.
 *
 * This is used to implement extra test filtering features like glob matching
 * on group or test names.
 */
bool test_can_run(const std::string& group_name, const std::string& test_name);

/**
 * Alternative to tut::test_group.
 *
 * Tests are registered using a vector of lambdas, and it implements smarter
 * matching on group names and test names.
 *
 * There can be any number of tests.
 *
 * This could not have been implemented on top of the existing tut::test_group,
 * because all its working components are declared private and are not
 * accessible to subclasses.
 */
template<typename T=Fixture>
struct test_group : public tut::group_base
{
    typedef std::function<void(T&)> test_func_t;
    typedef T Fixture;

    struct Test
    {
        std::string name;
        test_func_t func;

        Test(const std::string& name, test_func_t func)
            : name(name), func(func) {}
    };
    typedef std::vector<Test> Tests;

    /// Name of this test group
    std::string name;
    /// Storage for all our tests.
    Tests tests;
    /// Current test when running them in sequence
    typename std::vector<Test>::iterator cur_test;
    /// Fixture for the tests
    T* fixture = 0;

    test_group(const char* name, const std::vector<Test>& tests)
        : name(name), tests(tests)
    {
        // register itself
        tut::runner.get().register_group(name, this);
    }

    virtual T* create_fixture()
    {
        return new T;
    }

    void delete_fixture()
    {
        try {
            delete fixture;
        } catch (std::exception& e) {
            fprintf(stderr, "Warning: exception caught while deleting fixture for test group %s: %s", name.c_str(), e.what());
        }
        fixture = 0;
    }

    void rewind() override
    {
        delete_fixture();
        cur_test = tests.begin();
    }

    tut::test_result run_next() override
    {
        // Skip those tests that should not run
        while (cur_test != tests.end() && !test_can_run(name, cur_test->name))
            ++cur_test;

        if (cur_test == tests.end())
        {
            delete_fixture();
            throw tut::no_more_tests();
        }

        tut::test_result res = run(cur_test);
        ++cur_test;
        return res;
    }

    // execute one test
    tut::test_result run_test(int n) override
    {
        --n; // From 1-based to 0-based
        if (n < 0 || n >= tests.size())
            throw tut::beyond_last_test();

        if (!test_can_run(name, tests[n].name))
            throw tut::beyond_last_test();

        tut::test_result res = run(tests.begin() + n);
        delete_fixture();
        return res;
    }

    tut::test_result run(typename std::vector<Test>::iterator test)
    {
        int pos = test - tests.begin() + 1;
        // Create fixture if it does not exist yet
        if (!fixture)
        {
            try {
                fixture = create_fixture();
            } catch (std::exception& e) {
                fprintf(stderr, "Warning: exception caught while creating fixture for test group %s: %s", name.c_str(), e.what());
                return tut::test_result(name, pos, tut::test_result::ex_ctor, e);
            }
        } else {
            try {
                fixture->reset();
            } catch (std::exception& e) {
                fprintf(stderr, "Warning: exception caught while resetting the fixture for test group %s: %s", name.c_str(), e.what());
                return tut::test_result(name, pos, tut::test_result::ex_ctor, e);
            }
        }
        try {
            test->func(*fixture);
        } catch (const tut::failure& e) {
            // test failed because of ensure() or similar method
            return tut::test_result(name, pos, tut::test_result::fail, e);
        } catch (const std::exception& e) {
            // test failed with std::exception
            return tut::test_result(name, pos, tut::test_result::ex, e);
        } catch (...) {
            // test failed with unknown exception
            return tut::test_result(name, pos, tut::test_result::ex);
        }

        return tut::test_result(name, pos, tut::test_result::ok);
    }
};


// Some utility random generator functions

static inline int rnd(int min, int max)
{
	return min + (int) ((max - min) * (rand() / (RAND_MAX + 1.0)));
}

static inline double rnd(double min, double max)
{
	return min + (int) ((max - min) * (rand() / (RAND_MAX + 1.0)));
}

static inline std::string rnd(int len)
{
	std::string res;
	int max = rnd(1, len);
	for (int i = 0; i < max; i++)
		res += (char)rnd('a', 'z');
	return res;
}

static inline bool rnd(double prob)
{
	return (rnd(0, 100) < prob*100) ? true : false;
}

// Message reading functions

/// Return the pathname of a test file
std::string datafile(const std::string& fname);

std::unique_ptr<File> _open_test_data(const wibble::tests::Location& loc, const char* filename, Encoding type);
#define open_test_data(filename, type) dballe::tests::_open_test_data(wibble::tests::Location(__FILE__, __LINE__, "open " #filename " " #type), (filename), (type))
#define inner_open_test_data(filename, type) dballe::tests::_open_test_data(wibble::tests::Location(loc, __FILE__, __LINE__, #filename " " #type), (filename), (type))

std::unique_ptr<Rawmsg> _read_rawmsg(const wibble::tests::Location& loc, const char* filename, Encoding type);
#define read_rawmsg(filename, type) dballe::tests::_read_rawmsg(wibble::tests::Location(__FILE__, __LINE__, "load " #filename " " #type), (filename), (type))
#define inner_read_rawmsg(filename, type) dballe::tests::_read_rawmsg(wibble::tests::Location(loc, __FILE__, __LINE__, "load " #filename " " #type), (filename), (type))

/// Check that actual and expected have the same vars
template<typename K>
struct TestRecordValEqual
{
    const dballe::Record& actual;
    const dballe::Record& expected;
    K name;
    bool with_missing_int;

    TestRecordValEqual(const dballe::Record& actual, const dballe::Record& expected, const K& name, bool with_missing_int=false)
        : actual(actual), expected(expected), name(name), with_missing_int(with_missing_int) {}

    void check(WIBBLE_TEST_LOCPRM) const;
};

/// Check that actual and expected have the same vars
struct TestRecordVarsEqual
{
    const dballe::Record& actual;
    const dballe::Record& expected;

    TestRecordVarsEqual(const dballe::Record& actual, const dballe::Record& expected) : actual(actual), expected(expected) {}

    void check(WIBBLE_TEST_LOCPRM) const;
};

struct ActualRecord : public wibble::tests::Actual<const dballe::Record&>
{
    ActualRecord(const dballe::Record& actual) : wibble::tests::Actual<const dballe::Record&>(actual) {}

    template<typename K>
    TestRecordValEqual<K> equals(const Record& expected, const K& name) { return TestRecordValEqual<K>(this->actual, expected, name); }
    template<typename K>
    TestRecordValEqual<K> equals_with_missing_int(const Record& expected, const K& name)
    {
        return TestRecordValEqual<K>(this->actual, expected, name, true);
    }
    TestRecordVarsEqual vars_equal(const Record& expected) { return TestRecordVarsEqual(this->actual, expected); }
};

// Set a record from a ", "-separated string of assignments
void set_record_from_string(Record& rec, const std::string& s);
Record record_from_string(const std::string& s);

}
}

namespace wibble {
namespace tests {

inline dballe::tests::ActualRecord actual(const dballe::Record& actual) { return dballe::tests::ActualRecord(actual); }

}
}
