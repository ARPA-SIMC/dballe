#include "msgapi.h"
#include <wreport/var.h>
#include "dballe/file.h"
#include "dballe/importer.h"
#include "dballe/exporter.h"
#include "dballe/message.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/cursor.h"
#include "dballe/msg/context.h"
#include "dballe/core/var.h"
#include <cstring>
#include <cassert>

using namespace wreport;
using namespace std;

namespace dballe {
namespace fortran {

namespace {

struct QuantesonoOperation : public CursorOperation<CursorStation>
{
    const MsgAPI& api;

    QuantesonoOperation(const MsgAPI& api)
        : api(api)
    {
    }

    int run()
    {
        const impl::Message* curmsg = api.curmsg();
        if (!curmsg)
            throw error_consistency("query_stations called without a current message");
        cursor = curmsg->query_stations(api.input_query);
        return cursor->remaining();
    }

    bool next_station() override
    {
        return cursor->next();
    }

    void query_attributes(Attributes& dest) override { throw error_consistency("query_attributes cannot be called after query_stations/next_station"); }
    void insert_attribute(Values& qcinput) override { throw error_consistency("insert_attribute cannot be called after query_stations/next_station"); }
    void scusa() override { throw error_consistency("scusa cannot be called after query_stations/next_station"); }
};

template<typename Cursor>
struct CursorTraits {};

template<>
struct CursorTraits<impl::msg::CursorStationData>
{
    static inline std::unique_ptr<impl::msg::CursorStationData> query(const dballe::impl::Message& msg, const core::Query& query)
    {
        return std::unique_ptr<impl::msg::CursorStationData>(dynamic_cast<impl::msg::CursorStationData*>(msg.query_station_data(query).release()));
    }
    /*
    static inline void attr_insert(db::Transaction& tr, int id, const Values& values)
    {
        tr.attr_insert_station(id, values);
    }
    */
};

template<>
struct CursorTraits<impl::msg::CursorData>
{
    static inline std::unique_ptr<impl::msg::CursorData> query(const dballe::impl::Message& msg, const core::Query& query)
    {
        return std::unique_ptr<impl::msg::CursorData>(dynamic_cast<impl::msg::CursorData*>(msg.query_data(query).release()));
    }
    /*
    static inline void attr_insert(db::Transaction& tr, int id, const Values& values)
    {
        tr.attr_insert_data(id, values);
    }
    */
};

template<typename Cursor>
struct VoglioquestoOperation : public CursorOperation<Cursor>
{
    MsgAPI& api;
    bool valid_cached_attrs = false;
    bool next_data_ended = false;

    VoglioquestoOperation(MsgAPI& api)
        : api(api)
    {
    }

    int run()
    {
        impl::Message* msg = api.curmsg();
        if (!msg) return API::missing_int;

        // this->cursor.reset(CursorTraits<Cursor>::query(*msg, api.input_query).release());
        this->cursor.reset(dynamic_cast<Cursor*>(msg->query_station_and_data(api.input_query).release()));
        return this->cursor->remaining();
    }

    wreport::Varcode next_data() override
    {
        if (next_data_ended) return 0;

        if (this->cursor->next())
        {
            valid_cached_attrs = true;
            return this->cursor->get_varcode();
        } else {
            next_data_ended = true;
            return 0;
        }
    }

    void query_attributes(Attributes& dest) override
    {
        if (next_data_ended) throw error_consistency("query_attributes called after next_data returned end of data");

        wreport::Var var = this->cursor->get_var();
        api.qcoutput.values.clear();
        for (const Var* attr = var.next_attr(); attr; attr = attr->next_attr())
            api.qcoutput.values.set(*attr);
        api.qcoutput.has_new_values();
        api.qcinput.clear();
    }

    void insert_attribute(Values& qcinput) override
    {
        throw error_consistency("insert_attribute has been called without a previous insert_data");
    }

    void scusa() override
    {
        throw error_consistency("scusa does not make sense when writing messages");
    }
};

struct PrendiloOperation : public Operation
{
    /// Store database variable IDs for all last inserted variables
    MsgAPI& api;
    wreport::Varcode varcode = 0;
    /// Level for vars
    Level vars_level;
    /// Time range for vars
    Trange vars_trange;
    /// Last variables written with insert_data
    Values vars;

    PrendiloOperation(MsgAPI& api)
        : api(api)
    {
    }
    ~PrendiloOperation()
    {
        if (!vars.empty() && api.wmsg)
            flushVars();
    }

    void set_varcode(wreport::Varcode varcode) override { this->varcode = varcode; }

