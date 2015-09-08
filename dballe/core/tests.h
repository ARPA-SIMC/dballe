#include <wreport/tests.h>
#include <dballe/file.h>
#include <dballe/record.h>
#include <dballe/core/query.h>
#include <dballe/core/values.h>
#include <dballe/core/defs.h>
#include <cstdlib>
#include <climits>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

namespace dballe {
namespace tests {

using namespace wreport::tests;

/**
 * Check if a test can be run.
 *
 * This is used to implement extra test filtering features like glob matching
 * on group or test names.
 */
bool test_can_run(const std::string& group_name, const std::string& test_name);

#if 0
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
#endif

// Message reading functions

/// Return the pathname of a test file
std::string datafile(const std::string& fname);

std::unique_ptr<File> open_test_data(const char* filename, File::Encoding type);

BinaryMessage read_rawmsg(const char* filename, File::Encoding type);

#if 0
/// Check that actual and expected have the same vars
struct TestRecordValEqual
{
    const dballe::Record& actual;
    const dballe::Record& expected;
    const char* name;
    bool with_missing_int;

    TestRecordValEqual(const dballe::Record& actual, const dballe::Record& expected, const char* name, bool with_missing_int=false)
        : actual(actual), expected(expected), name(name), with_missing_int(with_missing_int) {}

    void check() const;
};

struct TestRecordVarsEqual
{
    const dballe::Record& actual;
    dballe::Values expected;

    TestRecordVarsEqual(const dballe::Record& actual, const dballe::Record& expected) : actual(actual), expected(expected) {}
    TestRecordVarsEqual(const dballe::Record& actual, const dballe::Values& expected) : actual(actual), expected(expected) {}

    void check() const;
};
#endif

struct ActualRecord : public wreport::tests::Actual<const dballe::Record&>
{
    ActualRecord(const dballe::Record& actual) : wreport::tests::Actual<const dballe::Record&>(actual) {}

#if 0
    TestRecordValEqual equals(const Record& expected, const char* name) { return TestRecordValEqual(this->actual, expected, name); }
    TestRecordValEqual equals_with_missing_int(const Record& expected, const char* name)
    {
        return TestRecordValEqual(this->actual, expected, name, true);
    }
#endif
    /// Check that actual and expected have the same vars
    void vars_equal(const Record& expected) const { vars_equal(Values(expected)); }
    /// Check that actual and expected have the same vars
    void vars_equal(const Values& expected) const;
};

// Set a record from a ", "-separated string of assignments
void set_record_from_string(Record& rec, const std::string& s);
std::unique_ptr<Record> record_from_string(const std::string& s);
std::unique_ptr<Query> query_from_string(const std::string& s);
core::Query core_query_from_string(const std::string& s);

struct ActualMatcherResult : public Actual<int>
{
    using Actual::Actual;

    void operator==(int expected) const;
    void operator!=(int expected) const;
};

inline ActualMatcherResult actual_matcher_result(int actual) { return ActualMatcherResult(actual); }

using wreport::tests::actual;

inline dballe::tests::ActualRecord actual(const dballe::Record& actual) { return dballe::tests::ActualRecord(actual); }

inline ActualCString actual(const dballe::Ident& ident) { return ActualCString(ident); }

}
}
