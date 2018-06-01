#include <wreport/utils/tests.h>
#include <wreport/utils/testrunner.h>
#include <wreport/utils/term.h>
#include <dballe/db/tests.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <exception>

using namespace wreport::tests;

void signal_to_exception(int)
{
    throw std::runtime_error("killing signal catched");
}

int main(int argc,const char* argv[])
{
    signal(SIGSEGV, signal_to_exception);
    signal(SIGILL, signal_to_exception);

    auto& tests = TestRegistry::get();

    wreport::term::Terminal output(stderr);

    std::unique_ptr<FilteringTestController> controller;

    bool verbose = (bool)getenv("TEST_VERBOSE");

    if (verbose)
        controller.reset(new VerboseTestController(output));
    else
        controller.reset(new SimpleTestController(output));

    if (const char* whitelist = getenv("TEST_WHITELIST"))
        controller->whitelist = whitelist;

    if (const char* blacklist = getenv("TEST_BLACKLIST"))
        controller->blacklist = blacklist;

    auto all_results = tests.run_tests(*controller);
    TestResultStats rstats(all_results);
    rstats.print_results(output);
    if (verbose) rstats.print_stats(output);
    rstats.print_summary(output);
    return rstats.success ? 0 : 1;
}
