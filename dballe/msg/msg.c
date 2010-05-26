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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

dba_err dba_msg_create(dba_msg* msg)
{
	dba_msg res = (dba_msg)calloc(1, sizeof(struct _dba_msg));
	if (res == NULL)
		return dba_error_alloc("allocating new dba_msg");

	res->type = MSG_GENERIC;
	res->data_count = 0;
	res->data_alloc = 0;
	res->data = NULL;

	*msg = res;
	
	return dba_error_ok();
}

/* Enlarge the number of data that can be held my msg, so that it is at least 1
 * (possibly much more) more than it is now */
static dba_err dba_msg_enlarge(dba_msg msg)
{
	const int new_size = msg->data_alloc == 0 ? 10 : msg->data_alloc * 2;

	if (msg->data == NULL)
	{
		msg->data = (dba_msg_context*)calloc(new_size, sizeof(dba_msg_context));
		if (msg->data == NULL)
			return dba_error_alloc("enlarging the size of a dba_msg");
	} else {
		int i;
		dba_msg_context* new_data;
		new_data = (dba_msg_context*)realloc(msg->data, new_size * sizeof(dba_msg_context));
		if (new_data == NULL)
			return dba_error_alloc("enlarging the size of a dba_msg");
		msg->data = new_data;
		
		for (i = msg->data_count; i < new_size; i++)
			msg->data[i] = NULL;
	}
	msg->data_alloc = new_size;

	return dba_error_ok();
}

static dba_err dba_msg_add_context_nocopy(dba_msg msg, dba_msg_context ctx)
{
	int pos;

	/* Enlarge the buffer if needed */
	if (msg->data_count == msg->data_alloc)
		DBA_RUN_OR_RETURN(dba_msg_enlarge(msg));

	/* Insertionsort.  Crude, but our datasets should be too small for an
	 * RB-Tree to be worth */

	for (pos = msg->data_count; pos > 0; pos--)
		if (dba_msg_context_compare(msg->data[pos - 1], ctx) > 0)
			msg->data[pos] = msg->data[pos - 1];
		else
			break;
	msg->data[pos] = ctx;

	msg->data_count++;

	return dba_error_ok();
}

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

dba_err dba_msg_set_nocopy(dba_msg msg, dba_var var, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
	dba_err err;
	dba_msg_context ctx = dba_msg_find_context(msg, ltype1, l1, ltype2, l2, pind, p1, p2);

	if (ctx == NULL)
	{
		/* Create the level if missing */

		DBA_RUN_OR_RETURN(dba_msg_context_create(ltype1, l1, ltype2, l2, pind, p1, p2, &ctx));
		DBA_RUN_OR_GOTO(fail, dba_msg_add_context_nocopy(msg, ctx));
	}

	return dba_msg_context_set_nocopy(ctx, var);
	
fail:
	if (ctx != NULL)
		dba_msg_context_delete(ctx);
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

dba_err dba_msg_set(dba_msg msg, dba_var var, dba_varcode code, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
	dba_err err;
	dba_var copy = NULL;

	/* Make a copy of the variable, to give it to dba_msg_add_nocopy */
	DBA_RUN_OR_RETURN(dba_var_create_local(code, &copy));
	/* Use copy_val to ensure we get the variable code we want */
	DBA_RUN_OR_GOTO(fail, dba_var_copy_val(copy, var));

	DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, copy, ltype1, l1, ltype2, l2, pind, p1, p2));

	return dba_error_ok();

fail:
	if (copy != NULL)
		dba_var_delete(copy);
	return err;
}

dba_err dba_msg_seti(dba_msg msg, dba_varcode code, int val, int conf, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
	dba_err err;
	dba_var var = NULL;
	dba_var attr = NULL;
	DBA_RUN_OR_RETURN(dba_var_create_local(code, &var));
	DBA_RUN_OR_GOTO(fail, dba_var_seti(var, val));
	if (conf != -1)
	{
		DBA_RUN_OR_GOTO(fail, dba_var_create_local(DBA_VAR(0, 33, 7), &attr));
		DBA_RUN_OR_GOTO(fail, dba_var_seti(attr, conf));
		DBA_RUN_OR_GOTO(fail, dba_var_seta_nocopy(var, attr));
		attr = NULL;
	}
	DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, var, ltype1, l1, ltype2, l2, pind, p1, p2));
	return dba_error_ok();
	
