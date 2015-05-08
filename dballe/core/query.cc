/*
 * dballe/core/query - Represent a filter for DB-All.e data
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "query.h"
#include "var.h"
#include "json.h"
#include <sstream>
#include <cmath>
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {

unsigned Query::get_modifiers() const
{
    return parse_modifiers(query.c_str());
}

void Query::clear()
{
    ana_id = MISSING_INT;
    prio_min = MISSING_INT;
    prio_max = MISSING_INT;
    rep_memo.clear();
    mobile = MISSING_INT;
    has_ident = false;
    ident.clear();
    coords_min = Coords();
    coords_max = Coords();
    datetime_min = Datetime();
    datetime_max = Datetime();
    level = Level();
    trange = Trange();
    varcodes.clear();
    query.clear();
    ana_filter.clear();
    data_filter.clear();
    attr_filter.clear();
    limit = MISSING_INT;
    block = MISSING_INT;
    station = MISSING_INT;
    data_id = MISSING_INT;
    query_station_vars = false;
}

void Query::seti(dba_keyword key, int val)
{
    switch (key)
    {
        case DBA_KEY_PRIORITY: prio_min = prio_max = val; break;
        case DBA_KEY_PRIOMAX: prio_max = val; break;
        case DBA_KEY_PRIOMIN: prio_min = val; break;
        case DBA_KEY_ANA_ID: ana_id = val; break;
        case DBA_KEY_MOBILE: mobile = val; break;
        case DBA_KEY_LAT: coords_min.set_lat(val); coords_max.set_lat(val); break;
        case DBA_KEY_LON: coords_min.set_lon(val); coords_max.set_lon(val); break;
        case DBA_KEY_LATMAX: coords_max.set_lat(val); break;
        case DBA_KEY_LATMIN: coords_min.set_lat(val); break;
        case DBA_KEY_LONMAX: coords_max.set_lon(val); break;
        case DBA_KEY_LONMIN: coords_min.set_lon(val); break;
        case DBA_KEY_YEAR: datetime_min.date.year = datetime_max.date.year = val; break;
        case DBA_KEY_MONTH: datetime_min.date.month = datetime_max.date.month = val; break;
        case DBA_KEY_DAY: datetime_min.date.day = datetime_max.date.day = val; break;
        case DBA_KEY_HOUR: datetime_min.time.hour = datetime_max.time.hour = val; break;
        case DBA_KEY_MIN: datetime_min.time.minute = datetime_max.time.minute = val; break;
        case DBA_KEY_SEC: datetime_min.time.second = datetime_max.time.second = val; break;
        case DBA_KEY_YEARMAX: datetime_max.date.year = val; break;
        case DBA_KEY_YEARMIN: datetime_min.date.year = val; break;
        case DBA_KEY_MONTHMAX: datetime_max.date.month = val; break;
        case DBA_KEY_MONTHMIN: datetime_min.date.month = val; break;
        case DBA_KEY_DAYMAX: datetime_max.date.day = val; break;
        case DBA_KEY_DAYMIN: datetime_min.date.day = val; break;
        case DBA_KEY_HOURMAX: datetime_max.time.hour = val; break;
        case DBA_KEY_HOURMIN: datetime_min.time.hour = val; break;
        case DBA_KEY_MINUMAX: datetime_max.time.minute = val; break;
        case DBA_KEY_MINUMIN: datetime_min.time.minute = val; break;
        case DBA_KEY_SECMAX: datetime_max.time.second = val; break;
        case DBA_KEY_SECMIN: datetime_min.time.second = val; break;
        case DBA_KEY_LEVELTYPE1: level.ltype1 = val; break;
        case DBA_KEY_L1: level.l1 = val; break;
        case DBA_KEY_LEVELTYPE2: level.ltype2 = val; break;
        case DBA_KEY_L2: level.l2 = val; break;
        case DBA_KEY_PINDICATOR: trange.pind = val; break;
        case DBA_KEY_P1: trange.p1 = val; break;
        case DBA_KEY_P2: trange.p2 = val; break;
        case DBA_KEY_VAR:
        case DBA_KEY_VARLIST:
             varcodes.clear();
             varcodes.insert((Varcode)val);
             break;
        case DBA_KEY_CONTEXT_ID: data_id = val; break;
        case DBA_KEY_LIMIT: limit = val; break;
        case DBA_KEY_ERROR:
        case DBA_KEY_REP_MEMO:
        case DBA_KEY_IDENT:
        case DBA_KEY_QUERY:
        case DBA_KEY_ANA_FILTER:
        case DBA_KEY_DATA_FILTER:
        case DBA_KEY_ATTR_FILTER:
        case DBA_KEY_VAR_RELATED:
        case DBA_KEY_COUNT:
            error_consistency::throwf("cannot set key %s to integer value %d", Record::keyword_name(key), val);
    }
}

void Query::setd(dba_keyword key, double val)
{
    switch (key)
    {
        case DBA_KEY_PRIORITY: prio_min = prio_max = lround(val); break;
        case DBA_KEY_PRIOMAX: prio_max = lround(val); break;
        case DBA_KEY_PRIOMIN: prio_min = lround(val); break;
        case DBA_KEY_ANA_ID: ana_id = lround(val); break;
        case DBA_KEY_MOBILE: mobile = lround(val); break;
        case DBA_KEY_LAT: coords_min.set_lat(val); coords_max.set_lat(val); break;
        case DBA_KEY_LON: coords_min.set_lon(val); coords_max.set_lon(val); break;
        case DBA_KEY_LATMAX: coords_max.set_lat(val); break;
        case DBA_KEY_LATMIN: coords_min.set_lat(val); break;
        case DBA_KEY_LONMAX: coords_max.set_lon(val); break;
        case DBA_KEY_LONMIN: coords_min.set_lon(val); break;
        case DBA_KEY_YEAR: datetime_min.date.year = datetime_max.date.year = lround(val); break;
        case DBA_KEY_MONTH: datetime_min.date.month = datetime_max.date.month = lround(val); break;
        case DBA_KEY_DAY: datetime_min.date.day = datetime_max.date.day = lround(val); break;
        case DBA_KEY_HOUR: datetime_min.time.hour = datetime_max.time.hour = lround(val); break;
        case DBA_KEY_MIN: datetime_min.time.minute = datetime_max.time.minute = lround(val); break;
        case DBA_KEY_SEC: datetime_min.time.second = datetime_max.time.second = lround(val); break;
        case DBA_KEY_YEARMAX: datetime_max.date.year = lround(val); break;
        case DBA_KEY_YEARMIN: datetime_min.date.year = lround(val); break;
        case DBA_KEY_MONTHMAX: datetime_max.date.month = lround(val); break;
        case DBA_KEY_MONTHMIN: datetime_min.date.month = lround(val); break;
        case DBA_KEY_DAYMAX: datetime_max.date.day = lround(val); break;
        case DBA_KEY_DAYMIN: datetime_min.date.day = lround(val); break;
        case DBA_KEY_HOURMAX: datetime_max.time.hour = lround(val); break;
        case DBA_KEY_HOURMIN: datetime_min.time.hour = lround(val); break;
        case DBA_KEY_MINUMAX: datetime_max.time.minute = lround(val); break;
        case DBA_KEY_MINUMIN: datetime_min.time.minute = lround(val); break;
        case DBA_KEY_SECMAX: datetime_max.time.second = lround(val); break;
        case DBA_KEY_SECMIN: datetime_min.time.second = lround(val); break;
        case DBA_KEY_LEVELTYPE1: level.ltype1 = lround(val); break;
        case DBA_KEY_L1: level.l1 = lround(val); break;
        case DBA_KEY_LEVELTYPE2: level.ltype2 = lround(val); break;
        case DBA_KEY_L2: level.l2 = lround(val); break;
        case DBA_KEY_PINDICATOR: trange.pind = lround(val); break;
        case DBA_KEY_P1: trange.p1 = lround(val); break;
        case DBA_KEY_P2: trange.p2 = lround(val); break;
        case DBA_KEY_CONTEXT_ID: data_id = lround(val); break;
        case DBA_KEY_LIMIT: limit = lround(val); break;
        case DBA_KEY_ERROR:
        case DBA_KEY_REP_MEMO:
        case DBA_KEY_IDENT:
        case DBA_KEY_VAR:
        case DBA_KEY_VARLIST:
        case DBA_KEY_QUERY:
        case DBA_KEY_ANA_FILTER:
        case DBA_KEY_DATA_FILTER:
        case DBA_KEY_ATTR_FILTER:
        case DBA_KEY_VAR_RELATED:
        case DBA_KEY_COUNT:
            error_consistency::throwf("cannot set key %s to floating point value %f", Record::keyword_name(key), val);
    }
}

void Query::setc(dba_keyword key, const char* val)
{
    switch (key)
    {
        case DBA_KEY_PRIORITY: prio_min = prio_max = strtol(val, 0, 10); break;
        case DBA_KEY_PRIOMAX: prio_max = strtol(val, 0, 10); break;
        case DBA_KEY_PRIOMIN: prio_min = strtol(val, 0, 10); break;
        case DBA_KEY_REP_MEMO: rep_memo = val; break;
        case DBA_KEY_ANA_ID: ana_id = strtol(val, 0, 10); break;
        case DBA_KEY_MOBILE: mobile = strtol(val, 0, 10); break;
        case DBA_KEY_IDENT: has_ident = true; ident = val; break;
        case DBA_KEY_LAT: { int lat = strtol(val, 0, 10); coords_min.set_lat(lat); coords_max.set_lat(lat); } break;
        case DBA_KEY_LON: { int lon = strtol(val, 0, 10); coords_min.set_lon(lon); coords_max.set_lon(lon); } break;
        case DBA_KEY_LATMAX: coords_max.set_lat((int)strtol(val, 0, 10)); break;
        case DBA_KEY_LATMIN: coords_min.set_lat((int)strtol(val, 0, 10)); break;
        case DBA_KEY_LONMAX: coords_max.set_lon((int)strtol(val, 0, 10)); break;
        case DBA_KEY_LONMIN: coords_min.set_lon((int)strtol(val, 0, 10)); break;
        case DBA_KEY_YEAR: datetime_min.date.year = datetime_max.date.year = strtol(val, 0, 10); break;
        case DBA_KEY_MONTH: datetime_min.date.month = datetime_max.date.month = strtol(val, 0, 10); break;
        case DBA_KEY_DAY: datetime_min.date.day = datetime_max.date.day = strtol(val, 0, 10); break;
        case DBA_KEY_HOUR: datetime_min.time.hour = datetime_max.time.hour = strtol(val, 0, 10); break;
        case DBA_KEY_MIN: datetime_min.time.minute = datetime_max.time.minute = strtol(val, 0, 10); break;
        case DBA_KEY_SEC: datetime_min.time.second = datetime_max.time.second = strtol(val, 0, 10); break;
        case DBA_KEY_YEARMAX: datetime_max.date.year = strtol(val, 0, 10); break;
        case DBA_KEY_YEARMIN: datetime_min.date.year = strtol(val, 0, 10); break;
        case DBA_KEY_MONTHMAX: datetime_max.date.month = strtol(val, 0, 10); break;
        case DBA_KEY_MONTHMIN: datetime_min.date.month = strtol(val, 0, 10); break;
        case DBA_KEY_DAYMAX: datetime_max.date.day = strtol(val, 0, 10); break;
        case DBA_KEY_DAYMIN: datetime_min.date.day = strtol(val, 0, 10); break;
        case DBA_KEY_HOURMAX: datetime_max.time.hour = strtol(val, 0, 10); break;
        case DBA_KEY_HOURMIN: datetime_min.time.hour = strtol(val, 0, 10); break;
        case DBA_KEY_MINUMAX: datetime_max.time.minute = strtol(val, 0, 10); break;
        case DBA_KEY_MINUMIN: datetime_min.time.minute = strtol(val, 0, 10); break;
        case DBA_KEY_SECMAX: datetime_max.time.second = strtol(val, 0, 10); break;
        case DBA_KEY_SECMIN: datetime_min.time.second = strtol(val, 0, 10); break;
        case DBA_KEY_LEVELTYPE1: level.ltype1 = strtol(val, 0, 10); break;
        case DBA_KEY_L1: level.l1 = strtol(val, 0, 10); break;
        case DBA_KEY_LEVELTYPE2: level.ltype2 = strtol(val, 0, 10); break;
        case DBA_KEY_L2: level.l2 = strtol(val, 0, 10); break;
        case DBA_KEY_PINDICATOR: trange.pind = strtol(val, 0, 10); break;
        case DBA_KEY_P1: trange.p1 = strtol(val, 0, 10); break;
        case DBA_KEY_P2: trange.p2 = strtol(val, 0, 10); break;
        case DBA_KEY_VAR:
            varcodes.clear();
            varcodes.insert(resolve_varcode_safe(val));
            break;
        case DBA_KEY_VARLIST:
            varcodes.clear();
            resolve_varlist_safe(val, varcodes);
            break;
        case DBA_KEY_CONTEXT_ID: data_id = strtol(val, 0, 10); break;
        case DBA_KEY_QUERY: query = val; break;
        case DBA_KEY_ANA_FILTER: ana_filter = val; break;
        case DBA_KEY_DATA_FILTER: data_filter = val; break;
        case DBA_KEY_ATTR_FILTER: attr_filter = val; break;
        case DBA_KEY_LIMIT: limit = strtol(val, 0, 10); break;
        case DBA_KEY_ERROR:
        case DBA_KEY_VAR_RELATED:
        case DBA_KEY_COUNT:
            error_consistency::throwf("cannot set key %s to string value %s", Record::keyword_name(key), val);
    }
}

void Query::setc(dba_keyword key, const std::string& val)
{
    switch (key)
    {
        case DBA_KEY_PRIORITY: prio_min = prio_max = stoi(val); break;
        case DBA_KEY_PRIOMAX: prio_max = stoi(val); break;
        case DBA_KEY_PRIOMIN: prio_min = stoi(val); break;
        case DBA_KEY_REP_MEMO: rep_memo = val; break;
        case DBA_KEY_ANA_ID: ana_id = stoi(val); break;
        case DBA_KEY_MOBILE: mobile = stoi(val); break;
        case DBA_KEY_IDENT: has_ident = true; ident = val; break;
        case DBA_KEY_LAT: { int lat = stoi(val); coords_min.set_lat(lat); coords_max.set_lat(lat); }; break;
        case DBA_KEY_LON: { int lon = stoi(val); coords_min.set_lon(lon); coords_max.set_lon(lon); }; break;
        case DBA_KEY_LATMAX: coords_max.set_lat(stoi(val)); break;
        case DBA_KEY_LATMIN: coords_min.set_lat(stoi(val)); break;
        case DBA_KEY_LONMAX: coords_max.set_lon(stoi(val)); break;
        case DBA_KEY_LONMIN: coords_min.set_lon(stoi(val)); break;
        case DBA_KEY_YEAR: datetime_min.date.year = datetime_max.date.year = stoi(val); break;
        case DBA_KEY_MONTH: datetime_min.date.month = datetime_max.date.month = stoi(val); break;
        case DBA_KEY_DAY: datetime_min.date.day = datetime_max.date.day = stoi(val); break;
        case DBA_KEY_HOUR: datetime_min.time.hour = datetime_max.time.hour = stoi(val); break;
        case DBA_KEY_MIN: datetime_min.time.minute = datetime_max.time.minute = stoi(val); break;
        case DBA_KEY_SEC: datetime_min.time.second = datetime_max.time.second = stoi(val); break;
        case DBA_KEY_YEARMAX: datetime_max.date.year = stoi(val); break;
        case DBA_KEY_YEARMIN: datetime_min.date.year = stoi(val); break;
        case DBA_KEY_MONTHMAX: datetime_max.date.month = stoi(val); break;
        case DBA_KEY_MONTHMIN: datetime_min.date.month = stoi(val); break;
        case DBA_KEY_DAYMAX: datetime_max.date.day = stoi(val); break;
        case DBA_KEY_DAYMIN: datetime_min.date.day = stoi(val); break;
        case DBA_KEY_HOURMAX: datetime_max.time.hour = stoi(val); break;
        case DBA_KEY_HOURMIN: datetime_min.time.hour = stoi(val); break;
        case DBA_KEY_MINUMAX: datetime_max.time.minute = stoi(val); break;
        case DBA_KEY_MINUMIN: datetime_min.time.minute = stoi(val); break;
        case DBA_KEY_SECMAX: datetime_max.time.second = stoi(val); break;
        case DBA_KEY_SECMIN: datetime_min.time.second = stoi(val); break;
        case DBA_KEY_LEVELTYPE1: level.ltype1 = stoi(val); break;
        case DBA_KEY_L1: level.l1 = stoi(val); break;
        case DBA_KEY_LEVELTYPE2: level.ltype2 = stoi(val); break;
        case DBA_KEY_L2: level.l2 = stoi(val); break;
        case DBA_KEY_PINDICATOR: trange.pind = stoi(val); break;
        case DBA_KEY_P1: trange.p1 = stoi(val); break;
        case DBA_KEY_P2: trange.p2 = stoi(val); break;
        case DBA_KEY_VAR:
            varcodes.clear();
            varcodes.insert(resolve_varcode_safe(val));
            break;
        case DBA_KEY_VARLIST:
            varcodes.clear();
            resolve_varlist_safe(val, varcodes);
            break;
        case DBA_KEY_CONTEXT_ID: data_id = stoi(val); break;
        case DBA_KEY_QUERY: query = val; break;
        case DBA_KEY_ANA_FILTER: ana_filter = val; break;
        case DBA_KEY_DATA_FILTER: data_filter = val; break;
        case DBA_KEY_ATTR_FILTER: attr_filter = val; break;
        case DBA_KEY_LIMIT: limit = stoi(val); break;
        case DBA_KEY_ERROR:
        case DBA_KEY_VAR_RELATED:
        case DBA_KEY_COUNT:
            error_consistency::throwf("cannot set key %s to string value %s", Record::keyword_name(key), val.c_str());
    }
}

void Query::unset(dba_keyword key)
{
    switch (key)
    {
        case DBA_KEY_PRIORITY: prio_min = prio_max = MISSING_INT; break;
        case DBA_KEY_PRIOMAX: prio_max = MISSING_INT; break;
        case DBA_KEY_PRIOMIN: prio_min = MISSING_INT; break;
        case DBA_KEY_REP_MEMO: rep_memo.clear(); break;
        case DBA_KEY_ANA_ID: ana_id = MISSING_INT; break;
        case DBA_KEY_MOBILE: mobile = MISSING_INT; break;
        case DBA_KEY_IDENT: has_ident = false; ident.clear(); break;
        case DBA_KEY_LAT: coords_min.set_lat(MISSING_INT); coords_max.set_lat(MISSING_INT); break;
        case DBA_KEY_LON: coords_min.set_lon(MISSING_INT); coords_max.set_lon(MISSING_INT); break;
        case DBA_KEY_LATMAX: coords_max.set_lat(MISSING_INT); break;
        case DBA_KEY_LATMIN: coords_min.set_lat(MISSING_INT); break;
        case DBA_KEY_LONMAX: coords_max.set_lon(MISSING_INT); break;
        case DBA_KEY_LONMIN: coords_min.set_lon(MISSING_INT); break;
        case DBA_KEY_YEAR: datetime_min.date.year = datetime_max.date.year = 0xffff; break;
        case DBA_KEY_MONTH: datetime_min.date.month = datetime_max.date.month = 0xff; break;
        case DBA_KEY_DAY: datetime_min.date.day = datetime_max.date.day = 0xff; break;
        case DBA_KEY_HOUR: datetime_min.time.hour = datetime_max.time.hour = 0xff; break;
        case DBA_KEY_MIN: datetime_min.time.minute = datetime_max.time.minute = 0xff; break;
        case DBA_KEY_SEC: datetime_min.time.second = datetime_max.time.second = 0xff; break;
        case DBA_KEY_YEARMAX: datetime_max.date.year = 0xffff; break;
        case DBA_KEY_YEARMIN: datetime_min.date.year = 0xffff; break;
        case DBA_KEY_MONTHMAX: datetime_max.date.month = 0xff; break;
        case DBA_KEY_MONTHMIN: datetime_min.date.month = 0xff; break;
        case DBA_KEY_DAYMAX: datetime_max.date.day = 0xff; break;
        case DBA_KEY_DAYMIN: datetime_min.date.day = 0xff; break;
        case DBA_KEY_HOURMAX: datetime_max.time.hour = 0xff; break;
        case DBA_KEY_HOURMIN: datetime_min.time.hour = 0xff; break;
        case DBA_KEY_MINUMAX: datetime_max.time.minute = 0xff; break;
        case DBA_KEY_MINUMIN: datetime_min.time.minute = 0xff; break;
        case DBA_KEY_SECMAX: datetime_max.time.second = 0xff; break;
        case DBA_KEY_SECMIN: datetime_min.time.second = 0xff; break;
        case DBA_KEY_LEVELTYPE1: level.ltype1 = MISSING_INT; break;
        case DBA_KEY_L1: level.l1 = MISSING_INT; break;
        case DBA_KEY_LEVELTYPE2: level.ltype2 = MISSING_INT; break;
        case DBA_KEY_L2: level.l2 = MISSING_INT; break;
        case DBA_KEY_PINDICATOR: trange.pind = MISSING_INT; break;
        case DBA_KEY_P1: trange.p1 = MISSING_INT; break;
        case DBA_KEY_P2: trange.p2 = MISSING_INT; break;
        case DBA_KEY_VAR: varcodes.clear(); break;
        case DBA_KEY_VARLIST: varcodes.clear(); break;
        case DBA_KEY_CONTEXT_ID: data_id = MISSING_INT; break;
        case DBA_KEY_QUERY: query.clear(); break;
        case DBA_KEY_ANA_FILTER: ana_filter.clear(); break;
        case DBA_KEY_DATA_FILTER: data_filter.clear(); break;
        case DBA_KEY_ATTR_FILTER: attr_filter.clear(); break;
        case DBA_KEY_LIMIT: limit = MISSING_INT; break;
        case DBA_KEY_ERROR:
        case DBA_KEY_VAR_RELATED:
        case DBA_KEY_COUNT:
            error_consistency::throwf("cannot unset key %s", Record::keyword_name(key));
    }
}

void Query::set_from_record(const Record& rec)
{
    // Set keys
    rec.iter_keys([&](dba_keyword key, const wreport::Var& var) {
        if (var.value())
            setc(key, var.value());
        return true;
    });

    // Set block and station, if present
    for (const auto& v : rec.vars())
        switch (v->code())
        {
            case WR_VAR(0, 1, 1): if (v->isset()) block = v->enqi(); break;
            case WR_VAR(0, 1, 2): if (v->isset()) station = v->enqi(); break;
        }

    query_station_vars = rec.is_ana_context();
}

void Query::set_from_string(const char* str)
{
    // Split the input as name=val
    const char* s = strchr(str, '=');

    if (!s) error_consistency::throwf("there should be an = between the name and the value in '%s'", str);

    string key(str, s - str);
    set_from_string(key.c_str(), s + 1);
}

void Query::set_from_string(const char* name, const char* val)
{
    dba_keyword key = Record::keyword_byname(name);

    if (key != DBA_KEY_ERROR)
        set_from_formatted(key, val);
    else
    {
        Varcode code = resolve_varcode_safe(name);
        switch (code)
        {
            case WR_VAR(0, 1, 1): block = strtol(val, 0, 10); break;
            case WR_VAR(0, 1, 2): station = strtol(val, 0, 10); break;
            default: error_consistency::throwf("cannot set query field %s to %s", name, val);
        }
    }
}

void Query::set_from_formatted(dba_keyword key, const char* val)
{
    // NULL or empty string, unset()
    if (val == NULL || val[0] == 0)
    {
        unset(key);
        return;
    }

    // Get the Varinfo for this key
    Varinfo i = Record::keyword_info(key);

    // If we're a string, it's easy
    if (i->is_string())
    {
        setc(key, val);
        return;
    }

    // Else use strtod
    setd(key, strtod(val, NULL));
}

void Query::set_from_test_string(const std::string& s)
{
    if (s.empty()) return;
    size_t cur = 0;
    while (true)
    {
        size_t next = s.find(", ", cur);
        if (next == string::npos)
        {
            set_from_string(s.substr(cur).c_str());
            break;
        } else {
            set_from_string(s.substr(cur, next - cur).c_str());
            cur = next + 2;
        }
    }
}

namespace {

bool removed_or_changed(int val, int other)
{
    return other != val && other != MISSING_INT;
}

bool removed_or_changed(const std::string& val, const std::string& other)
{
    return other != val && !other.empty();
}

// Return true if sub is a subset of sup, or the same as sup
template<typename T>
bool is_subset(const std::set<T>& sub, const std::set<T>& sup)
{
    auto isub = sub.begin();
    auto isup = sup.begin();
    while (isub != sub.end() && isup != sup.end())
    {
        // A common item: ok. Skip it and continue.
        if (*isub == *isup)
        {
            ++isub;
            ++isup;
            continue;
        }
        // sub contains an item not in sup: not a subset.
        if (*isub < *isup)
            return false;
        // sup contains an item not in sub: ok. Skip it and continue.
        ++isup;
    }
    // If we have seen all of sub so far, then it is a subset.
    return isub == sub.end();
}

// Return true if sub1--sub2 is contained in sup1--sup2, or is the same
bool is_subrange(int sub1, int sub2, int sup1, int sup2)
{
    // If sup is the whole domain, sub is contained in it
    if (sup1 == MISSING_INT && sup2 == MISSING_INT) return true;
    // If sup is left-open, only check the right bounds
    if (sup1 == MISSING_INT) return sub2 != MISSING_INT && sub2 <= sup2;
    // If sup is right-open, only check the left bounds
    if (sup2 == MISSING_INT) return sub1 != MISSING_INT && sub1 >= sup1;
    // sup is bounded both ways
    if (sub1 == MISSING_INT || sub1 < sup1) return false;
    if (sub2 == MISSING_INT || sub2 > sup2) return false;
    return true;
}

// Return true if sub1--sub2 is contained in sup1--sup2, or is the same
bool is_subrange(const Datetime& sub1, const Datetime& sub2, const Datetime& sup1, const Datetime& sup2)
{
    // If sup is the whole domain, sub is contained in it
    if (sup1.is_missing() && sup2.is_missing()) return true;
    // If sup is left-open, only check the right bounds
    if (sup1.is_missing()) return !sub2.is_missing() && !(sub2 > sup2);
    // If sup is right-open, only check the left bounds
    if (sup2.is_missing()) return !sub1.is_missing() && !(sub1 < sup1);
    // sup is bounded both ways
    if (sub1.is_missing() || sub1 < sup1) return false;
    if (sub2.is_missing() || sub2 > sup2) return false;
    return true;
}

// Return true if sub1--sub2 is contained in sup1--sup2, or is the same
bool is_subrange(const Coords& sub1, const Coords& sub2, const Coords& sup1, const Coords& sup2)
{
    // Check latitude first
    if (!is_subrange(sub1.lat, sub2.lat, sup1.lat, sup2.lat)) return false;

    // Both longitude extremes must be either missing or defined
    if (sup1.lon == MISSING_INT && sup2.lon == MISSING_INT) return true;
    if (sub1.lon == MISSING_INT && sub2.lon == MISSING_INT) return false;
    if (sup1.lon == MISSING_INT || sup2.lon == MISSING_INT
     || sub1.lon == MISSING_INT || sub2.lon == MISSING_INT)
        throw error_consistency("queries cannot contain an open ended longitude range");

    // Longitude ranges can match outside or inside the interval
    if (sup1.lon < sup2.lon)
    {
        // sup matches inside the interval
        if (sub1.lon < sub2.lon)
        {
            // sub matches inside the interval
            return sub1.lon >= sup1.lon && sub2.lon <= sup2.lon;
        } else {
            // sub matches outside the interval
            return false;
        }
    } else {
        // sup matches outside the interval
        if (sub1.lon < sub2.lon)
        {
            // sub matches inside the interval
            return sub2.lon <= sup1.lon || sub1.lon >= sup2.lon;
        } else {
            // sub matches outside the interval
            return sub1.lon <= sup1.lon || sub2.lon >= sup2.lon;
        }
    }
}

}

bool Query::is_subquery(const Query& other) const
{
    if (removed_or_changed(ana_id, other.ana_id)) return false;
    if (!is_subrange(prio_min, prio_max, other.prio_min, other.prio_max)) return false;
    if (removed_or_changed(rep_memo, other.rep_memo)) return false;
    if (removed_or_changed(mobile, other.mobile)) return false;
    if (other.has_ident && !has_ident) return false;
    if (other.has_ident && has_ident && other.ident != ident) return false;
    if (!is_subrange(coords_min, coords_max, other.coords_min, other.coords_max)) return false;
    if (!is_subrange(datetime_min.lower_bound(), datetime_max.upper_bound(),
                other.datetime_min.lower_bound(), other.datetime_max.upper_bound())) return false;
    if (removed_or_changed(level.ltype1, other.level.ltype1)) return false;
    if (removed_or_changed(level.l1, other.level.l1)) return false;
    if (removed_or_changed(level.ltype2, other.level.ltype2)) return false;
    if (removed_or_changed(level.l2, other.level.l2)) return false;
    if (removed_or_changed(trange.pind, other.trange.pind)) return false;
    if (removed_or_changed(trange.p1, other.trange.p1)) return false;
    if (removed_or_changed(trange.p2, other.trange.p2)) return false;
    // If other.varcodes is a subset, of varcodes, then we just added new
    // varcodes to the filter
    if (!is_subset(other.varcodes, varcodes)) return false;
    // Parse query and check its components
    unsigned mods = parse_modifiers(query.c_str());
    unsigned omods = parse_modifiers(other.query.c_str());
    if (mods != omods)
    {
        // The only relevant bits is query=best, all the rest we can safely ignore
        if (mods & DBA_DB_MODIFIER_BEST != omods & DBA_DB_MODIFIER_BEST) return false;
    }
    if (removed_or_changed(ana_filter, other.ana_filter)) return false;
    if (removed_or_changed(data_filter, other.data_filter)) return false;
    if (removed_or_changed(attr_filter, other.attr_filter)) return false;
    // We tolerate limit being changed as long as it has only been reduced
    if (other.limit != MISSING_INT && (limit == MISSING_INT || limit > other.limit)) return false;
    if (removed_or_changed(block, other.block)) return false;
    if (removed_or_changed(station, other.station)) return false;
    if (removed_or_changed(data_id, other.data_id)) return false;
    // This switches the query domain entirely, so if it changed, we are not a subquery
    if (query_station_vars != other.query_station_vars) return false;
    return true;
}

namespace {

struct Printer
{
    const Query& q;
    FILE* out;
    bool first = true;

    Printer(const Query& q, FILE* out) : q(q), out(out) {}

    void print_int(const char* name, int val)
    {
        if (val == MISSING_INT) return;
        if (!first) fputs(", ", out);
        fprintf(out, "%s=%d", name, val);
        first = false;
    }

    void print_str(const char* name, bool is_present, const std::string& val)
    {
        if (!is_present) return;
        if (!first) fputs(", ", out);
        fprintf(out, "%s=%s", name, val.c_str());
        first = false;
    }

    void print_coords(const char* suffix, const Coords& coords)
    {
        if (coords.lat != MISSING_INT)
        {
            if (!first) fputs(", ", out);
            fprintf(out, "lat%s=%.5f", suffix, coords.dlat());
            first = false;
        }

        if (coords.lon != MISSING_INT)
        {
            if (!first) fputs(", ", out);
            fprintf(out, "lon%s=%.5f", suffix, coords.dlon());
            first = false;
        }
    }

    void print_datetime(const char* name, const Datetime& dt)
    {
        if (dt.is_missing()) return;
        if (!first) fputs(", ", out);
        fprintf(out, "%s=%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
                name,
                dt.date.year, dt.date.month, dt.date.day,
                dt.time.hour, dt.time.minute, dt.time.second);
        first = false;
    }

    void print_level(const char* name, const Level& l)
    {
        if (l == Level()) return;
        if (!first) fputs(", ", out);
        stringstream buf;
        buf << l;
        fprintf(out, "%s=%s", name, buf.str().c_str());
        first = false;
    }

    void print_trange(const char* name, const Trange& t)
    {
        if (t == Trange()) return;
        if (!first) fputs(", ", out);
        stringstream buf;
        buf << t;
        fprintf(out, "%s=%s", name, buf.str().c_str());
        first = false;
    }

    void print_varcodes(const char* name, const set<Varcode>& varcodes)
    {
        if (varcodes.empty()) return;
        if (!first) fputs(", ", out);
        fputs(name, out);
        putc('=', out);
        bool lfirst = true;
        for (const auto& v : varcodes)
        {
            if (!lfirst) fputs(",", out);
            fprintf(out, "%01d%02d%03d", WR_VAR_F(v), WR_VAR_X(v), WR_VAR_Y(v));
            lfirst = false;
        }
        first = false;
    }

    void print()
    {
        print_int("ana_id", q.ana_id);
        print_int("prio_min", q.prio_min);
        print_int("prio_max", q.prio_max);
        print_str("rep_memo", !q.rep_memo.empty(), q.rep_memo);
        print_int("mobile", q.mobile);
        print_str("ident", q.has_ident, q.ident);
        print_coords("min", q.coords_min);
        print_coords("max", q.coords_max);
        print_datetime("datetime_min", q.datetime_min);
        print_datetime("datetime_max", q.datetime_max);
        print_level("level", q.level);
        print_trange("trange", q.trange);
        print_varcodes("varcodes", q.varcodes);
        print_str("query", !q.query.empty(), q.query);
        print_str("ana_filter", !q.ana_filter.empty(), q.ana_filter);
        print_str("data_filter", !q.data_filter.empty(), q.data_filter);
        print_str("attr_filter", !q.attr_filter.empty(), q.attr_filter);
        print_int("limit", q.limit);
        print_int("block", q.block);
        print_int("station", q.station);
        print_int("data_id", q.data_id);
        print_int("query_station_vars", q.query_station_vars ? 1 : MISSING_INT);
        putc('\n', out);
    }
};

}

void Query::print(FILE* out) const
{
    Printer printer(*this, out);
    printer.print();
}

void Query::serialize(JSONWriter& out) const
{
    if (ana_id != MISSING_INT) out.add("ana_id", ana_id);
    if (prio_min != MISSING_INT) out.add("prio_min", prio_min);
    if (prio_max != MISSING_INT) out.add("prio_max", prio_max);
    if (!rep_memo.empty()) out.add("rep_memo", rep_memo);
    if (mobile != MISSING_INT) out.add("mobile", mobile);
    if (has_ident) out.add("ident", ident);
    if (coords_min == coords_max)
    {
        if (!coords_min.is_missing()) out.add("coords", coords_min);
    } else {
        if (!coords_min.is_missing()) out.add("coords_min", coords_min);
        if (!coords_max.is_missing()) out.add("coords_max", coords_min);
    }
    if (coords_min.lat != MISSING_INT) out.add("latmin", coords_min.lat);
    if (coords_min.lon != MISSING_INT) out.add("lonmin", coords_min.lon);
    if (coords_max.lat != MISSING_INT) out.add("latmax", coords_max.lat);
    if (coords_max.lon != MISSING_INT) out.add("lonmax", coords_max.lon);
    if (datetime_min == datetime_max)
    {
        if (!datetime_min.is_missing()) out.add("datetime", datetime_min);
    } else {
        if (!datetime_min.is_missing()) out.add("datetime_min", datetime_min);
        if (!datetime_max.is_missing()) out.add("datetime_max", datetime_min);
    }
    if (!level.is_missing()) out.add("level", level);
    if (!trange.is_missing()) out.add("trange", trange);
    if (!varcodes.empty())
    {
        out.add("varcodes");
        out.add_list(varcodes);
    }
    if (!query.empty()) out.add("query", query);
    if (!ana_filter.empty()) out.add("ana_filter", ana_filter);
    if (!data_filter.empty()) out.add("data_filter", data_filter);
    if (!attr_filter.empty()) out.add("attr_filter", attr_filter);
    if (limit != MISSING_INT) out.add("limit", limit);
    if (block != MISSING_INT) out.add("block", block);
    if (station != MISSING_INT) out.add("station", station);
    if (data_id != MISSING_INT) out.add("data_id", data_id);
    out.add("query_station_vars", query_station_vars);
}

unsigned Query::parse_modifiers(const Record& rec)
{
    /* Decode query modifiers */
    const char* val = rec.key_peek_value(DBA_KEY_QUERY);
    if (!val) return 0;
    return parse_modifiers(val);
}

