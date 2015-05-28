#include "dballe/core/matcher.h"
#include "dballe/core/defs.h"
#include "dballe/core/record.h"
#include "dballe/core/query.h"
#include <cmath>
#include <iostream>

using namespace std;
using namespace wreport;

namespace dballe {

matcher::Result Matched::match_var_id(int) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_station_id(int) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_station_wmo(int, int) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_date(const Datetime&, const Datetime&) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_coords(const Coords& cmin, const Coords& cmax) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_rep_memo(const char* memo) const
{
    return matcher::MATCH_NA;
}

matcher::Result Matched::date_in_range(const Datetime& date, const Datetime& min, const Datetime& max)
{
    if (!min.is_missing() && date < min) return matcher::MATCH_NO;
    if (!max.is_missing() && date > max) return matcher::MATCH_NO;
    return matcher::MATCH_YES;
}

matcher::Result Matched::int_in_range(int val, int min, int max)
{
    if (min != MISSING_INT && val < min) return matcher::MATCH_NO;
    if (max != MISSING_INT && max < val) return matcher::MATCH_NO;
    return matcher::MATCH_YES;
}

matcher::Result Matched::lon_in_range(int val, int min, int max)
{
    if (min == MISSING_INT && max == MISSING_INT) return matcher::MATCH_YES;
    if (min == MISSING_INT || max == MISSING_INT)
        throw error_consistency("both minimum and maximum values must be set when matching longitudes");
    if (min < max)
        return (val >= min && val <= max) ? matcher::MATCH_YES : matcher::MATCH_NO;
    else
        return ((val >= min && val <= 18000000) || (val >= -18000000 && val <= max)) ? matcher::MATCH_YES : matcher::MATCH_NO;
}

namespace matcher {

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

    Result match(const Matched& item) const
    {
        if (exprs.empty()) return MATCH_YES;

        Result res = MATCH_NA;
        for (std::vector<const Matcher*>::const_iterator i = exprs.begin();
                i != exprs.end() && res != MATCH_NO; ++i)
        {
            switch ((*i)->match(item))
            {
                case MATCH_YES:
                    res = MATCH_YES;
                    break;
                case MATCH_NO:
                    res = MATCH_NO;
                    break;
                case MATCH_NA:
                    break;
            }
        }
        return res;
    }

    virtual void to_record(dballe::Record& query) const
    {
        for (std::vector<const Matcher*>::const_iterator i = exprs.begin();
                i != exprs.end(); ++i)
            (*i)->to_record(query);
    }
};

struct VarIDMatcher : public Matcher
{
    // Variable ID to match
    int var_id;

    VarIDMatcher(int var_id) : var_id(var_id) {}

    virtual Result match(const Matched& v) const
    {
        return v.match_var_id(var_id) == MATCH_YES ? MATCH_YES : MATCH_NO;
    }
    virtual void to_record(Record& query) const
    {
        query.set(WR_VAR(0, 33, 195), var_id);
    }
};

struct AnaIDMatcher : public Matcher
{
    // Station ID to match
    int ana_id;

    AnaIDMatcher(int ana_id) : ana_id(ana_id) {}

    virtual Result match(const Matched& v) const
    {
        return v.match_station_id(ana_id) == MATCH_YES ? MATCH_YES : MATCH_NO;
    }
    virtual void to_record(Record& query) const
    {
        query.set(DBA_KEY_ANA_ID, ana_id);
    }
};

struct WMOMatcher : public Matcher
{
    int block;
    int station;

    WMOMatcher(int block, int station=-1) : block(block), station(station) {}

    virtual Result match(const Matched& v) const
    {
        return v.match_station_wmo(block, station) == MATCH_YES ? MATCH_YES : MATCH_NO;
    }
    virtual void to_record(Record& query) const
    {
        query.set(WR_VAR(0, 1, 1), block);
        if (station != -1)
            query.set(WR_VAR(0, 1, 2), station);
    }
};


struct DateMatcher : public Matcher
{
    Datetime dtmin;
    Datetime dtmax;

    DateMatcher(const Datetime& dtmin, const Datetime& dtmax)
        : dtmin(dtmin), dtmax(dtmax)
    {
    }

    /// Return true if v1 == v2
    bool eq(const int* v1, const int* v2) const
    {
        for (int i = 0; i < 6; ++i)
            if (v1[i] != v2[i])
                return false;
        return true;
    }

