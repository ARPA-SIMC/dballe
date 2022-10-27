#define _DBALLE_LIBRARY_CODE
#include "summary_xapian.h"
#include "summary_utils.h"
#include "dballe/core/var.h"
#include "dballe/core/query.h"
#include "dballe/core/json.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include <array>
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

}

#define CATCH_XAPIAN_RETHROW_WREPORT \
    } catch (Xapian::Error& e) { \
        wreport::error_consistency::throwf("Xapian error %s", e.get_description().c_str()); \

struct XapianDBInmemory : public XapianDB
{
    Xapian::WritableDatabase db;

    XapianDBInmemory()
        // See https://en.cppreference.com/w/cpp/language/function-try-block
        try : db("/dev/null", Xapian::DB_BACKEND_INMEMORY)
    {
    CATCH_XAPIAN_RETHROW_WREPORT
    }

    Xapian::Database& reader() override { return db; }
    Xapian::WritableDatabase& writer() override { return db; }
    void commit() override { db.commit(); }

    void clear() override {
        db.close();
        db = Xapian::WritableDatabase("/dev/null", Xapian::DB_BACKEND_INMEMORY | Xapian::DB_CREATE_OR_OVERWRITE);
    }
};

struct XapianDBOndisk : public XapianDB
{
    std::string pathname;
    std::unique_ptr<Xapian::Database> db;
    std::unique_ptr<Xapian::WritableDatabase> writable_db;

    XapianDBOndisk(const std::string& pathname)
        : pathname(pathname)
    {
    }

    Xapian::Database& reader() override
    {
        if (!db)
            db.reset(new Xapian::Database(pathname, Xapian::DB_OPEN));
        return *db;
    }

    Xapian::WritableDatabase& writer() override
    {
        if (!writable_db)
            writable_db.reset(new Xapian::WritableDatabase(pathname, Xapian::DB_CREATE_OR_OPEN));
        return *writable_db;
    }

    void commit() override
    {
        // If we don't have a writer, create it just to commit.
        //
        // This means that even if no data were added, an index is still
        // getting created on disk
        writer().commit();
        writable_db.reset();
        if (db)
            db->reopen();
    }

    void clear() override {
        db.reset();
        writable_db.reset();

        Xapian::WritableDatabase(pathname, Xapian::DB_CREATE_OR_OVERWRITE);
    }
};


template<typename Station>
BaseSummaryXapian<Station>::BaseSummaryXapian()
{
    db.reset(new XapianDBInmemory);
}

template<typename Station>
BaseSummaryXapian<Station>::BaseSummaryXapian(const std::string& pathname)
{
    db.reset(new XapianDBOndisk(pathname));
}

template<typename Station>
BaseSummaryXapian<Station>::~BaseSummaryXapian()
{
}

template<typename Station>
bool BaseSummaryXapian<Station>::stations(std::function<bool(const Station&)> dest) const
{
    try {
        auto& db = this->db->reader();
        auto end = db.allterms_end("S");
        for (auto ti = db.allterms_begin("S"); ti != end; ++ti)
            if (!dest(station_from_term<Station>(*ti)))
                return false;
        return true;
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
bool BaseSummaryXapian<Station>::reports(std::function<bool(const std::string&)> dest) const
{
    try {
        auto& db = this->db->reader();
        core::SmallUniqueValueSet<std::string> res;
        auto end = db.allterms_end("S");
        for (auto ti = db.allterms_begin("S"); ti != end; ++ti)
            res.add(station_from_term<Station>(*ti).report);

        for (const auto& r: res)
            if (!dest(r))
                return false;
        return true;
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
bool BaseSummaryXapian<Station>::levels(std::function<bool(const Level&)> dest) const
{
    try {
        auto& db = this->db->reader();
        auto end = db.allterms_end("L");
        for (auto ti = db.allterms_begin("L"); ti != end; ++ti)
            if (!dest(level_from_term(*ti)))
                return false;
        return true;
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
bool BaseSummaryXapian<Station>::tranges(std::function<bool(const Trange&)> dest) const
{
    try {
        auto& db = this->db->reader();
        auto end = db.allterms_end("T");
        for (auto ti = db.allterms_begin("T"); ti != end; ++ti)
            if (!dest(trange_from_term(*ti)))
                return false;
        return true;
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}


template<typename Station>
bool BaseSummaryXapian<Station>::varcodes(std::function<bool(const wreport::Varcode&)> dest) const
{
    try {
        auto& db = this->db->reader();
        auto end = db.allterms_end("B");
        for (auto ti = db.allterms_begin("B"); ti != end; ++ti)
            if (!dest(varcode_from_term(*ti)))
                return false;
        return true;
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
Datetime BaseSummaryXapian<Station>::datetime_min() const
{
    try {
        auto& db = this->db->reader();
        std::string lb = db.get_value_lower_bound(0);
        if (lb.empty())
            return Datetime();
        return Datetime::from_iso8601(lb.c_str());
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
Datetime BaseSummaryXapian<Station>::datetime_max() const
{
    try {
        auto& db = this->db->reader();
        std::string lb = db.get_value_upper_bound(1);
        if (lb.empty())
            return Datetime();
        return Datetime::from_iso8601(lb.c_str());
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
unsigned BaseSummaryXapian<Station>::data_count() const
{
    try {
        auto& db = this->db->reader();
        unsigned res = 0;
        for (auto ival = db.valuestream_begin(2); ival != db.valuestream_end(2); ++ival)
            res += Xapian::sortable_unserialise(*ival);
        return res;
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
void BaseSummaryXapian<Station>::clear()
{
    try {
        db->clear();
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
void BaseSummaryXapian<Station>::add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count)
{
    try {
        auto writer = this->db->writer();
        std::array<std::string, 4> terms;
        terms[0] = to_term(station);
        terms[1] = to_term(vd.level);
        terms[2] = to_term(vd.trange);
        terms[3] = to_term(vd.varcode);

        Xapian::Query query(Xapian::Query::OP_AND, terms.begin(), terms.end());

        Xapian::Enquire enq(writer);
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

            writer.add_document(doc);
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

            writer.replace_document(doc.get_docid(), doc);
        }
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
void BaseSummaryXapian<Station>::commit()
{
    try {
        db->commit();
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
bool BaseSummaryXapian<Station>::iter(std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange& dtrange, size_t count)> dest) const
{
    try {
        auto& db = this->db->reader();
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
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
bool BaseSummaryXapian<Station>::iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange& dtrange, size_t count)> dest) const
{
    try {
        auto& db = this->db->reader();
        summary::StationFilter<Station> filter(query);
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
                xquery &= Xapian::Query(Xapian::Query::OP_OR, terms.begin(), terms.end());
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

        switch (q.varcodes.size())
        {
            case 0:
                break;
            case 1:
                xquery &= Xapian::Query(to_term(*q.varcodes.begin()));
                has_query = true;
                break;
            default:
            {
                std::vector<std::string> terms;
                for (const auto& varcode: q.varcodes)
                    terms.emplace_back(to_term(varcode));
                xquery &= Xapian::Query(Xapian::Query::OP_OR, terms.begin(), terms.end());
                has_query = true;
            }
        }

        if (!has_query)
        {
            return iter([&](const Station& station, const summary::VarDesc& var, const DatetimeRange& dtrange, size_t count) {
                if (wanted_dtrange.is_disjoint(dtrange))
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
            if (wanted_dtrange.is_disjoint(dtrange))
                continue;

            size_t count = Xapian::sortable_unserialise(doc.get_value(2));
            if (!dest(station, var, dtrange, count))
                return false;
        }

        return true;
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
void BaseSummaryXapian<Station>::to_json(core::JSONWriter& writer) const
{
    try {
        auto& db = this->db->reader();
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
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template<typename Station>
void BaseSummaryXapian<Station>::dump(FILE* out) const
{
    try {
        auto& db = this->db->reader();
        for (auto i = db.postlist_begin(""); i != db.postlist_end(""); ++i)
        {
            Xapian::Document doc = db.get_document(*i);
            fprintf(out, "Entry:\n");
            for (auto t = doc.termlist_begin(); t != doc.termlist_end(); ++t)
                fprintf(out, "  %s\n", (*t).c_str());

            fprintf(out, "  dmin: %s\n", doc.get_value(0).c_str());
            fprintf(out, "  dmax: %s\n", doc.get_value(1).c_str());
            fprintf(out, "  count: %s\n", doc.get_value(2).c_str());
        }
    CATCH_XAPIAN_RETHROW_WREPORT
    }
}

template class BaseSummaryXapian<dballe::Station>;
template class BaseSummaryXapian<dballe::DBStation>;

}
}
