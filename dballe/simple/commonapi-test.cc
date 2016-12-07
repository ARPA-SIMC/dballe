#include "core/tests.h"
#include "commonapi.h"

using namespace std;
using namespace dballe;
using namespace dballe::fortran;
using namespace dballe::tests;

namespace {

struct APITest : public CommonAPIImplementation
{
     void scopa(const char* repinfofile=0) override {}
     void remove_all() override {}
     int quantesono() override { return 0; }
     void elencamele() override {}
     int voglioquesto() override { return 0; }
     const char* dammelo() override { return nullptr; }
     void prendilo() override {}
     void dimenticami() override {}
     int voglioancora() override { return 0; }
     void critica() override {}
     void scusa() override {}
     void messages_open_input(const char* filename, const char* mode, File::Encoding format, bool simplified=true) override {}
     void messages_open_output(const char* filename, const char* mode, File::Encoding format) override {}
     bool messages_read_next() override { return false; }
     void messages_write_next(const char*) override {}
};


class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} newtg("simple_commonapi");

void Tests::register_tests()
{
    add_method("seti", []() {
        APITest api;
        api.seti("ana_id", 1);
        wassert(actual(api.test_get_input().get("ana_id")->enqi()) == 1);
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
