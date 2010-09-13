/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "msg.h"
#include "context.h"
#include "vars.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

using namespace std;

namespace dballe {

const char* msg_type_name(MsgType type)
{
	switch (type)
	{
		case MSG_GENERIC: return "generic";
		case MSG_SYNOP: return "synop";
		case MSG_PILOT: return "pilot";
		case MSG_TEMP: return "temp";
		case MSG_TEMP_SHIP: return "temp_ship";
		case MSG_AIREP: return "airep";
		case MSG_AMDAR: return "amdar";
		case MSG_ACARS: return "acars";
		case MSG_SHIP: return "ship";
		case MSG_BUOY: return "buoy";
		case MSG_METAR: return "metar";
		case MSG_SAT: return "sat";
		case MSG_POLLUTION: return "pollution";
	}
	return "(unknown)";
}

Msg::Msg()
{
    type = MSG_GENERIC;
}

Msg::~Msg()
{
    for (vector<msg::Context*>::iterator i = data.begin(); i != data.end(); ++i)
        delete *i;
}

Msg::Msg(const Msg& m)
    : type(m.type)
{
	// Reserve space for the new contexts
	data.reserve(m.data.size());
	
	// Copy the contexts
	for (vector<msg::Context*>::const_iterator i = m.data.begin();
			i != m.data.end(); ++i)
        data.push_back(new msg::Context(**i));
}

Msg& Msg::operator=(const Msg& m)
{
    // Manage a = a
    if (this == &m) return *this;

    type = m.type;

	// Delete existing vars
	for (vector<msg::Context*>::iterator i = data.begin();
			i != data.end(); ++i)
		delete *i;
	data.clear();

	// Reserve space for the new contexts
	data.reserve(m.data.size());
	
	// Copy the contexts
	for (vector<msg::Context*>::const_iterator i = m.data.begin();
			i != m.data.end(); ++i)
        data.push_back(new msg::Context(**i));
}

int Msg::find_index(int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2) const
{
	/* Binary search */
	int low = 0, high = data.size() - 1;
	while (low <= high)
	{
		int middle = low + (high - low)/2;
//fprintf(stderr, "DMFC lo %d hi %d mid %d\n", low, high, middle);
		int cmp = -data[middle]->compare(ltype1, l1, ltype2, l2, pind, p1, p2);
		if (cmp < 0)
			high = middle - 1;
		else if (cmp > 0)
			low = middle + 1;
		else
			return middle;
	}
	return -1;
}

const msg::Context* Msg::find_context(int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2) const
{
    int pos = find_index(ltype1, l1, ltype2, l2, pind, p1, p2);
    if (pos == -1)
        return NULL;
    return data[pos];
}

msg::Context& Msg::obtain_context(int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
    int pos = find_index(ltype1, l1, ltype2, l2, pind, p1, p2);
    if (pos == -1)
    {
        auto_ptr<msg::Context> c(new msg::Context(ltype1, l1, ltype2, l2, pind, p1, p2));
        msg::Context* res = c.get();
        add_context(c);
        return *res;
    }
    return *data[pos];
}

void Msg::add_context(auto_ptr<msg::Context> ctx)
{
	// Enlarge the data
    data.resize(data.size() + 1);

	/* Insertionsort.  Crude, but our datasets should be too small for an
	 * RB-Tree to be worth */
	int pos;
	for (pos = data.size() - 1; pos > 0; --pos)
		if (data[pos - 1]->compare(*ctx) > 0)
			data[pos] = data[pos - 1];
		else
			break;
	data[pos] = ctx.release();
}

const Var* Msg::find(Varcode code, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2) const
{
	const msg::Context* ctx = find_context(ltype1, l1, ltype2, l2, pind, p1, p2);
	if (ctx == NULL) return NULL;
	return ctx->find(code);
}

const Var* Msg::find_by_id(int id) const
{
	const MsgVarShortcut& v = shortcutTable[id];
	return find(v.code, v.ltype1, v.l1, v.ltype2, v.l2, v.pind, v.p1, v.p2);
}

void Msg::print(FILE* out) const
{
	fprintf(out, "%s message ", msg_type_name(type));

	if (data.empty())
    {
        fprintf(stderr, "(empty)\n");
        return;
    }
    fprintf(out, "with %zd contexts:\n", data.size());

	switch (type)
	{
		case MSG_PILOT:
		case MSG_TEMP:
		case MSG_TEMP_SHIP:
			for (vector<msg::Context*>::const_iterator i = data.begin(); i != data.end(); ++i)
			{
				const Var* vsig = (*i)->find_vsig();
				if (vsig != NULL)
				{
					int vs = vsig->enqi();
					const int VSIG_EXTRA = 128;
					const int VSIG_SURFACE = 64;
					const int VSIG_STANDARD = 32;
					const int VSIG_TROPOPAUSE = 16;
					const int VSIG_MAXWIND = 8;
					const int VSIG_SIGTEMP = 4;
					const int VSIG_SIGWIND = 2;
					const int VSIG_MISSING = 1;

					fprintf(out, "Sounding #%d (level %d -", (i - data.begin()) + 1, vs);
					if (vs & VSIG_EXTRA)
						fprintf(out, " extra");
					if (vs & VSIG_SURFACE)
						fprintf(out, " surface");
					if (vs & VSIG_STANDARD)
						fprintf(out, " standard");
					if (vs & VSIG_TROPOPAUSE)
						fprintf(out, " tropopause");
					if (vs & VSIG_MAXWIND)
						fprintf(out, " maxwind");
					if (vs & VSIG_SIGTEMP)
						fprintf(out, " sigtemp");
					if (vs & VSIG_SIGWIND)
						fprintf(out, " sigwind");
					if (vs & VSIG_MISSING)
						fprintf(out, " missing");
					fprintf(out, ") ");
				}
				(*i)->print(out);
			}
			break;
		default:
			for (vector<msg::Context*>::const_iterator i = data.begin(); i != data.end(); ++i)
				(*i)->print(out);
			break;
	}
}

static void context_summary(const msg::Context& c, FILE* out)
{
	fprintf(out, "c(%d,%d, %d,%d, %d,%d,%d)", c.ltype1, c.l1, c.ltype2, c.l2, c.pind, c.p1, c.p2);
}

unsigned Msg::diff(const Msg& msg, FILE* out) const
{
    unsigned diffs = 0;
	if (type != msg.type)
	{
		fprintf(out, "the messages have different type (first is %s (%d), second is %s (%d))\n",
				msg_type_name(type), type, msg_type_name(msg.type), msg.type);
		++diffs;
	}
	
	int i1 = 0, i2 = 0;
	while (i1 < data.size() || i2 < msg.data.size())
	{
		if (i1 == data.size())
		{
			fprintf(out, "Context "); context_summary(*msg.data[i2], out);
			fprintf(out, " exists only in the second message\n");
			++i2;
			++diffs;
		} else if (i2 == msg.data.size()) {
			fprintf(out, "Context "); context_summary(*data[i1], out);
			fprintf(out, " exists only in the first message\n");
			++i1;
			++diffs;
		} else {
			int cmp = data[i1]->compare(*msg.data[i2]);
			if (cmp == 0)
			{
				diffs += data[i1]->diff(*msg.data[i2], out);
				++i1;
				++i2;
			} else if (cmp < 0) {
				if (data[i1]->data.size() != 0)
				{
					fprintf(out, "Context "); context_summary(*data[i1], out);
					fprintf(out, " exists only in the first message\n");
					++diffs;
				}
				++i1;
			} else {
				if (msg.data[i2]->data.size() != 0)
				{
					fprintf(out, "Context "); context_summary(*msg.data[i2], out);
					fprintf(out, " exists only in the second message\n");
					++diffs;
				}
				++i2;
			}
		}
	}
    return diffs;
}

void Msg::set(const Var& var, Varcode code, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
    auto_ptr<Var> copy(new Var(code));
    *copy = var; // Assignment performs conversion if needed
	set(copy, ltype1, l1, ltype2, l2, pind, p1, p2);
}

void Msg::set(std::auto_ptr<Var> var, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
    msg::Context& ctx = obtain_context(ltype1, l1, ltype2, l2, pind, p1, p2);
    ctx.set(var);
}

void Msg::seti(Varcode code, int val, int conf, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
    auto_ptr<Var> var(new Var(code, val));
	if (conf != -1)
        var->seta(auto_ptr<Var>(new Var(DBA_VAR(0, 33, 7), conf)));
    set(var, ltype1, l1, ltype2, l2, pind, p1, p2);
}

void Msg::setd(Varcode code, double val, int conf, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
    auto_ptr<Var> var(new Var(code, val));
	if (conf != -1)
        var->seta(auto_ptr<Var>(new Var(DBA_VAR(0, 33, 7), conf)));
    set(var, ltype1, l1, ltype2, l2, pind, p1, p2);
}

void Msg::setc(Varcode code, const char* val, int conf, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
    auto_ptr<Var> var(new Var(code, val));
	if (conf != -1)
        var->seta(auto_ptr<Var>(new Var(DBA_VAR(0, 33, 7), conf)));
    set(var, ltype1, l1, ltype2, l2, pind, p1, p2);
}

MsgType Msg::type_from_repmemo(const char* repmemo)
{
	if (repmemo == NULL || repmemo[0] == 0) return MSG_GENERIC;
	switch (tolower(repmemo[0]))
	{
		case 'a':
			if (strcasecmp(repmemo+1, "cars")==0) return MSG_ACARS;
			if (strcasecmp(repmemo+1, "irep")==0) return MSG_AIREP;
			if (strcasecmp(repmemo+1, "mdar")==0) return MSG_AMDAR;
			break;
		case 'b':
			if (strcasecmp(repmemo+1, "uoy")==0) return MSG_BUOY;
			break;
		case 'm':
			if (strcasecmp(repmemo+1, "etar")==0) return MSG_METAR;
			break;
		case 'p':
			if (strcasecmp(repmemo+1, "ilot")==0) return MSG_PILOT;
			if (strcasecmp(repmemo+1, "ollution")==0) return MSG_POLLUTION;
			break;
		case 's':
			if (strcasecmp(repmemo+1, "atellite")==0) return MSG_SAT;
			if (strcasecmp(repmemo+1, "hip")==0) return MSG_SHIP;
			if (strcasecmp(repmemo+1, "ynop")==0) return MSG_SYNOP;
			break;
		case 't':
			if (strcasecmp(repmemo+1, "emp")==0) return MSG_TEMP;
			if (strcasecmp(repmemo+1, "empship")==0) return MSG_TEMP_SHIP;
			break;
	}
	return MSG_GENERIC;
}

const char* Msg::repmemo_from_type(MsgType type)
{
	switch (type)
	{
		case MSG_SYNOP:		return "synop";
		case MSG_METAR:		return "metar";
		case MSG_SHIP:		return "ship";
		case MSG_BUOY:		return "buoy";
		case MSG_AIREP:		return "airep";
		case MSG_AMDAR:		return "amdar";
		case MSG_ACARS:		return "acars";
		case MSG_PILOT:		return "pilot";
		case MSG_TEMP:		return "temp";
		case MSG_TEMP_SHIP:	return "tempship";
		case MSG_SAT:		return "satellite";
		case MSG_POLLUTION:	return "pollution";
		case MSG_GENERIC:
		default:			return "generic";
	}
}
#if 0

static dba_err dba_msg_add_context(dba_msg msg, dba_msg_context ctx)
{
	dba_err err;
	dba_msg_context copy = NULL;
	DBA_RUN_OR_RETURN(dba_msg_context_copy(ctx, &copy));
	DBA_RUN_OR_GOTO(fail, dba_msg_add_context_nocopy(msg, copy));
	return dba_error_ok();

fail:
	if (copy)
		dba_msg_context_delete(copy);
	return err;
}

dba_err dba_msg_set_nocopy_by_id(dba_msg msg, dba_var var, int id)
{
	dba_msg_var v = &dba_msg_vartable[id];
	return dba_msg_set_nocopy(msg, var, v->ltype1, v->l1, v->ltype2, v->l2, v->pind, v->p1, v->p2);
}

dba_err dba_msg_set_by_id(dba_msg msg, dba_var var, int id)
{
	dba_msg_var v = &dba_msg_vartable[id];
	dba_err err;
	dba_var copy = NULL;

	/* Make a copy of the variable, to give it to dba_msg_add_nocopy */
	DBA_RUN_OR_RETURN(dba_var_create_local(v->code, &copy));
	/* Use copy_val to ensure we get the variable code we want */
	DBA_RUN_OR_GOTO(fail, dba_var_copy_val(copy, var));

	DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, copy, v->ltype1, v->l1, v->ltype2, v->l2, v->pind, v->p1, v->p2));

	return dba_error_ok();

