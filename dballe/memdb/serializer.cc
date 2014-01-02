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
#include "serializer.h"
#include "memdb.h"
#include "dballe/core/var.h"
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;
using namespace wreport;
using namespace dballe::memdb;

namespace dballe {
namespace memdb {
namespace serialize {

CSVOutfile::CSVOutfile() : fd(0) {}

CSVOutfile::CSVOutfile(const std::string& pathname)
    : fd(0)
{
    open(pathname);
}

CSVOutfile::~CSVOutfile()
{
    rollback();
}

void CSVOutfile::open(const std::string& pathname)
{
    if (fd) rollback();

    this->pathname = pathname;

    // Create a template for the temporary file name
    char buffer[pathname.size() + 8];
    memcpy(buffer, pathname.data(), pathname.size());
    strcpy(buffer + pathname.size(), ".XXXXXX");

    // Create the temporary file
    int ifd = mkstemp(buffer);
    if (ifd == -1)
        error_system::throwf("cannot open temporary file %s", buffer);

    // Store the temp file name
    pathname_tmp = buffer;

    // Wrap with stdio
    fd = fdopen(ifd, "w");
    if (fd == NULL)
        error_system::throwf("cannot fdopen file %s", pathname_tmp.c_str());
}

void CSVOutfile::commit()
{
    if (!fd) return;
    fclose(fd);
    fd = 0;
    if (rename(pathname_tmp.c_str(), pathname.c_str()) == -1)
        error_system::throwf("cannot rename %s to %s", pathname_tmp.c_str(), pathname.c_str());
}

void CSVOutfile::rollback()
{
    if (!fd) return;
    fclose(fd);
    fd = 0;
    if (unlink(pathname_tmp.c_str()) == -1)
        error_system::throwf("cannot remove file %s", pathname_tmp.c_str());
}

void CSVOutfile::flush_row()
{
    if (fwrite(row.data(), row.size(), 1, fd) != 1)
        error_system::throwf("cannot write %zd bytes to file %s", row.size(), pathname_tmp.c_str());
    if (putc('\n', fd) == EOF)
        error_system::throwf("cannot write end of line character to file %s", pathname_tmp.c_str());
    row.clear();
}

CSVWriter::CSVWriter(const std::string& dir)
{
    // If it does not exist, make it
    if (::mkdir(dir.c_str(), 0777) == -1)
    {
        // throw on all errors except EEXIST. Note that EEXIST "includes the case
        // where pathname is a symbolic link, dangling or not."
        if (errno != EEXIST && errno != EISDIR)
            error_system::throwf("cannot create directory %s", dir.c_str());
    }

    out_station.open(dir + "/stations.csv");
    out_stationvalue.open(dir + "/stationvalues.csv");
    out_stationvalue_attr.open(dir + "/stationvalues-attrs.csv");
    out_value.open(dir + "/values.csv");
    out_value_attr.open(dir + "/values-attrs.csv");
}

void CSVWriter::commit()
{
    out_station.commit();
    out_stationvalue.commit();
    out_stationvalue_attr.commit();
    out_value.commit();
    out_value_attr.commit();
}

void CSVWriter::write(const Memdb& memdb)
{
    map<size_t, size_t> station_id_map;

    size_t new_id = 0;
    for (Stations::index_iterator i = memdb.stations.index_begin();
            i != memdb.stations.index_end(); ++i)
    {
        const Station& s = *memdb.stations[*i];
        station_id_map[s.id] = new_id;
        out_station.add_value(new_id);
        out_station.add_value(s.coords.lat);
        out_station.add_value(s.coords.lon);
        out_station.add_value(s.mobile ? 1 : 0);
        out_station.add_value(s.ident);
        out_station.add_value(s.report);
        out_station.flush_row();
        ++new_id;
    }

    new_id = 0;
    for (StationValues::index_iterator i = memdb.stationvalues.index_begin();
            i != memdb.stationvalues.index_end(); ++i)
    {
        const StationValue& v = *memdb.stationvalues[*i];
        out_stationvalue.add_value(new_id);
        out_stationvalue.add_value(station_id_map[v.station.id]);
        out_stationvalue.add_value(v.var->code());
        out_stationvalue.add_var_value(*v.var);
        out_stationvalue.flush_row();

        for (const Var* a = v.var->next_attr(); a != NULL; a = a->next_attr())
        {
            out_stationvalue_attr.add_value(new_id);
            out_stationvalue_attr.add_value(a->code());
            out_stationvalue_attr.add_var_value(*a);
            out_stationvalue_attr.flush_row();
        }

        ++new_id;
    }

    new_id = 0;
    for (Values::index_iterator i = memdb.values.index_begin();
            i != memdb.values.index_end(); ++i)
    {
        const Value& v = *memdb.values[*i];
        out_value.add_value(new_id);
        out_value.add_value(station_id_map[v.station.id]);
        out_value.add_value_withmissing(v.levtr.level.ltype1);
        out_value.add_value_withmissing(v.levtr.level.l1);
        out_value.add_value_withmissing(v.levtr.level.ltype2);
        out_value.add_value_withmissing(v.levtr.level.l2);
        out_value.add_value_withmissing(v.levtr.trange.pind);
        out_value.add_value_withmissing(v.levtr.trange.p1);
        out_value.add_value_withmissing(v.levtr.trange.p2);
        char buf[20];
        snprintf(buf, 20, "%04d-%02d-%02d %02d:%02d:%02d",
                v.datetime.date.year, v.datetime.date.month, v.datetime.date.day,
                v.datetime.time.hour, v.datetime.time.minute, v.datetime.time.second);
        out_value.add_value_raw(buf);
        out_value.add_value(v.var->code());
        out_value.add_var_value(*v.var);
        out_value.flush_row();

        for (const Var* a = v.var->next_attr(); a != NULL; a = a->next_attr())
        {
            out_value_attr.add_value(new_id);
            out_value_attr.add_value(a->code());
            out_value_attr.add_var_value(*a);
            out_value_attr.flush_row();
        }

        ++new_id;
    }
}

void CSVStationsInfile::read_stations(memdb::Stations& stations)
{
    size_t lineno = 0;
    while (next())
    {
        // 0:id, 1:lat, 2:lon, 3:mobile, 4:ident, 5:report
        ++lineno;
        if (cols.size() != 6)
            error_consistency::throwf("%s:%zd: expected 6 columns, got %zd",
                    pathname.c_str(), lineno, cols.size());
        size_t old_id = as_int(0);
        Coord coords(as_int(1), as_int(2));
        size_t new_id;
        if (cols[3] == "0")
            new_id = stations.obtain_fixed(coords, cols[5]);
        else
            new_id = stations.obtain_mobile(coords, cols[4], cols[5]);
        station_id_map[old_id] = new_id;
    }
}

const Station& CSVStationsInfile::station_by_id(const memdb::Stations& stations, size_t id) const
{
    // TODO: find a way to annotate the messages with file name and line
    // number. It likely requires the caller to catch and annotate the
    // exception

    map<size_t, size_t>::const_iterator i = station_id_map.find(id);
    if (i == station_id_map.end())
        error_consistency::throwf("station ID %zd not found in stations file", id);

    const Station* station = stations.get_checked(i->second);
    if (!station)
        error_consistency::throwf("station ID %zd does not map to a valid station", id);

    return *station;
}

CSVReader::CSVReader(const std::string& dir)
{
    in_station.open(dir + "/stations.csv");
    in_stationvalue.open(dir + "/stationvalues.csv");
    in_stationvalue_attr.open(dir + "/stationvalues-attrs.csv");
    in_value.open(dir + "/values.csv");
    in_value_attr.open(dir + "/values-attrs.csv");
}


void CSVReader::read(Memdb& memdb)
{
    in_station.read_stations(memdb.stations);

    // new_id = 0;
    size_t lineno = 0;
    while (in_stationvalue.next())
    {
        // 0: id, 1: stationid, 2: varcode, 3: value
        ++lineno;
        const std::vector<std::string>& cols = in_stationvalue.cols;
        if (cols.size() != 4)
            error_consistency::throwf("%s:%zd: expected 4 columns, got %zd",
                    in_stationvalue.pathname.c_str(), lineno, cols.size());

        size_t station_id = strtoul(cols[1].c_str(), 0, 10);
        const Station& station = in_station.station_by_id(memdb.stations, station_id);
        Varcode code = resolve_varcode_safe(cols[2]);
        std::auto_ptr<wreport::Var> var = newvar(code, cols[3].c_str());

        memdb.stationvalues.insert(station, var);
    }

    // new_id = 0;
    lineno = 0;
    while (in_value.next())
    {
        // 0:id, 1:stationid, 2:ltype1, 3:l1, 4:ltype2, 5:l2, 6:pind, 7:p1, 8:p2, 9:datetime 10:varcode, 11:value
        ++lineno;
        const std::vector<std::string>& cols = in_value.cols;
        if (cols.size() != 12)
            error_consistency::throwf("%s:%zd: expected 12 columns, got %zd",
                    in_value.pathname.c_str(), lineno, cols.size());

        size_t station_id = strtoul(cols[1].c_str(), 0, 10);
        const Station& station = in_station.station_by_id(memdb.stations, station_id);

        Level level(in_value.as_int_withmissing(2), in_value.as_int_withmissing(3),
                    in_value.as_int_withmissing(4), in_value.as_int_withmissing(5));
        Trange trange(in_value.as_int_withmissing(6), in_value.as_int_withmissing(7), in_value.as_int_withmissing(8));

        unsigned short year;
        unsigned char month, day, hour, minute, second;
        if (sscanf(cols[9].c_str(), "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
                &year, &month, &day, &hour, &minute, &second) != 6)
            error_consistency::throwf("%s:%zd: cannot parse datetime '%s'",
                    in_value.pathname.c_str(), lineno, cols[9].c_str());
        Datetime dt(year, month, day, hour, minute, second);

        size_t levtr_id = memdb.levtrs.obtain(level, trange);
        const LevTr& levtr = *memdb.levtrs[levtr_id];

        Varcode code = resolve_varcode_safe(cols[10]);
        std::auto_ptr<wreport::Var> var = newvar(code, cols[11].c_str());

        memdb.values.insert(station, levtr, dt, var);

        // ++new_id;
    }
}

CSVInfile::CSVInfile() : fd(0) {}

CSVInfile::CSVInfile(const std::string& pathname)
    : fd(0)
{
    open(pathname);
}

CSVInfile::~CSVInfile()
{
    if (fd) fclose(fd);
}

void CSVInfile::open(const std::string& pathname)
{
    if (fd)
    {
        fclose(fd);
        fd = 0;
    }

    this->pathname = pathname;

    fd = fopen(pathname.c_str(), "rt");
    if (fd == NULL)
    {
        if (errno == ENOENT)
            ; // If the file does not exist, treat it as an empty file
        else
            error_system::throwf("cannot open file %s", pathname.c_str());
    }
}

bool CSVInfile::nextline()
{
    if (fd == 0) return false;
    line.clear();
    while (true)
    {
        char c = getc(fd);
        switch (c)
        {
            case EOF:
                if (feof(fd))
                    return false;
                error_system::throwf("cannot read a character from %s", pathname.c_str());
                break;
            case '\n':
                // Break out of switch and while
                goto done;
            default:
                line += c;
                break;
        }
    }
done:
    return true;
}

}
}
}
