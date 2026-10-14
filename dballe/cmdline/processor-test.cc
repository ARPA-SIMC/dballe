#include "dballe/core/tests.h"
#include "processor.h"
#include <limits>

using namespace dballe;
using namespace dballe::cmdline;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("cmdline_processor");

void Tests::register_tests()
{

    add_method("parse_index", [] {
        Filter filter;
        filter.set_index_filter("99-101");
        wassert(actual(filter.match_index(98)).isfalse());
        wassert(actual(filter.match_index(99)).istrue());
        wassert(actual(filter.match_index(100)).istrue());
        wassert(actual(filter.match_index(101)).istrue());
        wassert(actual(filter.match_index(102)).isfalse());

        filter.set_index_filter("100-101");
        wassert(actual(filter.match_index(99)).isfalse());
        wassert(actual(filter.match_index(100)).istrue());
        wassert(actual(filter.match_index(101)).istrue());
        wassert(actual(filter.match_index(102)).isfalse());

        filter.set_index_filter("-10, 100-101, 103, 105-");
        wassert(actual(filter.imatcher.ranges.size()) == 4u);
        wassert(actual(filter.imatcher.ranges[0].first) == 0);
        wassert(actual(filter.imatcher.ranges[0].second) == 10);
        wassert(actual(filter.imatcher.ranges[1].first) == 100);
        wassert(actual(filter.imatcher.ranges[1].second) == 101);
        wassert(actual(filter.imatcher.ranges[2].first) == 103);
        wassert(actual(filter.imatcher.ranges[2].second) == 103);
        wassert(actual(filter.imatcher.ranges[3].first) == 105);
        wassert(actual(filter.imatcher.ranges[3].second) ==
                std::numeric_limits<int>::max());
        wassert(actual(filter.match_index(0)).istrue());
        wassert(actual(filter.match_index(10)).istrue());
        wassert(actual(filter.match_index(11)).isfalse());
        wassert(actual(filter.match_index(99)).isfalse());
        wassert(actual(filter.match_index(100)).istrue());
        wassert(actual(filter.match_index(101)).istrue());
        wassert(actual(filter.match_index(102)).isfalse());
        wassert(actual(filter.match_index(103)).istrue());
        wassert(actual(filter.match_index(104)).isfalse());
        wassert(actual(filter.match_index(105)).istrue());
        wassert(actual(filter.match_index(100000)).istrue());

        filter.set_index_filter("");
        wassert(actual(filter.match_index(0)).istrue());
        wassert(actual(filter.match_index(10)).istrue());
    });

    add_method("parse_json", [] {
        struct TestAction : public Action
        {
            std::vector<std::shared_ptr<dballe::Message>> messages;

            bool operator()(const Item& item) override
            {
                for (auto& m : *(item.msgs))
                {
                    messages.push_back(m->clone());
                }
                return true;
            }
        };

        ReaderOptions opts;
        opts.input_type = "json";
        Reader reader(opts);
        TestAction action;

        reader.read({dballe::tests::datafile("json/issue134.json")}, action);
        wassert(actual(action.messages.size()) == 5);

        {
            const wreport::Var* var = action.messages.at(0)->get(
                dballe::Level(103, 2000), dballe::Trange(254, 0, 0),
                WR_VAR(0, 12, 101));
            wassert(actual(var) != (const wreport::Var*)0);
            const wreport::Var* attr = var->enqa(WR_VAR(0, 33, 7));
            wassert(actual(attr) != (const wreport::Var*)0);
            wassert(actual(attr->enqi()) == 0);
        }
        {
            const wreport::Var* var = action.messages.at(1)->get(
                dballe::Level(103, 2000), dballe::Trange(254, 0, 0),
                WR_VAR(0, 12, 101));
            wassert(actual(var) != (const wreport::Var*)0);
            const wreport::Var* attr = var->enqa(WR_VAR(0, 33, 7));
            wassert(actual(attr) == (const wreport::Var*)0);
        }
        {
            const wreport::Var* var = action.messages.at(2)->get(
                dballe::Level(103, 2000), dballe::Trange(254, 0, 0),
                WR_VAR(0, 12, 101));
            wassert(actual(var) != (const wreport::Var*)0);
            const wreport::Var* attr = var->enqa(WR_VAR(0, 8, 44));
            wassert(actual(attr) != (const wreport::Var*)0);
            wassert(actual(attr->enqs()) == "ciao");
        }
        {
            const wreport::Var* var = action.messages.at(3)->get(
                dballe::Level(103, 2000), dballe::Trange(254, 0, 0),
                WR_VAR(0, 12, 101));
            wassert(actual(var) != (const wreport::Var*)0);
            const wreport::Var* attr = var->enqa(WR_VAR(0, 12, 102));
            wassert(actual(attr) != (const wreport::Var*)0);
            wassert(actual(attr->enqd()) == 0.1);
        }
        {
            const wreport::Var* var = action.messages.at(4)->get(
                dballe::Level(103, 2000), dballe::Trange(254, 0, 0),
                WR_VAR(0, 12, 101));
            wassert(actual(var) != (const wreport::Var*)0);
            const wreport::Var* attr1 = var->enqa(WR_VAR(0, 8, 44));
            wassert(actual(attr1) != (const wreport::Var*)0);
            wassert(actual(attr1->enqs()) == "ciao");
            const wreport::Var* attr2 = var->enqa(WR_VAR(0, 12, 102));
            wassert(actual(attr2) != (const wreport::Var*)0);
            wassert(actual(attr2->enqd()) == 0.1);
        }
    });

    add_method("issue77", [] {
        struct TestAction : public Action
        {
            bool operator()(const Item& item) override { return true; }
        };

        ReaderOptions opts;
        opts.input_type = "json";
        Reader reader(opts);
        TestAction action;

        reader.read({dballe::tests::datafile("json/issue77.json")}, action);
    });
}

} // namespace
