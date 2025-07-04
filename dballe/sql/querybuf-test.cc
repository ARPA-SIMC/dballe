#include "dballe/core/tests.h"
#include "dballe/sql/querybuf.h"

namespace dballe {
namespace tests {
// Workaround for rocky8: newer gcc compilers can use plain actual() without
// issues
inline ActualStdString actual_qb(const dballe::sql::Querybuf& actual)
{
    return ActualStdString(actual);
}
} // namespace tests
} // namespace dballe

using namespace wreport;
using namespace dballe;
using namespace dballe::tests;
using namespace std;
using dballe::sql::Querybuf;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("append", []() {
            Querybuf buf(10);

            // A new querybuf contains the empty string
            wassert(actual_qb(buf) == string());

            // Clearing should still be empty
            buf.clear();
            wassert(actual_qb(buf) == string());

            buf.append("ciao");
            wassert(actual_qb(buf) == "ciao");
            wassert(actual(buf.size()) == 4);

            buf.appendf("%d %s", 42, "--");
            wassert(actual_qb(buf) == "ciao42 --");
            wassert(actual(buf.size()) == 9);

            buf.clear();
            wassert(actual_qb(buf) == string());

            buf.append("123456789");
            wassert(actual_qb(buf) == "123456789");

            buf.clear();
            buf.start_list(", ");
            buf.append_list("1");
            buf.append_list("2");
            buf.append_listf("%d", 3);
            wassert(actual_qb(buf) == "1, 2, 3");
        });
        add_method("varlist", []() {
            Querybuf buf(50);
            buf.append_varlist("B12101,B12103,block");
            wassert(actual_qb(buf) == "3173,3175,257");
        });
    }
} test("db_querybuf");

} // namespace
