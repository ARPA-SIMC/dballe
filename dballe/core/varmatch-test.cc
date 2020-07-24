#include "tests.h"
#include "var.h"
#include "varmatch.h"

using namespace wreport;
using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_varmatch");

void Tests::register_tests() {

add_method("int", []() {
    Var var(varinfo(WR_VAR(0, 1, 1)), 42);

    wassert(actual((*Varmatch::parse("B01001<43"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001<42"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01001<41"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01001<=43"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001<=42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001<=41"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01001>41"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001>42"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01001>43"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01001>=41"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001>=42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001>=43"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01001==42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001=42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001==43"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01001=43"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01001<>43"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001<>42"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01001!=43"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001!=42"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("41<=B01001<=42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("42<=B01001<=42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("42<=B01001<=43"))(var)).istrue());
    wassert(actual((*Varmatch::parse("40<=B01001<=41"))(var)).isfalse());
});

add_method("decimal", []() {
    Var var(varinfo(WR_VAR(0, 12, 101)), 273.15);

    wassert(actual((*Varmatch::parse("B12101<274"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101<273.15"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B12101<273"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B12101<=274"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101<=273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101<=273"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B12101>273"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101>273.15"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B12101>274"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B12101>=273"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101>=273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101>=274"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B12101==273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101=273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101==274"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B12101=274"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B12101<>274"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101<>273.15"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B12101!=274"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101!=273.15"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("273<=B12101<=273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("273.15<=B12101<=273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("273.15<=B12101<=274"))(var)).istrue());
    wassert(actual((*Varmatch::parse("272<=B12101<=273"))(var)).isfalse());
});

add_method("string", []() {
    Var var(varinfo(WR_VAR(0, 1, 11)), "enrico");

    wassert(actual((*Varmatch::parse("B01011<paolo"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011<enrico"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01011<emanuele"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01011<=paolo"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011<=enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011<=emanuele"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01011>emanuele"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011>enrico"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01011>paolo"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01011>=emanuele"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011>=enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011>=paolo"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01011==enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011=enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011==paolo"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01011=paolo"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01011<>paolo"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011<>enrico"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01011!=paolo"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011!=enrico"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("emanuele<=B01011<=enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("enrico<=B01011<=enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("enrico<=B01011<=paolo"))(var)).istrue());
    wassert(actual((*Varmatch::parse("daniele<=B01011<=emanuele"))(var)).isfalse());
});

add_method("errors", []() {
    wassert_throws(wreport::error_consistency, Varmatch::parse("B01011"));
    wassert_throws(wreport::error_consistency, Varmatch::parse("enrico"));
    wassert_throws(wreport::error_consistency, Varmatch::parse("B01011 3"));
    wassert_throws(wreport::error_consistency, Varmatch::parse("B01011=!3"));
    wassert_throws(wreport::error_consistency, Varmatch::parse("B01011><3"));
    wassert_throws(wreport::error_consistency, Varmatch::parse("3<B01011>3"));
    wassert_throws(wreport::error_consistency, Varmatch::parse("3>B01011>3"));
    wassert_throws(wreport::error_consistency, Varmatch::parse("3>B01011<3"));
    wassert_throws(wreport::error_consistency, Varmatch::parse("3<B01011>=3"));
    wassert_throws(wreport::error_consistency, Varmatch::parse("3<=B01011>3"));
    wassert_throws(wreport::error_consistency, Varmatch::parse("3=B01011=3"));
});

}

}
