/*
 * memdb/value - In memory representation of a variable with metadata and
 *               attributes
 *
 * Copyright (C) 2013--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <dballe/memdb/valuestorage.h>
#include <dballe/memdb/index.h>
#include <dballe/memdb/valuebase.h>
#include <dballe/core/defs.h>
#include <wreport/var.h>
#include <memory>
#include <iosfwd>

namespace dballe {
struct Record;

namespace memdb {
template<typename T> struct Results;
struct Station;
struct LevTr;

/// Station information
struct Value : public ValueBase
{
    const Station& station;
    const LevTr& levtr;
    Datetime datetime;

    Value(const Station& station, const LevTr& levtr, const Datetime& datetime, std::unique_ptr<wreport::Var> var)
        : ValueBase(std::move(var)), station(station), levtr(levtr), datetime(datetime) {}
    ~Value();

    void dump(FILE* out) const;

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
    size_t insert(const Station& station, const LevTr& levtr, const Datetime& datetime, std::unique_ptr<wreport::Var> var, bool replace=true);

    /// Insert a new value, or replace an existing one
    size_t insert(const Station& station, const LevTr& levtr, const Datetime& datetime, const wreport::Var& var, bool replace=true);

    /**
     * Remove a value.
     *
     * Returns true if found and removed, false if it was not found.
     */
    bool remove(const Station& station, const LevTr& levtr, const Datetime& datetime, wreport::Varcode code);

    /// Removes a value, by index
    void erase(size_t idx);

    /// Query values returning the IDs
    void query(const Record& rec, Results<Station>& stations, Results<LevTr>& levtrs, Results<Value>& res) const;

    void dump(FILE* out) const;
};

}
}

#endif
