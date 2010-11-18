/*
 * dballe/matcher - Local query match infrastructure
 *
 * Copyright (C) 2009--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include <dballe/core/matcher.h>
#include <dballe/core/defs.h>
#include <dballe/core/record.h>
#include <cmath>

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
matcher::Result Matched::match_date(const int*, const int*) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_coords(int, int, int, int) const
{
    return matcher::MATCH_NA;
}
matcher::Result Matched::match_rep_memo(const char* memo) const
{
    return matcher::MATCH_NA;
}

/// Return true if v1 < v2
static bool lt(const int* v1, const int* v2)
{
    for (int i = 0; i < 5; ++i)
    {
        if (v1[i] < v2[i])
            return true;
        if (v1[i] > v2[i])
            return false;
    }
    return v1[5] < v2[5];
}

matcher::Result Matched::date_in_range(const int* date, const int* min, const int* max)
{
    if (min[0] != -1 && lt(date, min)) return matcher::MATCH_NO;
    if (max[1] != -1 && lt(max, date)) return matcher::MATCH_NO;
    return matcher::MATCH_YES;
}

matcher::Result Matched::int_in_range(int val, int min, int max)
{
    if (min != MISSING_INT && val < min) return matcher::MATCH_NO;
    if (max != MISSING_INT && max < val) return matcher::MATCH_NO;
    return matcher::MATCH_YES;
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
    int datemin[6];
    int datemax[6];

    DateMatcher(const int minvalues[6], const int maxvalues[6])
    {
        for (int i = 0; i < 6; ++i)
        {
            datemin[i] = minvalues[i];
            datemax[i] = maxvalues[i];
        }
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
        return v.match_date(datemin, datemax) == MATCH_YES ? MATCH_YES : MATCH_NO;
    }

    virtual void to_record(Record& query) const
    {
        if (datemin[0] != -1 && datemax[0] != -1 && eq(datemin, datemax))
        {
            query.set(DBA_KEY_YEAR, datemin[0]);
            query.set(DBA_KEY_MONTH, datemin[1]);
            query.set(DBA_KEY_DAY, datemin[2]);
            query.set(DBA_KEY_HOUR, datemin[3]);
            query.set(DBA_KEY_MIN, datemin[4]);
            query.set(DBA_KEY_SEC, datemin[5]);
        } else {
            if (datemin[0] != -1) {
                query.set(DBA_KEY_YEARMIN, datemin[0]);
                query.set(DBA_KEY_MONTHMIN, datemin[1]);
                query.set(DBA_KEY_DAYMIN, datemin[2]);
                query.set(DBA_KEY_HOURMIN, datemin[3]);
                query.set(DBA_KEY_MINUMIN, datemin[4]);
                query.set(DBA_KEY_SECMIN, datemin[5]);
            }
            if (datemax[0] != -1) {
                query.set(DBA_KEY_YEARMAX, datemax[0]);
                query.set(DBA_KEY_MONTHMAX, datemax[1]);
                query.set(DBA_KEY_DAYMAX, datemax[2]);
                query.set(DBA_KEY_HOURMAX, datemax[3]);
                query.set(DBA_KEY_MINUMAX, datemax[4]);
                query.set(DBA_KEY_SECMAX, datemax[5]);
            }
        }
    }
};

struct CoordMatcher : public Matcher
{
    int latmin, latmax;
    int lonmin, lonmax;

    CoordMatcher(int latmin, int latmax, int lonmin, int lonmax)
        : latmin(latmin), latmax(latmax), lonmin(lonmin), lonmax(lonmax) {}

    virtual Result match(const Matched& v) const
    {
        return v.match_coords(latmin, latmax, lonmin, lonmax) == MATCH_YES ? MATCH_YES : MATCH_NO;
    }

    virtual void to_record(Record& query) const
    {
        if (latmin != MISSING_INT) query.set(DBA_KEY_LATMIN, latmin);
        if (latmax != MISSING_INT) query.set(DBA_KEY_LATMAX, latmax);
        if (lonmin != MISSING_INT) query.set(DBA_KEY_LONMIN, lonmin);
        if (lonmax != MISSING_INT) query.set(DBA_KEY_LONMAX, lonmax);
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

std::auto_ptr<Matcher> Matcher::create(const Record& query)
{
    using namespace matcher;

    std::auto_ptr<And> res(new And);

    if (const Var* var = query.var_peek(WR_VAR(0, 33, 195)))
        res->exprs.push_back(new VarIDMatcher(var->enqi()));

    if (const Var* var = query.key_peek(DBA_KEY_ANA_ID))
        res->exprs.push_back(new AnaIDMatcher(var->enqi()));

    if (const Var* block = query.var_peek(WR_VAR(0, 1, 1)))
    {
        if (const Var* station = query.var_peek(WR_VAR(0, 1, 2)))
            res->exprs.push_back(new WMOMatcher(block->enqi(), station->enqi()));
        else
            res->exprs.push_back(new WMOMatcher(block->enqi()));
    }

    int minvalues[6], maxvalues[6];
    query.parse_date_extremes(minvalues, maxvalues);
    if (minvalues[0] != -1 || maxvalues[0] != -1)
        res->exprs.push_back(new DateMatcher(minvalues, maxvalues));

    int latmin = 0, latmax = 0, lonmin = 0, lonmax = 0;
    if (parse_lat_extremes(query, &latmin, &latmax, &lonmin, &lonmax))
        res->exprs.push_back(new CoordMatcher(latmin, latmax, lonmin, lonmax));

    if (const char* rete = query.key_peek_value(DBA_KEY_REP_MEMO))
        res->exprs.push_back(new ReteMatcher(rete));

    return auto_ptr<Matcher>(res.release());
}

}

/* vim:set syntax=cpp ts=4 sw=4 si: */
