#ifndef DBA_MEMDB_STATION_H
#define DBA_MEMDB_STATION_H

#include <dballe/memdb/valuestorage.h>
#include <dballe/memdb/index.h>
#include <dballe/core/defs.h>
#include <string>
#include <cstddef>

namespace dballe {
struct Record;
struct Msg;
struct Station;

namespace core {
struct Query;
}

namespace msg {
struct Context;
}

namespace memdb {
template<typename T> struct Results;

/// Station information
struct Station
{
    size_t id;
    Coords coords;
    bool mobile;
    std::string ident;
    std::string report;

    // Fixed station
    Station(size_t id, const Coords& coords, const std::string& report)
        : id(id), coords(coords), mobile(false), report(report) {}
    Station(size_t id, double lat, double lon, const std::string& report)
        : id(id), coords(lat, lon), mobile(false), report(report) {}

    // Mobile station
    Station(size_t id, const Coords& coords, const std::string& ident, const std::string& report)
        : id(id), coords(coords), mobile(true), ident(ident), report(report) {}
    Station(size_t id, double lat, double lon, const std::string& ident, const std::string& report)
        : id(id), coords(lat, lon), mobile(true), ident(ident), report(report) {}

    /**
     * Fill lat, lon, report information, message type (from report) and identifier in msg.
     *
     * Return the station level in msg, so further changes to msg will not need
     * to look it up again.
     */
    msg::Context& fill_msg(Msg& msg) const;

    bool operator<(const Station& o) const { return id < o.id; }
    bool operator>(const Station& o) const { return id > o.id; }
    bool operator==(const Station& o) const { return id == o.id; }
    bool operator!=(const Station& o) const { return id != o.id; }
};

/// Storage and index for station information
class Stations : public ValueStorage<Station>
{
protected:
    Index<int> by_lat;
    Index<std::string> by_ident;

public:
    Stations();

    void clear();

    /// Get a fixed Station record
    size_t obtain_fixed(const Coords& coords, const std::string& report, bool create=true);

    /// Get a mobile Station record
    size_t obtain_mobile(const Coords& coords, const std::string& ident, const std::string& report, bool create=true);

    /// Get a fixed or mobile Station record depending on the data in rec
    size_t obtain(const Record& rec, bool create=true);

    /// Get a fixed or mobile Station record depending on the data in rec
    size_t obtain(const dballe::Station& st, bool create=true);

    /// Query stations returning the IDs
    void query(const core::Query& q, Results<Station>& res) const;

    void dump(FILE* out) const;
};

}
}

#endif

