#include "json_codec.h"
#include "dballe/core/json.h"
#include "dballe/file.h"
#include "dballe/msg/msg.h"
#include <wreport/error.h>
#include <wreport/options.h>
#include <sstream>
#include <stack>

namespace dballe {
namespace impl {
namespace msg {

using core::JSONParseException;


struct JSONMsgReader : public core::JSONReader
{
    std::unique_ptr<impl::Message> msg;
    std::unique_ptr<impl::msg::Context> ctx;
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

    JSONMsgReader() {}

    bool parse_msgs(const std::string& buf, std::function<bool(std::unique_ptr<impl::Message>)> cb)
    {
        std::stringstream in(buf);
        while (!in.eof())
        {
            parse(in);
            if (not state.empty() && state.top() == MSG_END) {
                state.pop();
                if (!cb(std::move(msg)))
                    return false;
            }
            msg.reset();
        }
        if (not state.empty())
            throw JSONParseException("Incomplete JSON");
        return true;
    }

    void throw_error_if_empty_state()
    {
        if (state.empty())
            throw JSONParseException("Invalid JSON value");
    }

    void on_start_list() override
    {
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
            default: throw JSONParseException("Invalid JSON value start_list");
        }
    }

    void on_end_list() override
    {
        throw_error_if_empty_state();
        State s = state.top();
        switch (s) {
            case MSG_DATA_LIST: state.pop(); break;
            case MSG_DATA_LIST_ITEM_LEVEL_LIST: state.pop(); break;
            case MSG_DATA_LIST_ITEM_TRANGE_LIST: state.pop(); break;
            default: throw JSONParseException("Invalid JSON value end_list");
        }
    }

    void on_start_mapping() override
    {
        if (state.empty())
        {
            msg.reset(new impl::Message);
            state.push(MSG);
        }
        else
        {
            State s = state.top();
            switch (s) {
                case MSG_DATA_LIST:
                    state.push(MSG_DATA_LIST_ITEM);
                    ctx.reset(new impl::msg::Context(Level(), Trange()));
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
                default: throw JSONParseException("Invalid JSON value start_mapping");
            }
        }
    }

    void on_end_mapping() override
    {
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
                if (ctx->level.is_missing() && ctx->trange.is_missing())
                {
                    msg->station_data.merge(ctx->values);
                } else {
                    impl::msg::Context& ctx2 = msg->obtain_context(ctx->level, ctx->trange);
                    ctx2.values.merge(ctx->values);
                }
                state.pop();
                break;
            }
            case MSG_DATA_LIST_ITEM_VARS_MAPPING:
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING:
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_MAPPING:
                state.pop();
                break;
            default: throw JSONParseException("Invalid JSON value end_mapping");
        }
    }

