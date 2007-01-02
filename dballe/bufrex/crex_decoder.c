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
#include <dballe/core/rawfile.h>

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>	/* isspace */
#include <stdlib.h>	/* malloc */
#include <string.h>	/* memcpy */
#include <math.h>	/* NAN */
#include <assert.h>	/* NAN */
#include <errno.h>	/* NAN */

/*  #define TRACE_DECODER */

#ifdef TRACE_DECODER
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif

struct _decoder 
{
	/* Input message data */
	dba_rawmsg in;
	/* Output decoded variables */
	bufrex_msg out;
	/* Current subset we are decoding */
	bufrex_subset current_subset;

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
	/* Start of data not yet decoded */
	const unsigned char* cur;
};
typedef struct _decoder* decoder;

static dba_err decoder_create(decoder* res)
{
	if ((*res = (decoder)calloc(1, sizeof(struct _decoder))) == NULL)
		return dba_error_alloc("allocating a new crex decoder");

	return dba_error_ok();
}

static void decoder_delete(decoder res)
{
	if (res->ops != NULL)
		bufrex_opcode_delete(&(res->ops));
	free(res);
}

#if 0
dba_err crex_decoder_has_check_digit(crex_decoder msg, int* has_check_digit)
{
	*has_check_digit = msg->has_check_digit;
	return dba_error_ok();
}

dba_err crex_decoder_set_check_digit(crex_decoder msg, int has_check_digit)
{
	msg->has_check_digit = has_check_digit;
	return dba_error_ok();
}
#endif

/* Forward declaration of this static function, as we write recursive decoding
 * functions later */
static dba_err crex_decoder_parse_data_section(decoder d);

#define PARSE_ERROR(...) do { \
		err = dba_error_parse(d->in->file->name, d->in->offset + (d->cur - d->in->buf), __VA_ARGS__); \
		goto fail; \
	} while (0)

#define SKIP_SPACES() do { \
		for ( ; d->cur < (d->in->buf + d->in->len) && isspace(*d->cur); d->cur++) \
			; \
	} while (0)

#define SKIP_DATA_AND_SPACES(datalen) do { \
		for (d->cur += datalen; d->cur < (d->in->buf + d->in->len) && isspace(*d->cur); d->cur++) \
			; \
	} while (0)

#define CHECK_EOF(next) do { \
		TRACE(" - step %s -\n", next); \
		if (d->cur >= (d->in->buf + d->in->len)) \
			PARSE_ERROR("end of CREX message while looking for " next); \
	} while (0)

#define CHECK_AVAILABLE_DATA(datalen, next) do { \
		if (d->cur + datalen > (d->in->buf + d->in->len)) \
			PARSE_ERROR("end of CREX message while looking for " next); \
	} while (0)

