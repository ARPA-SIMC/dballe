#include "domain_errors.h"
#include "tests.h"
#include <cstring>
#include <wreport/options.h>

using namespace std;
using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("msg_domain_errors");

void Tests::register_tests()
{

    add_method("throw", []() {
        wassert_false(wreport::options::var_silent_domain_errors);
        wassert_false(wreport::options::var_clamp_domain_errors);
        wassert_false(wreport::options::var_hook_domain_errors);

        {
            impl::msg::WreportVarOptionsForImport o(
                ImporterOptions::DomainErrors::THROW);
            wassert_false(wreport::options::var_silent_domain_errors);
            wassert_false(wreport::options::var_clamp_domain_errors);
            wassert_false(wreport::options::var_hook_domain_errors);
        }

        wassert_false(wreport::options::var_silent_domain_errors);
        wassert_false(wreport::options::var_clamp_domain_errors);
        wassert_false(wreport::options::var_hook_domain_errors);
    });

    add_method("unset", []() {
        wassert_false(wreport::options::var_silent_domain_errors);
        wassert_false(wreport::options::var_clamp_domain_errors);
        wassert_false(wreport::options::var_hook_domain_errors);

        {
            impl::msg::WreportVarOptionsForImport o(
                ImporterOptions::DomainErrors::UNSET);
            wassert_true(wreport::options::var_silent_domain_errors);
            wassert_false(wreport::options::var_clamp_domain_errors);
            wassert_false(wreport::options::var_hook_domain_errors);
        }

        wassert_false(wreport::options::var_silent_domain_errors);
        wassert_false(wreport::options::var_clamp_domain_errors);
        wassert_false(wreport::options::var_hook_domain_errors);
    });

    add_method("clamp", []() {
        wassert_false(wreport::options::var_silent_domain_errors);
        wassert_false(wreport::options::var_clamp_domain_errors);
        wassert_false(wreport::options::var_hook_domain_errors);

        {
            impl::msg::WreportVarOptionsForImport o(
                ImporterOptions::DomainErrors::CLAMP);
            wassert_false(wreport::options::var_silent_domain_errors);
            wassert_true(wreport::options::var_clamp_domain_errors);
            wassert_false(wreport::options::var_hook_domain_errors);
        }

        wassert_false(wreport::options::var_silent_domain_errors);
        wassert_false(wreport::options::var_clamp_domain_errors);
        wassert_false(wreport::options::var_hook_domain_errors);
    });

    add_method("tag", []() {
        wassert_false(wreport::options::var_silent_domain_errors);
        wassert_false(wreport::options::var_clamp_domain_errors);
        wassert_false(wreport::options::var_hook_domain_errors);

        {
            impl::msg::WreportVarOptionsForImport o(
                ImporterOptions::DomainErrors::TAG);
            wassert_false(wreport::options::var_silent_domain_errors);
            wassert_false(wreport::options::var_clamp_domain_errors);
            wassert_true(wreport::options::var_hook_domain_errors);
        }

        wassert_false(wreport::options::var_silent_domain_errors);
        wassert_false(wreport::options::var_clamp_domain_errors);
        wassert_false(wreport::options::var_hook_domain_errors);
    });
}

} // namespace
