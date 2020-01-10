#include "tests.h"
#include "json_codec.h"
#include <cstring>

using namespace std;
using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("msg_json_codec");

void Tests::register_tests() {

add_method("read", []() {
    auto file = File::create(Encoding::JSON, tests::datafile("json/issue134.json"), "r");
    auto importer = Importer::create(Encoding::JSON);
    unsigned count = 0;
    wassert_true(file->foreach([&](const BinaryMessage& bmsg) {
        return importer->foreach_decoded(bmsg, [&](std::unique_ptr<Message> dest) {
            ++count;
            return true;
        });
    }));

    wassert(actual(count) == 5);
});

}

}
