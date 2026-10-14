#include "structbuf.h"
#include "tests.h"

using namespace dballe;
using namespace wreport;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        // Test an in-memory structbuf
        add_method("memory", []() {
            Structbuf<int, 3> buf;
            wassert(actual(buf.size()) == 0);
            wassert(actual(buf.is_file_backed()).isfalse());

            buf.append(1);
            wassert(actual(buf.size()) == 1);
            wassert(actual(buf.is_file_backed()).isfalse());

            buf.append(2);
            wassert(actual(buf.size()) == 2);
            wassert(actual(buf.is_file_backed()).isfalse());

            buf.append(3);
            wassert(actual(buf.size()) == 3);
            wassert(actual(buf.is_file_backed()).isfalse());

            buf.ready_to_read();
            for (unsigned i = 0; i < 3; ++i)
                wassert(actual(buf[i]) == i + 1);
        });

        // Test an file-backed structbuf
        add_method("file", []() {
            Structbuf<int, 3> buf;
            buf.append(1);
            buf.append(2);
            buf.append(3);
            wassert(actual(buf.size()) == 3);
            wassert(actual(buf.is_file_backed()).isfalse());

            buf.append(4);
            wassert(actual(buf.size()) == 4);
            wassert(actual(buf.is_file_backed()).istrue());

            buf.append(5);
            wassert(actual(buf.size()) == 5);
            buf.append(6);
            wassert(actual(buf.size()) == 6);
            buf.append(7);
            wassert(actual(buf.size()) == 7);
            buf.append(8);
            wassert(actual(buf.size()) == 8);

            buf.ready_to_read();
            for (unsigned i = 0; i < 8; ++i)
                wassert(actual(buf[i]) == i + 1);
        });
    }
} test("core_structbuf");

} // namespace
