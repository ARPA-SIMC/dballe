/*
 * memdb/stationvalue - In memory representation of station values
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

#ifndef DBA_MEMDB_STATIONVALUE_H
#define DBA_MEMDB_STATIONVALUE_H

#include <dballe/memdb/core.h>
#include <wreport/var.h>
#include <memory>

namespace dballe {

namespace msg {
struct Context;
}

namespace memdb {

struct Station;

/// Station information
struct StationValue
{
    const Station& station;
    wreport::Var* var;

    StationValue(const Station& station, std::auto_ptr<wreport::Var> var)
        : station(station), var(var.release()) {}
    ~StationValue();

    /// Replace the variable with the given one
    void replace(std::auto_ptr<wreport::Var> var);

private:
    StationValue(const StationValue&);
    StationValue& operator=(const StationValue&);
};

/// Storage and index for station information
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
    size_t insert(const Station& station, std::auto_ptr<wreport::Var> var, bool replace=true);

    /// Insert a new value, or replace an existing one for the same station
    size_t insert(const Station& station, const wreport::Var& var, bool replace=true);

    /**
     * Remove a value.
     *
     * Returns true if found and removed, false if it was not found.
     */
    bool remove(const Station& station, wreport::Varcode code);

    /// Fill a message context with all the variables for this station
    void fill_msg(const Station& station, msg::Context& ctx) const;
};

}
}

#endif


