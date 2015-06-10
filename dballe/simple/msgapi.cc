/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "msgapi.h"
#include <wreport/var.h>
#include <dballe/core/file.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/context.h>
#include <dballe/msg/codec.h>
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
		set_permissions("read", "read", "read");
	} else if (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL) {
		set_permissions("write", "add", "write");
	}
	Encoding etype = (Encoding)-1;
	if (strcasecmp(type, "BUFR") == 0)
		etype = BUFR;
	else if (strcasecmp(type, "CREX") == 0)
		etype = CREX;
	else if (strcasecmp(type, "AOF") == 0)
		etype = AOF;
	else if (strcasecmp(type, "AUTO") == 0)
		etype = (Encoding)-1;
	else
		error_consistency::throwf("\"%s\" is not one of the supported message types", type);

	file = File::create(etype, fname, mode).release();

	if (strchr(mode, 'r') != NULL)
	{
		importer = msg::Importer::create(etype).release();
	}
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
		return (*msgs)[curmsgidx];
	else
		return NULL;
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

	Rawmsg raw;
	if (file->read(raw))
	{
        unique_ptr<Msgs> new_msgs(new Msgs);
        importer->from_rawmsg(raw, *new_msgs);
        msgs = new_msgs.release();
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

    output.set_ana_context();
    output.set("mobile", 0);
    output.set("rep_memo", Msg::repmemo_from_type(msg->type));

    for (size_t l = 0; l < ctx->data.size(); ++l)
    {
        const Var& var = *(ctx->data[l]);
        switch (var.code())
        {
            case WR_VAR(0, 5,   1): output.set("lat", var.enqd()); break;
            case WR_VAR(0, 6,   1): output.set("lon", var.enqd()); break;
            case WR_VAR(0, 1,  11):
                output.set("ident", var.enqc());
                output.set("mobile", 1);
                break;
            case WR_VAR(0, 1, 192): output.set("ana_id", var.enqi()); break;
            case WR_VAR(0, 1, 194): output.set("rep_memo", var.enqc()); break;
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

const char* MsgAPI::dammelo()
{
	if ((state & STATE_VOGLIOQUESTO) == 0)
		throw error_consistency("dammelo called without a previous voglioquesto");

	output.clear();

	Msg* msg = curmsg();
    if (!msg) return 0;

	if (!incrementMsgIters())
		return 0;

    output.set(msg->datetime());

    // Set metainfo from msg ana layer
    if (const msg::Context* ctx = msg->find_context(Level(), Trange()))
    {
        output.set("mobile", 0);
        output.set("rep_memo", Msg::repmemo_from_type(msg->type));

        for (size_t l = 0; l < ctx->data.size(); ++l)
        {
            const Var& var = *(ctx->data[l]);
            switch (var.code())
            {
                case WR_VAR(0, 5,   1): output.set("lat", var.enqd()); break;
                case WR_VAR(0, 6,   1): output.set("lon", var.enqd()); break;
                case WR_VAR(0, 4,   1): output.seti("year", var.enqi()); break;
                case WR_VAR(0, 4,   2): output.seti("month", var.enqi()); break;
                case WR_VAR(0, 4,   3): output.seti("day", var.enqi()); break;
                case WR_VAR(0, 4,   4): output.seti("hour", var.enqi()); break;
                case WR_VAR(0, 4,   5): output.seti("min", var.enqi()); break;
                case WR_VAR(0, 4,   6): output.seti("sec", var.enqi()); break;
                case WR_VAR(0, 1,  11):
                    output.set("ident", var.enqc());
                    output.set("mobile", 1);
                    break;
                case WR_VAR(0, 1, 192): output.set("ana_id", var.enqi()); break;
                case WR_VAR(0, 1, 194): output.set("rep_memo", var.enqc()); break;
                default: output.set(var); break;
            }
        }
    }

    msg::Context* ctx = msg->data[iter_ctx];
    output.set(ctx->level);
    output.set(ctx->trange);

    const Var& var = *ctx->data[iter_var];

    char vname[10];
    Varcode code = var.code();
    snprintf(vname, 10, "B%02d%03d", WR_VAR_X(code), WR_VAR_Y(code));
    output.set("var", vname);
    output.set(var);

    // Return the pointer to the copy inside the output record. We cannot
    // return vname as it is in the local stack
    return output.get("var")->value();
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

		wmsg->set(move(var), vars_level, vars_trange);
	}
}

void MsgAPI::flushSubset()
{
	if (wmsg)
	{
		flushVars();
		unique_ptr<Msg> awmsg(wmsg);
		wmsg = 0;
		msgs->acquire(move(awmsg));
	}
}

void MsgAPI::flushMessage()
{
	if (msgs)
	{
		flushSubset();
		Rawmsg raw;
		if (exporter == 0)
		{
			msg::Exporter::Options opts;
			opts.template_name = exporter_template;
			exporter = msg::Exporter::create(file->type(), opts).release();
		}
		exporter->to_rawmsg(*msgs, raw);
		file->write(raw);
		delete msgs;
		msgs = 0;
	}
}

void MsgAPI::prendilo()
{
	if (perms & PERM_DATA_RO)
		error_consistency("prendilo cannot be called with the file open in read mode");

	if (!msgs) msgs = new Msgs;
	if (!wmsg) wmsg = new Msg;

    // Store record metainfo
    if (const Var* var = input.get("rep_memo"))
        if (const char* val = var->value())
        {
            wmsg->set_rep_memo(val);
            wmsg->type = Msg::type_from_repmemo(val);
        }
    if (const Var* var = input.get("ana_id"))
        wmsg->seti(WR_VAR(0, 1, 192), var->enqi(), -1, Level(), Trange());
    if (const Var* var = input.get("ident"))
        wmsg->set_ident(var->enqc());
    if (const Var* var = input.get("lat"))
        wmsg->set_latitude(var->enqd());
    if (const Var* var = input.get("lon"))
        wmsg->set_longitude(var->enqd());

    int ye = input.enq("year", MISSING_INT);
    int mo = input.enq("month", MISSING_INT);
    int da = input.enq("day", MISSING_INT);
    int ho = input.enq("hour", MISSING_INT);
    int mi = input.enq("min", MISSING_INT);
    int se = input.enq("sec", MISSING_INT);

    if (ye == MISSING_INT)
        throw error_consistency("no year information found in message to import");
    if (mo == MISSING_INT)
        throw error_consistency("no month information found in message to import");
    if (da == MISSING_INT)
        throw error_consistency("no day information found in message to import");
    if (ho == MISSING_INT)
        throw error_consistency("no hour information found in message to import");
    wmsg->set_datetime(Datetime(ye, mo, da, ho, mi, se));

	const vector<Var*>& in_vars = input.vars();
	flushVars();
	assert(vars.empty());

    vars_level.ltype1 = input.enq("leveltype1", MISSING_INT);
    vars_level.l1 = input.enq("l1", MISSING_INT);
    vars_level.ltype2 = input.enq("leveltype2", MISSING_INT);
    vars_level.l2 = input.enq("l2", MISSING_INT);
    vars_trange.pind = input.enq("pindicator", MISSING_INT);
    vars_trange.p1 = input.enq("p1", MISSING_INT);
    vars_trange.p2 = input.enq("p2", MISSING_INT);

	for (vector<Var*>::const_iterator v = in_vars.begin(); v != in_vars.end(); ++v)
		vars.push_back(new Var(**v));
	input.clear_vars();

    if (const Var* var = input.get("query"))
        if (const char* query = var->value())
        {
            if (strcasecmp(query, "subset") == 0)
            {
                flushSubset();
            } else if (strncasecmp(query, "message", 7) == 0) {
                // Check that message is followed by spaces or end of string
                const char* s = query + 7;
                if (*s != 0 && !isblank(*s))
                    error_consistency::throwf("Query type \"%s\" is not among the supported values", query);
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
                error_consistency::throwf("Query type \"%s\" is not among the supported values", query);

            // Uset query after using it: it needs to be explicitly set every time
            input.unset("query");
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

	int count = 0;
	for (const Var* attr = var.next_attr(); attr; attr = attr->next_attr())
	{
		qcoutput.set(*attr);
		++count;
	}

	qc_iter = 0;

	return count;
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

	const vector<Var*>& avars = qcinput.vars();
	for (vector<Var*>::const_iterator i = avars.begin(); i != avars.end(); ++i)
		vars[0]->seta(**i);
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
