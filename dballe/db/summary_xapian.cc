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
#include <sstream>

using namespace std;
using namespace dballe;

namespace dballe {
namespace db {

namespace {

std::string to_term(const dballe::Station& station)
{
    std::stringstream res;
    res << "S";
    core::JSONWriter writer(res);
    writer.add(station);
    return res.str();
}

std::string to_term(const dballe::DBStation& station)
{
    std::stringstream res;
    res << "S";
    core::JSONWriter writer(res);
    writer.add(station);
    return res.str();
}

std::string to_term(const dballe::Level& level)
{
    std::stringstream res;
    res << "L";
    core::JSONWriter writer(res);
    writer.add(level);
    return res.str();
}

std::string to_term(const dballe::Trange& trange)
{
    std::stringstream res;
    res << "T";
    core::JSONWriter writer(res);
    writer.add(trange);
    return res.str();
}

std::string to_term(const wreport::Varcode& varcode)
{
    return wreport::varcode_format(varcode);
}

template<typename Station>
Station station_from_term(const std::string& term)
{
    std::stringstream in(term);
    in.get();
    core::json::Stream json(in);
    return json.parse<Station>();
}

Level level_from_term(const std::string& term)
{
    std::stringstream in(term);
    in.get();
    core::json::Stream json(in);
    return json.parse<Level>();
}

Trange trange_from_term(const std::string& term)
{
    std::stringstream in(term);
    in.get();
    core::json::Stream json(in);
    return json.parse<Trange>();
}

wreport::Varcode varcode_from_term(const std::string& term)
{
    return WR_STRING_TO_VAR(term.c_str() + 1);
}


struct StationFilterBase
{
    const core::Query& q;
    bool has_flt_rep_memo;
    bool has_flt_ident;
    bool has_flt_area;
    bool has_flt_station;

    StationFilterBase(const dballe::Query& query)
        : q(core::Query::downcast(query))
    {
        // Scan the filter building a todo list of things to match

        // If there is any filtering on the station, build a whitelist of matching stations
        has_flt_rep_memo = !q.report.empty();
        has_flt_ident = !q.ident.is_missing();
        has_flt_area = !q.latrange.is_missing() || !q.lonrange.is_missing();
        has_flt_station = has_flt_rep_memo || has_flt_area || has_flt_ident;
    }

    template<typename Station>
    bool matches_station(const Station& station)
    {
        if (has_flt_area)
        {
            if (!q.latrange.contains(station.coords.lat) ||
                !q.lonrange.contains(station.coords.lon))
                return false;
        }

        if (has_flt_rep_memo && q.report != station.report)
            return false;

        if (has_flt_ident && q.ident != station.ident)
            return false;

        return true;
    }
};

template<class Station>
struct StationFilter;

template<>
struct StationFilter<dballe::Station> : public StationFilterBase
{
    using StationFilterBase::StationFilterBase;
    bool matches_station(const Station& station)
    {
        return StationFilterBase::matches_station(station);
    }
};

template<>
struct StationFilter<dballe::DBStation> : public StationFilterBase
{
    StationFilter(const dballe::Query& query)
        : StationFilterBase(query)
    {
        has_flt_station |= (q.ana_id != MISSING_INT);
    }

