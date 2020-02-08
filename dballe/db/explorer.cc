#define _DBALLE_LIBRARY_CODE
#include "explorer.h"
#include "summary_memory.h"
#include "dballe/core/query.h"
#include "dballe/core/json.h"
#include <wreport/utils/string.h>
#include <cstring>
#include "config.h"

#ifdef HAVE_XAPIAN
#include "summary_xapian.h"
#endif

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
    _global_summary = make_shared<db::BaseSummaryMemory<Station>>();
    _active_summary = _global_summary;
}

template<typename Station>
BaseExplorer<Station>::BaseExplorer(const std::string& pathname)
{
    using namespace wreport;
    if (str::endswith(pathname, ".json"))
        _global_summary = make_shared<db::BaseSummaryMemory<Station>>(pathname);
    else
    {
#ifdef HAVE_XAPIAN
        _global_summary = make_shared<db::BaseSummaryXapian<Station>>(pathname);
#else
        _global_summary = make_shared<db::BaseSummaryMemory<Station>>(pathname);
#endif
    }
    _active_summary = _global_summary;
}

template<typename Station>
BaseExplorer<Station>::~BaseExplorer()
{
}

template<typename Station>
const dballe::db::BaseSummary<Station>& BaseExplorer<Station>::global_summary() const
{
    if (!_global_summary)
        throw std::runtime_error("global summary is not available, call rebuild or update first");
    return *_global_summary;
}

template<typename Station>
const dballe::db::BaseSummary<Station>& BaseExplorer<Station>::active_summary() const
{
    if (!_active_summary)
        throw std::runtime_error("active summary is not available, call rebuild or update first");
    return *_active_summary;
}

template<typename Station>
void BaseExplorer<Station>::commit()
{
    if (_global_summary)
        _global_summary->commit();
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
    if (_global_summary)
        update_active_summary();
}

template<typename Station>
typename BaseExplorer<Station>::Update BaseExplorer<Station>::rebuild()
{
    if (!_global_summary)
        _global_summary = make_shared<db::BaseSummaryMemory<Station>>();
    else
        _global_summary->clear();
    _active_summary.reset();
    return Update(this);
}

template<typename Station>
typename BaseExplorer<Station>::Update BaseExplorer<Station>::update()
{
    if (!_global_summary)
        _global_summary = make_shared<db::BaseSummaryMemory<Station>>();
    _active_summary.reset();
    return Update(this);
}

template<typename Station>
void BaseExplorer<Station>::update_active_summary()
{
    if (filter.empty())
        _active_summary = _global_summary;
    else
    {
        auto new_active_summary = make_shared<db::BaseSummaryMemory<Station>>();
        new_active_summary->add_filtered(*_global_summary, filter);
        _active_summary = new_active_summary;
    }
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
BaseExplorer<Station>::Update::Update(BaseExplorer<Station>* explorer)
    : explorer(explorer) {}

template<typename Station>
BaseExplorer<Station>::Update::Update() {}

template<typename Station>
BaseExplorer<Station>::Update::Update(Update&& o)
    : explorer(o.explorer) { o.explorer = nullptr; }

template<typename Station>
BaseExplorer<Station>::Update::~Update()
{
    commit();
}

template<typename Station>
void BaseExplorer<Station>::Update::commit()
{
    if (!explorer)
        return;
    explorer->commit();
    explorer->update_active_summary();
    explorer = nullptr;
}

template<typename Station>
typename BaseExplorer<Station>::Update& BaseExplorer<Station>::Update::operator=(Update&& o)
{
    if (&o == this) return *this;
    explorer = o.explorer;
    o.explorer = nullptr;
    return *this;
}

template<typename Station>
void BaseExplorer<Station>::Update::add_db(dballe::db::Transaction& tr)
{
    core::Query query;
    query.query = "details";

    auto cur = tr.query_summary(query);
    add_cursor(*cur);
}

template<typename Station>
void BaseExplorer<Station>::Update::add_cursor(dballe::CursorSummary& cur)
{
    while (cur.next())
        explorer->_global_summary->add_cursor(cur);
}

template<typename Station>
void BaseExplorer<Station>::Update::add_json(core::json::Stream& in)
{
    in.parse_object([&](const std::string& key) {
        if (key == "summary")
            explorer->_global_summary->load_json(in);
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for db::Explorer");
    });
}

template<typename Station> template<typename OStation>
void BaseExplorer<Station>::Update::add_explorer(const BaseExplorer<OStation>& explorer)
{
    this->explorer->_global_summary->add_summary(explorer.active_summary());
}
template<> template<>
void BaseExplorer<Station>::Update::add_explorer(const BaseExplorer<Station>& explorer)
{
    if (this->explorer == &explorer)
        wreport::error_consistency::throwf("Adding an Explorer to itself is not supported");
    this->explorer->_global_summary->add_summary(explorer.active_summary());
}
template<> template<>
void BaseExplorer<DBStation>::Update::add_explorer(const BaseExplorer<DBStation>& explorer)
{
    if (this->explorer == &explorer)
        wreport::error_consistency::throwf("Adding an Explorer to itself is not supported");
    this->explorer->_global_summary->add_summary(explorer.active_summary());
}

template<typename Station>
void BaseExplorer<Station>::Update::add_message(const dballe::Message& message)
{
    this->explorer->_global_summary->add_message(message);
}

template<typename Station>
void BaseExplorer<Station>::Update::add_messages(const std::vector<std::shared_ptr<Message>>& messages)
{
    this->explorer->_global_summary->add_messages(messages);
}

template class BaseExplorer<dballe::Station>;
template void BaseExplorer<dballe::Station>::Update::add_explorer(const BaseExplorer<DBStation>&);
template class BaseExplorer<dballe::DBStation>;
template void BaseExplorer<dballe::DBStation>::Update::add_explorer(const BaseExplorer<Station>&);

}
}
