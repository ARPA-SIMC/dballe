#include "levtr.h"
#include "dballe/core/record.h"
#include "dballe/msg/msg.h"
#include <map>
#include <sstream>

//using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace sql {

LevTr::~LevTr() {}

LevTrCache::~LevTrCache() {}

struct MapLevTrCache : public LevTrCache
{
    struct Item
    {
        Level level;
        Trange trange;
        Item(const LevTr::DBRow& o)
            : level(o.ltype1, o.l1, o.ltype2, o.l2),
              trange(o.pind, o.p1, o.p2) {}

        void to_record(Record& rec) const
        {
            rec.set(level);
            rec.set(trange);
        }
    };

    mutable LevTr* levtr;
    mutable map<int, Item> cache;

    MapLevTrCache(LevTr& levtr)
        : levtr(&levtr)
    {
        // TODO: make prefetch optional if needed, controlled by an env
        //       variable
        levtr.read_all([&](const LevTr::DBRow& row) {
            cache.insert(make_pair(row.id, Item(row)));
        });
    }

    const Item* get(int id) const
    {
        map<int, Item>::const_iterator i = cache.find(id);

        // Cache hit
        if (i != cache.end()) return &(i->second);

        // Miss: try the DB
        const LevTr::DBRow* row = levtr->read(id);
        if (!row) return 0;

        // Fill cache
        pair<map<int, Item>::iterator, bool> res = cache.insert(make_pair(id, Item(*row)));
        return &(res.first->second);
    }

    bool to_rec(int id, Record& rec)
    {
        const Item* i = get(id);
        if (!i) return 0;
        i->to_record(rec);
        return true;
    }

    Level to_level(int id) const
    {
        const Item* i = get(id);
        if (!i) return Level();
        return i->level;
    }

    Trange to_trange(int id) const
    {
        const Item* i = get(id);
        if (!i) return Trange();
        return i->trange;
    }

    msg::Context* to_msg(int id, Msg& msg)
    {
        const Item* i = get(id);
        if (!i) return 0;
        msg::Context& res = msg.obtain_context(i->level, i->trange);
        return &res;
    }

    void invalidate()
    {
        cache.clear();
    }

    void dump(FILE* out) const
    {
        fprintf(out, "%zd elements in level/timerange cache:\n", cache.size());
        for (map<int, Item>::const_iterator i = cache.begin(); i != cache.end(); ++i)
        {
            stringstream str;
            str << i->second.level;
            str << " ";
            str << i->second.trange;
            fprintf(out, "  %d: %s\n", i->first, str.str().c_str());
        }
    }
};

std::unique_ptr<LevTrCache> LevTrCache::create(LevTr& levtr)
{
    // TODO: check env vars to select alternate caching implementations, such
    // as a hit/miss that queries single items from the DB when not in cache
    return unique_ptr<LevTrCache>(new MapLevTrCache(levtr));
}

}
}
}

