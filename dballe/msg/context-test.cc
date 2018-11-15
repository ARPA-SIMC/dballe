#include "dballe/msg/tests.h"
#include "context.h"
#include <memory>

using namespace wreport;
using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("compare", []() {
            Level lev(9, 8, 7, 6);
            unique_ptr<msg::Context> c1(new msg::Context(lev, Trange(1, 2, 3)));
            unique_ptr<msg::Context> c2(new msg::Context(lev, Trange(1, 3, 2)));

            wassert(actual(c1->values.size()) == 0);
            wassert(actual(c1->level) == lev);
            wassert(actual(c1->trange) == Trange(1, 2, 3));
            wassert(actual(c2->values.size()) == 0);
            wassert(actual(c2->level) == lev);
            wassert(actual(c2->trange) == Trange(1, 3, 2));

            c1->values.set(var(WR_VAR(0, 1, 1)));
            c2->values.set(var(WR_VAR(0, 1, 1)));

            wassert(actual(c1->compare(*c2)) < 0);
            wassert(actual(c2->compare(*c1)) > 0);
            wassert(actual(c1->compare(*c1)) == 0);
            wassert(actual(c2->compare(*c2)) == 0);

            wassert(actual(c1->compare(lev, Trange(1, 2, 4))) < 0);
            wassert(actual(c1->compare(lev, Trange(1, 2, 2))) > 0);
            wassert(actual(c1->compare(lev, Trange(1, 3, 3))) < 0);
            wassert(actual(c1->compare(lev, Trange(1, 1, 3))) > 0);
            wassert(actual(c1->compare(lev, Trange(2, 2, 3))) < 0);
            wassert(actual(c1->compare(lev, Trange(0, 2, 3))) > 0);
            wassert(actual(c1->compare(Level(9, 8, 7, 7), Trange(1, 2, 3))) < 0);
            wassert(actual(c1->compare(Level(9, 8, 7, 5), Trange(1, 2, 3))) > 0);
            wassert(actual(c1->compare(lev, Trange(1, 2, 3))) == 0);
        });

        // Test Context external ordering
        add_method("compare_external", []() {
            Trange tr(1, 2, 3);
            unique_ptr<msg::Context> c1(new msg::Context(Level(1, 2, 3, 4), tr));
            unique_ptr<msg::Context> c2(new msg::Context(Level(2, 1, 4, 3), tr));

            wassert(actual(c1->values.size()) == 0);
            wassert(actual(c1->level) == Level(1, 2, 3, 4));
            wassert(actual(c2->values.size()) == 0);
            wassert(actual(c2->level) == Level(2, 1, 4, 3));

            wassert(actual(c1->compare(*c2)) < 0);
            wassert(actual(c2->compare(*c1)) > 0);
            wassert(actual(c1->compare(*c1)) == 0);
            wassert(actual(c2->compare(*c2)) == 0);

            wassert(actual(c1->compare(Level(1, 2, 4, 4), tr)) < 0);
            wassert(actual(c1->compare(Level(1, 2, 2, 4), tr)) > 0);
            wassert(actual(c1->compare(Level(1, 3, 3, 4), tr)) < 0);
            wassert(actual(c1->compare(Level(1, 1, 3, 4), tr)) > 0);
            wassert(actual(c1->compare(Level(2, 2, 3, 4), tr)) < 0);
            wassert(actual(c1->compare(Level(0, 2, 3, 4), tr)) > 0);
            wassert(actual(c1->compare(Level(1, 2, 3, 4), tr)) == 0);
        });

        // Test msg::Context internal ordering
        add_method("compare_internal", []() {
            unique_ptr<msg::Context> c(new msg::Context(Level(1, 2, 3, 4), Trange::instant()));

            c->values.set(var(WR_VAR(0, 1, 1)));
            wassert(actual(c->values.size()) == 1);
            c->values.set(var(WR_VAR(0, 1, 7)));
            wassert(actual(c->values.size()) == 2);
            c->values.set(var(WR_VAR(0, 1, 2)));
            wassert(actual(c->values.size()) == 3);
            // Variables with same code must get substituded and not added
            c->values.set(var(WR_VAR(0, 1, 1)));
            wassert(actual(c->values.size()) == 3);

#if 0
            // Check that the datum vector inside the context is in strict ascending order
            for (unsigned i = 0; i < c->values.size() - 1; ++i)
                wassert(actual_varcode(c->data[i]->code()) < c->data[i + 1]->code());
#endif

            wassert(actual(c->values.maybe_var(WR_VAR(0, 1, 1))).istrue());
            wassert(actual_varcode(c->values.maybe_var(WR_VAR(0, 1, 1))->code()) == WR_VAR(0, 1, 1));

            wassert(actual(c->values.maybe_var(WR_VAR(0, 1, 2))).istrue());
            wassert(actual_varcode(c->values.maybe_var(WR_VAR(0, 1, 2))->code()) == WR_VAR(0, 1, 2));

            wassert(actual(c->values.maybe_var(WR_VAR(0, 1, 7))).istrue());
            wassert(actual_varcode(c->values.var(WR_VAR(0, 1, 7)).code()) == WR_VAR(0, 1, 7));

            wassert(actual(c->values.maybe_var(WR_VAR(0, 1, 8))) == (const Var*)0);
        });
    }
} test("msg_context");

}
