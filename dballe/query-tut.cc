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
        Datetime dt;
        dt.year = 2015; dt.month = 5;
        q->set_datetime_exact(dt);

        auto q1 = q->clone();
        Datetime dtmin;
        Datetime dtmax;
        q1->get_datetime_bounds(dtmin, dtmax);
        wassert(actual(dtmin) == Datetime(2015, 5, 1, 0, 0, 0));
        wassert(actual(dtmax) == Datetime(2015, 5, 31, 23, 59, 59));
    }),
    Test("date", [](Fixture& f) {
        wassert(actual(Date(2013, 1, 1)) < Date(2014, 1, 1));
        wassert(actual(Date(2013, 1, 1)) < Date(2013, 2, 1));
        wassert(actual(Date(2013, 1, 1)) < Date(2013, 1, 2));
        wassert(actual(Date(1945, 4, 25)) != Date(1945, 4, 26));
    }),
    Test("to_vars", [](Fixture& f) {
        auto to_vars = [](const std::string& test_string) -> std::string {
            std::string res;
            core::Query q;
            q.set_from_test_string(test_string);
            q.to_vars([&](const char* key, unique_ptr<Var>&& var) {
                if (!res.empty()) res += ", ";
                res += key;
                res += "=";
                res += var->format("");
            });
            return res;
        };

        wassert(actual(to_vars("")) == "");
        wassert(actual(to_vars("latmin=45.0")) == "latmin=45.00000");
        wassert(actual(to_vars("latmin=45.0, latmax=46.0")) == "latmin=45.00000, latmax=46.00000");
        wassert(actual(to_vars("latmin=45.0, latmax=46.0, lon=11.0")) == "latmin=45.00000, latmax=46.00000, lon=11.00000");
        wassert(actual(to_vars("latmin=45.0, latmax=46.0, lonmin=11.0, lonmax=11.5")) == "latmin=45.00000, latmax=46.00000, lonmin=11.00000, lonmax=11.50000");
    }),
};

test_group newtg("dballe_query", tests);

}

