#include "levtr.h"
#include "dballe/core/record.h"
#include "dballe/msg/msg.h"

using namespace std;

namespace dballe {
namespace db {
namespace v7 {

LevTrCache::~LevTrCache()
{
    for (auto& i: by_id)
        delete i.second;
}

void LevTrCache::clear()
{
    for (auto& i: by_id)
        delete i.second;
    by_id.clear();
    by_level.clear();
}

const LevTrEntry* LevTrCache::find_station(int id) const
{
    auto i = by_id.find(id);
    if (i == by_id.end())
        return nullptr;
    return i->second;
}

int LevTrCache::find_id(const LevTrEntry& lt) const
{
    if (lt.id != MISSING_INT)
        return lt.id;
    auto li = by_level.find(lt.level);
    if (li == by_level.end())
        return MISSING_INT;
    for (auto i: li->second)
        if (i->level == lt.level && i->trange == lt.trange)
            return i->id;
    return MISSING_INT;
}

const LevTrEntry* LevTrCache::insert(const LevTrEntry& lt)
{
    return insert(std::unique_ptr<LevTrEntry>(new LevTrEntry(lt)));
}

const LevTrEntry* LevTrCache::insert(const LevTrEntry& lt, int id)
{
    std::unique_ptr<LevTrEntry> nlt(new LevTrEntry(lt));
    nlt->id = id;
    return insert(move(nlt));
}

const LevTrEntry* LevTrCache::insert(std::unique_ptr<LevTrEntry> lt)
{
    if (lt->id == MISSING_INT)
        throw std::runtime_error("levtr to cache in transaction state must have a database ID");

    auto i = by_id.find(lt->id);
    if (i != by_id.end())
    {
        // LevTrCache do not move: if we have a match on the ID, we just need to
        // enforce that there is no mismatch on the station data
        if (*i->second != *lt)
            throw std::runtime_error("cannot replace a levtr with one with the same ID and different data");
        return i->second;
    }

    const LevTrEntry* res;
    res = lt.get();
    by_id.insert(make_pair(res->id, lt.release()));
    by_level_add(res);
    return res;
}

void LevTrCache::by_level_add(const LevTrEntry* lt)
{
    auto li = by_level.find(lt->level);
    if (li == by_level.end())
        by_level.insert(make_pair(lt->level, std::vector<const LevTrEntry*>{lt}));
    else
        li->second.push_back(lt);
}


LevTr::~LevTr() {}

msg::Context* LevTr::to_msg(State& st, int id, Msg& msg)
{
    auto i = lookup_id(st, id);
    msg::Context& res = msg.obtain_context(i->first.level, i->first.trange);
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
