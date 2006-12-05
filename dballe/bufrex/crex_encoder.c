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

#include "config.h"

#include "bufrex_opcode.h"
#include "bufrex_msg.h"

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>	/* isspace */
#include <stdlib.h>	/* malloc */
#include <string.h>	/* memcpy */
#include <math.h>	/* NAN */
#include <assert.h>	/* NAN */
#include <errno.h>	/* NAN */

/* #define TRACE_ENCODER */

#ifdef TRACE_ENCODER
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif


struct _encoder
{
	/* Input message data */
	bufrex_msg in;
	/* Output decoded variables */
	dba_rawmsg out;

	/* Offset of the start of CREX section 1 */
	int sec1_start;
	/* Offset of the start of CREX section 2 */
	int sec2_start;
	/* Offset of the start of CREX section 3 */
	int sec3_start;
	/* Offset of the start of CREX section 4 */
	int sec4_start;

	/* True if the CREX message uses the check digit feature */
	int has_check_digit;
	/* Value of the next expected check digit */
	int expected_check_digit;

	/* List of opcodes to decode */
	bufrex_opcode ops;
	/* Pointed to next variable not yet encoded in the variable array */
	dba_var* nextvar;
	/* Number of variables left to encode */
	int vars_left;
};
typedef struct _encoder* encoder;

static dba_err encoder_create(encoder* res)
{
	if ((*res = (encoder)calloc(1, sizeof(struct _encoder))) == NULL)
		return dba_error_alloc("allocating a new crex encoder");

	return dba_error_ok();
}

static void encoder_delete(encoder res)
{
	if (res->ops != NULL)
		bufrex_opcode_delete(&(res->ops));
	free(res);
}



#if 0
dba_err encoder_has_check_digit(crex_message msg, int* has_check_digit)
{
	*has_check_digit = msg->has_check_digit;
	return dba_error_ok();
}

dba_err encoder_set_check_digit(crex_message msg, int has_check_digit)
{
	msg->has_check_digit = has_check_digit;
	return dba_error_ok();
}
#endif


static dba_err encoder_raw_append(encoder e, const char* str, int len)
{
	while (e->out->len + len > e->out->alloclen)
		DBA_RUN_OR_RETURN(dba_rawmsg_expand_buffer(e->out));
	memcpy(e->out->buf + e->out->len, str, len);
	e->out->len += len;
	return dba_error_ok();
}

static dba_err encoder_raw_appendf(encoder e, const char* fmt, ...)
{
	char buf[256];
	int len;
	va_list ap;
	va_start(ap, fmt);
	len = vsnprintf(buf, 255, fmt, ap);
	va_end(ap);

	return encoder_raw_append(e, buf, len);
}

static dba_err encode_data_section(encoder e);

