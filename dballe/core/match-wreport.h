#ifndef DBALLE_CORE_MATCH_WREPORT_H
#define DBALLE_CORE_MATCH_WREPORT_H

/** @file
 * @ingroup core
 * Implement a storage object for a group of related observation data
 */

#include <dballe/core/matcher.h>

namespace wreport {
struct Var;
struct Subset;
struct Bulletin;
}

namespace dballe {

struct MatchedSubset : public Matched
{
    const wreport::Subset& r;

    MatchedSubset(const wreport::Subset& r);
    ~MatchedSubset();

    /**
     * Return YES if the subset contains at least one var with the given B33195
     * attribute; else return NA.
     */
    matcher::Result match_var_id(int val) const override;
    matcher::Result match_station_id(int val) const override;
    matcher::Result match_station_wmo(int block, int station=-1) const override;
    matcher::Result match_datetime(const DatetimeRange& range) const override;
    matcher::Result match_coords(const LatRange& latrange, const LonRange& lonrange) const override;
    matcher::Result match_rep_memo(const char* memo) const override;

protected:
    Datetime date;
    int lat, lon;
    const wreport::Var* var_ana_id;
    const wreport::Var* var_block;
    const wreport::Var* var_station;
    const wreport::Var* var_rep_memo;
};

/**
 * Match all subsets in turn, returning true if at least one subset matches
 */
struct MatchedBulletin : public Matched
{
    const wreport::Bulletin& r;

    MatchedBulletin(const wreport::Bulletin& r);
    ~MatchedBulletin();

    matcher::Result match_var_id(int val) const override;
    matcher::Result match_station_id(int val) const override;
    matcher::Result match_station_wmo(int block, int station=-1) const override;
    matcher::Result match_datetime(const DatetimeRange& range) const override;
    matcher::Result match_coords(const LatRange& latrange, const LonRange& lonrange) const override;
    matcher::Result match_rep_memo(const char* memo) const override;

protected:
    const MatchedSubset** subsets;
};

}
#endif
