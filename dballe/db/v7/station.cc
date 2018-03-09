#include "station.h"
#include "dballe/core/values.h"
#include "transaction.h"

using namespace wreport;
using namespace dballe::db;
using namespace std;

namespace dballe {
namespace db {
namespace v7 {

StationCache::~StationCache()
{
    for (auto& i: by_id)
        delete i.second;
}

void StationCache::clear()
{
    for (auto& i: by_id)
        delete i.second;
    by_id.clear();
    by_lon.clear();
}

const dballe::Station* StationCache::find_station(int id) const
{
    auto i = by_id.find(id);
    if (i == by_id.end())
        return nullptr;
    return i->second;
}

int StationCache::find_id(const dballe::Station& st) const
{
    if (st.ana_id != MISSING_INT)
        return st.ana_id;
    auto li = by_lon.find(st.coords.lon);
    if (li == by_lon.end())
        return MISSING_INT;
    for (auto i: li->second)
        if (i->report == st.report && i->coords == st.coords && i->ident == st.ident)
            return i->ana_id;
    return MISSING_INT;
}

const dballe::Station* StationCache::insert(const dballe::Station& st)
{
    return insert(std::unique_ptr<dballe::Station>(new dballe::Station(st)));
}

const dballe::Station* StationCache::insert(const dballe::Station& st, int id)
{
    std::unique_ptr<dballe::Station> nst(new dballe::Station(st));
    nst->ana_id = id;
    return insert(move(nst));
}

const dballe::Station* StationCache::insert(std::unique_ptr<dballe::Station> st)
{
    if (st->ana_id == MISSING_INT)
        throw std::runtime_error("station to cache in transaction state must have a database ID");

    auto i = by_id.find(st->ana_id);
    if (i != by_id.end())
    {
        // StationCache do not move: if we have a match on the ID, we just need to
        // enforce that there is no mismatch on the station data
        if (*i->second != *st)
            throw std::runtime_error("cannot replace a station with one with the same ID and different data");
        return i->second;
    }

    const dballe::Station* res;
    res = st.get();
    by_id.insert(make_pair(res->ana_id, st.release()));
    by_lon_add(res);
    return res;
}

void StationCache::by_lon_add(const dballe::Station* st)
{
    auto li = by_lon.find(st->coords.lon);
    if (li == by_lon.end())
        by_lon.insert(make_pair(st->coords.lon, std::vector<const dballe::Station*>{st}));
    else
        li->second.push_back(st);
}



Station::~Station()
{
}

void Station::clear_cache()
{
    cache.clear();
}

int Station::get_id(v7::Transaction& tr, const dballe::Station& desc)
{
    auto id = cache.find_id(desc);
    if (id != MISSING_INT)
        return id;

    if (maybe_get_id(tr, desc, &id))
    {
        cache.insert(desc, id);
        return id;
    }

    throw error_notfound("station not found in the database");
}

void Station::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table station:\n");

    _dump([&](int id, int rep, const Coords& coords, const char* ident) {
        fprintf(out, " %d, %d, %.5f, %.5f", id, rep, coords.dlat(), coords.dlon());
        if (!ident)
            putc('\n', out);
        else
            fprintf(out, ", %s\n", ident);
        ++count;
    });
    fprintf(out, "%d element%s in table station\n", count, count != 1 ? "s" : "");
}

}
}
}
