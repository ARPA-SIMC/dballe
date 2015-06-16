#include "core/tests.h"
#include "record.h"
#include <algorithm>

using namespace std;
using namespace wibble::tests;
using namespace dballe;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("record", [](Fixture& f) {
    }),
    Test("foreach_key", [](Fixture& f) {
        auto rec = Record::create();
        rec->set("lat", 44.5);
        rec->set("B12101", 290.4);
        vector<string> res;
        rec->foreach_key([&](const char* key, std::unique_ptr<wreport::Var>&& var) {
            res.push_back(string(key) + "=" + var->format());
        });
        sort(res.begin(), res.end());
        wassert(actual(res.size()) == 2);
        wassert(actual(res[0]) == "B12101=290.40");
        wassert(actual(res[1]) == "lat=44.50000");
    }),
    Test("metadata", [](Fixture& f) {
        wreport::Varinfo info = Record::key_info("rep_memo");
        wassert(actual(info->is_string()).istrue());

        info = Record::key_info("ana_id");
        wassert(actual(info->is_string()).isfalse());
    }),
};

test_group newtg("dballe_record", tests);

}
