/*
 * memdb/serializer - Read/write memdb contents to disk
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

/**
 * CSVWriter implementation that writes its output file atomically.
 */
struct CSVOutfile : public dballe::CSVWriter
{
    std::string pathname;
    std::string pathname_tmp;
    FILE* fd;

    CSVOutfile();
    CSVOutfile(const std::string& pathname);
    ~CSVOutfile();

    void open(const std::string& pathname);

    /**
     * Commit the write, closing the output temporary file and renaming it to
     * its final name
     */
    void commit();

    /**
     * Roll back the write, deleting the output temporary file and leaving the
     * original untouched
     */
    void rollback();

    virtual void flush_row();

private:
    CSVOutfile(const CSVOutfile&);
    CSVOutfile& operator=(const CSVOutfile&);
};

/**
 * Serializer for Memdb contents
 */
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

/**
 * CSVReader implementation that reads using stdio and has helper functions to
 * deserialize Memdb
 */
class CSVInfile : public dballe::CSVReader
{
public:
    std::string pathname;

public:
    CSVInfile();
    CSVInfile(const std::string& pathname);

    /// Same as CSVReader::open, but it considers missing files as empty files
    void open(const std::string& pathname);

    template<typename INFILE>
    void read_attrs(const INFILE& values);

private:
    CSVInfile(const CSVInfile&);
    CSVInfile& operator=(const CSVInfile&);
};

/**
 * Common implementation bits of ValueStorage deserializers.
 *
 * It supports mapping line numbers to the IDs of deserialized values.
 */
template<typename VALUES>
struct CSVValueStorageInfile : public CSVInfile
{
    typedef typename VALUES::value_type value_type;

    VALUES& values;
    // Map 0-based line numbers to IDs in values
    std::map<size_t, size_t> id_map;

    CSVValueStorageInfile(VALUES& values) : values(values) {}
    const typename VALUES::value_type& by_lineno(size_t lineno) const;
};

struct CSVStationsInfile : public CSVValueStorageInfile<memdb::Stations>
{
    CSVStationsInfile(memdb::Stations& stations);

    void read();
};

struct CSVStationValuesInfile : public CSVValueStorageInfile<memdb::StationValues>
{
    CSVStationValuesInfile(memdb::StationValues& stationvalues);

    void read(const CSVStationsInfile& stations);
};

struct CSVValuesInfile : public CSVValueStorageInfile<memdb::Values>
{
    Memdb& memdb;
    CSVValuesInfile(Memdb& memdb);

    void read(const CSVStationsInfile& stations);
};

/**
 * Deserializer for Memdb
 */
struct CSVReader
{
    Memdb& memdb;

    CSVStationsInfile in_station;
    CSVStationValuesInfile in_stationvalue;
    CSVInfile in_stationvalue_attr;
    CSVValuesInfile in_value;
    CSVInfile in_value_attr;

    CSVReader(const std::string& dir, Memdb& memdb);

    void read();
};

}
}
}

#endif