unsigned Query::parse_modifiers(const char* s)
{
    unsigned modifiers = 0;
    while (*s)
    {
        size_t len = strcspn(s, ",");
        int got = 1;
        switch (len)
        {
            case 0:
                /* If it's an empty token, skip it */
                break;
            case 4:
                /* "best": if more values exist in a point, get only the
                   best one */
                if (strncmp(s, "best", 4) == 0)
                    modifiers |= DBA_DB_MODIFIER_BEST;
                else
                    got = 0;
                break;
            case 6:
                /* "bigana": optimize with date first */
                if (strncmp(s, "bigana", 6) == 0)
                    ; // Not used anymore
                else if (strncmp(s, "nosort", 6) == 0)
                    modifiers |= DBA_DB_MODIFIER_UNSORTED;
                else if (strncmp(s, "stream", 6) == 0)
                    ; // Not used anymore
                else
                    got = 0;
                break;
            case 7:
                if (strncmp(s, "details", 7) == 0)
                    modifiers |= DBA_DB_MODIFIER_SUMMARY_DETAILS;
                else
                    got = 0;
                break;
            default:
                got = 0;
                break;
        }

        /* Check that we parsed it correctly */
        if (!got)
            error_consistency::throwf("Query modifier \"%.*s\" is not recognized", (int)len, s);

        /* Move to the next token */
        s += len;
        if (*s == ',')
            ++s;
    }

    return modifiers;
}


}
