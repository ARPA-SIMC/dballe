#include "summary.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include "dballe/core/json.h"
#include <cstring>

using namespace std;
using namespace dballe;

namespace dballe {
namespace db {
namespace summary {

Entry::Entry(dballe::db::CursorSummary& cur)
{
    station = cur.get_station();
    level = cur.get_level();
    trange = cur.get_trange();
    varcode = cur.get_varcode();
    count = cur.get_count();
    dtrange = cur.get_datetimerange();
}

void Entry::to_json(core::JSONWriter& writer)
{
    writer.start_mapping();
    writer.add("s");
    writer.start_mapping();
    writer.add("r", station.report);
    writer.add("c", station.coords);
    writer.add("i", station.ident);
    writer.end_mapping();
    writer.add("l", level);
    writer.add("t", trange);
    writer.add("v", varcode);
    writer.add("d", dtrange);
    writer.add("c", count);
    writer.end_mapping();
}

static Station station_from_json(core::json::Stream& in)
{
    Station res;
    in.parse_object([&](const std::string& key) {
        if (key == "r")
            res.report = in.parse_string();
        else if (key == "c")
            res.coords = in.parse_coords();
        else if (key == "i")
            res.ident = in.parse_ident();
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for Station");
    });
    return res;
}

Entry Entry::from_json(core::json::Stream& in)
{
    Entry res;
    in.parse_object([&](const std::string& key) {
        if (key == "s")
            res.station = station_from_json(in);
        else if (key == "l")
            res.level = in.parse_level();
        else if (key == "t")
            res.trange = in.parse_trange();
        else if (key == "v")
            res.varcode = in.parse_unsigned<unsigned short>();
        else if (key == "d")
            res.dtrange = in.parse_datetime_range();
        else if (key == "c")
            res.count = in.parse_unsigned<size_t>();
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for summary::Entry");
    });
    return res;
}

std::ostream& operator<<(std::ostream& out, const Entry& e)
{
    return out <<  "s:" << e.station
               << " l:" << e.level
               << " t:" << e.trange
               << " v:" << e.varcode
               << " d:" << e.dtrange
               << " c:" << e.count;
}

#if 0
    struct JSONMsgReader : public core::JSONReader {
        std::istream* in;
        bool close_on_exit;

        Msg msg;
        std::unique_ptr<msg::Context> ctx;
        std::unique_ptr<wreport::Var> var;
        std::unique_ptr<wreport::Var> attr;

        enum State {
            MSG,
            MSG_IDENT_KEY,
            MSG_VERSION_KEY,
            MSG_NETWORK_KEY,
            MSG_LON_KEY,
            MSG_LAT_KEY,
            MSG_DATE_KEY,
            MSG_DATA_KEY,
            MSG_DATA_LIST,
            MSG_DATA_LIST_ITEM,
            MSG_DATA_LIST_ITEM_VARS_KEY,
            MSG_DATA_LIST_ITEM_LEVEL_KEY,
            MSG_DATA_LIST_ITEM_TRANGE_KEY,
            MSG_DATA_LIST_ITEM_VARS_MAPPING,
            MSG_DATA_LIST_ITEM_VARS_MAPPING_KEY,
            MSG_DATA_LIST_ITEM_LEVEL_LIST,
            MSG_DATA_LIST_ITEM_TRANGE_LIST,
            MSG_DATA_LIST_ITEM_LEVEL_LIST_LTYPE1,
            MSG_DATA_LIST_ITEM_LEVEL_LIST_L1,
            MSG_DATA_LIST_ITEM_LEVEL_LIST_LTYPE2,
            MSG_DATA_LIST_ITEM_LEVEL_LIST_L2,
            MSG_DATA_LIST_ITEM_TRANGE_LIST_PIND,
            MSG_DATA_LIST_ITEM_TRANGE_LIST_P1,
            MSG_DATA_LIST_ITEM_TRANGE_LIST_P2,
            MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY,
            MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR,
            MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_MAPPING,
            MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_KEY,
            MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING,
            MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY,
            MSG_END,
        };

        std::stack<State> state;

        JSONMsgReader(std::istream& in) : in(&in), close_on_exit(false) {}
        JSONMsgReader(const std::string& name) : close_on_exit(true) {
            in = new ifstream(name);
            if (not in->good())
                throw runtime_error(strerror(errno));
        }
        ~JSONMsgReader() {
            if (close_on_exit)
                delete in;
        }

