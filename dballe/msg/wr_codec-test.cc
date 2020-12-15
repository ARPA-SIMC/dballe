#include "tests.h"
#include "wr_codec.h"
#include <wreport/options.h>
#include <cstring>

using namespace std;
using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("msg_wr_codec");

void Tests::register_tests() {

add_method("domain_throw", []() {
    auto file = File::create(Encoding::BUFR, tests::datafile("bufr/interpreted-range.bufr"), "r");
    auto options = ImporterOptions::create();
    options->domain_errors = ImporterOptions::DomainErrors::THROW;
    auto importer = Importer::create(Encoding::BUFR, *options);
    file->foreach([&](const BinaryMessage& bmsg) {
        wassert_throws(wreport::error_domain, importer->foreach_decoded(bmsg, [&](std::unique_ptr<Message> dest) { return true; }));
        return true;
    });
});

add_method("domain_unset", []() {
    auto file = File::create(Encoding::BUFR, tests::datafile("bufr/interpreted-range.bufr"), "r");
    auto options = ImporterOptions::create();
    options->domain_errors = ImporterOptions::DomainErrors::UNSET;
    auto importer = Importer::create(Encoding::BUFR, *options);
    unsigned count = 0;
    wassert_true(file->foreach([&](const BinaryMessage& bmsg) {
        return importer->foreach_decoded(bmsg, [&](std::unique_ptr<Message> dest) {
            // Would throw: wreport::error_domain: Value 329.2 is outside the range [0,327.66] for B22043 (SEA/WATER TEMPERATURE)
            const wreport::Var* val = dest->get(Level(1), Trange::instant(), WR_VAR(0, 22, 43));
            wassert_true(val);
            wassert_false(val->isset());
            ++count;
            return true;
        });
    }));
    wassert(actual(count) == 1);
});

#ifdef WREPORT_OPTIONS_HAS_VAR_CLAMP_DOMAIN_ERRORS
add_method("domain_clamp", []() {
    auto file = File::create(Encoding::BUFR, tests::datafile("bufr/interpreted-range.bufr"), "r");
    auto options = ImporterOptions::create();
    options->domain_errors = ImporterOptions::DomainErrors::CLAMP;
    auto importer = Importer::create(Encoding::BUFR, *options);
    unsigned count = 0;
    wassert_true(file->foreach([&](const BinaryMessage& bmsg) {
        return importer->foreach_decoded(bmsg, [&](std::unique_ptr<Message> dest) {
            // Would throw: wreport::error_domain: Value 329.2 is outside the range [0,327.66] for B22043 (SEA/WATER TEMPERATURE)
            const wreport::Var* val = dest->get(Level(1), Trange::instant(), WR_VAR(0, 22, 43));
            wassert_true(val);
            wassert(actual(*val) == 327.66);
            ++count;
            return true;
        });
    }));

    wassert(actual(count) == 1);
});
#endif

}

}