    void on_add_null() override
    {
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
                ctx->values.set(*var);
                state.pop();
                break;
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY:
                state.pop();
                attr->set(MISSING_INT);
                var->seta(*attr);
                ctx->values.set(*var);
                break;
            default: throw JSONParseException("Invalid JSON value add_null");
        }
    }

    void on_add_bool(bool val) override
    {
        throw_error_if_empty_state();
        State s = state.top();
        switch (s) {
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY:
                var->set(val);
                ctx->values.set(*var);
                state.pop();
                break;
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY:
                state.pop();
                attr->set(val);
                var->seta(*attr);
                ctx->values.set(*var);
                break;
            default: throw JSONParseException("Invalid JSON value add_bool");
        }
    }

    void on_add_int(int val) override
    {
        throw_error_if_empty_state();
        State s = state.top();
        switch (s) {
            case MSG_LON_KEY:
                msg->set_longitude_var(dballe::var("B06001", val));
                state.pop();
                break;
            case MSG_LAT_KEY:
                msg->set_latitude_var(dballe::var("B05001", val));
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
                var->setf(std::to_string(val).c_str());
                ctx->values.set(*var);
                state.pop();
                break;
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY:
                state.pop();
                attr->set(val);
                var->seta(*attr);
                ctx->values.set(*var);
                break;
            default: throw JSONParseException("Invalid JSON value add_int");
        }
    }

    void on_add_double(double val) override
    {
        throw_error_if_empty_state();
        State s = state.top();
        switch (s) {
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY:
                var->set(val);
                ctx->values.set(*var);
                state.pop();
                break;
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY:
                state.pop();
                attr->set(val);
                var->seta(*attr);
                ctx->values.set(*var);
                break;
            default: throw JSONParseException("Invalid JSON value add_double");
        }
    }

    void on_add_string(const std::string& val) override
    {
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
                    throw JSONParseException("Invalid JSON value");
                break;
            case MSG_IDENT_KEY:
                msg->set_ident(val.c_str());
                state.pop();
                break;
            case MSG_VERSION_KEY:
                if (val != DBALLE_JSON_VERSION)
                    throw JSONParseException("Invalid JSON version " + val);
                state.pop();
                break;
            case MSG_NETWORK_KEY:
                msg->set_rep_memo(val.c_str());
                state.pop();
                break;
            case MSG_DATE_KEY:
                msg->set_datetime(Datetime::from_iso8601(val.c_str()));
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
                    throw JSONParseException("Invalid JSON value");
                break;
            case MSG_DATA_LIST_ITEM_VARS_MAPPING:
                state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR);
                var = newvar(val);
                break;
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_MAPPING:
                if (val == "v")
                    state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY);
                else if (val == "a")
                    state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_KEY);
                else
                    throw JSONParseException("Invalid JSON value");
                break;
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_VAR_KEY:
                var->set(val);
                ctx->values.set(*var);
                state.pop();
                break;
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING:
                state.push(MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY);
                attr = newvar(val);
                break;
            case MSG_DATA_LIST_ITEM_VARS_MAPPING_ATTR_MAPPING_VAR_KEY:
                state.pop();
                attr->set(val);
                var->seta(*attr);
                ctx->values.set(*var);
                break;
            default: throw JSONParseException("Invalid JSON value add_string");
        }
    }
};


JsonImporter::JsonImporter(const dballe::ImporterOptions& opts)
    : Importer(opts) {}

JsonImporter::~JsonImporter() {}

bool JsonImporter::foreach_decoded(const BinaryMessage& msg, std::function<bool(std::unique_ptr<dballe::Message>)> dest) const
{
    auto lo1(wreport::options::local_override(wreport::options::var_silent_domain_errors, opts.domain_errors == ImporterOptions::DomainErrors::UNSET));
#ifdef WREPORT_OPTIONS_HAS_VAR_CLAMP_DOMAIN_ERRORS
    auto lo2(wreport::options::local_override(wreport::options::var_clamp_domain_errors, opts.domain_errors == ImporterOptions::DomainErrors::CLAMP));
#endif

    JSONMsgReader jsonreader;
    return jsonreader.parse_msgs(msg.data, dest);
}


JsonExporter::JsonExporter(const dballe::ExporterOptions& opts)
    : Exporter(opts) {}

JsonExporter::~JsonExporter() {}

std::string JsonExporter::to_binary(const std::vector<std::shared_ptr<dballe::Message>>& msgs) const
{
    std::stringstream buf;
    core::JSONWriter json(buf);

    for (const auto& mi: msgs) {
        auto msg = impl::Message::downcast(mi);
        json.start_mapping();
        json.add("version");
        json.add(DBALLE_JSON_VERSION);
        json.add("network");
        json.add(msg->get_rep_memo_var() ? msg->get_rep_memo_var()->enqc() : dballe::impl::Message::repmemo_from_type(msg->type));
        json.add("ident");
        if (msg->get_ident_var() != NULL)
            json.add(msg->get_ident_var()->enqc());
        else
            json.add_null();
        json.add("lon");
        json.add_int(msg->get_longitude_var()->enqi());
        json.add("lat");
        json.add_int(msg->get_latitude_var()->enqi());
        json.add("date");
        std::stringstream ss;
        msg->get_datetime().to_stream_iso8601(ss, 'T', "Z");
        json.add(ss.str().c_str());
        json.add("data");
        json.start_list();
            json.start_mapping();
            json.add("vars");
            json.add(msg->station_data);
            json.end_mapping();
            for (const auto& ctx: msg->data) {
                json.start_mapping();
                json.add("timerange");
                json.add(ctx.trange);
                json.add("level");
                json.add(ctx.level);
                json.add("vars");
                json.add(ctx.values);
                json.end_mapping();
            }
            json.end_list();
        json.end_mapping();
        json.add_break();
    }

    return buf.str();
}

}
}
}
