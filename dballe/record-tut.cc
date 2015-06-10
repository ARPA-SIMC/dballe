#include "core/test-utils-core.h"
#include "record.h"

using namespace std;
using namespace wibble::tests;
using namespace dballe;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("record", [](Fixture& f) {
    }),
    Test("metadata", [](Fixture& f) {
        wreport::Varinfo info = Record::key_info("rep_memo");
        wassert(actual(info->is_string()).istrue());

        info = Record::key_info("ana_id");
        wassert(actual(info->is_string()).isfalse());
    }),
};

test_group newtg("dballe_record", tests);

}