fail:
	if (var != NULL)
		dba_var_delete(var);
	if (attr != NULL)
		dba_var_delete(attr);
	return err;
}

dba_err dba_msg_setd(dba_msg msg, dba_varcode code, double val, int conf, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
	dba_err err;
	dba_var var = NULL;
	dba_var attr = NULL;
	DBA_RUN_OR_RETURN(dba_var_create_local(code, &var));
	DBA_RUN_OR_GOTO(fail, dba_var_setd(var, val));
	if (conf != -1)
	{
		DBA_RUN_OR_GOTO(fail, dba_var_create_local(DBA_VAR(0, 33, 7), &attr));
		DBA_RUN_OR_GOTO(fail, dba_var_seti(attr, conf));
		DBA_RUN_OR_GOTO(fail, dba_var_seta_nocopy(var, attr));
		attr = NULL;
	}
	DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, var, ltype1, l1, ltype2, l2, pind, p1, p2));
	return dba_error_ok();
	
fail:
	if (var != NULL)
		dba_var_delete(var);
	if (attr != NULL)
		dba_var_delete(attr);
	return err;
}

dba_err dba_msg_setc(dba_msg msg, dba_varcode code, const char* val, int conf, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
	dba_err err;
	dba_var var = NULL;
	dba_var attr = NULL;
	DBA_RUN_OR_RETURN(dba_var_create_local(code, &var));
	DBA_RUN_OR_GOTO(fail, dba_var_setc(var, val));
	if (conf != -1)
	{
		DBA_RUN_OR_GOTO(fail, dba_var_create_local(DBA_VAR(0, 33, 7), &attr));
		DBA_RUN_OR_GOTO(fail, dba_var_seti(attr, conf));
		DBA_RUN_OR_GOTO(fail, dba_var_seta_nocopy(var, attr));
		attr = NULL;
	}
	DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, var, ltype1, l1, ltype2, l2, pind, p1, p2));
	return dba_error_ok();
	
fail:
	if (var != NULL)
		dba_var_delete(var);
	if (attr != NULL)
		dba_var_delete(attr);
	return err;
}

dba_msg_context dba_msg_find_context(dba_msg msg, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
	/* Binary search */
	int low = 0, high = msg->data_count - 1;
	while (low <= high)
	{
		int middle = low + (high - low)/2;
//fprintf(stderr, "DMFC lo %d hi %d mid %d\n", low, high, middle);
		int cmp = -dba_msg_context_compare2(msg->data[middle], ltype1, l1, ltype2, l2, pind, p1, p2);
		if (cmp < 0)
			high = middle - 1;
		else if (cmp > 0)
			low = middle + 1;
		else
			return msg->data[middle];
	}
	return NULL;
}

dba_var dba_msg_find(dba_msg msg, dba_varcode code, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
	dba_msg_context ctx = dba_msg_find_context(msg, ltype1, l1, ltype2, l2, pind, p1, p2);
	if (ctx == NULL)
		return NULL;
	return dba_msg_context_find(ctx, code);
}

dba_var dba_msg_find_by_id(dba_msg msg, int id)
{
	dba_msg_var v = &dba_msg_vartable[id];
	return dba_msg_find(msg, v->code, v->ltype1, v->l1, v->ltype2, v->l2, v->pind, v->p1, v->p2);
}

dba_msg_type dba_msg_get_type(dba_msg msg)
{
	return msg->type;
}

const char* dba_msg_type_name(dba_msg_type type)
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

dba_msg_type dba_msg_type_from_repmemo(const char* repmemo)
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

const char* dba_msg_repmemo_from_type(dba_msg_type type)
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