dba_err crex_encoder_encode(bufrex_msg in, dba_rawmsg out)
{
	dba_err err = DBA_OK;
	bufrex_opcode cur;
	encoder e = NULL;
	bufrex_opcode ops = NULL;
	int i;

	DBA_RUN_OR_RETURN(encoder_create(&e));
	e->in = in;
	e->out = out;

	/* Encode section 0 */
	DBA_RUN_OR_GOTO(fail, encoder_raw_append(e, "CREX++\r\r\n", 9));

	/* Encode section 1 */
	e->sec1_start = e->out->len;

	DBA_RUN_OR_GOTO(fail, encoder_raw_appendf(e, "T%02d%02d%02d A%03d%03d",
				e->in->opt.crex.master_table,
				e->in->edition,
				e->in->opt.crex.table,
				e->in->type,
				e->in->subtype));

	DBA_RUN_OR_GOTO(fail, bufrex_msg_get_datadesc(e->in, &ops));
	if (ops == NULL)
	{
		TRACE("Regenerating datadesc\n");

		/* If the data descriptor list is not already present, try to generate it
		 * from the varcodes of the variables in the first subgroup to encode */
		DBA_RUN_OR_GOTO(fail, bufrex_msg_generate_datadesc(e->in));

		/* Reread the descriptors */
		DBA_RUN_OR_GOTO(fail, bufrex_msg_get_datadesc(e->in, &ops));
	} else {
		TRACE("Reusing datadesc\n");
	}
	
	/* Encode the data descriptor section */
	for (cur = ops; cur != NULL; cur = cur->next)
	{
		char prefix;
		switch (DBA_VAR_F(cur->val))
		{
			case 0: prefix = 'B'; break;
			case 1: prefix = 'R'; break;
			case 2: prefix = 'C'; break;
			case 3: prefix = 'D'; break;
			default: prefix = '?'; break;
		}

		/* Don't put delayed replication counts in the data section */
		if (DBA_VAR_F(cur->val) == 0 && DBA_VAR_X(cur->val) == 31 && DBA_VAR_Y(cur->val) < 3)
			continue;

		DBA_RUN_OR_GOTO(fail, encoder_raw_appendf(e, " %c%02d%03d",
					prefix, DBA_VAR_X(cur->val), DBA_VAR_Y(cur->val)));
	}
	if (e->has_check_digit)
	{
		DBA_RUN_OR_GOTO(fail, encoder_raw_append(e, " E", 2));
		e->expected_check_digit = 1;
	}
	DBA_RUN_OR_GOTO(fail, encoder_raw_append(e, "++\r\r\n", 5));

	/* Encode section 2 */
	e->sec2_start = e->out->len;
	for (i = 0; i < e->in->subsets_count; ++i)
	{
		TRACE("Start encoding subsection %d\n", 1);

		/* Initialise the encoder with the list of variables to encode */
		e->nextvar = e->in->subsets[i]->vars;
		e->vars_left = e->in->subsets[i]->vars_count;

		/* Duplicate the data description section as the encoding TODO-list */
		DBA_RUN_OR_GOTO(fail, bufrex_opcode_prepend(&(e->ops), ops));

		/* Encode the values */
		DBA_RUN_OR_GOTO(fail, encode_data_section(e));

		if (e->vars_left > 0)
		{
			err = dba_error_consistency("not all variables have been encoded");
			goto fail;
		}
		if (e->ops != NULL)
		{
			err = dba_error_consistency("not all operators have been encoded");
			goto fail;
		}

		/* Encode the subsection terminator */
		if (i < e->in->subsets_count - 1)
			DBA_RUN_OR_GOTO(fail, encoder_raw_append(e, "+\r\r\n", 4));
		else
			DBA_RUN_OR_GOTO(fail, encoder_raw_append(e, "++\r\r\n", 5));
	}

	/* Encode section 3 */
	e->sec3_start = e->out->len;
	/* Nothing to do, as we have no custom section */

	/* Encode section 4 */
	e->sec4_start = e->out->len;
	DBA_RUN_OR_RETURN(encoder_raw_append(e, "7777\r\r\n", 7));

	e->out->encoding = CREX;

	return dba_error_ok();

fail:
	if (e != NULL)
		encoder_delete(e);
	return err;
}

static dba_err encode_check_digit(encoder e)
{
	if (e->has_check_digit)
	{
		char c = '0' + e->expected_check_digit;
		DBA_RUN_OR_RETURN(encoder_raw_append(e, &c, 1));
		e->expected_check_digit = (e->expected_check_digit + 1) % 10;
	}
	return dba_error_ok();
}

