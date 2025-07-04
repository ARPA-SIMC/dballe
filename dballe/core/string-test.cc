#include "dballe/core/tests.h"
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

void Tests::register_tests()
{

    add_method("url_pop_query_string", []() {
        string url = "http://example.org";
        string res;
        wassert_false(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "");
        wassert(actual(url) == "http://example.org");

        url = "http://example.org?foo=bar";
        wassert_true(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "bar");
        wassert(actual(url) == "http://example.org");

        url = "http://example.org?foo=bar&baz=gnu";
        wassert_true(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "bar");
        wassert(actual(url) == "http://example.org?baz=gnu");

        url = "http://example.org?baz=gnu&foo=bar";
        wassert_true(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "bar");
        wassert(actual(url) == "http://example.org?baz=gnu");

        url = "http://example.org?baz=gnu&foo=bar&wibble=wobble";
        wassert_true(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "bar");
        wassert(actual(url) == "http://example.org?baz=gnu&wibble=wobble");

        url = "http://example.org?foo=bar&foo=baz";
        wassert_true(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "bar");
        wassert(actual(url) == "http://example.org?foo=baz");

        url = "http://example.org?foobar=bar&foo=baz";
        wassert_true(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "baz");
        wassert(actual(url) == "http://example.org?foobar=bar");

        url = "http://example.org?foo&bar=baz";
        wassert_true(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "");
        wassert(actual(url) == "http://example.org?bar=baz");

        url = "http://example.org?bar=baz&foo";
        wassert_true(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "");
        wassert(actual(url) == "http://example.org?bar=baz");

        url = "http://example.org?foo&";
        wassert_true(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "");
        wassert(actual(url) == "http://example.org");

        url = "http://example.org?foo=&";
        wassert_true(url_pop_query_string(url, "foo", res));
        wassert(actual(res) == "");
        wassert(actual(url) == "http://example.org");
    });
}

} // namespace
