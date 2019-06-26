#include "dballe/msg/tests.h"
#include "cursor.h"
#include <cstring>

using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} tests("msg_cursor");

void Tests::register_tests()
{

add_method("issue160", []() {
    auto msgs = read_msgs("bufr/issue160.bufr", Encoding::BUFR);

    std::shared_ptr<impl::Message> msg(impl::Message::downcast(msgs[0]));
    const Values& station_values = msg->find_station_context();
    wassert(actual(station_values.size()) == 12);

    core::Query query;
    auto cur = msg->query_station_data(query);

    wassert(actual(cur->remaining()) == 12);

    unsigned iterations = 0;
    while (cur->next())
        ++iterations;

    wassert(actual(iterations) == 12);
});

}

}
