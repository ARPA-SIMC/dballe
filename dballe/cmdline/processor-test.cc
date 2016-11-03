#include "core/tests.h"
#include "processor.h"
#include <limits>

using namespace dballe;
using namespace dballe::cmdline;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("cmdline_processor");

void Tests::register_tests() {

add_method("parse_index", [] {
    Filter filter;
    filter.set_index_filter("99-101");
    wassert(actual(filter.match_index(98)).isfalse());
    wassert(actual(filter.match_index(99)).istrue());
    wassert(actual(filter.match_index(100)).istrue());
    wassert(actual(filter.match_index(101)).istrue());
    wassert(actual(filter.match_index(102)).isfalse());

    filter.set_index_filter("100-101");
    wassert(actual(filter.match_index(99)).isfalse());
    wassert(actual(filter.match_index(100)).istrue());
    wassert(actual(filter.match_index(101)).istrue());
    wassert(actual(filter.match_index(102)).isfalse());

    filter.set_index_filter("-10, 100-101, 103, 105-");
    wassert(actual(filter.imatcher.ranges.size()) == 4u);
    wassert(actual(filter.imatcher.ranges[0].first) == 0);
    wassert(actual(filter.imatcher.ranges[0].second) == 10);
    wassert(actual(filter.imatcher.ranges[1].first) == 100);
    wassert(actual(filter.imatcher.ranges[1].second) == 101);
    wassert(actual(filter.imatcher.ranges[2].first) == 103);
    wassert(actual(filter.imatcher.ranges[2].second) == 103);
    wassert(actual(filter.imatcher.ranges[3].first) == 105);
    wassert(actual(filter.imatcher.ranges[3].second) == std::numeric_limits<int>::max());
    wassert(actual(filter.match_index(0)).istrue());
    wassert(actual(filter.match_index(10)).istrue());
    wassert(actual(filter.match_index(11)).isfalse());
    wassert(actual(filter.match_index(99)).isfalse());
    wassert(actual(filter.match_index(100)).istrue());
    wassert(actual(filter.match_index(101)).istrue());
    wassert(actual(filter.match_index(102)).isfalse());
    wassert(actual(filter.match_index(103)).istrue());
    wassert(actual(filter.match_index(104)).isfalse());
    wassert(actual(filter.match_index(105)).istrue());
    wassert(actual(filter.match_index(100000)).istrue());

    filter.set_index_filter("");
    wassert(actual(filter.match_index(0)).istrue());
    wassert(actual(filter.match_index(10)).istrue());
});

}

}
