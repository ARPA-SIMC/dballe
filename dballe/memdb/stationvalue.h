#ifndef DBA_MEMDB_STATIONVALUE_H
#define DBA_MEMDB_STATIONVALUE_H

#include <dballe/memdb/valuestorage.h>
#include <dballe/memdb/index.h>
#include <dballe/memdb/valuebase.h>
#include <wreport/var.h>
#include <memory>

namespace dballe {
struct Record;

namespace core {
struct Query;
}

namespace msg {
struct Context;
}

namespace memdb {
template<typename T> struct Results;

struct Station;

/// Value describing one property of a station
struct StationValue : public ValueBase
{
    const Station& station;

    StationValue(const Station& station, std::unique_ptr<wreport::Var> var)
        : ValueBase(std::move(var)), station(station) {}
    ~StationValue();

private:
    StationValue(const StationValue&);
    StationValue& operator=(const StationValue&);
};

/// Storage and index for station values
class StationValues : public ValueStorage<StationValue>
{
protected:
    Index<const Station*> by_station;

public:
    void clear();

    bool has_variables_for(const Station& station) const
    {
        return by_station.find(&station) != by_station.end();
    }

    const StationValue* get(const Station& station, wreport::Varcode code) const;

    /// Insert a new value, or replace an existing one for the same station
    size_t insert(const Station& station, std::unique_ptr<wreport::Var> var, bool replace=true);

    /// Insert a new value, or replace an existing one for the same station
    size_t insert(const Station& station, const wreport::Var& var, bool replace=true);

    /**
     * Remove a value.
     *
     * Returns true if found and removed, false if it was not found.
     */
    bool remove(const Station& station, wreport::Varcode code);

    /// Fill a record with all the variables for this station
    void fill_record(const Station& station, Record& rec) const;

    /// Fill a message context with all the variables for this station
    void fill_msg(const Station& station, msg::Context& ctx) const;

    /// Query values for the given stations
    void query(const core::Query& q, Results<Station>& stations, Results<StationValue>& res) const;

    void dump(FILE* out) const;
};

}
}

#endif


