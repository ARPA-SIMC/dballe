#include "core/tests.h"
#include "record.h"
#include <algorithm>

using namespace std;
using namespace wreport::tests;
using namespace dballe;
using namespace wreport;

namespace {

ostream& operator<<(ostream& out, Vartype t)
{
    return out << vartype_format(t);
}

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("record", []() {
        });
#if 0
        add_method("foreach_key", []() {
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
        });
#endif
        add_method("metadata", []() {
            wreport::Varinfo info = Record::key_info("rep_memo");
            wassert(actual(info->type) == Vartype::String);

            info = Record::key_info("ana_id");
            wassert(actual(info->type) == Vartype::Integer);
        });
    }
} test("dballe_record");

}
