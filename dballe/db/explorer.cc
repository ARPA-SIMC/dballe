#define _DBALLE_LIBRARY_CODE
#include "explorer.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include <cstring>

using namespace std;
using namespace dballe;

namespace dballe {
namespace db {

Explorer::Explorer()
{
}

Explorer::~Explorer()
{
    delete _global_summary;
    delete _active_summary;
}

const dballe::db::DBSummary& Explorer::global_summary() const
{
    if (!_global_summary)
        throw std::runtime_error("global summary is not available, call revalidate first");
    return *_global_summary;
}

const dballe::db::DBSummary& Explorer::active_summary() const
{
    if (!_active_summary)
        throw std::runtime_error("active summary is not available, call revalidate first");
    return *_active_summary;
}

const dballe::Query& Explorer::get_filter() const
{
    return filter;
}

void Explorer::set_filter(const dballe::Query& query)
{
    filter = core::Query::downcast(query);
    update_active_summary();
}

void Explorer::revalidate(dballe::db::Transaction& tr)
{
    delete _global_summary;
    _global_summary = nullptr;
    delete _active_summary;
    _active_summary = nullptr;

    core::Query query;
    query.query = "details";

    unique_ptr<db::DBSummary> new_global_summary(new db::DBSummary);

    auto cur = tr.query_summary(query);
    while (cur->next())
        new_global_summary->add_cursor(*cur);

    _global_summary = new_global_summary.release();

    update_active_summary();
}

#if 0
/// Represent whether a summary can satisfy a given query
enum Support
{
    /// It cannot.
    UNSUPPORTED = 0,
    /// The query may select less data than this summary can estimate.
    OVERESTIMATED = 1,
    /// The query selects data that this summary can estimate exactly.
    EXACT = 2,
};
#endif

void Explorer::update_active_summary()
{
    unique_ptr<db::DBSummary> new_active_summary(new db::DBSummary);
    new_active_summary->add_filtered(*_global_summary, filter);

#if 0
    switch (summary.supports(query))
    {
        case summary::Support::UNSUPPORTED:
            throw std::runtime_error("Source summary does not support the query for this summary");
        case summary::Support::OVERESTIMATED:
        case summary::Support::EXACT:
            break;
    }
#endif

    _active_summary = new_active_summary.release();
}

#if 0
summary::Support Summary::supports(const Query& query) const
{
    using namespace summary;

    // If query is not just a restricted version of our query, then it can
    // select more data than this summary knows about.
    if (!query.is_subquery(this->query))
        return Support::UNSUPPORTED;

    // Now we know that query has either more fields than this->query or changes
    // in datetime or data-related filters
    Support res = Support::EXACT;

    const DatetimeRange& new_range = core::Query::downcast(query).datetime;
    const DatetimeRange& old_range = core::Query::downcast(this->query).datetime;

    // Check if the query has more restrictive datetime extremes
    if (old_range != new_range)
    {
        if (count == MISSING_INT)
        {
            // We do not contain precise datetime information, so we cannot at
            // this point say anything better than "this summary may
            // overestimate the query"
            res = Support::OVERESTIMATED;
        } else {
            // The query introduced further restrictions, check with the actual entries what we can do
            for (const auto& e: entries)
            {
                if (new_range.contains(e.dtrange))
                    ; // If the query entirely contains this summary entry, we can still match it exactly
                else if (new_range.is_disjoint(e.dtrange))
                    // If the query is completely outside of this entry, we can still match exactly
                    ;
                else
                {
                    // If the query instead only partially overlaps this entry,
                    // we may overestimate the results
                    res = Support::OVERESTIMATED;
                    break;
                }
            }
        }
    }

    return res;
}
#endif

void Explorer::update_station(values::Value &val, const wreport::Var &new_val)
{
    /*
    DataValues vals;
    vals.info.ana_id = val.ana_id;
    vals.info.report = val.rep_memo;
    vals.info.level = val.level;
    vals.info.trange = val.trange;
    vals.info.datetime = val.date;
    vals.values.set(new_val);
    db->insert_data(vals, true, false);
    val.var = new_val;
    */
    throw std::runtime_error("not implemented");
}

void Explorer::update_data(values::Value &val, const wreport::Var &new_val)
{
    /*
    StationValues vals;
    vals.info.ana_id = val.ana_id;
    vals.info.report = val.rep_memo;
    vals.values.set(new_val);
    db->insert_station_data(vals, true, false);
    val.var = new_val;
    */
    throw std::runtime_error("not implemented");
}

void Explorer::update_attr(int var_id, wreport::Varcode var_related, const wreport::Var &new_val)
{
    /*
    Values values;
    values.set(new_val);
    db->attr_insert_data(var_id, values);
    */
    throw std::runtime_error("not implemented");
}

void Explorer::remove(const values::Value &val)
{
    /*
    auto change = Query::create();
    core::Query::downcast(*change).ana_id = val.ana_id;
    core::Query::downcast(*change).rep_memo = val.rep_memo;
    change->set_level(val.level);
    change->set_trange(val.trange);
    change->set_datetimerange(DatetimeRange(val.date, val.date));
    core::Query::downcast(*change).varcodes.clear();
    core::Query::downcast(*change).varcodes.insert(val.var.code());
    db.remove(*change);
    vector<Value>::iterator i = std::find(cache_values.begin(), cache_values.end(), val);
    if (i != cache_values.end())
    {
        cache_values.erase(i);
    }
    // TODO: Stations and summary also need revalidating
    */
    throw std::runtime_error("not implemented");
}

}
}
