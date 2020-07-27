#define _DBALLE_LIBRARY_CODE
#include "summary.h"
#include "summary_utils.h"
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
BaseSummary<Station>::BaseSummary()
{
}

template<typename Station>
BaseSummary<Station>::~BaseSummary()
{
}

template<typename Station>
std::unique_ptr<dballe::CursorSummary> BaseSummary<Station>::query_summary(const Query& query) const
{
    return std::unique_ptr<dballe::CursorSummary>(new summary::Cursor<Station>(*this, query));
}

template<typename Station>
void BaseSummary<Station>::add_cursor(const dballe::CursorSummary& cur)
{
    add(cur.get_station(), summary::VarDesc(cur.get_level(), cur.get_trange(), cur.get_varcode()), cur.get_datetimerange(), cur.get_count());
}

template<typename Station>
void BaseSummary<Station>::add_message(const dballe::Message& message, bool station_data, bool data)
{
    const impl::Message& msg = impl::Message::downcast(message);

    Station station;

    // Coordinates
    station.coords = msg.get_coords();
    if (station.coords.is_missing())
        throw wreport::error_notfound("coordinates not found in message to summarise");

    // Report code
    station.report = msg.get_report();

    // Station identifier
    station.ident = msg.get_ident();

    // Datetime
    Datetime dt = msg.get_datetime();
    DatetimeRange dtrange(dt, dt);

    // TODO: obtain the StationEntry only once, and add the rest to it, to
    // avoid looking it up for each variable

    // Station variables
    if (station_data)
    {
        summary::VarDesc vd_ana;
        vd_ana.level = Level();
        vd_ana.trange = Trange();
        for (const auto& val: msg.station_data)
        {
            vd_ana.varcode = val->code();
            add(station, vd_ana, dtrange, 1);
        }
    }

    // Variables
    if (data)
    {
        for (const auto& ctx: msg.data)
        {
            summary::VarDesc vd(ctx.level, ctx.trange, 0);

            for (const auto& val: ctx.values)
            {
                if (not val->isset()) continue;
                vd.varcode = val->code();
                add(station, vd, dtrange, 1);
            }
        }
    }
}

template<typename Station>
void BaseSummary<Station>::add_messages(const std::vector<std::shared_ptr<dballe::Message>>& messages, bool station_data, bool data)
{
    for (const auto& message: messages)
        add_message(*message, station_data, data);
}

template<typename Station>
void BaseSummary<Station>::add_filtered(const BaseSummary<Station>& summary, const dballe::Query& query)
{
    summary.iter_filtered(query, [&](const Station& station, const summary::VarDesc& var, const DatetimeRange& dtrange, size_t count) {
        add(station, var, dtrange, count);
        return true;
    });
}

template<typename Station>
void BaseSummary<Station>::add_summary(const BaseSummary<dballe::Station>& summary)
{
    summary.iter([&](const dballe::Station& station, const summary::VarDesc& var, const DatetimeRange& dtrange, size_t count) {
        add(summary::convert_station<Station, dballe::Station>(station), var, dtrange, count);
        return true;
    });
}

template<typename Station>
void BaseSummary<Station>::add_summary(const BaseSummary<dballe::DBStation>& summary)
{
    summary.iter([&](const dballe::DBStation& station, const summary::VarDesc& var, const DatetimeRange& dtrange, size_t count) {
        add(summary::convert_station<Station, dballe::DBStation>(station), var, dtrange, count);
        return true;
    });
}

namespace {

// This class is used to disentangle code a bit to try and workaround an
// internal compiler error on centos7: https://travis-ci.org/ARPA-SIMC/dballe/jobs/648366650#L1906
template<typename Station>
struct Loader
{
    BaseSummary<Station>& summary;
    Station station;
    std::vector<summary::VarEntry> vars;

    Loader(BaseSummary<Station>& summary)
        : summary(summary) {}

    void load_station_entry(core::json::Stream& in)
    {
        in.parse_array([&](){
            vars.clear();
            in.parse_object([&](const std::string& key) {
                if (key == "s")
                    station = in.parse<Station>();
                else if (key == "v")
                {
                    in.parse_array([&]{
                        vars.emplace_back();
                        summary::VarEntry& var = vars.back();

                        in.parse_object([&](const std::string& key) {
                            if (key == "l")
                                var.var.level = in.parse_level();
                            else if (key == "t")
                                var.var.trange = in.parse_trange();
                            else if (key == "v")
                                var.var.varcode = in.parse_unsigned<unsigned short>();
                            else if (key == "d")
                                var.dtrange = in.parse_datetimerange();
                            else if (key == "c")
                                var.count = in.parse_unsigned<size_t>();
                            else
                                throw core::JSONParseException("unsupported key \"" + key + "\" for summary::VarEntry");
                        });
                    });
                }
                else
                    throw core::JSONParseException("unsupported key \"" + key + "\" for summary::StationEntry");
            });
            for (const auto& e: vars)
                summary.add(station, e.var, e.dtrange, e.count);
        });
    }
};

}

template<typename Station>
void BaseSummary<Station>::load_json(core::json::Stream& in)
{
    Loader<Station> loader(*this);

    in.parse_object([&](const std::string& key) {
        if (key == "e")
            loader.load_station_entry(in);
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for summary::Entry");
    });
}


template class BaseSummary<dballe::Station>;
template class BaseSummary<dballe::DBStation>;

}
}
