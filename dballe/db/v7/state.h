#ifndef DBALLE_DB_V7_STATE_H
#define DBALLE_DB_V7_STATE_H

#include <wreport/var.h>
#include <dballe/core/defs.h>
#include <map>
#include <unordered_set>

namespace dballe {
namespace db {
namespace v7 {

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
};

typedef std::map<StationDesc, StationState> stations_t;


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
};

typedef std::map<LevTrDesc, LevTrState> levtrs_t;
typedef std::map<int, levtrs_t::iterator> levtr_id_t;


struct StationValueDesc
{
    stations_t::iterator station;
    wreport::Varcode varcode;

    StationValueDesc() {}
    StationValueDesc(const StationValueDesc&) = default;
    StationValueDesc(stations_t::iterator station, wreport::Varcode varcode)
        : station(station), varcode(varcode) {}
    StationValueDesc& operator=(const StationValueDesc&) = default;

    int compare(const StationValueDesc&) const;
    bool operator<(const StationValueDesc& o) const { return compare(o) < 0; }
};

struct StationValueState
{
    /// Variable value
    std::string value;

    /// Database ID
    int id;

    /// True if the station value has just been inserted
    bool is_new;
};

typedef std::map<StationValueDesc, StationValueState> stationvalues_t;


struct ValueDesc
{
    stations_t::iterator station;
    levtrs_t::iterator levtr;
    /// Date and time at which the value was measured or forecast
    Datetime datetime;
    wreport::Varcode varcode;

    ValueDesc() {}
    ValueDesc(const ValueDesc&) = default;
    ValueDesc(stations_t::iterator station, levtrs_t::iterator levtr, const Datetime& datetime, wreport::Varcode varcode)
        : station(station), levtr(levtr), datetime(datetime), varcode(varcode) {}
    ValueDesc& operator=(const ValueDesc&) = default;

    int compare(const ValueDesc&) const;
    bool operator<(const ValueDesc& o) const { return compare(o) < 0; }
};

struct ValueState
{
    /// Variable value
    std::string value;

    /// Database ID
    int id;

    /// True if the value has just been inserted
    bool is_new;
};

typedef std::map<ValueDesc, ValueState> values_t;


/**
 * Cache intermediate results during a database transaction, to avoid hitting
 * the database multiple times when we already know values from previous
 * operations.
 */
struct State
{
    stations_t stations;
    levtrs_t levtrs;
    levtr_id_t levtr_ids;
    stationvalues_t stationvalues;
    values_t values;
    std::unordered_set<int> stationvalues_new;
    std::unordered_set<int> values_new;

    stations_t::iterator add_station(const StationDesc& desc, const StationState& state);
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