        void parse_msgs(std::function<void(const Msg&)> cb) {
            if (in) {
                while (!in->eof())
                {
                    parse(*in);
                    if (not state.empty() && state.top() == MSG_END) {
                        state.pop();
                        cb(msg);
                    }
                    msg.clear();
                }
            }
            if (not state.empty())
                throw std::runtime_error("Incomplete JSON");
        }

        void throw_error_if_empty_state() {
            if (state.empty())
                throw std::runtime_error("Invalid JSON value");
        }

        virtual void on_start_list() {
            throw_error_if_empty_state();
            State s = state.top();
            switch (s) {
                case MSG_DATA_KEY:
                    state.pop();
                    state.push(MSG_DATA_LIST);
                    break;
                case MSG_DATA_LIST_ITEM_LEVEL_KEY:
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_LEVEL_LIST);
                    state.push(MSG_DATA_LIST_ITEM_LEVEL_LIST_LTYPE1);
                    break;
                case MSG_DATA_LIST_ITEM_TRANGE_KEY:
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_TRANGE_LIST);
                    state.push(MSG_DATA_LIST_ITEM_TRANGE_LIST_PIND);
                    break;
                default: throw std::runtime_error("Invalid JSON value start_list");
            }
        }
        virtual void on_end_list() {
            throw_error_if_empty_state();
            State s = state.top();
            switch (s) {
                case MSG_DATA_LIST: state.pop(); break;
                case MSG_DATA_LIST_ITEM_LEVEL_LIST: state.pop(); break;
                case MSG_DATA_LIST_ITEM_TRANGE_LIST: state.pop(); break;
                default: throw std::runtime_error("Invalid JSON value end_list");
            }
        }
        virtual void on_start_mapping() {
            if (state.empty())
            {
                msg.clear();
                state.push(MSG);
            }
            else
            {
                State s = state.top();
                switch (s) {
                    case MSG_DATA_LIST:
                        state.push(MSG_DATA_LIST_ITEM);
                        ctx.reset(new msg::Context(Level(), Trange()));
                        break;
                    case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR:
                        state.pop();
                        state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_MAPPING);
                        break;
                    case MSG_DATA_LIST_ITEM_VARS_KEY:
                        state.pop();
                        state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING);
                        break;
                    case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_KEY:
                        state.pop();
                        state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING);
                        break;
                    default: throw std::runtime_error("Invalid JSON value start_mapping");
                }
            }
        }
        virtual void on_end_mapping() {
            throw_error_if_empty_state();
            State s = state.top();
            switch (s) {
                case MSG:
                {
                    state.pop();
                    state.push(MSG_END);
                    break;
                }
                case MSG_DATA_LIST_ITEM:
                {
                    // NOTE: station context could be already created, because
                    // of "lon", "lat", "ident", "network".
                    // Then, context overwrite is allowed.
                    // msg.add_context(std::move(ctx));
                    msg::Context& ctx2 = msg.obtain_context(ctx->level, ctx->trange);
                    for (const auto& ci: ctx->data)
                        ctx2.set(*ci);
                    state.pop();
                    break;
                }
                case MSG_DATA_LIST_ITEM_VARS_MAPPING:
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING:
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_MAPPING:
                    state.pop();
                    break;
                default: throw std::runtime_error("Invalid JSON value end_mapping");
            }
        }
        virtual void on_add_null() {
            throw_error_if_empty_state();
            State s = state.top();
            switch (s) {
                case MSG_IDENT_KEY:
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_LEVEL_LIST_LTYPE1:
                    ctx->level.ltype1 = MISSING_INT;
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_LEVEL_LIST_L1);
                    break;
                case MSG_DATA_LIST_ITEM_LEVEL_LIST_L1:
                    ctx->level.l1 = MISSING_INT;
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_LEVEL_LIST_LTYPE2);
                    break;
                case MSG_DATA_LIST_ITEM_LEVEL_LIST_LTYPE2:
                    ctx->level.ltype2 = MISSING_INT;
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_LEVEL_LIST_L2);
                    break;
                case MSG_DATA_LIST_ITEM_LEVEL_LIST_L2:
                    ctx->level.l2 = MISSING_INT;
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_TRANGE_LIST_PIND:
                    ctx->trange.pind = MISSING_INT;
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_TRANGE_LIST_P1);
                    break;
                case MSG_DATA_LIST_ITEM_TRANGE_LIST_P1:
                    ctx->trange.p1 = MISSING_INT;
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_TRANGE_LIST_P2);
                    break;
                case MSG_DATA_LIST_ITEM_TRANGE_LIST_P2:
                    ctx->trange.p2 = MISSING_INT;
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY:
                    var->unset();
                    ctx->set(*var);
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY:
                    state.pop();
                    attr->set(MISSING_INT);
                    var->seta(*attr);
                    break;
                default: throw std::runtime_error("Invalid JSON value add_null");
            }
        }
        virtual void on_add_bool(bool val) {
            throw_error_if_empty_state();
            State s = state.top();
            switch (s) {
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY:
                    var->set(val);
                    ctx->set(*var);
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY:
                    state.pop();
                    attr->set(val);
                    var->seta(*attr);
                    break;
                default: throw std::runtime_error("Invalid JSON value add_bool");
            }
        }
        virtual void on_add_int(int val) {
            throw_error_if_empty_state();
            State s = state.top();
            switch (s) {
                case MSG_LON_KEY:
                    msg.set_longitude_var(dballe::var("B06001", val));
                    state.pop();
                    break;
                case MSG_LAT_KEY:
                    msg.set_latitude_var(dballe::var("B05001", val));
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_LEVEL_LIST_LTYPE1:
                    ctx->level.ltype1 = val;
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_LEVEL_LIST_L1);
                    break;
                case MSG_DATA_LIST_ITEM_LEVEL_LIST_L1:
                    ctx->level.l1 = val;
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_LEVEL_LIST_LTYPE2);
                    break;
                case MSG_DATA_LIST_ITEM_LEVEL_LIST_LTYPE2:
                    ctx->level.ltype2 = val;
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_LEVEL_LIST_L2);
                    break;
                case MSG_DATA_LIST_ITEM_LEVEL_LIST_L2:
                    ctx->level.l2 = val;
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_TRANGE_LIST_PIND:
                    ctx->trange.pind = val;
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_TRANGE_LIST_P1);
                    break;
                case MSG_DATA_LIST_ITEM_TRANGE_LIST_P1:
                    ctx->trange.p1 = val;
                    state.pop();
                    state.push(MSG_DATA_LIST_ITEM_TRANGE_LIST_P2);
                    break;
                case MSG_DATA_LIST_ITEM_TRANGE_LIST_P2:
                    ctx->trange.p2 = val;
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY:
                    // Var::seti on decimal vars is considered as the value
                    // with the scale already applied
                    var->setf(to_string(val).c_str());
                    ctx->set(*var);
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY:
                    state.pop();
                    attr->set(val);
                    var->seta(*attr);
                    break;
                default: throw std::runtime_error("Invalid JSON value add_int");
            }
        }
        virtual void on_add_double(double val) {
            throw_error_if_empty_state();
            State s = state.top();
            switch (s) {
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY:
                    var->set(val);
                    ctx->set(*var);
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY:
                    state.pop();
                    attr->set(val);
                    var->seta(*attr);
                    break;
                default: throw std::runtime_error("Invalid JSON value add_double");
            }
        }
        virtual void on_add_string(const std::string& val) {
            throw_error_if_empty_state();
            State s = state.top();
            switch (s) {
                case MSG:
                    if (val == "ident")
                        state.push(MSG_IDENT_KEY);
                    else if (val == "version")
                        state.push(MSG_VERSION_KEY);
                    else if (val == "network")
                        state.push(MSG_NETWORK_KEY);
                    else if (val == "lon")
                        state.push(MSG_LON_KEY);
                    else if (val == "lat")
                        state.push(MSG_LAT_KEY);
                    else if (val == "date")
                        state.push(MSG_DATE_KEY);
                    else if (val == "data")
                        state.push(MSG_DATA_KEY);
                    else
                        throw std::runtime_error("Invalid JSON value");
                    break;
                case MSG_IDENT_KEY:
                    msg.set_ident(val.c_str());
                    state.pop();
                    break;
                case MSG_VERSION_KEY:
                    if (strcmp(val.c_str(), DBALLE_JSON_VERSION) != 0)
                        throw std::runtime_error("Invalid JSON version " + val);
                    state.pop();
                    break;
                case MSG_NETWORK_KEY:
                    msg.set_rep_memo(val.c_str());
                    state.pop();
                    break;
                case MSG_DATE_KEY:
                    msg.set_datetime(Datetime::from_iso8601(val.c_str()));
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM:
                    if (val == "vars")
                        state.push(MSG_DATA_LIST_ITEM_VARS_KEY);
                    else if (val == "level")
                        state.push(MSG_DATA_LIST_ITEM_LEVEL_KEY);
                    else if (val == "timerange")
                        state.push(MSG_DATA_LIST_ITEM_TRANGE_KEY);
                    else
                        throw std::runtime_error("Invalid JSON value");
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING:
                    state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR);
                    var.reset(new Var(dballe::var(val)));
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_MAPPING:
                    if (val == "v")
                        state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY);
                    else if (val == "a")
                        state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_KEY);
                    else
                        throw std::runtime_error("Invalid JSON value");
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY:
                    var->set(val);
                    ctx->set(*var);
                    state.pop();
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING:
                    state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY);
                    attr.reset(new Var(dballe::var(val)));
                    break;
                case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY:
                    state.pop();
                    attr->set(val);
                    var->seta(*attr);
                    break;
                default: throw std::runtime_error("Invalid JSON value add_string");
            }
        }
    };