dba_err crex_decoder_decode(dba_rawmsg in, bufrex_msg out)
{
	dba_err err;
	decoder d = NULL;
	int i;

	DBA_RUN_OR_RETURN(decoder_create(&d));
	d->in = in;
	d->out = out;
	d->cur = in->buf;
	
	/* Read crex section 0 (Indicator section) */
	CHECK_AVAILABLE_DATA(6, "initial header of CREX message");
	if (strncmp((const char*)d->cur, "CREX++", 6) != 0)
		PARSE_ERROR("data does not start with CREX header (\"%.6s\" was read instead)", d->in->buf);

	SKIP_DATA_AND_SPACES(6);
	TRACE(" -> is CREX\n");

	/* Read crex section 1 (Data description section) */

	CHECK_EOF("start of section 1");
	d->sec1_start = d->cur - d->in->buf;

	/* T<version> */
	if (*d->cur != 'T')
		PARSE_ERROR("version not found in CREX data description");

	{
		char edition[11];
		for (i = 0; i < 10 && d->cur < (d->in->buf + d->in->len) && !isspace(*d->cur); d->cur++, i++)
			edition[i] = *d->cur;
		edition[i] = 0;

		if (sscanf(edition, "T%02d%02d%02d",
					&(d->out->opt.crex.master_table),
					&(d->out->edition),
					&(d->out->opt.crex.table)) != 3)
			DBA_FAIL_GOTO(fail, dba_error_consistency("Edition (T%s) is not in format Ttteevv", edition+1));
		
		SKIP_SPACES();
		TRACE(" -> edition %d\n", strtol(edition + 1, 0, 10));
	}

	/* A<atable code> */
	CHECK_EOF("A code");
	if (*d->cur != 'A')
		PARSE_ERROR("A Table informations not found in CREX data description");
	{
		char atable[20];
		int val;
		for (i = 0, d->cur++; d->cur < (d->in->buf + d->in->len) && i < 10 && isdigit(*d->cur); d->cur++, i++)
			atable[i] = *d->cur;
		atable[i] = 0;
		val = strtol(atable, 0, 10);
		switch (strlen(atable))
		{
			case 3:
				d->out->type = val;
				d->out->subtype = 0;
				TRACE(" -> category %d\n", strtol(atable, 0, 10));
				break;
			case 6:
				d->out->type = val / 1000;
				d->out->subtype = val % 1000;
				TRACE(" -> category %d, subcategory %d\n", val / 1000, val % 1000);
				break;
			default:
				err = dba_error_consistency("Cannot parse an A table indicator %d digits long", strlen(atable));
				goto fail;
		}
	}
	SKIP_SPACES();

	/* Load tables and set category/subcategory */
	DBA_RUN_OR_GOTO(fail, bufrex_msg_load_tables(d->out));

	/* data descriptors followed by (E?)\+\+ */
	CHECK_EOF("data descriptor section");

	d->has_check_digit = 0;
	while (1)
	{
		if (*d->cur == 'B' || *d->cur == 'R' || *d->cur == 'C' || *d->cur == 'D')
		{
			dba_varcode var;
			CHECK_AVAILABLE_DATA(6, "one data descriptor");
			var = dba_descriptor_code((const char*)d->cur);
			DBA_RUN_OR_GOTO(fail, bufrex_msg_append_datadesc(d->out, var));
			SKIP_DATA_AND_SPACES(6);
		}
		else if (*d->cur == 'E')
		{
			d->has_check_digit = 1;
			d->expected_check_digit = 1;
			SKIP_DATA_AND_SPACES(1);
		}
		else if (*d->cur == '+')
		{
			CHECK_AVAILABLE_DATA(1, "end of data descriptor section");
			if (*(d->cur+1) != '+')
				PARSE_ERROR("data section ends with only one '+'");
			SKIP_DATA_AND_SPACES(2);
			break;
		}
	}
	IFTRACE{
		bufrex_opcode ops = NULL;
		TRACE(" -> data descriptor section: ");
		DBA_RUN_OR_GOTO(fail, bufrex_msg_get_datadesc(d->out, &ops));
		bufrex_opcode_print(ops, stderr);
		bufrex_opcode_delete(&ops);
		TRACE("\n");
	}

	/* Decode crex section 2 */
	CHECK_EOF("data section");
	d->sec2_start = d->cur - d->in->buf;
	for (i = 0; ; ++i)
	{
		DBA_RUN_OR_GOTO(fail, bufrex_msg_get_subset(d->out, i, &(d->current_subset)));
		DBA_RUN_OR_GOTO(fail, bufrex_msg_get_datadesc(d->out, &(d->ops)));
		DBA_RUN_OR_GOTO(fail, crex_decoder_parse_data_section(d));
		SKIP_SPACES();
		CHECK_EOF("end of data section");

		if (*d->cur != '+')
			PARSE_ERROR("there should be a '+' at the end of the data section");
		d->cur++;

		/* Peek at the next character to see if it's end of section */
		CHECK_EOF("end of data section");
		if (*d->cur == '+')
		{
			d->cur++;
			break;
		}
	}
	SKIP_SPACES();

	/* Decode crex optional section 3 */
	CHECK_AVAILABLE_DATA(4, "CREX optional section 3 or end of CREX message");
	d->sec3_start = d->cur - d->in->buf;
	if (strncmp((const char*)d->cur, "SUPP", 4) == 0)
	{
		for (d->cur += 4; strncmp((const char*)d->cur, "++", 2) != 0; d->cur++)
			CHECK_AVAILABLE_DATA(2, "end of CREX optional section 3");
		SKIP_SPACES();
	}

	/* Decode crex end section 4 */
	CHECK_AVAILABLE_DATA(4, "end of CREX message");
	d->sec4_start = d->cur - d->in->buf;
	if (strncmp((const char*)d->cur, "7777", 4) != 0)
		PARSE_ERROR("unexpected data after data section or optional section 3");

	if (d != NULL)
		decoder_delete(d);
	return dba_error_ok();

fail:
	TRACE(" -> crex_decoder_parse failed.\n");

	if (d != NULL)
		decoder_delete(d);
	return err;
}



