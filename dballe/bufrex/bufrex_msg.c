/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <config.h>

#include "bufrex_opcode.h"
#include "bufrex_msg.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


dba_err bufrex_msg_create(bufrex_msg* msg, bufrex_type type)
{
	*msg = (bufrex_msg)calloc(1, sizeof(struct _bufrex_msg));
	if (*msg == NULL)
		return dba_error_alloc("allocating new storage for decoded BUFR/CREX data");
    (*msg)->datadesc_last = &((*msg)->datadesc);
	(*msg)->encoding_type = type;
	return dba_error_ok();
}

void bufrex_msg_reset(bufrex_msg msg)
{
	int i;

	/* Preserve vars and vars_alloclen so that allocated memory can be reused */
	for (i = 0; i < msg->subsets_count; i++)
		bufrex_subset_delete(msg->subsets[i]);
	msg->subsets_count = 0;

	if (msg->datadesc != NULL)
	{
		bufrex_opcode_delete(&(msg->datadesc));
		msg->datadesc_last = &(msg->datadesc);
	}

	msg->type = 0;
	msg->subtype = 0;
}

void bufrex_msg_delete(bufrex_msg msg)
{
	bufrex_msg_reset(msg);

	if (msg->subsets)
	{
		free(msg->subsets);
		msg->subsets = NULL;
		msg->subsets_alloclen = 0;
	}

	free(msg);
}

dba_err bufrex_msg_get_subset(bufrex_msg msg, int subsection, bufrex_subset* vars)
{
	/* First ensure we have the allocated space we need */
	if (subsection >= msg->subsets_alloclen)
	{
		if (msg->subsets == NULL)
		{
			msg->subsets_alloclen = 1;
			if ((msg->subsets = (bufrex_subset*)malloc(msg->subsets_alloclen * sizeof(bufrex_subset))) == NULL)
				return dba_error_alloc("allocating memory for message subsets");
		} else {
			bufrex_subset* newbuf;

			while (subsection >= msg->subsets_alloclen)
				/* Grow by doubling the allocated space */
				msg->subsets_alloclen <<= 1;

			if ((newbuf = (bufrex_subset*)realloc(
					msg->subsets, msg->subsets_alloclen * sizeof(bufrex_subset))) == NULL)
				return dba_error_alloc("allocating more memory for message subsets");
			msg->subsets = newbuf;
		}
	}

	/* Then see if we need to initialize more of the allocated subsets */
	while (msg->subsets_count <= subsection)
	{
		DBA_RUN_OR_RETURN(bufrex_subset_create(msg->btable, &(msg->subsets[msg->subsets_count])));
		++msg->subsets_count;
	}

	/* Finally, return the subsection requested */
	*vars = msg->subsets[subsection];

	return dba_error_ok();
}


dba_err bufrex_msg_get_table_id(bufrex_msg msg, const char** id)
{
	if (msg->btable == NULL)
		*id = NULL;
	else
		*id = dba_vartable_id(msg->btable);
	return dba_error_ok();
}

dba_err bufrex_msg_load_tables(bufrex_msg msg)
{
	char id[30];
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR:
			sprintf(id, "B00000%03d%03d%02d%02d",
					0,
					msg->opt.bufr.origin,
					msg->opt.bufr.master_table,
					msg->opt.bufr.local_table);
			break;
		case BUFREX_CREX:
			sprintf(id, "B%02d%02d%02d",
					msg->opt.crex.master_table,
					msg->edition,
					msg->opt.crex.table);
			break;
	}

	DBA_RUN_OR_RETURN(dba_vartable_create(id, &(msg->btable)));
	/* TRACE(" -> loaded B table %s\n", id); */

	id[0] = 'D';
	DBA_RUN_OR_RETURN(bufrex_dtable_create(id, &(msg->dtable)));
	/* TRACE(" -> loaded D table %s\n", id); */

	return dba_error_ok();
}

dba_err bufrex_msg_query_btable(bufrex_msg msg, dba_varcode code, dba_varinfo* info)
{
	return dba_vartable_query(msg->btable, code, info);
}

dba_err bufrex_msg_query_dtable(bufrex_msg msg, dba_varcode code, struct _bufrex_opcode** res)
{
	return bufrex_dtable_query(msg->dtable, code, res);
}


void bufrex_msg_reset_datadesc(bufrex_msg msg)
{
	if (msg->datadesc != NULL)
	{
		bufrex_opcode_delete(&(msg->datadesc));
		msg->datadesc_last = &(msg->datadesc);
	}
}

