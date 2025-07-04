#include "levtr.h"
#include "dballe/msg/msg.h"

using namespace std;

namespace dballe {
namespace db {
namespace v7 {

LevTr::LevTr(v7::Transaction& tr) : tr(tr) {}

LevTr::~LevTr() {}

void LevTr::clear_cache() { cache.clear(); }

const LevTrEntry& LevTr::lookup_cache(int id)
{
    const LevTrEntry* res = cache.find_entry(id);
    if (!res)
        wreport::error_notfound::throwf("LevTr with ID %d not found in cache",
                                        id);
    return *res;
}

impl::msg::Context* LevTr::to_msg(Tracer<>& trc, int id, impl::Message& msg)
{
    auto i                  = lookup_id(trc, id);
    impl::msg::Context& res = msg.obtain_context(i->level, i->trange);
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
        while (written++ < 21)
            putc(' ', out);
        written = trange.print(out, "-", "");
        while (written++ < 11)
            putc(' ', out);
        putc('\n', out);
        ++count;
    });
    fprintf(out, "%d element%s in table levtr\n", count, count != 1 ? "s" : "");
}

} // namespace v7
} // namespace db
} // namespace dballe
