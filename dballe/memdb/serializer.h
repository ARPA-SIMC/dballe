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

}
}
}

#endif