static dba_err crex_decoder_parse_value(
			decoder d,
			int len,
			int is_signed,
			const unsigned char** d_start,
			const unsigned char** d_end)
{
	dba_err err;

	TRACE("crex_decoder_parse_value(%d, %s): ", len, is_signed ? "signed" : "unsigned");

	/* Check for 2 more because we may have extra sign and check digit */
	CHECK_AVAILABLE_DATA(len + 2, "end of data descriptor section");

	if (d->has_check_digit)
	{
		if ((*d->cur - '0') != d->expected_check_digit)
			PARSE_ERROR("check digit mismatch: expected %d, found %d, rest of message: %.*s",
					d->expected_check_digit,
					(*d->cur - '0'),
					(d->in->buf + d->in->len) - d->cur,
					d->cur);

		d->expected_check_digit = (d->expected_check_digit + 1) % 10;
		d->cur++;
	}

	/* Set the value to start after the check digit (if present) */
	*d_start = d->cur;

	/* Cope with one extra character in case the sign is present */
	if (is_signed && *d->cur == '-')
		len++;

	/* Go to the end of the message */
	d->cur += len;

	/* Set the end value, removing trailing spaces */
	for (*d_end = d->cur; *d_end > *d_start && isspace(*(*d_end - 1)); (*d_end)--)
		;
	
	/* Skip trailing spaces */
	SKIP_SPACES();

	TRACE("%.*s\n", *d_end - *d_start, *d_start);

	return dba_error_ok();

fail:
	return err;
}

#if 0
/**
 * Compute a value from a CREX message
 *
 * @param value
 *   The value as found in the CREX message
 *
 * @param info
 *   The B table informations for the value
 *
 * @param cmodifier
 *   The C table modifier in effect for this value, or NULL if no C table
 *   modifier is in effect
 *
 * @returns
 *   The decoded value
 */
/* TODO: implement c modifier computation */
static double crex_decoder_compute_value(bufrex_decoder decoder, const char* value, dba_varinfo* info)
{
	double val;

	/* TODO use the C table values */
	
	if (value[0] == '/')
		return NAN;
 
	val = strtol(value, NULL, 10);

	if (info->scale != 0)
	{
		int scale = info->scale;

		if (info->scale > 0)
			while (scale--)
				val /= 10;
		else
			while (scale++)
				val *= 10;
	}

	3A
	return val;
}
#endif

static dba_err crex_decoder_parse_b_data(decoder d)
{
	dba_err err;
	dba_var var = NULL;
	dba_varinfo crexinfo = NULL;
	const unsigned char* d_start;
	const unsigned char* d_end;
	char* buf = NULL;

	IFTRACE{
		TRACE("crex_decoder_parse_b_data: items: ");
		bufrex_opcode_print(d->ops, stderr);
		TRACE("\n");
	}

	DBA_RUN_OR_RETURN(bufrex_msg_query_btable(d->out, d->ops->val, &crexinfo));

	/* Create the new dba_var */
	DBA_RUN_OR_GOTO(fail, dba_var_create(crexinfo, &var));

	/* Parse value from the data section */
	DBA_RUN_OR_GOTO(fail, crex_decoder_parse_value(d, crexinfo->len, !crexinfo->is_string, &d_start, &d_end));

	/* If the variable is not missing, set its value */
	if (*d_start != '/')
	{
		if (crexinfo->is_string)
		{
			const int len = d_end - d_start;
			char* buf = (char*)malloc(len + 1);
			memcpy(buf, d_start, len);
			buf[len] = 0;
			DBA_RUN_OR_GOTO(fail, dba_var_setc(var, buf));
			free(buf);
			buf = NULL;
		} else {
			int val = strtol((const char*)d_start, 0, 10);

			/* FIXME: here goes handling of active C table modifiers */

			DBA_RUN_OR_GOTO(fail, dba_var_seti(var, val));
		}
	}

	/* Store the variable that we found */
	DBA_RUN_OR_GOTO(fail, bufrex_subset_store_variable(d->current_subset, var));
	IFTRACE{
		TRACE("  stored variable: "); dba_var_print(var, stderr); TRACE("\n");
	}

	/* Remove from the chain the item that we handled */
	{
		bufrex_opcode op;
		DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(d->ops), &op));
		bufrex_opcode_delete(&op);
	}

#ifdef TRACE_DECODER
	{
		int left = (d->in->buf + d->in->len) - d->cur;
		TRACE("crex_decoder_parse_b_data -> %.*s (items:", left > 30 ? 30 : left, d->cur);
		bufrex_opcode_print(d->ops, stderr);
		TRACE(")\n");
	}
#endif

	return dba_error_ok();

fail:
	if (buf != NULL)
		free(buf);
	if (var != NULL)
		dba_var_delete(var);
	return err;
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

