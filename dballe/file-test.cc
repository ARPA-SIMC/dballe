#include "core/tests.h"
#include "types.h"

using namespace std;
using namespace wreport::tests;
using namespace dballe;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("binarymessage", []() {
            BinaryMessage bm(File::BUFR);
            wassert(actual(bm.encoding) == File::BUFR);
            wassert(actual(bm.pathname.empty()).istrue());
            wassert(actual(bm.offset) == -1);
            wassert(actual(bm.index) == MISSING_INT);
            wassert(actual(bm.data.size()) == 0);
            wassert(actual(bm.data.empty()).istrue());
        });
        add_method("bufr", []() {
            // BUFR Read test
            auto file = File::create(File::BUFR, tests::datafile("bufr/bufr1"), "r");
            BinaryMessage msg = wcallchecked(file->read());
            wassert(actual(msg).istrue());
            wassert(actual(msg.data.size()) == 182u);
            wassert(actual(msg.index) == 0);
            wassert(actual(msg.offset) == 0);
        });
        add_method("crex", []() {
            // CREX Read test
            auto file = File::create(File::CREX, tests::datafile("crex/test-synop0.crex"), "r");
            BinaryMessage msg = wcallchecked(file->read());
            wassert(actual(msg).istrue());
            wassert(actual(msg.data.size()) == 251u);
            wassert(actual(msg.index) == 0);
            wassert(actual(msg.offset) == 0);
        });
        add_method("aof", []() {
            // AOF Read test
            auto file = File::create(File::AOF, tests::datafile("aof/obs1-11.0.aof"), "r");
            BinaryMessage msg = wcallchecked(file->read());
            wassert(actual(msg).istrue());
            wassert(actual(msg.data.size()) == 140u);
            wassert(actual(msg.index) == 0);
            wassert(actual(msg.offset) == 140);
        });
        add_method("parse_encoding", []() {
            // Parse encoding test
            wassert(actual(File::parse_encoding("BUFR")) == File::BUFR);
            wassert(actual(File::parse_encoding("bufr")) == File::BUFR);
            wassert(actual(File::parse_encoding("Bufr")) == File::BUFR);
            wassert(actual(File::parse_encoding("CREX")) == File::CREX);
            wassert(actual(File::parse_encoding("crex")) == File::CREX);
            wassert(actual(File::parse_encoding("CreX")) == File::CREX);
            wassert(actual(File::parse_encoding("AOF")) == File::AOF);
            wassert(actual(File::parse_encoding("aof")) == File::AOF);
            wassert(actual(File::parse_encoding("AoF")) == File::AOF);
        });
    }
} test("dballe_file");

}