void dba_msg_print(dba_msg msg, FILE* out)
{
	int i;

	fprintf(out, "%s message\n", dba_msg_type_name(msg->type));

	if (msg->data_count == 0) return;
	fprintf(out, "%d contexts:\n", msg->data_count);

	switch (msg->type)
	{
		case MSG_PILOT:
		case MSG_TEMP:
		case MSG_TEMP_SHIP:
			for (i = 0; i < msg->data_count; i++)
			{
				dba_var vsig_var = dba_msg_context_find_vsig(msg->data[i]);
				if (vsig_var != NULL)
				{
					int vsig = strtol(dba_var_value(vsig_var), 0, 10);
					const int VSIG_EXTRA = 128;
					const int VSIG_SURFACE = 64;
					const int VSIG_STANDARD = 32;
					const int VSIG_TROPOPAUSE = 16;
					const int VSIG_MAXWIND = 8;
					const int VSIG_SIGTEMP = 4;
					const int VSIG_SIGWIND = 2;
					const int VSIG_MISSING = 1;

					fprintf(out, "Sounding #%d (level %d -", i + 1, vsig);
					if (vsig & VSIG_EXTRA)
						fprintf(out, " extra");
					if (vsig & VSIG_SURFACE)
						fprintf(out, " surface");
					if (vsig & VSIG_STANDARD)
						fprintf(out, " standard");
					if (vsig & VSIG_TROPOPAUSE)
						fprintf(out, " tropopause");
					if (vsig & VSIG_MAXWIND)
						fprintf(out, " maxwind");
					if (vsig & VSIG_SIGTEMP)
						fprintf(out, " sigtemp");
					if (vsig & VSIG_SIGWIND)
						fprintf(out, " sigwind");
					if (vsig & VSIG_MISSING)
						fprintf(out, " missing");
					fprintf(out, ") ");
				}
				dba_msg_context_print(msg->data[i], out);
			}
			break;
		default:
			for (i = 0; i < msg->data_count; i++)
				dba_msg_context_print(msg->data[i], out);
			break;
	}
}

static void context_summary(dba_msg_context c, FILE* out)
{
	fprintf(out, "c(%d,%d, %d,%d, %d,%d,%d)", c->ltype1, c->l1, c->ltype2, c->l2, c->pind, c->p1, c->p2);
}

void dba_msg_diff(dba_msg msg1, dba_msg msg2, int* diffs, FILE* out)
{
	int i1 = 0, i2 = 0;
	if (msg1->type != msg2->type)
	{
		fprintf(out, "the messages have different type (first is %s (%d), second is %s (%d))\n",
				dba_msg_type_name(msg1->type), msg1->type, dba_msg_type_name(msg2->type), msg2->type);
		(*diffs)++;
	}
	
	while (i1 < msg1->data_count || i2 < msg2->data_count)
	{
		if (i1 == msg1->data_count)
		{
			fprintf(out, "Context "); context_summary(msg2->data[i2], out);
			fprintf(out, " exists only in the second message\n");
			++i2;
			++*diffs;
		} else if (i2 == msg2->data_count) {
			fprintf(out, "Context "); context_summary(msg1->data[i1], out);
			fprintf(out, " exists only in the first message\n");
			++i1;
			++*diffs;
		} else {
			int cmp = dba_msg_context_compare(msg1->data[i1], msg2->data[i2]);
			if (cmp == 0)
			{
				dba_msg_context_diff(msg1->data[i1], msg2->data[i2], diffs, out);
				++i1;
				++i2;
			} else if (cmp < 0) {
				if (msg1->data[i1]->data_count != 0)
				{
					fprintf(out, "Context "); context_summary(msg1->data[i1], out);
					fprintf(out, " exists only in the first message\n");
					++*diffs;
				}
				++i1;
			} else {
				if (msg2->data[i2]->data_count != 0)
				{
					fprintf(out, "Context "); context_summary(msg2->data[i2], out);
					fprintf(out, " exists only in the second message\n");
					++*diffs;
				}
				++i2;
			}
		}
	}
}

void dba_msg_delete(dba_msg m)
{
	if (m->data_alloc)
	{
		int i;
		for (i = 0; i < m->data_count; i++)
			dba_msg_context_delete(m->data[i]);
		free(m->data);
	}
	free(m);
}


/* vim:set ts=4 sw=4: */
