#include "levtr.h"
#include "dballe/core/record.h"
#include "dballe/msg/msg.h"

using namespace std;

namespace dballe {
namespace db {
namespace v7 {

LevTr::~LevTr() {}

msg::Context* LevTr::to_msg(State& st, int id, Msg& msg)
{
    auto i = lookup_id(st, id);
    msg::Context& res = msg.obtain_context(i->first.level, i->first.trange);
    return &res;
}

}
}
}
