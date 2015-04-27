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
#include <cmath>

using namespace wreport;
using namespace std;

namespace dballe {

void Query::seti(dba_keyword key, int val)
{
    switch (key)
    {
        case DBA_KEY_PRIORITY: prio_exact = val; break;
        case DBA_KEY_PRIOMAX: prio_max = val; break;
        case DBA_KEY_PRIOMIN: prio_min = val; break;
        case DBA_KEY_ANA_ID: ana_id = val; break;
        case DBA_KEY_MOBILE: mobile = val; break;
        case DBA_KEY_LAT: coords_exact.set_lat(val); break;
        case DBA_KEY_LON: coords_exact.set_lon(val); break;
        case DBA_KEY_LATMAX: coords_max.set_lat(val); break;
        case DBA_KEY_LATMIN: coords_min.set_lat(val); break;
        case DBA_KEY_LONMAX: coords_max.set_lon(val); break;
        case DBA_KEY_LONMIN: coords_min.set_lon(val); break;
        case DBA_KEY_YEAR: datetime_exact.date.year = val; break;
        case DBA_KEY_MONTH: datetime_exact.date.month = val; break;
        case DBA_KEY_DAY: datetime_exact.date.day = val; break;
        case DBA_KEY_HOUR: datetime_exact.time.hour = val; break;
        case DBA_KEY_MIN: datetime_exact.time.minute = val; break;
        case DBA_KEY_SEC: datetime_exact.time.second = val; break;
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
        case DBA_KEY_CONTEXT_ID:
             throw error_unimplemented("setting context_id in query is not implemented");
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
        case DBA_KEY_PRIORITY: prio_exact = lround(val); break;
        case DBA_KEY_PRIOMAX: prio_max = lround(val); break;
        case DBA_KEY_PRIOMIN: prio_min = lround(val); break;
        case DBA_KEY_ANA_ID: ana_id = lround(val); break;
        case DBA_KEY_MOBILE: mobile = lround(val); break;
        case DBA_KEY_LAT: coords_exact.set_lat(val); break;
        case DBA_KEY_LON: coords_exact.set_lon(val); break;
        case DBA_KEY_LATMAX: coords_max.set_lat(val); break;
        case DBA_KEY_LATMIN: coords_min.set_lat(val); break;
        case DBA_KEY_LONMAX: coords_max.set_lon(val); break;
        case DBA_KEY_LONMIN: coords_min.set_lon(val); break;
        case DBA_KEY_YEAR: datetime_exact.date.year = lround(val); break;
        case DBA_KEY_MONTH: datetime_exact.date.month = lround(val); break;
        case DBA_KEY_DAY: datetime_exact.date.day = lround(val); break;
        case DBA_KEY_HOUR: datetime_exact.time.hour = lround(val); break;
        case DBA_KEY_MIN: datetime_exact.time.minute = lround(val); break;
        case DBA_KEY_SEC: datetime_exact.time.second = lround(val); break;
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
        case DBA_KEY_CONTEXT_ID:
             throw error_unimplemented("setting context_id in query is not implemented");
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
        case DBA_KEY_PRIORITY: prio_exact = strtol(val, 0, 10); break;
        case DBA_KEY_PRIOMAX: prio_max = strtol(val, 0, 10); break;
        case DBA_KEY_PRIOMIN: prio_min = strtol(val, 0, 10); break;
        case DBA_KEY_REP_MEMO: rep_memo = val; break;
        case DBA_KEY_ANA_ID: ana_id = strtol(val, 0, 10); break;
        case DBA_KEY_MOBILE: mobile = strtol(val, 0, 10); break;
        case DBA_KEY_IDENT: ident = val; break;
        case DBA_KEY_LAT: coords_exact.set_lat((int)strtol(val, 0, 10)); break;
        case DBA_KEY_LON: coords_exact.set_lon((int)strtol(val, 0, 10)); break;
        case DBA_KEY_LATMAX: coords_max.set_lat((int)strtol(val, 0, 10)); break;
        case DBA_KEY_LATMIN: coords_min.set_lat((int)strtol(val, 0, 10)); break;
        case DBA_KEY_LONMAX: coords_max.set_lon((int)strtol(val, 0, 10)); break;
        case DBA_KEY_LONMIN: coords_min.set_lon((int)strtol(val, 0, 10)); break;
        case DBA_KEY_YEAR: datetime_exact.date.year = strtol(val, 0, 10); break;
        case DBA_KEY_MONTH: datetime_exact.date.month = strtol(val, 0, 10); break;
        case DBA_KEY_DAY: datetime_exact.date.day = strtol(val, 0, 10); break;
        case DBA_KEY_HOUR: datetime_exact.time.hour = strtol(val, 0, 10); break;
        case DBA_KEY_MIN: datetime_exact.time.minute = strtol(val, 0, 10); break;
        case DBA_KEY_SEC: datetime_exact.time.second = strtol(val, 0, 10); break;
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
        case DBA_KEY_CONTEXT_ID:
            throw error_unimplemented("setting context_id in query is not implemented");
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
        case DBA_KEY_PRIORITY: prio_exact = stoi(val); break;
        case DBA_KEY_PRIOMAX: prio_max = stoi(val); break;
        case DBA_KEY_PRIOMIN: prio_min = stoi(val); break;
        case DBA_KEY_REP_MEMO: rep_memo = val; break;
        case DBA_KEY_ANA_ID: ana_id = stoi(val); break;
        case DBA_KEY_MOBILE: mobile = stoi(val); break;
        case DBA_KEY_IDENT: ident = val; break;
        case DBA_KEY_LAT: coords_exact.set_lat(stoi(val)); break;
        case DBA_KEY_LON: coords_exact.set_lon(stoi(val)); break;
        case DBA_KEY_LATMAX: coords_max.set_lat(stoi(val)); break;
        case DBA_KEY_LATMIN: coords_min.set_lat(stoi(val)); break;
        case DBA_KEY_LONMAX: coords_max.set_lon(stoi(val)); break;
        case DBA_KEY_LONMIN: coords_min.set_lon(stoi(val)); break;
        case DBA_KEY_YEAR: datetime_exact.date.year = stoi(val); break;
        case DBA_KEY_MONTH: datetime_exact.date.month = stoi(val); break;
        case DBA_KEY_DAY: datetime_exact.date.day = stoi(val); break;
        case DBA_KEY_HOUR: datetime_exact.time.hour = stoi(val); break;
        case DBA_KEY_MIN: datetime_exact.time.minute = stoi(val); break;
        case DBA_KEY_SEC: datetime_exact.time.second = stoi(val); break;
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
        case DBA_KEY_CONTEXT_ID:
            throw error_unimplemented("setting context_id in query is not implemented");
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
        case DBA_KEY_PRIORITY: prio_exact = MISSING_INT; break;
        case DBA_KEY_PRIOMAX: prio_max = MISSING_INT; break;
        case DBA_KEY_PRIOMIN: prio_min = MISSING_INT; break;
        case DBA_KEY_REP_MEMO: rep_memo.clear(); break;
        case DBA_KEY_ANA_ID: ana_id = MISSING_INT; break;
        case DBA_KEY_MOBILE: mobile = MISSING_INT; break;
        case DBA_KEY_IDENT: ident.clear(); break;
        case DBA_KEY_LAT: coords_exact.set_lat(MISSING_INT); break;
        case DBA_KEY_LON: coords_exact.set_lon(MISSING_INT); break;
        case DBA_KEY_LATMAX: coords_max.set_lat(MISSING_INT); break;
        case DBA_KEY_LATMIN: coords_min.set_lat(MISSING_INT); break;
        case DBA_KEY_LONMAX: coords_max.set_lon(MISSING_INT); break;
        case DBA_KEY_LONMIN: coords_min.set_lon(MISSING_INT); break;
        case DBA_KEY_YEAR: datetime_exact.date.year = 0xffff; break;
        case DBA_KEY_MONTH: datetime_exact.date.month = 0xff; break;
        case DBA_KEY_DAY: datetime_exact.date.day = 0xff; break;
        case DBA_KEY_HOUR: datetime_exact.time.hour = 0xff; break;
        case DBA_KEY_MIN: datetime_exact.time.minute = 0xff; break;
        case DBA_KEY_SEC: datetime_exact.time.second = 0xff; break;
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
        case DBA_KEY_CONTEXT_ID:
            throw error_unimplemented("unsetting context_id in query is not implemented");
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

}
