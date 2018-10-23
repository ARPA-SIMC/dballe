#define _DBALLE_LIBRARY_CODE
#include "explorer.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include "dballe/core/json.h"
#include <cstring>

using namespace std;
using namespace dballe;

namespace dballe {
namespace db {

#if 0
namespace {
template<typename T>
bool has_db() { return false; }
template<>
bool has_db<dballe::DBStation>() { return true; }
}
#endif

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
void BaseExplorer<Station>::to_json(core::JSONWriter& writer) const
{
    writer.start_mapping();
    writer.add("summary");
    _global_summary->to_json(writer);
    writer.end_mapping();
}

template<typename Station>
void BaseExplorer<Station>::from_json(core::json::Stream& in)
{
    delete _global_summary;
    _global_summary = nullptr;
    delete _active_summary;
    _active_summary = nullptr;

    std::unique_ptr<BaseSummary<Station>> new_global_summary(new BaseSummary<Station>);
    in.parse_object([&](const std::string& key) {
        if (key == "summary")
            new_global_summary->from_json(in);
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for db::Explorer");
    });

    _global_summary = new_global_summary.release();

    update_active_summary();
}

template class BaseExplorer<dballe::Station>;
template class BaseExplorer<dballe::DBStation>;

}
}
