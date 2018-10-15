#include "tests.h"
#include "tests-lua.h"
#include "msg.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("message", []() {
            // Test message access

            // Get a test message
            Messages msgs = read_msgs("bufr/obs0-1.22.bufr", Encoding::BUFR);
            wassert(actual(msgs.size()) == 1u);
            auto msg = Msg::downcast(msgs[0]);

            dballe::tests::Lua test(
                "function test() \n"
                "  if msg:type() ~= 'synop' then return 'type is '..msg:type()..' instead of synop' end \n"
                "  if msg:size() ~= 17 then return 'size is '..msg:size()..' instead of 17' end \n"

                "  count = 0\n"
                "  msg:foreach(function(x) count = count + 1 end)\n"
                "  if count ~= 17 then return 'count is '..count..' instead of 17' end\n"

                "  count = 0\n"
                "  msg:foreach(function(x) count = count + x:size() end)\n"
                "  if count ~= 43 then return 'count is '..count..' instead of 43' end\n"

                "  count = 0\n"
                "  msg:foreach(function(x) x:foreach(function(y) count = count + 1 end) end)\n"
                "  if count ~= 43 then return 'count is '..count..' instead of 43' end\n"

                "  context = nil\n"
                "  msg:foreach(function(x) context=x end)\n"
                "  if context.ltype1 == nil then return 'context.ltype1 is nil' end\n"
                "  if context.l1 == nil then return 'context.l1 is nil' end\n"
                "  if context.ltype2 == nil then return 'context.ltype2 is nil' end\n"
                "  if context.l2 == nil then return 'context.l2 is nil' end\n"
                "  if context.pind == nil then return 'context.pind is nil' end\n"
                "  if context.p1 == nil then return 'context.p1 is nil' end\n"
                "  if context.p2 == nil then return 'context.p2 is nil' end\n"

                "  var = msg:find('temp_2m')\n"
                "  if var == nil then return 'temp_2m is nil' end\n"
                "  if var:enqd() ~= 289.2 then return 'temp_2m is '..var:enqd()..' instead of 289.2' end\n"

                "  var = msg:find('B12101', 103, 2000, nil, nil, 254, 0, 0)\n"
                "  if var == nil then return 'B12101 is nil' end\n"
                "  if var:enqd() ~= 289.2 then return 'B12101 is '..var:enqd()..' instead of 289.2' end\n"
                "end \n"
            );

            // Push the variable as a global
            msg->lua_push(test.L);
            lua_setglobal(test.L, "msg");

            // Check that we can retrieve it
            lua_getglobal(test.L, "msg");
            Msg* msg1 = Msg::lua_check(test.L, 1);
            lua_pop(test.L, 1);
            wassert(actual(msg.get() == msg1).istrue());

            wassert(actual(test.run()) == "");
        });
    }
} test("lua");

}
