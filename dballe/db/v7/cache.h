#ifndef DBALLE_DB_V7_CACHE_H
#define DBALLE_DB_V7_CACHE_H

#include <dballe/types.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include <iosfwd>

namespace dballe {
struct Station;

namespace db {
namespace v7 {
struct LevTrEntry;


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


struct LevTrEntry
{
    // Database ID
    int id = MISSING_INT;

    /// Vertical level or layer
    Level level;

    /// Time range
    Trange trange;

    LevTrEntry() = default;
    LevTrEntry(int id, const Level& level, const Trange& trange) : id(id), level(level), trange(trange) {}
    LevTrEntry(const Level& level, const Trange& trange) : level(level), trange(trange) {}
    LevTrEntry(const LevTrEntry&) = default;
    LevTrEntry(LevTrEntry&&) = default;
    LevTrEntry& operator=(const LevTrEntry&) = default;
    LevTrEntry& operator=(LevTrEntry&&) = default;

    bool operator==(const LevTrEntry& o) const;
    bool operator!=(const LevTrEntry& o) const;
};

std::ostream& operator<<(std::ostream&, const LevTrEntry&);

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
