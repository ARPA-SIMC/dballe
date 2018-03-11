#include "core/tests.h"
#include "string.h"

using namespace dballe;
using namespace wreport::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_string");

void Tests::register_tests() {

add_method("url_pop_query_string", []() {
    string url = "http://example.org";
    string res;
    res = url_pop_query_string(url, "foo");
    wassert(actual(res) == "");
    wassert(actual(url) == "http://example.org");

    url = "http://example.org?foo=bar";
    res = url_pop_query_string(url, "foo");
    wassert(actual(res) == "bar");
    wassert(actual(url) == "http://example.org");

    url = "http://example.org?foo=bar&baz=gnu";
    res = url_pop_query_string(url, "foo");
    wassert(actual(res) == "bar");
    wassert(actual(url) == "http://example.org?baz=gnu");

    url = "http://example.org?baz=gnu&foo=bar";
    res = url_pop_query_string(url, "foo");
    wassert(actual(res) == "bar");
    wassert(actual(url) == "http://example.org?baz=gnu");

    url = "http://example.org?baz=gnu&foo=bar&wibble=wobble";
    res = url_pop_query_string(url, "foo");
    wassert(actual(res) == "bar");
    wassert(actual(url) == "http://example.org?baz=gnu&wibble=wobble");

    url = "http://example.org?foo=bar&foo=baz";
    res = url_pop_query_string(url, "foo");
    wassert(actual(res) == "bar");
    wassert(actual(url) == "http://example.org?foo=baz");
});

}

}
