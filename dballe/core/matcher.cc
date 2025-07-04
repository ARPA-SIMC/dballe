#include "matcher.h"
#include "defs.h"
#include "query.h"
#include <cmath>
#include <iostream>
#include <vector>

using namespace std;
using namespace wreport;

namespace dballe {

matcher::Result Matched::match_var_id(int) const { return matcher::MATCH_NA; }
matcher::Result Matched::match_station_id(int) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_station_wmo(int, int) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_datetime(const DatetimeRange&) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_coords(const LatRange&, const LonRange&) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_rep_memo(const char* memo) const
{
    return matcher::MATCH_NA;
}

matcher::Result Matched::int_in_range(int val, int min, int max)
{
    if (min != MISSING_INT && val < min)
        return matcher::MATCH_NO;
    if (max != MISSING_INT && max < val)
        return matcher::MATCH_NO;
    return matcher::MATCH_YES;
}

matcher::Result Matched::lon_in_range(int val, int min, int max)
{
    if (min == MISSING_INT && max == MISSING_INT)
        return matcher::MATCH_YES;
    if (min == MISSING_INT || max == MISSING_INT)
        throw error_consistency("both minimum and maximum values must be set "
                                "when matching longitudes");
    if (min < max)
        return (val >= min && val <= max) ? matcher::MATCH_YES
                                          : matcher::MATCH_NO;
    else
        return ((val >= min && val <= 18000000) ||
                (val >= -18000000 && val <= max))
                   ? matcher::MATCH_YES
                   : matcher::MATCH_NO;
}

namespace matcher {

std::string result_format(Result res)
{
    switch (res)
    {
        case MATCH_YES: return "yes";
        case MATCH_NO:  return "no";
        case MATCH_NA:  return "n/a";
    }
    return "unknown";
}

struct And : public Matcher
{
    /**
     * Matchers added to exprs are *owned* and will be deallocated in the
     * destructor
     */
    std::vector<const Matcher*> exprs;

    virtual ~And()
    {
        for (std::vector<const Matcher*>::iterator i = exprs.begin();
             i != exprs.end(); ++i)
            delete *i;
    }

    Result match(const Matched& item) const override
    {
        if (exprs.empty())
            return MATCH_YES;

        Result res = MATCH_NA;
        for (std::vector<const Matcher*>::const_iterator i = exprs.begin();
             i != exprs.end() && res != MATCH_NO; ++i)
        {
            switch ((*i)->match(item))
            {
                case MATCH_YES: res = MATCH_YES; break;
                case MATCH_NO:  res = MATCH_NO; break;
                case MATCH_NA:  break;
            }
        }
        return res;
    }

    void to_query(core::Query& query) const override
    {
        for (std::vector<const Matcher*>::const_iterator i = exprs.begin();
             i != exprs.end(); ++i)
            (*i)->to_query(query);
    }
};

struct AnaIDMatcher : public Matcher
{
    // Station ID to match
    int ana_id;

    AnaIDMatcher(int ana_id) : ana_id(ana_id) {}

    Result match(const Matched& v) const override
    {
        return v.match_station_id(ana_id) == MATCH_YES ? MATCH_YES : MATCH_NO;
    }
    void to_query(core::Query& query) const override { query.ana_id = ana_id; }
};

struct WMOMatcher : public Matcher
{
    int block;
    int station;

    WMOMatcher(int block, int station = -1) : block(block), station(station) {}

    Result match(const Matched& v) const override
    {
        return v.match_station_wmo(block, station) == MATCH_YES ? MATCH_YES
                                                                : MATCH_NO;
    }
    void to_query(core::Query& query) const override
    {
        query.block = block;
        if (station != -1)
            query.station = station;
        else
            query.station = MISSING_INT;
    }
};

struct DateMatcher : public Matcher
{
    DatetimeRange range;

    DateMatcher(const DatetimeRange& range) : range(range) {}

    Result match(const Matched& v) const override
    {
        return v.match_datetime(range) == MATCH_YES ? MATCH_YES : MATCH_NO;
    }

    void to_query(core::Query& query) const override { query.dtrange = range; }
};

struct CoordMatcher : public Matcher
{
    LatRange latrange;
    LonRange lonrange;

    CoordMatcher(const LatRange& latrange, const LonRange& lonrange)
        : latrange(latrange), lonrange(lonrange)
    {
    }

    Result match(const Matched& v) const override
    {
        return v.match_coords(latrange, lonrange) == MATCH_YES ? MATCH_YES
                                                               : MATCH_NO;
    }

    void to_query(core::Query& query) const override
    {
        query.latrange = latrange;
        query.lonrange = lonrange;
    }
};

static string tolower(const std::string& s)
{
    string res(s);
    for (string::iterator i = res.begin(); i != res.end(); ++i)
        *i = ::tolower(*i);
    return res;
}

struct ReteMatcher : public Matcher
{
    string rete;

    ReteMatcher(const std::string& rete) : rete(tolower(rete)) {}

    Result match(const Matched& v) const override
    {
        return v.match_rep_memo(rete.c_str()) == MATCH_YES ? MATCH_YES
                                                           : MATCH_NO;
    }
    void to_query(core::Query& query) const override { query.report = rete; }
};

} // namespace matcher

std::unique_ptr<Matcher> Matcher::create(const dballe::Query& query_gen)
{
    using namespace matcher;
    const core::Query& query = core::Query::downcast(query_gen);

    std::unique_ptr<And> res(new And);

    if (query.ana_id != MISSING_INT)
        res->exprs.push_back(new AnaIDMatcher(query.ana_id));

    if (query.block != MISSING_INT)
    {
        if (query.station != MISSING_INT)
            res->exprs.push_back(new WMOMatcher(query.block, query.station));
        else
            res->exprs.push_back(new WMOMatcher(query.block));
    }

    if (!query.dtrange.is_missing())
        res->exprs.push_back(new DateMatcher(query.dtrange));

    if (!query.latrange.is_missing() || !query.lonrange.is_missing())
        res->exprs.push_back(new CoordMatcher(query.latrange, query.lonrange));

    if (!query.report.empty())
        res->exprs.push_back(new ReteMatcher(query.report));

    return unique_ptr<Matcher>(res.release());
}

} // namespace dballe