static dba_err crex_decoder_parse_r_data(decoder d)
{
	dba_err err;
	int group = DBA_VAR_X(d->ops->val);
	int count = DBA_VAR_Y(d->ops->val);
	bufrex_opcode rep_op;
	
	TRACE("R DATA %01d%02d%03d %d %d", 
			DBA_VAR_F(d->ops->val), DBA_VAR_X(d->ops->val), DBA_VAR_Y(d->ops->val), group, count);

	/* Pop the R repetition node, since we have read its value in group and count */
	{
		bufrex_opcode op;
		DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(d->ops), &op));
		bufrex_opcode_delete(&op);
	}

	/* Pop the first `group' nodes, since we handle them here */
	DBA_RUN_OR_RETURN(bufrex_opcode_pop_n(&(d->ops), &rep_op, group));

	if (count == 0)
	{
		/* Delayed replication */
		const unsigned char* d_start;
		const unsigned char* d_end;
		dba_varinfo info;
		dba_var var;
		
		/* Fetch the repetition count */
		DBA_RUN_OR_GOTO(fail, crex_decoder_parse_value(d, 4, 0, &d_start, &d_end));
		count = strtol((const char*)d_start, NULL, 10);

		/* Insert the repetition count among the parsed variables */
		DBA_RUN_OR_GOTO(fail, bufrex_msg_query_btable(d->out, DBA_VAR(0, 31, 1), &info));
		DBA_RUN_OR_GOTO(fail, dba_var_createi(info, count, &var));
		DBA_RUN_OR_GOTO(fail, bufrex_subset_store_variable(d->current_subset, var));

		TRACE("read_c_data %d items %d times (delayed)\n", group, count);
	} else
		TRACE("read_c_data %d items %d times\n", group, count);


	/* Perform replication */
	while (count--)
	{
		/* fprintf(stderr, "Debug: rep %d left\n", count); */
		bufrex_opcode chain = NULL;
		bufrex_opcode saved = NULL;
		DBA_RUN_OR_GOTO(fail, bufrex_opcode_prepend(&chain, rep_op));

		IFTRACE{
			TRACE("read_c_data %d items %d times left; items: ", group, count);
			bufrex_opcode_print(chain, stderr);
			TRACE("\n");
		}

		saved = d->ops;
		d->ops = chain;
		if ((err = crex_decoder_parse_data_section(d)) != DBA_OK)
		{
			if (d->ops != NULL)
				bufrex_opcode_delete(&(d->ops));
			d->ops = saved;
			goto fail;
		}
		/* chain should always be NULL when parsing succeeds */
		assert(d->ops == NULL);
		d->ops = saved;
	}

	bufrex_opcode_delete(&rep_op);
	return dba_error_ok();

fail:
	bufrex_opcode_delete(&rep_op);
	return err;
}

/**
 *
 */
static dba_err crex_decoder_parse_data_section(decoder d)
{
	dba_err err;

	/*
	fprintf(stderr, "read_data: ");
	bufrex_opcode_print(ops, stderr);
	fprintf(stderr, "\n");
	*/
	TRACE("crex_decoder_parse_data_section: START\n");

	while (d->ops != NULL)
	{
		IFTRACE{
			TRACE("crex_decoder_parse_data_section TODO: ");
			bufrex_opcode_print(d->ops, stderr);
			TRACE("\n");
		}

		switch (DBA_VAR_F(d->ops->val))
		{
			case 0:
				DBA_RUN_OR_RETURN(crex_decoder_parse_b_data(d));
				break;
			case 1:
				DBA_RUN_OR_RETURN(crex_decoder_parse_r_data(d));
				break;
			case 2:
				PARSE_ERROR("C modifiers are not yet supported");
				/* DBA_RUN_OR_RETURN(crex_decoder_parse_c_data(d)); */
				break;
			case 3:
			{
				/* D table opcode: expand the chain */
				bufrex_opcode op;
				bufrex_opcode exp;

				/* Pop the first opcode */
				DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(d->ops), &op));
				
				if ((err = bufrex_msg_query_dtable(d->out, op->val, &exp)) != DBA_OK)
				{
					bufrex_opcode_delete(&op);
					return err;
				}

				/* Push the expansion back in the fields */
				if ((err = bufrex_opcode_join(&exp, d->ops)) != DBA_OK)
				{
					bufrex_opcode_delete(&op);
					bufrex_opcode_delete(&exp);
					return err;
				}
				d->ops = exp;
				break;
			}
			default:
				PARSE_ERROR("cannot handle field %01d%02d%03d",
							DBA_VAR_F(d->ops->val),
							DBA_VAR_X(d->ops->val),
							DBA_VAR_Y(d->ops->val));
		}
	}

	return dba_error_ok();

fail:
	return err;
}

/* vim:set ts=4 sw=4: */
