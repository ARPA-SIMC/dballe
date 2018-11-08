#include "msgapi.h"
#include <wreport/var.h>
#include "dballe/file.h"
#include "dballe/importer.h"
#include "dballe/exporter.h"
#include "dballe/message.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include "dballe/core/var.h"
#include <cstring>
#include <cassert>

using namespace wreport;
using namespace std;

namespace dballe {
namespace fortran {


MsgAPI::MsgAPI(const char* fname, const char* mode, const char* type)
    : file(0), state(STATE_BLANK), importer(0), exporter(0), msgs(0), wmsg(0), curmsgidx(0), iter_ctx(-1), iter_var(-1),
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
    if (perms & (PERM_DATA_WRITE | PERM_DATA_ADD))
    {
        if (wmsg) flushSubset();
        if (msgs) flushMessage();
    } else {
        if (wmsg) delete wmsg;
        if (msgs) delete msgs;
    }
    if (file) delete file;
    if (importer) delete importer;
    if (exporter) delete exporter;
    for (vector<Var*>::iterator i = vars.begin(); i != vars.end(); ++i)
        delete *i;
}

Msg* MsgAPI::curmsg()
{
    if (msgs && curmsgidx < msgs->size())
        return &Msg::downcast(*(*msgs)[curmsgidx]);
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

void MsgAPI::scopa(const char* repinfofile)
{
    if (!(perms & PERM_DATA_WRITE))
        throw error_consistency(
            "scopa must be run with the database open in data write mode");

    // FIXME: In theory, nothing to do
    // FIXME: In practice, we could reset all buffered data and ftruncate the file
}

int MsgAPI::quantesono()
{
    if (state & (STATE_BLANK | STATE_QUANTESONO))
        readNextMessage();
    if (state & STATE_EOF)
        return missing_int;
    state |= STATE_QUANTESONO;
        
    return 1;
}

void MsgAPI::elencamele()
{
    if ((state & STATE_QUANTESONO) == 0)
        throw error_consistency("elencamele called without a previous quantesono");

    output.clear();

    Msg* msg = curmsg();
    if (!msg) return;

    const msg::Context* ctx = msg->find_context(Level(), Trange());
    if (!ctx) return;

    output.mobile = 0;
    output.station.report = Msg::repmemo_from_type(msg->type);

    for (size_t l = 0; l < ctx->data.size(); ++l)
    {
        const Var& var = *(ctx->data[l]);
        switch (var.code())
        {
            case WR_VAR(0, 5,   1): output.station.coords.set_lat(var.enqd()); break;
            case WR_VAR(0, 6,   1): output.station.coords.set_lon(var.enqd()); break;
            case WR_VAR(0, 1,  11):
                output.station.ident = var.enqc();
                output.mobile = 1;
                break;
            case WR_VAR(0, 1, 192): output.station.id = var.enqi(); break;
            case WR_VAR(0, 1, 194): output.station.report = var.enqc(); break;
            default: output.set(var); break;
        }
    }
}

bool MsgAPI::incrementMsgIters()
{
    if (iter_ctx < 0)
    {
        iter_ctx = 0;
        iter_var = -1;
    }

    Msg* msg = curmsg();
    if ((unsigned)iter_ctx >= msg->data.size())
        return false;

    const msg::Context* ctx = msg->data[iter_ctx];
    if (iter_var < (int)ctx->data.size() - 1)
    {
        ++iter_var;
    } else {
        ++iter_ctx;
        iter_var = 0;
    }

    // Skip redundant variables in the pseudoana layer
    if ((unsigned)iter_ctx < msg->data.size() && msg->data[iter_ctx]->level == Level())
    {
        vector<Var*> data = msg->data[iter_ctx]->data;
        while((unsigned)iter_var < data.size() && WR_VAR_X(data[iter_var]->code()) >= 4 && WR_VAR_X(data[iter_var]->code()) <= 6)
            ++iter_var;
        if ((unsigned)iter_var == data.size())
        {
            ++iter_ctx;
            iter_var = 0;
        }
    }

    if ((unsigned)iter_ctx >= msg->data.size())
        return false;

    return true;
}

int MsgAPI::voglioquesto()
{
    if (state & (STATE_BLANK | STATE_VOGLIOQUESTO))
        readNextMessage();
    if (state & STATE_EOF)
        return missing_int;
    state |= STATE_VOGLIOQUESTO;

    iter_ctx = iter_var = -1;

    Msg* msg = curmsg();
    if (!msg) return missing_int;

    int count = 0;
    for (size_t l = 0; l < msg->data.size(); ++l)
    {
        const msg::Context* ctx = msg->data[l];
        if (ctx->level == Level())
        {
            // Count skipping datetime and coordinate variables
            for (vector<Var*>::const_iterator i = ctx->data.begin();
                    i != ctx->data.end(); ++i)
                if (WR_VAR_X((*i)->code()) < 4 || WR_VAR_X((*i)->code()) > 6)
                    ++count;
        } else
            count += ctx->data.size();
    }
    return count;
}

wreport::Varcode MsgAPI::dammelo()
{
    if ((state & STATE_VOGLIOQUESTO) == 0)
        throw error_consistency("dammelo called without a previous voglioquesto");

    output.clear();

    Msg* msg = curmsg();
    if (!msg) return 0;

    if (!incrementMsgIters())
        return 0;

    output.set(msg->get_datetime());

    // Set metainfo from msg ana layer
    if (const msg::Context* ctx = msg->find_context(Level(), Trange()))
    {
        output.mobile = 0;
        output.station.report = Msg::repmemo_from_type(msg->type);

        Datetime dt;
        for (size_t l = 0; l < ctx->data.size(); ++l)
        {
            const Var& var = *(ctx->data[l]);
            switch (var.code())
            {
                case WR_VAR(0, 5,   1): output.station.coords.set_lat(var.enqd()); break;
                case WR_VAR(0, 6,   1): output.station.coords.set_lon(var.enqd()); break;
                case WR_VAR(0, 4,   1): dt.year   = var.enqi(); break;
                case WR_VAR(0, 4,   2): dt.month  = var.enqi(); break;
                case WR_VAR(0, 4,   3): dt.day    = var.enqi(); break;
                case WR_VAR(0, 4,   4): dt.hour   = var.enqi(); break;
                case WR_VAR(0, 4,   5): dt.minute = var.enqi(); break;
                case WR_VAR(0, 4,   6): dt.second = var.enqi(); break;
                case WR_VAR(0, 1,  11):
                    output.station.ident = var.enqc();
                    output.mobile = 1;
                    break;
                case WR_VAR(0, 1, 192): output.station.id = var.enqi(); break;
                case WR_VAR(0, 1, 194): output.station.report = var.enqc(); break;
                default: output.set(var); break;
            }
        }
        output.set(dt);
    }

    msg::Context* ctx = msg->data[iter_ctx];
    output.set(ctx->level);
    output.set(ctx->trange);

    const Var& var = *ctx->data[iter_var];
    output.var = var.code();
    output.set(var);
    return var.code();
}

void MsgAPI::flushVars()
{
    // Acquire the variables still around from the last prendilo
    while (!vars.empty())
    {
        // Pop a variable from the vector and take ownership of
        // its memory management
        unique_ptr<Var> var(vars.back());
        vars.pop_back();

        wmsg->set(vars_level, vars_trange, move(var));
    }
}

void MsgAPI::flushSubset()
{
    if (wmsg)
    {
        flushVars();
        unique_ptr<Message> awmsg(wmsg);
        wmsg = 0;
        msgs->emplace_back(move(awmsg));
    }
}

void MsgAPI::flushMessage()
{
    if (msgs)
    {
        flushSubset();
        if (exporter == 0)
        {
            ExporterOptions opts;
            opts.template_name = exporter_template;
            exporter = Exporter::create(file->encoding(), opts).release();
        }
        file->write(exporter->to_binary(*msgs));
        delete msgs;
        msgs = 0;
    }
}

void MsgAPI::prendilo()
{
    if (perms & PERM_DATA_RO)
        error_consistency("prendilo cannot be called with the file open in read mode");

    if (!msgs) msgs = new Messages;
    if (!wmsg) wmsg = new Msg;

    // Store record metainfo
    if (!input.station.report.empty())
    {
        wmsg->set_rep_memo(input.station.report.c_str());
        wmsg->type = Msg::type_from_repmemo(input.station.report.c_str());
    }
    DBStation station = input.get_dbstation();
    if (station.id != MISSING_INT)
        wmsg->set(Level(), Trange(), newvar(WR_VAR(0, 1, 192), station.id));
    if (!station.ident.is_missing())
        wmsg->set_ident(station.ident);
    if (station.coords.lat != MISSING_INT)
        wmsg->set_latitude(station.coords.dlat());
    if (station.coords.lon != MISSING_INT)
        wmsg->set_longitude(station.coords.dlon());

    Datetime dt = input.get_datetime();
    if (!dt.is_missing())
    {
        dt.set_lower_bound();
        wmsg->set_datetime(dt);
    }

    const vector<Var*>& in_vars = input.vars();
    flushVars();
    assert(vars.empty());

    vars_level = input.level;
    vars_trange = input.trange;

    for (vector<Var*>::const_iterator v = in_vars.begin(); v != in_vars.end(); ++v)
        vars.push_back(new Var(**v));
    input.clear_vars();

    if (!input.query.empty())
    {
        if (strcasecmp(input.query.c_str(), "subset") == 0)
        {
            flushSubset();
        } else if (strncasecmp(input.query.c_str(), "message", 7) == 0) {
            // Check that message is followed by spaces or end of string
            const char* s = input.query.c_str() + 7;
            if (*s != 0 && !isblank(*s))
                error_consistency::throwf("Query type \"%s\" is not among the supported values", input.query.c_str());
            // Skip the spaces after message
            while (*s != 0 && isblank(*s))
                ++s;

            // Set or reset the exporter template
            if (exporter_template != s)
            {
                // If it has changed, we need to recreate the exporter
                delete exporter;
                exporter = 0;
                exporter_template = s;
            }

            flushMessage();
        } else
            error_consistency::throwf("Query type \"%s\" is not among the supported values", input.query.c_str());

        // Uset query after using it: it needs to be explicitly set every time
        input.query.clear();
    }
}

void MsgAPI::dimenticami()
{
    throw error_consistency("dimenticami does not make sense when writing messages");
}

int MsgAPI::voglioancora()
{
    Msg* msg = curmsg();
    if (msg == 0 || iter_ctx < 0 || iter_var < 0)
        throw error_consistency("voglioancora called before dammelo");

    if ((unsigned)iter_ctx >= msg->data.size()) return 0;
    const msg::Context& ctx = *(msg->data[iter_ctx]);

    if ((unsigned)iter_var >= ctx.data.size()) return 0;
    const Var& var = *(ctx.data[iter_var]);

    qcoutput.clear();
    for (const Var* attr = var.next_attr(); attr; attr = attr->next_attr())
        qcoutput.push_back(*attr);
    qc_iter = 0;
    return qcoutput.size();
}

void MsgAPI::critica()
{
    if (perms & PERM_ATTR_RO)
        throw error_consistency(
            "critica cannot be called with the database open in attribute readonly mode");
    if (vars.empty())
        throw error_consistency("critica has been called without a previous prendilo");
    if (vars.size() > 1)
        throw error_consistency("critica has been called after setting many variables with a single prendilo, so I do not know which one should get the attributes");

    for (const auto& i: qcinput)
        vars[0]->seta(*i.second.var);
    qcinput.clear();
}

void MsgAPI::scusa()
{
    throw error_consistency("scusa does not make sense when writing messages");
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
