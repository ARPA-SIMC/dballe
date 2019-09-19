#include "processor.h"
#include <wreport/bulletin.h>
#include <wreport/utils/string.h>
#include "dballe/file.h"
#include "dballe/message.h"
#include "dballe/msg/context.h"
#include "dballe/msg/msg.h"
#include "dballe/core/csv.h"
#include "dballe/core/json.h"
#include "dballe/core/match-wreport.h"
#include "dballe/cmdline/cmdline.h"
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <limits>

using namespace wreport;
using namespace std;

// extern int op_verbose;

namespace dballe {
namespace cmdline {

void ProcessingException::initmsg(const std::string& fname, unsigned index, const char* msg)
{
    char buf[512];
    snprintf(buf, 512, "%s:#%u: %s", fname.c_str(), index, msg);
    this->msg = buf;
}

static void print_parse_error(const BinaryMessage& msg, error& e)
{
    fprintf(stderr, "Cannot parse %s message #%d: %s at offset %jd.\n",
            File::encoding_name(msg.encoding), msg.index, e.what(), (intmax_t)msg.offset);
}


Item::Item()
    : idx(0), rmsg(0), bulletin(0), msgs(0)
{
}

Item::~Item()
{
    if (msgs) delete msgs;
    if (bulletin) delete bulletin;
    if (rmsg) delete rmsg;
}

void Item::set_msgs(std::vector<std::shared_ptr<dballe::Message>>* new_msgs)
{
    if (msgs) delete msgs;
    msgs = new_msgs;
}

void Item::decode(Importer& imp, bool print_errors)
{
    if (!rmsg) return;

    if (bulletin)
    {
        delete bulletin;
        bulletin = 0;
    }

    if (msgs)
    {
        delete msgs;
        msgs = 0;
    }

    // First step: decode raw message to bulletin
    switch (rmsg->encoding)
    {
        case Encoding::BUFR:
            try {
                bulletin = BufrBulletin::decode(rmsg->data, rmsg->pathname.c_str(), rmsg->offset).release();
            } catch (error& e) {
                if (print_errors) print_parse_error(*rmsg, e);
                delete bulletin;
                bulletin = 0;
            }
            break;
        case Encoding::CREX:
            try {
                bulletin = CrexBulletin::decode(rmsg->data, rmsg->pathname.c_str(), rmsg->offset).release();
            } catch (error& e) {
                if (print_errors) print_parse_error(*rmsg, e);
                delete bulletin;
                bulletin = 0;
            }
            break;
    }

    // Second step: decode to msgs
    switch (rmsg->encoding)
    {
        case Encoding::BUFR:
        case Encoding::CREX:
            if (bulletin)
            {
                msgs = new std::vector<std::shared_ptr<dballe::Message>>;
                try {
                    *msgs = imp.from_bulletin(*bulletin);
                } catch (error& e) {
                    if (print_errors) print_parse_error(*rmsg, e);
                    delete msgs;
                    msgs = 0;
                }
            }
            break;
    }
}

void Item::processing_failed(std::exception& e) const
{
    throw ProcessingException(rmsg ? rmsg->pathname : "(unknown)", idx, e);
}

void IndexMatcher::parse(const std::string& str)
{
    ranges.clear();
    str::Split parts(str, ",", true);
    for (const auto& s: parts)
    {
        size_t pos = s.find('-');
        if (pos == 0)
            ranges.push_back(make_pair(0, std::stoi(s.substr(pos + 1))));
        else if (pos == s.size() - 1)
            ranges.push_back(make_pair(std::stoi(s.substr(0, pos)), std::numeric_limits<int>::max()));
        else if (pos == string::npos)
        {
            int val = std::stoi(s);
            ranges.push_back(make_pair(val, val));
        }
        else
            ranges.push_back(make_pair(std::stoi(s.substr(0, pos)), std::stoi(s.substr(pos + 1))));
    }
}

bool IndexMatcher::match(int val) const
{
    if (ranges.empty()) return true;

    for (const auto& range: ranges)
        if (val >= range.first && val <= range.second)
            return true;
    return false;
}


Filter::Filter() {}
Filter::Filter(const ReaderOptions& opts)
    : category(opts.category),
      subcategory(opts.subcategory),
      checkdigit(opts.checkdigit),
      unparsable(opts.unparsable),
      parsable(opts.parsable)
{
    if (opts.index_filter) imatcher.parse(opts.index_filter);
}

Filter::~Filter()
{
    delete matcher;
}

void Filter::set_index_filter(const std::string& val)
{
    imatcher.parse(val);
}

void Filter::matcher_reset()
{
    if (matcher)
    {
        delete matcher;
        matcher = 0;
    }
}

void Filter::matcher_from_record(const Query& query)
{
    if (matcher)
    {
        delete matcher;
        matcher = 0;
    }
    matcher = Matcher::create(query).release();
}

bool Filter::match_index(int idx) const
{
    return imatcher.match(idx);
}

bool Filter::match_common(const BinaryMessage&, const std::vector<std::shared_ptr<dballe::Message>>* msgs) const
{
    if (msgs == NULL && parsable)
        return false;
    if (msgs != NULL && unparsable)
        return false;
    return true;
}

bool Filter::match_bufrex(const BinaryMessage& rmsg, const Bulletin* rm, const std::vector<std::shared_ptr<dballe::Message>>* msgs) const
{
    if (!match_common(rmsg, msgs))
        return false;

    if (category != -1)
        if (category != rm->data_category)
            return false;

    if (subcategory != -1)
        if (subcategory != rm->data_subcategory)
            return false;

    if (matcher)
    {
        if (msgs)
        {
            if (!match_msgs(*msgs))
                return false;
        } else if (rm) {
            if (matcher->match(MatchedBulletin(*rm)) != matcher::MATCH_YES)
                return false;
        }
    }

    return true;
}

bool Filter::match_bufr(const BinaryMessage& rmsg, const Bulletin* rm, const std::vector<std::shared_ptr<dballe::Message>>* msgs) const
{
    if (!match_bufrex(rmsg, rm, msgs))
        return false;
    return true;
}

bool Filter::match_crex(const BinaryMessage& rmsg, const Bulletin* rm, const std::vector<std::shared_ptr<dballe::Message>>* msgs) const
{
    if (!match_bufrex(rmsg, rm, msgs))
        return false;
    return true;

#if 0
    if (grepdata->checkdigit != -1)
    {
        int checkdigit;
        DBA_RUN_OR_RETURN(crex_message_has_check_digit(msg, &checkdigit));
        if (grepdata->checkdigit != checkdigit)
        {
            *match = 0;
            return dba_error_ok();
        }
    }
#endif
}

bool Filter::match_msgs(const std::vector<std::shared_ptr<dballe::Message>>& msgs) const
{
    if (matcher && matcher->match(impl::MatchedMessages(msgs)) != matcher::MATCH_YES)
        return false;

    return true;
}

bool Filter::match_item(const Item& item) const
{
    if (item.rmsg)
    {
        switch (item.rmsg->encoding)
        {
            case Encoding::BUFR: return match_bufr(*item.rmsg, item.bulletin, item.msgs);
            case Encoding::CREX: return match_crex(*item.rmsg, item.bulletin, item.msgs);
            default: return false;
        }
    } else if (item.msgs)
        return match_msgs(*item.msgs);
    else
        return false;
}

Reader::Reader(const ReaderOptions& opts)
    : input_type(opts.input_type), fail_file_name(opts.fail_file_name), filter(opts)
{
}

bool Reader::has_fail_file() const
{
    return fail_file_name != nullptr;
}

void Reader::read_csv(const std::list<std::string>& fnames, Action& action)
{
    // This cannot be implemented in dballe::File at the moment, since
    // dballe::File reads dballe::BinaryMessage strings, and here we read dballe::Messages
    // directly. We could split the input into several BinaryMessage strings, but that
    // would mean parsing the CSV twice: once to detect the message boundaries
    // and once to parse the BinaryMessage strings.
    Item item;
    unique_ptr<CSVReader> csvin;

    list<string>::const_iterator name = fnames.begin();
    do
    {
        if (name != fnames.end())
        {
            csvin.reset(new CSVReader(*name));
            ++name;
        } else {
            // name = "(stdin)";
            csvin.reset(new CSVReader(cin));
        }

        while (true)
        {
            // Read input message
            unique_ptr<impl::Message> msg(new impl::Message);
            if (!msg->from_csv(*csvin))
                break;

            // Match against index matcher
            ++item.idx;
            if (!filter.match_index(item.idx))
                continue;

            // We want it: move it to the item
            unique_ptr<impl::Messages> msgs(new impl::Messages);
            msgs->emplace_back(move(msg));
            item.set_msgs(msgs.release());

            if (!filter.match_item(item))
                continue;

            action(item);
        }
    } while (name != fnames.end());
}

void Reader::read_json(const std::list<std::string>& fnames, Action& action)
{
    using core::JSONParseException;

    struct JSONMsgReader : public core::JSONReader {
        std::istream* in;
        bool close_on_exit;

        impl::Message msg;
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

        void parse_msgs(std::function<void(const impl::Message&)> cb) {
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
                throw JSONParseException("Incomplete JSON");
        }

        void throw_error_if_empty_state() {
            if (state.empty())
                throw JSONParseException("Invalid JSON value");
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
                default: throw JSONParseException("Invalid JSON value start_list");
            }
        }
        virtual void on_end_list() {
            throw_error_if_empty_state();
            State s = state.top();
            switch (s) {
                case MSG_DATA_LIST: state.pop(); break;
                case MSG_DATA_LIST_ITEM_LEVEL_LIST: state.pop(); break;
                case MSG_DATA_LIST_ITEM_TRANGE_LIST: state.pop(); break;
                default: throw JSONParseException("Invalid JSON value end_list");
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
                    if (ctx->level.is_missing() && ctx->trange.is_missing())
                    {
                        msg.station_data.merge(ctx->values);
                    } else {
                        impl::msg::Context& ctx2 = msg.obtain_context(ctx->level, ctx->trange);
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
        virtual void on_add_bool(bool val) {
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
        virtual void on_add_double(double val) {
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
                        throw JSONParseException("Invalid JSON value");
                    break;
                case MSG_IDENT_KEY:
                    msg.set_ident(val.c_str());
                    state.pop();
                    break;
                case MSG_VERSION_KEY:
                    if (strcmp(val.c_str(), DBALLE_JSON_VERSION) != 0)
                        throw JSONParseException("Invalid JSON version " + val);
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
    Item item;
    std::unique_ptr<JSONMsgReader> jsonreader;

    list<string>::const_iterator name = fnames.begin();
    try {
        do {
            if (name != fnames.end()) {
                jsonreader.reset(new JSONMsgReader(*name));
                ++name;
            } else {
                jsonreader.reset(new JSONMsgReader(cin));
            }
            jsonreader->parse_msgs([&](const impl::Message& msg) {
                ++item.idx;
                if (!filter.match_index(item.idx))
                    return;
                unique_ptr<impl::Messages> msgs(new impl::Messages);
                msgs->emplace_back(make_shared<impl::Message>(msg));
                item.set_msgs(msgs.release());

                if (!filter.match_item(item))
                    return;

                action(item);
            });
        } while (name != fnames.end());
    } catch (const JSONParseException& e) {
        // If name points to begin(), then it's the standard input, because
        // after parsing a file the iterator is incremented.
        const std::string f = ( name != fnames.begin() ? *(--name) : "stdin" );
        throw JSONParseException("Error while parsing JSON from " + f +
                                 " at char " + std::to_string(jsonreader->in->tellg()) +
                                 ": " + e.what());
    }
}

void Reader::read_file(const std::list<std::string>& fnames, Action& action)
{
    bool print_errors = !filter.unparsable;
    std::unique_ptr<File> fail_file;

    list<string>::const_iterator name = fnames.begin();
    do
    {
        unique_ptr<File> file;

        if (input_type == "auto")
        {
            if (name != fnames.end())
            {
                file = File::create(*name, "r");
                ++name;
            } else {
                file = File::create(stdin, false, "standard input");
            }
        } else {
            Encoding intype = string_to_encoding(input_type.c_str());
            if (name != fnames.end())
            {
                file = File::create(intype, *name, "r");
                ++name;
            } else {
                file = File::create(intype, stdin, false, "standard input");
            }
        }


        std::unique_ptr<Importer> imp = Importer::create(file->encoding(), import_opts);
        while (BinaryMessage bm = file->read())
        {
            Item item;
            item.rmsg = new BinaryMessage(bm);
            item.idx = bm.index;
            bool processed = false;

            try {
    //          if (op_verbose)
    //              fprintf(stderr, "Reading message #%d...\n", item.index);

                if (!filter.match_index(item.idx))
                    continue;

                try {
                    item.decode(*imp, print_errors);
                } catch (std::exception& e) {
                    // Convert decode errors into ProcessingException, to skip
                    // this item if it fails to decode. We can safely skip,
                    // because if file->read() returned successfully the next
                    // read should properly start at the next item
                    item.processing_failed(e);
                }

                //process_input(*file, rmsg, grepdata, action);

                if (!filter.match_item(item))
                    continue;

                processed = action(item);
            } catch (ProcessingException& pe) {
                // If ProcessingException has been raised, we can safely skip
                // to the next input
                processed = false;
                if (verbose)
                    fprintf(stderr, "%s\n", pe.what());
            } catch (std::exception& e) {
                if (verbose)
                    fprintf(stderr, "%s:#%d: %s\n", file->pathname().c_str(), item.idx, e.what());
                throw;
            }

            // Output items that have not been processed successfully
            if (!processed && fail_file_name)
            {
                if (!fail_file.get())
                    fail_file = File::create(file->encoding(), fail_file_name, "ab");
                fail_file->write(item.rmsg->data);
            }
            if (processed)
                ++count_successes;
            else
                ++count_failures;
        }
    } while (name != fnames.end());
}

void Reader::read(const std::list<std::string>& fnames, Action& action)
{
    if (input_type == "csv")
        read_csv(fnames, action);
    else if (input_type == "json")
        read_json(fnames, action);
    else
        read_file(fnames, action);
}

}
}
