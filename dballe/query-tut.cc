#include "core/test-utils-core.h"
#include "query.h"

using namespace std;
using namespace wibble::tests;
using namespace wreport;
using namespace dballe;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("clone", [](Fixture& f) {
        auto q = Query::create();
        q->set_datetimerange(DatetimeRange(
                2015, 5, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT,
                2015, 5, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT));

        auto q1 = q->clone();
        wassert(actual(q1->get_datetimerange().min) == Datetime(2015, 5, 1, 0, 0, 0));
        wassert(actual(q1->get_datetimerange().max) == Datetime(2015, 5, 31, 23, 59, 59));
    }),
    Test("foreach_key", [](Fixture& f) {
        auto foreach_key = [](const std::string& test_string) -> std::string {
            std::string res;
            core::Query q;
            q.set_from_test_string(test_string);
            q.foreach_key([&](const char* key, unique_ptr<Var>&& var) {
                if (!res.empty()) res += ", ";
                res += key;
                res += "=";
                res += var->format("");
            });
            return res;
        };

        wassert(actual(foreach_key("")) == "");
        wassert(actual(foreach_key("latmin=45.0")) == "latmin=45.00000");
        wassert(actual(foreach_key("latmin=45.0, latmax=46.0")) == "latmin=45.00000, latmax=46.00000");
        wassert(actual(foreach_key("latmin=45.0, latmax=46.0, lon=11.0")) == "latmin=45.00000, latmax=46.00000, lon=11.00000");
        wassert(actual(foreach_key("latmin=45.0, latmax=46.0, lonmin=11.0, lonmax=11.5")) == "latmin=45.00000, latmax=46.00000, lonmin=11.00000, lonmax=11.50000");
    }),
};

test_group newtg("dballe_query", tests);

}

