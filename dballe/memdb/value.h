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

/// Simple datetime structure
struct Datetime
{
    unsigned short year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;

    Datetime(unsigned short year, unsigned char month=1, unsigned char day=1,
             unsigned char hour=0, unsigned char minute=0, unsigned char second=0)
        : year(year), month(month), day(day), hour(hour), minute(minute), second(second)
    {
    }

    bool operator==(const Datetime& dt) const
    {
        return year == dt.year && month == dt.month && day == dt.day
            && hour == dt.hour && minute == dt.minute && second == dt.second;
    }

    bool operator!=(const Datetime& dt) const
    {
        return year != dt.year || month != dt.month || day != dt.day
            || hour != dt.hour || minute != dt.minute || second != dt.second;
    }
};

std::ostream& operator<<(std::ostream& out, const Datetime& dt);

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

public:
    /// Get a fixed Station record
    const Value& insert_or_replace(const Station& station, const LevTr& levtr, const Datetime& datetime, std::auto_ptr<wreport::Var> var);

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
