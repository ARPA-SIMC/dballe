/*
 * memdb/station - In memory representation of stations
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef DBA_MEMDB_STATION_H
#define DBA_MEMDB_STATION_H

#include <dballe/memdb/core.h>
#include <string>
#include <vector>

namespace dballe {
namespace memdb {

/// Station information
struct Station
{
    Coord coords;
    bool mobile;
    std::string ident;
    std::string report;

    // Fixed station
    Station(const Coord& coords, const std::string& report)
        : coords(coords), mobile(false), report(report) {}
    Station(double lat, double lon, const std::string& report)
        : coords(lat, lon), mobile(false), report(report) {}

    // Mobile station
    Station(const Coord& coords, const std::string& ident, const std::string& report)
        : coords(coords), mobile(true), ident(ident), report(report) {}
    Station(double lat, double lon, const std::string& ident, const std::string& report)
        : coords(lat, lon), mobile(true), ident(ident), report(report) {}
};

/// Storage and index for station information
class Stations : public ValueStorage<Station>
{
protected:
    Index<Coord> by_coord;
    Index<std::string> by_ident;

public:
    Stations();

    /// Get a fixed Station record
    const Station& obtain(double lat, double lon, const std::string& report);

    /// Get a mobile Station record
    const Station& obtain(double lat, double lon, const std::string& ident, const std::string& report);
};

}
}

#endif

