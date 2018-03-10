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

struct StationValueEntry
{
    int id = MISSING_INT;
    int station;
    wreport::Varcode varcode;

    StationValueEntry() {}
    StationValueEntry(const StationValueEntry&) = default;
    StationValueEntry(int id, int station, wreport::Varcode varcode)
        : id(id), station(station), varcode(varcode) {}
    StationValueEntry(int station, wreport::Varcode varcode)
        : station(station), varcode(varcode) {}
    StationValueEntry& operator=(const StationValueEntry&) = default;

    int compare(const StationValueEntry&) const;
    bool operator<(const StationValueEntry& o) const { return compare(o) < 0; }
    bool operator==(const StationValueEntry& o) const
    {
        if (id != MISSING_INT && o.id != MISSING_INT)
            return id == o.id;
        if (station != o.station) return false;
        return varcode == o.varcode;
    }
    bool operator!=(const StationValueEntry& o) const
    {
        if (id != MISSING_INT && o.id != MISSING_INT)
            return id != o.id;
        if (station == o.station) return false;
        return varcode != o.varcode;
    }
};


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


struct ValueEntry
{
    int id = MISSING_INT;
    int station;
    int levtr;
    /// Date and time at which the value was measured or forecast
    Datetime datetime;
    wreport::Varcode varcode;

    ValueEntry() {}
    ValueEntry(const ValueEntry&) = default;
    ValueEntry(int id, int station, int levtr, const Datetime& datetime, wreport::Varcode varcode)
        : id(id), station(station), levtr(levtr), datetime(datetime), varcode(varcode) {}
    ValueEntry(int station, int levtr, const Datetime& datetime, wreport::Varcode varcode)
        : station(station), levtr(levtr), datetime(datetime), varcode(varcode) {}
    ValueEntry& operator=(const ValueEntry&) = default;

    int compare(const ValueEntry&) const;
    bool operator<(const ValueEntry& o) const { return compare(o) < 0; }
    bool operator==(const ValueEntry& o) const
    {
        if (id != MISSING_INT && o.id != MISSING_INT)
            return id == o.id;
        if (station != o.station) return false;
        if (levtr != o.levtr) return false;
        return varcode == o.varcode;
    }
    bool operator!=(const ValueEntry& o) const
    {
        if (id != MISSING_INT && o.id != MISSING_INT)
            return id != o.id;
        if (station == o.station) return false;
        if (levtr == o.levtr) return false;
        return varcode != o.varcode;
    }
};

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
    stationvalues_t stationvalues;
    values_t values;
    std::unordered_set<int> stationvalues_new;
    std::unordered_set<int> values_new;

    stationvalues_t::iterator add_stationvalue(const StationValueDesc& desc, const StationValueState& state);
    values_t::iterator add_value(const ValueDesc& desc, const ValueState& state);

    /// Clear the state, removing all cached data
    void clear();
};

}
}
}
#endif