static dba_err encode_b_data(encoder e)
{
	dba_err err = DBA_OK;
	dba_varinfo crexinfo = NULL;
	int len;

	IFTRACE{
		TRACE("crex_message_encode_b_data: items: ");
		bufrex_opcode_print(e->ops, stderr);
		TRACE("\n");

		TRACE("Next 5 variables:\n");
		int i = 0;
		for (; i < 5 && e->nextvar[i] != NULL; i++)
			dba_var_print(e->nextvar[i], stderr);
	}

	if (e->vars_left <= 0)
		return dba_error_consistency("checking for availability of data to encode");

	/* Get informations about the variable */
	DBA_RUN_OR_RETURN(bufrex_msg_query_btable(e->in, dba_var_code(*e->nextvar), &crexinfo));

	len = crexinfo->len;
	DBA_RUN_OR_GOTO(cleanup, encoder_raw_append(e, " ", 1));
	DBA_RUN_OR_GOTO(cleanup, encode_check_digit(e));
	if (dba_var_value(*e->nextvar) == NULL)
	{
		int i;
		TRACE("len0: %d\n", len);
		for (i = 0; i < len; i++)
			DBA_RUN_OR_GOTO(cleanup, encoder_raw_append(e, "/", 1));
	} else if (crexinfo->is_string) {
		TRACE("len1: %d\n", len);
		DBA_RUN_OR_GOTO(cleanup, encoder_raw_appendf(e, "%-*.*s", len, len, dba_var_value(*e->nextvar)));
	} else {
		int val;

		DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(*e->nextvar, &val));

		/* FIXME: here goes handling of active C table modifiers */
		
		if (val < 0)
			len++;
		
		TRACE("len2: %d\n", len);
		DBA_RUN_OR_GOTO(cleanup, encoder_raw_appendf(e, "%0*d", len, val));
	}
	
	/* Remove from the chain the item that we handled */
	{
		bufrex_opcode op;
		DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(e->ops), &op));
		bufrex_opcode_delete(&op);
	}

	e->nextvar++;
	e->vars_left--;

	IFTRACE {
		TRACE("crex_message_encode_b_data (items:");
		bufrex_opcode_print(e->ops, stderr);
		TRACE(")\n");
	}

cleanup:
	return err == DBA_OK ? dba_error_ok() : err;
}

#if 0
static dba_err crex_read_c_data(bufrex_decoder decoder, bufrex_opcode* ops)
{
	bufrex_opcode op;
	dba_err err;
	/* Node affected by the operator */
	bufrex_opcode affected_op;

	/* Pop the C modifier node */
	DBA_RUN_OR_RETURN(bufrex_opcode_pop(ops, &op));

	TRACE("read_c_data\n");

	/* Pop the first node, since we handle it here */
	if ((err = bufrex_opcode_pop(ops, &affected_op)) != DBA_OK)
		goto fail1;

	/* Activate this C modifier */
	switch (DBA_VAR_X(op->val))
	{
		case 1:
			decoder->c_width = DBA_VAR_Y(op->val);
			break;
		case 2:
			decoder->c_scale = DBA_VAR_Y(op->val);
			break;
		case 5:
		case 7:
		case 60:
			err = dba_error_parse(decoder->fname, decoder->line_no,
					"C modifier %d is not supported", DBA_VAR_X(op->val));
			goto fail;
		default:
			err = dba_error_parse(decoder->fname, decoder->line_no,
					"Unknown C modifier %d", DBA_VAR_X(op->val));
			goto fail;
	}

	/* Decode the affected data */
	if ((err = crex_read_data(decoder, &affected_op)) != DBA_OK)
		goto fail;

	/* Deactivate the C modifier */
	decoder->c_width = 0;
	decoder->c_scale = 0;

	/* FIXME: affected_op should always be NULL */
	assert(affected_op == NULL);
	bufrex_opcode_delete(&affected_op);
	return dba_error_ok();

fail:
	bufrex_opcode_delete(&affected_op);
fail1:
	bufrex_opcode_delete(&op);
	return err;
}
#endif

