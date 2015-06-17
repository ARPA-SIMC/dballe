#include "tests.h"
#include "wr_codec.h"
#include <cstring>

using namespace std;
using namespace wibble::tests;
using namespace dballe;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("empty", [](Fixture& f) {
    }),
};

test_group newtg("msg_wr_codec", tests);

}

