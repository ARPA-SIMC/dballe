#ifndef DBALLE_DB_V7_CACHE_H
#define DBALLE_DB_V7_CACHE_H

#include <dballe/db/v7/state.h>
#include <unordered_map>

namespace dballe {
namespace db {
namespace v7 {

template<typename Entry, typename Reverse>
struct Cache
{
    std::unordered_map<int, Entry*> by_id;
    Reverse reverse;

    Cache() = default;
    Cache(const Cache&) = delete;
    Cache(Cache&&) = delete;
    Cache& operator=(const Cache&) = delete;
    Cache& operator=(Cache&&) = delete;
    ~Cache();

    const Entry* find_entry(int id) const;
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

extern template class Cache<dballe::Station, StationReverseIndex>;
extern template class Cache<LevTrEntry, LevTrReverseIndex>;

}
}
}

#endif
