#ifndef DBALLE_DB_V7_CACHE_H
#define DBALLE_DB_V7_CACHE_H

#include <dballe/db/v7/state.h>
#include <unordered_map>

namespace dballe {
namespace db {
namespace v7 {

template<typename Entry>
struct ForwardCache
{
    std::unordered_map<int, Entry*> by_id;

    ForwardCache() = default;
    ForwardCache(const ForwardCache&) = delete;
    ForwardCache(ForwardCache&&) = delete;
    ForwardCache& operator=(const ForwardCache&) = delete;
    ForwardCache& operator=(ForwardCache&&) = delete;
    ~ForwardCache();

    void clear();
    const Entry* find_entry(int id) const;
    const Entry* insert(std::unique_ptr<Entry> e);
};

template<typename Entry, typename Reverse>
struct Cache : public ForwardCache<Entry>
{
    using ForwardCache<Entry>::ForwardCache;
    Reverse reverse;

    int find_id(const Entry& e) const;

    const Entry* insert(const Entry& e);
    const Entry* insert(const Entry& e, int id);
    const Entry* insert(std::unique_ptr<Entry> e);

    void clear();
};

struct StationReverseIndex : public std::unordered_map<int, std::vector<const dballe::Station*>>
{
    int find_id(const dballe::Station& st) const;
    void add(const dballe::Station* st);
};

struct StationCache : Cache<dballe::Station, StationReverseIndex>
{
    using Cache::Cache;
};



struct LevTrReverseIndex : public std::unordered_map<Level, std::vector<const LevTrEntry*>>
{
    int find_id(const LevTrEntry& st) const;
    void add(const LevTrEntry* st);
};

struct LevTrCache : public Cache<LevTrEntry, LevTrReverseIndex>
{
    using Cache::Cache;
};


struct StationValueCache : public ForwardCache<StationValueEntry>
{
    using ForwardCache::ForwardCache;
};

struct ValueCache : public ForwardCache<ValueEntry>
{
    using ForwardCache::ForwardCache;
};


extern template class ForwardCache<dballe::Station>;
extern template class ForwardCache<LevTrEntry>;
extern template class ForwardCache<StationValueEntry>;
extern template class ForwardCache<ValueEntry>;
extern template class Cache<dballe::Station, StationReverseIndex>;
extern template class Cache<LevTrEntry, LevTrReverseIndex>;

}
}
}

#endif
