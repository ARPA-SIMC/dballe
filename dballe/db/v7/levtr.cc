#include "levtr.h"
#include "dballe/core/record.h"
#include "dballe/msg/msg.h"

using namespace std;

namespace dballe {
namespace db {
namespace v7 {

LevTr::~LevTr() {}

void LevTr::clear_cache()
{
    cache.clear();
}

msg::Context* LevTr::to_msg(State& st, int id, Msg& msg)
{
    auto i = lookup_id(id);
    msg::Context& res = msg.obtain_context(i->level, i->trange);
    return &res;
}

void LevTr::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table levtr:\n");
    fprintf(out, "   id   lev                  tr\n");
    _dump([&](int id, const Level& level, const Trange& trange) {
        fprintf(out, " %4d   ", id);
        int written = level.print(out, "-", "");
        while (written++ < 21) putc(' ', out);
        written = trange.print(out, "-", "");
        while (written++ < 11) putc(' ', out);
        putc('\n', out);
        ++count;
    });
    fprintf(out, "%d element%s in table levtr\n", count, count != 1 ? "s" : "");
}

}
}
}
