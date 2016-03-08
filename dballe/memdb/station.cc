#include "station.h"
#include "results.h"
#include "dballe/record.h"
#include "dballe/core/query.h"
#include "dballe/core/stlutils.h"
#include "dballe/core/values.h"
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

size_t Stations::obtain_fixed(const Coords& coords, const std::string& report, bool create)
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

size_t Stations::obtain_mobile(const Coords& coords, const std::string& ident, const std::string& report, bool create)
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
    if (const Var* var = rec.get("ana_id"))
    {
        size_t res = var->enqi();
        if (res > values.size() || !values[res])
            error_notfound::throwf("ana_id %zd is invalid", res);
        return res;
    }

    // Lookup by lat, lon and ident
    int s_lat;
    if (const Var* var = rec.get("lat"))
        s_lat = var->enqi();
    else
        throw error_notfound("record with no latitude, looking up a memdb Station");

    int s_lon;
    if (const Var* var = rec.get("lon"))
        s_lon = var->enqi();
    else
        throw error_notfound("record with no longitude, looking up a memdb Station");

    const char* s_report = nullptr;
    if (const Var* var = rec.get("rep_memo"))
        if (var->isset())
            s_report = var->enqc();
    if (!s_report)
        throw error_notfound("record with no rep_memo, looking up a memdb Station");

    const Var* var_ident = rec.get("ident");
    if (var_ident and var_ident->isset())
        return obtain_mobile(Coords(s_lat, s_lon), var_ident->enqc(), s_report, create);
    else
        return obtain_fixed(Coords(s_lat, s_lon), s_report, create);
}

size_t Stations::obtain(const dballe::Station& st, bool create)
{
    // Shortcut by ana_id
    if (st.ana_id != MISSING_INT)
    {
        if ((unsigned)st.ana_id > values.size() || !values[st.ana_id])
            error_notfound::throwf("ana_id %d is invalid", st.ana_id);
        return st.ana_id;
    }

    if (st.ident.is_missing())
        return obtain_fixed(st.coords, st.report, create);
    else
        return obtain_mobile(st.coords, st.ident, st.report, create);
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

struct MatchLon : public Match<Station>
{
    LonRange range;

    MatchLon(const LonRange& range) : range(range) {}

    virtual bool operator()(const Station& val) const
    {
        return range.contains(val.coords.lon);
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

}

void Stations::query(const core::Query& q, Results<Station>& res) const
{
    if (q.ana_id != MISSING_INT)
    {
        trace_query("Found ana_id %d\n", q.ana_id);
        size_t pos = q.ana_id;
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

    if (!q.lonrange.is_missing())
        res.add(new MatchLon(q.lonrange));

    if (q.mobile != MISSING_INT)
    {
        trace_query("Found mobile=%d, using MatchMobile\n", q.mobile);
        res.add(new MatchMobile(q.mobile == 1));
    }

    // Lookup latitude query parameters
    if (!q.latrange.is_missing())
    {
        // In this cose, we can assume that if we match the whole index, then
        // we match every station.
        //
        // We can do this only because the index covers all the items: that is,
        // there are no stations without a latitude. We cannot do this, for
        // example, with the ident index.
        const int& latmin = q.latrange.imin;
        const int& latmax = q.latrange.imax;
        trace_query("Found latmin=%d, latmax=%d\n", latmin, latmax);

        if (latmin != LatRange::IMIN && latmax != LatRange::IMAX)
        {
            bool found;
            unique_ptr< stl::Sequences<size_t> > sequences(by_lat.search_between(latmin, latmax + 1, found));
            if (!found)
            {
                trace_query(" latmin-latmax range is outside the index: setting empty result set\n");
                res.set_to_empty();
                return;
            }
            if (sequences.get())
                res.add_union(std::move(sequences));
            else
                trace_query(" latmin-latmax range covers the whole index: no point in adding a filter\n");
        } else if (latmin != LatRange::IMIN) {
            bool found;
            unique_ptr< stl::Sequences<size_t> > sequences(by_lat.search_from(latmin, found));
            if (!found)
            {
                trace_query(" latmin is beyond end of index: setting empty result set\n");
                res.set_to_empty();
                return;
            }
            if (sequences.get())
                res.add_union(std::move(sequences));
            else
                trace_query(" latmin matches beginning of index: no point in adding a filter\n");
        } else if (latmax != LatRange::IMAX) {
            bool found;
            unique_ptr< stl::Sequences<size_t> > sequences(by_lat.search_to(latmax + 1, found));
            if (!found)
            {
                trace_query(" latmax is before start of index: setting empty result set\n");
                res.set_to_empty();
                return;
            }
            if (sequences.get())
                res.add_union(std::move(sequences));
            else
                trace_query(" latmax matches end of index: no point in adding a filter\n");
        }
    }

    // Lookup ident
    if (!q.ident.is_missing())
    {
        trace_query("Found ident=%s\n", q.ident.get());
        Index<std::string>::const_iterator iident = by_ident.find(q.ident.get());
        if (iident == by_ident.end())
        {
            trace_query(" ident not found in index: setting empty result set\n");
            res.set_to_empty();
            return;
        }

        res.add_set(iident->second);
    }

    if (!q.rep_memo.empty())
        res.add(new MatchReport(q.rep_memo));

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
    for (Index<Coords>::const_iterator i = by_coord.begin(); i != by_coord.end(); ++i)
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