    void flushVars()
    {
        // Acquire the variables still around from the last insert_data
        vars.move_to([&](std::unique_ptr<wreport::Var> var) {
            api.wmsg->set(vars_level, vars_trange, std::move(var));
        });
    }

    void run()
    {
        if (!api.msgs) api.msgs = new std::vector<std::shared_ptr<dballe::Message>>;
        if (!api.wmsg) api.wmsg = new impl::Message;

        // Store record metainfo
        if (!api.input_data.station.report.empty())
        {
            api.wmsg->set_rep_memo(api.input_data.station.report.c_str());
            api.wmsg->type = impl::Message::type_from_repmemo(api.input_data.station.report.c_str());
        }
        if (api.input_data.station.id != MISSING_INT)
            api.wmsg->station_data.set(newvar(WR_VAR(0, 1, 192), api.input_data.station.id));
        if (!api.input_data.station.ident.is_missing())
            api.wmsg->set_ident(api.input_data.station.ident);
        if (api.input_data.station.coords.lat != MISSING_INT)
            api.wmsg->set_latitude(api.input_data.station.coords.dlat());
        if (api.input_data.station.coords.lon != MISSING_INT)
            api.wmsg->set_longitude(api.input_data.station.coords.dlon());

        Datetime dt = api.input_data.datetime;
        if (!api.input_data.datetime.is_missing())
        {
            dt.set_lower_bound();
            api.wmsg->set_datetime(dt);
        }

        flushVars();
        assert(vars.empty());

        vars_level = api.input_data.level;
        vars_trange = api.input_data.trange;

        vars = std::move(api.input_data.values);

        if (!api.input_query.query.empty())
        {
            if (strcasecmp(api.input_query.query.c_str(), "subset") == 0)
            {
                api.flushSubset();
            } else if (strncasecmp(api.input_query.query.c_str(), "message", 7) == 0) {
                // Check that message is followed by spaces or end of string
                const char* s = api.input_query.query.c_str() + 7;
                if (*s != 0 && !isblank(*s))
                    error_consistency::throwf("Query type \"%s\" is not among the supported values", api.input_query.query.c_str());
                // Skip the spaces after message
                while (*s != 0 && isblank(*s))
                    ++s;

                // Set or reset the exporter template
                api.set_exporter(s);

                api.flushMessage();
            } else
                error_consistency::throwf("Query type \"%s\" is not among the supported values", api.input_query.query.c_str());

            // Uset query after using it: it needs to be explicitly set every time
            api.input_query.query.clear();
        }
    }
    void query_attributes(Attributes& dest) override
    {
        throw error_consistency("query_attributes cannot be called after a insert_data");
    }
    void insert_attribute(Values& qcinput) override
    {
        if (vars.empty())
            throw error_consistency("insert_attribute has been called without a previous insert_data");
        if (vars.size() > 1)
            throw error_consistency("insert_attribute has been called after setting many variables with a single insert_data, so I do not know which one should get the attributes");

        qcinput.move_to_attributes(**vars.begin());
    }

    void scusa() override
    {
        throw error_consistency("scusa does not make sense when writing messages");
    }
    int enqi(const char* param) const override { wreport::error_consistency::throwf("enqi %s cannot be called after a insert_data", param); }
    double enqd(const char* param) const override { throw wreport::error_consistency("enqd cannot be called after a insert_data"); }
    bool enqc(const char* param, std::string& res) const override { throw wreport::error_consistency("enqc cannot be called after a insert_data"); }
    void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2) const override { throw wreport::error_consistency("enqlevel cannot be called after a insert_data"); }
    void enqtimerange(int& ptype, int& p1, int& p2) const override { throw wreport::error_consistency("enqtimerange cannot be called after a insert_data"); }
    void enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec) const override { throw wreport::error_consistency("enqdate cannot be called after a insert_data"); }
};

}


MsgAPI::MsgAPI(const char* fname, const char* mode, const char* type)
    : file(0), state(STATE_BLANK), importer(0), curmsgidx(0),
        cached_cat(0), cached_subcat(0), cached_lcat(0)
{
    if (strchr(mode, 'r') != NULL)
    {
        perms = compute_permissions("read", "read", "read");
    } else if (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL) {
        perms = compute_permissions("write", "add", "write");
    }

    if (strcasecmp(type, "BUFR") == 0)
        file = File::create(Encoding::BUFR, fname, mode).release();
    else if (strcasecmp(type, "CREX") == 0)
        file = File::create(Encoding::CREX, fname, mode).release();
    else if (strcasecmp(type, "AUTO") == 0)
        file = File::create(fname, mode).release();
    else
        error_consistency::throwf("\"%s\" is not one of the supported message types", type);

    if (strchr(mode, 'r') != NULL)
        importer = Importer::create(file->encoding()).release();
}