    bool matches_station(const DBStation& station)
    {
        if (q.ana_id != MISSING_INT and station.id != q.ana_id)
            return false;
        return StationFilterBase::matches_station(station);
    }
};

}


template<typename Station>
BaseSummaryXapian<Station>::BaseSummaryXapian()
    : db("/dev/null", Xapian::DB_BACKEND_INMEMORY)
{
}

template<typename Station>
const summary::StationEntries<Station>& BaseSummaryXapian<Station>::stations() const
{
static summary::StationEntries<Station> FIXMEentries;

    FIXMEentries = summary::StationEntries<Station>();
    auto end = db.allterms_end("S");
    for (auto ti = db.allterms_begin("S"); ti != end; ++ti)
    {
        Station station = station_from_term<Station>(*ti);
        FIXMEentries.add(station, summary::VarDesc(Level(), Trange(), 0), DatetimeRange(), 0);
    }

    return FIXMEentries;
}

template<typename Station>
const core::SortedSmallUniqueValueSet<std::string>& BaseSummaryXapian<Station>::reports() const
{
static core::SortedSmallUniqueValueSet<std::string> FIXMEentries;

    FIXMEentries.clear();
    auto end = db.allterms_end("S");
    for (auto ti = db.allterms_begin("S"); ti != end; ++ti)
        FIXMEentries.add(station_from_term<Station>(*ti).report);
    return FIXMEentries;
}

template<typename Station>
const core::SortedSmallUniqueValueSet<dballe::Level>& BaseSummaryXapian<Station>::levels() const
{
static core::SortedSmallUniqueValueSet<dballe::Level> FIXMEentries;

    FIXMEentries.clear();
    auto end = db.allterms_end("L");
    for (auto ti = db.allterms_begin("L"); ti != end; ++ti)
        FIXMEentries.add(level_from_term(*ti));
    return FIXMEentries;
}

template<typename Station>
const core::SortedSmallUniqueValueSet<dballe::Trange>& BaseSummaryXapian<Station>::tranges() const
{
static core::SortedSmallUniqueValueSet<dballe::Trange> FIXMEentries;

    FIXMEentries.clear();
    auto end = db.allterms_end("T");
    for (auto ti = db.allterms_begin("T"); ti != end; ++ti)
        FIXMEentries.add(trange_from_term(*ti));
    return FIXMEentries;
}

template<typename Station>
const core::SortedSmallUniqueValueSet<wreport::Varcode>& BaseSummaryXapian<Station>::varcodes() const
{
static core::SortedSmallUniqueValueSet<wreport::Varcode> FIXMEentries;

    FIXMEentries.clear();
    auto end = db.allterms_end("B");
    for (auto ti = db.allterms_begin("B"); ti != end; ++ti)
        FIXMEentries.add(varcode_from_term(*ti));
    return FIXMEentries;
}

template<typename Station>
Datetime BaseSummaryXapian<Station>::datetime_min() const
{
    std::string lb = db.get_value_lower_bound(0);
    if (lb.empty())
        return Datetime();
    return Datetime::from_iso8601(lb.c_str());
}

template<typename Station>
Datetime BaseSummaryXapian<Station>::datetime_max() const
{
    std::string lb = db.get_value_upper_bound(1);
    if (lb.empty())
        return Datetime();
    return Datetime::from_iso8601(lb.c_str());
}

template<typename Station>
unsigned BaseSummaryXapian<Station>::data_count() const
{
    unsigned res = 0;
    for (auto ival = db.valuestream_begin(2); ival != db.valuestream_end(2); ++ival)
        res += Xapian::sortable_unserialise(*ival);
    return res;
}

template<typename Station>
std::unique_ptr<dballe::CursorSummary> BaseSummaryXapian<Station>::query_summary(const Query& query) const
{
    throw wreport::error_unimplemented("SummaryXapian::query_summary()");
}

template<typename Station>
void BaseSummaryXapian<Station>::add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count)
{
    try {
        std::array<std::string, 4> terms;
        terms[0] = to_term(station);
        terms[1] = to_term(vd.level);
        terms[2] = to_term(vd.trange);
        terms[3] = to_term(vd.varcode);

        Xapian::Query query(Xapian::Query::OP_AND, terms.begin(), terms.end());

        Xapian::Enquire enq(db);
        enq.set_query(query);

        Xapian::MSet mset = enq.get_mset(0, 1);
        if (mset.empty())
        {
            // Insert
            Xapian::Document doc;
            for (const auto& term: terms)
                doc.add_term(term);

            doc.add_value(0, dtrange.min.to_string());
            doc.add_value(1, dtrange.max.to_string());
            doc.add_value(2, Xapian::sortable_serialise(count));

            // TODO: move transaction in explorer updater
            db.add_document(doc);
        } else {
            // Update
            Xapian::Document doc = mset[0].get_document();
            // TODO: merge dtrange, count

            DatetimeRange range(
                    Datetime::from_iso8601(doc.get_value(0).c_str()),
                    Datetime::from_iso8601(doc.get_value(1).c_str()));
            range.merge(dtrange);

            doc.add_value(0, range.min.to_string());
            doc.add_value(1, range.max.to_string());

            int old_count = Xapian::sortable_unserialise(doc.get_value(2));
            doc.add_value(2, Xapian::sortable_serialise(old_count + count));

            // TODO: move transaction in explorer updater
            db.replace_document(doc.get_docid(), doc);
        }
    } catch (Xapian::Error& e) {
        wreport::error_consistency::throwf("Xapian error %s: %s [%s]", e.get_type(), e.get_msg().c_str(), e.get_context().c_str());
    }
}

template<typename Station>
bool BaseSummaryXapian<Station>::iter(std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange& dtrange, size_t count)> dest) const
{
    // Iterate stations
    auto send = db.allterms_end("S");
    for (auto si = db.allterms_begin("S"); si != send; ++si)
    {
        std::stringstream in(*si);
        in.get();
        core::json::Stream json(in);
        Station station = json.parse<Station>();

        // Iterate all documents for this station
        auto end = db.postlist_end(*si);
        for (auto idoc = db.postlist_begin(*si); idoc != end; ++idoc)
        {
            Xapian::Document doc = db.get_document(*idoc);
            summary::VarDesc var;

            for (auto ti = doc.termlist_begin(); ti != doc.termlist_end(); ++ti)
            {
                std::string term = *ti;
                switch (term[0])
                {
                    case 'L': var.level = level_from_term(term); break;
                    case 'T': var.trange = trange_from_term(term); break;
                    case 'B': var.varcode = varcode_from_term(term); break;
                }
            }

            DatetimeRange dtrange(
                    Datetime::from_iso8601(doc.get_value(0).c_str()),
                    Datetime::from_iso8601(doc.get_value(1).c_str()));
            size_t count = Xapian::sortable_unserialise(doc.get_value(2));

            if (!dest(station, var, dtrange, count))
                return false;
        }
    }

    return true;
}

