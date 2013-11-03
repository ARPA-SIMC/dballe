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
    void insert(const Msg& msg, bool overwrite=true, bool with_station_info=true, bool with_attrs=true, const char* force_report=NULL);

    /// Query stations, returning a list of station IDs
    void query_stations(const Record& rec, memdb::Results<memdb::Station>& res) const;
    /// Query data, returning a list of Value IDs
    void query_data(const Record& rec, memdb::Results<memdb::Value>& res) const;

    void dump(FILE* out) const;

private:
    Memdb(const Memdb&);
    Memdb& operator=(const Memdb&);
};

}

#endif

