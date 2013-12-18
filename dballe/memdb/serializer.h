/*
 * memdb/serializer - Read/write memdb contents to disk
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

#ifndef DBA_MEMDB_SERIALIZER_H
#define DBA_MEMDB_SERIALIZER_H

#include <dballe/memdb/station.h>
#include <dballe/memdb/stationvalue.h>
#include <dballe/memdb/levtr.h>
#include <dballe/memdb/value.h>
#include <dballe/core/csv.h>
#include <map>
#include <cstdio>

namespace dballe {
struct Memdb;

namespace memdb {
namespace serialize {

struct CSVOutfile : public CSVWriter
{
    std::string pathname;
    std::string pathname_tmp;
    FILE* fd;

    CSVOutfile();
    CSVOutfile(const std::string& pathname);
    ~CSVOutfile();

    void open(const std::string& pathname);

    void commit();
    void rollback();

    virtual void flush_row();

private:
    CSVOutfile(const CSVOutfile&);
    CSVOutfile& operator=(const CSVOutfile&);
};

struct CSVWriter
{
    CSVOutfile out_station;
    CSVOutfile out_stationvalue;
    CSVOutfile out_stationvalue_attr;
    CSVOutfile out_value;
    CSVOutfile out_value_attr;

    CSVWriter(const std::string& dir);

    void write(const Memdb& memdb);

    void commit();
};

class CSVInfile : public dballe::CSVReader
{
public:
    std::string pathname;

protected:
    FILE* fd;

    virtual bool nextline();

public:
    CSVInfile();
    CSVInfile(const std::string& pathname);
    virtual ~CSVInfile();

    void open(const std::string& pathname);

private:
    CSVInfile(const CSVInfile&);
    CSVInfile& operator=(const CSVInfile&);
};

struct CSVStationsInfile : public CSVInfile
{
    std::map<size_t, size_t> station_id_map;

    const Station& station_by_id(const memdb::Stations& stations, size_t id) const;
    void read_stations(memdb::Stations& stations);
};

struct CSVReader
{
    CSVStationsInfile in_station;
    CSVInfile in_stationvalue;
    CSVInfile in_stationvalue_attr;
    CSVInfile in_value;
    CSVInfile in_value_attr;

    CSVReader(const std::string& dir);

    void read(Memdb& memdb);
};

}
}
}

#endif


