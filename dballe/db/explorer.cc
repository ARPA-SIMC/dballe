#define _DBALLE_LIBRARY_CODE
#include "explorer.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include <cstring>

using namespace std;
using namespace dballe;

namespace dballe {
namespace db {

template<typename Station>
BaseExplorer<Station>::BaseExplorer()
{
}

template<typename Station>
BaseExplorer<Station>::~BaseExplorer()
{
    delete _global_summary;
    delete _active_summary;
}

template<typename Station>
const dballe::db::BaseSummary<Station>& BaseExplorer<Station>::global_summary() const
{
    if (!_global_summary)
        throw std::runtime_error("global summary is not available, call revalidate first");
    return *_global_summary;
}

template<typename Station>
const dballe::db::BaseSummary<Station>& BaseExplorer<Station>::active_summary() const
{
    if (!_active_summary)
        throw std::runtime_error("active summary is not available, call revalidate first");
    return *_active_summary;
}

template<typename Station>
const dballe::Query& BaseExplorer<Station>::get_filter() const
{
    return filter;
}

template<typename Station>
void BaseExplorer<Station>::set_filter(const dballe::Query& query)
{
    filter = core::Query::downcast(query);
    update_active_summary();
}

template<typename Station>
void BaseExplorer<Station>::revalidate(dballe::db::Transaction& tr)
{
    delete _global_summary;
    _global_summary = nullptr;
    delete _active_summary;
    _active_summary = nullptr;

    core::Query query;
    query.query = "details";

    unique_ptr<db::BaseSummary<Station>> new_global_summary(new db::BaseSummary<Station>);

    auto cur = tr.query_summary(query);
    while (cur->next())
        new_global_summary->add_cursor(*cur);

    _global_summary = new_global_summary.release();

    update_active_summary();
}

template<typename Station>
void BaseExplorer<Station>::update_active_summary()
{
    unique_ptr<db::BaseSummary<Station>> new_active_summary(new db::BaseSummary<Station>);
    new_active_summary->add_filtered(*_global_summary, filter);
    _active_summary = new_active_summary.release();
}

template<typename Station>
void BaseExplorer<Station>::update_station(values::Value &val, const wreport::Var &new_val)
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

template<typename Station>
void BaseExplorer<Station>::update_data(values::Value &val, const wreport::Var &new_val)
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

template<typename Station>
void BaseExplorer<Station>::update_attr(int var_id, wreport::Varcode var_related, const wreport::Var &new_val)
{
    /*
    Values values;
    values.set(new_val);
    db->attr_insert_data(var_id, values);
    */
    throw std::runtime_error("not implemented");
}

template<typename Station>
void BaseExplorer<Station>::remove(const values::Value &val)
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

template class BaseExplorer<dballe::Station>;
template class BaseExplorer<dballe::DBStation>;

}
}
