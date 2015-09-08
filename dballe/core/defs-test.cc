#include "core/tests.h"
#include "core/defs.h"

using namespace dballe;
using namespace wreport::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("ident", []() {
            wassert(actual(Ident()) == Ident());
            wassert(actual(Ident("foo")) == Ident("foo"));
            wassert(actual(Ident().is_missing()).istrue());
            wassert(actual(Ident("foo").is_missing()).isfalse());
            Ident test;
            wassert(actual((const char*)test == nullptr).istrue());
            test = "antani";
            wassert(actual((const char*)test) == "antani");
            Ident test1 = test;
            wassert(actual(test) == Ident("antani"));
            wassert(actual(test1) == Ident("antani"));
            test1 = test1;
            wassert(actual(test) == Ident("antani"));
            wassert(actual(test1) == Ident("antani"));
            test1 = Ident("blinda");
            wassert(actual(test) == Ident("antani"));
            wassert(actual(test1) == Ident("blinda"));
            test = move(test1);
            wassert(actual(test) == Ident("blinda"));
            wassert(actual(test1.is_missing()).istrue());
            Ident test2(move(test));
            wassert(actual(test.is_missing()).istrue());
            wassert(actual(test2) == Ident("blinda"));
            test = string("foo");
            wassert(actual(test) == Ident("foo"));
            wassert(actual(test2) == Ident("blinda"));
            wassert(actual(Ident("foo") == Ident("foo")).istrue());
            wassert(actual(Ident("foo") <= Ident("foo")).istrue());
            wassert(actual(Ident("foo") >= Ident("foo")).istrue());
            wassert(actual(Ident("foo") == Ident("bar")).isfalse());
            wassert(actual(Ident("foo") != Ident("foo")).isfalse());
            wassert(actual(Ident("foo") != Ident("bar")).istrue());
            wassert(actual(Ident("antani") <  Ident("blinda")).istrue());
            wassert(actual(Ident("antani") <= Ident("blinda")).istrue());
            wassert(actual(Ident("antani") >  Ident("blinda")).isfalse());
            wassert(actual(Ident("antani") >= Ident("blinda")).isfalse());
            wassert(actual(Ident("blinda") <  Ident("antani")).isfalse());
            wassert(actual(Ident("blinda") <= Ident("antani")).isfalse());
            wassert(actual(Ident("blinda") >  Ident("antani")).istrue());
            wassert(actual(Ident("blinda") >= Ident("antani")).istrue());
        });
    }
} test("core_defs");

}
