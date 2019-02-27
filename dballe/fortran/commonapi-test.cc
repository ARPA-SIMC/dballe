#include "dballe/core/tests.h"
#include "commonapi.h"

using namespace std;
using namespace dballe;
using namespace dballe::fortran;
using namespace dballe::tests;

namespace {

struct APITest : public CommonAPIImplementation
{
     void reinit_db(const char* repinfofile=0) override {}
     void remove_all() override {}
     int query_stations() override { return 0; }
     void next_station() override {}
     int query_data() override { return 0; }
     wreport::Varcode next_data() override { return 0; }
     void insert_data() override {}
     void remove_data() override {}
     int query_attributes() override { return 0; }
     void insert_attributes() override {}
     void remove_attributes() override {}
     void messages_open_input(const char* filename, const char* mode, Encoding format, bool simplified=true) override {}
     void messages_open_output(const char* filename, const char* mode, Encoding format) override {}
     bool messages_read_next() override { return false; }
     void messages_write_next(const char*) override {}
};


class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} newtg("fortran_commonapi");

void Tests::register_tests()
{
    add_method("seti", []() {
        APITest api;
        api.seti("ana_id", 1);
        wassert(actual(api.test_get_input_query().ana_id) == 1);
        wassert(actual(api.test_get_input_data().station.id) == 1);
    });

    add_method("set_undef_key", []() {
        APITest api;
        try {
            api.seti("", 1);
            wassert(throw TestFailed("setting an empty keyword should raise error_notfound"));
        } catch (wreport::error_notfound&) {
        }
    });
}

}
