/*
 * memdb/station - In memory representation of stations
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

#include "station.h"
#include <dballe/core/record.h>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace wreport;

namespace dballe {
namespace memdb {

Stations::Stations() : ValueStorage<Station>() {}

void Stations::clear()
{
    by_coord.clear();
    by_ident.clear();
    ValueStorage<Station>::clear();
}

size_t Stations::obtain_fixed(const Coord& coords, const std::string& report, bool create)
{
    // Search
    Positions res = by_coord.search(coords);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
    {
        const Station* s = (*this)[*i];
        if (s && !s->mobile && s->report == report)
            return *i;
    }

    if (!create)
        error_notfound::throwf("%s station not found at %f,%f", report.c_str(), coords.dlat(), coords.dlon());

    // Station not found, create it
    size_t pos = value_add(new Station(coords, report));
    // Index it
    by_coord[coords].insert(pos);
    // And return it
    return pos;
}

size_t Stations::obtain_mobile(const Coord& coords, const std::string& ident, const std::string& report, bool create)
{
    // Search
    Positions res = by_coord.search(coords);
    by_ident.refine(ident, res);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
    {
        const Station* s = (*this)[*i];
        if (s && s->mobile && s->report == report)
            return *i;
    }

    if (!create)
        error_notfound::throwf("%s station %s not found at %f,%f", report.c_str(), ident.c_str(), coords.dlat(), coords.dlon());

    // Station not found, create it
    size_t pos = value_add(new Station(coords, ident, report));
    // Index it
    by_coord[coords].insert(pos);
    by_ident[ident].insert(pos);
    // And return it
    return pos;
}

size_t Stations::obtain(const Record& rec, bool create)
{
    int s_lat;
    if (const Var* var = rec.key_peek(DBA_KEY_LAT))
        s_lat = var->enqi();
    else
        throw error_notfound("record with no latitude, looking up a memdb Station");

    int s_lon;
    if (const Var* var = rec.key_peek(DBA_KEY_LON))
        s_lon = var->enqi();
    else
        throw error_notfound("record with no longitude, looking up a memdb Station");

    const char* s_ident = rec.key_peek_value(DBA_KEY_IDENT);

    const char* s_report;
    if (const char* memo = rec.key_peek_value(DBA_KEY_REP_MEMO))
        s_report = memo;
    else
        throw error_notfound("record with no rep_memo, looking up a memdb Station");

    if (s_ident)
        return obtain_mobile(Coord(s_lat, s_lon), s_ident, s_report, create);
    else
        return obtain_fixed(Coord(s_lat, s_lon), s_report, create);
}

#if 0
namespace {

struct CoordsIter
{
};

}
#endif

void Stations::query(const Record& rec, Results& res) const
{
    if (const char* ana_id = rec.key_peek_value(DBA_KEY_ANA_ID))
    {
        size_t pos = strtoul(ana_id, 0, 10);
        if (pos >= 0 && pos < values.size() && values[pos])
            res.intersect(pos);
    }
#if 0
    void add_lat()
    {
        add_int(DBA_KEY_LAT, "%s.lat=%d");
        int latmin = rec.get(DBA_KEY_LATMIN, -9000000);
        if (latmin > -9000000)
        {
            q.append_listf("%s.lat>=%d", tbl, latmin);
            found = true;
        }
        int latmax = rec.get(DBA_KEY_LATMAX, 9000000);
        if (latmax < 9000000)
        {
            q.append_listf("%s.lat<=%d", tbl, latmax);
            found = true;
        }
    }

    void add_lon()
    {
        //add_int(rec, cur->sel_lonmin, DBA_KEY_LON, "pa.lon=?", DBA_DB_FROM_PA);
        if (const char* val = rec.key_peek_value(DBA_KEY_LON))
        {
            q.append_listf("%s.lon=%d", tbl, normalon(strtol(val, 0, 10)));
            found = true;
        }
        if (rec.key_peek_value(DBA_KEY_LONMIN) && rec.key_peek_value(DBA_KEY_LONMAX))
        {
            int lonmin = rec.key(DBA_KEY_LONMIN).enqi();
            int lonmax = rec.key(DBA_KEY_LONMAX).enqi();
            if (lonmin == lonmax)
            {
                q.append_listf("%s.lon=%d", tbl, normalon(lonmin));
                found = true;
            } else {
                lonmin = normalon(lonmin);
                lonmax = normalon(lonmax);
                if (lonmin < lonmax)
                {
                    q.append_listf("%s.lon>=%d AND %s.lon<=%d", tbl, lonmin, tbl, lonmax);
                    found = true;
                } else if (lonmin > lonmax) {
                    q.append_listf("((%s.lon>=%d AND %s.lon<=18000000) OR (%s.lon>=-18000000 AND %s.lon<=%d))",
                            tbl, lonmin, tbl, tbl, tbl, lonmax);
                    found = true;
                }
                // If after being normalised min and max are the same, we
                // assume that one wants "any longitude", as is the case with
                // lonmin=0 lonmax=360 or lonmin=-180 lonmax=180
            }
        } else if (rec.key_peek_value(DBA_KEY_LONMIN) != NULL) {
            throw error_consistency("'lonmin' query parameter was specified without 'lonmax'");
        } else if (rec.key_peek_value(DBA_KEY_LONMAX) != NULL) {
            throw error_consistency("'lonmax' query parameter was specified without 'lonmin'");
        }
    }

    void add_mobile()
    {
        if (const char* val = rec.key_peek_value(DBA_KEY_MOBILE))
        {
            if (val[0] == '0')
            {
                q.append_listf("%s.ident IS NULL", tbl);
                TRACE("found fixed/mobile: adding AND %s.ident IS NULL.\n", tbl);
            } else {
                q.append_listf("NOT (%s.ident IS NULL)", tbl);
                TRACE("found fixed/mobile: adding AND NOT (%s.ident IS NULL)\n", tbl);
            }
            found = true;
        }
    }
    if (const char* val = rec.key_peek_value(DBA_KEY_IDENT))
    {
        sql_where.append_listf("%s.ident=?", tbl);
        TRACE("found ident: adding AND %s.ident = ?.  val is %s\n", tbl, val);
        stm.bind_in(qargs.input_seq++, val);
        c.found = true;
    }
#endif
}

}
}

#include "core.tcc"
