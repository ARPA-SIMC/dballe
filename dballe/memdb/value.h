/*
 * memdb/value - In memory representation of a variable with metadata and
 *               attributes
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

#ifndef DBA_MEMDB_VALUE_H
#define DBA_MEMDB_VALUE_H

#include <dballe/memdb/core.h>
#include <dballe/core/defs.h>
#include <wreport/var.h>
#include <memory>
#include <iosfwd>

namespace dballe {
namespace memdb {

struct Station;
struct LevTr;

/// Station information
struct Value
{
    const Station& station;
    const LevTr& levtr;
    Datetime datetime;
    wreport::Var* var;

    Value(const Station& station, const LevTr& levtr, const Datetime& datetime, std::auto_ptr<wreport::Var> var)
        : station(station), levtr(levtr), datetime(datetime), var(var.release()) {}
    ~Value();

    /// Replace the variable with the given one
    void replace(std::auto_ptr<wreport::Var> var);

private:
    Value(const Value&);
    Value& operator=(const Value&);
};

/// Storage and index for station information
class Values : public ValueStorage<Value>
{
protected:
    Index<const Station*> by_station;
    Index<const LevTr*> by_levtr;
    Index<Date> by_date;

public:
    void clear();

    /// Insert a new value, or replace an existing one
    size_t insert(const Station& station, const LevTr& levtr, const Datetime& datetime, std::auto_ptr<wreport::Var> var, bool replace=true);

    /// Insert a new value, or replace an existing one
    size_t insert(const Station& station, const LevTr& levtr, const Datetime& datetime, const wreport::Var& var, bool replace=true);

    /**
     * Remove a value.
     *
     * Returns true if found and removed, false if it was not found.
     */
    bool remove(const Station& station, const LevTr& levtr, const Datetime& datetime, wreport::Varcode code);
};

}
}

#endif
