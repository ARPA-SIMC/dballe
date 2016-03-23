#ifndef DBALLE_DB_V7_STATE_H
#define DBALLE_DB_V7_STATE_H

#include <dballe/core/defs.h>
#include <map>

namespace dballe {
namespace db {
namespace v7 {

#if 0
struct ValueDesc
{
    /// Date and time at which the value was measured or forecast
    Datetime datetime;

    wreport::Varcode varcode;
};

struct ValueState
{
    /// wreport::Var representing the value
    wreport::Var* var = nullptr;

    /// Database ID
    int id;

    // True if the value has just been inserted
    bool is_new;

    std::map<wreport::Varcode, ValueState> attributes;
};
#endif

struct StationDesc
{
    int rep;
    Coords coords;
    Ident ident;

    int compare(const StationDesc&) const;
    bool operator<(const StationDesc& o) const { return compare(o) < 0; }
};

struct StationState
{
    // Database ID
    int id;

    // True if the station has just been inserted
    bool is_new;

#if 0
    std::map<wreport::Varcode, ValueState> station_values;

    std::map<LevTrDesc, LevTrState> levtrs;
#endif
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

struct LevTrState
{
    /// ID in the database
    int id;

    // True if the value has just been inserted
    bool is_new;

#if 0
    std::map<ValueDesc, ValueState> values;
#endif
};

/**
 * Cache intermediate results during a database transaction, to avoid hitting
 * the database multiple times when we already know values from previous
 * operations.
 */
struct State
{
    typedef std::map<StationDesc, StationState> stations_t;
    typedef std::map<LevTrDesc, LevTrState> levels_t;
    typedef std::map<int, levels_t::iterator> level_id_t;

    stations_t stations;
    levels_t levels;
    level_id_t level_ids;

    stations_t::iterator add_station(const StationDesc& desc, const StationState& state);
    levels_t::iterator add_levtr(const LevTrDesc& desc, const LevTrState& state);

    /// Clear the state, removing all cached data
    void clear();
};

}
}
}
#endif