template<typename Station>
bool BaseSummaryXapian<Station>::iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange& dtrange, size_t count)> dest) const
{
    StationFilter<Station> filter(query);
    const core::Query& q = core::Query::downcast(query);
    DatetimeRange wanted_dtrange = q.get_datetimerange();

    Xapian::Query xquery("");
    bool has_query = false;

    if (filter.has_flt_station)
    {
        // Iterate stations to build the station term list
        bool all_stations = true;
        std::vector<std::string> terms;
        auto send = db.allterms_end("S");
        for (auto si = db.allterms_begin("S"); si != send; ++si)
        {
            Station station = station_from_term<Station>(*si);
            if (filter.matches_station(station))
                terms.push_back(*si);
            else
                all_stations = false;
        }

        // No stations match: we are done
        if (terms.empty())
            return true;

        // Skip filtering if we just matched all stations
        if (!all_stations)
        {
            xquery &= Xapian::Query(Xapian::Query::OP_AND, terms.begin(), terms.end());
            has_query = true;
        }
    }

    if (!q.level.is_missing())
    {
        xquery &= Xapian::Query(to_term(q.level));
        has_query = true;
    }

    if (!q.trange.is_missing())
    {
        xquery &= Xapian::Query(to_term(q.trange));
        has_query = true;
    }

    for (const auto& varcode: q.varcodes)
    {
        xquery &= Xapian::Query(to_term(varcode));
        has_query = true;
    }

    if (!has_query)
    {
        return iter([&](const Station& station, const summary::VarDesc& var, const DatetimeRange& dtrange, size_t count) {
            if (!wanted_dtrange.contains(dtrange))
                return true;
            return dest(station, var, dtrange, count);
        });
    }

    Xapian::Enquire enq(db);
    enq.set_query(xquery);

    Xapian::MSet mset = enq.get_mset(0, db.get_doccount());
    for (auto mi = mset.begin(); mi != mset.end(); ++mi)
    {
        Xapian::Document doc = mi.get_document();
        Station station;
        summary::VarDesc var;

        for (auto ti = doc.termlist_begin(); ti != doc.termlist_end(); ++ti)
        {
            std::string term = *ti;
            switch (term[0])
            {
                case 'S': station = station_from_term<Station>(term); break;
                case 'L': var.level = level_from_term(term); break;
                case 'T': var.trange = trange_from_term(term); break;
                case 'B': var.varcode = varcode_from_term(term); break;
            }
        }

        DatetimeRange dtrange(
                Datetime::from_iso8601(doc.get_value(0).c_str()),
                Datetime::from_iso8601(doc.get_value(1).c_str()));
        if (!wanted_dtrange.contains(dtrange))
            continue;

        size_t count = Xapian::sortable_unserialise(doc.get_value(2));
        if (!dest(station, var, dtrange, count))
            return false;
    }

    return true;
}

