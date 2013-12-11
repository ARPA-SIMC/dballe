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
#include "results.h"
#include "dballe/core/record.h"
#include "dballe/core/stlutils.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include <algorithm>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace wreport;

namespace dballe {
namespace memdb {

msg::Context& Station::fill_msg(Msg& msg) const
{
    msg::Context& c_st = msg.obtain_station_context();

    // Fill in report information
    msg.type = Msg::type_from_repmemo(report.c_str());
    c_st.set_rep_memo(report.c_str());

    // Fill in the basic station values
    c_st.seti(WR_VAR(0, 5, 1), coords.lat);
    c_st.seti(WR_VAR(0, 6, 1), coords.lon);
    if (!ident.empty())
        c_st.set_ident(ident.c_str());

    return c_st;
}

Stations::Stations() : ValueStorage<Station>() {}

void Stations::clear()
{
    by_lat.clear();
    by_ident.clear();
    ValueStorage<Station>::clear();
}

size_t Stations::obtain_fixed(const Coord& coords, const std::string& report, bool create)
{
    // Search
    const set<size_t>* res = by_lat.search(coords.lat);
    if (res)
        for (set<size_t>::const_iterator i = res->begin(); i != res->end(); ++i)
        {
            const Station* s = (*this)[*i];
            if (s && s->coords == coords && !s->mobile && s->report == report)
                return *i;
        }

    if (!create)
        error_notfound::throwf("%s station not found at %f,%f", report.c_str(), coords.dlat(), coords.dlon());

    // Station not found, create it
    size_t pos = value_add(new Station(0, coords, report));
    values[pos]->id = pos;
    // Index it
    by_lat[coords.lat].insert(pos);
    // And return it
    return pos;
}

size_t Stations::obtain_mobile(const Coord& coords, const std::string& ident, const std::string& report, bool create)
{
    // Search
    stl::SetIntersection<size_t> res;
    if (by_lat.search(coords.lat, res) && by_ident.search(ident, res))
        for (stl::SetIntersection<size_t>::const_iterator i = res.begin(); i != res.end(); ++i)
        {
            const Station* s = (*this)[*i];
            if (s && s->coords == coords && s->mobile && s->report == report)
                return *i;
        }

    if (!create)
        error_notfound::throwf("%s station %s not found at %f,%f", report.c_str(), ident.c_str(), coords.dlat(), coords.dlon());

    // Station not found, create it
    size_t pos = value_add(new Station(0, coords, ident, report));
    values[pos]->id = pos;
    // Index it
    by_lat[coords.lat].insert(pos);
    by_ident[ident].insert(pos);
    // And return it
    return pos;
}

size_t Stations::obtain(const Record& rec, bool create)
{
    // Shortcut by ana_id
    if (const Var* var = rec.key_peek(DBA_KEY_ANA_ID))
    {
        size_t res = var->enqi();
        if (res > values.size() || !values[res])
            error_notfound::throwf("ana_id %zd is invalid", res);
        return res;
    }

    // Lookup by lat, lon and ident
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

namespace {

struct MatchMobile : public Match<Station>
{
    bool mobile;

    MatchMobile(bool mobile) : mobile(mobile) {}
    virtual bool operator()(const Station& val) const
    {
        return val.mobile == mobile;
    }
};

struct MatchLonExact : public Match<Station>
{
    int lon;

    MatchLonExact(int lon) : lon(Coord::normalon(lon)) {}

    virtual bool operator()(const Station& val) const
    {
        return val.coords.lon == lon;
    }
};

struct MatchLonInside : public Match<Station>
{
    int lonmin;
    int lonmax;

    MatchLonInside(int lonmin, int lonmax) : lonmin(lonmin), lonmax(lonmax) {}

    virtual bool operator()(const Station& val) const
    {
        return val.coords.lon >= lonmin && val.coords.lon <= lonmax;
    }
};

struct MatchLonOutside : public Match<Station>
{
    int lonmin;
    int lonmax;

    MatchLonOutside(int lonmin, int lonmax) : lonmin(lonmin), lonmax(lonmax) {}

    virtual bool operator()(const Station& val) const
    {
        return ((val.coords.lon >= lonmin and val.coords.lon <= 18000000)
             or (val.coords.lon >= -18000000 and val.coords.lon <= lonmax));
    }
};

struct MatchReport : public Match<Station>
{
    string report;

    MatchReport(const std::string& report) : report(report) {}

    virtual bool operator()(const Station& val) const
    {
        return val.report == report;
    }
};

auto_ptr< Match<Station> > build_lon_filter(const Record& rec)
{
    typedef auto_ptr< Match<Station> > RES;
    if (const char* val = rec.key_peek_value(DBA_KEY_LON))
    {
        trace_query("Found lon %s, using MatchLonExact\n", val);
        return RES(new MatchLonExact(strtoul(val, 0, 10)));
    }

    if (rec.key_peek_value(DBA_KEY_LONMIN) && rec.key_peek_value(DBA_KEY_LONMAX))
    {
        int lonmin = rec.key(DBA_KEY_LONMIN).enqi();
        int lonmax = rec.key(DBA_KEY_LONMAX).enqi();
        if (lonmin == lonmax)
        {
            trace_query("Found lonmin=lonmax=%d, using MatchLonExact\n", lonmin);
            return RES(new MatchLonExact(lonmin));
        } else {
            lonmin = Coord::normalon(lonmin);
            lonmax = Coord::normalon(lonmax);
            if (lonmin < lonmax)
            {
                trace_query("Found lonmin=%d, lonmax=%d, using MatchLonInside\n", lonmin, lonmax);
                return RES(new MatchLonInside(lonmin, lonmax));
            }
            else if (lonmin > lonmax)
            {
                trace_query("Found lonmin=%d, lonmax=%d, using MatchLonOutside\n", lonmin, lonmax);
                return RES(new MatchLonOutside(lonmin, lonmax));
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
    return RES(0);
}

}

void Stations::query(const Record& rec, Results<Station>& res) const
{
    if (const char* ana_id = rec.key_peek_value(DBA_KEY_ANA_ID))
    {
        trace_query("Found ana_id %s\n", ana_id);
        size_t pos = strtoul(ana_id, 0, 10);
        if (pos >= 0 && pos < values.size() && values[pos])
        {
            trace_query(" intersect with %zu\n", pos);
            res.add_singleton(pos);
        } else {
            trace_query(" set to empty result set\n");
            res.set_to_empty();
            return;
        }
    }

    auto_ptr< Match<Station> > lon_filter(build_lon_filter(rec));
    if (lon_filter.get())
        res.add(lon_filter.release());

    if (const char* val = rec.key_peek_value(DBA_KEY_MOBILE))
    {
        trace_query("Found mobile=%s, using MatchMobile\n", val);
        res.add(new MatchMobile(val[0] == '1'));
    }

    // Lookup latitude query parameters
    int latmin = MISSING_INT;
    int latmax = MISSING_INT;
    if (const char* lat = rec.key_peek_value(DBA_KEY_LAT))
    {
        latmin = latmax = strtoul(lat, 0, 10);
    } else {
        if (const char* v = rec.key_peek_value(DBA_KEY_LATMIN))
        {
            latmin = strtoul(v, 0, 10);
            if (latmin <= -9000000) latmin = MISSING_INT;
        }
        if (const char* v = rec.key_peek_value(DBA_KEY_LATMAX))
        {
            latmax = strtoul(v, 0, 10);
            if (latmin >= 9000000) latmin = MISSING_INT;
        }
    }

    // Lookup ident
    const char* ident = rec.key_peek_value(DBA_KEY_IDENT);

    if (latmin != MISSING_INT || latmax != MISSING_INT)
    {
        // In this cose, we can assume that if we match the whole index, then
        // we match every station.
        //
        // We can do this only because the index covers all the items: that is,
        // there are no stations without a latitude. We cannot do this, for
        // example, with the ident index.
        trace_query("Found latmin=%d, latmax=%d\n", latmin, latmax);

        if (latmin != MISSING_INT && latmax != MISSING_INT)
        {
            bool found;
            auto_ptr< stl::Sequences<size_t> > sequences(by_lat.search_between(latmin, latmax + 1, found));
            if (!found)
            {
                trace_query(" latmin-latmax range is outside the index: setting empty result set\n");
                res.set_to_empty();
                return;
            }
            if (sequences.get())
                res.add_union(sequences);
            else
                trace_query(" latmin-latmax range covers the whole index: no point in adding a filter\n");
        } else if (latmin != MISSING_INT) {
            bool found;
            auto_ptr< stl::Sequences<size_t> > sequences(by_lat.search_from(latmin, found));
            if (!found)
            {
                trace_query(" latmin is beyond end of index: setting empty result set\n");
                res.set_to_empty();
                return;
            }
            if (sequences.get())
                res.add_union(sequences);
            else
                trace_query(" latmin matches beginning of index: no point in adding a filter\n");
        } else if (latmax != MISSING_INT) {
            bool found;
            auto_ptr< stl::Sequences<size_t> > sequences(by_lat.search_to(latmax + 1, found));
            if (!found)
            {
                trace_query(" latmax is before start of index: setting empty result set\n");
                res.set_to_empty();
                return;
            }
            if (sequences.get())
                res.add_union(sequences);
            else
                trace_query(" latmax matches end of index: no point in adding a filter\n");
        }
    }

    if (ident)
    {
        trace_query("Found ident=%s\n", ident);
        Index<std::string>::const_iterator iident = by_ident.find(ident);
        if (iident == by_ident.end())
        {
            trace_query(" ident not found in index: setting empty result set\n");
            res.set_to_empty();
            return;
        }

        res.add_set(iident->second);
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_REP_MEMO))
        res.add(new MatchReport(val));

    //strategy.activate(res);
}

void Stations::dump(FILE* out) const
{
    fprintf(out, "Stations:\n");
    for (size_t pos = 0; pos < values.size(); ++pos)
    {
        if (values[pos])
            fprintf(out, " %4zu %d %d %d %s %s\n",
                    pos, values[pos]->coords.lat, values[pos]->coords.lon,
                    values[pos]->mobile, values[pos]->report.c_str(), values[pos]->ident.c_str());
        else
            fprintf(out, " %4zu (empty)\n", pos);
    }

    /*
    fprintf(out, " coord index:\n");
    for (Index<Coord>::const_iterator i = by_coord.begin(); i != by_coord.end(); ++i)
    {
        fprintf(out, "  %d %d -> ", i->first.lat, i->first.lon);
        i->second.dump(out);
        putc('\n', out);
    }
    fprintf(out, " ident index:\n");
    for (Index<string>::const_iterator i = by_ident.begin(); i != by_ident.end(); ++i)
    {
        fprintf(out, "  %s -> \n", i->first.c_str());
        i->second.dump(out);
        putc('\n', out);
    }
    */
};

}
}

#include "valuestorage.tcc"
#include "results.tcc"

namespace dballe {
namespace memdb {
template class Results<Station>;
}
}