MsgAPI::~MsgAPI()
{
    reset_operation();
    if (wmsg)
    {
        flushSubset();
        flushMessage();
    }
    if (file) delete file;
    if (importer) delete importer;
    if (exporter) delete exporter;
}

void MsgAPI::flushSubset()
{
    unique_ptr<Message> awmsg(wmsg);
    wmsg = nullptr;
    msgs->emplace_back(move(awmsg));
}

void MsgAPI::flushMessage()
{
    if (wmsg)
        flushSubset();
    if (!msgs->empty())
    {
        if (!exporter)
        {
            impl::ExporterOptions opts;
            opts.template_name = exporter_template;
            exporter = Exporter::create(file->encoding(), opts).release();
        }
        file->write(exporter->to_binary(*msgs));
    }
    delete msgs;
    msgs = nullptr;
}

const impl::Message* MsgAPI::curmsg() const
{
    if (msgs && curmsgidx < msgs->size())
        return &impl::Message::downcast(*(*msgs)[curmsgidx]);
    else
        return nullptr;
}

impl::Message* MsgAPI::curmsg()
{
    if (msgs && curmsgidx < msgs->size())
        return &impl::Message::downcast(*(*msgs)[curmsgidx]);
    else
        return nullptr;
}

bool MsgAPI::readNextMessage()
{
    if (state & STATE_EOF)
        return false;

    if (msgs && curmsgidx < msgs->size() - 1)
    {
        ++curmsgidx;
        return true;
    }

    state = STATE_BLANK;
    curmsgidx = 0;
    if (msgs)
    {
        delete msgs;
        msgs = 0;
    }

    if (BinaryMessage raw = file->read())
    {
        auto messages = importer->from_binary(raw);
        msgs = new std::vector<std::shared_ptr<Message>>(std::move(messages));
        state &= ~STATE_BLANK;
        return true;
    }

    state &= ~STATE_BLANK;
    state |= STATE_EOF;
    return false;
}

void MsgAPI::reinit_db(const char* repinfofile)
{
    if (!(perms & PERM_DATA_WRITE))
        throw error_consistency(
            "reinit_db must be run with the database open in data write mode");

    // FIXME: In theory, nothing to do
    // FIXME: In practice, we could reset all buffered data and ftruncate the file
}

int MsgAPI::query_stations()
{
    if (state & (STATE_BLANK | STATE_QUANTESONO))
        readNextMessage();
    if (state & STATE_EOF)
        return missing_int;
    validate_input_query();
    state |= STATE_QUANTESONO;
    return reset_operation(new QuantesonoOperation(*this));
}

int MsgAPI::query_data()
{
    if (state & (STATE_BLANK | STATE_VOGLIOQUESTO))
        readNextMessage();
    if (state & STATE_EOF)
        return missing_int;
    validate_input_query();
    state |= STATE_VOGLIOQUESTO;

#if 0
    if (station_context)
        return reset_operation(new VoglioquestoOperation<impl::msg::CursorStationData>(*this));
    else
#endif
        return reset_operation(new VoglioquestoOperation<impl::msg::CursorData>(*this));
}

void MsgAPI::set_exporter(const char* template_name)
{
    if (exporter and exporter_template == template_name)
        return;

    // If it has changed, we need to recreate the exporter
    delete exporter;
    exporter = nullptr;
    exporter_template = template_name;
}

void MsgAPI::insert_data()
{
    if (perms & PERM_DATA_RO)
        error_consistency("insert_data cannot be called with the file open in read mode");

    input_data.datetime.set_lower_bound();
    reset_operation(new PrendiloOperation(*this));
    unsetb();
}

void MsgAPI::remove_data()
{
    throw error_consistency("remove_data does not make sense when writing messages");
}

void MsgAPI::messages_open_input(const char* filename, const char* mode, Encoding format, bool)
{
    throw error_unimplemented("MsgAPI::messages_open_input");
}

void MsgAPI::messages_open_output(const char* filename, const char* mode, Encoding format)
{
    throw error_unimplemented("MsgAPI::messages_open_output");
}

bool MsgAPI::messages_read_next()
{
    throw error_unimplemented("MsgAPI::messages_read_next");
}

void MsgAPI::messages_write_next(const char*)
{
    throw error_unimplemented("MsgAPI::messages_write_next");
}

void MsgAPI::remove_all()
{
    throw error_unimplemented("MsgAPI::remove_all");
}

}
}

/* vim:set ts=4 sw=4: */
