#include "core/tests.h"
#include "stlutils.h"

using namespace dballe;
using namespace dballe::stl;
using namespace wreport::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        // Test Intersection
        add_method("intersection", []() {
            unique_ptr<stl::Sequences<int> > sequences(new stl::Sequences<int>);

            vector<int> a;
            a.push_back(1); a.push_back(2); a.push_back(3);
            sequences->add(a);

            vector<int> b;
            b.push_back(2); b.push_back(3);
            sequences->add(b);

            vector<int> c;
            c.push_back(0); c.push_back(2);
            sequences->add(c);

            Intersection<int> intersection;

            Intersection<int>::const_iterator i = intersection.begin(sequences);
            wassert(actual(i != intersection.end()).istrue());
            wassert(actual(*i) == 2);
            ++i;
            wassert(actual(i == intersection.end()).istrue());
        });

        // Test Intersection
        add_method("intersection1", []() {
            unique_ptr<stl::Sequences<int> > sequences(new stl::Sequences<int>);

            vector<int> a;
            a.push_back(1);
            sequences->add(a);

            vector<int> b;
            b.push_back(1);
            sequences->add(b);

            Intersection<int> intersection;
            Intersection<int>::const_iterator i = intersection.begin(sequences);
            wassert(actual(i != intersection.end()).istrue());
            wassert(actual(*i) == 1);
            ++i;
            wassert(actual(i == intersection.end()).istrue());
        });

        // Test SetIntersection
        add_method("set_intersection", []() {
            stl::SetIntersection<int> intersection;

            set<int> a;
            a.insert(1); a.insert(2); a.insert(3);
            intersection.add(a);

            set<int> b;
            b.insert(2); b.insert(3);
            intersection.add(b);

            set<int> c;
            c.insert(0); c.insert(2);
            intersection.add(c);

            stl::SetIntersection<int>::const_iterator i = intersection.begin();
            wassert(actual(i != intersection.end()).istrue());
            wassert(actual(*i) == 2);
            ++i;
            wassert(actual(i == intersection.end()).istrue());
        });

        // Test Union
        add_method("union", []() {
            unique_ptr<stl::Sequences<int> > sequences(new stl::Sequences<int>);
            vector<int> a;
            sequences->add(a);
            vector<int> b;
            sequences->add(b);

            Union<int> ab;
            Union<int>::const_iterator i = ab.begin(sequences);
            wassert(actual(i == ab.end()).istrue());
        });

        // Test Union
        add_method("union1", []() {
            unique_ptr<stl::Sequences<int> > sequences(new stl::Sequences<int>);
            vector<int> a;
            a.push_back(1);
            sequences->add(a);
            vector<int> b;
            sequences->add(b);

            Union<int> ab;
            Union<int>::const_iterator i = ab.begin(sequences);
            wassert(actual(i != ab.end()).istrue());
            wassert(actual(*i) == 1);
            ++i;
            wassert(actual(i == ab.end()).istrue());
        });

        // Test Union
        add_method("union2", []() {
            unique_ptr<stl::Sequences<int> > sequences(new stl::Sequences<int>);
            vector<int> a;
            a.push_back(1);
            a.push_back(2);
            sequences->add(a);
            vector<int> b;
            b.push_back(2);
            b.push_back(3);
            sequences->add(b);

            Union<int> ab;
            Union<int>::const_iterator i = ab.begin(sequences);
            wassert(actual(i != ab.end()).istrue());
            wassert(actual(*i) == 1);
            ++i;
            wassert(actual(i != ab.end()).istrue());
            wassert(actual(*i) == 2);
            ++i;
            wassert(actual(i != ab.end()).istrue());
            wassert(actual(*i) == 3);
            ++i;
            wassert(actual(i == ab.end()).istrue());
        });
    }
} test("core_stlutils");

}

#include "stlutils.tcc"