dba_err bufrex_msg_get_datadesc(bufrex_msg msg, bufrex_opcode* res)
{
	*res = NULL;
	return bufrex_opcode_prepend(res, msg->datadesc);
}

dba_err bufrex_msg_append_datadesc(bufrex_msg msg, dba_varcode varcode)
{
	DBA_RUN_OR_RETURN(bufrex_opcode_append(msg->datadesc_last, varcode));
	msg->datadesc_last = &((*(msg->datadesc_last))->next);
	return dba_error_ok();
}

dba_err bufrex_msg_decode(bufrex_msg msg, dba_rawmsg raw)
{
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR: return bufr_decoder_decode(raw, msg);
		case BUFREX_CREX: return crex_decoder_decode(raw, msg);
	}
	return dba_error_consistency("Got invalid encoding type %d", msg->encoding_type);
}

dba_err bufrex_msg_encode(bufrex_msg msg, dba_rawmsg* raw)
{
	dba_err err;

	DBA_RUN_OR_RETURN(dba_rawmsg_create(raw));
	
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR: DBA_RUN_OR_GOTO(fail, bufr_encoder_encode(msg, *raw)); break;
		case BUFREX_CREX: DBA_RUN_OR_GOTO(fail, crex_encoder_encode(msg, *raw)); break;
		default:
			err = dba_error_consistency("Got invalid encoding type %d", msg->encoding_type);
			goto fail;
	}

	return dba_error_ok();

fail:
	if (*raw != NULL)
		dba_rawmsg_delete(*raw);
	return err;
}

void bufrex_msg_print(bufrex_msg msg, FILE* out)
{
	bufrex_opcode cur;
	int i, j;
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR: fprintf(out, "BUFR o%d m%d l%d", msg->opt.bufr.origin, msg->opt.bufr.master_table, msg->opt.bufr.local_table); break;
		case BUFREX_CREX: fprintf(out, "CREX T00%02d%02d", msg->opt.crex.master_table, msg->opt.crex.table); break;
	}
	fprintf(out, " type %d subtype %d edition %d table %s alloc %d, %d subsets.\n",
			msg->type, msg->subtype, msg->edition, msg->btable == NULL ? NULL : dba_vartable_id(msg->btable),
			msg->subsets_alloclen, msg->subsets_count);
	fprintf(out, "Data descriptors:");
	for (cur = msg->datadesc; cur != NULL; cur = cur->next)
		fprintf(out, " %d%02d%03d", DBA_VAR_F(cur->val), DBA_VAR_X(cur->val), DBA_VAR_Y(cur->val));
	putc('\n', out);
	for (i = 0; i < msg->subsets_count; i++)
	{
		fprintf(out, "Variables in section %d:\n", i);
		for (j = 0; j < msg->subsets[i]->vars_count; ++j)
			dba_var_print(msg->subsets[i]->vars[j], out);
	}
}

#if 0
#include <dballe/dba_check.h>

#ifdef HAVE_CHECK

#include <string.h> /* strcmp */

void test_bufrex_msg()
{
	/* dba_err err; */
	int val, val1;
	dba_var* vars;
	bufrex_msg msg;

	CHECKED(bufrex_msg_create(&msg));

	/* Resetting things should not fail */
	bufrex_msg_reset(msg);

	/* The message should be properly empty */
	CHECKED(bufrex_msg_get_category(msg, &val, &val1));
	fail_unless(val == 0);
	fail_unless(val1 == 0);
	CHECKED(bufrex_msg_get_vars(msg, &vars, &val));
	fail_unless(vars == 0);
	fail_unless(val == 0);

	CHECKED(bufrex_msg_set_category(msg, 42, 24));
	CHECKED(bufrex_msg_get_category(msg, &val, &val1));
	fail_unless(val == 42);
	fail_unless(val1 == 24);

	{
		dba_varinfo info;
		dba_var v;
		CHECKED(dba_varinfo_query_local(DBA_VAR(0, 1, 2), &info));
		CHECKED(dba_var_createi(info, &v, 7));
		CHECKED(bufrex_msg_store_variable(msg, v));
	}

	CHECKED(bufrex_msg_get_vars(msg, &vars, &val));
	fail_unless(vars != 0);
	fail_unless(vars[0] != 0);
	fail_unless(val == 1);
	fail_unless(dba_var_code(vars[0]) == DBA_VAR(0, 1, 2));
	fail_unless(strcmp(dba_var_value(vars[0]), "7") == 0);

	bufrex_msg_delete(msg);
}

#endif
#endif

/* vim:set ts=4 sw=4: */
