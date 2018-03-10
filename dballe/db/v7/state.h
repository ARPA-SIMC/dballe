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


}
}
}
#endif