#endif

}

Summary::Summary(const Query& query)
    : query(core::Query::downcast(query))
{
}

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

void Summary::aggregate(const summary::Entry &val)
{
    all_stations.insert(make_pair(val.station.id, val.station));
    all_reports.insert(val.station.report);
    all_levels.insert(val.level);
    all_tranges.insert(val.trange);
    all_varcodes.insert(val.varcode);

    if (val.count != MISSING_INT)
    {
        if (count == MISSING_INT)
        {
            dtrange = val.dtrange;
            count = val.count;
        } else {
            dtrange.merge(val.dtrange);
            count += val.count;
        }
    }

    valid = true;
}

void Summary::add_filtered(const Summary& summary)
{
    switch (summary.supports(query))
    {
        case summary::Support::UNSUPPORTED:
            throw std::runtime_error("Source summary does not support the query for this summary");
        case summary::Support::OVERESTIMATED:
        case summary::Support::EXACT:
            break;
    }

    const core::Query& q = core::Query::downcast(query);

    // Scan the filter building a todo list of things to match

    // If there is any filtering on the station, build a whitelist of matching stations
    bool has_flt_rep_memo = !query.rep_memo.empty();
    std::set<int> wanted_stations;
    bool has_flt_ident = !q.ident.is_missing();
    bool has_flt_area = !q.get_latrange().is_missing() || !q.get_lonrange().is_missing();
    bool has_flt_station = has_flt_ident || has_flt_area || q.ana_id != MISSING_INT;
    if (has_flt_station)
    {
        LatRange flt_area_latrange = q.get_latrange();
        LonRange flt_area_lonrange = q.get_lonrange();
        for (auto s: summary.all_stations)
        {
            const Station& station = s.second;
            if (q.ana_id != MISSING_INT && station.id != q.ana_id)
                continue;

            if (has_flt_area)
            {
                if (!flt_area_latrange.contains(station.coords.lat) ||
                    !flt_area_lonrange.contains(station.coords.lon))
                    continue;
            }

            if (has_flt_rep_memo && query.rep_memo != station.report)
                continue;

            if (has_flt_ident && query.ident != station.ident)
                continue;

            wanted_stations.insert(station.id);
        }
    }

    bool has_flt_level = !query.level.is_missing();
    bool has_flt_trange = !query.trange.is_missing();
    bool has_flt_varcode = !query.varcodes.empty();
    wreport::Varcode wanted_varcode = has_flt_varcode ? *query.varcodes.begin() : 0;
    DatetimeRange wanted_dtrange = query.get_datetimerange();

    for (const auto& entry: summary.entries)
    {
        if (has_flt_station)
        {
            if (wanted_stations.find(entry.station.id) == wanted_stations.end())
                continue;
        } else if (has_flt_rep_memo && query.rep_memo != entry.station.report)
            continue;

        if (has_flt_level && query.level != entry.level)
            continue;

        if (has_flt_trange && query.trange != entry.trange)
            continue;

        if (has_flt_varcode && wanted_varcode != entry.varcode)
            continue;

        if (!wanted_dtrange.contains(entry.dtrange))
            continue;

        add_entry(entry);
    }
}

void Summary::add_summary(dballe::db::CursorSummary& cur)
{
    entries.emplace_back(cur);
    aggregate(entries.back());
}

void Summary::add_entry(const summary::Entry &entry)
{
    entries.push_back(entry);
    aggregate(entries.back());
}

bool Summary::iterate(std::function<bool(const summary::Entry&)> f) const
{
    for (auto i: entries)
        if (!f(i))
            return false;
    return true;
}

}
}
