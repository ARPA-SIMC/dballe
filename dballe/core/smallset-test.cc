#include "dballe/core/smallset.h"
#include "dballe/core/tests.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_smallset");

struct IntSmallSet : public core::SmallSet<int>
{
};

void Tests::register_tests()
{

    add_method("by_1", [] {
        IntSmallSet s;
        for (int i = 0; i < 32; ++i)
        {
            s.add(i);
            auto it = s.find(i);
            wassert_true(it != s.end());
            wassert(actual(*it) == i);
        }

        for (int i = 0; i < 32; ++i)
        {
            auto it = s.find(i);
            wassert_true(it != s.end());
            wassert(actual(*it) == i);
        }

        wassert_true(s.find(33) == s.end());
    });

    add_method("by_5", [] {
        IntSmallSet s;
        for (int i = 0; i < 32; ++i)
        {
            for (int pad = 0; pad < 5; ++pad)
                s.add((i + 1) * 100 + pad);
            s.add(i);
            auto it = s.find(i);
            wassert_true(it != s.end());
            wassert(actual(*it) == i);
        }

        for (int i = 0; i < 32; ++i)
        {
            auto it = s.find(i);
            wassert_true(it != s.end());
            wassert(actual(*it) == i);
        }

        wassert_true(s.find(33) == s.end());
    });

    add_method("by_64", [] {
        IntSmallSet s;
        for (int i = 0; i < 32; ++i)
        {
            for (int pad = 0; pad < 64; ++pad)
                s.add((i + 1) * 100 + pad);
            s.add(i);
            auto it = s.find(i);
            wassert_true(it != s.end());
            wassert(actual(*it) == i);
        }

        for (int i = 0; i < 32; ++i)
        {
            auto it = s.find(i);
            wassert_true(it != s.end());
            wassert(actual(*it) == i);
        }

        wassert_true(s.find(33) == s.end());
    });
}

} // namespace