static dba_err encode_r_data(encoder e)
{
	dba_err err;
	int group = DBA_VAR_X(e->ops->val);
	int count = DBA_VAR_Y(e->ops->val);
	bufrex_opcode rep_op;

	IFTRACE{
		TRACE("encode_r_data: items: ");
		bufrex_opcode_print(e->ops, stderr);
		TRACE("\n");

		TRACE("Next 5 variables:\n");
		int i = 0;
		for (; i < 5 && e->nextvar[i] != NULL; i++)
			dba_var_print(e->nextvar[i], stderr);
	}

	
	TRACE("R DATA %01d%02d%03d %d %d", 
			DBA_VAR_F(e->ops->val), DBA_VAR_X(e->ops->val), DBA_VAR_Y(e->ops->val), group, count);

	/* Pop the R repetition node, since we have read its value in group and count */
	{
		bufrex_opcode op;
		DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(e->ops), &op));
		bufrex_opcode_delete(&op);
	}

	if (count == 0)
	{
		/* Delayed replication */
		if (e->vars_left <= 0)
			return dba_error_consistency("checking for availability of data to encode");
		
		/* Encode the repetition count */
		DBA_RUN_OR_RETURN(encoder_raw_append(e, " ", 1));
		DBA_RUN_OR_RETURN(encode_check_digit(e));
		DBA_RUN_OR_RETURN(dba_var_enqi(*e->nextvar, &count));
		DBA_RUN_OR_RETURN(encoder_raw_appendf(e, "%04d", count));

		e->nextvar++;
		e->vars_left--;

		/* Pop the node with the repetition count */
		if (DBA_VAR_F(e->ops->val) == 0 && DBA_VAR_X(e->ops->val) == 31 && DBA_VAR_Y(e->ops->val) < 3)
		{
			bufrex_opcode op;
			DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(e->ops), &op));
			bufrex_opcode_delete(&op);
		}

		TRACE("read_c_data %d items %d times (delayed)\n", group, count);
	} else
		TRACE("read_c_data %d items %d times\n", group, count);

	/* Pop the first `group' nodes, since we handle them here */
	DBA_RUN_OR_RETURN(bufrex_opcode_pop_n(&(e->ops), &rep_op, group));

	/* Perform replication */
	while (count--)
	{
		/* fprintf(stderr, "Debug: rep %d left\n", count); */
		bufrex_opcode chain = NULL;
		bufrex_opcode saved = NULL;
		DBA_RUN_OR_GOTO(fail, bufrex_opcode_prepend(&chain, rep_op));

		IFTRACE{
			TRACE("encode_r_data %d items %d times left; items: ", group, count);
			bufrex_opcode_print(chain, stderr);
			TRACE("\n");
		}

		saved = e->ops;
		e->ops = chain;
		if ((err = encode_data_section(e)) != DBA_OK)
		{
			if (e->ops != NULL)
				bufrex_opcode_delete(&(e->ops));
			e->ops = saved;
			goto fail;
		}
		/* chain should always be NULL when encoding succeeded */
		assert(e->ops == NULL);
		e->ops = saved;
	}

	bufrex_opcode_delete(&rep_op);
	return dba_error_ok();

fail:
	bufrex_opcode_delete(&rep_op);
	return err;
}

static dba_err encode_data_section(encoder e)
{
	dba_err err;

	/*
	fprintf(stderr, "read_data: ");
	bufrex_opcode_print(ops, stderr);
	fprintf(stderr, "\n");
	*/
	TRACE("crex_message_encode_data_section: START\n");

	while (e->ops != NULL)
	{
		IFTRACE{
			TRACE("crex_message_encode_data_section TODO: ");
			bufrex_opcode_print(e->ops, stderr);
			TRACE("\n");
		}

		switch (DBA_VAR_F(e->ops->val))
		{
			case 0:
				DBA_RUN_OR_RETURN(encode_b_data(e));
				break;
			case 1:
				DBA_RUN_OR_RETURN(encode_r_data(e));
				break;
			case 2:
				err = dba_error_unimplemented("encoding C modifiers is not supported yet");
				goto fail;
				/* DBA_RUN_OR_RETURN(encode_c_data(e)); */
				break;
			case 3:
			{
				/* D table opcode: expand the chain */
				bufrex_opcode op;
				bufrex_opcode exp;

				/* Pop the first opcode */
				DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(e->ops), &op));
				
				if ((err = bufrex_msg_query_dtable(e->in, op->val, &exp)) != DBA_OK)
				{
					bufrex_opcode_delete(&op);
					return err;
				}

				/* Push the expansion back in the fields */
				if ((err = bufrex_opcode_join(&exp, e->ops)) != DBA_OK)
				{
					bufrex_opcode_delete(&op);
					bufrex_opcode_delete(&exp);
					return err;
				}
				e->ops = exp;
				break;
			}
			default:
				return dba_error_consistency(
						"variable %01d%02d%03d cannot be handled",
							DBA_VAR_F(e->ops->val),
							DBA_VAR_X(e->ops->val),
							DBA_VAR_Y(e->ops->val));
		}
	}

	return dba_error_ok();

fail:
	return err;
}


/* vim:set ts=4 sw=4: */
