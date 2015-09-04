#include "core/tests.h"
#include "types.h"

using namespace std;
using namespace wibble::tests;
using namespace dballe;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("binarymessage", [](Fixture& f) {
        BinaryMessage bm(File::BUFR);
        wassert(actual(bm.encoding) == File::BUFR);
        wassert(actual(bm.pathname.empty()).istrue());
        wassert(actual(bm.offset) == -1);
        wassert(actual(bm.index) == MISSING_INT);
        wassert(actual(bm.data.size()) == 0);
        wassert(actual(bm.data.empty()).istrue());
    }),
    Test("bufr", [](Fixture& f) {
        // BUFR Read test
        auto file = File::create(File::BUFR, tests::datafile("bufr/bufr1"), "r");
        BinaryMessage msg(file->encoding());
        wrunchecked(msg = file->read());
        wassert(actual(msg).istrue());
        wassert(actual(msg.data.size()) == 182u);
        wassert(actual(msg.index) == 0);
        wassert(actual(msg.offset) == 0);
    }),
    Test("crex", [](Fixture& f) {
        // CREX Read test
        auto file = File::create(File::CREX, tests::datafile("crex/test-synop0.crex"), "r");
        BinaryMessage msg(file->encoding());
        wrunchecked(msg = file->read());
        wassert(actual(msg).istrue());
        wassert(actual(msg.data.size()) == 251u);
        wassert(actual(msg.index) == 0);
        wassert(actual(msg.offset) == 0);
    }),
    Test("aof", [](Fixture& f) {
        // AOF Read test
        auto file = File::create(File::AOF, tests::datafile("aof/obs1-11.0.aof"), "r");
        BinaryMessage msg(file->encoding());
        wrunchecked(msg = file->read());
        wassert(actual(msg).istrue());
        wassert(actual(msg.data.size()) == 140u);
        wassert(actual(msg.index) == 0);
        wassert(actual(msg.offset) == 140);
    }),
    Test("parse_encoding", [](Fixture&f) {
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
    }),
};

test_group newtg("dballe_file", tests);

}
