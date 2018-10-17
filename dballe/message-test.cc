#include "msg/tests.h"
#include "message.h"

using namespace std;
using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("dballe_message");

void Tests::register_tests()
{

add_method("get", []() {
    auto msgs = read_msgs("bufr/gts-acars-uk1.bufr", Encoding::BUFR);

    wassert(actual(msgs[0]->get_type()) == MessageType::AMDAR);
    wassert(actual(msgs[0]->get_datetime()) == Datetime(2009, 2, 24, 11, 31));
});

add_method("get_shortcut", []() {
    auto msgs = read_msgs("bufr/gts-acars-uk1.bufr", Encoding::BUFR);

    const wreport::Var* var = msgs[0]->get_shortcut("ident");
    wassert(actual(var).istrue());
    wassert(actual(*var) == dballe::var("B01011", "EU3375"));
});

add_method("foreach_var", []() {
    auto msgs = read_msgs("bufr/gts-acars-uk1.bufr", Encoding::BUFR);

    struct Result
    {
        Level l;
        Trange t;
        wreport::Var v;

        Result(const Level& l, const Trange& t, const wreport::Var& v)
            : l(l), t(t), v(v) {}
    };

    std::vector<Result> results;
    msgs[0]->foreach_var([&](const Level& l, const Trange& t, const wreport::Var& v) {
        results.emplace_back(l, t, v);
        return true;
    });

    wassert(actual(results.size()) == 18u);
    wassert(actual(results[0].l) == Level(102, 6260000));
    wassert(actual(results[0].t) == Trange::instant());
    wassert(actual(results[0].v) == var("B01006", "LH968"));
    wassert(actual(results[1].l) == Level(102, 6260000));
    wassert(actual(results[1].t) == Trange::instant());
    wassert(actual(results[1].v) == var("B02061", 0));
    wassert(actual(results[2].l) == Level(102, 6260000));
    wassert(actual(results[2].t) == Trange::instant());
    wassert(actual(results[2].v) == var("B02062", 3));
    wassert(actual(results[3].l) == Level(102, 6260000));
    wassert(actual(results[3].t) == Trange::instant());
    wassert(actual(results[3].v) == var("B02064", 0));
    wassert(actual(results[4].l) == Level(102, 6260000));
    wassert(actual(results[4].t) == Trange::instant());
    wassert(actual(results[4].v) == var("B07030", 6260.0));
    wassert(actual(results[5].l) == Level(102, 6260000));
    wassert(actual(results[5].t) == Trange::instant());
    wassert(actual(results[5].v) == var("B08004", 3));
    wassert(actual(results[6].l) == Level(102, 6260000));
    wassert(actual(results[6].t) == Trange::instant());
    wassert(actual(results[6].v) == var("B11001", 33));
    wassert(actual(results[7].l) == Level(102, 6260000));
    wassert(actual(results[7].t) == Trange::instant());
    wassert(actual(results[7].v) == var("B11002", 33.4));
    wassert(actual(results[8].l) == Level(102, 6260000));
    wassert(actual(results[8].t) == Trange::instant());
    wassert(actual(results[8].v) == var("B12101", 240.0));
    wassert(actual(results[9].l) == Level(102, 6260000));
    wassert(actual(results[9].t) == Trange::instant());
    wassert(actual(results[9].v) == var("B13002", 0.0));
    wassert(actual(results[10].l) == Level());
    wassert(actual(results[10].t) == Trange());
    wassert(actual(results[10].v) == var("B01011", "EU3375"));
    wassert(actual(results[11].l) == Level());
    wassert(actual(results[11].t) == Trange());
    wassert(actual(results[11].v) == var("B04001", 2009));
    wassert(actual(results[12].l) == Level());
    wassert(actual(results[12].t) == Trange());
    wassert(actual(results[12].v) == var("B04002", 2));
    wassert(actual(results[13].l) == Level());
    wassert(actual(results[13].t) == Trange());
    wassert(actual(results[13].v) == var("B04003", 24));
    wassert(actual(results[14].l) == Level());
    wassert(actual(results[14].t) == Trange());
    wassert(actual(results[14].v) == var("B04004", 11));
    wassert(actual(results[15].l) == Level());
    wassert(actual(results[15].t) == Trange());
    wassert(actual(results[15].v) == var("B04005", 31));
    wassert(actual(results[16].l) == Level());
    wassert(actual(results[16].t) == Trange());
    wassert(actual(results[16].v) == var("B05001", 48.90500));
    wassert(actual(results[17].l) == Level());
    wassert(actual(results[17].t) == Trange());
    wassert(actual(results[17].v) == var("B06001", 10.63667));
});

}

}

