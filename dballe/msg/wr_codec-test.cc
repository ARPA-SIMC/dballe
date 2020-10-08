#include "tests.h"
#include "wr_codec.h"
#include "dballe/file.h"
#include <cstring>

using namespace std;
using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("msg_wr_codec");


void Tests::register_tests() {

add_method("issue239", []() {
    auto msg = std::make_shared<impl::Message>();
    msg->set_datetime(Datetime(2020, 10, 8, 0, 0, 0));
    msg->set_rep_memo("test");
    msg->set_longitude(12.12345);
    msg->set_latitude(43.12345);
    msg->obtain_context(Level(1), Trange(254, 0, 0)).values.set(WR_VAR(0, 13, 211), 12.123);

    auto exporter = Exporter::create(Encoding::BUFR);

    std::vector<std::shared_ptr<Message>> msgs {msg};
    auto bulletin = exporter->to_bulletin(msgs);

    BinaryMessage bmsg(Encoding::BUFR);
    bmsg.data = bulletin->encode();

    auto importer = Importer::create(Encoding::BUFR);
    auto imported = importer->from_binary(bmsg);

    const wreport::Var* var = imported[0]->get(Level(1), Trange(254, 0, 0), WR_VAR(0, 13, 211));
    wassert(actual(var->code()) == WR_VAR(0, 13, 211));
    wassert(actual(var->enqi()) == 12);
});

}

}