template<typename Station>
void BaseSummaryXapian<Station>::to_json(core::JSONWriter& writer) const
{
    writer.start_mapping();
    writer.add("e");
    writer.start_list();

    // Iterate stations
    auto send = db.allterms_end("S");
    for (auto si = db.allterms_begin("S"); si != send; ++si)
    {
        std::stringstream in(*si);
        in.get();
        core::json::Stream json(in);
        Station station = json.parse<Station>();

        writer.start_mapping();
        writer.add("s");
        writer.add(station);
        writer.add("v");
        writer.start_list();

        // Iterate all documents for this station
        auto end = db.postlist_end(*si);
        for (auto idoc = db.postlist_begin(*si); idoc != end; ++idoc)
        {
            Xapian::Document doc = db.get_document(*idoc);
            Level level;
            Trange trange;
            wreport::Varcode varcode = 0;

            for (auto ti = doc.termlist_begin(); ti != doc.termlist_end(); ++ti)
            {
                std::string term = *ti;
                switch (term[0])
                {
                    case 'L': level = level_from_term(term); break;
                    case 'T': trange = trange_from_term(term); break;
                    case 'B': varcode = varcode_from_term(term); break;
                }
            }

            DatetimeRange dtrange(
                    Datetime::from_iso8601(doc.get_value(0).c_str()),
                    Datetime::from_iso8601(doc.get_value(1).c_str()));
            int count = Xapian::sortable_unserialise(doc.get_value(2));

            writer.start_mapping();
            writer.add("l", level);
            writer.add("t", trange);
            writer.add("v", varcode);
            writer.add("d", dtrange);
            writer.add("c", count);
            writer.end_mapping();
        }
        writer.end_list();
        writer.end_mapping();
    }

    writer.end_list();
    writer.end_mapping();
}

template<typename Station>
void BaseSummaryXapian<Station>::load_json(core::json::Stream& in)
{
    in.parse_object([&](const std::string& key) {
        if (key == "e")
            in.parse_array([&]{
                in.parse_object([&](const std::string& key) {
                    Station station;
                    if (key == "s")
                        station = in.parse<Station>();
                    else if (key == "v")
                        in.parse_array([&]{
                            summary::VarDesc vd;
                            DatetimeRange dtrange;
                            size_t count = 0;

                            in.parse_object([&](const std::string& key) {
                                if (key == "l")
                                    vd.level = in.parse_level();
                                else if (key == "t")
                                    vd.trange = in.parse_trange();
                                else if (key == "v")
                                    vd.varcode = in.parse_unsigned<unsigned short>();
                                else if (key == "d")
                                    dtrange = in.parse_datetimerange();
                                else if (key == "c")
                                    count = in.parse_unsigned<size_t>();
                                else
                                    throw core::JSONParseException("unsupported key \"" + key + "\" for summary::VarEntry");
                            });

                            add(station, vd, dtrange, count);
                        });
                    else
                        throw core::JSONParseException("unsupported key \"" + key + "\" for summary::StationEntry");
                });
            });
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for summary::Entry");
    });
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
