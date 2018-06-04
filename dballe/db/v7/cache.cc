#include "cache.h"
#include "dballe/core/values.h"
#include <ostream>

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
}

const dballe::Station* StationCache::find_entry(int id) const
{
    auto i = by_id.find(id);
    if (i == by_id.end())
        return nullptr;
    return i->second;
}

const dballe::Station* StationCache::insert(const dballe::Station& e)
{
    return insert(std::unique_ptr<dballe::Station>(new dballe::Station(e)));
}

const dballe::Station* StationCache::insert(const dballe::Station& e, int id)
{
    std::unique_ptr<dballe::Station> ne(new dballe::Station(e));
    ne->id = id;
    return insert(move(ne));
}

const dballe::Station* StationCache::insert(std::unique_ptr<dballe::Station> e)
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

    const dballe::Station* res = e.get();
    this->by_id.insert(make_pair(res->id, e.release()));
    return res;
}


bool LevTrEntry::operator==(const LevTrEntry& o) const
{
    if (id != MISSING_INT && o.id != MISSING_INT)
        return id == o.id;
    if (level != o.level) return false;
    return trange == o.trange;
}

bool LevTrEntry::operator!=(const LevTrEntry& o) const
{
    if (id != MISSING_INT && o.id != MISSING_INT)
        return id != o.id;
    if (level == o.level) return false;
    return trange != o.trange;
}

std::ostream& operator<<(std::ostream& out, const LevTrEntry& l)
{
    out << "(";
    if (l.id == MISSING_INT)
        out << "-";
    else
        out << l.id;
    return out << ":" << l.level << ":" << l.trange;
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
    reverse.clear();
}

const LevTrEntry* LevTrCache::find_entry(int id) const
{
    auto i = by_id.find(id);
    if (i == by_id.end())
        return nullptr;
    return i->second;
}

const LevTrEntry* LevTrCache::insert(const LevTrEntry& e)
{
    return insert(std::unique_ptr<LevTrEntry>(new LevTrEntry(e)));
}

const LevTrEntry* LevTrCache::insert(const LevTrEntry& e, int id)
{
    std::unique_ptr<LevTrEntry> ne(new LevTrEntry(e));
    ne->id = id;
    return insert(move(ne));
}

const LevTrEntry* LevTrCache::insert(std::unique_ptr<LevTrEntry> e)
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

    const LevTrEntry* res = e.get();
    this->by_id.insert(make_pair(res->id, e.release()));
    reverse.add(res);
    return res;
}

int LevTrCache::find_id(const LevTrEntry& e) const
{
    if (e.id != MISSING_INT)
        return e.id;
    return reverse.find_id(e);
}

}
}
}
