#include <cmath>
#include <cstring>
#include <dballe/core/defs.h>
#include <dballe/core/match-wreport.h>
#include <wreport/bulletin.h>
#include <wreport/subset.h>

using namespace wreport;
using namespace std;

namespace dballe {

MatchedSubset::MatchedSubset(const Subset& r)
    : r(r), lat(MISSING_INT), lon(MISSING_INT), var_ana_id(0), var_block(0),
      var_station(0), var_rep_memo(0)
{
    int ye = MISSING_INT, mo = MISSING_INT, da = MISSING_INT, ho = MISSING_INT,
        mi = MISSING_INT, se = MISSING_INT;

    // Scan message taking note of significant values
    for (Subset::const_iterator i = r.begin(); i != r.end(); ++i)
    {
        switch (i->code())
        {
            case WR_VAR(0, 1, 1):   var_block = &*i; break;
            case WR_VAR(0, 1, 2):   var_station = &*i; break;
            case WR_VAR(0, 1, 192): var_ana_id = &*i; break;
            case WR_VAR(0, 1, 194): var_rep_memo = &*i; break;
            case WR_VAR(0, 4, 1):   ye = i->enq(MISSING_INT); break;
            case WR_VAR(0, 4, 2):   mo = i->enq(MISSING_INT); break;
            case WR_VAR(0, 4, 3):   da = i->enq(MISSING_INT); break;
            case WR_VAR(0, 4, 4):   ho = i->enq(MISSING_INT); break;
            case WR_VAR(0, 4, 5):   mi = i->enq(MISSING_INT); break;
            case WR_VAR(0, 4, 6):   se = i->enq(MISSING_INT); break;
            case WR_VAR(0, 4, 7):
                se = i->isset() ? lround(i->enqd()) : MISSING_INT;
                break;
            case WR_VAR(0, 5, 1):
            case WR_VAR(0, 5, 2):
                if (i->isset())
                    lat = i->enqd() * 100000;
                break;
            case WR_VAR(0, 6, 1):
            case WR_VAR(0, 6, 2):
                if (i->isset())
                    lon = i->enqd() * 100000;
                break;
        }
    }

    // Fill in missing date bits
    date = Datetime::lower_bound(ye, mo, da, ho, mi, se);
}

MatchedSubset::~MatchedSubset() {}

matcher::Result MatchedSubset::match_var_id(int val) const
{
    for (Subset::const_iterator i = r.begin(); i != r.end(); ++i)
    {
        if (const Var* a = i->enqa(WR_VAR(0, 33, 195)))
            if (a->enqi() == val)
                return matcher::MATCH_YES;
    }
    return matcher::MATCH_NA;
}

matcher::Result MatchedSubset::match_station_id(int val) const
{
    if (!var_ana_id)
        return matcher::MATCH_NA;
    if (!var_ana_id->isset())
        return matcher::MATCH_NA;
    return var_ana_id->enqi() == val ? matcher::MATCH_YES : matcher::MATCH_NO;
}

matcher::Result MatchedSubset::match_station_wmo(int block, int station) const
{
    if (!var_block)
        return matcher::MATCH_NA;
    if (!var_block->isset())
        return matcher::MATCH_NA;
    if (var_block->enqi() != block)
        return matcher::MATCH_NO;
    if (station == -1)
        return matcher::MATCH_YES;
    if (!var_station)
        return matcher::MATCH_NA;
    if (!var_station->isset())
        return matcher::MATCH_NA;
    if (var_station->enqi() != station)
        return matcher::MATCH_NO;
    return matcher::MATCH_YES;
}

matcher::Result MatchedSubset::match_datetime(const DatetimeRange& range) const
{
    if (date.is_missing())
        return matcher::MATCH_NA;
    return range.contains(date) ? matcher::MATCH_YES : matcher::MATCH_NO;
}

matcher::Result MatchedSubset::match_coords(const LatRange& latrange,
                                            const LonRange& lonrange) const
{
    matcher::Result r1 = matcher::MATCH_NA;
    if (lat != MISSING_INT)
        r1 = latrange.contains(lat) ? matcher::MATCH_YES : matcher::MATCH_NO;
    else if (latrange.is_missing())
        r1 = matcher::MATCH_YES;

    matcher::Result r2 = matcher::MATCH_NA;
    if (lon != MISSING_INT)
        r2 = lonrange.contains(lon) ? matcher::MATCH_YES : matcher::MATCH_NO;
    else if (lonrange.is_missing())
        r2 = matcher::MATCH_YES;

    if (r1 == matcher::MATCH_YES && r2 == matcher::MATCH_YES)
        return matcher::MATCH_YES;
    if (r1 == matcher::MATCH_NO || r2 == matcher::MATCH_NO)
        return matcher::MATCH_NO;
    return matcher::MATCH_NA;
}

matcher::Result MatchedSubset::match_rep_memo(const char* memo) const
{
    if (!var_rep_memo)
        return matcher::MATCH_NA;
    if (!var_rep_memo->isset())
        return matcher::MATCH_NA;
    return strcmp(memo, var_rep_memo->enqc()) == 0 ? matcher::MATCH_YES
                                                   : matcher::MATCH_NO;
}

MatchedBulletin::MatchedBulletin(const wreport::Bulletin& r) : r(r)
{
    subsets = new const MatchedSubset*[r.subsets.size()];
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        subsets[i] = new MatchedSubset(r.subsets[i]);
}
MatchedBulletin::~MatchedBulletin()
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        delete subsets[i];
    delete[] subsets;
}

matcher::Result MatchedBulletin::match_var_id(int val) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_var_id(val) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedBulletin::match_station_id(int val) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_station_id(val) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedBulletin::match_station_wmo(int block, int station) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_station_wmo(block, station) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result
MatchedBulletin::match_datetime(const DatetimeRange& range) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_datetime(range) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedBulletin::match_coords(const LatRange& latrange,
                                              const LonRange& lonrange) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_coords(latrange, lonrange) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedBulletin::match_rep_memo(const char* memo) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_rep_memo(memo) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

} // namespace dballe
