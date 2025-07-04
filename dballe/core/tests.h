#include <climits>
#include <cstdlib>
#include <dballe/core/csv.h>
#include <dballe/core/defs.h>
#include <dballe/core/query.h>
#include <dballe/file.h>
#include <dballe/values.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <wreport/tests.h>

namespace dballe {
namespace tests {

using namespace wreport::tests;

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
std::filesystem::path datafile(const std::string& fname);

std::unique_ptr<File> open_test_data(const char* filename, Encoding type);

BinaryMessage read_rawmsg(const char* filename, Encoding type);

class MemoryCSVWriter : public CSVWriter
{
public:
    std::stringstream buf;

    void flush_row() override
    {
        buf << row << std::endl;
        row.clear();
    }
};

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

// Set a query from a ", "-separated string of assignments
std::unique_ptr<Query> query_from_string(const std::string& s);
core::Query core_query_from_string(const std::string& s);

struct ActualMatcherResult : public Actual<int>
{
    using Actual::Actual;

    void operator==(int expected) const;
    void operator!=(int expected) const;
};

inline ActualMatcherResult actual_matcher_result(int actual)
{
    return ActualMatcherResult(actual);
}

using wreport::tests::actual;

inline ActualCString actual(const dballe::Ident& ident)
{
    return ActualCString(ident);
}

} // namespace tests
} // namespace dballe
