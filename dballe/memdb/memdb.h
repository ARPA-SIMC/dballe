/*
 * memdb/memdb - In-memory indexed storage of DB-All.e data
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

#ifndef DBA_MEMDB_MEMDB_H
#define DBA_MEMDB_MEMDB_H

#include <dballe/memdb/station.h>
#include <dballe/memdb/stationvalue.h>
#include <dballe/memdb/levtr.h>
#include <dballe/memdb/value.h>

namespace dballe {
struct Record;
struct Msg;

namespace memdb {
template<typename T> struct Results;

/**
 * Wraps a Value providing a std::map key that considers all values the same as
 * long as they have the same station, level, timerange and varcode.
 */
struct SummaryContext
{
    const Value& sample;

    SummaryContext(const Value& val) : sample(val) {}

    bool operator<(const SummaryContext& c) const;
};

/// Statistics about all 'Value's with the same SummaryContext
struct SummaryStats
{
    size_t count;
    Datetime dtmin;
    Datetime dtmax;

    SummaryStats(const Datetime& dt) : count(1), dtmin(dt), dtmax(dt) {}

    void extend(const Datetime& dt);
};

/// A summary of a set of 'Value's
typedef std::map<memdb::SummaryContext, memdb::SummaryStats> Summary;

/// Build a summary one Value at a time
struct Summarizer
{
    Summary& summary;

    Summarizer(Summary& summary) : summary(summary) {}

    void insert(const Value* val);
};

}

/// In-memory database backend
struct Memdb
{
    memdb::Stations stations;
    memdb::StationValues stationvalues;
    memdb::LevTrs levtrs;
    memdb::Values values;

    Memdb();

    void clear();
    void insert_or_replace(const Record& rec);
    void insert(const Msg& msg, bool replace=true, bool with_station_info=true, bool with_attrs=true, const char* force_report=NULL);
    size_t insert(
            const Coords& coords, const std::string& ident, const std::string& report,
            const Level& level, const Trange& trange, const Datetime& datetime,
            std::unique_ptr<wreport::Var> var);
    size_t insert(
            const Coords& coords, const std::string& ident, const std::string& report,
            const Level& level, const Trange& trange, const Datetime& datetime,
            const wreport::Var& var);

    void remove(memdb::Results<memdb::StationValue>& query);
    void remove(memdb::Results<memdb::Value>& query);

    void dump(FILE* out) const;

private:
    Memdb(const Memdb&);
    Memdb& operator=(const Memdb&);
};

}

#endif

