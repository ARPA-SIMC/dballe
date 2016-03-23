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

struct LevTrDesc
{
    /// Vertical level or layer
    Level level;

    /// Time range
    Trange trange;
}

struct LevTrState
{
    /// ID in the database
    int id;

    // True if the value has just been inserted
    bool is_new;

    std::map<ValueDesc, ValueState> values;
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

/**
 * Cache intermediate results during a database transaction, to avoid hitting
 * the database multiple times when we already know values from previous
 * operations.
 */
struct State
{
    typedef std::map<StationDesc, StationState> stations_t;

    stations_t stations;

    /// Clear the state, removing all cached data
    void clear();
};

}
}
}
#endif
