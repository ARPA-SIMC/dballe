#include "cache.h"

using namespace std;

namespace dballe {
namespace db {
namespace v7 {

template<typename Entry>
ForwardCache<Entry>::~ForwardCache()
{
    for (auto& i: by_id)
        delete i.second;
}

template<typename Entry>
void ForwardCache<Entry>::clear()
{
    for (auto& i: by_id)
        delete i.second;
    by_id.clear();
}

template<typename Entry>
const Entry* ForwardCache<Entry>::find_entry(int id) const
{
    auto i = by_id.find(id);
    if (i == by_id.end())
        return nullptr;
    return i->second;
}

template<typename Entry>
const Entry* ForwardCache<Entry>::insert(std::unique_ptr<Entry> e)
{
    if (e->id == MISSING_INT)
        throw std::runtime_error("station to cache in transaction state must have a database ID");

    auto i = by_id.find(e->id);
    if (i != by_id.end())
    {
        // StationCache do not move: if we have a match on the ID, we just need to
        // enforce that there is no mismatch on the station data
        if (*i->second != *e)
            throw std::runtime_error("cannot replace a cached DB entry with one with the same ID and different data");
        return i->second;
    }

    const Entry* res = e.get();
    by_id.insert(make_pair(res->id, e.release()));
    return res;
}


template<typename Entry, typename Reverse>
void Cache<Entry, Reverse>::clear()
{
    ForwardCache<Entry>::clear();
    reverse.clear();
}

template<typename Entry, typename Reverse>
int Cache<Entry, Reverse>::find_id(const Entry& e) const
{
    if (e.id != MISSING_INT)
        return e.id;
    return reverse.find_id(e);
}

template<typename Entry, typename Reverse>
const Entry* Cache<Entry, Reverse>::insert(const Entry& e)
{
    return insert(std::unique_ptr<Entry>(new Entry(e)));
}

template<typename Entry, typename Reverse>
const Entry* Cache<Entry, Reverse>::insert(const Entry& e, int id)
{
    std::unique_ptr<Entry> ne(new Entry(e));
    ne->id = id;
    return insert(move(ne));
}

template<typename Entry, typename Reverse>
const Entry* Cache<Entry, Reverse>::insert(std::unique_ptr<Entry> e)
{
    if (e->id == MISSING_INT)
        throw std::runtime_error("station to cache in transaction state must have a database ID");

    auto i = this->by_id.find(e->id);
    if (i != this->by_id.end())
    {
        // StationCache do not move: if we have a match on the ID, we just need to
        // enforce that there is no mismatch on the station data
        if (*i->second != *e)
            throw std::runtime_error("cannot replace a cached DB entry with one with the same ID and different data");
        return i->second;
    }

    const Entry* res = e.get();
    this->by_id.insert(make_pair(res->id, e.release()));
    reverse.add(res);
    return res;
}



int StationReverseIndex::find_id(const dballe::Station& st) const
{
    auto li = find(st.coords.lon);
    if (li == end())
        return MISSING_INT;
    for (auto i: li->second)
        if (i->report == st.report && i->coords == st.coords && i->ident == st.ident)
            return i->id;
    return MISSING_INT;
}

void StationReverseIndex::add(const dballe::Station* st)
{
    auto li = find(st->coords.lon);
    if (li == end())
        insert(make_pair(st->coords.lon, std::vector<const dballe::Station*>{st}));
    else
        li->second.push_back(st);
}


int LevTrReverseIndex::find_id(const LevTrEntry& lt) const
{
    auto li = find(lt.level);
    if (li == end())
        return MISSING_INT;
    for (auto i: li->second)
        if (i->level == lt.level && i->trange == lt.trange)
            return i->id;
    return MISSING_INT;
}

void LevTrReverseIndex::add(const LevTrEntry* lt)
{
    auto li = find(lt->level);
    if (li == end())
        insert(make_pair(lt->level, std::vector<const LevTrEntry*>{lt}));
    else
        li->second.push_back(lt);
}


template class ForwardCache<dballe::Station>;
template class ForwardCache<LevTrEntry>;
template class ForwardCache<StationValueEntry>;
template class ForwardCache<ValueEntry>;
template class Cache<dballe::Station, StationReverseIndex>;
template class Cache<LevTrEntry, LevTrReverseIndex>;

}
}
}
