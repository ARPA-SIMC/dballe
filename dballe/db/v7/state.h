#ifndef DBALLE_DB_V7_STATE_H
#define DBALLE_DB_V7_STATE_H

#include <wreport/var.h>
#include <dballe/core/defs.h>
#include <dballe/core/values.h>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace dballe {
struct Record;

namespace db {
namespace v7 {

struct Stations
{
    std::unordered_map<int, dballe::Station*> by_id;
    std::unordered_map<int, std::vector<const dballe::Station*>> by_lon;

    Stations() = default;
    Stations(const Stations&) = delete;
    Stations(Stations&&) = delete;
    Stations& operator=(const Stations&) = delete;
    Stations& operator=(Stations&&) = delete;
    ~Stations();

    const dballe::Station* insert(const dballe::Station& st);
    const dballe::Station* insert(const dballe::Station& st, int id);
    const dballe::Station* insert(std::unique_ptr<dballe::Station> st);

    const dballe::Station* find_station(int id) const;
    int find_id(const dballe::Station& st) const;

    void clear();

protected:
    void by_lon_add(const dballe::Station* st);
};


struct ItemState
{
    // Database ID
    int id;

    // True if the item has just been inserted
    bool is_new;

    ItemState() {}
    ItemState(int id, bool is_new) : id(id), is_new(is_new) {}
    ItemState(const ItemState&) = default;
    ItemState(ItemState&&) = default;
    ItemState& operator=(const ItemState&) = default;
    ItemState& operator=(ItemState&&) = default;
};


struct LevTrDesc
{
    /// Vertical level or layer
    Level level;

    /// Time range
    Trange trange;

    LevTrDesc() = default;
    LevTrDesc(const Level& level, const Trange& trange) : level(level), trange(trange) {}
    LevTrDesc(const LevTrDesc&) = default;
    LevTrDesc(LevTrDesc&&) = default;
    LevTrDesc& operator=(const LevTrDesc&) = default;
    LevTrDesc& operator=(LevTrDesc&&) = default;

    int compare(const LevTrDesc&) const;
    bool operator<(const LevTrDesc& o) const { return compare(o) < 0; }
};

struct LevTrState : public ItemState
{
    using ItemState::ItemState;
};

typedef std::map<LevTrDesc, LevTrState> levtrs_t;
typedef std::map<int, levtrs_t::iterator> levtr_id_t;


struct StationValueDesc
{
    int station;
    wreport::Varcode varcode;

    StationValueDesc() {}
    StationValueDesc(const StationValueDesc&) = default;
    StationValueDesc(int station, wreport::Varcode varcode)
        : station(station), varcode(varcode) {}
    StationValueDesc& operator=(const StationValueDesc&) = default;

    int compare(const StationValueDesc&) const;
    bool operator<(const StationValueDesc& o) const { return compare(o) < 0; }
};

struct StationValueState : public ItemState
{
    using ItemState::ItemState;
};

typedef std::map<StationValueDesc, StationValueState> stationvalues_t;


struct ValueDesc
{
    int station;
    int levtr;
    /// Date and time at which the value was measured or forecast
    Datetime datetime;
    wreport::Varcode varcode;

    ValueDesc() {}
    ValueDesc(const ValueDesc&) = default;
    ValueDesc(int station, int levtr, const Datetime& datetime, wreport::Varcode varcode)
        : station(station), levtr(levtr), datetime(datetime), varcode(varcode) {}
    ValueDesc& operator=(const ValueDesc&) = default;

    int compare(const ValueDesc&) const;
    bool operator<(const ValueDesc& o) const { return compare(o) < 0; }
};

struct ValueState : public ItemState
{
    using ItemState::ItemState;
};

typedef std::map<ValueDesc, ValueState> values_t;


/**
 * Cache intermediate results during a database transaction, to avoid hitting
 * the database multiple times when we already know values from previous
 * operations.
 */
struct State
{
    Stations stations;
    levtrs_t levtrs;
    levtr_id_t levtr_ids;
    stationvalues_t stationvalues;
    values_t values;
    std::unordered_set<int> stationvalues_new;
    std::unordered_set<int> values_new;

    levtrs_t::iterator add_levtr(const LevTrDesc& desc, const LevTrState& state);
    stationvalues_t::iterator add_stationvalue(const StationValueDesc& desc, const StationValueState& state);
    values_t::iterator add_value(const ValueDesc& desc, const ValueState& state);

    /// Clear the state, removing all cached data
    void clear();
};

}
}
}
#endif