fail:
	if (copy != NULL)
		dba_var_delete(copy);
	return err;
}



dba_msg_type dba_msg_get_type(dba_msg msg)
{
	return msg->type;
}



dba_err dba_msg_sounding_pack_levels(dba_msg msg, dba_msg* dst)
{
	dba_err err;
	dba_msg res = NULL;
	int i;

	DBA_RUN_OR_RETURN(dba_msg_create(&res));
	res->type = msg->type;

	for (i = 0; i < msg->data_count; i++)
	{
		dba_msg_context ctx = msg->data[i];
		int j;

		if (dba_msg_context_find_vsig(ctx) == NULL)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_add_context(res, msg->data[i]));
			continue;
		}

		for (j = 0; j < ctx->data_count; j++)
		{
			dba_var copy;
			DBA_RUN_OR_GOTO(fail, dba_var_copy(ctx->data[j], &copy));
			DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(res, copy, ctx->ltype1, ctx->l1, 0, 0, ctx->pind, ctx->p1, ctx->p2));
		}
	}

	*dst = res;

	return dba_error_ok();

fail:
	*dst = 0;
	if (res != NULL)
		dba_msg_delete(res);
	return err;
}

dba_err dba_msg_sounding_unpack_levels(dba_msg msg, dba_msg* dst)
{
	const int VSIG_MISSING = 1;
	const int VSIG_SIGWIND = 2;		/* 6 */
	const int VSIG_SIGTEMP = 4;		/* 5 */
	const int VSIG_MAXWIND = 8;		/* 4 */
	const int VSIG_TROPOPAUSE = 16;	/* 3 */
	const int VSIG_STANDARD = 32;	/* 2 */
	const int VSIG_SURFACE = 64;	/* 1 */

	dba_err err;
	dba_msg res = NULL;
	int i;

	DBA_RUN_OR_RETURN(dba_msg_create(&res));
	res->type = msg->type;

	for (i = 0; i < msg->data_count; i++)
	{
		dba_msg_context ctx = msg->data[i];
		dba_msg_context copy;
		dba_var vsig_var = dba_msg_context_find_vsig(ctx);
		int vsig;

		if (vsig_var == NULL)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_add_context(res, msg->data[i]));
			continue;
		}

		DBA_RUN_OR_GOTO(fail, dba_var_enqi(vsig_var, &vsig));
		if (vsig & VSIG_MISSING)
		{
			/* If there is no vsig, then we consider it a normal level */
			DBA_RUN_OR_GOTO(fail, dba_msg_add_context(res, msg->data[i]));
			continue;
		}

		/* DBA_RUN_OR_GOTO(fail, dba_var_enqi(msg->data[i].var_press, &press)); */

		/* TODO: delete the dba_msg_datum that do not belong in that level */

		if (vsig & VSIG_SIGWIND)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_context_copy(ctx, &copy));
			copy->l2 = 6;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_context_nocopy(res, copy));
		}
		if (vsig & VSIG_SIGTEMP)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_context_copy(ctx, &copy));
			copy->l2 = 5;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_context_nocopy(res, copy));
		}
		if (vsig & VSIG_MAXWIND)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_context_copy(ctx, &copy));
			copy->l2 = 4;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_context_nocopy(res, copy));
		}
		if (vsig & VSIG_TROPOPAUSE)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_context_copy(ctx, &copy));
			copy->l2 = 3;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_context_nocopy(res, copy));
		}
		if (vsig & VSIG_STANDARD)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_context_copy(ctx, &copy));
			copy->l2 = 2;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_context_nocopy(res, copy));
		}
		if (vsig & VSIG_SURFACE)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_context_copy(ctx, &copy));
			copy->l2 = 1;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_context_nocopy(res, copy));
		}
	}

	*dst = res;

	return dba_error_ok();

fail:
	*dst = 0;
	if (res != NULL)
		dba_msg_delete(res);
	return err;
}


#endif

} // namespace dballe

/* vim:set ts=4 sw=4: */
