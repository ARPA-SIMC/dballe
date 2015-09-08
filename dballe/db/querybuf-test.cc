#include "db/tests.h"
#include "db/querybuf.h"

using namespace wreport;
using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("append", []() {
            Querybuf buf(10);

            // A new querybuf contains the empty string
            wassert(actual(buf) == string());

            // Clearing should still be empty
            buf.clear();
            wassert(actual(buf) == string());

            buf.append("ciao");
            wassert(actual(buf) == "ciao");
            wassert(actual(buf.size()) == 4);

            buf.appendf("%d %s", 42, "--");
            wassert(actual(buf) == "ciao42 --");
            wassert(actual(buf.size()) == 9);

            buf.clear();
            wassert(actual(buf) == string());

            buf.append("123456789");
            wassert(actual(buf) == "123456789");

            buf.clear();
            buf.start_list(", ");
            buf.append_list("1");
            buf.append_list("2");
            buf.append_listf("%d", 3);
            wassert(actual(buf) == "1, 2, 3");
        });
        add_method("varlist", []() {
            Querybuf buf(50);
            buf.append_varlist("B12101,B12103,block");
            wassert(actual((string)buf) == "3173,3175,257");
        });
    }
} test("db_querybuf");

}