    virtual Result match(const Matched& v) const
    {
        return v.match_date(dtmin, dtmax) == MATCH_YES ? MATCH_YES : MATCH_NO;
    }

    virtual void to_record(Record& query) const
    {
        if (dtmin == dtmax)
        {
            if (!dtmin.is_missing())
                query.set(dtmin);
        } else {
            if (!dtmin.is_missing())
                query.setmin(dtmin);
            if (!dtmax.is_missing())
                query.setmax(dtmax);
        }
    }
};

struct CoordMatcher : public Matcher
{
    Coords cmin;
    Coords cmax;

    CoordMatcher(const Coords& cmin, const Coords& cmax)
        : cmin(cmin), cmax(cmax) {}

    virtual Result match(const Matched& v) const
    {
        return v.match_coords(cmin, cmax) == MATCH_YES ? MATCH_YES : MATCH_NO;
    }

    virtual void to_record(Record& query) const
    {
        if (cmin.lat != MISSING_INT) query.set(DBA_KEY_LATMIN, cmin.lat);
        if (cmax.lat != MISSING_INT) query.set(DBA_KEY_LATMAX, cmax.lat);
        if (cmin.lon != MISSING_INT) query.set(DBA_KEY_LONMIN, cmin.lon);
        if (cmax.lon != MISSING_INT) query.set(DBA_KEY_LONMAX, cmax.lon);
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

    virtual Result match(const Matched& v) const
    {
        return v.match_rep_memo(rete.c_str()) == MATCH_YES ? MATCH_YES : MATCH_NO;
    }
    virtual void to_record(Record& query) const
    {
        query.set(DBA_KEY_REP_MEMO, rete.c_str());
    }
};

}

static inline int int_or_missing(const Record& query, dba_keyword key)
{
    if (const Var* var = query.key_peek(key))
        return var->enqi();
    else
        return MISSING_INT;
}

// Returns false if no date filter was found at all
// Limits are degrees * 100000
static bool parse_lat_extremes(const Record& query, int* rlatmin, int* rlatmax, int* rlonmin, int* rlonmax)
{
    int lat = int_or_missing(query, DBA_KEY_LAT);
    if (lat != MISSING_INT)
    {
        *rlatmin = lat;
        *rlatmax = lat;
    } else {
        *rlatmin = int_or_missing(query, DBA_KEY_LATMIN);
        *rlatmax = int_or_missing(query, DBA_KEY_LATMAX);
    }

    int lon = int_or_missing(query, DBA_KEY_LON);
    if (lon != MISSING_INT)
    {
        *rlonmin = lon;
        *rlonmax = lon;
    } else {
        *rlonmin = int_or_missing(query, DBA_KEY_LONMIN);
        *rlonmax = int_or_missing(query, DBA_KEY_LONMAX);
    }

    if (*rlatmin == MISSING_INT && *rlatmax == MISSING_INT
     && *rlonmin == MISSING_INT && *rlonmax == MISSING_INT)
        return false;

    return true;
}

std::unique_ptr<Matcher> Matcher::create(const dballe::Query& query_gen)
{
    using namespace matcher;
    const core::Query& query = core::Query::downcast(query_gen);

    std::unique_ptr<And> res(new And);

    if (query.data_id != MISSING_INT)
        res->exprs.push_back(new VarIDMatcher(query.data_id));

    if (query.ana_id != MISSING_INT)
        res->exprs.push_back(new AnaIDMatcher(query.ana_id));

    if (query.block != MISSING_INT)
    {
        if (query.station != MISSING_INT)
            res->exprs.push_back(new WMOMatcher(query.block, query.station));
        else
            res->exprs.push_back(new WMOMatcher(query.block));
    }

    Datetime dtmin = query.datetime_min.lower_bound();
    Datetime dtmax = query.datetime_max.upper_bound();
    if (!dtmin.is_missing() || !dtmax.is_missing())
        res->exprs.push_back(new DateMatcher(dtmin, dtmax));

    if (query.coords_min.lat != MISSING_INT || query.coords_min.lon != MISSING_INT
     || query.coords_max.lat != MISSING_INT || query.coords_max.lon != MISSING_INT)
        res->exprs.push_back(new CoordMatcher(query.coords_min, query.coords_max));

    if (!query.rep_memo.empty())
        res->exprs.push_back(new ReteMatcher(query.rep_memo));

    return unique_ptr<Matcher>(res.release());
}

}

/* vim:set syntax=cpp ts=4 sw=4 si: */
