#define _DBALLE_LIBRARY_CODE
#include "summary_xapian.h"
#include "dballe/core/var.h"
#include "dballe/core/query.h"
#include "dballe/core/json.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include <algorithm>
#include <unordered_set>
#include <cstring>

using namespace std;
using namespace dballe;

namespace dballe {
namespace db {

template<typename Station>
BaseSummaryXapian<Station>::BaseSummaryXapian()
    : db("/dev/null", Xapian::DB_BACKEND_INMEMORY)
{
}

template<typename Station>
const summary::StationEntries<Station>& BaseSummaryXapian<Station>::stations() const
{
    throw wreport::error_unimplemented("SummaryXapian::stations()");
}

template<typename Station>
const core::SortedSmallUniqueValueSet<std::string>& BaseSummaryXapian<Station>::reports() const
{
    throw wreport::error_unimplemented("SummaryXapian::reports()");
}

template<typename Station>
const core::SortedSmallUniqueValueSet<dballe::Level>& BaseSummaryXapian<Station>::levels() const
{
    throw wreport::error_unimplemented("SummaryXapian::levels()");
}

template<typename Station>
const core::SortedSmallUniqueValueSet<dballe::Trange>& BaseSummaryXapian<Station>::tranges() const
{
    throw wreport::error_unimplemented("SummaryXapian::tranges()");
}

template<typename Station>
const core::SortedSmallUniqueValueSet<wreport::Varcode>& BaseSummaryXapian<Station>::varcodes() const
{
    throw wreport::error_unimplemented("SummaryXapian::varcodes()");
}

template<typename Station>
const Datetime& BaseSummaryXapian<Station>::datetime_min() const
{
    throw wreport::error_unimplemented("SummaryXapian::datetime_min()");
}

template<typename Station>
const Datetime& BaseSummaryXapian<Station>::datetime_max() const
{
    throw wreport::error_unimplemented("SummaryXapian::datetime_max()");
}

template<typename Station>
unsigned BaseSummaryXapian<Station>::data_count() const
{
    throw wreport::error_unimplemented("SummaryXapian::data_count()");
}

template<typename Station>
std::unique_ptr<dballe::CursorSummary> BaseSummaryXapian<Station>::query_summary(const Query& query) const
{
    throw wreport::error_unimplemented("SummaryXapian::query_summary()");
}

template<typename Station>
void BaseSummaryXapian<Station>::add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count)
{
    throw wreport::error_unimplemented("SummaryXapian::add(station, vardesc, dtrange, count)");
}

template<typename Station>
void BaseSummaryXapian<Station>::add_cursor(const dballe::CursorSummary& cur)
{
    throw wreport::error_unimplemented("SummaryXapian::add_cursor()");
}

template<typename Station>
void BaseSummaryXapian<Station>::add_message(const dballe::Message& message)
{
    throw wreport::error_unimplemented("SummaryXapian::add_message()");
}

template<typename Station>
void BaseSummaryXapian<Station>::add_filtered(const BaseSummary<Station>& summary, const dballe::Query& query)
{
    throw wreport::error_unimplemented("SummaryXapian::add_filtered()");
}

template<typename Station>
void BaseSummaryXapian<Station>::add_summary(const BaseSummary<dballe::Station>& summary)
{
    throw wreport::error_unimplemented("SummaryXapian::add_summary(station)");
}

template<typename Station>
void BaseSummaryXapian<Station>::add_summary(const BaseSummary<dballe::DBStation>& summary)
{
    throw wreport::error_unimplemented("SummaryXapian::add_summary(dbstation)");
}

template<typename Station>
void BaseSummaryXapian<Station>::to_json(core::JSONWriter& writer) const
{
    throw wreport::error_unimplemented("SummaryXapian::to_json()");
}

template<typename Station>
void BaseSummaryXapian<Station>::load_json(core::json::Stream& in)
{
    throw wreport::error_unimplemented("SummaryXapian::load_json()");
}

template<typename Station>
void BaseSummaryXapian<Station>::dump(FILE* out) const
{
    throw wreport::error_unimplemented("SummaryXapian::dump()");
}

template class BaseSummaryXapian<dballe::Station>;
template class BaseSummaryXapian<dballe::DBStation>;

}
}
