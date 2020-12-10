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

add_method("domain_throw", []() {
    // TODO: bufr/interpreted-range.bufr for BUFR
    auto file = File::create(Encoding::JSON, tests::datafile("json/issue241.json"), "r");
    auto options = ImporterOptions::create();
    options->domain_errors = ImporterOptions::DomainErrors::THROW;
    auto importer = Importer::create(Encoding::JSON, *options);
    file->foreach([&](const BinaryMessage& bmsg) {
        wassert_throws(wreport::error_domain, importer->foreach_decoded(bmsg, [&](std::unique_ptr<Message> dest) { return true; }));
        return true;
    });
});

add_method("domain_unset", []() {
    auto file = File::create(Encoding::JSON, tests::datafile("json/issue241.json"), "r");
    auto options = ImporterOptions::create();
    options->domain_errors = ImporterOptions::DomainErrors::UNSET;
    auto importer = Importer::create(Encoding::JSON, *options);
    unsigned count = 0;
    wassert_true(file->foreach([&](const BinaryMessage& bmsg) {
        return importer->foreach_decoded(bmsg, [&](std::unique_ptr<Message> dest) {
            // Would throw: wreport::error_domain Value -30 is outside the range [-20,1048554] for 013013 (TOTAL SNOW DEPTH)
            const wreport::Var* val = dest->get(Level(1), Trange::instant(), WR_VAR(0, 13, 13));
            wassert_true(val);
            wassert_false(val->isset());
            ++count;
            return true;
        });
    }));
    wassert(actual(count) == 1);
});

add_method("domain_clamp", []() {
    auto file = File::create(Encoding::JSON, tests::datafile("json/issue241.json"), "r");
    auto options = ImporterOptions::create();
    options->domain_errors = ImporterOptions::DomainErrors::CLAMP;
    auto importer = Importer::create(Encoding::JSON, *options);
    unsigned count = 0;
    wassert_true(file->foreach([&](const BinaryMessage& bmsg) {
        return importer->foreach_decoded(bmsg, [&](std::unique_ptr<Message> dest) {
            // Would throw: wreport::error_domain Value -30 is outside the range [-20,1048554] for 013013 (TOTAL SNOW DEPTH)
            const wreport::Var* val = dest->get(Level(1), Trange::instant(), WR_VAR(0, 13, 13));
            wassert_true(val);
            wassert(actual(*val) == -20);
            ++count;
            return true;
        });
    }));

    wassert(actual(count) == 1);
});

}

}
